/*
 * test.c
 *
 * Test Suite
 *
 * Copyright (C) 2018 Brian Silverman <bri@readysetstem.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 */
#include <project.h>
#include <errno.h>
#include "printf.h"
#include "assert.h"
#include "spi.h"
#include "serialram.h"
#include "queue.h"
#include "i2s.h"
#include "oled.h"
#include "fonts.h"
#include "colors.h"
#include "timeit.h"
#include "util.h"
#include "draw.h"

#define TEST_VERBOSE 0

int pass = 0;
int fail = 0;
#define TEST(func) \
    { \
        int fail_this_test; \
        xprintf("????: %s\r\n", #func); \
        fail_this_test = func; \
        xprintf("\r%s\r\n", fail_this_test ?  "FAIL" : "\e[FPASS"); \
        if (fail_this_test) { \
            fail++; \
        } else { \
            pass++; \
        } \
    }

#define MTEST(func) \
    { \
        xprintf("\aMANU: %s\r\n", #func); \
        func; \
    }

#define TEST_INIT \
    int testAssertFails = 0

#define TEST_ASSERT(assert) \
    testAssertFails += _TestAssert(assert, #assert, TEST_VERBOSE, __FILE__, __LINE__)

#define TEST_ASSERT_INT_EQ(val1, val2) \
    testAssertFails += _TestAssertIntEq(val1, val2, #val1, #val2, TEST_VERBOSE, __FILE__, __LINE__)

#define TEST_ASSERT_PRINT(assert, args...) \
    { \
        int fail = _TestAssert(assert, #assert, TEST_VERBOSE, __FILE__, __LINE__); \
        testAssertFails += fail; \
        if (fail) xprintf("\t" args); \
    }

#define TEST_RETURN \
    { \
        if (testAssertFails) xprintf("\r\n"); \
        return testAssertFails; \
    }

#define MTEST_DELAY      (2000)
#define MTEST_DELAY_FAST (1000)
#define MTEST_DELAY_SLOW (4000)

int _TestAssert(
    int assertion,
    char * assertStr,
    int verbose,
    char * file,
    int line
    )
{
    if (!assertion || verbose) {
        xprintf("\t%s(%s) @ %s, %d\r\n", assertion ? "PASS" : "FAIL", assertStr, file, line);
    }
    return !assertion;
}

int _TestAssertIntEq(
    int val1,
    int val2,
    char * val1Str,
    char * val2Str,
    int verbose,
    char * file,
    int line
    )
{
    int assertion = val1 == val2;
    if (!assertion || verbose) {
        xprintf("\t%s(%s == %s), (%d == %d) @ %s, %d\r\n",
            assertion ? "PASS" : "FAIL", val1Str, val2Str, val1, val2, file, line);
    }
    return !assertion;
}

/////////////////////////////////////////////////////////////////////
//
// Test functions and variables
//
/////////////////////////////////////////////////////////////////////


// Freq of Timer_Programmable_* input clock
#define TIMER_CLOCK_FREQ_MHZ 1

void DoneCallback(
    void * done
    )
{
    if (done) {
        *((int *) done) = 1;
    }
}

#define TEST_SPI_XFER_RX 0
#define TEST_SPI_XFER_TX 1
int TestSpiXferInterrupts(
    int isTx,
    int num
    )
{
    TEST_INIT;

    int done;
    int i;
    uint8 * pbuf1 = SerialRamGetBuf(0);
    uint8 * pbuf2 = SerialRamGetBuf(1);
    uint32 ss = SPI_SS_MEM_0;

    spiTxIsrCalls = spiRxIsrCalls = 0;
    for (i = 0; i < num; i++) {
        done = 0;
        SpiXferUnlocked(pbuf1, isTx ? NULL : pbuf2, SERIAL_RAM_BUFSIZE, ss, DoneCallback, &done, 0);
        TEST_ASSERT_INT_EQ(spiTxIsrCalls, ( isTx ? i : 0));
        TEST_ASSERT_INT_EQ(spiRxIsrCalls, (!isTx ? i : 0));
        while (!done);
        TEST_ASSERT_INT_EQ(spiTxIsrCalls, ( isTx ? i + 1 : 0));
        TEST_ASSERT_INT_EQ(spiRxIsrCalls, (!isTx ? i + 1 : 0));
    }

    TEST_RETURN;
}

#define TEST_SPI_XFER_UNLOCKED 0
#define TEST_SPI_XFER_LOCKED 1
int TestSpiXferWriteReadMem(
    int isLocked
    )
{
    TEST_INIT;

    int done;
    int i;
    uint8 * pbuf1 = SerialRamGetBuf(0);
    uint8 * pbuf2 = SerialRamGetBuf(1);
    uint32 ss = SPI_SS_MEM_0;

    memset(pbuf1, 0, SERIAL_RAM_BUFSIZE);
    for (i = 1; i < SERIAL_RAM_BUFSIZE/sizeof(uint32); i++) {
        ((uint32 *) pbuf1)[i] = i;
    }
    pbuf1[0] = 0x02;
    done = 0;
    if (isLocked) {
        SpiXfer(pbuf1, NULL, SERIAL_RAM_BUFSIZE, ss, DoneCallback, &done, 0);
    } else {
        SpiXferUnlocked(pbuf1, NULL, SERIAL_RAM_BUFSIZE, ss, DoneCallback, &done, 0);
    }
    while (!done);
    memset(pbuf1, 0, SERIAL_RAM_BUFSIZE);
    memset(pbuf2, 0, SERIAL_RAM_BUFSIZE);
    pbuf1[0] = 0x03;
    done = 0;
    if (isLocked) {
        SpiXfer(pbuf1, pbuf2, SERIAL_RAM_BUFSIZE, ss, DoneCallback, &done, 0);
    } else {
        SpiXferUnlocked(pbuf1, pbuf2, SERIAL_RAM_BUFSIZE, ss, DoneCallback, &done, 0);
    }
    while (!done);
    for (i = 1; i < SERIAL_RAM_BUFSIZE/sizeof(uint32); i++) {
        TEST_ASSERT(((uint32 *) pbuf2)[i] == i);
    }

    TEST_RETURN;
}

int TestSpiXferTooMany()
{
    TEST_INIT;

    int i;
    uint8 * pbuf1 = SerialRamGetBuf(0);
    uint32 ss = SPI_SS_MEM_0;

    TEST_ASSERT_INT_EQ(SpiXfer(pbuf1, NULL, SERIAL_RAM_BUFSIZE, ss, NULL, NULL, 0), 0);
    for (i = 0; i < NUM_SPI_QDS - 1; i++) {
        TEST_ASSERT_INT_EQ(SpiXfer(pbuf1, NULL, SERIAL_RAM_BUFSIZE, ss, NULL, NULL, 0), -EAGAIN);
    }
    TEST_ASSERT_INT_EQ(SpiXfer(pbuf1, NULL, SERIAL_RAM_BUFSIZE, ss, NULL, NULL, 0), -ENOMEM);

    TEST_RETURN;
}

int TestSerialRamFuncs()
{
    TEST_INIT;

    uint8 * pbuf1 = SerialRamGetBuf(0);

    TEST_ASSERT(SerialRamRead(pbuf1, 0x0000000, SERIAL_RAM_BUFSIZE, NULL, NULL) == 0);
    CyDelay(10);
    TEST_ASSERT(SerialRamWrite(pbuf1, 0x0000000, SERIAL_RAM_BUFSIZE, NULL, NULL) == 0);
    CyDelay(10);

    TEST_RETURN;
}

int TestSerialRamWriteRead()
{
    TEST_INIT;

    int i, j;
    int ret;
    uint8 * pbuf1 = SerialRamGetBuf(0);
    uint8 * pbuf2 = SerialRamGetBuf(1);

    for (i = 0; i < 3; i++) {
        uint32 ramAddress = i * 0x00002345; // Arbitrary address

        memset(pbuf1, 0, SERIAL_RAM_BUFSIZE);
        memset(pbuf2, 0, SERIAL_RAM_BUFSIZE);
        for (j = 0; j <= SERIAL_RAM_BUFSIZE/sizeof(uint32); j++) {
            // Arbitrary different values on each iteration
            ((uint32 *) pbuf1)[j] = i*SERIAL_RAM_BUFSIZE+j;
        }
        ret = SerialRamWriteBlocking(pbuf1, ramAddress, SERIAL_RAM_BUFSIZE);
        TEST_ASSERT(ret == 0);
        ret = SerialRamReadBlocking(pbuf2, ramAddress, SERIAL_RAM_BUFSIZE);
        TEST_ASSERT(ret == 0);

        for (j = 0; j < SERIAL_RAM_BUFSIZE/sizeof(uint32); j++) {
            TEST_ASSERT_INT_EQ(((uint32 *) pbuf2)[j], i*SERIAL_RAM_BUFSIZE+j);
        }
    }

    TEST_RETURN;
}

//
// Fill serial ram with arbitrary value and test reading arbitrary location,
// then refill and retest with another value and location.
//
int TestSerialRamFillMem()
{
    TEST_INIT;

    uint8 * pbuf1 = SerialRamGetBuf(0);
    uint8 * pbuf2 = SerialRamGetBuf(1);
    const uint8 arbitraryVal1 = 0x12;
    const uint8 arbitraryVal2 = 0x56;
    const uint32 arbitraryAddr1 = 0x1234;
    const uint32 arbitraryAddr2 = 0xABCD;
    int i;

    TEST_ASSERT(SerialRamFillBlocking(arbitraryVal1) == 0);
    TEST_ASSERT(SerialRamReadBlocking(pbuf1, arbitraryAddr1, SERIAL_RAM_BUFSIZE) == 0);
    for (i = 0; i < SERIAL_RAM_BUFSIZE; i++) {
        TEST_ASSERT_INT_EQ(arbitraryVal1, pbuf1[i]);
    }
    TEST_ASSERT(SerialRamFillBlocking(arbitraryVal2) == 0);
    TEST_ASSERT(SerialRamReadBlocking(pbuf2, arbitraryAddr2, SERIAL_RAM_BUFSIZE) == 0);
    for (i = 0; i < SERIAL_RAM_BUFSIZE; i++) {
        TEST_ASSERT_INT_EQ(arbitraryVal2, pbuf2[i]);
    }

    TEST_RETURN;
}

//
// Write two buffers, that overlap in both time and location.  Transactions
// should be serialized.  Data should be partially overwritten from overlap.
//
int TestSerialRamXfersOverlapping()
{
    TEST_INIT;

    const uint32 HALF_BUFSIZE = SERIAL_RAM_BUFSIZE/2;
    const uint32 HALF = HALF_BUFSIZE/sizeof(uint32);
    int i;
    int ret;
    uint8 * pbuf1 = SerialRamGetBuf(0);
    uint8 * pbuf2 = SerialRamGetBuf(1);

    uint32 ramAddress = 0x00012345; // Arbitrary address

    for (i = 0; i <= SERIAL_RAM_BUFSIZE/sizeof(uint32); i++) {
        ((uint32 *) pbuf1)[i] = i + 0x1000;
    }
    for (i = 0; i <= SERIAL_RAM_BUFSIZE/sizeof(uint32); i++) {
        ((uint32 *) pbuf2)[i] = i + 0x2000;
    }

    //
    // Write first buffer without waiting, second buffer block until done.
    // Second buffer overlaps half of first, so total written length is 1.5
    // buffer sizes.
    //
    ret = SerialRamWrite(pbuf1, ramAddress, SERIAL_RAM_BUFSIZE, NULL, NULL);
    TEST_ASSERT(ret == 0);
    ret = SerialRamWriteBlocking(pbuf2, ramAddress + HALF_BUFSIZE, SERIAL_RAM_BUFSIZE);
    TEST_ASSERT(ret == 0);

    //
    // Read back memory buffers, not overlapped locations.  Verify in 3 sections (first half,
    // second half, third half) of 1.5 length of SERIAL_RAM_BUFSIZE.
    //
    memset(pbuf1, 0, SERIAL_RAM_BUFSIZE);
    memset(pbuf2, 0, SERIAL_RAM_BUFSIZE);
    ret = SerialRamRead(pbuf1, ramAddress, SERIAL_RAM_BUFSIZE, NULL, NULL);
    TEST_ASSERT(ret == 0);
    ret = SerialRamReadBlocking(pbuf2, ramAddress + SERIAL_RAM_BUFSIZE, SERIAL_RAM_BUFSIZE);
    TEST_ASSERT(ret == 0);
    for (i = 0; i < HALF; i++) {
        uint32 * pbuf = (uint32 *) pbuf1;
        TEST_ASSERT_INT_EQ(pbuf[i], i + 0x1000);
        TEST_ASSERT_INT_EQ(pbuf[i + HALF], i + 0x2000);
    }
    for (i = 0; i < HALF; i++) {
        uint32 * pbuf = (uint32 *) pbuf2;
        TEST_ASSERT_INT_EQ(pbuf[i], i + HALF + 0x2000);
    }

    TEST_RETURN;
}

//
// For DEBUG_MEMTEST_CONCURRENCY testing, periodic enqueuing of bytes
//
int enqCount;
int enqPeriodUs = TIMER_CLOCK_FREQ_MHZ * 5000;
int prevFree;
CY_ISR(DebugMemtestIsr)
{
    int i;
    uint8 * pbuf1 = SerialRamGetBuf(0);

    Timer_Programmable_ClearInterrupt(Timer_Programmable_GetInterruptSource());
    for (i = 0; i < SERIAL_RAM_BUFSIZE/sizeof(uint32); i++) {
        ((uint32 *) pbuf1)[i] = enqCount++;
    }
    int ret = EnqueueBytes(pbuf1, SERIAL_RAM_BUFSIZE, NULL);
    // If didn't queue, rollback counter
    if (ret == -ENOSPC) {
        enqCount -= SERIAL_RAM_BUFSIZE/sizeof(uint32);
    }
    if (BufQueueFree() > prevFree) {
        enqPeriodUs -= 100;
    } else if (BufQueueFree() < prevFree) {
        enqPeriodUs += 100;
    }
    prevFree = BufQueueFree();

    Timer_Programmable_WritePeriod(enqPeriodUs);
    Timer_Programmable_Enable();
}

int TestQueueEnqueueOneDequeueOne()
{
    TEST_INIT;

    int i;

    uint8 * pbuf1 = SerialRamGetBuf(0);
    uint8 * pbuf2 = SerialRamGetBuf(1);

    BufQueueInit();
    memset(pbuf1, 0, SERIAL_RAM_BUFSIZE);
    memset(pbuf2, 0, SERIAL_RAM_BUFSIZE);

    TEST_ASSERT(BufQueueFree() == QUEUE_SIZE);
    TEST_ASSERT(BufQueueUsed() == 0);
    for (i = 0; i < SERIAL_RAM_BUFSIZE/sizeof(uint32); i++) {
        ((uint32 *) pbuf1)[i] = i;
    }
    TEST_ASSERT(EnqueueBytesBlocking(pbuf1, SERIAL_RAM_BUFSIZE) == 0);

    TEST_ASSERT(DequeueBytesBlocking(pbuf2, SERIAL_RAM_BUFSIZE) == 0);
    for (i = 0; i < SERIAL_RAM_BUFSIZE/sizeof(uint32); i++) {
        TEST_ASSERT_INT_EQ(((uint32 *) pbuf2)[i], i);
    }
    TEST_ASSERT(BufQueueFree() == QUEUE_SIZE);
    TEST_ASSERT(BufQueueUsed() == 0);

    TEST_RETURN;
}

int TestQueueEnqueueDequeueVariableSizes()
{
    TEST_INIT;

    int i, j, x;
    uint8 copyBuf[32];

    uint8 * pbuf1 = SerialRamGetBuf(0);
    uint8 * pbuf2 = SerialRamGetBuf(1);

    BufQueueInit();
    memset(pbuf1, 0, SERIAL_RAM_BUFSIZE);
    memset(pbuf2, 0, SERIAL_RAM_BUFSIZE);

    TEST_ASSERT(BufQueueFree() == QUEUE_SIZE);
    TEST_ASSERT(BufQueueUsed() == 0);

    //
    // Enqueue bytes: 1, 12, 123, 1234, 12345
    //
    for (i = 1; i <= 5; i++) {
        pbuf1[i-1] = i;
        TEST_ASSERT(EnqueueBytesBlocking(pbuf1, i) == 0);
    }

    //
    // Dequeue bytes in backwards chunks: 11212, 3123, 412, 34, 5
    //
    x = 0;
    for (i = 5; i >= 1; i--) {
        TEST_ASSERT(DequeueBytesBlocking(pbuf2, i) == 0);
        memcpy(&copyBuf[x], pbuf2, i);
        x += i;
    }
    TEST_ASSERT(BufQueueFree() == QUEUE_SIZE);
    TEST_ASSERT(BufQueueUsed() == 0);

    //
    // Verify enqueued/dequeued bytes
    //
    x = 0;
    for (i = 1; i <= 5; i++) {
        for (j = 1; j <= i; j++) {
            TEST_ASSERT_INT_EQ((int) copyBuf[x++], j);
        }
    }

    TEST_RETURN;
}

int TestQueueFillThenEmpty()
{
    TEST_INIT;

    int deqCount;
    int enqCount;
    int ret;
    int i;

    uint8 * pbuf1 = SerialRamGetBuf(0);
    uint8 * pbuf2 = SerialRamGetBuf(1);

    BufQueueInit();
    memset(pbuf1, 0, SERIAL_RAM_BUFSIZE);
    memset(pbuf2, 0, SERIAL_RAM_BUFSIZE);
    enqCount = 0;
    TEST_ASSERT(BufQueueFree() == QUEUE_SIZE);
    TEST_ASSERT(BufQueueUsed() == 0);

    //
    // Enqueue until full
    //
    while (BufQueueFree() >= SERIAL_RAM_BUFSIZE) {
        for (i = 0; i < SERIAL_RAM_BUFSIZE/sizeof(uint32); i++) {
            ((uint32 *) pbuf1)[i] = enqCount++;
        }
        TEST_ASSERT(EnqueueBytesBlocking(pbuf1, SERIAL_RAM_BUFSIZE) == 0);
    }
    TEST_ASSERT(BufQueueFree() == 0);
    TEST_ASSERT(BufQueueUsed() == QUEUE_SIZE);

    // Test overfilling
    ret = EnqueueBytes(pbuf1, SERIAL_RAM_BUFSIZE, NULL);
    TEST_ASSERT(ret == -ENOSPC);
    deqCount = 0;

    //
    // Dequeue until empty
    //
    while (BufQueueUsed() > 0) {
        TEST_ASSERT(DequeueBytesBlocking(pbuf2, SERIAL_RAM_BUFSIZE) == 0);
        for (i = 0; i < SERIAL_RAM_BUFSIZE/sizeof(uint32); i++) {
            TEST_ASSERT_INT_EQ(((uint32 *) pbuf2)[i], deqCount);
            deqCount++;
        }
    }
    TEST_ASSERT(BufQueueFree() == QUEUE_SIZE);
    TEST_ASSERT(BufQueueUsed() == 0);

    // Test over-dequeueing
    ret = DequeueBytes(pbuf2, SERIAL_RAM_BUFSIZE, NULL);
    TEST_ASSERT(ret == -ENODATA);

    TEST_RETURN;
}

int TestQueueConcurrency()
{
    TEST_INIT;

    int deqCount;
    int i;

    uint8 * pbuf1 = SerialRamGetBuf(0);
    uint8 * pbuf2 = SerialRamGetBuf(1);

    BufQueueInit();
    enqCount = 0;
    deqCount = 0;

    //
    // Fill queue until half full.
    //
    while (BufQueueUsed() < QUEUE_SIZE/2) {
        for (i = 0; i < SERIAL_RAM_BUFSIZE/sizeof(uint32); i++) {
            ((uint32 *) pbuf1)[i] = enqCount++;
        }
        TEST_ASSERT(EnqueueBytesBlocking(pbuf1, SERIAL_RAM_BUFSIZE) == 0);
    }

    //
    // Concurrent fill and empty at same time, with verify.
    // Fill via timer interrupt, to test concurrency.
    //
    prevFree = BufQueueFree();
    Timer_Programmable_SetOneShot(1);
    Timer_Programmable_WritePeriod(enqPeriodUs);
    Timer_Programmable_ISR_StartEx(DebugMemtestIsr);
    Timer_Programmable_Enable();

    while (deqCount < 30000) {
        TEST_ASSERT_INT_EQ(DequeueBytesBlocking(pbuf2, SERIAL_RAM_BUFSIZE), 0);
        for (i = 0; i < SERIAL_RAM_BUFSIZE/sizeof(uint32); i++) {
            TEST_ASSERT_INT_EQ(((uint32 *) pbuf2)[i], deqCount);
            deqCount++;
        }
    }

    Timer_Programmable_ISR_Stop();

    TEST_RETURN;
}

int TestMicDump()
{
    int i;
    int done;
    uint8 * pbuf3 = SerialRamGetBuf(2);

    BufQueueInit();

    I2sStartDma(NULL, NULL);
    while (BufQueueUsed() < 100000);
    I2sStopDma();

    while (DequeueBytes(pbuf3, SERIAL_RAM_BUFSIZE, &done) != -ENODATA) {
        while (!done);
        for (i = 0; i < SERIAL_RAM_BUFSIZE; i += 2) {
            xprintf("%04X\r\n", *((uint16 *) &pbuf3[i]));
        }
    }

    return 0;
}

//
// DISPLAY FUNCTIONS
//

int TestDisplayEraseSpeed()
{
    TEST_INIT;

    int usecs = TimeIt(DisplayErase, 30);
    TEST_ASSERT_PRINT(usecs < 26000, "usecs = %d", usecs);

    TEST_RETURN;
}

void FillWithBytesSame() { DisplayFill(0xFEFE); }
void FillWithBytesDifferent() { DisplayFill(0xFEDC); }
int TestDisplayFillSpeed()
{
    TEST_INIT;
    int usecs;

    //
    // DisplayFill() should take almost exactly the same as DisplayErase(), if
    // the color's high/low bytes are identical (due to an optimization).  If
    // not, then it takes slightly longer.
    //
    usecs = TimeIt(FillWithBytesSame, 30);
    TEST_ASSERT_PRINT(usecs < 26000, "usecs = %d", usecs);
    usecs = TimeIt(FillWithBytesDifferent, 30);
    TEST_ASSERT_PRINT(usecs < 37000, "usecs = %d", usecs);

    TEST_RETURN;
}

int TestDisplayRgbColors()
{
    TEST_INIT;

    TEST_ASSERT_INT_EQ(RGB565(   0,    0,    0), 0x0000);

    TEST_ASSERT_INT_EQ(RGB565(1<<3,    0,    0), 0x0008);
    TEST_ASSERT_INT_EQ(RGB565(1<<4,    0,    0), 0x0010);
    TEST_ASSERT_INT_EQ(RGB565(1<<5,    0,    0), 0x0020);
    TEST_ASSERT_INT_EQ(RGB565(1<<6,    0,    0), 0x0040);
    TEST_ASSERT_INT_EQ(RGB565(1<<7,    0,    0), 0x0080);

    TEST_ASSERT_INT_EQ(RGB565(   0, 1<<2,    0), 0x2000);
    TEST_ASSERT_INT_EQ(RGB565(   0, 1<<3,    0), 0x4000);
    TEST_ASSERT_INT_EQ(RGB565(   0, 1<<4,    0), 0x8000);
    TEST_ASSERT_INT_EQ(RGB565(   0, 1<<5,    0), 0x0001);
    TEST_ASSERT_INT_EQ(RGB565(   0, 1<<6,    0), 0x0002);
    TEST_ASSERT_INT_EQ(RGB565(   0, 1<<7,    0), 0x0004);

    TEST_ASSERT_INT_EQ(RGB565(   0,    0, 1<<3), 0x0100);
    TEST_ASSERT_INT_EQ(RGB565(   0,    0, 1<<4), 0x0200);
    TEST_ASSERT_INT_EQ(RGB565(   0,    0, 1<<5), 0x0400);
    TEST_ASSERT_INT_EQ(RGB565(   0,    0, 1<<6), 0x0800);
    TEST_ASSERT_INT_EQ(RGB565(   0,    0, 1<<7), 0x1000);

    TEST_RETURN;
}

int TestDoRectsIntersect()
{
    TEST_INIT;

    // null rectangles
    TEST_ASSERT(! DoRectsIntersect((RECT) {0, 0, 0, 0}, (RECT) {0, 0, 0, 0}));
    TEST_ASSERT(! DoRectsIntersect((RECT) {0, 0, 0, 0}, (RECT) {10, 10, 0, 0}));

    // Invalid height width
    TEST_ASSERT(! DoRectsIntersect((RECT) {0, 0, -1, -1}, (RECT) {0, 0, -1, -1}));
    TEST_ASSERT(! DoRectsIntersect((RECT) {0, 0, -1, -1}, (RECT) {10, 10, -1, -1}));

    // 10x10 Corners just missing
    TEST_ASSERT(! DoRectsIntersect((RECT) {0, 0, 10, 10}, (RECT) {10, 10, 10, 10}));
    // 10x10 Corners overlapping by 1 pixel
    TEST_ASSERT(DoRectsIntersect((RECT) {0, 0, 10, 10}, (RECT) {9, 9, 10, 10}));
    // 10x10 half side overlapping by 1 pixel
    TEST_ASSERT(DoRectsIntersect((RECT) {0, 0, 10, 10}, (RECT) {9, 5, 10, 10}));
    // 10x10 full side overlapping by 1 pixel
    TEST_ASSERT(DoRectsIntersect((RECT) {0, 0, 10, 10}, (RECT) {9, 0, 10, 10}));
    // 10x10 heavy overlap
    TEST_ASSERT(DoRectsIntersect((RECT) {0, 0, 10, 10}, (RECT) {4, 4, 10, 10}));
    // 10x10 same rect
    TEST_ASSERT(DoRectsIntersect((RECT) {0, 0, 10, 10}, (RECT) {0, 0, 10, 10}));
    // 10x10, rect below
    TEST_ASSERT(! DoRectsIntersect((RECT) {0, 0, 10, 10}, (RECT) {0, 20, 10, 10}));
    // 10x10, rect to the right
    TEST_ASSERT(! DoRectsIntersect((RECT) {0, 0, 10, 10}, (RECT) {20, 0, 10, 10}));
    // 10x10, rect to the right and below
    TEST_ASSERT(! DoRectsIntersect((RECT) {0, 0, 10, 10}, (RECT) {20, 20, 10, 10}));
    
    // Same as above section, rect1/rect2 flipped
    TEST_ASSERT(! DoRectsIntersect((RECT) {10, 10, 10, 10}, (RECT) {0, 0, 10, 10}));
    TEST_ASSERT(DoRectsIntersect((RECT) {9, 9, 10, 10}, (RECT) {0, 0, 10, 10}));
    TEST_ASSERT(DoRectsIntersect((RECT) {9, 5, 10, 10}, (RECT) {0, 0, 10, 10}));
    TEST_ASSERT(DoRectsIntersect((RECT) {9, 0, 10, 10}, (RECT) {0, 0, 10, 10}));
    TEST_ASSERT(DoRectsIntersect((RECT) {4, 4, 10, 10}, (RECT) {0, 0, 10, 10}));
    TEST_ASSERT(DoRectsIntersect((RECT) {0, 0, 10, 10}, (RECT) {0, 0, 10, 10}));
    TEST_ASSERT(! DoRectsIntersect((RECT) {0, 20, 10, 10}, (RECT) {0, 0, 10, 10}));
    TEST_ASSERT(! DoRectsIntersect((RECT) {20, 0, 10, 10}, (RECT) {0, 0, 10, 10}));
    TEST_ASSERT(! DoRectsIntersect((RECT) {20, 20, 10, 10}, (RECT) {0, 0, 10, 10}));

    // Same as original section, for bigger width/height
    TEST_ASSERT(! DoRectsIntersect((RECT) {0, 0, 20, 30}, (RECT) {20, 30, 40, 50}));
    TEST_ASSERT(DoRectsIntersect((RECT) {0, 0, 20, 30}, (RECT) {19, 29, 40, 50}));
    TEST_ASSERT(DoRectsIntersect((RECT) {0, 0, 20, 30}, (RECT) {19, 15, 40, 50}));
    TEST_ASSERT(DoRectsIntersect((RECT) {0, 0, 20, 30}, (RECT) {19, 0, 40, 50}));
    TEST_ASSERT(DoRectsIntersect((RECT) {0, 0, 20, 30}, (RECT) {10, 15, 40, 50}));
    TEST_ASSERT(DoRectsIntersect((RECT) {0, 0, 20, 30}, (RECT) {0, 0, 40, 50}));
    TEST_ASSERT(! DoRectsIntersect((RECT) {0, 0, 20, 30}, (RECT) {0, 40, 40, 50}));
    TEST_ASSERT(! DoRectsIntersect((RECT) {0, 0, 20, 30}, (RECT) {40, 0, 40, 50}));
    TEST_ASSERT(! DoRectsIntersect((RECT) {0, 0, 20, 30}, (RECT) {40, 40, 40, 50}));

    // Same as above section, for offset origin
    TEST_ASSERT(! DoRectsIntersect((RECT) {100, 100, 20, 30}, (RECT) {120, 130, 40, 50}));
    TEST_ASSERT(DoRectsIntersect((RECT) {100, 100, 20, 30}, (RECT) {119, 129, 40, 50}));
    TEST_ASSERT(DoRectsIntersect((RECT) {100, 100, 20, 30}, (RECT) {119, 115, 40, 50}));
    TEST_ASSERT(DoRectsIntersect((RECT) {100, 100, 20, 30}, (RECT) {119, 100, 40, 50}));
    TEST_ASSERT(DoRectsIntersect((RECT) {100, 100, 20, 30}, (RECT) {110, 115, 40, 50}));
    TEST_ASSERT(DoRectsIntersect((RECT) {100, 100, 20, 30}, (RECT) {100, 100, 40, 50}));
    TEST_ASSERT(! DoRectsIntersect((RECT) {100, 100, 20, 30}, (RECT) {100, 140, 40, 50}));
    TEST_ASSERT(! DoRectsIntersect((RECT) {100, 100, 20, 30}, (RECT) {140, 100, 40, 50}));
    TEST_ASSERT(! DoRectsIntersect((RECT) {100, 100, 20, 30}, (RECT) {140, 140, 40, 50}));

    // Same as above section, negative origins
    TEST_ASSERT(! DoRectsIntersect((RECT) {-50, -50, 20, 30}, (RECT) {-30, -20, 40, 50}));
    TEST_ASSERT(DoRectsIntersect((RECT) {-50, -50, 20, 30}, (RECT) {-31, -21, 40, 50}));
    TEST_ASSERT(DoRectsIntersect((RECT) {-50, -50, 20, 30}, (RECT) {-31, -35, 40, 50}));
    TEST_ASSERT(DoRectsIntersect((RECT) {-50, -50, 20, 30}, (RECT) {-31, -50, 40, 50}));
    TEST_ASSERT(DoRectsIntersect((RECT) {-50, -50, 20, 30}, (RECT) {-40, -35, 40, 50}));
    TEST_ASSERT(DoRectsIntersect((RECT) {-50, -50, 20, 30}, (RECT) {-50, -50, 40, 50}));
    TEST_ASSERT(! DoRectsIntersect((RECT) {-50, -50, 20, 30}, (RECT) {-50, -10, 40, 50}));
    TEST_ASSERT(! DoRectsIntersect((RECT) {-50, -50, 20, 30}, (RECT) {-10, -50, 40, 50}));
    TEST_ASSERT(! DoRectsIntersect((RECT) {-50, -50, 20, 30}, (RECT) {-10, -10, 40, 50}));

    TEST_RETURN;
}

int TestRectIntersection()
{
    TEST_INIT;

    RECT r;

    // null rect
    r = RectIntersection((RECT) {0, 0, 0, 0}, (RECT) {0, 0, 0, 0});
    TEST_ASSERT(r.x == 0 && r.y == 0 && r.width == 0 && r.height == 0); 

    // equal rects
    r = RectIntersection((RECT) {10, 20, 30, 40}, (RECT) {10, 20, 30, 40});
    TEST_ASSERT(r.x == 10 && r.y == 20 && r.width == 30 && r.height == 40); 

    // simple rects overlapped
    r = RectIntersection((RECT) {0, 0, 10, 10}, (RECT) {5, 5, 50, 50});
    TEST_ASSERT(r.x == 5 && r.y == 5 && r.width == 5 && r.height == 5); 
    // swapped args
    r = RectIntersection((RECT) {5, 5, 50, 50}, (RECT) {0, 0, 10, 10});
    TEST_ASSERT(r.x == 5 && r.y == 5 && r.width == 5 && r.height == 5); 

    // negative rect
    r = RectIntersection((RECT) {-10, -20, 15, 25}, (RECT) {0, 0, 30, 10});
    TEST_ASSERT(r.x == 0 && r.y == 0 && r.width == 5 && r.height == 5); 

    // rect in rect
    r = RectIntersection((RECT) {10, 20, 100, 120}, (RECT) {30, 40, 10, 20});
    TEST_ASSERT(r.x == 30 && r.y == 40 && r.width == 10 && r.height == 20); 

    TEST_RETURN;
}

int TestExpandedRect()
{
    TEST_INIT;

    RECT r1 = {10, 11, 12, 13};
    RECT r2;

    r2 = ExpandedRect(r1, 0);
    TEST_ASSERT(r2.x == 10 && r2.y == 11 && r2.width == 12 && r2.height == 13); 
    r2 = ExpandedRect(r1, 1);
    TEST_ASSERT(r2.x == 9 && r2.y == 10 && r2.width == 14 && r2.height == 15); 
    r2 = ExpandedRect(r1, 5);
    TEST_ASSERT(r2.x == 5 && r2.y == 6 && r2.width == 22 && r2.height == 23); 

    TEST_RETURN;
}

void CenteredText(
    char * s,
    int y,
    int border
    )
{
    const int font = FONT_5X8;

    if (border < 0) {
        border = 2;
    }

    RECT textRect;
    GetTextDimensions(s, font, &textRect);
    RECT r = (RECT) {
        (SCREEN_WIDTH - textRect.width) / 2, 
        y >= 0 ? y : (SCREEN_HEIGHT - textRect.height) / 2,
        textRect.width,
        textRect.height
    };


    RECT borderRect = ExpandedRect(r, border);
    DrawRect(borderRect, BLACK);
    DrawText(s, r.x, r.y, font, WHITE, BLACK);
}

void TestDisplayUpperLeftCorner()
{
    int font = FONT_5X5;
    const char * text[] = { "UPPER", "LEFT", "CORNER" };
    RECT r;

    DisplayErase();

    GetTextDimensions(NULL, font, &r);
    for (int line = 0; line < ARRAY_SIZEOF(text); line++) {
        DrawText((char *) text[line], 0, line * (r.height + 1), font, WHITE, BLACK);
    }
    CyDelay(MTEST_DELAY_FAST);
}

void TestDisplayFill(
    char * colorName,
    int color
    )
{
    DisplayFill(color);

    char s[32];
    sprintf(s, "%s FILL", colorName);
    CenteredText(s, -1, -1);

    CyDelay(MTEST_DELAY_FAST);
}

void TestDrawLine()
{
    // Horizontal
    // Vertical
    // diagonal
    // colors
    // OOB
    // overlapping
    // on top of colored rectangle
    CyDelay(MTEST_DELAY);
}

void TestDrawRect()
{
    // OOB
    // Colors
    // Overlapping
    // Points may be flipped
    CyDelay(MTEST_DELAY);
}

void TestDrawImage()
{
    CyDelay(MTEST_DELAY);
}

void TestDisplayScrollUp()
{
    CyDelay(MTEST_DELAY);
}

void TestDisplayScrollDown()
{
    CyDelay(MTEST_DELAY);
}

void TestTextBoxFullScreen()
{
    int shiftUp = 4;
    int justify = 0;
    char * lines[] = {
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",
        "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC",
        "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD",
        "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE",
        "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",
        "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG",
        "HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH",
        "IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII",
        "JJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJ",
        "KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK",
    };

    //
    // Title
    //
    DisplayErase();

    DrawTextBox(
        lines, ARRAY_SIZEOF(lines), SCREEN_BOUNDS, shiftUp, justify, FONT_5X8, WHITE, BLACK
        );

    CenteredText("DrawTextBox()", 35, 4);
    CenteredText("COVERING SCREEN", 50, 4);

    CyDelay(MTEST_DELAY);
}

void TestTextBox()
{
    RECT r;
    RECT borderRect;
    char * llines[] = {
        "Right",
        "Justified",
        "OOB",
        "TextBox",
        "0123456789ABCDEF",
        "0123456789ABCDEF",
        "0123456789ABCDEF",
        "0123456789ABCDEF",
    };
    char * clines[] = {
        "Center",
        "Justified",
        "OOB Box",
        "0123456789ABCDEF",
        "0123456789ABCDEF",
        "0123456789ABCDEF",
    };
    char * rlines[] = {
        "Left",
        "Justified",
        "TextBox",
    };

    //
    // Title
    //
    DisplayErase();
    CenteredText("Overlap'd TextBoxes", 0, 0);

    //
    // Center Justified Text Box
    //
    r = (RECT) {30, 55, 70, 70};
    borderRect = ExpandedRect(r, 1);
    DrawRect(borderRect, GREEN);
    DrawTextBox(clines, ARRAY_SIZEOF(clines), r, 0, CENTER_JUSTIFIED, FONT_5X8, WHITE, BLACK);

    //
    // Left Text Box (Right Justified)
    //
    r = (RECT) {-20, 30, 60, 50};
    borderRect = ExpandedRect(r, 1);
    DrawRect(borderRect, RED);
    DrawTextBox(llines, ARRAY_SIZEOF(llines), r, 0, RIGHT_JUSTIFIED, FONT_5X8, WHITE, BLACK);

    //
    // Right Text Box (Left Justified)
    //
    r = (RECT) {80, 20, 46, 50};
    borderRect = ExpandedRect(r, 1);
    DrawRect(borderRect, BLUE);
    DrawTextBox(rlines, ARRAY_SIZEOF(rlines), r, 0, LEFT_JUSTIFIED, FONT_5X8, WHITE, BLACK);

    CyDelay(MTEST_DELAY);
}

void TestTextBoxScrolling()
{
    // Full screen scrolling, full text
}

void TestColors()
{
    RECT r = {0, 0, 8, SCREEN_HEIGHT};

    DisplayErase();

    // RED
    for (int i = 0, intensity = 0x80; i < 5; i++, intensity >>= 1) {
        DrawRect(r, RGB565(intensity, 0x00, 0x00));
        r.x += 8;
    }

    // GREEN
    for (int i = 0, intensity = 0x80; i < 6; i++, intensity >>= 1) {
        DrawRect(r, RGB565(0x00, intensity, 0x00));
        r.x += 8;
    }

    // BLUE
    for (int i = 0, intensity = 0x80; i < 5; i++, intensity >>= 1) {
        DrawRect(r, RGB565(0x00, 0x00, intensity));
        r.x += 8;
    }

    CenteredText("RGB COLOR BARS", -1, -1);

    CyDelay(MTEST_DELAY);
}

void TestFont(
    int font
    )
{
    const char LINE_LEN = 16;
    char s[LINE_LEN+1];
    RECT r;

    DisplayErase();

    GetTextDimensions(NULL, font, &r);
    for (int line = 0; line < 128/LINE_LEN; line++) {
        for (int j = 0; j < LINE_LEN; j++) {
            s[j] = line * LINE_LEN + j;
        }
        DrawText(s, 0, line * (r.height + 1), font, WHITE, BLACK);
    }

    CyDelay(MTEST_DELAY);
}

void TestDrawText()
{
    int font = FONT_5X8;
    RECT r;
    char * s;

    DisplayErase();

    //
    // Title
    //
    r = (RECT) {10, 10};
    DrawText("DrawText()", r.x, r.y, font, WHITE, BLACK);

    //
    // Bordered text
    //
    s = "Tight red border";
    GetTextDimensions(s, font, &r);
    r.y += 10;
    RECT borderRect = ExpandedRect(r, 1);
    DrawRect(borderRect, RED);
    DrawText(s, r.x, r.y, font, WHITE, BLACK);

    //
    // Inverted characters - TBD
    //

    //
    // FG/BG colors
    //
    r.y = 55;
    DrawText("RED on GREEN", r.x, r.y, font, RED, GREEN);
    r.y += 10;
    DrawText("YELLOW on BLUE", r.x, r.y, font, YELLOW, BLUE);
    r.y += 10;
    DrawText("ORANGE on PURPLE", r.x, r.y, font, ORANGE, PURPLE);

    CyDelay(MTEST_DELAY);
}

void TestDrawTextOobEdges()
{
    int font = FONT_5X8;

    DisplayErase();

    //
    // Title
    //
    CenteredText("DrawText()", 30, -1);
    CenteredText("OOB", 40, -1);
    CenteredText("AT EDGES", 50, -1);
    CenteredText("12345", 60, -1);

    //
    // OOB coordinates
    //
    char * s = "12345";
    RECT textRect;
    GetTextDimensions(s, font, &textRect);
    int hCenter = (SCREEN_WIDTH - textRect.width)/2;
    int vCenter = (SCREEN_HEIGHT - textRect.height)/2;
    int left = -textRect.width/2;
    int top = -textRect.height/2;
    int right = SCREEN_WIDTH - textRect.width/2;
    int bottom = SCREEN_HEIGHT - textRect.height/2;

    //
    // OOB at top/bottom edge
    //
    DrawText(s, hCenter,    top,        font, WHITE, BLACK);
    DrawText(s, hCenter,    bottom,     font, WHITE, BLACK);

    //
    // OOB at left/right edge, various shifts
    //
    for (int i = -4; i <= 4; i++) {
        DrawText(s, left + i,  vCenter + i*9, font, WHITE, BLACK);
        DrawText(s, right + i,  vCenter + i*9, font, WHITE, BLACK);
    }

    //
    // OOB at corners
    //
    DrawText(s, left,       top,        font, WHITE, BLACK);
    DrawText(s, left,       bottom,     font, WHITE, BLACK);
    DrawText(s, right,      top,        font, WHITE, BLACK);
    DrawText(s, right,      bottom,     font, WHITE, BLACK);

    CyDelay(MTEST_DELAY_SLOW);
}

void TestDrawTextOob()
{
    int font = FONT_5X8;

    DisplayErase();

    //
    // Title
    //
    CenteredText("DrawText() OOB", 30, -1);
    CenteredText("SHOULD BE", 40, -1);
    CenteredText("OTHERWISE", 50, -1);
    CenteredText("BLANK", 60, -1);

    char * s = "12345";

    //
    // OOB text to the left
    //
    DrawText(s, -100, -100, font, WHITE, BLACK);
    DrawText(s, -100, -4, font, WHITE, BLACK);
    DrawText(s, -100, 50, font, WHITE, BLACK);
    DrawText(s, -100, SCREEN_HEIGHT - 4, font, WHITE, BLACK);

    //
    // OOB text to the right
    //
    DrawText(s, 200, -100, font, WHITE, BLACK);
    DrawText(s, 200, -4, font, WHITE, BLACK);
    DrawText(s, 200, 50, font, WHITE, BLACK);
    DrawText(s, 200, SCREEN_HEIGHT - 4, font, WHITE, BLACK);

    //
    // OOB text above the top
    //
    DrawText(s, -10,                -100, font, WHITE, BLACK);
    DrawText(s, 50,                 -100, font, WHITE, BLACK);
    DrawText(s, SCREEN_WIDTH - 10,  -100, font, WHITE, BLACK);

    //
    // OOB text below the bottom
    //
    DrawText(s, -10,                200, font, WHITE, BLACK);
    DrawText(s, 50,                 200, font, WHITE, BLACK);
    DrawText(s, SCREEN_WIDTH - 10,  200, font, WHITE, BLACK);

    CyDelay(MTEST_DELAY_FAST);
}

/////////////////////////////////////////////////////////////////////
//
// Test Suite
//
/////////////////////////////////////////////////////////////////////

void test()
{
    DrawTextBounded("0123456789ABCDEF", 0, 0, FONT_5X8, WHITE, BLACK, (RECT){0, 0, 100, 100});
}

void TestSuite()
{
    xprintf("==== TEST SUITE RUNNING @ %s, %s ====\r\n", __DATE__, __TIME__);

    TEST(TestSpiXferInterrupts(TEST_SPI_XFER_TX, 1));
    TEST(TestSpiXferInterrupts(TEST_SPI_XFER_TX, 5));
    TEST(TestSpiXferInterrupts(TEST_SPI_XFER_RX, 1));
    TEST(TestSpiXferInterrupts(TEST_SPI_XFER_RX, 5));
    // Test TX after RX, as it was previously failing
    TEST(TestSpiXferInterrupts(TEST_SPI_XFER_TX, 1));

    TEST(TestSpiXferTooMany());

    TEST(TestSpiXferWriteReadMem(TEST_SPI_XFER_UNLOCKED));
    TEST(TestSpiXferWriteReadMem(TEST_SPI_XFER_LOCKED));

    TEST(TestSerialRamFuncs());
    TEST(TestSerialRamWriteRead());
    TEST(TestSerialRamFillMem());
    TEST(TestSerialRamXfersOverlapping());

    TEST(TestQueueEnqueueOneDequeueOne());
    TEST(TestQueueEnqueueDequeueVariableSizes());
    TEST(TestQueueFillThenEmpty());
    TEST(TestQueueConcurrency());

    TEST(TestDisplayRgbColors());
    TEST(TestDoRectsIntersect());
    TEST(TestRectIntersection());
    TEST(TestExpandedRect());

    TEST(TestDisplayEraseSpeed());
    TEST(TestDisplayFillSpeed());

    MTEST(TestDisplayUpperLeftCorner());
    MTEST(TestDisplayFill("RED", RED));
    MTEST(TestDisplayFill("BLUE", BLUE));
    MTEST(TestDisplayFill("GREEN", GREEN));
#if 0
    MTEST(TestDisplayScrollUp());
    MTEST(TestDisplayScrollDown());
#endif

    MTEST(TestDrawText());
    MTEST(TestDrawTextOobEdges());
    MTEST(TestDrawTextOob());
    MTEST(TestTextBoxScrolling());
    MTEST(TestDrawRect());
    MTEST(TestColors());
    MTEST(TestTextBoxFullScreen());
    MTEST(TestTextBox());
#if 0
    MTEST(TestDrawLine());
    MTEST(TestDrawImage());
    MTEST(TestFont(FONT_5X5));
    MTEST(TestFont(FONT_5X8));
    MTEST(TestDrawStatusBar());
#endif

    xprintf("==== TEST SUITE DONE @ %s, %s ====\r\n", __DATE__, __TIME__);
}
