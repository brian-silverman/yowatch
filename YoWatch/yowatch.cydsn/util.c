/*
 * util.c
 *
 * Copyright (C) 2018 Brian Silverman <bri@readysetstem.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 */
#include <project.h>
#include "printf.h"
#include "assert.h"
#include "serialram.h"

#define SAMPLES ((int) (SERIAL_RAM_BUFSIZE / sizeof(int16)))

int AudioBufMean(
    uint8 * pbuf
    )
{
    int16 * psamples = (int16 *) pbuf;
    int sum = 0;
    for (int i = 0; i < SAMPLES; i++) {
        sum += psamples[i];
    }
    return sum/SAMPLES;
}

uint AudioBufVariance(
    uint8 * pbuf
    )
{
    int16 * psamples = (int16 *) pbuf;
    int mean = AudioBufMean(pbuf);

    uint sum = 0;
    for (int i = 0; i < SAMPLES; i++) {
        int n = psamples[i] - mean;
        sum += n * n;
    }
    return sum/SAMPLES;
}

