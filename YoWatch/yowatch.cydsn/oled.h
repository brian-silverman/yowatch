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

void DisplayChar(
    unsigned char c,
    uint32 x,
    uint32 y,
    int font,
    int fgcolor,
    int bgcolor
    );

void DisplayText(
    unsigned char * text,
    uint32 x,
    uint32 y,
    int font,
    int fgcolor,
    int bgcolor
    );

#endif
