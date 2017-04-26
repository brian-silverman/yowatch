/*
 * YoWatch main
 *
 * The YoWatch smartwatch runs on the PSoC 4 BLE (specifically the
 * CY8C4248LQI-BL583).  The watch is intended to be paired with a Android phone
 * (appropriately called YoPhone) - without one, it is virtually useless.  The
 * watch has 3 primary functions:
 *      1) Tell time - duh!
 *      2) Display notifications received in YoPhone (email, texts, etc)
 *      3) Fast answers - Alexa style response to questions
 *
 * As such, #3 is the bulk of the work in this app.  To support this, the
 * hardware contains:
 *      - A capsense proximity sensor
 *      - A I2S Microphone
 *      - A Serial RAM for audio buffering
 *      - A BLE antenna (and BLE subsystem supported by the PSoC)
 *      - An OLED display
 *
 * So, the normal use case to support #3 is:
 *      - Proximity sensor detect when watch is near user's face
 *      - Microphone begins recording audio, and buffering it
 *      - Audio is sent via BLE to YoPhone
 *      - YoPhone uses Speech-to-text cloud service
 *          - Cloud converts audio to text
 *          - YoPhone sends back partial text to YoWatch
 *          - YoWatch displays partial text when it receives it
 *      - YoPhone uses final text to run specific command.  
 *          - In general, the command does a web search for some data, parses
 *          the data, and sends the data back to YoWatch
 *          - YoPhone may use a combination of cloud services, like Google.
 *          - YoWatch displays the final data
 *      - Example:
 *          - User says "how many square miles in maryand"
 *              - YoWatch displays: "12,407 mi²"
 *          - User says "What is the capital of vermont"
 *              - YoWatch displays: "Montpelier"
 *          - User says "who played deckard in blade runner"
 *              - YoWatch displays: "Harrison Ford"
 *          - User says "text jason lets do lunch"
 *              - YoWatch sends a text to Jason
 *      - YoWatch is dumb, in that it doesn't understand the commands.  It just
 *      sends audio and receives text to display.  All the command types are
 *      handled by YoPhone.
 *
 * The flow of audio is:
 *
 *           DMA
 *  I2S Mic -----> buf1
 *                       DMA               DMA
 *                 buf2 -----> Serial RAM -----> buf -> compress -> BLE
 *
 *           (ping pong bufs)
 *
 * Copyright (C) 2017 Brian Silverman <bri@readysetstem.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 */
#include <project.h>
#include <BLEApplications.h>
#include <errno.h>
#include <stdio.h>

//
// Speedtest buffer
// TODO: merge with I2S DMA buffers
//
uint8 buffer[MAX_MTU_SIZE+4];
int speedTestPackets;

int deviceConnected = 0;

//
// Audio DMA buffers
//
// Bufs are oversized by 4 bytes, because when the buffer are sent to the SPI
// Serial RAM buffer, the received buffer included the 4 byte SPI Serial RAM
// address.
//
#define BUFSIZE (0x100)
uint8 buf1[BUFSIZE + 4];
uint8 buf2[BUFSIZE + 4];
int stopI2sDma = 1;

//
// SPI bus used for:
//      - Serial RAM writes (requires TX DMA)
//      - Serial RAM reads (requires TX and RX DMA)
//      - LCD writes (requires TX DMA)
// These variables keep track of the current SPI state - only valid during the
// locked period of a SPI transaction (LockSpiDma()/UnlockSpiDma())
//
// Usage of the SPI bus must be locked.  When a SPI DMA transaction can not be
// completed because the bus is locked, the transaction is queued, and
// performed immediately after the current transaction finishes.
//
// The bus must not be overutilized for a period greater than the queue allows.
// If it is, the pending transaction will overfill, and bad things will ensue
// (assertion).
//
enum SPI_MODE {
    SPI_MODE_NONE,
    SPI_MODE_ENQUEUE,
    SPI_MODE_DEQUEUE,
    SPI_MODE_LCD,
} spiTxMode, spiRxMode;

int spiDmaLocked = 0;

//
// SPI Serial RAM buffer queue (circular)
//
// Queue starts with a count of QUEUE_SIZE bytes free.  used + free + pending =
// QUEUE_SIZE.  Queue starts with head = tail = 0, and bytes are added to tail,
// which increments tail.  Pending bytes are added/removed from queue when DMA
// completes.
//
#define QUEUE_SIZE (128 * 1024)
struct {
    uint32 pendingTail;
    uint32 pendingTailBytes;
    uint32 pendingHead;
    uint32 pendingHeadBytes;
    uint32 tail;
    uint32 head;
    uint32 used;
    uint32 free;
} bufq;

//
// Circular buffer of queue descriptors.  To simplify queueing, one dummy
// descriptor is between head and tail.  head == tail means empty,
// (tail + 1 % NUM) == head means full.
//
#define NUM_QDS 8

struct QUEUED_DESCRIPTOR {
    void (*call)();
    uint8 * buf;
    uint32 bytes;
    int * done;
};

struct QUEUED_DESCRIPTORS {
    struct QUEUED_DESCRIPTOR queue[NUM_QDS];
    uint32 head;
    uint32 tail;
} queuedDescriptors;

// Freq of Timer_Programmable_* input clock
#define TIMER_CLOCK_FREQ_MHZ 1

//
// For debug testing
//
enum CONCURRENCY_TEST_TYPE {
    CONCURRENCY_TEST_NORMAL,
    CONCURRENCY_TEST_FAST_DEQ,
    CONCURRENCY_TEST_FAST_ENQ,
    CONCURRENCY_TEST_MAX,
};

// count for debug testing
int enqCount;

//
// For debug, enqueue/dequeue test periods, for each CONCURRENCY_TEST_TYPE.
//
#define APPROX_BUF_WRITE_PERIOD_US 1000
uint32 enqPeriodUs[CONCURRENCY_TEST_MAX] = {
    (2+1)*APPROX_BUF_WRITE_PERIOD_US,
    (5+1)*APPROX_BUF_WRITE_PERIOD_US,
    (2+1)*APPROX_BUF_WRITE_PERIOD_US
    };
uint32 deqPeriodUs[CONCURRENCY_TEST_MAX] = {
    2*APPROX_BUF_WRITE_PERIOD_US,
    2*APPROX_BUF_WRITE_PERIOD_US,
    5*APPROX_BUF_WRITE_PERIOD_US
    };

//
// Main state machine STATEs
//
enum STATE {
    NONE,
    OFF,
    SLEEP,
    TIME,
    VOICE,
    NOTES,
    NOTE_VIEW,
    RUN_CMD,
    RESULT,
    CUSTOM_CMD,
    DEBUG_IDLE,
    DEBUG_SPEEDTEST,
    DEBUG_SPEEDTEST_2,
    DEBUG_MIC,
    DEBUG_MIC_2,
    DEBUG_MIC_3,
    DEBUG_MIC_4,
    DEBUG_MEMTEST_FILL,
    DEBUG_MEMTEST_FILL_2,
    DEBUG_MEMTEST_FILL_3,
    DEBUG_MEMTEST_FILL_4,
    DEBUG_MEMTEST_DEPLETE,
    DEBUG_MEMTEST_DEPLETE_2,
    DEBUG_MEMTEST_DEPLETE_3,
    DEBUG_MEMTEST_CONCURRENCY,
    DEBUG_MEMTEST_CONCURRENCY_2,
    DEBUG_MEMTEST_CONCURRENCY_3,
    DEBUG_MEMTEST_CONCURRENCY_4,
    DEBUG_MEMTEST_CONCURRENCY_5,
    DEBUG_MEMTEST_CONCURRENCY_6,
} state, bleSelectedNextState;

void PrintState(
    enum STATE state
    )
{
    #define PRINT_CASE(state) case state: UART_UartPutString("State: " #state "\r\n"); break
    #define NO_PRINT_CASE(state) case state: break

    switch (state) {
        PRINT_CASE(NONE);
        PRINT_CASE(OFF);
        PRINT_CASE(SLEEP);
        PRINT_CASE(TIME);
        PRINT_CASE(VOICE);
        PRINT_CASE(NOTES);
        PRINT_CASE(NOTE_VIEW);
        PRINT_CASE(RUN_CMD);
        PRINT_CASE(RESULT);
        PRINT_CASE(CUSTOM_CMD);
        PRINT_CASE(DEBUG_IDLE);
        PRINT_CASE(DEBUG_SPEEDTEST);
        PRINT_CASE(DEBUG_SPEEDTEST_2);
        PRINT_CASE(DEBUG_MIC);
        NO_PRINT_CASE(DEBUG_MIC_2);
        NO_PRINT_CASE(DEBUG_MIC_3);
        NO_PRINT_CASE(DEBUG_MIC_4);
        PRINT_CASE(DEBUG_MEMTEST_FILL);
        NO_PRINT_CASE(DEBUG_MEMTEST_FILL_2);
        NO_PRINT_CASE(DEBUG_MEMTEST_FILL_3);
        PRINT_CASE(DEBUG_MEMTEST_FILL_4);
        NO_PRINT_CASE(DEBUG_MEMTEST_DEPLETE);
        NO_PRINT_CASE(DEBUG_MEMTEST_DEPLETE_2);
        PRINT_CASE(DEBUG_MEMTEST_DEPLETE_3);
        PRINT_CASE(DEBUG_MEMTEST_CONCURRENCY);
        NO_PRINT_CASE(DEBUG_MEMTEST_CONCURRENCY_2);
        NO_PRINT_CASE(DEBUG_MEMTEST_CONCURRENCY_3);
        PRINT_CASE(DEBUG_MEMTEST_CONCURRENCY_4);
        NO_PRINT_CASE(DEBUG_MEMTEST_CONCURRENCY_5);
        NO_PRINT_CASE(DEBUG_MEMTEST_CONCURRENCY_6);
        default:
            UART_UartPutString("State: UNKNOWN!\r\n");
            break;
    }
}

//
// assert() function
// TODO: send assert to LCD display
//
#define assert(x) _assert(x, #x, __LINE__, __FILE__)
void _assert(int assertion, char * assertionStr, int line, char * file)
{
    char s[100];
    if (! assertion) {
        snprintf(s, sizeof(s), "ASSERT(%s) at line %d, %s.\r\n", assertionStr, line, file);
        UART_UartPutString(s);
        CyHalt(0);
    }
}

//
// BLE callback when debug command received
//
CYBLE_API_RESULT_T OnDebugCommandCharacteristic(
    CYBLE_GATT_DB_ATTR_HANDLE_T handle,
    uint8 * data,
    int len
    )
{
    char s[32];
    sprintf(s, "%d, %d, %d\r\n", *data, len, state);
    UART_UartPutString(s);

    // TODO Fix hardcoded constants
    switch (*data) {
        case 0: // Exit debug mode
            bleSelectedNextState = SLEEP;
            break;
        case 1: // Go to debug
            if (state == SLEEP) {
                bleSelectedNextState = DEBUG_IDLE;
            }
            UART_UartPutString("Go to DEBUG_IDLE\r\n");
            PrintState(state);
            break;
        case 2: // Go to debug speedtest
            speedTestPackets = 100; // TODO need correct # packets
            if (state == DEBUG_IDLE) {
                bleSelectedNextState = DEBUG_SPEEDTEST;
            }
            break;
        case 3:
            if (state == DEBUG_IDLE) {
                bleSelectedNextState = DEBUG_MIC;
            }
            break;
        default:
            bleSelectedNextState = NONE;
            break;
    }

    return 0;
}

//
// BLE callback when connection changed
// TODO handle state change (in main state machine) when device disconnects
//
void OnConnectionChange(
    int connected
    )
{
    uint8 c = 0;
    BleWriteCharacteristic(CYBLE_SMARTWATCH_SERVICE_VOICE_DATA_CHAR_HANDLE, &c, sizeof(c));
    deviceConnected = connected;
}

//
// Lock the SPI bus to perform an atomic SPI DMA transaction.
//
// If lock is unavailable, transaction /qd/ is queued, and will performed (in
// FIFO order) once the current SPI DMA transaction completes.
//
// /qd/ must be valid.
//
// Returns true if the bus was idle (and is now locked).  Returns false if the
// bus was already locked - the given /qd/ is now queued.
//
int LockSpiDma(struct QUEUED_DESCRIPTOR * qd)
{
    int wasLocked;
    uint8 interruptState;

    interruptState = CyEnterCriticalSection();

    // Atomic test-and-set of lock variable (spiDmaLocked)
    wasLocked = spiDmaLocked;
    spiDmaLocked = 1;

    // Queue descriptor if DMA was locked
    if (wasLocked) {
        assert(((queuedDescriptors.tail + 1) % NUM_QDS) != queuedDescriptors.head);
        queuedDescriptors.queue[queuedDescriptors.tail] = *qd;
        queuedDescriptors.tail = (queuedDescriptors.tail + 1) % NUM_QDS;
    }

    CyExitCriticalSection(interruptState);

    return wasLocked;
}

//
// Unlocks SPI bus from a previous LockSpiDma() call.
//
// If previous transactions were queued, causes next transaction to be started
// (and keeps bus locked).
//
void UnlockSpiDma()
{
    uint8 interruptState;
    struct QUEUED_DESCRIPTOR qd;
    struct QUEUED_DESCRIPTOR * pqd = NULL;

    interruptState = CyEnterCriticalSection();

    assert(spiDmaLocked);

    // If queue is not empty, dequeue descriptor
    if (queuedDescriptors.head != queuedDescriptors.tail) {
        qd = queuedDescriptors.queue[queuedDescriptors.head];
        pqd = &qd;
        queuedDescriptors.head = (queuedDescriptors.head + 1) % NUM_QDS;
        // DMA stays locked
    } else {
        pqd = NULL;
        spiDmaLocked = 0;
    }

    CyExitCriticalSection(interruptState);

    // If descriptor was dequeued, run it
    if (pqd) {
        pqd->call(pqd->buf, pqd->bytes, pqd->done);
    }
}

//
// SPI TX DMA ISR - data sent, update queue.
//
// Second spurious ISR may (always?) occur - code handles being called multiple
// times in a row safely, by setting mode to SPI_MODE_NONE.
//
// spiTxDmaDone is a pointer to a flag - it must have been configured and
// zeroed at the start of the DMA transaction, and it is set to 1 now to signal
// completion.
//
int * spiTxDmaDone;
void SpiTxDmaIsr(void)
{
    enum SPI_MODE mode;
    mode = spiTxMode;
    spiTxMode = SPI_MODE_NONE;
    switch (mode) {
        case SPI_MODE_ENQUEUE:
            bufq.tail = bufq.pendingTail;
            bufq.used += bufq.pendingTailBytes;
            bufq.pendingTailBytes = 0;
            *spiTxDmaDone = 1;
            UnlockSpiDma();
            return;
        case SPI_MODE_LCD:
            return;
        default:
            return;
    }
}

//
// SPI TX DMA ISR - data received, update queue.
//
// See SpiTxDmaIsr() for more info.
//
int * spiRxDmaDone;
void SpiRxDmaIsr(void)
{
    enum SPI_MODE mode;
    mode = spiRxMode;
    spiRxMode = SPI_MODE_NONE;
    switch (mode) {
        case SPI_MODE_DEQUEUE:
            bufq.head = bufq.pendingHead;
            bufq.free += bufq.pendingHeadBytes;
            bufq.pendingHeadBytes = 0;
            *spiRxDmaDone = 1;
            UnlockSpiDma();
            return;
        default:
            return;
    }
}

//
// Init /bufq/
//
void BufQueueInit()
{
    bufq.pendingTail = 0;
    bufq.pendingHead = 0;
    bufq.tail = 0;
    bufq.head = 0;
    bufq.used = 0;
    bufq.free = QUEUE_SIZE;
}

//
// Enqueue given buffer to SPI Serial RAM via DMA
//
// The SPI DMA is serialized - only one DMA can run at a time.  If the SPI
// bus is running, the operation is added to a pending SPI bus queue, and it
// will be run when the current operation compeletes.
//
// The buffer is added to the queue tail, but the tail will only be updated
// once the DMA completes.
//
// @param buf       buffer to be enqueued to Serial RAM
// @param bytes     size in bytes of /buf/
// @param done      flag that will be set when DMA completes
//
// @return 0 on success, or negative value on error.  EAGAIN indicates call has
// been queued, will be recalled when running DMA is complete.
//
int enqueueDone;
int EnqueueBytesNoLock(uint8 *buf, uint32 bytes, int * done);
int EnqueueBytes(uint8 *buf, uint32 bytes, int * done)
{
    struct QUEUED_DESCRIPTOR qd;

    if (bufq.free < bytes) return -ENOSPC;

    if (done != NULL) {
        *done = 0;
    }
    qd.call = (void (*)()) &EnqueueBytesNoLock;
    qd.buf = buf;
    qd.bytes = bytes;
    qd.done = done;
    if (LockSpiDma(&qd)) return -EAGAIN;

    return EnqueueBytesNoLock(buf, bytes, done);
}

int EnqueueBytesNoLock(uint8 *buf, uint32 bytes, int * done)
{
    uint32 tail;
    uint8 interruptState;

    // Back to back DMAs can clobber bits at the end of the first DMA.
    CyDelayUs(5);

    interruptState = CyEnterCriticalSection();

    assert(bufq.free >= bytes);
    assert(bufq.tail == bufq.pendingTail);
    tail = bufq.tail;
    bufq.pendingTail = (bufq.tail + bytes) % QUEUE_SIZE;
    assert(bufq.free >= bytes);
    bufq.free -= bytes;
    bufq.pendingTailBytes = bytes;

    CyExitCriticalSection(interruptState);

    if (done == NULL) {
        done = &enqueueDone;
    }
    *done = 0;
    spiTxDmaDone = done;

    SpiTxDma_SetInterruptCallback(&SpiTxDmaIsr);
    SpiTxDma_SetSrcAddress(0, buf);
    SpiTxDma_SetDstAddress(0, (void *) SPI_1_TX_FIFO_WR_PTR);
    SpiTxDma_SetNumDataElements(0, bytes);
    SpiTxDma_ValidateDescriptor(0);

    SPI_1_SpiUartClearRxBuffer();
    SPI_1_SpiUartClearTxBuffer();

    //
    // Send read command/address, and start DMAs
    //
    // Must disable interrupts, as the initial TX FIFO bytes must be filled and
    // DMA started before those bytes are sent, or there will be a gap in the
    // SPI transaction.
    //
    interruptState = CyEnterCriticalSection();
    SPI_1_SpiUartWriteTxData(0x02);
    SPI_1_SpiUartWriteTxData((tail >> 16) & 0xFF);
    SPI_1_SpiUartWriteTxData((tail >> 8) & 0xFF);
    SPI_1_SpiUartWriteTxData((tail >> 0) & 0xFF);
    spiTxMode = SPI_MODE_ENQUEUE;
    SpiTxDma_ChEnable();
    CyExitCriticalSection(interruptState);

    return 0;
}

//
// Dequeue given buffer from SPI Serial RAM via DMA
//
// The SPI DMA is serialized - only one DMA can run at a time.  If the SPI
// bus is running, the operation is added to a pending SPI bus queue, and it
// will be run when the current operation compeletes.
//
// The buffer is removed from the queue head, but the head will only be updated
// once the DMA completes.
//
// @param buf       buffer to be dequeued from Serial RAM.
//                  The given /buf/ must be 4 bytes longer than the /bytes/
//                  specified.  A SPI command/address will be filled in as the
//                  first 4 bytes of the returned /buf/.
// @param bytes     size in bytes of /buf/
// @param done      flag that will be set when DMA completes
//
// @return 0 on success, or negative value on error.  EAGAIN indicates call has
// been queued, will be recalled when running DMA is complete.
//
uint8 allff[BUFSIZE + 4];
int dequeueDone;
int DequeueBytesNoLock(uint8 *buf, int bytes, int * done);
int DequeueBytes(uint8 *buf, int bytes, int * done)
{
    struct QUEUED_DESCRIPTOR qd;

    if (bufq.used < bytes) return -ENODATA;

    if (done != NULL) {
        *done = 0;
    }
    qd.call = (void (*)()) &DequeueBytesNoLock;
    qd.buf = buf;
    qd.bytes = bytes;
    qd.done = done;
    if (LockSpiDma(&qd)) return -EAGAIN;

    return DequeueBytesNoLock(buf, bytes, done);
}

int DequeueBytesNoLock(uint8 *buf, int bytes, int * done)
{
    uint32 head;
    uint8 interruptState;

    // Back to back DMAs can clobber bits at the end of the first DMA.
    CyDelayUs(5);

    interruptState = CyEnterCriticalSection();

    assert(bufq.used >= bytes);
    assert(bufq.head == bufq.pendingHead);
    head = bufq.head;
    bufq.pendingHead = (bufq.head + bytes) % QUEUE_SIZE;
    assert(bufq.used >= bytes);
    bufq.used -= bytes;
    bufq.pendingHeadBytes = bytes;

    CyExitCriticalSection(interruptState);

    if (done == NULL) {
        done = &dequeueDone;
    }
    *done = 0;
    spiRxDmaDone = done;

    // TODO - need to write from FF buf without source increment
    memset(allff, 0xFF, sizeof(allff));

    spiTxMode = SPI_MODE_DEQUEUE;
    SpiTxDma_SetInterruptCallback(&SpiTxDmaIsr);
    SpiTxDma_SetSrcAddress(0, allff);
    SpiTxDma_SetDstAddress(0, (void *) SPI_1_TX_FIFO_WR_PTR);
    SpiTxDma_SetNumDataElements(0, bytes);
    SpiTxDma_ValidateDescriptor(0);

    SpiRxDma_SetInterruptCallback(&SpiRxDmaIsr);
    SpiRxDma_SetSrcAddress(0, (void *) SPI_1_RX_FIFO_RD_PTR);
    SpiRxDma_SetDstAddress(0, buf);
    SpiRxDma_SetNumDataElements(0, bytes + 4); // extra four for command/address
    SpiRxDma_ValidateDescriptor(0);

    SPI_1_SpiUartClearRxBuffer();
    SPI_1_SpiUartClearTxBuffer();

    //
    // Send write command/address, and start DMAs
    //
    // Must disable interrupts, as the initial TX FIFO bytes must be filled and
    // DMA started before those bytes are sent, or there will be a gap in the
    // SPI transaction.
    //
    interruptState = CyEnterCriticalSection();
    SPI_1_SpiUartWriteTxData(0x03);
    SPI_1_SpiUartWriteTxData((head >> 16) & 0xFF);
    SPI_1_SpiUartWriteTxData((head >> 8) & 0xFF);
    SPI_1_SpiUartWriteTxData((head >> 0) & 0xFF);
    spiRxMode = SPI_MODE_DEQUEUE;
    SpiRxDma_ChEnable();
    spiTxMode = SPI_MODE_DEQUEUE;
    SpiTxDma_ChEnable();
    CyExitCriticalSection(interruptState);

    return 0;
}

//
// For DEBUG_MEMTEST_CONCURRENCY testing, periodic enqueuing of bytes
CY_ISR(DebugMemtestIsr)
{
    int i;

    Timer_Programmable_ClearInterrupt(Timer_Programmable_GetInterruptSource());
    for (i = 0; i < BUFSIZE/sizeof(uint32); i++) {
        ((uint32 *) buf1)[i] = enqCount++;
    }
    int ret = EnqueueBytes(buf1, BUFSIZE, NULL);
    // If didn't queue, rollback counter
    if (ret == -ENOSPC) {
        enqCount -= BUFSIZE/sizeof(uint32);
    }
}

//
// I2S DMA complete ISR.
//
// When I2S DMA from Mic to RAM buf completes, enqueue the buffer to SPI Serial
// RAM, and start a new I2S DMA on the ping pong buf.
//
uint8 * I2sStartDma();
void I2sRxDmaIsr(void)
{
    if (stopI2sDma) {
        I2S_1_DisableRx();
    } else {
        uint8 * buf = I2sStartDma();

        // Enqueue audio buf to Serial RAM (don't care when it finishes)
        int ret = EnqueueBytes(buf, BUFSIZE, NULL);
        if (ret == -ENOSPC) {
            // Handle error?
        }

    }
}

//
// Start an I2S Mic DMA transaction to the given /buf/.
//
// As the completion ISR starts another DMA transaction when this one
// completes, the sequenece of DMAs will run forever until stopped
// (via I2sStopDma())
//
uint8 * I2sStartDma()
{
    static uint8 * pCurrentBuf = buf1;
    uint8 * pPrevBuf;
    pPrevBuf = pCurrentBuf;
    pCurrentBuf = pCurrentBuf == buf1 ? buf2 : buf1;
    if (stopI2sDma) {
        stopI2sDma = 0;
        I2S_1_ClearRxFIFO();
        I2S_1_EnableRx();
    }
    I2sRxDma_SetInterruptCallback(&I2sRxDmaIsr);
    I2sRxDma_SetSrcAddress(0, (void *) I2S_1_RX_CH0_F0_PTR);
    I2sRxDma_SetDstAddress(0, pCurrentBuf);
    I2sRxDma_SetNumDataElements(0, BUFSIZE);
    I2sRxDma_ValidateDescriptor(0);
    I2sRxDma_ChEnable();
    return pPrevBuf;
}

//
// Stop an I2S DMA transaction.  See I2sStartDma().
//
void I2sStopDma()
{
    stopI2sDma = 1;
}

enum STATE GetBleStateIfNew(enum STATE state)
{
    if (bleSelectedNextState != NONE) {
        state = bleSelectedNextState;
    }
    bleSelectedNextState = NONE;
    return state;
}

//
// Main entry point:
//      - Init modules
//      - Run state machine forever
//
int main()
{
    uint32 n = 0;
    int ret;
    enum STATE newState, prevState;
    int i;
    char s[32];
    enum CONCURRENCY_TEST_TYPE concurrencyTestType;
    int deqCount;
    int done;
    int micBytes;

    prevState = OFF;
    state = SLEEP;

    CyDmaEnable();
    CyIntEnable(CYDMA_INTR_NUMBER);

    CyGlobalIntEnable;

    I2sRxDma_Init();
    SpiTxDma_Init();
    SpiRxDma_Init();

    BleStart();
    BleRegisterWriteCallback(
        OnDebugCommandCharacteristic,
        CYBLE_SMARTWATCH_SERVICE_SERVICE_INDEX,
        CYBLE_SMARTWATCH_SERVICE_DEBUG_COMMAND_CHAR_INDEX
        );

    I2S_1_Start();
    Timer_1_Start();
    SPI_1_Start();
    UART_Start();

    Timer_Programmable_Init();

    BufQueueInit();

    UART_UartPutString("--- STARTUP ---\r\n");

    for (;;) {
        CyBle_ProcessEvents();

        if (state != prevState) PrintState(state);

        newState = NONE;
        switch (state) {
            case OFF:
                newState = SLEEP;
                break;

            case SLEEP:
                newState = GetBleStateIfNew(newState);
                break;

            case TIME:
                break;

            case VOICE:
                break;

            case NOTES:
                break;

            case NOTE_VIEW:
                break;

            case RUN_CMD:
                break;

            case RESULT:
                break;

            case CUSTOM_CMD:
                break;

            case DEBUG_IDLE:
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_SPEEDTEST:
                if (speedTestPackets > 0) {
                    if(CyBle_GattGetBusyStatus() == CYBLE_STACK_STATE_FREE) {
                        ((uint32 *) buffer)[0] = 0;
                        buffer[0] = 3;
                        ((uint32 *) buffer)[1] = n++;
                        ((uint32 *) buffer)[2] = 0x11223344;

                        ret = BleSendNotification(
                            CYBLE_SMARTWATCH_SERVICE_DEBUG_COMMAND_CHAR_HANDLE, buffer, 500);
                        if (ret == 0) {
                            speedTestPackets--;
                        }
                    }
                } else {
                    newState = DEBUG_SPEEDTEST_2;
                }
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_SPEEDTEST_2:
                if(CyBle_GattGetBusyStatus() == CYBLE_STACK_STATE_FREE) {
                    ((uint32 *) buffer)[0] = 0;
                    buffer[0] = 4;

                    ret = BleSendNotification(
                        CYBLE_SMARTWATCH_SERVICE_DEBUG_COMMAND_CHAR_HANDLE, buffer, 1);
                    if (ret == 0) {
                        newState = DEBUG_IDLE;
                    }
                }
                newState = GetBleStateIfNew(newState);
                break;


            case DEBUG_MIC:
                BufQueueInit();
                micBytes = 0;
                I2sStartDma();
                newState = DEBUG_MIC_2;
                newState = GetBleStateIfNew(newState);
                break;

#if 0
            case DEBUG_MIC_2:
                if (bufq.used >= 100000) {
                    I2sStopDma();
                    newState = DEBUG_MIC_3;
                }
                break;

            case DEBUG_MIC_3:
                ret = DequeueBytes(buffer, BUFSIZE, &done);
                if (ret == -ENODATA) {
                    newState = DEBUG_IDLE;
                } else {
                    newState = DEBUG_MIC_4;

                }
                break;

            case DEBUG_MIC_4:
                if (done) {
                    for (i = 4; i < BUFSIZE + 4; i += 2) {
                        sprintf(s, "%04X\r\n", *((uint16 *) &buffer[i]));
                        UART_UartPutString(s);
                    }
                    newState = DEBUG_MIC_3;
                }
                break;
#else
            case DEBUG_MIC_2:
                ret = DequeueBytes(buffer, 496, &done);
                if (ret != -ENODATA) {
                    newState = DEBUG_MIC_3;
                }
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_MIC_3:
                if (done) {
                    newState = DEBUG_MIC_4;
                }
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_MIC_4:
                if(CyBle_GattGetBusyStatus() == CYBLE_STACK_STATE_FREE) {
                    buffer[0] = 5;
                    ret = BleSendNotification(
                        CYBLE_SMARTWATCH_SERVICE_DEBUG_COMMAND_CHAR_HANDLE, buffer, 500);
                    if (ret == 0) {
                        micBytes += 500;
                        newState = DEBUG_MIC_2;
                    }
                }
                //if (micBytes > 200000) {
                if (micBytes > 100000) {
                    I2sStopDma();
                    newState = DEBUG_IDLE;
                }
                newState = GetBleStateIfNew(newState);
                break;
#endif

            case DEBUG_MEMTEST_FILL:
                //
                // Enqueue until full.
                //
                UART_UartPutString("MEMTEST: Filling queue...\r\n");
                BufQueueInit();
                enqCount = 0;
                newState = DEBUG_MEMTEST_FILL_2;
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_MEMTEST_FILL_2:
                if (bufq.free >= BUFSIZE) {
                    for (i = 0; i < BUFSIZE/sizeof(uint32); i++) {
                        ((uint32 *) buf1)[i] = enqCount++;
                    }
                    ret = EnqueueBytes(buf1, BUFSIZE, &done);
                    assert(ret == 0);
                    newState = DEBUG_MEMTEST_FILL_3;
                } else {
                    newState = DEBUG_MEMTEST_FILL_4;
                }
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_MEMTEST_FILL_3:
                if (done) {
                    newState = DEBUG_MEMTEST_FILL_2;
                }
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_MEMTEST_FILL_4:
                UART_UartPutString("MEMTEST: Queue filled.\r\n");
                sprintf(s, "MEMTEST: %d words\r\n", enqCount);
                UART_UartPutString(s);
                UART_UartPutString("MEMTEST: Dequeueing...\r\n");

                assert(bufq.free == 0);
                assert(bufq.used == QUEUE_SIZE);

                // Test overfilling
                ret = EnqueueBytes(buf1, BUFSIZE, &done);
                assert(ret == -ENOSPC);
                deqCount = 0;
                newState = DEBUG_MEMTEST_DEPLETE;
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_MEMTEST_DEPLETE:
                //
                // Dequeue until empty
                //
                if (bufq.used > 0) {
                    ret = DequeueBytes(buf1, BUFSIZE, &done);
                    assert(ret == 0);
                    newState = DEBUG_MEMTEST_DEPLETE_2;
                } else {
                    newState = DEBUG_MEMTEST_DEPLETE_3;
                }
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_MEMTEST_DEPLETE_2:
                if (done) {
                    // Skip first word, which contains SPI command/address
                    for (i = 1; i <= BUFSIZE/sizeof(uint32); i++) {
                        assert(((uint32 *) buf1)[i] == deqCount);
                        deqCount++;
                    }
                    newState = DEBUG_MEMTEST_DEPLETE;
                }
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_MEMTEST_DEPLETE_3:
                UART_UartPutString("MEMTEST: Queue emptied.\r\n");

                assert(bufq.free == QUEUE_SIZE);
                assert(bufq.used == 0);

                // Test over-dequeueing
                ret = DequeueBytes(buf1, BUFSIZE, &done);
                assert(ret == -ENODATA);
                concurrencyTestType = CONCURRENCY_TEST_NORMAL;
                newState = DEBUG_MEMTEST_CONCURRENCY;
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_MEMTEST_CONCURRENCY:
                UART_UartPutString("MEMTEST: Concurrency test.  Half filling queue...\r\n");
                BufQueueInit();
                enqCount = 0;
                deqCount = 0;
                newState = DEBUG_MEMTEST_CONCURRENCY_2;
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_MEMTEST_CONCURRENCY_2:
                //
                // Fill queue until half full.
                //
                if (bufq.used < QUEUE_SIZE/2) {
                    for (i = 0; i < BUFSIZE/sizeof(uint32); i++) {
                        ((uint32 *) buf1)[i] = enqCount++;
                    }
                    ret = EnqueueBytes(buf1, BUFSIZE, &done);
                    assert(ret == 0);
                    newState = DEBUG_MEMTEST_CONCURRENCY_3;
                } else {
                    newState = DEBUG_MEMTEST_CONCURRENCY_4;
                }
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_MEMTEST_CONCURRENCY_3:
                if (done) {
                    newState = DEBUG_MEMTEST_CONCURRENCY_2;
                }
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_MEMTEST_CONCURRENCY_4:
                //
                // Concurrent fill and empty at same time, with verify.
                // Fill via timer interrupt, to test concurrency.
                //
                Timer_Programmable_WritePeriod(
                    TIMER_CLOCK_FREQ_MHZ * enqPeriodUs[concurrencyTestType]
                    );
                Timer_Programmable_ISR_StartEx(DebugMemtestIsr);
                Timer_Programmable_Enable();
                newState = DEBUG_MEMTEST_CONCURRENCY_5;
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_MEMTEST_CONCURRENCY_5:
                CyDelayUs(deqPeriodUs[concurrencyTestType]);
                ret = DequeueBytes(buf2, BUFSIZE, &done);
                if (ret == 0 || ret == -EAGAIN) {
                    newState = DEBUG_MEMTEST_CONCURRENCY_6;
                }
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_MEMTEST_CONCURRENCY_6:
                if (done) {
                    for (i = 1; i <= BUFSIZE/sizeof(uint32); i++) {
                        assert(((uint32 *) buf2)[i] == deqCount);
                        deqCount++;
                    }
                    if (deqCount < 1000000) {
                        newState = DEBUG_MEMTEST_CONCURRENCY_5;
                    } else {
                        Timer_Programmable_ISR_Stop();
                        concurrencyTestType++;
                        if (concurrencyTestType < CONCURRENCY_TEST_MAX) {
                            newState = DEBUG_MEMTEST_CONCURRENCY;
                        } else {
                            newState = DEBUG_IDLE;
                        }
                    }
                }
                newState = GetBleStateIfNew(newState);
                break;

            default:
                break;
        }

        prevState = state;
        if (newState != NONE) {
            state = newState;
        }
    }
}
