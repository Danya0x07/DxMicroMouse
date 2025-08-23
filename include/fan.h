#ifndef _INC_FAN_H
#define _INC_FAN_H

#include "module.h"

void Fan_On(void);
void Fan_Off(void);
void Fan_SetPwm(uint16_t duty);
bool Fan_IsOn(void);

extern struct Module Fan_module;

#endif // _INC_FAN_H