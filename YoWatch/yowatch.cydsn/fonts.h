#ifndef _FONTS_H_
#define _FONTS_H_

#include <project.h>
#include "fonts/_fonts.h"
#include "rect.h"

struct FONT_CHAR {
    const uint16 * image;
    uint16 width;
    uint16 height;
};

#define MAX_CHARS 128

extern const struct FONT_CHAR *fonts[MAX_FONTS];

void GetTextDimensions(
    char * s,
    int font,
    RECT * r
    );

int GetInterCharSpacing(
    int font
    );

int GetInterLineSpacing(
    int font
    );

int GetFontHeight(
    int font
    );

int GetCharWidth(
    char c,
    int font
    );

#endif
