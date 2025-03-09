#ifndef _INC_MANEUVER_H
#define _INC_MANEUVER_H

#include "router.h"

typedef enum {
    ManeuverStatus_COMPLETED,
    ManeuverStatus_FAILED
} ManeuverStatus;

#define MANEUVERKIND_SHIFT  4

enum ManeuverKind {
    ManeuverKind_BACKWARD = MANEUVERKIND_SHIFT,
    ManeuverKind_FORWARD,
    ManeuverKind_SMOOTHTURN,
    ManeuverKind_PIVOTTURN,
    ManeuverKind_STOP
};

enum Maneuver {
    Maneuver_NONE = 0,

    Maneuver_BACKTRIM           = 1 << ManeuverKind_BACKWARD | 0,
    Maneuver_BACKTRIM_RUSH      = 1 << ManeuverKind_BACKWARD | 1,

    Maneuver_FORWARD            = 1 << ManeuverKind_FORWARD | 0,
    Maneuver_FORWARD_SLOWDOWN   = 1 << ManeuverKind_FORWARD | 1,
    Maneuver_FORWARD_SLOW       = 1 << ManeuverKind_FORWARD | 2,
    Maneuver_FORWARD_SPEEDUP    = 1 << ManeuverKind_FORWARD | 3,

    Maneuver_SMOOTHLEFT         = 1 << ManeuverKind_SMOOTHTURN | 0,
    Maneuver_SMOOTHRIGHT        = 1 << ManeuverKind_SMOOTHTURN | 1,
    Maneuver_SMOOTHLEFT_LONG    = 1 << ManeuverKind_SMOOTHTURN | 2,
    Maneuver_SMOOTHRIGHT_LONG   = 1 << ManeuverKind_SMOOTHTURN | 3,

    Maneuver_TURN_BACK          = 1 << ManeuverKind_PIVOTTURN | 0,

    Maneuver_STOP               = 1 << ManeuverKind_STOP | 0,
    Maneuver_STOP_RUSH          = 1 << ManeuverKind_STOP | 1
};

void Maneuver_PrepareToRun(RouterRunType runType);
ManeuverStatus Maneuver_Perform(enum Maneuver maneuver);
void Maneuver_SetDistanceError(int32_t distanceInMm);
int32_t Maneuver_GetDistanceError(void);
void Maneuver_Abort(void);

static inline bool Maneuver_OfKind(enum Maneuver maneuver, enum ManeuverKind kind)
{
    return !!(maneuver & kind);
}

#endif // _INC_MANEUVER_H