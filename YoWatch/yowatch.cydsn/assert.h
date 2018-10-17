#ifndef _ASSERT_H_
#define _ASSERT_H_

#include "printf.h"

void _assert(int assertion, char * assertionStr, int line, char * file);

#define assert(x) _assert(x, #x, __LINE__, __FILE__)

#define assert_printf(ass, fmt, args...) {if (!(ass)) xprintf(fmt, args); assert(ass);}

#endif
