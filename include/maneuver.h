#ifndef _INC_MANEUVER_H
#define _INC_MANEUVER_H

#include "router.h"

typedef enum {
    ManeuverStatus_COMPLETED,
    ManeuverStatus_FAILED
} ManeuverStatus;

#define MANEUVERKIND_SHIFT  4

enum ManeuverKind {
    ManeuverKind_BACKWARD       = 1 << (MANEUVERKIND_SHIFT + 0),
    ManeuverKind_FORWARD        = 1 << (MANEUVERKIND_SHIFT + 1),
    ManeuverKind_SMOOTHTURN     = 1 << (MANEUVERKIND_SHIFT + 2),
    ManeuverKind_PIVOTTURN      = 1 << (MANEUVERKIND_SHIFT + 3),
    ManeuverKind_STOP           = 1 << (MANEUVERKIND_SHIFT + 4)
};

enum Maneuver {
    Maneuver_NONE = 0,

    Maneuver_BACKTRIM           = ManeuverKind_BACKWARD | 0,
    Maneuver_BACKTRIM_RUSH      = ManeuverKind_BACKWARD | 1,

    Maneuver_FORWARD            = ManeuverKind_FORWARD | 0,
    Maneuver_FORWARD_SLOWDOWN   = ManeuverKind_FORWARD | 1,
    Maneuver_FORWARD_SLOW       = ManeuverKind_FORWARD | 2,
    Maneuver_FORWARD_SPEEDUP    = ManeuverKind_FORWARD | 3,

    Maneuver_SMOOTHLEFT         = ManeuverKind_SMOOTHTURN | 0,
    Maneuver_SMOOTHRIGHT        = ManeuverKind_SMOOTHTURN | 1,
    Maneuver_SMOOTHLEFT_LONG    = ManeuverKind_SMOOTHTURN | 2,
    Maneuver_SMOOTHRIGHT_LONG   = ManeuverKind_SMOOTHTURN | 3,

    Maneuver_TURN_BACK          = ManeuverKind_PIVOTTURN | 0,

    Maneuver_STOP               = ManeuverKind_STOP | 0,
    Maneuver_STOP_RUSH          = ManeuverKind_STOP | 1
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