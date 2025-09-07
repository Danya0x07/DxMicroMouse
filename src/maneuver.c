#include "maneuver.h"
#include "motion.h"
#include "speedctl.h"
#include "buzzer.h"
#include "odometry.h"
#include "utils.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ========== Compile-time params ========== */
#define MAXIMUM_ALLOWED_CORRECTION  20
#define DISTANCE_ERROR_THRESHOLD    3

/* ========== Module variables ========== */
static ManeuverMode maneuverMode = ManeuverMode_SEARCH;
static bool needToAbort;
static int distanceError;
static struct Motion MOTION_Dash[2];
static int distanceToDash;
static void (*disposableBacktrimCallback)(void);

/* ========== NVM configuration ========== */
static struct ManeuverConfig {
    struct MotionConfig motionConfig;
    int32_t vTransTurn;
    int32_t vTransDash;
    int32_t vRot;
} configs[2] = {
    [ManeuverMode_SEARCH] = {
        .motionConfig = {
            .aTrans = 3000,
            .aRot = 6000
        },
        .vTransTurn = 360,
        .vTransDash = 400,
        .vRot = 720
    },
    [ManeuverMode_FAST] = {
        .motionConfig = {
            .aTrans = 4000,
            .aRot = 8000
        },
        .vTransTurn = 400,
        .vTransDash = 700,
        .vRot = 1080
    }
};

/* ========== Maneuver control structures =========== */
typedef struct { int vTrans, vRot; } Speeds;

struct ManeuverCtlBlock {
    const struct Motion **motions;
    unsigned numMotions;
    Speeds (*onNextMotion)(unsigned idx, struct Motion *m, bool keepSpeed);
    void (*loop)(unsigned idx);
    void (*onComplete)(ManeuverStatus status);
};

/* ========== Generic callbacks ========== */
static void _Generic_Loop(unsigned idx);
static void _Generic_OnComplete(ManeuverStatus status);

/* ========== Maneuver-specific OnNextMotion callbacks ========== */
static Speeds _BTR_OnNextMotion(unsigned idx, struct Motion *m, bool keepSpeed);
static Speeds _FWD_OnNextMotion(unsigned idx, struct Motion *m, bool keepSpeed);
//~ static Speeds _TP90_OnNextMotion(unsigned idx, struct Motion *m, bool keepSpeed);
static Speeds _TS90_OnNextMotion(unsigned idx, struct Motion *m, bool keepSpeed);
static Speeds _TS180_OnNextMotion(unsigned idx, struct Motion *m, bool keepSpeed);
static Speeds _TBACK_OnNextMotion(unsigned idx, struct Motion *m, bool keepSpeed);
//~ static Speeds _SD_OnNextMotion(unsigned idx, struct Motion *m, bool keepSpeed);
static Speeds _FD45_OnNextMotion(unsigned idx, struct Motion *m, bool keepSpeed);
static Speeds _FD135_OnNextMotion(unsigned idx, struct Motion *m, bool keepSpeed);
static Speeds _D2D_OnNextMotion(unsigned idx, struct Motion *m, bool keepSpeed);
static Speeds _DASH_OnNextMotion(unsigned idx, struct Motion *m, bool keepSpeed);

#define _TP90_OnNextMotion  _TBACK_OnNextMotion
#define _SD_OnNextMotion    _TS90_OnNextMotion

/* ========== Maneuver-specific loop callbacks ========== */
#define _BTR_Loop   _Generic_Loop
#define _FWD_Loop   _Generic_Loop
#define _TP90_Loop  _Generic_Loop
#define _TS90_Loop  _Generic_Loop
#define _TS180_Loop _Generic_Loop
#define _TBACK_Loop  _Generic_Loop
#define _SD_Loop    _Generic_Loop
#define _FD_Loop    _Generic_Loop
#define _D2D_Loop   _Generic_Loop
#define _DASH_Loop  _Generic_Loop

/* ========== Maneuver-specific OnComplete callbacks ========== */
//~ static void _BTR_OnComplete(ManeuverStatus status);
static void _FWD_OnComplete(ManeuverStatus status);
static void _HFWD_OnComplete(ManeuverStatus status);
static void _DFWD_OnComplete(ManeuverStatus status);
//~ static void _TS90_OnComplete(ManeuverStatus status);
static void _TS180_OnComplete(ManeuverStatus status);
static void _TBACK_OnComplete(ManeuverStatus status);
static void _SD_OnComplete(ManeuverStatus status);
//~ static void _FD_OnComplete(ManeuverStatus status);
//~ static void _D2D_OnComplete(ManeuverStatus status);
//~ static void _DASH_OnComplete(ManeuverStatus status);

#define _BTR_OnComplete     _Generic_OnComplete
#define _TP90_OnComplete    _Generic_OnComplete
#define _TS90_OnComplete    _Generic_OnComplete
#define _FD_OnComplete      _Generic_OnComplete
#define _D2D_OnComplete     _Generic_OnComplete
#define _DASH_OnComplete    _Generic_OnComplete

/* ========== Maneuvers definitions ========== */
static const struct ManeuverCtlBlock maneuvers[] = {
    [Maneuver_NONE] = {
        .numMotions = 0
    },
    [Maneuver_BTR2M] = {
        .motions = (const struct Motion *[]) {
            &MOTION_BACK_PARK_1, &MOTION_BACK_PARK_2,
            &MOTION_FWD_UNPARK2C, &MOTION_FWD_UNPARK2C},
        .numMotions = 4,
        .onNextMotion = _BTR_OnNextMotion,
        .loop = _BTR_Loop,
        .onComplete = _BTR_OnComplete
    },
    [Maneuver_BTR2C] = {
        .motions = (const struct Motion *[]){&MOTION_BACK_PARK_1, &MOTION_BACK_PARK_2, &MOTION_FWD_UNPARK2C},
        .numMotions = 3,
        .onNextMotion = _BTR_OnNextMotion,
        .loop = _BTR_Loop,
        .onComplete = _BTR_OnComplete
    },
    [Maneuver_HFWD] = {
        .motions = (const struct Motion *[]){&MOTION_FWD_M2C},
        .numMotions = 1,
        .onNextMotion = _FWD_OnNextMotion,
        .loop = _FWD_Loop,
        .onComplete = _HFWD_OnComplete
    },
    [Maneuver_FWD] = {
        .motions = (const struct Motion *[]){&MOTION_FWD_M2M},
        .numMotions = 1,
        .onNextMotion = _FWD_OnNextMotion,
        .loop = _FWD_Loop,
        .onComplete = _FWD_OnComplete
    },
    [Maneuver_DFWD] = {
        .motions = (const struct Motion *[]){&MOTION_DFWD},
        .numMotions = 1,
        .onNextMotion = _FWD_OnNextMotion,
        .loop = _FWD_Loop,
        .onComplete = _DFWD_OnComplete
    },
    [Maneuver_LP90] = {
        .motions = (const struct Motion *[]){&MOTION_LP90_1, &MOTION_LP90_2},
        .numMotions = 2,
        .onNextMotion = _TP90_OnNextMotion,
        .loop = _TP90_Loop,
        .onComplete = _TP90_OnComplete
    },
    [Maneuver_RP90] = {
        .motions = (const struct Motion *[]){&MOTION_RP90_1, &MOTION_RP90_2},
        .numMotions = 2,
        .onNextMotion = _TP90_OnNextMotion,
        .loop = _TP90_Loop,
        .onComplete = _TP90_OnComplete
    },
    [Maneuver_LS90] = {
        .motions = (const struct Motion *[]){&MOTION_FWD_M2T90, &MOTION_LS90_1, &MOTION_LS90_2, &MOTION_FWD_M2T90},
        .numMotions = 4,
        .onNextMotion = _TS90_OnNextMotion,
        .loop = _TS90_Loop,
        .onComplete = _TS90_OnComplete
    },
    [Maneuver_RS90] = {
        .motions = (const struct Motion *[]){&MOTION_FWD_M2T90, &MOTION_RS90_1, &MOTION_RS90_2, &MOTION_FWD_M2T90},
        .numMotions = 4,
        .onNextMotion = _TS90_OnNextMotion,
        .loop = _TS90_Loop,
        .onComplete = _TS90_OnComplete
    },
    [Maneuver_LS180] = {
        .motions = (const struct Motion *[]){&MOTION_LS180_1, &MOTION_LS180_2},
        .numMotions = 2,
        .onNextMotion = _TS180_OnNextMotion,
        .loop = _TS180_Loop,
        .onComplete = _TS180_OnComplete
    },
    [Maneuver_RS180] = {
        .motions = (const struct Motion *[]){&MOTION_RS180_1, &MOTION_RS180_2},
        .numMotions = 2,
        .onNextMotion = _TS180_OnNextMotion,
        .loop = _TS180_Loop,
        .onComplete = _TS180_OnComplete
    },
    [Maneuver_TBACK] = {
        .motions = (const struct Motion *[]){&MOTION_RP180_1, &MOTION_RP180_2},
        .numMotions = 2,
        .onNextMotion = _TBACK_OnNextMotion,
        .loop = _TBACK_Loop,
        .onComplete = _TBACK_OnComplete
    },
    [Maneuver_SDL45] = {
        .motions = (const struct Motion *[]){&MOTION_FWD_C245, &MOTION_LS45_1, &MOTION_LS45_2},
        .numMotions = 3,
        .onNextMotion = _SD_OnNextMotion,
        .loop = _SD_Loop,
        .onComplete = _SD_OnComplete
    },
    [Maneuver_SDR45] = {
        .motions = (const struct Motion *[]){&MOTION_FWD_C245, &MOTION_RS45_1, &MOTION_RS45_2},
        .numMotions = 3,
        .onNextMotion = _SD_OnNextMotion,
        .loop = _SD_Loop,
        .onComplete = _SD_OnComplete
    },
    [Maneuver_FDL45] = {
        .motions = (const struct Motion *[]){&MOTION_LS45_1, &MOTION_LS45_2, &MOTION_FWD_C245},
        .numMotions = 3,
        .onNextMotion = _FD45_OnNextMotion,
        .loop = _FD_Loop,
        .onComplete = _FD_OnComplete
    },
    [Maneuver_FDR45] = {
        .motions = (const struct Motion *[]){&MOTION_RS45_1, &MOTION_RS45_2, &MOTION_FWD_C245},
        .numMotions = 3,
        .onNextMotion = _FD45_OnNextMotion,
        .loop = _FD_Loop,
        .onComplete = _FD_OnComplete
    },
    [Maneuver_SDL135] = {
        .motions = (const struct Motion *[]){&MOTION_FWD_C2135, &MOTION_LS135_1, &MOTION_LS135_2},
        .numMotions = 3,
        .onNextMotion = _SD_OnNextMotion,
        .loop = _SD_Loop,
        .onComplete = _SD_OnComplete
    },
    [Maneuver_SDR135] = {
        .motions = (const struct Motion *[]){&MOTION_FWD_C2135, &MOTION_RS135_1, &MOTION_RS135_2},
        .numMotions = 3,
        .onNextMotion = _SD_OnNextMotion,
        .loop = _SD_Loop,
        .onComplete = _SD_OnComplete
    },
    [Maneuver_FDL135] = {
        .motions = (const struct Motion *[]){&MOTION_LS135_1, &MOTION_LS135_2, &MOTION_FWD_C2135},
        .numMotions = 3,
        .onNextMotion = _FD135_OnNextMotion,
        .loop = _FD_Loop,
        .onComplete = _FD_OnComplete
    },
    [Maneuver_FDR135] = {
        .motions = (const struct Motion *[]){&MOTION_RS135_1, &MOTION_RS135_2, &MOTION_FWD_C2135},
        .numMotions = 3,
        .onNextMotion = _FD135_OnNextMotion,
        .loop = _FD_Loop,
        .onComplete = _FD_OnComplete
    },
    [Maneuver_D2DL] = {
        .motions = (const struct Motion *[]) {
            &MOTION_DFWD_D2D, &MOTION_LS90_D2D_1, &MOTION_LS90_D2D_2, &MOTION_DFWD_D2D
        },
        .numMotions = 4,
        .onNextMotion = _D2D_OnNextMotion,
        .loop = _D2D_Loop,
        .onComplete = _D2D_OnComplete
    },
    [Maneuver_D2DR] = {
        .motions = (const struct Motion *[]) {
            &MOTION_DFWD_D2D, &MOTION_RS90_D2D_1, &MOTION_RS90_D2D_2, &MOTION_DFWD_D2D
        },
        .numMotions = 4,
        .onNextMotion = _D2D_OnNextMotion,
        .loop = _D2D_Loop,
        .onComplete = _D2D_OnComplete
    },
    [Maneuver_DASH] = {
        .motions = (const struct Motion *[]){&MOTION_Dash[0], &MOTION_Dash[1]},
        .numMotions = 2,
        .onNextMotion = _DASH_OnNextMotion,
        .loop = _DASH_Loop,
        .onComplete = _DASH_OnComplete
    }
};

const char *const MANEUVERS_STR[] = {
    [Maneuver_NONE] = "NONE",
    [Maneuver_BTR2M] = "BTR2M",
    [Maneuver_BTR2C] = "BTR2C",
    [Maneuver_HFWD] = "HFWD",
    [Maneuver_FWD] = "FWD",
    [Maneuver_DFWD] = "DFWD",
    [Maneuver_LP90] = "LP90",
    [Maneuver_RP90] = "RP90",
    [Maneuver_LS90] = "LS90",
    [Maneuver_RS90] = "RS90",
    [Maneuver_LS180] = "LS180",
    [Maneuver_RS180] = "RS180",
    [Maneuver_TBACK] = "TBACK",
    [Maneuver_SDL45] = "SDL45",
    [Maneuver_SDR45] = "SDR45",
    [Maneuver_FDL45] = "FDL45",
    [Maneuver_FDR45] = "FDR45",
    [Maneuver_SDL135] = "SDL135",
    [Maneuver_SDR135] = "SDR135",
    [Maneuver_FDL135] = "FDL135",
    [Maneuver_FDR135] = "FDR135",
    [Maneuver_D2DL] = "D2DL",
    [Maneuver_D2DR] = "D2DR",
    [Maneuver_DASH] = "DASH"
};

/* ========== Public functions ========== */

void Maneuver_SetMode(ManeuverMode mode)
{
    maneuverMode = mode;
    distanceError = 0;
    needToAbort = false;

    Motion_Configure(&configs[maneuverMode].motionConfig);
}

void Maneuver_SetupDash(int distance)
{
    distanceToDash += distance;
}

void Maneuver_BindDisposableBacktrimCallback(void (*callback)(void))
{
    disposableBacktrimCallback = callback;
}

ManeuverStatus Maneuver_Perform(enum Maneuver maneuver, bool keepSpeed)
{
    ManeuverStatus status = ManeuverStatus_COMPLETED;
    const struct ManeuverCtlBlock *mcb = &maneuvers[maneuver];

    for (unsigned motionIdx = 0; motionIdx < mcb->numMotions; motionIdx++) {
        struct Motion motion = *mcb->motions[motionIdx];
        Speeds endSpeeds = mcb->onNextMotion(motionIdx, &motion, keepSpeed);

        Motion_Start(&motion, endSpeeds.vTrans, endSpeeds.vRot);
        while (Motion_IsOngoing()) {
            mcb->loop(motionIdx);

            if (needToAbort) {
                status = ManeuverStatus_FAILED;
                needToAbort = false;
                goto abort;
            }
        }
    }

abort:
    mcb->onComplete(status);
    return status;
}

void Maneuver_Abort(void)
{
    needToAbort = true;
}

/* ========== Utility functions ========== */

static void UpdateDistanceError(int predictionDelta)
{
    int predictedDistance, predictedAngle, distance, angle;

    Odometry_UpdatePrediction(predictionDelta, 0);
    Odometry_GetPrediction(&predictedDistance, &predictedAngle);
    Odometry_GetFusion(&distance, &angle);

    int error = predictedDistance - distance;
    if (error >= DISTANCE_ERROR_THRESHOLD || error <= -DISTANCE_ERROR_THRESHOLD) {
        distanceError += error;
    }
}

static void ApplyDistanceCorrection(struct Motion *m)
{
    int correction = distanceError;
    int addableDistance = MAXIMUM_ALLOWED_CORRECTION;
    int substractableDistance = min(m->distanceInMm, MAXIMUM_ALLOWED_CORRECTION);

    if (m->distanceInMm > 0) {
        if (correction > addableDistance)
            correction = addableDistance;
        else if (correction < -substractableDistance)
            correction = -substractableDistance;
    }
    else if (m->distanceInMm < 0) {
        if (correction < -addableDistance)
            correction = -addableDistance;
        else if (correction > substractableDistance)
            correction = substractableDistance;
    }
    m->distanceInMm += correction;
    distanceError -= 0;
}

static void ResetDistanceError(void)
{
    Odometry_Reset();
    distanceError = 0;
}

static Speeds ChooseDefaultSpeeds(bool keepTrans, bool keepRot)
{
    return (Speeds) {keepTrans ? configs[maneuverMode].vTransTurn : 0, keepRot ? configs[maneuverMode].vRot : 0};
}

static void CheckStatus(ManeuverStatus status)
{
    if (status == ManeuverStatus_COMPLETED) {
        Buzzer_BeepManeuverCompleted();
    }
    else {
        SpeedCtl_SetState(DISABLE);
        Buzzer_BlinkManeuverFailed();
    }
}

static Speeds CalcSpeedsForExitingDiagonal(unsigned idx, int distance0, int distance1, bool keepSpeed)
{
    int vTrans1 = SquareRootRounded(2 * configs[maneuverMode].motionConfig.aTrans * distance1);
    vTrans1 = min(vTrans1, configs[maneuverMode].vTransTurn);

    int vTrans0 = SquareRootRounded(
            2 * configs[maneuverMode].motionConfig.aTrans * distance0 + vTrans1 * vTrans1);
    vTrans0 = min(vTrans0, configs[maneuverMode].vTransTurn);

    switch (idx) {
        case 0:
            SpeedCtl_SetMode(SpeedCtlMode_TURN);
            return keepSpeed ? ChooseDefaultSpeeds(1, 1) : (Speeds) {vTrans0, configs[maneuverMode].vRot};
        case 1:
            return keepSpeed ? ChooseDefaultSpeeds(1, 0) : (Speeds) {vTrans1, 0};
        case 2:
        default:
            SpeedCtl_SetMode(SpeedCtlMode_STRAIGHT);
            return ChooseDefaultSpeeds(keepSpeed, 0);
    }
}

/* ========== General callbacks ========== */

static void _Generic_Loop(unsigned idx)
{
    (void)idx;  // (-_-)
}

static void _Generic_OnComplete(ManeuverStatus status)
{
    CheckStatus(status);
    ResetDistanceError();
}

/* ========== Maneuver-specific OnNextMotion callbacks ========== */

static Speeds _BTR_OnNextMotion(unsigned idx, struct Motion *m, bool keepSpeed)
{
    switch (idx) {
        case 0:
            SpeedCtl_Reset();
            SpeedCtl_SetMode(SpeedCtlMode_BACKTRIM);
            return (Speeds) {200, 0};
        case 1:
            return ChooseDefaultSpeeds(0, 0);
        case 2:
            Millis_Wait(100);
            SpeedCtl_Reset();
            Millis_Wait(100);
            if (disposableBacktrimCallback) {
                disposableBacktrimCallback();
                disposableBacktrimCallback = NULL;
            }
            SpeedCtl_SetMode(SpeedCtlMode_STRAIGHT);
            return ChooseDefaultSpeeds(1, 0);
        case 3:
        default:
            return ChooseDefaultSpeeds(keepSpeed, 0);
    }
}

static Speeds _FWD_OnNextMotion(unsigned idx, struct Motion *m, bool keepSpeed)
{
    ApplyDistanceCorrection(m);
    return ChooseDefaultSpeeds(keepSpeed, 0);
}

static Speeds _TS90_OnNextMotion(unsigned idx, struct Motion *m, bool keepSpeed)
{
    switch (idx) {
        case 0:
            //~ ApplyDistanceCorrection(m);
            return ChooseDefaultSpeeds(1, 0);
        case 1:
            SpeedCtl_SetMode(SpeedCtlMode_TURN);
            return ChooseDefaultSpeeds(1, 1);
        case 2:
            return ChooseDefaultSpeeds(1, 0);
        case 3:
        default:
            SpeedCtl_SetMode(SpeedCtlMode_STRAIGHT);
            return ChooseDefaultSpeeds(keepSpeed, 0);
    }
}

static Speeds _TS180_OnNextMotion(unsigned idx, struct Motion *m, bool keepSpeed)
{
    switch (idx) {
        case 0:
            SpeedCtl_SetMode(SpeedCtlMode_TURN);
            return ChooseDefaultSpeeds(1, 1);
        case 1:
        default:
            return ChooseDefaultSpeeds(keepSpeed, 0);
    }
}

static Speeds _TBACK_OnNextMotion(unsigned idx, struct Motion *m, bool keepSpeed)
{
    switch (idx) {
        case 0:
            SpeedCtl_SetMode(SpeedCtlMode_TURN);
            return ChooseDefaultSpeeds(0, 1);
        case 1:
        default:
            return ChooseDefaultSpeeds(0, 0);
    }
}

static Speeds _FD45_OnNextMotion(unsigned idx, struct Motion *m, bool keepSpeed)
{
    return CalcSpeedsForExitingDiagonal(idx, MOTION_LS45_2.distanceInMm, MOTION_FWD_C245.distanceInMm, keepSpeed);
}

static Speeds _FD135_OnNextMotion(unsigned idx, struct Motion *m, bool keepSpeed)
{
    return CalcSpeedsForExitingDiagonal(idx, MOTION_LS135_2.distanceInMm, MOTION_FWD_C2135.distanceInMm, keepSpeed);
}

static Speeds _D2D_OnNextMotion(unsigned idx, struct Motion *m, bool keepSpeed)
{
    switch (idx) {
        case 0:
            return ChooseDefaultSpeeds(1, 0);
        case 1:
            SpeedCtl_SetMode(SpeedCtlMode_TURN);
            return ChooseDefaultSpeeds(1, 1);
        case 2:
            return ChooseDefaultSpeeds(1, 0);
        case 3:
        default:
            SpeedCtl_SetMode(SpeedCtlMode_DIAGONAL);
            return ChooseDefaultSpeeds(keepSpeed, 0);
    }
}

static Speeds _DASH_OnNextMotion(unsigned idx, struct Motion *m, bool keepSpeed)
{
    int vTrans = configs[maneuverMode].vTransDash; // TODO: choose max speed based on distance

    switch (idx) {
        case 0:
            m->distanceInMm = distanceToDash / 2;
            break;
        case 1:
        default:
            vTrans = configs[maneuverMode].vTransTurn * keepSpeed;
            m->distanceInMm = distanceToDash;
    }
    distanceToDash -= m->distanceInMm;
    return (Speeds) {vTrans, 0};
}

/* ========== Maneuver-specific OnComplete callbacks ========== */

static void _FWD_OnComplete(ManeuverStatus status)
{
    CheckStatus(status);
    UpdateDistanceError(MOTION_FWD_M2M.distanceInMm);
}

static void _HFWD_OnComplete(ManeuverStatus status)
{
    CheckStatus(status);
    UpdateDistanceError(MOTION_FWD_M2C.distanceInMm);
}

static void _DFWD_OnComplete(ManeuverStatus status)
{
    CheckStatus(status);
    UpdateDistanceError(MOTION_DFWD.distanceInMm);
}

static void _TS180_OnComplete(ManeuverStatus status)
{
    _Generic_OnComplete(status);
    SpeedCtl_SetMode(SpeedCtlMode_STRAIGHT);
}

static void _TBACK_OnComplete(ManeuverStatus status)
{
    Buzzer_BeepTurningBack();
}

static void _SD_OnComplete(ManeuverStatus status)
{
    _Generic_OnComplete(status);
    SpeedCtl_SetMode(SpeedCtlMode_DIAGONAL);
}

/* ========== Module service things ========== */

static int execute(int argc, char *argv[])
{
    if (argc < 1)
        return -1;

    struct ManeuverConfig *config = &configs[maneuverMode];

    if (!strcmp(argv[0], "cfg") && argc == 6) {
        config->vTransTurn = atoi(argv[1]);
        config->vTransDash = atoi(argv[2]);
        config->vRot = atoi(argv[3]);
        config->motionConfig.aTrans = atoi(argv[4]);
        config->motionConfig.aRot = atoi(argv[5]);
    }
    else if (!strcmp(argv[0], "mode") && argc == 2) {
        ManeuverMode newMode = (ManeuverMode)(atoi(argv[1]) & 1);
        Maneuver_SetMode(newMode);
    }
    else if (!strcmp(argv[0], "mt") && argc == 3) {
        int distanceInMm = atoi(argv[1]);
        int angleInDeg = atoi(argv[2]);

        struct Motion motionPart1 = {distanceInMm / 2, angleInDeg / 2};
        struct Motion motionPart2 = {distanceInMm - motionPart1.distanceInMm, angleInDeg - motionPart1.angleInDeg};

        Motion_Start(&motionPart1, config->vTransTurn, config->vRot);
        while (Motion_IsOngoing());
        Motion_Start(&motionPart2, 0, 0);
        while (Motion_IsOngoing());
    }
    else if (!strcmp(argv[0], "mn") && argc == 2) {
        enum Maneuver maneuver = (enum Maneuver)atoi(argv[1]);
        Maneuver_Perform(maneuver, 0);
    }
    else if (!strcmp(argv[0], "ps")) {
        printf("Motion config:\n"
               "V1: %ld\tV2: %ld\tW: %ld\n"
               "At: %ld\tAr: %ld\n",
               config->vTransTurn, config->vTransDash, config->vRot,
               config->motionConfig.aTrans, config->motionConfig.aRot);
    }
    else
        return -2;

    return 0;
}

static void load(const uint8_t *buffer)
{
    memcpy(configs, buffer, sizeof(configs));
}

static void save(uint8_t *buffer)
{
    memcpy(buffer, configs, sizeof(configs));
}

const struct Settings SETT_Maneuver = {
    .dataSize = sizeof(configs),
    .load = load,
    .save = save
};

const struct ShellCommand CMD_Maneuver = {
    .name = "mnv",
    .execute = execute,
};
