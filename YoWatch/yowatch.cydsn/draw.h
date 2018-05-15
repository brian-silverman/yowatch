#ifndef _DRAW_H_
#define _DRAW_H_

#include <rect.h>

#define LEFT_JUSTIFIED      0
#define CENTER_JUSTIFIED    1
#define RIGHT_JUSTIFIED     2

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

#endif
