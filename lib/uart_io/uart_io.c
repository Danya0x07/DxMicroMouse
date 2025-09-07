#include "uart_io.h"
#include "uart_io_port.h"

static char rxBuffer[0x80] = {0};
#define INDEX_MASK  (sizeof(rxBuffer) - 1)

static uint32_t count = 0;
static uint32_t writeIndex = 0;
static uint32_t readIndex = 0;

static inline bool BufferEmpty(void)
{
    return count == 0;
}

static inline bool BufferFull(void)
{
    return count == sizeof(rxBuffer);
}

static char PeekLast(void)
{
    if (BufferEmpty())
        return 0;

    uint8_t idx = (writeIndex - 1) & INDEX_MASK;
    return rxBuffer[idx];
}

static inline bool IsEndOfLine(char c)
{
    return c == '\n';
}

void UART_SendString(const char *str)
{
    while (*str)
        UART_SendChar(*str++);
}

static inline char ReadChar(void)
{
    char c = '\0';

    if (!BufferEmpty()) {
        c = rxBuffer[readIndex++];
        readIndex &= INDEX_MASK;
        count--;
    }

    return c;
}

char UART_ReadChar(void)
{
    RECV_IRQ_OFF();
    char c = ReadChar();
    RECV_IRQ_ON();
    return c;
}

bool UART_LineReceived(void)
{
    char last = PeekLast();

    if (IsEndOfLine(last))
        return true;

    if (BufferFull())
        UART_Flush();

    return false;
}

uint32_t UART_ReadLine(char *buff, uint32_t size)
{
    if (!size)
        return 0;

    uint8_t len = 0;
    --size;  // for '\0'

    if (!UART_LineReceived())
        goto out;

    RECV_IRQ_OFF();

    char c = ReadChar();

    while (!IsEndOfLine(c) && size--) {
        *buff++ = c;
        len++;
        c = ReadChar();
    }

    RECV_IRQ_ON();

out:
    *buff = '\0';
    return len;
}

void UART_Flush(void)
{
    RECV_IRQ_OFF();
    readIndex = writeIndex = count = 0;
    RECV_IRQ_ON();
}

void UART_RxCallback(void)
{
    if (!BufferFull()) {
        rxBuffer[writeIndex++] = UART_ReceiveChar();
        writeIndex &= INDEX_MASK;
        count++;
    } else {
        (void)UART_ReceiveChar();
    }
}