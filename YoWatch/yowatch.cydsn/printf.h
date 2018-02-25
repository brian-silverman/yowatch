#ifndef _PRINTF_H_
#define _PRINTF_H_

#include <stdio.h>

#define printf ERROR

#define MAX_FORMAT_BUFFER_SIZE (128)
extern uint8 formatBuffer[MAX_FORMAT_BUFFER_SIZE];
#define xprintf(...) \
    { \
        snprintf((char *)formatBuffer, MAX_FORMAT_BUFFER_SIZE, __VA_ARGS__); \
        formatBuffer[MAX_FORMAT_BUFFER_SIZE-1] = '\0'; \
        UART_1_PutString((const char *) formatBuffer); \
    }

#endif
