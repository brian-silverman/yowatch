/*
 * assert.c
 *
 * Copyright (C) 2018 Brian Silverman <bri@readysetstem.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 */
#include <project.h>
#include "printf.h"

//
// assert() function
// TODO: send assert to LCD display
//
void _assert(int assertion, char * assertionStr, int line, char * file)
{
    if (! assertion) {
        xprintf("ASSERT(%s) at line %d, %s.\r\n", assertionStr, line, file);
        CyHalt(0);
    }
}


