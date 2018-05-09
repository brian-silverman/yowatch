/*
 * draw.c
 *
 * Display drawing functions
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
#include "fonts.h"
#include "oled.h"
#include "colors.h"
#include "fonts.h"
#include "util.h"

#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   96

#define DISPLAY_BUF_SIZE (4 * 1024)
extern uint8 displayBuf[DISPLAY_BUF_SIZE];

static int CharImageCopy(
    uint16 * dest,
    const struct FONT_CHAR * p,
    int leftTrim,
    int topTrim,
    int rightTrim,
    int bottomTrim,
    int fgcolor,
    int bgcolor
    )
{
    int pixels = 0;

    if (leftTrim + rightTrim >= p->width) return pixels;
    if (topTrim + bottomTrim >= p->height) return pixels;

    if (leftTrim == 0 && rightTrim == 0 && topTrim == 0 && bottomTrim == 0 
        && fgcolor == WHITE && bgcolor == BLACK) 
    {
        pixels = (p->width * p->height);
        memcpy(dest, p->image, pixels * 2);
    } else {
        int i = 0;
        for (int x = leftTrim; x < p->width - rightTrim; x++) {
            for (int y = topTrim; y < p->height - bottomTrim; y++) {
                dest[i++] = p->image[x * p->height + y] ? fgcolor : bgcolor;
                pixels++;
            }
        }
    }

    return pixels;
}

static int CharSpacingFill(
    uint16 * dest,
    int height,
    int leftTrim,
    int topTrim,
    int rightTrim,
    int bottomTrim,
    int color
    )
{
    if (leftTrim > 0 || rightTrim > 0) return 0;

    int pixels = (height - topTrim - bottomTrim) * INTER_CHAR_SPACING;
    if (pixels <= 0) return 0;

    if (color == BLACK) {  // TBD if high/low byte equal
        memset(dest, color, pixels * 2);
    } else {
        for (int i = 0; i < pixels; i++) {
            dest[i] = color;
        }
    }
    return pixels;
}

void DrawText(
    char * text,
    int x,
    int y,
    int font,
    int fgcolor,
    int bgcolor
    )
{
    const struct FONT_CHAR * pfont = fonts[font];

    int leftBound = 0;
    int topBound = 0;
    int rightBound = SCREEN_WIDTH - 1;
    int bottomBound = SCREEN_HEIGHT - 1;

    int textWidth = 0;
    int textHeight = 0;
    GetTextDimensions(text, font, &textWidth, &textHeight);

    //
    // DrawText() uses convention of x/y at left/bottom.  Convert to left/top.
    //
    y = y - textHeight + 1;

    //
    // Top bottom bounds are the same for every char
    //
    int top = y;
    int bottom = top + textHeight - 1;;
    int topTrim = top < topBound ? topBound - top : 0;
    int bottomTrim = bottom > bottomBound ? bottom - bottomBound : 0;

    uint16 * dest = (uint16 *) displayBuf;
    int left = x;
    for (char * t = text; *t != '\0'; t++) {
        assert(*t >= 0 && *t < MAX_CHARS);
        const struct FONT_CHAR * p = &pfont[(int) *t];

        int right = left + p->width - 1;
        int leftTrim = left < leftBound ? leftBound - left : 0;
        int rightTrim = right > rightBound ? right - rightBound : 0;

        dest += CharImageCopy(dest, p, leftTrim, topTrim, rightTrim, bottomTrim, fgcolor, bgcolor);
        dest += CharSpacingFill(dest, p->height, leftTrim, topTrim, rightTrim, bottomTrim, bgcolor);

        left += p->width + INTER_CHAR_SPACING;
    }

    int x2 = x + textWidth - 1;
    int y2 = y + textHeight - 1;
    int croppedX1 = CROP(x, leftBound, rightBound);
    int croppedX2 = CROP(x2, leftBound, rightBound);
    int croppedY1 = CROP(y, topBound, bottomBound);
    int croppedY2 = CROP(y2, topBound, bottomBound);

    DisplayBitmap(displayBuf, croppedX1, croppedY1, croppedX2, croppedY2);
}

void DrawRect(
    int x1,
    int y1,
    int x2,
    int y2,
    int color,
    int border,
    int borderColor
    )
{
    DisplayRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1, color);
}


