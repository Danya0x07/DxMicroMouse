#include "uart.h"
#include "mcu.h"

static volatile char rxBuffer[0x80] = {0};
#define INDEX_MASK  (sizeof(rxBuffer) - 1)

static volatile uint32_t count = 0;
static volatile uint32_t writeIndex = 0;
static uint32_t readIndex = 0;

#define RECV_IRQ_ON()  NVIC_EnableIRQ(USART1_IRQn)
#define RECV_IRQ_OFF() NVIC_DisableIRQ(USART1_IRQn)

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

void UART_SendChar(char c)
{
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
        ;
    USART_SendData(USART1, c);
}

void UART_SendString(const char *str)
{
    while (*str)
        UART_SendChar(*str++);
}

__attribute__((interrupt("WCH-Interrupt-fast")))
void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        if (!BufferFull()) {
            rxBuffer[writeIndex++] = USART_ReceiveData(USART1);
            writeIndex &= INDEX_MASK;
            count++;
        } else {
            (void)USART_ReceiveData(USART1);
        }
    }
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