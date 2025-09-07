#ifndef _INC_SPEEDCTL_H
#define _INC_SPEEDCTL_H

#include <scheduler.h>
#include <shell.h>
#include <settings.h>

#include "mcu.h"

typedef enum {
    SpeedCtlMode_STRAIGHT,
    SpeedCtlMode_TURN,
    SpeedCtlMode_BACKTRIM,
    SpeedCtlMode_DIAGONAL = SpeedCtlMode_TURN
} SpeedCtlMode;

void SpeedCtl_Reset(void);
void SpeedCtl_SetState(FunctionalState newState);
FunctionalState SpeedCtl_GetState(void);
void SpeedCtl_SetMode(SpeedCtlMode newMode);
void SpeedCtl_Setup(void);
void SpeedCtl_SetTarget(int32_t vTransInMmPerS, int32_t vRotInDegPerS);
void SpeedCtl_GetSpeed(int32_t *vTransInMmPerS, int32_t *vRotInDegPerS);
void SpeedCtl_Update(void);

extern struct SchedulerTask TASK_TmSpeedCtl;
extern const struct ShellCommand CMD_SpeedCtl;
extern const struct Settings SETT_SpeedCtl;

#endif // _INC_SPEEDCTL_H