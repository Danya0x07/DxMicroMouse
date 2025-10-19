#ifndef _INC_MOTION_H
#define _INC_MOTION_H

#include "mcu.h"

struct MotionConfig {
    int32_t aTrans;
    int32_t aRot;
};

struct Motion {
    int32_t distanceInMm;
    int32_t angleInDeg;
};

extern const struct Motion
    MOTION_BACK_PARK_1,
    MOTION_BACK_PARK_2,
    MOTION_FWD_UNPARK2M,
    MOTION_FWD_UNPARK2C,
    MOTION_FWD_M2M,
    MOTION_FWD_M2T90,
    MOTION_FWD_M2C,
    MOTION_LS90_1,
    MOTION_LS90_2,
    MOTION_RS90_1,
    MOTION_RS90_2,
    MOTION_LP90_1,
    MOTION_LP90_2,
    MOTION_RP90_1,
    MOTION_RP90_2,
    MOTION_LP180_1,
    MOTION_LP180_2,
    MOTION_RP180_1,
    MOTION_RP180_2,
    MOTION_LS180_1,
    MOTION_LS180_2,
    MOTION_RS180_1,
    MOTION_RS180_2,
    MOTION_FWD_C245,
    MOTION_FWD_C2135,
    MOTION_LS45_1,
    MOTION_LS45_2,
    MOTION_RS45_1,
    MOTION_RS45_2,
    MOTION_LS135_1,
    MOTION_LS135_2,
    MOTION_RS135_1,
    MOTION_RS135_2,
    MOTION_DFWD,
    MOTION_D2W,
    MOTION_DFWD_D2D,
    MOTION_LS90_D2D_1,
    MOTION_LS90_D2D_2,
    MOTION_RS90_D2D_1,
    MOTION_RS90_D2D_2
;

void Motion_Configure(const struct MotionConfig *newConfig);
void Motion_Start(const struct Motion *motion, int32_t endVTrans, int32_t endVRot);
bool Motion_IsOngoing(void);
void Motion_Update(void);

#endif // _INC_MOTION_H