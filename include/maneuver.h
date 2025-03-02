#ifndef _INC_MANEUVER_H
#define _INC_MANEUVER_H

#include "router.h"

typedef enum {
    ManeuverStatus_COMPLETED,
    ManeuverStatus_FAILED
} ManeuverStatus;

enum Maneuver {
    Maneuver_NONE,
    Maneuver_BACKTRIM,
    Maneuver_FORWARD,
    Maneuver_SMOOTHLEFT,
    Maneuver_SMOOTHRIGHT,
    Maneuver_TURN_BACK,
    Maneuver_STOP
};

void Maneuver_PrepareToRun(RouterRunType runType);
ManeuverStatus Maneuver_Perform(enum Maneuver maneuver);
void Maneuver_SetDistanceError(int32_t distanceInMm);
int32_t Maneuver_GetDistanceError(void);

#endif // _INC_MANEUVER_H