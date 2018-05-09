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
// @param text         string to get width/height of.  Can be NULL if getting only
//                  height.
// @param font      font
// @param width     optional output width.  Requires valid string /text/.
// @param height    output height.  Does not require string /text/.  Currently
//                  uses height of first char - assumes fixed height fonts.
//
void GetTextDimensions(
    char * text,
    int font,
    int * width,
    int * height
    )
{
    const struct FONT_CHAR * pfont = fonts[font];
    int _width = 0;
    int _height = 0;

    if (!text || *text == '\0') {
        _width = 0;
        _height = pfont[0].height;
    } else {
        for (char * t = text; *t != '\0'; t++) {
            assert(*t >= 0 && *t < MAX_CHARS);
            const struct FONT_CHAR * p = &pfont[(int) *t];
            _width += p->width + INTER_CHAR_SPACING;
            _height = MAX(_height, p->height);
            // For now, only handle fixed height fonts
            assert(_height == p->height);
        }
        _width--;
    }

    if (width) *width = _width;
    if (height) *height = _height;
}

