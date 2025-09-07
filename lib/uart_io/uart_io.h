#ifndef _INC_UART_IO_H
#define _INC_UART_IO_H

#include <stdint.h>
#include <stdbool.h>

void UART_SendChar(char c);
void UART_SendString(const char *str);
char UART_ReadChar(void);
bool UART_LineReceived(void);
uint32_t UART_ReadLine(char *buff, uint32_t size);
void UART_Flush(void);

void UART_RxCallback(void);

#endif // _INC_UART_IO_H