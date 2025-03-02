#ifndef _INC_QUEUE_H
#define _INC_QUEUE_H

#include <stdint.h>
#include <stdbool.h>

struct Queue {
    const uint_fast8_t itemSize;
    const uint_fast8_t len;
    void *const buffer;  // array[len] of itemSize elements

    uint8_t *_writePtr;
    uint8_t *_readPtr;
    uint_fast8_t _count;
};

void Queue_Init(struct Queue *queue);
void Queue_Clear(struct Queue *queue);
int Queue_Push(struct Queue *queue, const void *item);
int Queue_Pop(struct Queue *queue, void *item);
bool Queue_IsFull(const struct Queue *queue);
bool Queue_IsEmpty(const struct Queue *queue);

#endif // _INC_QUEUE_H