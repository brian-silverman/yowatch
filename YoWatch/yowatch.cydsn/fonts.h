#ifndef _FONTS_H_
#define _FONTS_H_

#include <project.h>

struct FONT_CHAR {
    uint8 * image;
    uint16 width;
    uint16 height;
};

enum {
    FONT_5X8,
    FONT_5X8_FIXED,
    MAX_FONTS
};

#define MAX_CHARS 128

extern struct FONT_CHAR *fonts[MAX_FONTS];

#endif


