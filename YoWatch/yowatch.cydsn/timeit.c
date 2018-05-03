/*
 * timeit.c
 *
 * Profiling/Timing functions
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

#define DEFAULT_RUNS 1000

//
// Time given func by running /runs/ times (if given, else DEFAULT_RUNS).
// Print the average time per run.
//
void TimeIt(
    void (*func)(),
    int runs
    )
{
    int msecs;

    if (runs == 0) {
        runs = DEFAULT_RUNS;
    }

    TimerMillisec_Start();
    TimerMillisec_Stop();
    TimerMillisec_WriteCounter(0);
    xprintf("TIMEIT START\r\n");

    TimerMillisec_Enable();
    for (int i = 0; i < runs; i++) {
        func();
    }
    msecs = TimerMillisec_ReadCounter();

    if (TimerMillisec_ReadStatus() & TimerMillisec_STATUS_RUNNING) {
        xprintf(
            "TIMEIT DONE: total time %d.%03d seconds for %d runs.\r\n", 
            msecs / 1000,
            msecs % 1000,
            runs
            );
        xprintf("TIMEIT: %d microseconds per run.\r\n", (msecs * 1000) / runs);
    } else {
        xprintf("TIMEIT DONE: ERROR timer overflow - test too long\r\n");
    }
}

