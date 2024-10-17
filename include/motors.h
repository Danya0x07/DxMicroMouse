#ifndef _INC_MOTORS_H
#define _INC_MOTORS_H

#include "module.h"

void Motors_SetL(int16_t duty);
void Motors_SetR(int16_t duty);
void Motors_Update(void);

extern struct Module Motors_module;

#endif // _INC_MOTORS_H