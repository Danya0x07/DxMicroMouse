#ifndef _INC_SPEEDCTL_H
#define _INC_SPEEDCTL_H

#include "module.h"
#include "mcu.h"

typedef enum {
    SpeedCtlMode_STRAIGHT,
    SpeedCtlMode_TURN,
    SpeedCtlMode_BACKTRIM,
    SpeedCtlMode_DIAGONAL = SpeedCtlMode_TURN
} SpeedCtlMode;

void SpeedCtl_Reset(void);
void SpeedCtl_SetState(FunctionalState newState);
void SpeedCtl_SetMode(SpeedCtlMode newMode);
void SpeedCtl_Setup(void);
void SpeedCtl_SetTarget(int32_t vTransInMmPerS, int32_t vRotInDegPerS);
void SpeedCtl_GetSpeed(int32_t *vTransInMmPerS, int32_t *vRotInDegPerS);
void SpeedCtl_Update(void);

extern struct Module SpeedCtl_module;

#endif // _INC_SPEEDCTL_H