#ifndef _TICK_H_
#define _TICK_H_

void TickInit();

uint GetTick();

int Expired(
    uint msec,
    uint * since
    );

#endif
