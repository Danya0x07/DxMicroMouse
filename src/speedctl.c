#include "speedctl.h"
#include "sensors.h"
#include "receivers.h"
#include "motors.h"
#include "encoders.h"
#include "imu.h"
#include "odometry.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define ENCODER_RESOLUTION  12
#define WHEEL_DIAMETER  21
//#define COUNT_PER_MM    ((float)(1 << ENCODER_RESOLUTION) / (WHEEL_DIAMETER * M_PI))
#define COUNT_PER_MM    60.0
#define DEG_PER_MM_ROT  0.93 // arcsin((dxr - dxl) / mouse_width)

static FunctionalState state = ENABLE;

static int32_t transKp, transKd, rotKp, rotKd;
static int32_t targetVTransInCountsPer1024Ms, targetVRotInLsbsPer1024Ms;
static int32_t currentVTransInMmPerS, currentVRotInDegPerS;

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

void SpeedCtl_Setup(int32_t newTransKp, int32_t newTransKd, int32_t newRotKp, int32_t newRotKd)
{
    transKp = newTransKp;
    transKd = newTransKd;
    rotKp = newRotKp;
    rotKd = newRotKd;
}

void SpeedCtl_SetTarget(int32_t vTransInMmPerS, int32_t vRotInDegPerS)
{
    targetVTransInCountsPer1024Ms = ((vTransInMmPerS << 10) * COUNT_PER_MM / 100 + 5) / 10;
    targetVRotInLsbsPer1024Ms = (((int64_t)vRotInDegPerS << 25) / 200000 + 5) / 10;
}

static int64_t TranslationControl(int32_t deltaInCounts)
{
    static int32_t prevPositionErrorInCounts = 0;
    static int64_t positionErrorInCounts = 0;

    int32_t expectedDeltaInCounts = targetVTransInCountsPer1024Ms; // * 1ms

    positionErrorInCounts += expectedDeltaInCounts - deltaInCounts;
    int32_t diffErrorInCounts = positionErrorInCounts - prevPositionErrorInCounts;
    prevPositionErrorInCounts = positionErrorInCounts;

    return transKp * positionErrorInCounts + transKd * diffErrorInCounts;
}

static int64_t RotationControl(int32_t deltaInLsbs)
{
    static int32_t prevRotationErrorInLsbs = 0;
    static int64_t rotationErrorInLsbs = 0;

    int32_t expectedDeltaInLsbs = targetVRotInLsbsPer1024Ms; // * 1ms

    rotationErrorInLsbs += expectedDeltaInLsbs - deltaInLsbs;
    int32_t diffErrorInLsbs = rotationErrorInLsbs - prevRotationErrorInLsbs;
    prevRotationErrorInLsbs = rotationErrorInLsbs;

    return rotKp * rotationErrorInLsbs + rotKd * diffErrorInLsbs;
}

void SpeedCtl_Update(void)
{
    int32_t dl, dr;
    struct IMU_Data imuData;

    Encoders_GetDelta(&dl, &dr);
    IMU_GetData(&imuData);

    int32_t deltaPosInCounts = (dl + dr) / 2;
    int32_t deltaAngInLsbs = imuData.gyroZ;

    currentVTransInMmPerS = deltaPosInCounts * 1000 / COUNT_PER_MM;
    currentVRotInDegPerS = (deltaAngInLsbs * 2000 / 16384 + 1) / 2;

    //Odometry_Update(deltaPosInCounts, deltaAngInLsbs);

    int64_t posOutput = TranslationControl(deltaPosInCounts << 10);
    int64_t rotOutput = RotationControl(((deltaAngInLsbs << 10) / 100 + 5) / 10);
    int32_t leftOutput = (posOutput - rotOutput) / (1 << 20);
    int32_t rightOutput = (posOutput + rotOutput) / (1 << 20);

    if (state == ENABLE)
        Motors_SetPwm(leftOutput, rightOutput);
}

void TestOpenLoop(int16_t pwmL, int16_t pwmR)
{
    struct IMU_Data imuData;
    int32_t dl, dr, dp;

    printf("Test %d %d\n", pwmL, pwmR);
    Motors_SetPwm(pwmL, pwmR);
    for (int i = 0; i < 300; i++) {
        IMU_GetData(&imuData);
        Encoders_GetDelta(&dl, &dr);
        dp = (dl + dr) / 2;
        printf("%d,%ld\n", imuData.gyroZ, dp);
        Millis_Wait(10);
    }
    Motors_SetPwm(0, 0);
}

int32_t SpeedCtl_GetActualTransSpeed(void)
{
    return currentVTransInMmPerS;
}

int32_t SpeedCtl_GetActualRotSpeed(void)
{
    return currentVRotInDegPerS;
}


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
        transKp = atol(argv[1]);
        transKd = atol(argv[2]);
        rotKp = atol(argv[3]);
        rotKd = atol(argv[4]);
    }
    else if (!strcmp(argv[0], "test")) {
        if (argc != 3)
            return -1;
        int16_t pwmL = atoi(argv[1]);
        int16_t pwmR = atoi(argv[2]);
        TestOpenLoop(pwmL, pwmR);
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
        if (mode > SpeedCtlMode_TEST_PERPENDICULAR)
            return -2;

        SpeedCtl_SetMode((SpeedCtlMode)mode);
    }
    else if (!strcmp(argv[0], "state")) {
        if (argc != 2)
            return -1;

        SpeedCtl_SetState((FunctionalState)!!atoi(argv[1]));
    }
    else {
        return -2;
    }

    return 0;
}

static void WriteTelemetry(char out[TELEMETRY_STRING_SIZE])
{
    snprintf(out, TELEMETRY_STRING_SIZE, "V:%ld\tW:%ld\n", currentVTransInMmPerS, currentVRotInDegPerS);
}

static struct TelemetryControlBlock telemetryControlBlock = {
    .interval = 50,
    .write = WriteTelemetry
};

struct Module SpeedCtl_module = {
    .name = "spctl",
    .execute = execute,
    .telemetry = &telemetryControlBlock
};