#ifndef _LISTEN_H_
#define _LISTEN_H_

#define LISTEN_QUEUE_NUM_BUFS 16

#define LISTEN_MIN_BUFS_ABOVE_LOW_THRESHOLD 15
#define LISTEN_MIN_BUFS_ABOVE_HIGH_THRESHOLD 12

#define LISTEN_LOW_THRESHOLD (5000)
#define LISTEN_HIGH_THRESHOLD (100000)

int ListenActivity(
    );

int ListenTimeout(
    );

void ListenStart(
    );

void ListenStop(
    );

uint ListenLevel(
    );

#endif

