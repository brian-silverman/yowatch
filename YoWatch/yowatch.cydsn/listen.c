/*
 * listen.c
 *
 * Before being streamed over BLE, audio is buffered in a queue for Voice
 * Activity Detection.
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
#include "listen.h"
#include "serialram.h"
#include "util.h"
#include "queue.h"
#include "i2s.h"

static uint8 * circQ[LISTEN_QUEUE_NUM_BUFS];
static uint varianceQ[LISTEN_QUEUE_NUM_BUFS];
static int head;
static int tail;
static int len;

int ListenActivity(
    )
{
    int ret = 0;

    //
    // Dequeue all bufs from SRAM until empty.  If listen queue is full, sill
    // keep adding (head is lost).  This keeps listen queue always full with
    // latest audio bufs.
    //
    for (;;) {
        ret = DequeueBytesBlocking(circQ[tail], SERIAL_RAM_BUFSIZE);
        if (ret == -ENODATA) break;

        varianceQ[tail] = AudioBufVariance(circQ[tail]);

        tail = (tail + 1) % LISTEN_QUEUE_NUM_BUFS;
        if (len < LISTEN_QUEUE_NUM_BUFS) {
            len++;
        } else {
            assert((head + 1) % LISTEN_QUEUE_NUM_BUFS == tail);
            head = tail;
        }
    }

    if (len == LISTEN_QUEUE_NUM_BUFS) {
        int aboveLowThreshold = 0;
        int aboveHighThreshold = 0;
        for (int j = 0; j < LISTEN_QUEUE_NUM_BUFS; j++) {
            if (varianceQ[j] > LISTEN_LOW_THRESHOLD) aboveLowThreshold++;
            if (varianceQ[j] > LISTEN_HIGH_THRESHOLD) aboveHighThreshold++;
        }
        return aboveLowThreshold > LISTEN_MIN_BUFS_ABOVE_LOW_THRESHOLD
            && aboveHighThreshold > LISTEN_MIN_BUFS_ABOVE_HIGH_THRESHOLD;
    }

    return 0;
}

int ListenTimeout(
    )
{
    return 0;
}

uint ListenLevel(
    )
{
    if (len < LISTEN_QUEUE_NUM_BUFS) return 0;

    uint sum = 0;
    for (int i = 0; i < LISTEN_QUEUE_NUM_BUFS; i++) {
        sum += varianceQ[i];
    }

    return sum/LISTEN_QUEUE_NUM_BUFS;
}

void ListenStart(
    )
{
    int i;

    for (i = 0; i < LISTEN_QUEUE_NUM_BUFS; i++) {
        circQ[i] = SerialRamGetBuf(FIRST_LISTEN_QUEUE_BUF + i);
    }
    head = tail = len = 0;
    I2sStartDma(NULL, NULL);
}

void ListenStop(
    )
{
    I2sStopDma();
}
