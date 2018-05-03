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

#define TEST_RETURN \
    { \
        if (testAssertFails) xprintf("\r\n"); \
        return testAssertFails; \
    }

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

void TestDisplayUpperLeftCorner()
{
}

void TestDisplayFill()
{
}

void TestDisplayHline()
{
}

void TestDisplayVline()
{
}

void TestDisplayLine()
{
}

void TestDisplayImage()
{
}

void TestDisplayText()
{
}

void TestDisplayScrollUp()
{
}

void TestDisplayScrollDown()
{
}

void TestTextBox()
{
    int fgColor = 0;
    int bgColor = 0;
    int shiftUp = 0;
    int justify = 0;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    char * lines[] = {
        "This is the first line",
        "Second line",
        "This line is long 1234567890",
        "Four",
        "Five",
        "Six",
        "Seven",
        "Eight",
        "Nine",
        "Ten",
        "Eleven",
        "Twelve",
    };

    TextBoxDraw(lines, x, y, width, height, shiftUp, justify, fgColor, bgColor);
}

void TestTextBoxSmall()
{
    int fgColor = 0;
    int bgColor = 0;
    int shiftUp = 0;
    int justify = 0;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    char * lines[] = {
        "TOP LINE",
        "TRIMMED BOX",
        "Six",
        "Seven",
        "Eight",
        "Nine",
        "Ten",
        "Eleven",
        "Twelve",
    };

    TextBoxDraw(lines, x, y, width, height, shiftUp, justify, fgColor, bgColor);
}

void TestTextBoxScrolling()
{
}

void TestListBox()
{
}

#define TEST_DISPLAY_DELAY      (2000)
void TestDisplayRect()
{
    int i;
    DisplayErase();
    for (i = 0; i < 16; i++) {
        DisplayRect(8*i, 0, 8, 96, 1 << i);
    }
    CyDelay(TEST_DISPLAY_DELAY);
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

void test() { DisplayText("ABCDEFGHIJKLMNOPQRST", 0, 10, FONT_5X8, 0, 0); }

void TestDisplayChar()
{
#if 0
    int i;
#endif
    DisplayErase();

    //TimeIt(DisplayErase, 0);
#if 0
    DisplayText("ABCDEFGHIJKLMNOP -=_+", 0, 10, FONT_5X5, 0, 0);
#endif
#if 0
    for (i = 0; i < 16; i++) {
        DisplayRect(8*i, 0, 8, 96, 1 << i);
    }
#endif
#if 0
    for (int i = 0; i < 1000; i++) {
        DisplayRect(0, 10, 128, 10, 0);
        DisplayText("QRSTUVWXYZ !@#$% []{}", 0, 10, FONT_5X8, 0, 0);
        CyDelay(50);
        DisplayRect(0, 20, 128, 10, 0);
        DisplayText("abcdefghijklmnop |\\/?", 0, 20, FONT_5X8, 0, 0);
        CyDelay(50);
    }
#endif
    DisplayText("ABCDEFGHIJKLMNOP -=_+", 0, 10, FONT_5X5, 0, 0);
    DisplayText("QRSTUVWXYZ !@#$% []{}", 0, 20, FONT_5X8, 0, 0);
    DisplayText("QRSTUVWXYZ !@#$% []{}", 0, 30, FONT_5X8, 0, 0);
    //DisplayText("F", 0, 10, FONT_5X5, 0, 0);
    //DisplayText("ABCDEFGHIJKLMNOP -=_+", 0, 10, FONT_5X5, 0, 0);
#if 0
    DisplayText("ABCDEFGHIJKLMNOP -=_+", 0, 10, FONT_5X5, 0, 0);
    DisplayText("QRSTUVWXYZ !@#$% []{}", 0, 20, FONT_5X8, 0, 0);
    DisplayText("abcdefghijklmnop |\\/?", 0, 30, FONT_5X8, 0, 0);
    DisplayText("qrstuvwxyz ^&*() <>,.", 0, 40, FONT_5X8, 0, 0);
    DisplayText("ABCDEFGHIJKLMNOP -=_+", 0, 50, FONT_5X8_FIXED, 0, 0);
    DisplayText("QRSTUVWXYZ !@#$% []{}", 0, 60, FONT_5X8_FIXED, 0, 0);
    DisplayText("abcdefghijklmnop |\\/?", 0, 70, FONT_5X8_FIXED, 0, 0);
    DisplayText("qrstuvwxyz ^&*() <>,.", 0, 80, FONT_5X8_FIXED, 0, 0);
#endif
    CyDelay(TEST_DISPLAY_DELAY);
}

/////////////////////////////////////////////////////////////////////
//
// Test Suite
//
/////////////////////////////////////////////////////////////////////

void TestSuite()
{
    xprintf("==== TEST SUITE RUNNING @ %s, %s ====\r\n", __DATE__, __TIME__);

    TEST(TestSpiXferInterrupts(TEST_SPI_XFER_TX, 1));
    TEST(TestSpiXferInterrupts(TEST_SPI_XFER_TX, 5));
    TEST(TestSpiXferInterrupts(TEST_SPI_XFER_RX, 1));
    TEST(TestSpiXferInterrupts(TEST_SPI_XFER_RX, 5));
    // Test TX after RX, as it was previously failing
    TEST(TestSpiXferInterrupts(TEST_SPI_XFER_TX, 1));

#if 0
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
#endif

    TEST(TestDisplayRgbColors());
    MTEST(TestDisplayChar());

    MTEST(TestDisplayUpperLeftCorner());
    MTEST(TestDisplayFill(RED));
    MTEST(TestDisplayFill(BLUE));
    MTEST(TestDisplayFill(GREEN));
    MTEST(TestDisplayHline());
    MTEST(TestDisplayVline());
    MTEST(TestDisplayLine());
    MTEST(TestDisplayImage());
    MTEST(TestDisplayText());
    // ??? MTEST(TestDisplayImageTiling());
    MTEST(TestDisplayScrollUp());
    MTEST(TestDisplayScrollDown());
    MTEST(TestDisplayRect());
    MTEST(TestTextBox());
    MTEST(TestTextBoxScrolling());
    MTEST(TestListBox());

    xprintf("==== TEST SUITE DONE @ %s, %s ====\r\n", __DATE__, __TIME__);
}
