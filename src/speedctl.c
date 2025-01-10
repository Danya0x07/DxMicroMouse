#include "speedctl.h"
#include "sensors.h"
#include "receivers.h"
#include "motors.h"
#include "encoders.h"
#include "imu.h"
#include "regulator.h"
#include "odometry.h"
#include <stdlib.h>
#include <string.h>

#define MIN_OUTPUT_THRESHOLD    50

static FunctionalState state = DISABLE;

static int32_t targetVTransInCountsPerS, targetVRotInLsbs;
static struct Regulator vTransRegulator, vRotRegulator;

void SpeedCtl_Reset(void)
{
    Regulator_Reset(&vTransRegulator);
    Regulator_Reset(&vRotRegulator);
    Encoders_Reset();
}

void SpeedCtl_SetState(FunctionalState newState)
{
    state = newState;
}

void SpeedCtl_SetMode(SpeedCtlMode mode)
{
    switch (mode) {
    case SpeedCtlMode_ENCODER:

        break;

    case SpeedCtlMode_IMU:

        break;

    case SpeedCtlMode_COMBINED:

        break;

    case SpeedCtlMode_TEST_PERPENDICULAR:

        break;
    }
}

void SpeedCtl_Setup(int32_t vTransKp, int32_t vTransKi, int32_t vRotKp, int32_t vRotKi)
{
    // Velocity PI regulator = Position PD regulator. Математика, ёпт.
    SpeedCtl_Reset();
    Regulator_Setup(&vTransRegulator, vTransKp, vTransKi, 0);
    Regulator_Setup(&vRotRegulator, vRotKp, vRotKi, 0);
}

void SpeedCtl_SetTarget(int32_t vTransInMmPerS, int32_t vRotInDegPerS)
{
    targetVTransInCountsPerS = vTransInMmPerS * COUNTS_PER_MM;
    targetVRotInLsbs = ((vRotInDegPerS << 15) / 200 + 5) / 10;
}

static int32_t AdjustRegOutput(int32_t regOutput)
{
    if (regOutput < MIN_OUTPUT_THRESHOLD && -regOutput < MIN_OUTPUT_THRESHOLD) {
        return 0;
    }
    return regOutput;
}

void SpeedCtl_Update(void)
{
    int32_t dl, dr;
    struct IMU_Data imuData;

    Encoders_GetDelta(&dl, &dr);
    IMU_GetData(&imuData);

    int32_t transInCounts = (dl + dr) / 2;
    int32_t vRotInLsbs = imuData.gyroZ;

    // IMU LSBs = ImuUnits/S = mImuUnits/ms ~ Deg/S = mDeg/ms
    //                                  ^
    int32_t deltaAngInMimuUnits = vRotInLsbs; // * 1ms;

    Odometry_Update(transInCounts, deltaAngInMimuUnits);

    int64_t transOutput = Regulator_Output(&vTransRegulator, targetVTransInCountsPerS, transInCounts * 1000);
    int64_t rotOutput = Regulator_Output(&vRotRegulator, targetVRotInLsbs, vRotInLsbs);

    int32_t leftOutput = ((transOutput - rotOutput) / 100000 + 5) / 10;
    int32_t rightOutput = ((transOutput + rotOutput) / 100000 + 5) / 10;

    leftOutput = AdjustRegOutput(leftOutput);
    rightOutput = AdjustRegOutput(rightOutput);

    if (state == ENABLE)
        Motors_SetPwm(leftOutput, rightOutput);
}

//~ static void TestOpenLoop(int16_t pwmL, int16_t pwmR)
//~ {
    //~ struct IMU_Data imuData;
    //~ int32_t dl, dr, dp;

    //~ printf("Test %d %d\n", pwmL, pwmR);
    //~ Motors_SetPwm(pwmL, pwmR);
    //~ for (int i = 0; i < 300; i++) {
        //~ IMU_GetData(&imuData);
        //~ Encoders_GetDelta(&dl, &dr);
        //~ dp = (dl + dr) / 2;
        //~ printf("%d,%ld\n", imuData.gyroZ, dp);
        //~ Millis_Wait(10);
    //~ }
    //~ Motors_SetPwm(0, 0);
//~ }

//~ void Controller_Update(void)
//~ {
    //~ uint16_t sensorsValues[5];

    //~ Sensors_ReadToBuffer(sensorsValues);

    //~ if (dance) {
        //~ int16_t pwmL = ((int32_t)2000 - sensorsValues[ReceiverChannel_LeftFront]) * 120 / 2000;
        //~ int16_t pwmR = ((int32_t)2000 - sensorsValues[ReceiverChannel_RightFront]) * 120 / 2000;

        //~ pwmL = pwmL > 0 ? pwmL + 240 : pwmL - 240;
        //~ pwmR = pwmR > 0 ? pwmR + 240 : pwmR - 240;

        //~ Motors_SetpwmLeft(pwmL);
        //~ Motors_SetpwmRight(pwmR);
        //~ Motors_SetTargetpwm(pwmL, pwmR);
    //~ } else {
        //~ Motors_SetTargetpwm(0, 0);
    //~ }
//~ }

static int execute(int argc, char *argv[])
{
    if (argc < 1)
        return -1;

    if (!strcmp(argv[0], "set")) {
        if (argc != 5)
            return -1;
        int32_t vTransKp = atol(argv[1]);
        int32_t vTransKi = atol(argv[2]);
        int32_t vRotKp = atol(argv[3]);
        int32_t vRotKi = atol(argv[4]);
        SpeedCtl_Setup(vTransKp, vTransKi, vRotKp, vRotKi);
    }
    //~ else if (!strcmp(argv[0], "test")) {
        //~ if (argc != 3)
            //~ return -1;
        //~ int16_t pwmL = atoi(argv[1]);
        //~ int16_t pwmR = atoi(argv[2]);
        //~ TestOpenLoop(pwmL, pwmR);
    //~ }
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
        if (mode > SpeedCtlMode_TEST_PERPENDICULAR)
            return -2;

        SpeedCtl_SetMode((SpeedCtlMode)mode);
    }
    else if (!strcmp(argv[0], "state")) {
        if (argc != 2)
            return -1;

        FunctionalState newState = (FunctionalState)!!atoi(argv[1]);
        SpeedCtl_SetState(newState);
    }
    else {
        return -2;
    }

    return 0;
}

struct Module SpeedCtl_module = {
    .name = "spctl",
    .execute = execute,
    .telemetry = NULL
};