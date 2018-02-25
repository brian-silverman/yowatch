#ifndef _OLED_H_
#define _OLED_H_

#include <project.h>

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

#endif
