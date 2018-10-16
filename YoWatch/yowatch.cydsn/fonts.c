/*
 * fonts.c
 *
 * Font Functions
 *
 * Copyright (C) 2018 Brian Silverman <bri@readysetstem.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 */
#include <project.h>
#include "printf.h"
#include "assert.h"
#include "fonts.h"
#include "util.h"

//
// Get pixel width/height of string for a given font
//
// @param text      string to get width/height of.  Can be NULL if getting only
//                  height.
// @param font      font
// @param r         Required output RECT.  Only width/height are set.
//
void GetTextDimensions(
    char * text,
    int font,
    RECT * r
    )
{
    const struct FONT_CHAR * pfont = fonts[font];
    int _width = 0;
    int _height = 0;

    if (!text || *text == '\0') {
        _width = 0;
        _height = pfont[0].height;
    } else {
        int interCharSpacing = GetInterCharSpacing(font);
        for (char * t = text; *t != '\0'; t++) {
            assert(*t >= 0 && *t < MAX_CHARS);
            const struct FONT_CHAR * p = &pfont[(int) *t];
            _width += p->width + interCharSpacing;
            _height = MAX(_height, p->height);
            // For now, only handle fixed height fonts
            assert(_height == p->height);
        }
        _width--;
    }

    r->width = _width;
    r->height = _height;
}

//
// Gets font height (assume fixed height fonts)
//
int GetFontHeight(
    int font
    )
{
    RECT r;
    GetTextDimensions("", font, &r);
    return r.height;
}

//
// Gets width of a char for a given font 
//
int GetCharWidth(
    char c,
    int font
    )
{
    const struct FONT_CHAR * pfont = fonts[font];
    const struct FONT_CHAR * p = &pfont[(int) c];
    return p->width;
}

//
// Gets inter char spacing, based on font size
//
int GetInterCharSpacing(
    int font
    )
{
    return GetFontHeight(font) > 20 ? 2 : 1;
}

//
// Gets inter line spacing, based on font size
//
int GetInterLineSpacing(
    int font
    )
{
    return GetFontHeight(font) > 20 ? 2 : 1;
}

