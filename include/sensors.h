#ifndef _INC_SENSORS_H
#define _INC_SENSORS_H

#include "mcu.h"
#include "module.h"

extern void (*Sensors_Update)(void);
void Sensors_SetLightening(FunctionalState state);
void Sensors_ReadToBuffer(uint16_t buffer[5]);

extern struct Module Sensors_module;

#endif // _INC_SENSORS_H