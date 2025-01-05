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
void SpeedCtl_Setup(float kpv, float kdv, float kpw, float kdw);
void SpeedCtl_SetTarget(int32_t v, int32_t w);
void SpeedCtl_Update(void);
int32_t SpeedCtl_GetActualTransSpeed(void);
int32_t SpeedCtl_GetActualRotSpeed(void);

extern struct Module SpeedCtl_module;

#endif // INC_SPEEDCTL_H