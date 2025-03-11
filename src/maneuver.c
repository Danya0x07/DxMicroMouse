#include "maneuver.h"
#include "motion.h"
#include "speedctl.h"
#include "buzzer.h"

static int32_t distanceError;
static bool needToAbort;

static void ApplyCorrection(void)
{
    Motion_SetCorrection((struct MotionCorrection){distanceError});
    distanceError = 0;
}

static void ResetCorrection(void)
{
    Motion_SetCorrection((struct MotionCorrection){0});
    distanceError = 0;
}

static void _DoNothing(uint32_t idx)
{
    (void)idx;  // (-_-)
}

static void _CheckCompletionStatus(ManeuverStatus status)
{
    if (status != ManeuverStatus_COMPLETED) {
        SpeedCtl_SetState(DISABLE);
        Buzzer_Blink(6, 800, 80);
    }
}

static void _Backtrim_OnNextMotion(uint32_t idx)
{
    if (idx == 0) {
        SpeedCtl_Reset();
        SpeedCtl_SetMode(SpeedCtlMode_BACKTRIM);
    }
    else if (idx == 1) {
        Router_UpdateWalls();
        //SpeedCtl_Reset();
        SpeedCtl_SetMode(SpeedCtlMode_STRAIGHT);
    }
}

static void _CorrectDistance(uint32_t idx)
{
    Buzzer_BeepAsync(4000, 20);
    ApplyCorrection();
}

static void _SmoothTurn_OnNextMotion(uint32_t idx)
{
    if (idx == 0) {
        Buzzer_BeepAsync(4000, 20);
        ApplyCorrection();
    }
    else if (idx == 1) {
        SpeedCtl_SetMode(SpeedCtlMode_TURN);
    }
    else if (idx == 2) {
        SpeedCtl_SetMode(SpeedCtlMode_STRAIGHT);
    }
}

static void _SmoothTurn_OnComplete(ManeuverStatus status)
{
    _CheckCompletionStatus(status);
}

static void _SmoothTurnLong_OnNextMotion(uint32_t idx)
{
    ResetCorrection();
    SpeedCtl_SetMode(SpeedCtlMode_TURN);
}

static void _SmoothTurnLong_OnComplete(ManeuverStatus status)
{
    _CheckCompletionStatus(status);
    SpeedCtl_SetMode(SpeedCtlMode_STRAIGHT);
}

static void _TurnBack_OnNextMotion(uint32_t idx)
{
    if (idx == 0) {
        ApplyCorrection();
    }
    else if (idx == 1) {
        SpeedCtl_SetMode(SpeedCtlMode_TURN);
        Buzzer_BeepAsync(1500, 30);
    }
    else if (idx == 2) {
        SpeedCtl_SetMode(SpeedCtlMode_BACKTRIM);
    }
    else if (idx == 3) {
        SpeedCtl_SetMode(SpeedCtlMode_STRAIGHT);
    }
}

static void _Stop_OnNextMotion(uint32_t idx)
{
    if (idx == 0) {
        Buzzer_BeepAsync(2600, 30);
        ApplyCorrection();
    }
    else if (idx == 1) {
        //SpeedCtl_Reset();
        SpeedCtl_SetMode(SpeedCtlMode_TURN);
    }
}

static void _Stop_OnComplete(ManeuverStatus status)
{
    SpeedCtl_Reset();
    _CheckCompletionStatus(status);
}

static const struct ManeuverCtlBlock {
    const enum Motion *motions;
    uint32_t numMotions;
    void (*onNextMotion)(uint32_t idx);
    void (*loop)(uint32_t idx);
    void (*onComplete)(ManeuverStatus status);
} maneuvers[] = {
    [Maneuver_NONE] = {
        .numMotions = 0
    },
    [Maneuver_BACKTRIM] = {
        .motions = (const enum Motion []){Motion_PARK_BACK2WALL, Motion_PARK_FWD2DP},
        .numMotions = 2,
        .onNextMotion = _Backtrim_OnNextMotion,
        .loop = _DoNothing,
        .onComplete = _CheckCompletionStatus
    },
    [Maneuver_BACKTRIM_RUSH] = {
        .motions = (const enum Motion []){Motion_PARK_BACK2WALL, Motion_PARK_FWD2DP_ACC2SLOW},
        .numMotions = 2,
        .onNextMotion = _Backtrim_OnNextMotion,
        .loop = _DoNothing,
        .onComplete = _CheckCompletionStatus
    },
    [Maneuver_FORWARD] = {
        .motions = (const enum Motion []){Motion_FWD_DP2DP},
        .numMotions = 1,
        .onNextMotion = _CorrectDistance,
        .loop = _DoNothing,
        .onComplete = _CheckCompletionStatus
    },
    [Maneuver_FORWARD_SLOWDOWN] = {
        .motions = (const enum Motion []){Motion_FWD_DP2DP_DECC},
        .numMotions = 1,
        .onNextMotion = _CorrectDistance,
        .loop = _DoNothing,
        .onComplete = _CheckCompletionStatus
    },
    [Maneuver_FORWARD_SLOW] = {
        .motions = (const enum Motion []){Motion_FWD_DP2DP_SLOW},
        .numMotions = 1,
        .onNextMotion = _CorrectDistance,
        .loop = _DoNothing,
        .onComplete = _CheckCompletionStatus
    },
    [Maneuver_FORWARD_SPEEDUP] = {
        .motions = (const enum Motion []){Motion_FWD_DP2DP_ACC},
        .numMotions = 1,
        .onNextMotion = _CorrectDistance,
        .loop = _DoNothing,
        .onComplete = _CheckCompletionStatus
    },
    [Maneuver_SMOOTHLEFT] = {
        .motions = (const enum Motion []){Motion_FWD_DP2T, Motion_SMOOTH_LEFT90, Motion_FWD_T2DP},
        .numMotions = 3,
        .onNextMotion = _SmoothTurn_OnNextMotion,
        .loop = _DoNothing, // TODO: Implement crash detection
        .onComplete = _SmoothTurn_OnComplete
    },
    [Maneuver_SMOOTHRIGHT] = {
        .motions = (const enum Motion []){Motion_FWD_DP2T, Motion_SMOOTH_RIGHT90, Motion_FWD_T2DP},
        .numMotions = 3,
        .onNextMotion = _SmoothTurn_OnNextMotion,
        .loop = _DoNothing,
        .onComplete = _SmoothTurn_OnComplete
    },
    [Maneuver_SMOOTHLEFT_LONG] = {
        .motions = (const enum Motion []){Motion_SMOOTH_LEFT90_LONG},
        .numMotions = 1,
        .onNextMotion = _SmoothTurnLong_OnNextMotion,
        .loop = _DoNothing, // TODO: Implement crash detection
        .onComplete = _SmoothTurnLong_OnComplete
    },
    [Maneuver_SMOOTHRIGHT_LONG] = {
        .motions = (const enum Motion []){Motion_SMOOTH_RIGHT90_LONG},
        .numMotions = 1,
        .onNextMotion = _SmoothTurnLong_OnNextMotion,
        .loop = _DoNothing,
        .onComplete = _SmoothTurnLong_OnComplete
    },
    [Maneuver_TURN_BACK] = {
        .motions = (const enum Motion []) {
                Motion_FWD_DP2C, Motion_PIVOT_LEFT180, Motion_PARK_BACK2WALL, Motion_PARK_FWD2DP},
        .numMotions = 4,
        .onNextMotion = _TurnBack_OnNextMotion,
        .loop = _DoNothing,
        .onComplete = _CheckCompletionStatus
    },
    [Maneuver_STOP] = {
        .motions = (const enum Motion []){Motion_FWD_DP2C, Motion_PIVOT_RIGHT180},
        .numMotions = 2,
        .onNextMotion = _Stop_OnNextMotion,
        .loop = _DoNothing,
        .onComplete = _Stop_OnComplete
    },
    [Maneuver_STOP_RUSH] = {
        .motions = (const enum Motion []){Motion_FWD_DP2C_FROMSLOW, Motion_PIVOT_RIGHT180},
        .numMotions = 2,
        .onNextMotion = _Stop_OnNextMotion,
        .loop = _DoNothing,
        .onComplete = _Stop_OnComplete
    }
};

void Maneuver_PrepareToRun(RouterRunType runType)
{
    distanceError = 0;
    needToAbort = false;

    Motion_SetMode(runType == RouterRunType_RUSH ? MotionMode_FAST : MotionMode_SLOW);
}

ManeuverStatus Maneuver_Perform(enum Maneuver maneuver)
{
    ManeuverStatus status = ManeuverStatus_COMPLETED;
    const struct ManeuverCtlBlock *m = &maneuvers[maneuver];
    enum Motion motion;

    for (uint32_t motionIdx = 0; motionIdx < m->numMotions; motionIdx++) {
        motion = m->motions[motionIdx];
        m->onNextMotion(motionIdx);
        Motion_Start(motion);
        while (Motion_IsOngoing()) {
            m->loop(motionIdx);

            if (needToAbort) {
                status = ManeuverStatus_FAILED;
                needToAbort = false;
                goto abort;
            }
        }
    }

abort:
    m->onComplete(status);
    return status;
}

void Maneuver_SetDistanceError(int32_t distanceInMm)
{
    distanceError = distanceInMm;
}

int32_t Maneuver_GetDistanceError(void)
{
    return distanceError;
}

void Maneuver_Abort(void)
{
    needToAbort = true;
}
