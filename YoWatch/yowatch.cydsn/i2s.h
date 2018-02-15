#ifndef _I2S_H_
#define _I2S_H_

#include <project.h>

void I2sGetBuf(
    uint8 * buf
    );

uint8 * I2sStartDma(
    uint8 * buf,
    void (*isr)(void)
    );

void I2sStopDma();

#endif

