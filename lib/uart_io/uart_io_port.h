#ifndef _INC_UART_IO_PORT_H
#define _INC_UART_IO_PORT_H

#include "mcu.h"

#define RECV_IRQ_ON()   NVIC_EnableIRQ(USART1_IRQn)
#define RECV_IRQ_OFF()  NVIC_DisableIRQ(USART1_IRQn)

static inline char UART_ReceiveChar(void)
{
    return USART_ReceiveData(USART1);
}

void UART_SendChar(char c)
{
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
        ;
    USART_SendData(USART1, c);
}

#endif // _INC_UART_IO_PORT_H