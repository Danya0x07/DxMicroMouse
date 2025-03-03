#include "speedctl.h"
#include "sensors.h"
#include "motors.h"
#include "encoders.h"
#include "imu.h"
#include "regulator.h"
#include "odometry.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static FunctionalState state = DISABLE;
static SpeedCtlMode mode = SpeedCtlMode_STRAIGHT;

static int32_t targetVTransInUmPerS, targetVRotInLsbs;
static int32_t vTransInUmPerS;

static struct Regulator vTransRegulator, vRotRegulator;

static struct {
    int32_t vTransKp, vTransKi, vTransKd;
    int32_t vRotKp, vRotKi, vRotKd;
    int32_t coeffAccel;
    int32_t coeffSensors;
    int32_t minOutputThreshold;
    int32_t motorFeedForward;
} params = {
    .vTransKp = 7000, .vTransKi = 15, .vTransKd = 0,
    .vRotKp = 100000, .vRotKi = 30000, .vRotKd = 0,
    .coeffAccel = 990,
    .coeffSensors = 1000,
    .minOutputThreshold = 0,
    .motorFeedForward = 0
};

void SpeedCtl_Reset(void)
{
    vTransInUmPerS = 0;
    Regulator_Reset(&vTransRegulator);
    Regulator_Reset(&vRotRegulator);
    Encoders_Reset();
}

void SpeedCtl_SetState(FunctionalState newState)
{
    state = newState;
    if (state == DISABLE) {
        SpeedCtl_SetTarget(0, 0);
        Motors_SetPwm(0, 0);
    }
}

void SpeedCtl_SetMode(SpeedCtlMode newMode)
{
    mode = newMode;
}

void SpeedCtl_Setup(void)
{
    // Velocity PI regulator = Position PD regulator. Математика, ёпт.
    SpeedCtl_Reset();
    Regulator_Setup(&vTransRegulator, params.vTransKp, params.vTransKi, params.vTransKd);
    Regulator_Setup(&vRotRegulator, params.vRotKp, params.vRotKi, params.vRotKd);
}

void SpeedCtl_SetTarget(int32_t vTransInMmPerS, int32_t vRotInDegPerS)
{
    SpeedCtl_Reset();
    //~ targetVTransInUmPerMs = ((vTransInMmPerS << 15) / 4000 + 5) / 10;
    targetVTransInUmPerS = vTransInMmPerS * 1000;
    targetVRotInLsbs = (vRotInDegPerS << 15) / 2000;
}

static int32_t AdjustRegOutput(int32_t regOutput)
{
    int32_t sign = 1;

    if (regOutput < 0) {
        sign = -1;
        regOutput = -regOutput;
    }

    if (regOutput < params.minOutputThreshold)
        regOutput = 0;
    else
        regOutput += params.motorFeedForward;

    if (regOutput > MOTOR_PWM_MAX)
        regOutput = MOTOR_PWM_MAX;

    return sign * regOutput;
}

void SpeedCtl_Update(void)
{
    int32_t dL, dR;
    struct IMU_Data imuData;

    Encoders_GetDelta(&dL, &dR);
    IMU_GetData(&imuData);

    int32_t transInCounts = dL + dR;

    if (transInCounts > 0)
        transInCounts++;
    else if (transInCounts < 0)
        transInCounts--;
    transInCounts /= 2;

    // IMU LSBs = ImuUnits/S = mImuUnits/ms

    // mImuUnits/ms ~ m/(s^2) = (m/s)/s = (mm/s)/ms
    vTransInUmPerS =
            ((int64_t)vTransInUmPerS * params.coeffAccel - (int64_t)imuData.accelX * 9800 * params.coeffAccel / 8192
            + (1000 - params.coeffAccel) * (int64_t)transInCounts * 1000000 / COUNTS_PER_MM) / 100;
    vTransInUmPerS += vTransInUmPerS > 0 ? 5 : -5;
    vTransInUmPerS /= 10;

    int32_t vRotInLsbs = imuData.gyroZ;

    // mImuUnits/ms ~ Deg/S = mDeg/ms
    int32_t rotInMimuUnits = vRotInLsbs; // * 1ms;

    Odometry_UpdateReckon(transInCounts, rotInMimuUnits);

    int64_t transOutput = Regulator_Output(&vTransRegulator, targetVTransInUmPerS, vTransInUmPerS);
    int64_t rotOutput = Regulator_Output(&vRotRegulator, targetVRotInLsbs, vRotInLsbs);

    if (mode == SpeedCtlMode_STRAIGHT) {
        rotOutput += (int64_t)params.coeffSensors * Sensors_GetSteeringError();
    }

    int32_t leftOutput = (transOutput - rotOutput) / 100000;
    leftOutput += leftOutput > 0 ? 5 : -5;
    leftOutput /= 10;

    int32_t rightOutput = (transOutput + rotOutput) / 100000;
    rightOutput += rightOutput > 0 ? 5 : -5;
    rightOutput /= 10;

    leftOutput = AdjustRegOutput(leftOutput);
    rightOutput = AdjustRegOutput(rightOutput);

    if (state == ENABLE)
        Motors_SetPwm(leftOutput, rightOutput);
}

static int execute(int argc, char *argv[])
{
    if (argc < 1)
        return -1;

    if (!strcmp(argv[0], "sett")) {
        if (argc != 4)
            return -1;
        params.vTransKp = atol(argv[1]);
        params.vTransKi = atol(argv[2]);
        params.vTransKd = atol(argv[3]);
        SpeedCtl_Setup();
    }
    else if (!strcmp(argv[0], "setr")) {
        if (argc != 4)
            return -1;
        params.vRotKp = atol(argv[1]);
        params.vRotKi = atol(argv[2]);
        params.vRotKd = atol(argv[3]);
        SpeedCtl_Setup();
    }
    else if (!strcmp(argv[0], "th")) {
        if (argc != 3)
            return -1;
        params.minOutputThreshold = atoi(argv[1]);
        params.motorFeedForward = atoi(argv[2]);
    }
    else if (!strcmp(argv[0], "tgt")) {
        if (argc != 3)
            return -1;
        int32_t vTransInMmPerS = atoi(argv[1]);
        int32_t vRotInDegPerS = atoi(argv[2]);
        SpeedCtl_SetTarget(vTransInMmPerS, vRotInDegPerS);
    }
    else if (!strcmp(argv[0], "mode")) {
        if (argc != 2)
            return -1;

        unsigned mode = atoi(argv[1]);
        if (mode > SpeedCtlMode_TURN)
            return -2;

        SpeedCtl_SetMode((SpeedCtlMode)mode);
    }
    else if (!strcmp(argv[0], "state")) {
        if (argc != 2)
            return -1;

        FunctionalState newState = (FunctionalState)!!atoi(argv[1]);
        SpeedCtl_SetState(newState);
    }
    else if (!strcmp(argv[0], "acc")) {
        if (argc != 2)
            return -1;
        params.coeffAccel = atoi(argv[1]);
    }
    else if (!strcmp(argv[0], "sens")) {
        if (argc != 2)
            return -1;
        params.coeffSensors = atoi(argv[1]);
    }
    else if (!strcmp(argv[0], "ps")) {
        printf("SpeedCtl settings:\n"
               "vTrans: %ld %ld %ld\n"
               "vRot: %ld %ld %ld\n"
               "cA: %ld\tcS: %ld\n"
               "minThresh: %ld\tmFF: %ld\n",
               params.vTransKp, params.vTransKi, params.vTransKd,
               params.vRotKp, params.vRotKi, params.vRotKd,
               params.coeffAccel, params.coeffSensors,
               params.minOutputThreshold, params.motorFeedForward);
    }
    else {
        return -2;
    }

    return 0;
}

static void WriteTelemetry(char out[TELEMETRY_STRING_SIZE])
{
    snprintf(out, TELEMETRY_STRING_SIZE,
        "tgt: %ld\tv: %ld\n",
        targetVTransInUmPerS,
        vTransInUmPerS
    );
}

static struct ModuleTelemetry telemetry = {
    .interval = 200,
    .write = WriteTelemetry
};

static void load(const uint8_t *buffer)
{
    memcpy(&params, buffer, sizeof(params));
}

static void save(uint8_t *buffer)
{
    memcpy(buffer, &params, sizeof(params));
}

static struct ModuleSettings settings = {
    .dataSize = sizeof(params),
    .load = load,
    .save = save
};

struct Module SpeedCtl_module = {
    .name = "spctl",
    .execute = execute,
    .telemetry = &telemetry,
    .settings = &settings
};