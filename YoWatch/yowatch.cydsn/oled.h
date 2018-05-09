#ifndef _OLED_H_
#define _OLED_H_

#include <project.h>

#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   96

void DisplayInit();

void DisplayRect(
    uint32 x,
    uint32 y,
    uint32 width,
    uint32 height,
    uint16 color
    );

void DisplayErase();

void DisplayFill(
    uint16 color
    );

void DisplayBitmap(
    uint8 * buf,
    int x1,
    int y1,
    int x2,
    int y2
    );

#endif
