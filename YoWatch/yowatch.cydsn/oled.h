#ifndef _OLED_H_
#define _OLED_H_

#include <project.h>
#include "display.h"
#include "rect.h"

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
    uint32 x1,
    uint32 y1,
    uint32 x2,
    uint32 y2
    );

#endif
