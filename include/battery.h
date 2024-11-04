#ifndef _INC_BATTERY_H
#define _INC_BATTERY_H

#include "module.h"

typedef enum {
    BatteryStatus_DEAD = 0,
    BatteryStatus_LOW,
    BatteryStatus_MEDIUM,
    BatteryStatus_HIGH,
    BatteryStatus_FULL
} BatteryStatus;

void Battery_Update(void);
BatteryStatus Battery_GetStatus(void);

extern struct Module Battery_module;

#endif // _INC_BATTERY_H