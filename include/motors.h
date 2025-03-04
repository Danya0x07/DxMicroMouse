#ifndef _INC_MOTORS_H
#define _INC_MOTORS_H

#include "module.h"

void Motors_SetPwm(int left, int right);
void Motors_GetPwm(int *left, int *right);

extern struct Module Motors_module;

#endif // _INC_MOTORS_H