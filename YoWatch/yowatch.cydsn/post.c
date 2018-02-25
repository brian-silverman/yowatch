/*
 * post.c
 *
 * Power On Self Test
 *
 * Copyright (C) 2018 Brian Silverman <bri@readysetstem.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 */
#include <project.h>
#include "printf.h"
#include "test.h"
#include "version.h"
#include "assert.h"
#include "serialram.h"
#include "i2s.h"

#define PASS_PREFIX "  PASS: "
#define FAIL_PREFIX "**FAIL: "

void MicrophonePost(
    )
{
    int i;
    int high, low, other;
    uint16 currMsbs, currLsbs, prevMsbs, prevLsbs;
    int lsbChange, msbChange;
    int fail = 0;
    uint32 savedTimerPeriod;
    uint8 * pbuf1 = SerialRamGetBuf(0);
    
    savedTimerPeriod = Timer_1_ReadPeriod();

    //
    // Sample MSBs should filp very little, LSBs should flip a lot.
    //
    I2sGetBuf(pbuf1);
    I2sGetBuf(pbuf1);
    currMsbs = prevMsbs = 0;
    currLsbs = prevLsbs = 0;
    msbChange = lsbChange = 0;
    for (i = 0; i < 0x100; i += 2) {
        uint16 sample = *((uint16 *) &pbuf1[i]);
        currMsbs = sample & 0xC000;
        currLsbs = sample & 0x0003;
        if (currMsbs != prevMsbs) msbChange++;
        if (currLsbs != prevLsbs) lsbChange++;
        prevMsbs = currMsbs;
        prevLsbs = currLsbs;
    }
    if (lsbChange < 50) {
        xprintf("  LSBs not flippy\r\n");
        fail++;
    }
    if (msbChange > 50) {
        xprintf("  MSBs too flippy\r\n");
        fail++;
    }
    
    //
    // The Word Clock is shifted via the TimerCounter register in the hardware
    // design, in order to align the low 16-bits of the sample (from the total
    // of 18 bits per sample).
    //
    // If we shift the word clock back by 15 bits, it should put the LSB up at
    // the MSB - we should only read 0x8000 and 0x0000, and they should be
    // about half the time for each.
    //
    Timer_1_WritePeriod(savedTimerPeriod - 15);
    I2sGetBuf(pbuf1);
    I2sGetBuf(pbuf1);
    high = low = other = 0;
    for (i = 0; i < 0x100; i += 2) {
        switch (*((uint16 *) &pbuf1[i])) {
            case 0x8000:    high++;     break;
            case 0x0000:    low++;      break;
            default:        other++;    break;
        }
    }
    if (((high * 100) / (high + low)) < 25) {
        xprintf("  Not enough high LSBs\r\n");
        fail++;
    }
    if (((low * 100) / (high + low)) < 25) {
        xprintf("  Not enough low LSBs\r\n");
        fail++;
    }
    if (other > 0) {
        xprintf("  Some low bits set (%d)\r\n", other);
        fail++;
    }

    //
    // This time, shift word clock by 16 bits, which should completely shift
    // the sample data out of range.  We should expect all zeroes.
    //
    Timer_1_WritePeriod(savedTimerPeriod - 16);
    I2sGetBuf(pbuf1);
    I2sGetBuf(pbuf1);
    other = 0;
    for (i = 0; i < 0x100; i += 2) {
        if (*((uint16 *) &pbuf1[i]) != 0x0000) other++;
    }
    if (other > 0) {
        xprintf("  All samples not 0 (%d)\r\n", other);
        fail++;
    }

    xprintf("%sMicrophone\r\n", fail ? FAIL_PREFIX : PASS_PREFIX);

    //
    // Restore Timer Period
    //
    Timer_1_WritePeriod(savedTimerPeriod);
}

//
// Memory is tested during SerialRamInit
//
void MemoryPost(
    int memIndex
    )
{
    xprintf("%sMemory %d\r\n", SerialRamMemExists(memIndex) ? PASS_PREFIX : FAIL_PREFIX, memIndex);
}

void AccelerometerPost(
    )
{
    xprintf("%sAccel (TBD)\r\n", FAIL_PREFIX);
}

void VibPost(
    )
{
    xprintf("%sVibrator (TBD)\r\n", FAIL_PREFIX);
}

void Post(
    )
{
    //
    // Start console mode - make all xprintf's go to Display AND UART.
    //
    xprintf("YoWatch Version %s\r\n", GetVersionStr());
    xprintf("Power On Self Test:\r\n");
    MicrophonePost();
    MemoryPost(0);
    MemoryPost(1);
    AccelerometerPost();
    VibPost();

    xprintf("Total Memory: %dk\r\n", (int) (SerialRamTotalSize()/1024));

    TestSuite();
}
