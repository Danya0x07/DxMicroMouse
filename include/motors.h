#ifndef _INC_MOTORS_H
#define _INC_MOTORS_H

#include "module.h"

void Motors_SetPwm(int16_t left, int16_t right);
void Motors_GetPwm(int16_t *left, int16_t *right);

extern struct Module Motors_module;

#endif // _INC_MOTORS_H