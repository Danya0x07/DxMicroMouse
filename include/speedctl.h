#ifndef _INC_SPEEDCTL_H
#define _INC_SPEEDCTL_H

#include "module.h"
#include "mcu.h"

typedef enum {
    SpeedCtlMode_ENCODER,
    SpeedCtlMode_IMU,
    SpeedCtlMode_COMBINED,
    SpeedCtlMode_TEST_PERPENDICULAR
} SpeedCtlMode;

void SpeedCtl_Reset(void);
void SpeedCtl_SetState(FunctionalState newState);
void SpeedCtl_SetMode(SpeedCtlMode mode);
void SpeedCtl_Setup(int32_t vTransKp, int32_t vTransKi, int32_t vRotKp, int32_t vRotKi);
void SpeedCtl_SetTarget(int32_t vTransInMmPerS, int32_t vRotInDegPerS);
void SpeedCtl_Update(void);
int32_t SpeedCtl_GetVTransInMmPerS(void);
int32_t SpeedCtl_GetVRotInDegPerS(void);

extern struct Module SpeedCtl_module;

#endif // _INC_SPEEDCTL_H