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

static float kPv, kDv, kPw, kDw;
static float positionError, rotationError;
static volatile int32_t targetV, targetW;
static volatile int32_t currentV, currentW;

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

void SpeedCtl_Setup(float kpv, float kdv, float kpw, float kdw)
{
    kPv = kpv;
    kDv = kdv;
    kPw = kpw;
    kDw = kdw;
}

void SpeedCtl_SetTarget(int32_t v, int32_t w)
{
    targetV = v;
    targetW = w;
}

static float TranslationControl(float posDelta)
{
    static float prevPositionError = 0.0;
    float expectedPosDelta = targetV * 0.001;

    positionError += expectedPosDelta - posDelta;
    float diffError = positionError - prevPositionError;
    prevPositionError = positionError;

    return kPv * positionError + kDv * diffError;
}

static float RotationControl(float rotDelta)
{
    static float prevRotationError = 0.0;
    float expectedRotDelta = targetW * 0.001;

    rotationError += expectedRotDelta - rotDelta;
    float diffError = rotationError - prevRotationError;
    prevRotationError = rotationError;

    return kPw * rotationError + kDw * diffError;
}

void SpeedCtl_Update(void)
{
    //~ int32_t tgtV = targetV, tgtW = targetW;
    int32_t dCountL, dCountR;
    struct IMU_Data imuData;

    Encoders_GetDelta(&dCountL, &dCountR);
    IMU_GetData(&imuData);

    float dl = dCountL / COUNT_PER_MM;
    float dr = dCountR / COUNT_PER_MM;
    float dpos = (dl + dr) / 2;
    //float dang = (dr - dl) * DEG_PER_MM_ROT;
    float dang = imuData.gyroZ * 2000.0 / 32768;

    currentV = dpos * 1000;
    //currentW = dang * 1000;
    currentW = dang;

    Odometry_Update(dpos, dang);

    float posOutput = TranslationControl(dpos);
    float rotOutput = RotationControl(dang);
    int16_t leftOutput = posOutput - rotOutput;
    int16_t rightOutput = posOutput + rotOutput;

    if (leftOutput > 100) {
        leftOutput += 220;
    }
    else if (leftOutput < -100) {
        leftOutput -= 220;
    }
    else {
        leftOutput = 0;
    }

    if (rightOutput > 100) {
        rightOutput += 220;
    }
    else if (rightOutput < -100) {
        rightOutput -= 220;
    }
    else {
        rightOutput = 0;
    }

    if (state == ENABLE)
        Motors_SetPwm(leftOutput, rightOutput);
}

void TestOpenLoop(int16_t dutyL, int16_t dutyR)
{
    struct IMU_Data imuData;
    int32_t dl, dr, dp;

    printf("Test %d %d\n", dutyL, dutyR);
    Motors_SetPwm(dutyL, dutyR);
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
    return currentV;
}

int32_t SpeedCtl_GetActualRotSpeed(void)
{
    return currentW;
}


//~ void Controller_Update(void)
//~ {
    //~ uint16_t sensorsValues[5];

    //~ Sensors_ReadToBuffer(sensorsValues);

    //~ if (dance) {
        //~ int16_t dutyL = ((int32_t)2000 - sensorsValues[ReceiverChannel_LeftFront]) * 120 / 2000;
        //~ int16_t dutyR = ((int32_t)2000 - sensorsValues[ReceiverChannel_RightFront]) * 120 / 2000;

        //~ dutyL = dutyL > 0 ? dutyL + 240 : dutyL - 240;
        //~ dutyR = dutyR > 0 ? dutyR + 240 : dutyR - 240;

        //~ Motors_SetDutyLeft(dutyL);
        //~ Motors_SetDutyRight(dutyR);
        //~ Motors_SetTargetDuty(dutyL, dutyR);
    //~ } else {
        //~ Motors_SetTargetDuty(0, 0);
    //~ }
//~ }

static int execute(int argc, char *argv[])
{
    if (argc < 1)
        return -1;

    if (!strcmp(argv[0], "set")) {
        if (argc != 5)
            return -1;
        kPv = atoi(argv[1]) / 10.0;
        kDv = atoi(argv[2]) / 10.0;
        kPw = atoi(argv[3]) / 10.0;
        kDw = atoi(argv[4]) / 10.0;
    }
    else if (!strcmp(argv[0], "test")) {
        if (argc != 3)
            return -1;
        int16_t dutyL = atoi(argv[1]);
        int16_t dutyR = atoi(argv[2]);
        TestOpenLoop(dutyL, dutyR);
    }
    else if (!strcmp(argv[0], "tgt")) {
        if (argc != 3)
            return -1;
        int32_t tgtV = atoi(argv[1]);
        int32_t tgtW = atoi(argv[2]);
        SpeedCtl_SetTarget(tgtV, tgtW);
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
    snprintf(out, TELEMETRY_STRING_SIZE, "V:%ld\tW:%ld\n", currentV, currentW);
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