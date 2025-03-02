#include "queue.h"
#include "queue_port.h"

#include <string.h>

void Queue_Init(struct Queue *queue)
{
    Queue_Clear(queue);
    memset(queue->buffer, 0, queue->len * queue->itemSize);
}

void Queue_Clear(struct Queue *queue)
{
    queue->_writePtr = queue->_readPtr = queue->buffer;
    queue->_count = 0;
}

int Queue_Push(struct Queue *queue, const void *item)
{
    if (Queue_IsFull(queue)) {
        QueueFullErrorHook(queue);
        return -1;
    }

    memcpy(queue->_writePtr, item, queue->itemSize);
    queue->_writePtr += queue->itemSize;
    if (queue->_writePtr > (uint8_t *)queue->buffer + queue->len * queue->itemSize)
        queue->_writePtr = queue->buffer;
    queue->_count++;

    return 0;
}

int Queue_Pop(struct Queue *queue, void *item)
{
    if (Queue_IsEmpty(queue)) {
        QueueEmptyErrorHook(queue);
        return -1;
    }

    memcpy(item, queue->_readPtr, queue->itemSize);
    queue->_readPtr += queue->itemSize;
    if (queue->_readPtr > (uint8_t *)queue->buffer + queue->len * queue->itemSize)
        queue->_readPtr = queue->buffer;
    queue->_count--;

    return 0;
}

bool Queue_IsFull(const struct Queue *queue)
{
    return queue->_writePtr == queue->_readPtr && queue->_count != 0;
}

bool Queue_IsEmpty(const struct Queue *queue)
{
    return queue->_count == 0;
}