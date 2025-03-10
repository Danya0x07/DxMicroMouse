#include "motion.h"
#include "profile.h"
#include "speedctl.h"
#include <string.h>
#include <stdlib.h>

struct MotionConfig {
    int32_t vTransA;
    int32_t vTransB;
    int32_t vRot;
    int32_t aTrans;
    int32_t aRot;
};

static struct MotionConfig configs[] = {
    [MotionMode_SLOW] = {
        .vTransA = 400,
        .vTransB = 360,
        .vRot = 1000,
        .aTrans = 2500,
        .aRot = 5000
    },
    [MotionMode_FAST] = {
        .vTransA = 1100,
        .vTransB = 640,
        .vRot = 1000,
        .aTrans = 3000,
        .aRot = 9000
    }
};

static struct MotionConfig *config = &configs[MotionMode_SLOW];

#define CELLWIDTH   180
#define CELLHALF    90
#define WALLTHICKNESS   12
#define MOUSEBACKLEN    30

static const struct MotionCtlBlock {
    int32_t distanceInMm;
    int32_t angleInDeg;
    enum VTransChange {
        VTransChange_A2A,
        VTransChange_B2B,
        VTransChange_A20,
        VTransChange_B20,
        VTransChange_A2B,
        VTransChange_B2A
    } vTransChange;
} motions[] = {
    [Motion_PARK_BACK2WALL] = {
        .distanceInMm = -(CELLHALF - MOUSEBACKLEN - WALLTHICKNESS/2 + 7),
        .angleInDeg = 0,
        .vTransChange = VTransChange_B20
    },
    [Motion_PARK_FWD2DP] = {
        .distanceInMm = +(CELLHALF - MOUSEBACKLEN - WALLTHICKNESS/2 + CELLHALF),
        .angleInDeg = 0,
        .vTransChange = VTransChange_A2A
    },
    [Motion_PARK_FWD2DP_ACC2SLOW] = {
        .distanceInMm = +(CELLHALF - MOUSEBACKLEN - WALLTHICKNESS/2 + CELLHALF),
        .angleInDeg = 0,
        .vTransChange = VTransChange_B2B
    },
    [Motion_FWD_DP2DP] = {
        .distanceInMm = +(CELLWIDTH),
        .angleInDeg = 0,
        .vTransChange = VTransChange_A2A
    },
    [Motion_FWD_DP2DP_DECC] = {
        .distanceInMm = +(CELLWIDTH),
        .angleInDeg = 0,
        .vTransChange = VTransChange_A2B
    },
    [Motion_FWD_DP2DP_SLOW] = {
        .distanceInMm = +(CELLWIDTH),
        .angleInDeg = 0,
        .vTransChange = VTransChange_B2B
    },
    [Motion_FWD_DP2DP_ACC] = {
        .distanceInMm = +(CELLWIDTH),
        .angleInDeg = 0,
        .vTransChange = VTransChange_B2A
    },
    [Motion_FWD_DP2T] = {
        .distanceInMm = 20,
        .angleInDeg = 0,
        .vTransChange = VTransChange_A2B
    },
    [Motion_FWD_T2DP] = {
        .distanceInMm = 20,
        .angleInDeg = 0,
        .vTransChange = VTransChange_B2A
    },
    [Motion_FWD_DP2C] = {
        .distanceInMm = +(CELLHALF),
        .angleInDeg = 0,
        .vTransChange = VTransChange_A20
    },
    [Motion_FWD_DP2C_FROMSLOW] = {
        .distanceInMm = +(CELLHALF),
        .angleInDeg = 0,
        .vTransChange = VTransChange_B20
    },
    [Motion_SMOOTH_LEFT90] = {
        .distanceInMm = 110,
        .angleInDeg = 90,
        .vTransChange = VTransChange_B2B
    },
    [Motion_SMOOTH_RIGHT90] = {
        .distanceInMm = 110,
        .angleInDeg = -90,
        .vTransChange = VTransChange_B2B
    },
    [Motion_SMOOTH_LEFT90_LONG] = {
        .distanceInMm = 142,
        .angleInDeg = 90,
        .vTransChange = VTransChange_B2B
    },
    [Motion_SMOOTH_RIGHT90_LONG] = {
        .distanceInMm = 142,
        .angleInDeg = -90,
        .vTransChange = VTransChange_B2B
    },
    [Motion_PIVOT_LEFT90] = {
        .distanceInMm = 0,
        .angleInDeg = 90,
        .vTransChange = VTransChange_B20
    },
    [Motion_PIVOT_RIGHT90] = {
        .distanceInMm = 0,
        .angleInDeg = -90,
        .vTransChange = VTransChange_B20
    },
    [Motion_PIVOT_LEFT180] = {
        .distanceInMm = 0,
        .angleInDeg = 180,
        .vTransChange = VTransChange_B20
    },
    [Motion_PIVOT_RIGHT180] = {
        .distanceInMm = 0,
        .angleInDeg = -180,
        .vTransChange = VTransChange_B20
    },
};

#define MAXIMUM_ALLOWED_CORRECTION  20

static struct MotionCorrection correction = {0};
static struct Profile vTransProfile, vRotProfile;
static bool ongoing = false;
static FunctionalState discreteMotion = DISABLE;

static void Configure(const struct MotionConfig *newConfig)
{
    if (newConfig->vTransA > 0 && newConfig->vTransB > 0 &&
            newConfig->vRot > 0 && newConfig->aTrans > 0 && newConfig->aRot > 0)
        *config = *newConfig;
}

static void Start(const struct MotionCtlBlock *m)
{
    if (m->distanceInMm > 0) {
        if (correction.distanceInMm < -MAXIMUM_ALLOWED_CORRECTION)
            correction.distanceInMm = -MAXIMUM_ALLOWED_CORRECTION;
    }
    else {
        if (correction.distanceInMm > MAXIMUM_ALLOWED_CORRECTION)
            correction.distanceInMm = MAXIMUM_ALLOWED_CORRECTION;
    }

    vTransProfile.square = m->distanceInMm + correction.distanceInMm;
    correction.distanceInMm = 0;
    vTransProfile.a1 = vTransProfile.a2 = config->aTrans;
    vTransProfile.vStart = vTransProfile.vEnd;

    if (discreteMotion) {
        vTransProfile.vCoast = config->vTransB;
        vTransProfile.vEnd = 0;
    }
    else switch (m->vTransChange) {
        case VTransChange_A2A:
            vTransProfile.vCoast = config->vTransA;
            vTransProfile.vEnd = config->vTransA;
            break;

        case VTransChange_B2B:
            vTransProfile.vCoast = config->vTransB;
            vTransProfile.vEnd = config->vTransB;
            break;

        case VTransChange_A20:
            vTransProfile.vCoast = config->vTransA;
            vTransProfile.vEnd = 0;
            break;

        case VTransChange_B20:
            vTransProfile.vCoast = config->vTransB;
            vTransProfile.vEnd = 0;
            break;

        case VTransChange_A2B:
            vTransProfile.vCoast = config->vTransA;
            vTransProfile.vEnd = config->vTransB;
            break;

        case VTransChange_B2A:
            vTransProfile.vCoast = config->vTransB;
            vTransProfile.vEnd = config->vTransA;
            break;
    }

    vRotProfile.square = m->angleInDeg;
    vRotProfile.a1 = vRotProfile.a2 = config->aRot;
    vRotProfile.vStart = vRotProfile.vEnd = 0;
    vRotProfile.vCoast = config->vRot;

    if (m->distanceInMm != 0) {
        Profile_Setup(&vTransProfile, Millis_Get());
        Profile_SyncByTotalTime(&vRotProfile, &vTransProfile);
    }
    else {
        Profile_Setup(&vRotProfile, Millis_Get());
    }

    ongoing = true;
}

void Motion_SetMode(enum MotionMode newMode)
{
    config = &configs[newMode];
}

void Motion_SetDiscreteMotion(FunctionalState newState)
{
    discreteMotion = newState;
}

void Motion_Start(enum Motion motion)
{
    //printf("Motion: %d, derr: %ld\n", motion, correction.distanceInMm);
    Start(&motions[motion]);
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

void Motion_SetCorrection(struct MotionCorrection newCorrection)
{
    correction = newCorrection;
}

struct MotionCorrection Motion_GetCorrection(void)
{
    return correction;
}

static int execute(int argc, char *argv[])
{
    if (argc < 1)
        return -1;

    if (!strcmp(argv[0], "set") && argc == 6) {
        struct MotionConfig newConfig = {
            .vTransA = atoi(argv[1]),
            .vTransB = atoi(argv[2]),
            .vRot = atoi(argv[3]),
            .aTrans = atoi(argv[4]),
            .aRot = atoi(argv[5])
        };
        Configure(&newConfig);
    }
    else if (!strcmp(argv[0], "mode") && argc == 2) {
        enum MotionMode newMode = (enum MotionMode)(atoi(argv[1]) & 1);
        Motion_SetMode(newMode);
    }
    else if (!strcmp(argv[0], "mv") && argc == 3) {
        struct MotionCtlBlock motion = {
            .distanceInMm = atoi(argv[1]),
            .angleInDeg = atoi(argv[2]),
            .vTransChange = VTransChange_A20
        };
        Start(&motion);
    }
    else if (!strcmp(argv[0], "ps")) {
        printf("Motion config:\n"
               "Va: %ld\tVb: %ld\tAt: %ld\n"
               "Wc: %ld\tAr: %ld\n",
               config->vTransA, config->vTransB, config->aTrans,
               config->vRot, config->aRot);
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

static struct ModuleSettings settings = {
    .dataSize = sizeof(configs),
    .load = load,
    .save = save
};

struct Module Motion_module = {
    .name = "motion",
    .execute = execute,
    .settings = &settings
};