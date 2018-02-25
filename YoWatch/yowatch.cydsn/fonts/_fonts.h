#ifndef __FONTS_H_
#define __FONTS_H_

#include <project.h>
#include "fonts.h"

#define M 0xFFFF,
#define _ 0x0000,

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#define WIDTH(c) ARRAY_SIZE(c[0])
#define HEIGHT(c) ARRAY_SIZE(c)

#define F(c) {(uint8 *) &c, WIDTH(c), HEIGHT(c)}

#endif

