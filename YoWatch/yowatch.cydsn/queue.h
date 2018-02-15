#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <project.h>
#include "serialram.h"

#define QUEUE_SIZE (SERIAL_RAM_SIZE)

void BufQueueInit();

int BufQueueFree();

int BufQueueUsed();

int EnqueueBytes(
    uint8 * buf,
    uint32 bytes,
    int * pDone
    );

int EnqueueBytesBlocking(
    uint8 * buf,
    uint32 bytes
    );

int DequeueBytes(
    uint8 * buf,
    uint32 bytes,
    int * pDone
    );

int DequeueBytesBlocking(
    uint8 * buf,
    uint32 bytes
    );

#endif
