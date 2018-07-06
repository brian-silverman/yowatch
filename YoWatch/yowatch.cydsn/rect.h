#ifndef _RECT_H_
#define _RECT_H_

#include "display.h"

//
// Note about RECT: Passing and returning RECTs to functions is a slight
// performance hit, but overal lends itself to greater readability.  Timing of
// function passing a RECT (via func((RECT) {1,2,3,4}) was 167usecs per call vs
// direct passing of parameters at 68 usecs per call - a ~100usec cost.  This
// is much less by comparison to the OLED transfer times (for any reasonably
// sized rect).
//
typedef struct {
    int x;
    int y;
    int width;
    int height;
} RECT;

#endif
