/*
 * queue.c
 *
 * Queueing bytes to/from serial RAM.
 *
 * Copyright (C) 2018 Brian Silverman <bri@readysetstem.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 */
#include <project.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "printf.h"
#include "assert.h"
#include "serialram.h"
#include "queue.h"

//
// Serial RAM buffer queue (circular)
//
// Queue starts with a count of QUEUE_SIZE bytes free.  
// When NO queue transactions are pending:
//      used + free = QUEUE_SIZE.  
// When queue transactions ARE pending:
//      used + free + pending*Bytes = QUEUE_SIZE.  
// Queue starts with head = tail = 0, and bytes are added to tail, which
// increments tail.  Pending bytes are added/removed from queue when DMA
// completes.
//
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

void BufQueueInit()
{
    bufq.pendingTail = 0;
    bufq.pendingHead = 0;
    bufq.tail = 0;
    bufq.head = 0;
    bufq.used = 0;
    bufq.free = QUEUE_SIZE;
}

int BufQueueFree()
{
    return bufq.free;
}

int BufQueueUsed()
{
    return bufq.used;
}

//
// Completion routine for EnqueueBytes
//
static void EnqueueBytesDone(
    void * pDone
    )
{
    bufq.tail = bufq.pendingTail;
    bufq.used += bufq.pendingTailBytes;
    bufq.pendingTailBytes = 0;

    if (pDone) {
        *((int *) pDone) = 1;
    }
}

//
// Enqueue given buffer to SPI Serial RAM via DMA
//
// The buffer is added to the queue tail, but the tail will only be updated
// once the DMA completes.
//
// @param buf       buffer to be enqueued to Serial RAM, mus have been created
//                  with SerialRamGetBuf()
// @param bytes     size in bytes of /buf/
// @param pDone     flag that will be set when DMA completes
//
// @return 0 on success, or negative value on error.  -EAGAIN indicates call has
// been queued, will be recalled when running DMA is complete.
//
int EnqueueBytes(
    uint8 * buf,
    uint32 bytes,
    int * pDone
    )
{
    uint8 interruptState;
    uint32 tail;
    int ret = 0;

    interruptState = CyEnterCriticalSection();

    if (bufq.free < bytes) ret = -ENOSPC;

    if (!ret) {
        assert(bufq.free >= bytes);
        assert(bufq.tail == bufq.pendingTail);
        tail = bufq.tail;
        bufq.pendingTail = (bufq.tail + bytes) % QUEUE_SIZE;
        assert(bufq.free >= bytes);
        bufq.free -= bytes;
        bufq.pendingTailBytes = bytes;

        if (pDone != NULL) {
            *pDone = 0;
        }
    }

    CyExitCriticalSection(interruptState);

    if (ret) return ret;

    return SerialRamWrite(buf, tail, bytes, EnqueueBytesDone, pDone);
}

//
// Blocking call to EnqueueBytes()
//
// @return 0 on success, or negative value on error.
//
int EnqueueBytesBlocking(
    uint8 * buf,
    uint32 bytes
    )
{
    int done;
    int ret = EnqueueBytes(buf, bytes, &done);
    if (ret != 0 && ret != -EAGAIN) return ret;
    while (!done);
    return 0;
}

//
// Completion routine for DequeueBytes
//
static void DequeueBytesDone(
    void * pDone
    )
{
    bufq.head = bufq.pendingHead;
    bufq.free += bufq.pendingHeadBytes;
    bufq.pendingHeadBytes = 0;

    if (pDone) {
        *((int *) pDone) = 1;
    }
}

//
// Dequeue given buffer from SPI Serial RAM via DMA
//
// The buffer is removed from the queue head, but the head will only be updated
// once the DMA completes.
//
// @param buf       buffer to be dequeued to Serial RAM, mus have been created
//                  with SerialRamGetBuf()
// @param bytes     size in bytes of /buf/
// @param pDone     flag that will be set when DMA completes
//
// @return 0 on success, or negative value on error.  -EAGAIN indicates call has
// been queued, will be recalled when running DMA is complete.
//
int DequeueBytes(
    uint8 * buf,
    uint32 bytes,
    int * pDone
    )
{
    uint8 interruptState;
    uint32 head;

    if (bufq.used < bytes) return -ENODATA;

    interruptState = CyEnterCriticalSection();

    assert(bufq.used >= bytes);
    assert(bufq.head == bufq.pendingHead);
    head = bufq.head;
    bufq.pendingHead = (bufq.head + bytes) % QUEUE_SIZE;
    assert(bufq.used >= bytes);
    bufq.used -= bytes;
    bufq.pendingHeadBytes = bytes;

    if (pDone != NULL) {
        *pDone = 0;
    }

    CyExitCriticalSection(interruptState);

    return SerialRamRead(buf, head, bytes, DequeueBytesDone, pDone);
}

//
// Blocking call to DequeueBytes()
//
// @return 0 on success, or negative value on error.
//
int DequeueBytesBlocking(
    uint8 * buf,
    uint32 bytes
    )
{
    int done;
    int ret = DequeueBytes(buf, bytes, &done);
    if (ret != 0 && ret != -EAGAIN) return ret;
    while (!done);
    return 0;
}

