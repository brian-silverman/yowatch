#ifndef _SPI_H_
#define _SPI_H_

#include <project.h>

#define NUM_SPI_QDS 8

//
// Flags
//
#define SPI_DONE_CALLBACK       (1 << 0)
#define SPI_TX_BYTE_REPEATED    (1 << 1)
#define SPI_SHORT_AND_SWEET     (1 << 2)

//
// There are only 2 ping-ponged audio buffers, before heading to Serial RAM.
// Therefore the SPI bus can not be hogged for too long, or audio packes would
// be dropped.
//
// Audio, at 16-bit 16kHz, needs to write a 512 bytes packet every (512 bytes /
// (2 bytes * 16kHz)) = 16msecs.  To be safe, we restrict continuous bus usage
// to roughly half of that - at 8Mbps, that is (8Mbps * 8msec = 64kbits =
// 8kbytes).
//
#define MAX_SPI_BUS_HOGGING_BYTES 8000

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

