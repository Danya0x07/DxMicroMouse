#ifndef _INC_QUEUE_PORT_H
#define _INC_QUEUE_PORT_H

#include <stdio.h>

#include "buzzer.h"

static inline void QueueErrorHook(void)
{
    Buzzer_Blink(6, 800, 200);
    for (;;) {}
}

static inline void QueueFullErrorHook(struct Queue *queue)
{
    printf("QUEUE OVF: %d\n", queue->_count);
    QueueErrorHook();
}

static inline void QueueEmptyErrorHook(struct Queue *queue)
{
    printf("QUEUE UNF, LEN: %d\n", queue->len);
    QueueErrorHook();
}

#endif // _INC_QUEUE_PORT_H