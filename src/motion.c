#include "motion.h"
#include "profile.h"
#include "speedctl.h"
#include "utils.h"

#include <stdio.h>

#define CELLWIDTH   180
#define CELLHALF    90
#define WALLTHICKNESS   12
#define MOUSEBACKLEN    30

#define FIRSTHALF(x)    ((x) / 2)
#define SECONDHALF(x)   ((x) - FIRSTHALF(x))

const struct Motion
    MOTION_BACK_PARK_1 = {
        .distanceInMm = FIRSTHALF(-(CELLHALF - MOUSEBACKLEN - WALLTHICKNESS/2 + 7)),
        .angleInDeg = 0
    },
    MOTION_BACK_PARK_2 = {
        .distanceInMm = SECONDHALF(-(CELLHALF - MOUSEBACKLEN - WALLTHICKNESS/2 + 7)),
        .angleInDeg = 0
    },
    MOTION_FWD_UNPARK2M = {
        .distanceInMm = +(CELLHALF - MOUSEBACKLEN - WALLTHICKNESS/2 + CELLHALF),
        .angleInDeg = 0
    },
    MOTION_FWD_UNPARK2C = {
        .distanceInMm = +(CELLHALF - MOUSEBACKLEN - WALLTHICKNESS/2),
        .angleInDeg = 0
    },
    MOTION_FWD_M2M = {
        .distanceInMm = +(CELLWIDTH),
        .angleInDeg = 0
    },
    MOTION_FWD_M2T90 = {
        .distanceInMm = 20,
        .angleInDeg = 0
    },
    MOTION_FWD_M2C = {
        .distanceInMm = +(CELLHALF),
        .angleInDeg = 0
    },
    MOTION_LS90_1 = {
        .distanceInMm = FIRSTHALF(110),
        .angleInDeg = FIRSTHALF(90)
    },
    MOTION_LS90_2 = {
        .distanceInMm = SECONDHALF(110),
        .angleInDeg = SECONDHALF(90)
    },
    MOTION_RS90_1 = {
        .distanceInMm = FIRSTHALF(110),
        .angleInDeg = FIRSTHALF(-90)
    },
    MOTION_RS90_2 = {
        .distanceInMm = SECONDHALF(110),
        .angleInDeg = SECONDHALF(-90)
    },
    MOTION_LP90_1 = {
        .distanceInMm = 0,
        .angleInDeg = FIRSTHALF(90)
    },
    MOTION_LP90_2 = {
        .distanceInMm = 0,
        .angleInDeg = SECONDHALF(90)
    },
    MOTION_RP90_1 = {
        .distanceInMm = 0,
        .angleInDeg = FIRSTHALF(-90)
    },
    MOTION_RP90_2 = {
        .distanceInMm = 0,
        .angleInDeg = SECONDHALF(-90)
    },
    MOTION_LP180_1 = {
        .distanceInMm = 0,
        .angleInDeg = FIRSTHALF(180)
    },
    MOTION_LP180_2 = {
        .distanceInMm = 0,
        .angleInDeg = SECONDHALF(180)
    },
    MOTION_RP180_1 = {
        .distanceInMm = 0,
        .angleInDeg = FIRSTHALF(-180)
    },
    MOTION_RP180_2 = {
        .distanceInMm = 0,
        .angleInDeg = SECONDHALF(-180)
    },
    // Used only in speed run
    MOTION_LS180_1 = {
        .distanceInMm = FIRSTHALF(283),
        .angleInDeg = FIRSTHALF(180)
    },
    MOTION_LS180_2 = {
        .distanceInMm = SECONDHALF(283),
        .angleInDeg = SECONDHALF(180)
    },
    MOTION_RS180_1 = {
        .distanceInMm = FIRSTHALF(283),
        .angleInDeg = FIRSTHALF(-180)
    },
    MOTION_RS180_2 = {
        .distanceInMm = SECONDHALF(283),
        .angleInDeg = SECONDHALF(-180)
    },
    MOTION_FWD_C245 = {
        .distanceInMm = 26,
        .angleInDeg = 0
    },
    MOTION_FWD_C2135 = {
        .distanceInMm = 79,
        .angleInDeg = 0
    },
    MOTION_LS45_1 = {
        .distanceInMm = FIRSTHALF(121),
        .angleInDeg = FIRSTHALF(45)
    },
    MOTION_LS45_2 = {
        .distanceInMm = SECONDHALF(121),
        .angleInDeg = SECONDHALF(45)
    },
    MOTION_RS45_1 = {
        .distanceInMm = FIRSTHALF(121),
        .angleInDeg = FIRSTHALF(-45)
    },
    MOTION_RS45_2 = {
        .distanceInMm = SECONDHALF(121),
        .angleInDeg = SECONDHALF(-45)
    },
    MOTION_LS135_1 = {
        .distanceInMm = FIRSTHALF(186),
        .angleInDeg = FIRSTHALF(135)
    },
    MOTION_LS135_2 = {
        .distanceInMm = SECONDHALF(186),
        .angleInDeg = SECONDHALF(135)
    },
    MOTION_RS135_1 = {
        .distanceInMm = FIRSTHALF(186),
        .angleInDeg = FIRSTHALF(-135)
    },
    MOTION_RS135_2 = {
        .distanceInMm = SECONDHALF(186),
        .angleInDeg = SECONDHALF(-135)
    },
    MOTION_DFWD = {
        .distanceInMm = 127,
        .angleInDeg = 0
    },
    MOTION_DFWD_D2D = {
        .distanceInMm = 101,
        .angleInDeg = 0
    },
    MOTION_LS90_D2D_1 = {
        .distanceInMm = FIRSTHALF(141),
        .angleInDeg = FIRSTHALF(90)
    },
    MOTION_LS90_D2D_2 = {
        .distanceInMm = SECONDHALF(141),
        .angleInDeg = SECONDHALF(90)
    },
    MOTION_RS90_D2D_1 = {
        .distanceInMm = FIRSTHALF(141),
        .angleInDeg = FIRSTHALF(-90)
    },
    MOTION_RS90_D2D_2 = {
        .distanceInMm = SECONDHALF(141),
        .angleInDeg = SECONDHALF(-90)
    }
;

static struct MotionConfig config = {.aTrans = 4000, .aRot = 5000};
static struct Profile vTransProfile, vRotProfile;
static bool ongoing = false;

void Motion_Configure(const struct MotionConfig *newConfig)
{
    if (newConfig->aTrans > 0 && newConfig->aRot > 0)
        config = *newConfig;
}

void Motion_Start(const struct Motion *motion, int32_t endVTrans, int32_t endVRot)
{
    struct ProfileParams vTransParams = {
        .square = motion->distanceInMm,
        .vStart = vTransProfile.vEnd,
        .vEnd = endVTrans * !!motion->distanceInMm,
        .accel = config.aTrans
    };

    struct ProfileParams vRotParams = {
        .square = motion->angleInDeg,
        .vStart = vRotProfile.vEnd,
        .vEnd = endVRot * !!motion->angleInDeg,
        .accel = config.aRot
    };

    int32_t t = Millis_Get();

    if (motion->distanceInMm != 0) {
        Profile_Setup(&vTransProfile, &vTransParams, t);

        if (motion->angleInDeg != 0) {
            Profile_Setup(&vRotProfile, &vRotParams, t);

            if (vTransProfile.t2 - vTransProfile.t0 >= vRotProfile.t2 - vRotProfile.t0) {
                Profile_SyncByTotalTime(&vRotProfile, &vRotParams, &vTransProfile);
                //~ printf("R s:%ld vs:%ld ve:%ld a1:%ld a2:%ld\n", vRotParams.square, vRotProfile.vStart, vRotProfile.vEnd, vRotProfile.a1,  vRotProfile.a2);
            }
            else {
                Profile_SyncByTotalTime(&vTransProfile, &vTransParams, &vRotProfile);
                //~ printf("T s:%ld vs:%ld ve:%ld a1:%ld a2:%ld\n", vTransParams.square, vTransProfile.vStart, vTransProfile.vEnd, vTransProfile.a1,  vTransProfile.a2);
            }
        }
    }
    else if (motion->angleInDeg != 0) {
        Profile_Setup(&vRotProfile, &vRotParams, t);
    }

    ongoing = true;
}

bool Motion_IsOngoing(void)
{
    return ongoing;
}

void Motion_Update(void)
{
    if (!ongoing)
        return;

    uint32_t t = Millis_Get();

    int32_t vTrans = Profile_GetValue(&vTransProfile, t);
    int32_t vRot = Profile_GetValue(&vRotProfile, t);

    SpeedCtl_SetTarget(vTrans, vRot);

    if (Profile_GetState(&vTransProfile, t) == ProfileState_FINISHED
            && Profile_GetState(&vRotProfile, t) == ProfileState_FINISHED)
        ongoing = false;
}
