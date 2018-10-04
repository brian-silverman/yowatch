/*
 * oled.c
 *
 * OLED display interface
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
#include "oled.h"
#include "printf.h"
#include "spi.h"
#include "assert.h"
#include "colors.h"
#include "util.h"
#include "status.h"

#define CMD_ARGS(cmd,argc,...) \
    { \
        uint8 data[argc] = {__VA_ARGS__}; \
        SendCommand(cmd, data, argc); \
    }


#define SET_COLUMN_ADDRESS(a, b)                CMD_ARGS(0x15, 2, a, b)
#define SET_ROW_ADDRESS(a, b)                   CMD_ARGS(0x75, 2, a, b)
#define WRITE_RAM_COMMAND()                     CMD_ARGS(0x5C, 0)
#define READ_RAM_COMMAND()                      CMD_ARGS(0x5D, 0)
#define SET_REMAP(a)                            CMD_ARGS(0xA0, 1, a)

#define SET_DISPLAY_START_LINE(a)               CMD_ARGS(0xA1, 1, a)
#define SET_DISPLAY_OFFSET(a)                   CMD_ARGS(0xA2, 1, a)
#define SET_DISPLAY_MODE_ALL_OFF()              CMD_ARGS(0xA4, 0)
#define SET_DISPLAY_MODE_ALL_ON()               CMD_ARGS(0xA5, 0)
#define SET_DISPLAY_MODE_NORMAL()               CMD_ARGS(0xA6, 0)
#define SET_DISPLAY_MODE_INVERSE()              CMD_ARGS(0xA7, 0)
#define SET_FUNCTION_SELECTION(a)               CMD_ARGS(0xAB, 1, a)
#define SET_SLEEP_MODE_ON()                     CMD_ARGS(0xAE, 0)
#define SET_SLEEP_MODE_OFF()                    CMD_ARGS(0xAF, 0)
#define SET_PHASE(a)                            CMD_ARGS(0xB1, 1, a)

#define DISPLAY_ENHANCEMENT(a, b, c)            CMD_ARGS(0xB2, 3, a, b, c)
#define SET_FRONT_CLOCK_DIVIDER(a)              CMD_ARGS(0xB3, 1, a)
#define SET_SEGMENT_LOW_VOLTAGE(a, b, c)        CMD_ARGS(0xB4, 3, a, b, c)
#define SET_GPIO(a)                             CMD_ARGS(0xB5, 1, a)
#define SET_SECOND_PRECHARGE_PERIOD(a)          CMD_ARGS(0xB6, 1, a)

#define USE_BUILTIN_LINEAR_LUT()                CMD_ARGS(0xB9, 0)
#define SET_PRECHARGE_VOLTAGE(a)                CMD_ARGS(0xBB, 1, a)
#define SET_VCOMH_VOLTAGE(a)                    CMD_ARGS(0xBE, 1, a)

#define SET_CONTRAST_CURRENT_FOR_COLOR(a, b, c) CMD_ARGS(0xC1, 3, a, b, c)
#define MASTER_CONTRAST_CURRENT_CONTROL(a)      CMD_ARGS(0xC7, 1, a)
#define SET_MUX_RATIO(a)                        CMD_ARGS(0xCA, 1, a)
#define SET_COMMAND_LOCK(a)                     CMD_ARGS(0xFD, 1, a)

#define HORIZONTAL_SCROLL(a, b, c, d, e)        CMD_ARGS(0x96, a, b, c, d, e)
#define STOP_MOVING()                           CMD_ARGS(0x9E, 0)
#define START_MOVING()                          CMD_ARGS(0x9F, 0)

#define GAMMA_TABLE_CMD                         0xB8
#define GAMMA_TABLE_BYTES                       63

//
// Suggested gamma lookup table from Univision Users Guide
//
uint8 gammaTable[GAMMA_TABLE_BYTES] = {
    0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 
    0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 
    0x12, 0x13, 0x15, 0x17, 0x19, 0x1B, 0x1D, 0x1F, 
    0x21, 0x23, 0x25, 0x27, 0x2A, 0x2D, 0x30, 0x33, 
    0x36, 0x39, 0x3C, 0x3F, 0x42, 0x45, 0x48, 0x4C, 
    0x50, 0x54, 0x58, 0x5C, 0x60, 0x64, 0x68, 0x6C, 
    0x70, 0x74, 0x78, 0x7D, 0x82, 0x87, 0x8C, 0x91, 
    0x96, 0x9B, 0xA0, 0xA5, 0xAA, 0xAF, 0xB4
    };
    
//
// Display buf that is used for all Display function as a temp framebuffer for
// faster display writing.  Display buf functions are not reentrant, so this
// same buf can be used by all functions.
//
// Size of display buf must be at least as big as status bar.
//
#define DISPLAY_BUF_SIZE (4 * 1024)
uint8 displayBuf[DISPLAY_BUF_SIZE];
#if DISPLAY_BUF_SIZE < STATUS_BAR_HEIGHT * STATUS_BAR_WIDTH * 2
    #error DISPLAY_BUF_SIZE is not big enough to fit status bar
#endif


static int SendDataRepeatedBlocking(
    uint8 data,
    uint32 bytes
    )
{
    int ret;
    int done;

    OLED_DAT_CMD_Write(1);
    ret = SpiXfer(
        &data, NULL, bytes, SPI_SS_OLED, NULL, &done, SPI_DONE_CALLBACK | SPI_TX_BYTE_REPEATED
        );
    if (ret == 0 || ret == -EAGAIN) {
        while (!done);
    }
    OLED_DAT_CMD_Write(0);
    return ret;
}

static int SendBytesBlocking(
    uint8 * data,
    uint32 bytes,
    int isCmd
    )
{
    int ret;
    int done;

    if (bytes == 0) return 0;

    OLED_DAT_CMD_Write(isCmd ? 0 : 1);
    ret = SpiXfer(data, NULL, bytes, SPI_SS_OLED, NULL, &done, SPI_DONE_CALLBACK);
    if (ret == 0 || ret == -EAGAIN) {
        while (!done);
    }
    OLED_DAT_CMD_Write(0);
    return ret;
}

static void SendCommand(
    uint8 cmd,
    uint8 * data,
    uint32 bytes
    )
{
    assert(SendBytesBlocking(&cmd, 1, 1) == 0);
    assert(SendBytesBlocking(data, bytes, 0) == 0);
}

static void SendGammaTable(
    uint8 data[GAMMA_TABLE_BYTES]
    )
{
    uint8 cmd = GAMMA_TABLE_CMD;
    assert(SendBytesBlocking(&cmd, 1, 1) == 0);
    assert(SendBytesBlocking(data, GAMMA_TABLE_BYTES, 0) == 0);
}

//
// GenerateGammaTable - create a quadratic gamma curve given 3 points
//
// The suggested gamma lookup table from Univision does a very poor job of
// representing low value colors.  The built-in linear table is only marginally
// better.  This functions generates a table from a quadratic curve given 3
// points.  The points must be between 0 and 1 in both x and y.
//
// For example, the points (0,0), (0.5,0.5), (1,1) would generate a linear
// gamma, where (0,0), (0.5,0.6), (1,1) would generate a slight hump to the
// gamma.
//
// The polynoimial generated is printed so the curve can be viewed via an
// online graphing calculator.
//
void GenerateGammaTable(
    uint8 gammas[GAMMA_TABLE_BYTES],
    float x1,
    float y1,
    float x2,
    float y2,
    float x3,
    float y3
    )
{
    float denom, a, b, c;

    assert(0.0 <= x1 && x1 <= 1.0);
    assert(0.0 <= y1 && y1 <= 1.0);
    assert(0.0 <= x2 && x2 <= 1.0);
    assert(0.0 <= y2 && y2 <= 1.0);
    assert(0.0 <= x3 && x3 <= 1.0);
    assert(0.0 <= y3 && y3 <= 1.0);

    //
    // Quadratic solution (y = a*x^2 + b*x +c) given 3 points:
    //
    denom = (x1 - x2)*(x1 - x3)*(x2 - x3);
    a = (x3 * (y2 - y1) + x2 * (y1 - y3) + x1 * (y3 - y2)) / denom;
    b = (x3*x3 * (y1 - y2) + x2*x2 * (y3 - y1) + x1*x1 * (y2 - y3)) / denom;
    c = (x2 * x3 * (x2 - x3) * y1 + x3 * x1 * (x3 - x1) * y2 + x1 * x2 * (x1 - x2) * y3) / denom;

    if (0) {
        xprintf("Display gamma quadratic:\r\n");
        xprintf("\t%c %d.%0dx^2 %c %d.%0dx %c %d.%0d\r\n", 
            a < 0 ? '-' : '+', (int) ABS(a), (int) ABS((a - (int)a)*1000),
            b < 0 ? '-' : '+', (int) ABS(b), (int) ABS((b - (int)b)*1000),
            c < 0 ? '-' : '+', (int) ABS(c), (int) ABS((c - (int)c)*1000));
    }

    const int GAMMA_MAX = 0xD0;
    int prev_val = 0;
    for (int i = 0; i < GAMMA_TABLE_BYTES; i++) {
        float x = (float) i / (float) GAMMA_TABLE_BYTES;
        float y = a * x * x + b * x + c;
        y *= GAMMA_MAX;
        int val = (int) y;
        val = val > GAMMA_MAX ? GAMMA_MAX: val;
        val = val < prev_val ? prev_val : val;
        gammas[i] = val;
        prev_val = val;
    }
}

void DisplayInit()
{
    //
    // Univision Technology OLED display init.  Modified from UG-2896GDEAF11.
    //
    SET_COMMAND_LOCK(0x12);
    SET_COMMAND_LOCK(0xB1);
    SET_SLEEP_MODE_ON();
    SET_FRONT_CLOCK_DIVIDER(0xF1);
    // 96 line display
    SET_MUX_RATIO(0x5F);
    SET_DISPLAY_OFFSET(0x00);
    SET_DISPLAY_START_LINE(0x00);
    // SET_REMAP:
    //      A[0]=1b, Vertical address increment
    //      A[1]=1b, Column address 0 is mapped to SEG0
    //      A[2]=1b, Color sequence: C->B->A (sets correct RGB565 order)
    //      A[3]=0b, Reserved
    //      A[4]=0b, Scan from COM[N-1] to COM0. 
    //      A[5]=1b, Enable COM Split Odd Even [reset]
    //      A[7:6]=00b Set Color Depth 65k color 
    // Bits 1 and 4 set the correct screen orientation
    SET_REMAP(0x27);    
    SET_GPIO(0x00);
    SET_FUNCTION_SELECTION(0x01);
    SET_SEGMENT_LOW_VOLTAGE(0xA0, 0xB5, 0x55);
    SET_CONTRAST_CURRENT_FOR_COLOR(0xC8, 0x80, 0x8A);
    MASTER_CONTRAST_CURRENT_CONTROL(0x0F);
    GenerateGammaTable(gammaTable, 0.0, 0.1, 0.4, 0.3, 1.0, 1.0);
    SendGammaTable(gammaTable);
    SET_PHASE(0x32);
    DISPLAY_ENHANCEMENT(0xA4, 0x00, 0x00);
    SET_PRECHARGE_VOLTAGE(0x17);
    SET_SECOND_PRECHARGE_PERIOD(0x01);
    SET_VCOMH_VOLTAGE(0x05);
    SET_DISPLAY_MODE_NORMAL();
    SET_SLEEP_MODE_OFF();
}

void DisplayRect(
    uint32 x1,
    uint32 y1,
    uint32 width,
    uint32 height,
    uint16 color
    )
{
    int i;
    int pixels = width * height;

    uint32 x2 = x1 + width - 1;
    uint32 y2 = y1 + height - 1;

    assert(x1 < SCREEN_WIDTH);
    assert(x2 < SCREEN_WIDTH);
    assert(y1 < SCREEN_HEIGHT);
    assert(y2 < SCREEN_HEIGHT);

    SET_COLUMN_ADDRESS(x1, x2);
    SET_ROW_ADDRESS(y1, y2);
    WRITE_RAM_COMMAND();

    //
    // Optimize writes if possible
    //
    if ((color >> 8) == (color & 0xFF)) {
        //
        // If the 16-bit color has the same high and low byte (which is true
        // for the very common black (0x0000) and white (0xFFFF), then we can
        // do a significant speed enhancement by sending just that byte.  It's
        // faster because the transfer (via SpiXfer(..., SPI_TX_BYTE_REPEATED))
        // is done solely via DMA.
        //
        // The only negative is that if we hog the bus for too long audio
        // packets will be dropped while getting written to RAM.  See
        // MAX_SPI_BUS_HOGGING_BYTES.
        //
        int bytes = pixels * sizeof(uint16);
        while (bytes > MAX_SPI_BUS_HOGGING_BYTES) {
            SendDataRepeatedBlocking(color & 0xFF, MAX_SPI_BUS_HOGGING_BYTES);
            bytes -= MAX_SPI_BUS_HOGGING_BYTES;
        }
        if (bytes > 0) {
            SendDataRepeatedBlocking(color & 0xFF, bytes);
        }
    } else {
        //
        // Generic writes get a significant speed boost if we can write them
        // (via DMA) in big chunks (as opposed to one write per pixel).
        //
        int numBytes = 0;
        uint16 * p = (uint16 *) displayBuf;
        for (i = 0; i < pixels; i++) {
            *(p++) = color;
            numBytes += sizeof(color);
            if (numBytes >= sizeof(displayBuf)) {
                SendBytesBlocking(displayBuf, numBytes, 0);
                numBytes = 0;
                p = (uint16 *) displayBuf;
            }
        }
        if (numBytes > 0) {
            SendBytesBlocking(displayBuf, numBytes, 0);
        }
    }
}

void DisplayFill(
    uint16 color
    )
{
    DisplayRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color);
}

void DisplayErase()
{
    DisplayFill(BLACK);
}

void DisplayBitmap(
    uint8 * buf,
    uint32 x1,
    uint32 y1,
    uint32 x2,
    uint32 y2
    )
{
    int width, height;

    assert(x1 < SCREEN_WIDTH);
    assert(x2 < SCREEN_WIDTH);
    assert(y1 < SCREEN_HEIGHT);
    assert(y2 < SCREEN_HEIGHT);

    SET_COLUMN_ADDRESS(x1, x2);
    SET_ROW_ADDRESS(y1, y2);
    WRITE_RAM_COMMAND();

    width = x2 - x1 + 1;
    height = y2 - y1 + 1;
    SendBytesBlocking(buf, width * height * 2, 0);
}

