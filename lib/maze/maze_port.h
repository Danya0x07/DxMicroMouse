#ifndef _INC_MAZE_PORT_H
#define _INC_MAZE_PORT_H

#include <uart_io.h>

#define MAZE_PUTC(x)    (UART_SendChar((x)))
#define MAZE_PUTS(x)    (UART_SendString((x)))

#endif // _INC_MAZE_PORT_H