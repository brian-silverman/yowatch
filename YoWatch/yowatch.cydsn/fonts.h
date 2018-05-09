#ifndef _FONTS_H_
#define _FONTS_H_

#include <project.h>
#include "fonts/_fonts.h"

struct FONT_CHAR {
    const uint16 * image;
    uint16 width;
    uint16 height;
};

#define MAX_CHARS 128
#define INTER_CHAR_SPACING 1

extern const struct FONT_CHAR *fonts[MAX_FONTS];

void GetTextDimensions(
    char * s,
    int font,
    int * width,
    int * height
    );

#endif
