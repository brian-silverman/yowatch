/*
 * i2s.c
 *
 * I2S microphone (read only)
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

//
// ISR for I2sGetBuf()
//
uint8 i2sIsrDone;
static void I2sRxDmaIsrOneshot()
{
    I2S_1_DisableRx();
    i2sIsrDone = 1;
}

//
// Fill a single I2S buf
//
void I2sGetBuf(
    uint8 * buf
    )
{
    i2sIsrDone = 0;
    I2sStartDma(buf, I2sRxDmaIsrOneshot);
    while (!i2sIsrDone);
    I2sStopDma();
}

//
// I2S DMA complete ISR.
//
// When I2S DMA from Mic to RAM buf completes, enqueue the buffer to SPI Serial
// RAM, and start a new I2S DMA on the ping pong buf.
//
int stopI2sDma = 1;
static void I2sRxDmaIsr(void)
{
    if (stopI2sDma) {
        I2S_1_DisableRx();
    } else {
        uint8 * buf = I2sStartDma(NULL, NULL);

        //
        // Enqueue audio buf to Serial RAM (don't care when it finishes).  
        //
        // Time to enqueue must be less than time to collect I2S data (I2S
        // bitrate is 16kHz * 16 bits/sample = 256 kbps which is much less than
        // the maximum SPI data rate of 8Mbps).
        // 
        int ret = EnqueueBytes(buf, SERIAL_RAM_BUFSIZE, NULL);
        if (ret == -ENOSPC) {
            // Flag error?
        }

    }
}

//
// Start an I2S Mic DMA transaction to a ping-pong buf
//
// As the completion ISR starts another DMA transaction when this one
// completes, the sequenece of DMAs will run forever until stopped
// (via I2sStopDma())
//
uint8 * I2sStartDma(
    uint8 * buf,
    void (*isr)(void)
    )
{
    uint8 * pbuf1 = SerialRamGetBuf(0);
    uint8 * pbuf2 = SerialRamGetBuf(1);
    static uint8 * pCurrentBuf = NULL;
    uint8 * pPrevBuf;

    if (buf) {
        pPrevBuf = buf;
    } else {
        pPrevBuf = pCurrentBuf;
        pCurrentBuf = pCurrentBuf == pbuf1 ? pbuf2 : pbuf1;
        buf = pCurrentBuf;
    }
    isr = isr ? isr : &I2sRxDmaIsr;
    if (stopI2sDma) {
        stopI2sDma = 0;
        I2S_1_ClearRxFIFO();
        I2S_1_EnableRx();
    }
    I2sRxDma_SetInterruptCallback(isr);
    I2sRxDma_SetSrcAddress(0, (void *) I2S_1_RX_CH0_F0_PTR);
    I2sRxDma_SetDstAddress(0, buf);
    I2sRxDma_SetNumDataElements(0, SERIAL_RAM_BUFSIZE);
    I2sRxDma_ValidateDescriptor(0);
    I2sRxDma_ChEnable();
    return pPrevBuf;
}

//
// Stop an I2S DMA transaction.  See I2sStartDma().
//
void I2sStopDma()
{
    stopI2sDma = 1;
}
