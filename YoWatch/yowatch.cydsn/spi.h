#ifndef _SPI_H_
#define _SPI_H_

#include <project.h>

#define NUM_SPI_QDS 8

//
// Flags
//
#define SPI_DONE_CALLBACK (1 << 0)


//
// Slave selects
//
#define SPI_SS_MEM_0    SPI_1_SPI_SLAVE_SELECT0
#define SPI_SS_MEM_1    SPI_1_SPI_SLAVE_SELECT1
#define SPI_SS_OLED     SPI_1_SPI_SLAVE_SELECT2

extern int spiRxIsrCalls;
extern int spiTxIsrCalls;

void SpiXferUnlocked(
    uint8 * sendBuf,
    uint8 * recvBuf,
    uint32 bytes,
    uint32 ss,
    void (*doneCallback)(void *),
    void * doneArg,
    uint32 flags
    );

int SpiXfer(
    uint8 * sendBuf,
    uint8 * recvBuf,
    uint32 bytes,
    uint32 ss,
    void (*doneCallback)(void *),
    void * doneArg,
    uint32 flags
    );

CY_ISR_PROTO(SpiTxIsr);

#endif

