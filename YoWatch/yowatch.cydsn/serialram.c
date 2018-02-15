/*
 * serialram.c
 *
 * SPI Serial RAM functions
 *
 * Copyright (C) 2018 Brian Silverman <bri@readysetstem.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 */
#include <project.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "printf.h"
#include "assert.h"
#include "spi.h"
#include "serialram.h"

#define SERIAL_RAM_WRITE_CMD 0x02
#define SERIAL_RAM_READ_CMD 0x03

#define COMMAND_HEADER_LEN (offsetof(struct SerialRamPdu, buf))

//
// Memory availability bitmask
//
#define MEM_BITMASK(index) (1 << index)
int availableRamBitmask = -1;

struct SerialRamPdu {
    uint8 command[4];
    uint8 buf[SERIAL_RAM_BUFSIZE];
} serialRamBufArray[SERIAL_RAM_NUM_BUFS];

static struct SerialRamPdu * SetCommand(
    uint8 * buf,
    uint32 address,
    uint8 command
    )
{
    struct SerialRamPdu * pcmdbuf;

    assert((address & 0xFF000000) == 0);

    pcmdbuf = (struct SerialRamPdu *) (buf - COMMAND_HEADER_LEN);
    pcmdbuf->command[0] = command;
    pcmdbuf->command[1] = (address >> 16) & 0xFF;
    pcmdbuf->command[2] = (address >>  8) & 0xFF;
    pcmdbuf->command[3] = (address >>  0) & 0xFF;

    return pcmdbuf;
}

//
// @return slave select given address
//
static uint32 AddressToSlaveSelect(
    uint32 address
    )
{
    int assumedRamBitmask = availableRamBitmask;
    if (assumedRamBitmask < 0) {
        assumedRamBitmask = MEM_BITMASK(0) | MEM_BITMASK(1);
    }
    if ((assumedRamBitmask & MEM_BITMASK(0)) && (assumedRamBitmask & MEM_BITMASK(1))) {
        //
        // Assuming SERIAL_RAM_SIZE is a power of two size, we can just
        // bitwise-AND to check which space it is in.  This also will
        // convienently allow addresses that are greater than the size of the
        // full RAM space to wrap around.
        //
        return (address & SERIAL_RAM_SIZE) ? SPI_SS_MEM_1 : SPI_SS_MEM_0;
    } else if (assumedRamBitmask & MEM_BITMASK(0)) {
        return SPI_SS_MEM_0;
    } else if (assumedRamBitmask & MEM_BITMASK(1)) {
        return SPI_SS_MEM_1;
    }
    return 0;
}

//  
// Serial RAM Read
//
// For efficiency, we use the same buffer for both send and receive.  This
// means that garbage goes out on the wire, which is okay because only the
// command portion is used by the part (the incoming data to the serial RAM is
// ignored for the read command).
//
static int Read(
    uint8 * buf,
    uint32 address,
    uint32 bytes,
    void (*doneCallback)(void *),
    void * doneArg,
    uint32 flags
    )
{
    uint32 ss = AddressToSlaveSelect(address);
    struct SerialRamPdu * pcmdbuf = SetCommand(buf, address, SERIAL_RAM_READ_CMD);
    return SpiXfer(
        (uint8 *) pcmdbuf,
        (uint8 *) pcmdbuf,
        bytes + COMMAND_HEADER_LEN,
        ss,
        doneCallback,
        doneArg,
        flags
        );
}

//  
// Serial RAM Write
//
static int Write(
    uint8 * buf,
    uint32 address,
    uint32 bytes,
    void (*doneCallback)(void *),
    void * doneArg,
    uint32 flags
    )
{
    uint32 ss = AddressToSlaveSelect(address);
    struct SerialRamPdu * pcmdbuf = SetCommand(buf, address, SERIAL_RAM_WRITE_CMD);
    return SpiXfer(
        (uint8 *) pcmdbuf,
        NULL,
        bytes + COMMAND_HEADER_LEN,
        ss,
        doneCallback,
        doneArg,
        flags
        );
}

static int BlockingReadWrite(
    int (*call)(uint8 *, uint32, uint32, void (*)(void *), void *, uint32),
    uint8 * buf,
    uint32 address,
    uint32 bytes
    )
{
    int ret, done;
    ret = call(buf, address, bytes, NULL, &done, SPI_DONE_CALLBACK);
    if (ret != 0 && ret != -EAGAIN) return ret;
    while (!done);
    return 0;
}

//
// Fill buf with a randomish mix of numbers.  Write it then read back from
// memory and verify
//
// @param address   where to read/wrtie mem
// @param skipVal   amount to skip when generating randomish numbers.  Any old
//                  value will do, a medium sized odd number is a good pick
//
// @return 1 if mem exists, else 0.
//
static int memExists(
    uint32 address,
    uint8 skipVal
    )
{
    int i;
    uint8 x;
    uint8 * pbuf1 = SerialRamGetBuf(0);
    uint8 * pbuf2 = SerialRamGetBuf(1);

    for (i = 0, x = 0; i < SERIAL_RAM_BUFSIZE; i++, x += skipVal) {
        pbuf1[i] = x;
    }
    memset(pbuf2, 0, SERIAL_RAM_BUFSIZE);
    BlockingReadWrite(Write, pbuf1, address, SERIAL_RAM_BUFSIZE);
    BlockingReadWrite(Read, pbuf2, address, SERIAL_RAM_BUFSIZE);
    for (i = 0, x = 0; i < SERIAL_RAM_BUFSIZE; i++, x += skipVal) {
        if (pbuf2[i] != x) break;
    }
    return (i == SERIAL_RAM_BUFSIZE);
}
    
//
// @return 1 if given RAM exists.  Requires SerialRamInit() was run.
//
uint32 SerialRamMemExists(
    uint32 memIndex
    )
{
    assert(availableRamBitmask >= 0);
    return availableRamBitmask & MEM_BITMASK(memIndex);
}

//
// Return total size based on available RAM.  Requires SerialRamInit() was run.
//
uint32 SerialRamTotalSize()
{
    uint32 size = 0;
    assert(availableRamBitmask >= 0);
    if (availableRamBitmask & MEM_BITMASK(0)) {
        size += SERIAL_RAM_SIZE;
    }
    if (availableRamBitmask & MEM_BITMASK(1)) {
        size += SERIAL_RAM_SIZE;
    }
    return size;
}

//
// Determine serial RAM address size by detecting if we have 0, 1, or 2 serial
// RAMs.
//
void SerialRamInit()
{
    int mem0;
    int mem1;

    mem0 = memExists(0, 0x33) ? MEM_BITMASK(0) : 0;
    mem1 = memExists(SERIAL_RAM_SIZE, 0x55) ? MEM_BITMASK(1) : 0;

    availableRamBitmask = mem0 | mem1;
}

//
// SerialRamGetBuf()
//
// @param   index   Index of buf to get.  SERIAL_RAM_NUM_BUFS bufs are available.
//
// @return  Buffer that can be used to read/write to/from serial RAM.  Size of buf is
// SERIAL_RAM_BUFSIZE.
//
// Bufs are limited and have specific purposes - so you have to keep track of
// who is using each buf.
//
uint8 * SerialRamGetBuf(
    int index
    )
{
    assert(index < SERIAL_RAM_NUM_BUFS);
    return serialRamBufArray[index].buf;
}
  
//
// Write given buf to serial RAM.  Nonblocking.
//
// @param buf           Buffer to write, from calling SerialRamGetBuf()
// @param address       Address to write to.
// @param bytes         Number of bytes of buf to write
// @param doneCallback  Function to call after transaction completes
// @param doneArg       Arg to pass to doneCallback
//
// @return 0 or -EAGAIN on success, else negative error.  See SpiXfer() return values.
//
int SerialRamWrite(
    uint8 * buf,
    uint32 address,
    uint32 bytes,
    void (*doneCallback)(void *),
    void * doneArg
    )
{
    return Write(buf, address, bytes, doneCallback, doneArg, 0);
}

//
// Blocking version of SerialRamWrite()
//
int SerialRamWriteBlocking(
    uint8 * buf,
    uint32 address,
    uint32 bytes
    )
{
    return BlockingReadWrite(Write, buf, address, bytes);
}

//
// Read given buf from serial RAM.  Nonblocking.
//
// Same parameters as SerialRamWrite(), but for read.
//
// @return See SerialRamWrite()
//
int SerialRamRead(
    uint8 * buf,
    uint32 address,
    uint32 bytes,
    void (*doneCallback)(void *),
    void * doneArg
    )
{
    return Read(buf, address, bytes, doneCallback, doneArg, 0);
}

//
// Blocking version of SerialRamRead()
//
int SerialRamReadBlocking(
    uint8 * buf,
    uint32 address,
    uint32 bytes
    )
{
    return BlockingReadWrite(Read, buf, address, bytes);
}

//
// Fill all of RAM with one byte value
//
int SerialRamFillBlocking(
    uint8 val
    )
{
    uint8 * pbuf1 = SerialRamGetBuf(0);
    int ret;
    int i;

    memset(pbuf1, val, SERIAL_RAM_BUFSIZE);
    for (i = 0; i < SERIAL_RAM_SIZE; i += SERIAL_RAM_BUFSIZE) {
        ret = SerialRamWriteBlocking(pbuf1, i, SERIAL_RAM_BUFSIZE);
        if (ret) return ret;
    }
    return 0;
}

