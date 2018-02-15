#ifndef _SERIALRAM_H_
#define _SERIALRAM_H_

#include <project.h>

#define SERIAL_RAM_BUFSIZE 256

#define SERIAL_RAM_SIZE (128 * 1024)

#define SERIAL_RAM_NUM_BUFS 3

uint32 SerialRamMemExists(
    uint32 memIndex
    );

uint32 SerialRamTotalSize();

void SerialRamInit();

//
// SerialRamGetBuf()
//
// @param   index   Index of buf to get.  SERIAL_RAM_NUM_BUFS bufs are available.
//

uint8 * SerialRamGetBuf(
    int index
    );
    
int SerialRamWrite(
    uint8 * buf, 
    uint32 address, 
    uint32 bytes, 
    void (*doneCallback)(void *),
    void * doneArg
    );

int SerialRamWriteBlocking(
    uint8 * buf, 
    uint32 address, 
    uint32 bytes
    );

int SerialRamRead(
    uint8 * buf, 
    uint32 address, 
    uint32 bytes, 
    void (*doneCallback)(void *),
    void * doneArg
    );

int SerialRamReadBlocking(
    uint8 * buf, 
    uint32 address, 
    uint32 bytes
    );

int SerialRamFillBlocking(
    uint8 val
    );

#endif

