#ifndef _INC_SENSORS_H
#define _INC_SENSORS_H

#include <scheduler.h>
#include <shell.h>
#include <settings.h>

#include "mcu.h"

struct SensorsDistance {
    int32_t leftFront;
    int32_t leftSide;
    int32_t rightSide;
    int32_t rightFront;
};

struct SensorsWalls {
    bool front;
    bool left;
    bool right;
};

extern void (*Sensors_Update)(void);
void Sensors_SetState(FunctionalState state);
FunctionalState Sensors_GetState(void);
void Sensors_ReadDistance(struct SensorsDistance *distance);
void Sensors_ReadWalls(struct SensorsWalls *walls);
bool Sensors_DetectFinger(void);
bool Sensors_DetectTransition(void);

int32_t Sensors_GetStraightDeviation(void);
void Sensors_GetTrimmingErrors(int32_t *transError, int32_t *rotError);

extern struct SchedulerTask TASK_TmSensors;
extern const struct ShellCommand CMD_Sensors;
extern const struct Settings SETT_Sensors;

#endif // _INC_SENSORS_H