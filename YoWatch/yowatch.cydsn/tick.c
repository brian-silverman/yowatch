/*
 * tick.c
 *
 * Freerunning millisecond tick counter.
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
#include "util.h"

uint periodCount = 0;

CY_ISR(TimerMillisecIsr)
{
    TimerMillisec_ClearInterrupt(TimerMillisec_GetInterruptSource());
    periodCount++;
}

//
// Init Tick API
//
// Currently using a 1kHz clock for the counter, based off of the IMO clock.
// But it only has an accuracy of +/- 2% (.3% for current device under test).
// Could be greatly improved if it ran off the +/- 20ppm LFCLK.
//
// What's worse, it appears a TCPWM block won't run in deep sleep.  It may be
// possible, though, to run the WDT off of the LFCLK, which will run in deep
// sleep.
//
void TickInit(
    )
{
    TimerMillisecIsr_StartEx(TimerMillisecIsr);

    TimerMillisec_Init();
    TimerMillisec_SetOneShot(0);
    TimerMillisec_WritePeriod(0);
    TimerMillisec_WriteCompare(0);
    TimerMillisec_WriteCounter(0);
    TimerMillisec_Enable();
}

uint GetTick(
    )
{
    return periodCount;
}

int Expired(
    uint msec,
    uint * since
    )
{
    uint now = GetTick();
    int elapsed = now - *since;
    assert_printf(elapsed > 0, "%d, %d, %d\r\n", elapsed, now, *since);
    if (elapsed > msec) {
        //
        // If we've missed a whole period since last test, then just jump right
        // to now.  (This is uncommon, occurs if period is too short, or if
        // there's a long wait on startup from testing).
        //
        // Otherwise, we adjust the "since" time to be more exact (now minus
        // any extra time) to reduce drift.
        //
        if (elapsed > 2 * msec) {
            *since = now;
        } else {
            *since = now - (elapsed - msec);
        }
        return 1;
    }
    return 0;
}
    
