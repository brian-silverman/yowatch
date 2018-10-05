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

#define TICKS_PER_PERIOD_MS 65000

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
void TickInit()
{
    TimerMillisecIsr_StartEx(TimerMillisecIsr);

    TimerMillisec_Init();
    TimerMillisec_SetOneShot(0);
    TimerMillisec_WritePeriod(TICKS_PER_PERIOD_MS - 1);
    TimerMillisec_WriteCompare(TICKS_PER_PERIOD_MS - 1);
    TimerMillisec_WriteCounter(0);
    TimerMillisec_Enable();
}

uint GetTick()
{
    return periodCount * TICKS_PER_PERIOD_MS + TimerMillisec_ReadCounter() % TICKS_PER_PERIOD_MS;
}


