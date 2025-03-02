#ifndef _INC_MOTION_H
#define _INC_MOTION_H

#include "module.h"

enum MotionMode {
    MotionMode_SLOW,
    MotionMode_FAST,
};

struct MotionCorrection {
    int32_t distanceInMm;
};

enum Motion {
    Motion_PARK_BACK2WALL,
    Motion_PARK_FWD2DP,

    Motion_FWD_DP2DP,
    Motion_FWD_DP2C,

    Motion_SMOOTH_LEFT90,
    Motion_SMOOTH_RIGHT90,

    Motion_PIVOT_LEFT90,
    Motion_PIVOT_RIGHT90,
    Motion_PIVOT_LEFT180,
    Motion_PIVOT_RIGHT180,
};

void Motion_SetMode(enum MotionMode newMode);
void Motion_SetDiscreteMotion(FunctionalState newState);
void Motion_Start(enum Motion motion);
bool Motion_IsOngoing(void);
void Motion_Update(void);
void Motion_SetCorrection(struct MotionCorrection newCorrection);
struct MotionCorrection Motion_GetCorrection(void);

extern struct Module Motion_module;

#endif // _INC_MOTION_H