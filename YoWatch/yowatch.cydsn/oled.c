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
#include "printf.h"
#include "spi.h"
#include "assert.h"
#include "fonts.h"

#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   96

#define STATUS_BAR_HEIGHT   7   // 5x5 font plus top/bottom pixel
#define STATUS_BAR_WIDTH    SCREEN_WIDTH

#define CMD_ARGS(cmd,argc,...) \
    { \
        uint8 data[argc] = {__VA_ARGS__}; \
        SendCommand(cmd, data, argc); \
    }

#define MIN(a,b) ((a < b) ? (a) : (b))
#define MAX(a,b) ((a > b) ? (a) : (b))
#define CROP(x,a,b) (x < a ? (a) : (x > b ? (b) : (x)))

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

#define LOOKUP_TABLE_FOR_GRAYSCALE              0xB8
#define LOOKUP_TABLE_FOR_GRAYSCALE_BYTES        63

#ifdef UNIVISION_GAMMA_LOOKUP_TABLE
//
// Suggested gamma lookup table from Univision Users Guide
//
uint8 lookupTableForGrayscale[LOOKUP_TABLE_FOR_GRAYSCALE_BYTES] = {
    0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 
    0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 
    0x12, 0x13, 0x15, 0x17, 0x19, 0x1B, 0x1D, 0x1F, 
    0x21, 0x23, 0x25, 0x27, 0x2A, 0x2D, 0x30, 0x33, 
    0x36, 0x39, 0x3C, 0x3F, 0x42, 0x45, 0x48, 0x4C, 
    0x50, 0x54, 0x58, 0x5C, 0x60, 0x64, 0x68, 0x6C, 
    0x70, 0x74, 0x78, 0x7D, 0x82, 0x87, 0x8C, 0x91, 
    0x96, 0x9B, 0xA0, 0xA5, 0xAA, 0xAF, 0xB4
    };
#else
//
// The suggested gamma lookup table from Univision does a very poor job of
// representing low value colors.  The built-in linear table is only marginally
// better.  This table below was generated from a polynomial curve that boosts
// the low end pixel values.  It was generated in python with:
//
//  for i, val in enumerate(-.0007*(63-x)**(3.0)+200 for x in range(63)):
//      print("0x{:02X}, ".format(int(val)), end="" if (i+1)%8 else "\n")
//  print()
//
//  The formula was experimentally selected using a graphing calculator,
//  primarily to adjust the coefficient and exponent.
//
uint8 lookupTableForGrayscale[LOOKUP_TABLE_FOR_GRAYSCALE_BYTES] = {
    0x18, 0x21, 0x29, 0x30, 0x38, 0x3F, 0x46, 0x4D, 
    0x53, 0x59, 0x5F, 0x65, 0x6B, 0x70, 0x75, 0x7A, 
    0x7F, 0x83, 0x88, 0x8C, 0x90, 0x94, 0x97, 0x9B, 
    0x9E, 0xA1, 0xA4, 0xA7, 0xA9, 0xAC, 0xAE, 0xB1, 
    0xB3, 0xB5, 0xB6, 0xB8, 0xBA, 0xBB, 0xBD, 0xBE, 
    0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xC3, 0xC4, 0xC5, 
    0xC5, 0xC6, 0xC6, 0xC6, 0xC7, 0xC7, 0xC7, 0xC7, 
    0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 
    };
#endif
    
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

static void SendLookupTableForGrayscale(
    uint8 data[LOOKUP_TABLE_FOR_GRAYSCALE_BYTES]
    )
{
    uint8 cmd = LOOKUP_TABLE_FOR_GRAYSCALE;
    assert(SendBytesBlocking(&cmd, 1, 1) == 0);
    assert(SendBytesBlocking(data, LOOKUP_TABLE_FOR_GRAYSCALE_BYTES, 0) == 0);
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
    SendLookupTableForGrayscale(lookupTableForGrayscale);
    SET_PHASE(0x32);
    DISPLAY_ENHANCEMENT(0xA4, 0x00, 0x00);
    SET_PRECHARGE_VOLTAGE(0x17);
    SET_SECOND_PRECHARGE_PERIOD(0x01);
    SET_VCOMH_VOLTAGE(0x05);
    SET_DISPLAY_MODE_NORMAL();
    SET_SLEEP_MODE_OFF();
}

void DisplayRect(
    int x,
    int y,
    uint32 width,
    uint32 height,
    uint16 color
    )
{
    int i;
    int pixels = width * height;

    SET_COLUMN_ADDRESS(x, x + width - 1);
    SET_ROW_ADDRESS(y, y + height - 1);
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
    DisplayFill(0x0000);
}

void DisplayText(
    char * text,
    uint32 x,
    uint32 y,
    int font,
    int fgcolor,
    int bgcolor
    )
{
    const struct FONT_CHAR * pfont = fonts[font];
    char * t;
    int textWidth = 0;
    int textHeight = 0;
    int textOnScreenX1, textOnScreenX2;
    int textOnScreenY1, textOnScreenY2;
    int textOnScreenWidth, textOnScreenHeight;
    uint16 * dest;

    //
    // Determine text height/width
    //
#if 0
xprintf("----%s: %d\r\n", __FILE__, __LINE__);
#endif
    t = text;
    while (*t != '\0') {
        assert(*t >= 0 && *t < MAX_CHARS);
        const struct FONT_CHAR * p = &pfont[(int) *t];
        textWidth += p->width + INTER_CHAR_SPACING;
        textHeight = MAX(textHeight, p->height);
        // For now, only handle fixed height fonts
        assert(textHeight == p->height);
        t++;
    }
    textWidth--;

    //
    // Determine rect that text overlaps with screen
    //
    textOnScreenX1 = CROP(x, 0, SCREEN_WIDTH);
    textOnScreenY1 = CROP(y, 0, SCREEN_HEIGHT);
    textOnScreenX2 = CROP(x + textWidth - 1, 0, SCREEN_WIDTH);
    textOnScreenY2 = CROP(y + textHeight - 1, 0, SCREEN_HEIGHT);
    textOnScreenWidth = textOnScreenX2 - textOnScreenX1 + 1;
    textOnScreenHeight = textOnScreenY2 - textOnScreenY1 + 1;

    memset(displayBuf, 0, sizeof(displayBuf));

    t = text;
    dest = (uint16 *) displayBuf;
    while (*t != '\0') {
        const struct FONT_CHAR * p = &pfont[(int) *t];
        int bytes = p->width * p->height * sizeof(uint16);
        memcpy(dest, p->image, bytes);
        dest += bytes/2;
        memset(dest, 0, p->height * INTER_CHAR_SPACING);
        dest += p->height * INTER_CHAR_SPACING;
        t++;
    }

    xprintf("%d, %d, %d, %d\r\n", textOnScreenX1, textOnScreenX2, textOnScreenY1, textOnScreenY2);
    SET_COLUMN_ADDRESS(textOnScreenX1, textOnScreenX2);
    SET_ROW_ADDRESS(textOnScreenY1, textOnScreenY2);
    WRITE_RAM_COMMAND();

    //
    // Copy if colored. If -1, default color is used
    //
    SendBytesBlocking(displayBuf, textOnScreenWidth * textOnScreenHeight * 2, 0);
}
