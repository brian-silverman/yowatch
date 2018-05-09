#ifndef _UTIL_H_
#define _UTIL_H_

#define MIN(a,b) ((a < b) ? (a) : (b))
#define MAX(a,b) ((a > b) ? (a) : (b))
#define CROP(x,a,b) (x < a ? (a) : (x > b ? (b) : (x)))
#define ARRAY_SIZEOF(a) (sizeof(a)/sizeof(a[0]))

#endif

