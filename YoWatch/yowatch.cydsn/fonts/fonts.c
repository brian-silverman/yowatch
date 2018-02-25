#include "fonts.h"

extern struct FONT_CHAR font_5x8[];
extern struct FONT_CHAR font_5x8_fixed[];

struct FONT_CHAR *fonts[MAX_FONTS] = {
    font_5x8,
    font_5x8_fixed,
};

