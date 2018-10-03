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
#include "status.h"

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
    // WHITE on BLACK (which is the default colors in the image), then just
    // memcpy() it.
    //
    if (bound.width >= p->width && bound.height >= p->height && bound.x == 0 && bound.y == 0
        && ((fgColor == WHITE && bgColor == BLACK) || (fgColor == NATIVE && bgColor == NATIVE)))
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
                int pixel = p->image[srcIndex++];
                if ((pixel && fgColor == NATIVE) || (! pixel && bgColor == NATIVE)) {
                    dest[destIndex++] = pixel;
                } else {
                    dest[destIndex++] = pixel ? fgColor : bgColor;
                }
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
// Called by DrawTextBounded(). 
//
// Assumes that /text/ is not too big to fit in displayBuf.
//
static void _DrawTextBounded(
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
    int interCharSpacing = GetInterCharSpacing(font);
    for (char * t = text; *t != '\0'; t++) {
        assert(*t >= 0 && *t < MAX_CHARS);
        const struct FONT_CHAR * p = &pfont[(int) *t];

        RECT charRect = {charX, y, p->width + interCharSpacing, p->height};
        RECT boundedCharRect = RectIntersection(charRect, boundedTextRect);
        boundedCharRect.x -= charX;
        boundedCharRect.y -= y;

        dest += CharImageCopy(dest, p, boundedCharRect, fgColor, bgColor);

        charX += p->width + interCharSpacing;
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
    //
    // If font is so big that a single fullscreen line is greater than the size
    // of displayBuf, then we'll print char by char.  If a char itself is too
    // big, we're screwed.
    //
    int height = GetFontHeight(font);
    int interCharSpacing = GetInterCharSpacing(font);
    if (SCREEN_WIDTH * height * 2 > sizeof(displayBuf)) {
        char c[2];
        c[1] = 0;
        while (text[0]) {
            int cwidth = GetCharWidth(*text, font);
            c[0] = *text;
            assert((cwidth + interCharSpacing) * height <= sizeof(displayBuf));
            _DrawTextBounded(c, x, y, font, fgColor, bgColor, bound);
            x += cwidth;
            DrawRect((RECT) {x, y, interCharSpacing, height}, bgColor);
            x += interCharSpacing;
            text++;
        }
    } else {
        _DrawTextBounded(text, x, y, font, fgColor, bgColor, bound);
    }
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
// @param justify   Justifications flags:
//                  One of (or none of):
//                      LEFT_JUSTIFIED (default), RIGHT_JUSTIFIED, HCENTER_JUSTIFIED
//                  ORed with, one of (or none of):
//                      TOP_JUSTIFIED (default), VCENTER_JUSTIFIED, BOTTOM_JUSTIFIED
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
    RECT r, r2;
    RECT textRect;
    int lineX;
    int lineY;

    //
    // Compute starting y line, based on veritcal justify
    //
    int totalTextHeight = GetFontHeight(font) * num;
    lineY = box.y - shiftUp; // starting y position
    switch (justify & VERT_JUST_MASK) {
        case 0:
        case TOP_JUSTIFIED:
            // Do nothing - this is the default
            break;

        case BOTTOM_JUSTIFIED:
            lineY += box.height - totalTextHeight;
            break;

        case VCENTER_JUSTIFIED:
            lineY += (box.height - totalTextHeight)/2;
            break;

        default:
            assert(0);
            break;
    }

    // Erase any space above the text
    r = (RECT) {box.x, box.y, box.width, lineY - box.y};
    DrawRect(r, bgColor);

    //
    // For each line draw the text, and also clear any non-text areas.
    // Non-text areas must be cleared separately (as opposed to clearing the
    // whole text box beforehand) to avoid flicker - display is not double
    // buffered.
    //
    int interLineSpacing = GetInterLineSpacing(font);
    for (int i = 0; i < num; i++) {
        GetTextDimensions(lines[i], font, &textRect);

        switch (justify & HORIZ_JUST_MASK) {
            case 0:
            case LEFT_JUSTIFIED:
                lineX = box.x;

                // Erase BG to right of text line
                r = (RECT) {
                    lineX + textRect.width, lineY, box.width - textRect.width, textRect.height};
                r = RectIntersection(r, box);
                DrawRect(r, bgColor);
                break;

            case RIGHT_JUSTIFIED:
                lineX = box.x + box.width - textRect.width;

                // Erase BG to left of text line
                r = (RECT) {box.x, lineY, box.width - textRect.width, textRect.height};
                r = RectIntersection(r, box);
                DrawRect(r, bgColor);
                break;

            case HCENTER_JUSTIFIED:
                lineX = box.x + (box.width - textRect.width)/2;

                // Erase BG to left of text line
                r = (RECT) {box.x, lineY, lineX - box.x, textRect.height};
                r = RectIntersection(r, box);
                DrawRect(r, bgColor);

                // Erase BG to right of text line
                r2 = (RECT) {
                    lineX + textRect.width,
                    lineY,
                    (box.x + box.width) - (lineX + textRect.width),
                    textRect.height};
                r2 = RectIntersection(r2, box);
                DrawRect(r2, bgColor);
                break;

            default:
                assert(0);
                break;
        }
        DrawTextBounded(lines[i], lineX, lineY, font, fgColor, bgColor, box);
        lineY += textRect.height;

        // Erase space between lines
        r = (RECT) {box.x, lineY, box.width, interLineSpacing};
        r = RectIntersection(r, box);
        DrawRect(r, bgColor);
        lineY += interLineSpacing;
    }

    // Erase the rest of the text box
    r = (RECT) {box.x, lineY, box.width, box.y + box.height - lineY};
    r = RectIntersection(r, box);
    DrawRect(r, bgColor);
}

void DrawPoint(
    int x,
    int y,
    int color
    )
{
    if (x < 0 || x >= SCREEN_WIDTH) return;
    if (y < 0 || y >= SCREEN_HEIGHT) return;
    DisplayRect(x, y, 1, 1, color);
}

//
// Diagonal line, shallow slope (Bresenham's line algorithm via Wikipedia)
//
static void DrawLineLow(
    int x1,
    int y1, 
    int x2,
    int y2,
    int color
    )
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    int yi = 1;
    if (dy < 0) {
        yi = -1;
        dy = -dy;
    }
    int D = 2*dy - dx;
    int y = y1;

    for (int x = x1; x <= x2; x++) {
        DrawPoint(x, y, color);
        if (D > 0) {
            y = y + yi;
            D = D - 2*dx;
        }
        D = D + 2*dy;
    }
}

//
// Diagonal line, steep slope (Bresenham's line algorithm via Wikipedia)
//
static void DrawLineHigh(
    int x1,
    int y1, 
    int x2,
    int y2,
    int color
    )
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    int xi = 1;
    if (dx < 0) {
        xi = -1;
        dx = -dx;
    }
    int D = 2*dx - dy;
    int x = x1;

    for (int y = y1; y <= y2; y++) {
        DrawPoint(x, y, color);
        if (D > 0) {
            x = x + xi;
            D = D - 2*dy;
        }
        D = D + 2*dx;
    }
}

void DrawLine(
    int x1,
    int y1,
    int x2,
    int y2,
    int color
    )
{
    if (x1 == x2) {
        //
        // Vertical line
        //
        DrawRect((RECT) {x1, MIN(y1, y2), 1, ABS(y2 - y1) + 1}, color);

    } else if (y1 == y2) {
        //
        // Horizontal line
        //
        DrawRect((RECT) {MIN(x1, x2), y1, ABS(x2 - x1) + 1, 1}, color);

    } else {
        //
        // Diagonal line (Bresenham's line algorithm via Wikipedia)
        //
        if (ABS(y2 - y1) < ABS(x2 - x1)) {
            if (x1 > x2) {
                DrawLineLow(x2, y2, x1, y1, color);
            } else {
                DrawLineLow(x1, y1, x2, y2, color);
            }
        } else {
            if (y1 > y2) {
                DrawLineHigh(x2, y2, x1, y1, color);
            } else {
                DrawLineHigh(x1, y1, x2, y2, color);
            }
        }
    }
}

void DrawStatusBar(
    int erase
    )
{
    const int font = FONT_5X5;
    static char lastStatus[NUM_STATUSES + 1];
    static char lastNotif[NUM_NOTIFS + 1];
    static char status[NUM_STATUSES + 1];
    static char notif[NUM_NOTIFS + 1];

    if (erase) {
        RECT r = (RECT) {0, 0, SCREEN_WIDTH, STATUS_BAR_HEIGHT};
        DrawRect(r, BLACK);
    }

    StatusGetString(status, FIRST_STATUS, LAST_STATUS);
    StatusGetString(notif, FIRST_NOTIF, LAST_NOTIF);

    if (strcmp(lastNotif, notif) != 0) {
        RECT textRect;
        GetTextDimensions(lastNotif, font, &textRect);
        textRect.x = 0;
        textRect.y = 0;
        char * lines[] = {notif};
        DrawTextBox(lines, 1, textRect, 0, LEFT_JUSTIFIED, font, WHITE, BLACK);
        strcpy(lastNotif, notif);
    }
    if (strcmp(lastStatus, status) != 0) {
        RECT textRect;
        GetTextDimensions(lastStatus, font, &textRect);
        textRect.x = SCREEN_WIDTH - textRect.width;
        textRect.y = 0;
        char * lines[] = {status};
        DrawTextBox(lines, 1, textRect, 0, RIGHT_JUSTIFIED, font, WHITE, BLACK);
        strcpy(lastStatus, status);
    }

    int h = GetFontHeight(font);
    DrawLine(0, h + 0, SCREEN_WIDTH - 1, h + 0, BLACK);
    DrawLine(0, h + 1, SCREEN_WIDTH - 1, h + 1, WHITE);
    DrawLine(0, h + 2, SCREEN_WIDTH - 1, h + 2, BLACK);
    assert(h + 3 == STATUS_BAR_HEIGHT);
}
