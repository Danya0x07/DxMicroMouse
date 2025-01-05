#ifndef INC_SPEEDCTL_H
#define INC_SPEEDCTL_H

#include "module.h"
#include "mcu.h"

typedef enum {
    SpeedCtlMode_ENCODER,
    SpeedCtlMode_IMU,
    SpeedCtlMode_COMBINED,
    SpeedCtlMode_TEST_PERPENDICULAR
} SpeedCtlMode;

void SpeedCtl_SetState(FunctionalState newState);
void SpeedCtl_SetMode(SpeedCtlMode mode);
void SpeedCtl_Setup(int32_t newTransKp, int32_t newTransKd, int32_t newRotKp, int32_t newRotKd);
void SpeedCtl_SetTarget(int32_t vTransInMmPerS, int32_t vRotInDegPerS);
void SpeedCtl_Update(void);
int32_t SpeedCtl_GetActualTransSpeed(void);
int32_t SpeedCtl_GetActualRotSpeed(void);

extern struct Module SpeedCtl_module;

#endif // INC_SPEEDCTL_H