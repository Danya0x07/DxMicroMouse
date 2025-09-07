#ifndef _INC_FAN_H
#define _INC_FAN_H

#include <stdint.h>
#include <stdbool.h>
#include <shell.h>

void Fan_On(void);
void Fan_Off(void);
void Fan_SetPwm(uint16_t duty);
bool Fan_IsOn(void);

extern const struct ShellCommand CMD_Fan;

#endif // _INC_FAN_H