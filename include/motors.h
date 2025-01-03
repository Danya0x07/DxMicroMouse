#ifndef _INC_MOTORS_H
#define _INC_MOTORS_H

#include "module.h"

void Motors_SetDutyLeft(int16_t duty);
void Motors_SetDutyRight(int16_t duty);
void Motors_SetTargetDuty(int16_t left, int16_t right);
void Motors_Update(void);

extern struct Module Motors_module;

#endif // _INC_MOTORS_H