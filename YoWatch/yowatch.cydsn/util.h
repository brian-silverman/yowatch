#ifndef _UTIL_H_
#define _UTIL_H_

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define ABS(a) (((a) < 0) ? -(a) : (a))
#define CROP(a,b,c) (((b) < (a)) ? (a) : (((b) > (c)) ? (c) : (b)))
#define ARRAY_SIZEOF(a) (sizeof(a)/sizeof(a[0]))

int AudioBufMean(
    uint8 * pbuf
    );

uint AudioBufVariance(
    uint8 * pbuf
    );

#endif
