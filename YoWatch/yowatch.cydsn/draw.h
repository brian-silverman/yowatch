#ifndef _DRAW_H_
#define _DRAW_H_

#include <rect.h>

#define LEFT_JUSTIFIED      0x01
#define HCENTER_JUSTIFIED   0x02
#define RIGHT_JUSTIFIED     0x03
#define HORIZ_JUST_MASK     0x03

#define TOP_JUSTIFIED       0x04
#define VCENTER_JUSTIFIED   0x08
#define BOTTOM_JUSTIFIED    0x0C
#define VERT_JUST_MASK      0x0C

extern RECT SCREEN_BOUNDS;

void DrawText(
    char * text,
    int x,
    int y,
    int font,
    int fgcolor,
    int bgcolor
    );

void DrawRect(
    RECT r,
    int color
    );

void DrawTextBox(
    char * lines[],
    int num,
    RECT box,
    int shiftUp,
    int justify,
    int font,
    int fgColor,
    int bgColor
    );

void DrawTextBounded(
    char * text,
    int x,
    int y,
    int font,
    int fgcolor,
    int bgcolor,
    RECT bound
    );

int DoRectsIntersect(
    RECT r1,
    RECT r2
    );

RECT ExpandedRect(
    RECT r,
    int border
    );

RECT RectIntersection(
    RECT r1,
    RECT r2
    );

void DrawLine(
    int x1,
    int y1,
    int x2,
    int y2,
    int color
    );

void DrawStatusBar(
    );

#endif
