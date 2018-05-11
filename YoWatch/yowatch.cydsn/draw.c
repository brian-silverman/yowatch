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
#include "draw.h"

#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   96

#define DISPLAY_BUF_SIZE (4 * 1024)
extern uint8 displayBuf[DISPLAY_BUF_SIZE];

RECT SCREEN_BOUNDS = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };

//
// Copy the character pixel image /p/ into the destination, bounded by /bound/.
//
// @param dest      destination pixels
// @param p         character image to copy
// @param bound     Relative bounds rectangle of character.  That is, specifies
//                  a rectangle within the character image that is to be
//                  copied.  The bounds may be larger in width to the right of
//                  the character - if so, this indicates a blank "spacing"
//                  area to be filled by the bacground color.
// @param fgColor   foreground color
// @param bgColor   background color
//
// @return the number of pixels written
//
static int CharImageCopy(
    uint16 * dest,
    const struct FONT_CHAR * p,
    RECT bound,
    int fgColor,
    int bgColor
    )
{
    int pixels = 0;

    // Spacing width to the right of character
    int charSpacing = bound.x + bound.width - p->width;

    // If bound is null, nothing to copy
    if (bound.height <= 0 || bound.width <= 0 ) return 0;

    //
    // Optimization: if bound indicates full char is to be copied, and char is
    // WHITE on BLACK (which is the default sorted in the image), then just
    // memcpy() it.
    //
    if (bound.width >= p->width && bound.height >= p->height && bound.x == 0 && bound.y == 0
        && fgColor == WHITE && bgColor == BLACK) 
    {
        pixels = (p->width * p->height);
        memcpy(dest, p->image, pixels * 2);
    } else {
        //
        // General case - copy the only the bounding area of the image.
        //
        int destIndex = 0;
        int srcIndex = bound.x * p->height + bound.y;
        int xSkip = p->height - bound.height;
        int w = MIN(p->width - bound.x, bound.width);
        int h = bound.height;
        for (int i = 0; i < w; i++) {
            for (int j = 0; j < h; j++) {
                dest[destIndex++] = p->image[srcIndex++] ? fgColor : bgColor;
                pixels++;
            }
            srcIndex += xSkip;
        }
    }

    //
    // Now, copy the blank spacing area to the right of the character.
    //
    dest += pixels;
    if (charSpacing > 0) {
        int spacingPixels = bound.height * charSpacing;

        if (bgColor == BLACK) {  // TODO or if high/low byte equal
            memset(dest, bgColor, spacingPixels * 2);
        } else {
            for (int i = 0; i < spacingPixels; i++) {
                dest[i++] = bgColor;
            }
        }
        pixels += spacingPixels;
    }

    return pixels;
}

//
// @return an expanded version of rect, that is larger by /border/ on each
// side.
//
RECT ExpandedRect(
    RECT r,
    int border
    )
{
    return (RECT) {r.x - border, r.y - border, r.width + 2 * border, r.height + 2 * border};
}

//
// Perform an intersection of two rectangles
//
// @return the intersection rectangle
//
RECT RectIntersection(
    RECT r1,
    RECT r2
    )
{
    RECT r3;
    r3.x = MAX(r1.x, r2.x);
    r3.y = MAX(r1.y, r2.y);
    r3.width = MIN(r1.x + r1.width, r2.x + r2.width) - r3.x;
    r3.height = MIN(r1.y + r1.height, r2.y + r2.height) - r3.y;
    return r3;
}

//
// Determine if two rectangles intersect
//
// @return true if they intersect
//
int DoRectsIntersect(
    RECT r1,
    RECT r2
    )
{
    if (r1.x >= r2.x + r2.width || r2.x >= r1.x + r1.width ) return 0;
    if (r1.y >= r2.y + r2.height || r2.y >= r1.y + r1.height) return 0;
    return 1;
}

//
// Draw a solid (filled) rectangle of /color/
//
void DrawRect(
    RECT r,
    int color
    )
{
    if (!DoRectsIntersect(r, SCREEN_BOUNDS)) return;

    RECT cropped = RectIntersection(r, SCREEN_BOUNDS);
    DisplayRect(cropped.x, cropped.y, cropped.width, cropped.height, color);
}

//
// Draw a border of width /borderWidth/ and /color/ within rectangle /r/
//
void DrawBorder(
    RECT r,
    int borderWidth,
    int color
    )
{
    //DisplayRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1, color);
}

//
// Draw text cropped by the given /bound/
//
// @param text      text to draw
// @param x         left start point of text
// @param y         top start point of text
// @param font      text font
// @param fgColor   foreground color of text
// @param bgColor   background color of text
// @param bound     bounding rectangle of text.  Any part of text outside of
//                  this rectangle will be cropped.
//
void DrawTextBounded(
    char * text,
    int x,
    int y,
    int font,
    int fgColor,
    int bgColor,
    RECT bound
    )
{
    const struct FONT_CHAR * pfont = fonts[font];

    RECT textRect = {x, y};
    GetTextDimensions(text, font, &textRect);

    //
    // If the text is off screen, just return
    //
    bound = RectIntersection(bound, SCREEN_BOUNDS);
    if (!DoRectsIntersect(textRect, bound)) return;

    //
    // Find minimal intersection rectangle that will have text in it.
    //
    RECT boundedTextRect = RectIntersection(bound, textRect);

    //
    // For each char in string, create a rectangle that is the bounds of the
    // char to be printed.  Convert that rectangle to relative coordinates, as
    // required by CharImageCopy().
    //
    uint16 * dest = (uint16 *) displayBuf;
    int charX = x;
    for (char * t = text; *t != '\0'; t++) {
        assert(*t >= 0 && *t < MAX_CHARS);
        const struct FONT_CHAR * p = &pfont[(int) *t];

        RECT charRect = {charX, y, p->width + INTER_CHAR_SPACING, p->height};
        RECT boundedCharRect = RectIntersection(charRect, boundedTextRect);
        boundedCharRect.x -= charX;
        boundedCharRect.y -= y;

        dest += CharImageCopy(dest, p, boundedCharRect, fgColor, bgColor);

        charX += p->width + INTER_CHAR_SPACING;
    }


    // 
    // DisplayBitmap.  Note: the number of pixels written to the displayBuf
    // should be identical to the area of the boundedTextRect - assert this.
    //
    assert(dest - (uint16 *) displayBuf == boundedTextRect.width * boundedTextRect.height);
    DisplayBitmap(
        displayBuf, 
        boundedTextRect.x, 
        boundedTextRect.y, 
        boundedTextRect.x + boundedTextRect.width - 1, 
        boundedTextRect.y + boundedTextRect.height - 1
        );
}

//
// Draw text on screen
//
// Calls DrawTextBounded() with screen bounds.
//
void DrawText(
    char * text,
    int x,
    int y,
    int font,
    int fgColor,
    int bgColor
    )
{
    DrawTextBounded(text, x, y, font, fgColor, bgColor, SCREEN_BOUNDS);
}

//
// Draw multiple lines of text in a bounded box
//
// @param lines     lines of text to draw
// @param num       number of /lines/
// @param box       bounding box to draw in.  May extend outside of screen
//                  (although it will get cropped to the screen)
// @param shiftUp   pixels to shift text up vertically
// @param justify   One of: LEFT_JUSTIFIED, RIGHT_JUSTIFIED, CENTER_JUSTIFIED
// @param font      text font
// @param fgColor   foreground color of text
// @param bgColor   background color of text
//
void DrawTextBox(
    char * lines[],
    int num,
    RECT box,
    int shiftUp,
    int justify,
    int font,
    int fgColor,
    int bgColor
    )
{
    int lineX = box.x;
    int lineY = box.y - shiftUp;

    DrawRect(box, bgColor);
    for (int i = 0; i < num; i++) {
        RECT textRect = box;
        GetTextDimensions(lines[i], font, &textRect);

        switch (justify) {
            case LEFT_JUSTIFIED:
                lineX = box.x;
                break;
            case RIGHT_JUSTIFIED:
                lineX = box.x + box.width - textRect.width;
                break;
            case CENTER_JUSTIFIED:
                lineX = box.x + (box.width - textRect.width)/2;
                break;
            default:
                assert(0);
        }
        DrawTextBounded(lines[i], lineX, lineY, font, fgColor, bgColor, box);
        lineY += textRect.height + INTER_LINE_SPACING;
    }
}

void DrawPoint(
    int x,
    int y,
    int color
    )
{
    DisplayRect(x, y, 1, 1, color);
}
