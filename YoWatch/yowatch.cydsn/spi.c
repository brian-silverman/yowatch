/*
 * spi.c
 *
 * SPI Xfer functions
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
#include "spi.h"

//
// SPI bus is shared among devices.  As such, it must be locked to be used
// (unless the user is sure no one else is using it).
//
// When a SPI DMA transaction can not be completed because the bus is locked,
// the transaction is queued, and performed immediately after the current
// transaction finishes.
//
// The bus must not be overutilized for a period greater than the queue allows.
//
int spiBusLocked = 0;
int spiRxIsrCalls = 0;
int spiTxIsrCalls = 0;

//
// Circular buffer of queue descriptors.  To simplify queueing, one dummy
// descriptor is between head and tail.  head == tail means empty,
// (tail + 1 % NUM) == head means full.
//
struct QUEUED_DESCRIPTOR {
    uint8 * sendBuf;
    uint8 * recvBuf;
    uint32 bytes;
    uint32 ss;
    void (*doneCallback)(void *);
    void * doneArg;
    uint32 flags;
};

struct QUEUED_DESCRIPTORS {
    struct QUEUED_DESCRIPTOR queue[NUM_SPI_QDS];
    uint32 head;
    uint32 tail;
} queuedDescriptors;

void SpiDoneCallback(
    void * pDone
    )
{
    if (pDone) {
        *((int *) pDone) = 1;
    }
}

//
// Lock the SPI bus to perform an atomic SPI DMA transaction.
//
// If lock is unavailable, transaction /qd/ is queued, and will performed (in
// FIFO order) once the current SPI DMA transaction completes.
//
// /qd/ must be valid.
//
// @return true if the bus was idle (and is now locked).  Returns false if the
// bus was already locked - the given /qd/ is now queued.
//
int LockSpiBus(struct QUEUED_DESCRIPTOR * qd)
{
    int wasLocked;
    int full = 0;
    uint8 interruptState;

    interruptState = CyEnterCriticalSection();

    // Atomic test-and-set of lock variable (spiBusLocked)
    wasLocked = spiBusLocked;
    spiBusLocked = 1;

    // Queue descriptor if DMA was locked
    if (wasLocked) {
        full = ((queuedDescriptors.tail + 1) % NUM_SPI_QDS) == queuedDescriptors.head;
        if (!full) {
            queuedDescriptors.queue[queuedDescriptors.tail] = *qd;
            queuedDescriptors.tail = (queuedDescriptors.tail + 1) % NUM_SPI_QDS;
        }
    }

    CyExitCriticalSection(interruptState);

    return full ? -ENOMEM : wasLocked;
}

//
// Unlocks SPI bus from a previous LockSpiBus() call.
//
// If previous transactions were queued, causes next transaction to be started
// (and keeps bus locked).
//
// Intended to be called from SPI completion interrupt.
//
void UnlockSpiBus()
{
    uint8 interruptState;
    struct QUEUED_DESCRIPTOR qd;
    struct QUEUED_DESCRIPTOR * pqd = NULL;

    interruptState = CyEnterCriticalSection();

    // If queue is not empty, dequeue descriptor
    if (spiBusLocked && queuedDescriptors.head != queuedDescriptors.tail) {
        qd = queuedDescriptors.queue[queuedDescriptors.head];
        pqd = &qd;
        queuedDescriptors.head = (queuedDescriptors.head + 1) % NUM_SPI_QDS;
        // DMA stays locked
    } else {
        pqd = NULL;
        spiBusLocked = 0;
    }

    CyExitCriticalSection(interruptState);

    // If descriptor was dequeued, run it
    if (pqd) {
        SpiXferUnlocked(
            pqd->sendBuf, 
            pqd->recvBuf, 
            pqd->bytes, 
            pqd->ss, 
            pqd->doneCallback, 
            pqd->doneArg, 
            pqd->flags
            );
    }
}

//
// SPI TX ISR - data sent completion interrupt
//
void ((*spiDoneCallback)(void *));
void * spiDoneArg;
CY_ISR(SpiTxIsr)
{
    SPI_1_ClearMasterInterruptSource(SPI_1_GetMasterInterruptSourceMasked());
    spiTxIsrCalls++;
    if (spiDoneCallback) {
        spiDoneCallback(spiDoneArg);
    }
    UnlockSpiBus();
}

//
// SPI RX DMA ISR - data received completion interrupt
//
void SpiRxDmaIsr(void)
{
    spiRxIsrCalls++;
    if (spiDoneCallback) {
        spiDoneCallback(spiDoneArg);
    }
    UnlockSpiBus();
}

//
// Send/recv a SPI tranaction without locking the SPI bus.
//
// The SPI transaction sends and optionally receives data, in both cases via
// DMA.  The transmitted data has fully completed once the last byte heads out
// of the SPI TX FIFO.  OTOH, the received data is done when the DMA has
// finished writing the last byte into memory.  Therefore, the completion ISR
// is different for each (for TX, it's SPI done, for RX is DMA done).
//
// @param sendBuf       Buffer of bytes to send.  Must be present.
// @param recvBuf       Buffer of bytes to receive.  If present, this is a
//                      receive transaction.  If NULL, this is a transmit
//                      transaction.
// @param bytes         Number of bytes to send/recv.  Must be <=
//                      MAX_SPI_BUS_HOGGING_BYTES
// @param ss            Slave select number.  One of:
//                      SPI_SS_MEM_0
//                      SPI_SS_MEM_1
//                      SPI_SS_OLED
// @param doneCallback  Function to call after transaction completes
// @param doneArg       Arg to pass to doneCallback
// @param flags         Optional ORable flags.
//                      SPI_DONE_CALLBACK: Use pre-defined callback function
//                      that just sets single (int *) argument to 1.  If used,
//                      then doneCallback is unused.
//                      SPI_TX_BYTE_REPEATED: Only the first byte of sendBuf is
//                      used, and it is sent out repeatedly for /bytes/ bytes.
//
void SpiXferUnlocked(
    uint8 * sendBuf,
    uint8 * recvBuf,
    uint32 bytes,
    uint32 ss,
    void (*doneCallback)(void *),
    void * doneArg,
    uint32 flags
    )
{
    assert(bytes <= MAX_SPI_BUS_HOGGING_BYTES);

    if (flags & SPI_SHORT_AND_SWEET) {
        SPI_1_SpiUartPutArray(sendBuf, bytes);
        while (SPI_1_SpiUartGetTxBufferSize() > 0);
        UnlockSpiBus();
        return;
    }

    if (flags & SPI_DONE_CALLBACK) {
        doneCallback = SpiDoneCallback;
        assert(doneArg != NULL);
        *((int *) doneArg) = 0;
    }

    spiDoneCallback = doneCallback;
    spiDoneArg = doneArg;

    SpiRxDma_ChDisable();
    SpiTxDma_ChDisable();

    SpiTxDma_SetAddressIncrement(
        0, flags & SPI_TX_BYTE_REPEATED ? CYDMA_INC_NONE : CYDMA_INC_SRC_ADDR
        );
    SpiTxDma_SetSrcAddress(0, sendBuf);
    SpiTxDma_SetDstAddress(0, (void *) SPI_1_TX_FIFO_WR_PTR);
    SpiTxDma_SetNumDataElements(0, bytes);
    SpiTxDma_ValidateDescriptor(0);

    if (recvBuf) {
        SpiRxDma_SetInterruptCallback(&SpiRxDmaIsr);
        SpiRxDma_SetSrcAddress(0, (void *) SPI_1_RX_FIFO_RD_PTR);
        SpiRxDma_SetDstAddress(0, recvBuf);
        SpiRxDma_SetNumDataElements(0, bytes);
        SpiRxDma_ValidateDescriptor(0);
    }

    SPI_1_SpiSetActiveSlaveSelect(ss);

    SPI_1_SpiUartClearRxBuffer();
    SPI_1_SpiUartClearTxBuffer();

    //
    // Start DMAs.  
    //
    // For receiving, use both TX and RX DMA, and the previously setup SPI RX
    // DMA ISR.  
    //
    // For sending, Use on the TX DMA, and setup the SPI TX FIFO completion ISR
    // (which requires clearing pending ISRs first).
    //
    if (recvBuf) {
        SpiTxIsr_Disable();
        SpiRxDma_ChEnable();
        SpiTxDma_ChEnable();
    } else {
        SPI_1_ClearMasterInterruptSource(SPI_1_GetMasterInterruptSourceMasked());
        SpiTxIsr_ClearPending();
        SpiTxIsr_Enable();
        SpiTxDma_ChEnable();
    }
}

//
// Call SpiXferUnlocked() with the SPI bus locked.
//
// If the bus was already locked, transaction gets queued for later.
//
// @return 0 if the bus was successfully locked and the transaction was
// immediately started, -EAGAIN if the transaction was queued for later because
// the bus was already locked, or -ENOMEM of the bus was locked and there's no
// space to queue it.
//
int SpiXfer(
    uint8 * sendBuf,
    uint8 * recvBuf,
    uint32 bytes,
    uint32 ss,
    void (*doneCallback)(void *),
    void * doneArg,
    uint32 flags
    )
{
    struct QUEUED_DESCRIPTOR qd;
    int wasLocked;

    if (flags & SPI_DONE_CALLBACK) {
        doneCallback = SpiDoneCallback;
        assert(doneArg != NULL);
        *((int *) doneArg) = 0;
    }

    qd.sendBuf = sendBuf;
    qd.recvBuf = recvBuf;
    qd.bytes = bytes;
    qd.ss = ss;
    qd.doneCallback = doneCallback;
    qd.doneArg = doneArg;
    qd.flags = flags;
    wasLocked = LockSpiBus(&qd);
    if (wasLocked < 0) return wasLocked;
    if (wasLocked) return -EAGAIN;

    SpiXferUnlocked(sendBuf, recvBuf, bytes, ss, doneCallback, doneArg, flags);
    return 0;
}
