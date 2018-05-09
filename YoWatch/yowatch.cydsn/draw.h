#ifndef _DRAW_H_
#define _DRAW_H_

void DrawText(
    char * text,
    int x,
    int y,
    int font,
    int fgcolor,
    int bgcolor
    );

void DrawBorderedRect(
    int x1,
    int y1,
    int x2,
    int y2,
    int color,
    int border,
    int borderColor
    );

void DrawRect(
    int x1,
    int y1,
    int x2,
    int y2,
    int color
    );

#endif

