int p1, p2, p3;
#include <main.h>
#include <BLEApplications.h>
#include <errno.h>
#include <stdio.h>

uint8 buffer[MAX_MTU_SIZE];
int deviceConnected = 0;
int speedTestPackets;

int enqCount;

#define QUEUE_SIZE (128 * 1024)

#define CLOCK_FREQ_MHZ 1

enum DMA_MODE {
    DMA_MODE_NONE,
    DMA_MODE_ENQUEUE,
    DMA_MODE_DEQUEUE,
    DMA_MODE_LCD,
} spiTxDmaMode, spiRxDmaMode;

enum CONCURRENCY_TEST_TYPE {
    CONCURRENCY_TEST_NORMAL,
    CONCURRENCY_TEST_FAST_DEQ,
    CONCURRENCY_TEST_FAST_ENQ,
    CONCURRENCY_TEST_MAX,
};

//
// For debug, enqueue/dequeue test periods, for each CONCURRENCY_TEST_TYPE
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
    #define X_PRINT_CASE(state) case state: break

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
        PRINT_CASE(DEBUG_MEMTEST_FILL);
        X_PRINT_CASE(DEBUG_MEMTEST_FILL_2);
        X_PRINT_CASE(DEBUG_MEMTEST_FILL_3);
        PRINT_CASE(DEBUG_MEMTEST_FILL_4);
        X_PRINT_CASE(DEBUG_MEMTEST_DEPLETE);
        X_PRINT_CASE(DEBUG_MEMTEST_DEPLETE_2);
        PRINT_CASE(DEBUG_MEMTEST_DEPLETE_3);
        PRINT_CASE(DEBUG_MEMTEST_CONCURRENCY);
        X_PRINT_CASE(DEBUG_MEMTEST_CONCURRENCY_2);
        X_PRINT_CASE(DEBUG_MEMTEST_CONCURRENCY_3);
        PRINT_CASE(DEBUG_MEMTEST_CONCURRENCY_4);
        X_PRINT_CASE(DEBUG_MEMTEST_CONCURRENCY_5);
        X_PRINT_CASE(DEBUG_MEMTEST_CONCURRENCY_6);
        default:
            UART_UartPutString("State: UNKNOWN!\r\n");
            break;
    }
}

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
        default:
            bleSelectedNextState = NONE;
            break;
    }

    return 0;
}

void OnConnectionChange(
    int connected
    )
{
    uint8 c = 0;
    BleWriteCharacteristic(CYBLE_SMARTWATCH_SERVICE_VOICE_DATA_CHAR_HANDLE, &c, sizeof(c));
    deviceConnected = connected;
}

#define BUFSIZE (0x100)
uint8 buf[BUFSIZE + 4];
uint8 buf2[BUFSIZE + 4];

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

struct QUEUED_DESCRIPTOR {
    void (*call)();
    uint8 * buf;
    uint32 bytes;
    int * done;
};

#define NUM_QDS 8

//
// Circular buffer of queue descriptors.  To simplify queueing, one dummy
// descriptor is between head and tail.  head == tail means empty,
// (tail + 1 % NUM) == head means full.
//
struct QUEUED_DESCRIPTORS {
    struct QUEUED_DESCRIPTOR queue[NUM_QDS];
    uint32 head;
    uint32 tail;
} queuedDescriptors;

int spiDmaLocked = 0;

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
        pqd->call(pqd->buf, pqd->bytes, pqd->done, 1);
    }
}

void I2sRxDmaIsr(void)
{
}

int * spiTxDmaDone;
int * spiRxDmaDone;

void DummyDmaIsr(void) {}

//
// SPI TX DMA ISR - data sent, update queue.
//
// Second spurious ISR may (always?) occur - code handles being called multiple
// times in a row safely.
//
void SpiTxDmaIsr(void)
{
    enum DMA_MODE mode;
    mode = spiTxDmaMode;
    spiTxDmaMode = DMA_MODE_NONE;
    switch (mode) {
        case DMA_MODE_ENQUEUE:
            bufq.tail = bufq.pendingTail;
            bufq.used += bufq.pendingTailBytes;
            bufq.pendingTailBytes = 0;
            *spiTxDmaDone = 1;
            UnlockSpiDma();
            return;
        case DMA_MODE_LCD:
            return;
        default:
            return;
    }
}

//
// SPI TX DMA ISR - data received, update queue.
//
// See SpiTxDmaIsr() about second spurious ISR.
//
void SpiRxDmaIsr(void)
{
    enum DMA_MODE mode;
    mode = spiRxDmaMode;
    spiRxDmaMode = DMA_MODE_NONE;
    switch (mode) {
        case DMA_MODE_DEQUEUE:
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
// Returns 0 on success, or negative value on error.  EAGAIN indicates call has
// been queued, will be recalled when running DMA is complete.
//
int enqueueDone;
int EnqueueBytes(uint8 *buf, uint32 bytes, int * done, int queuedCall)
{
    char s[32];
    uint32 tail;
    int isSpaceAvailable;
    uint8 interruptState;
    struct QUEUED_DESCRIPTOR qd;
    int ret;

    if (done != NULL) {
        *done = 0;
    }
    if (! queuedCall) {
        qd.call = &EnqueueBytes;
        qd.buf = buf;
        qd.bytes = bytes;
        qd.done = done;
        if (LockSpiDma(&qd)) return -EAGAIN;
    }

    // Back to back DMAs can clobber bits at the end of the first DMA.
    // FIXME: only need 5 us?
    CyDelayUs(5);

    interruptState = CyEnterCriticalSection();

    isSpaceAvailable = bufq.free >= bytes;
    if (isSpaceAvailable) {
        assert(bufq.tail == bufq.pendingTail);
        tail = bufq.tail;
        bufq.pendingTail = (bufq.tail + bytes) % QUEUE_SIZE;
        assert(bufq.free >= bytes);
        bufq.free -= bytes;
        bufq.pendingTailBytes = bytes;
    }

    CyExitCriticalSection(interruptState);

    if (! isSpaceAvailable) {
        UnlockSpiDma();
        return -ENOSPC;
    }

    if (done == NULL) {
        done = &enqueueDone;
    }
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
    spiTxDmaMode = DMA_MODE_ENQUEUE;
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
// The given /buf/ must be 4 bytes longer than the /bytes/ specified.  A SPI
// command/address will be filled in as the first 4 bytes of the returned /buf/.
//
uint8 allff[BUFSIZE + 4];
int dequeueDone;
int DequeueBytes(uint8 *buf, int bytes, int * done, int queuedCall)
{
    uint32 head;
    int j;
    int isDataAvailable;
    uint8 interruptState;
    struct QUEUED_DESCRIPTOR qd;

    if (done != NULL) {
        *done = 0;
    }
    if (! queuedCall) {
        qd.call = &DequeueBytes;
        qd.buf = buf;
        qd.bytes = bytes;
        qd.done = done;
        if (LockSpiDma(&qd)) return -EAGAIN;
    }

    // Back to back DMAs can clobber bits at the end of the first DMA.
    CyDelayUs(5);

    interruptState = CyEnterCriticalSection();

    isDataAvailable = bufq.used >= bytes;
    if (isDataAvailable) {
        assert(bufq.head == bufq.pendingHead);
        head = bufq.head;
        bufq.pendingHead = (bufq.head + bytes) % QUEUE_SIZE;
        assert(bufq.used >= bytes);
        bufq.used -= bytes;
        bufq.pendingHeadBytes = bytes;
    }

    CyExitCriticalSection(interruptState);

    if (! isDataAvailable) {
        UnlockSpiDma();
        return -ENODATA;
    }

    if (done == NULL) {
        done = &enqueueDone;
    }
    spiRxDmaDone = done;

    // TODO - need to write from FF buf without source increment
    memset(allff, 0xFF, sizeof(allff));

    spiTxDmaMode = DMA_MODE_DEQUEUE;
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
    spiRxDmaMode = DMA_MODE_DEQUEUE;
    SpiRxDma_ChEnable();
    spiTxDmaMode = DMA_MODE_DEQUEUE;
    SpiTxDma_ChEnable();
    CyExitCriticalSection(interruptState);

    return 0;
}

CY_ISR(DebugMemtestIsr)
{
    int i;

Pin_1_Write(p1); p1 = ! p1;
    Timer_Programmable_ClearInterrupt(Timer_Programmable_GetInterruptSource());
    for (i = 0; i < BUFSIZE/sizeof(uint32); i++) {
        ((uint32 *) buf)[i] = enqCount++;
    }
    int ret = EnqueueBytes(buf, BUFSIZE, NULL, 0);
    // If didn't queue, rollback counter
    if (ret == -ENOSPC) {
        enqCount -= BUFSIZE/sizeof(uint32);
    }
}

int main()
{
    uint32 n = 0;
    int ret;
    enum STATE newState, prevState;
    int j;
    char s[32];
    enum CONCURRENCY_TEST_TYPE concurrencyTestType;
    int deqCount;
    int done;

    prevState = OFF;
    state = SLEEP;

    CyDmaEnable();
    CyIntEnable(CYDMA_INTR_NUMBER);

    CyGlobalIntEnable;

    I2sRxDma_SetInterruptCallback(&I2sRxDmaIsr);

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

    UART_UartPutString("--- STARTUP ---\r\n");

    if (1) {
        uint8 c;
        uint32 sample;
        int i = 0;
        int n = 0;
        int count = 0;

#if 0
        I2S_1_ClearRxFIFO();
        I2S_1_EnableRx();
        I2sRxDma_Start((void *) I2S_1_RX_CH0_F0_PTR, buf);
        I2sRxDma_SetNumDataElements(0, BUFSIZE * 2);
        while (!done);
        UART_UartPutString("done 1\r\n");
#if 0
        SpiTxDma_Start(buf, buf2);
        SpiTxDma_SetNumDataElements(0, 0x40);
        SpiTxDma_Trigger();
        UART_UartPutString("triggered\r\n");
        while (!done2);
#endif
        UART_UartPutString("done 2\r\n");
#else
        while (n < BUFSIZE) {
            while(!((c = I2S_1_ReadRxStatus()) &
                (I2S_1_RX_FIFO_0_NOT_EMPTY | I2S_1_RX_FIFO_1_NOT_EMPTY)));

            if (c & I2S_1_RX_FIFO_0_NOT_EMPTY) {
                sample = sample << 8 | I2S_1_ReadByte(I2S_1_RX_LEFT_CHANNEL);
                i = (i + 1) % 4;
                if (i == 0) {
                    buf[n++] = (uint16) sample;
                }
            }
            if (c & I2S_1_RX_FIFO_1_NOT_EMPTY) {
                I2S_1_ReadByte(I2S_1_RX_RIGHT_CHANNEL);
            }
        }
#endif
        //for (i = 0; i < BUFSIZE; i++) {
        UART_UartPutString("------------------------\r\n");
        for (i = 0; i < BUFSIZE; i++) {
            sprintf(s, "%04X\r\n", buf[i]);
            UART_UartPutString(s);
        }
        UART_UartPutString("------------------------\r\n");
#if 0
        for (i = 0; i < 16; i++) {
            sprintf(s, "%04X\r\n", buf2[i]);
            UART_UartPutString(s);
        }
#endif
        UART_UartPutString("------------------------\r\n");
        sprintf(s, "%d\r\n", count); UART_UartPutString(s);
    }

    int i = 0;
    for (;;) {
        CyBle_ProcessEvents();

        if (state != prevState) PrintState(state);

        newState = NONE;
        switch (state) {
            case OFF:
                newState = SLEEP;
                break;

            case SLEEP:
                if (bleSelectedNextState != NONE) {
                    newState = bleSelectedNextState;
                }
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
                if (bleSelectedNextState != NONE) {
                    newState = bleSelectedNextState;
                }
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
                break;


            case DEBUG_MIC:
                {
                    uint8 c;
                    uint32 sample;

                    while(!((c = I2S_1_ReadRxStatus()) & (I2S_1_RX_FIFO_0_NOT_EMPTY | I2S_1_RX_FIFO_1_NOT_EMPTY))) {
                    }
                    if (c & I2S_1_RX_FIFO_0_NOT_EMPTY) {
                        sample = sample << 8 | I2S_1_ReadByte(I2S_1_RX_LEFT_CHANNEL);
                        i = (i + 1) % 4;
                        if (i == 0) {
                            //UART_UartPutChar((uint8) ((sample >> 14) & 0xFF));
                            //UART_UartPutChar((uint8) ((sample >> 22) & 0xFF));
                        }
                    }
                    if (c & I2S_1_RX_FIFO_1_NOT_EMPTY) {
                        I2S_1_ReadByte(I2S_1_RX_RIGHT_CHANNEL);
                    }
                }
                break;

            case DEBUG_MEMTEST_FILL:
                //
                // Enqueue until full.
                //
                UART_UartPutString("MEMTEST: Filling queue...\r\n");
                BufQueueInit();
                enqCount = 0;
                newState = DEBUG_MEMTEST_FILL_2;
                break;

            case DEBUG_MEMTEST_FILL_2:
                if (bufq.free >= BUFSIZE) {
                    for (i = 0; i < BUFSIZE/sizeof(uint32); i++) {
                        ((uint32 *) buf)[i] = enqCount++;
                    }
                    ret = EnqueueBytes(buf, BUFSIZE, &done, 0);
                    assert(ret == 0);
                    newState = DEBUG_MEMTEST_FILL_3;
                } else {
                    newState = DEBUG_MEMTEST_FILL_4;
                }
                break;

            case DEBUG_MEMTEST_FILL_3:
                if (done) {
                    newState = DEBUG_MEMTEST_FILL_2;
                }
                break;

            case DEBUG_MEMTEST_FILL_4:
                UART_UartPutString("MEMTEST: Queue filled.\r\n");
                sprintf(s, "MEMTEST: %d words\r\n", enqCount);
                UART_UartPutString(s);
                UART_UartPutString("MEMTEST: Dequeueing...\r\n");

                assert(bufq.free == 0);
                assert(bufq.used == QUEUE_SIZE);

                // Test overfilling
                ret = EnqueueBytes(buf, BUFSIZE, &done, 0);
                assert(ret == -ENOSPC);
                deqCount = 0;
                newState = DEBUG_MEMTEST_DEPLETE;
                break;

            case DEBUG_MEMTEST_DEPLETE:
                //
                // Dequeue until empty
                //
                if (bufq.used > 0) {
                    ret = DequeueBytes(buf, BUFSIZE, &done, 0);
                    assert(ret == 0);
                    newState = DEBUG_MEMTEST_DEPLETE_2;
                } else {
                    newState = DEBUG_MEMTEST_DEPLETE_3;
                }
                break;

            case DEBUG_MEMTEST_DEPLETE_2:
                if (done) {
                    // Skip first word, which contains SPI command/address
                    for (i = 1; i <= BUFSIZE/sizeof(uint32); i++) {
                        assert(((uint32 *) buf)[i] == deqCount);
                        deqCount++;
                    }
                    newState = DEBUG_MEMTEST_DEPLETE;
                }
                break;

            case DEBUG_MEMTEST_DEPLETE_3:
                UART_UartPutString("MEMTEST: Queue emptied.\r\n");

                assert(bufq.free == QUEUE_SIZE);
                assert(bufq.used == 0);

                // Test over-dequeueing
                ret = DequeueBytes(buf, BUFSIZE, &done, 0);
                assert(ret == -ENODATA);
                concurrencyTestType = CONCURRENCY_TEST_NORMAL;
                newState = DEBUG_MEMTEST_CONCURRENCY;
                break;

            case DEBUG_MEMTEST_CONCURRENCY:
                UART_UartPutString("MEMTEST: Concurrency test.  Half filling queue...\r\n");
                BufQueueInit();
                enqCount = 0;
                deqCount = 0;
                newState = DEBUG_MEMTEST_CONCURRENCY_2;
                break;

            case DEBUG_MEMTEST_CONCURRENCY_2:
                //
                // Fill queue until half full.
                //
                if (bufq.used < QUEUE_SIZE/2) {
                    for (i = 0; i < BUFSIZE/sizeof(uint32); i++) {
                        ((uint32 *) buf)[i] = enqCount++;
                    }
                    ret = EnqueueBytes(buf, BUFSIZE, &done, 0);
                    assert(ret == 0);
                    newState = DEBUG_MEMTEST_CONCURRENCY_3;
                } else {
                    newState = DEBUG_MEMTEST_CONCURRENCY_4;
                }
                break;

            case DEBUG_MEMTEST_CONCURRENCY_3:
                if (done) {
                    newState = DEBUG_MEMTEST_CONCURRENCY_2;
                }
                break;

            case DEBUG_MEMTEST_CONCURRENCY_4:
                //
                // Concurrent fill and empty at same time, with verify.
                // Fill via timer interrupt, to test concurrency.
                //
                Timer_Programmable_WritePeriod(
                    CLOCK_FREQ_MHZ * enqPeriodUs[concurrencyTestType]
                    );
                Timer_Programmable_ISR_StartEx(DebugMemtestIsr);
                Timer_Programmable_Enable();
                newState = DEBUG_MEMTEST_CONCURRENCY_5;
                break;

            case DEBUG_MEMTEST_CONCURRENCY_5:
                CyDelayUs(deqPeriodUs[concurrencyTestType]);
                ret = DequeueBytes(buf2, BUFSIZE, &done, 0);
                if (ret == 0 || ret == -EAGAIN) {
                    newState = DEBUG_MEMTEST_CONCURRENCY_6;
                }
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
                break;

            default:
                break;
        }

        bleSelectedNextState = NONE;
        prevState = state;
        if (newState != NONE) {
            state = newState;
        }
    }
}
