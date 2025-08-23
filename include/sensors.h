#ifndef _INC_SENSORS_H
#define _INC_SENSORS_H

#include "module.h"
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

int32_t Sensors_GetSteeringError(void);

extern struct Module Sensors_module;

#endif // _INC_SENSORS_H