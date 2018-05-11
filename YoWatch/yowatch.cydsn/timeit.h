#ifndef _TIMEIT_H_
#define _TIMEIT_H_

void TimeItAndPrint(
    void (*func)(),
    int runs
    );

int TimeIt(
    void (*func)(),
    int runs
    );

#endif



