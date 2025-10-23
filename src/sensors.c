#include "sensors.h"
#include "mcu.h"
#include "emitters.h"
#include "receivers.h"
#include "utils.h"

#include <string.h>
#include <stdlib.h>

#define FINGER_THRESHOLD    950
#define MAX_ERROR   30

typedef struct SensorsDistance SensorData;

static struct SensorsDistance distance;
static struct SensorsWalls walls;
static bool transitionDetected;

static SensorData calibValue = {
    .leftFront = 6545,
    .leftSide = 6818,
    .rightSide = 7051,
    .rightFront = 6816
};

static SensorData middleValue = {
    .leftFront = 980,
    .leftSide = 914,
    .rightSide = 903,
    .rightFront = 980
};

static struct {
    int32_t left;
    int32_t front;
    int32_t right;
} threshold = {980, 990, 980};

static struct {
    int32_t left;
    int32_t right;
} diagonalMiddleMinimum = {1000, 1000};

static enum TelemetryMode {
    TelemetryMode_DISTANCES,
    TelemetryMode_WALLS
} telemetryMode;

static void MeasureReflection(SensorData *reflection)
{
    Emitters_LeftFrontOn();
    Emitters_RightFrontOn();
    Micros_Wait(60);

    reflection->leftFront = Receivers_ReadChannel(ReceiverChannel_LF);
    reflection->rightFront = Receivers_ReadChannel(ReceiverChannel_RF);

    Emitters_LeftFrontOff();
    Emitters_RightFrontOff();
    Micros_Wait(60);

    reflection->leftFront -= Receivers_ReadChannel(ReceiverChannel_LF);
    reflection->rightFront -= Receivers_ReadChannel(ReceiverChannel_RF);

    Emitters_LeftSideOn();
    Emitters_RightSideOn();
    Micros_Wait(60);

    reflection->leftSide = Receivers_ReadChannel(ReceiverChannel_LS);
    reflection->rightSide = Receivers_ReadChannel(ReceiverChannel_RS);
    Emitters_LeftSideOff();
    Emitters_RightSideOff();
    Micros_Wait(60);

    reflection->leftSide -= Receivers_ReadChannel(ReceiverChannel_LS);
    reflection->rightSide -= Receivers_ReadChannel(ReceiverChannel_RS);

    if (reflection->leftFront < 0)
        reflection->leftFront = 0;
    if (reflection->leftSide < 0)
        reflection->leftSide = 0;
    if (reflection->rightSide < 0)
        reflection->rightSide = 0;
    if (reflection->rightFront < 0)
        reflection->rightFront = 0;
}

static void Update(void)
{
    SensorData reflection;

    MeasureReflection(&reflection);

    distance.leftFront = 1000 * calibValue.leftFront / ln1000(reflection.leftFront);
    distance.leftSide = 1000 * calibValue.leftSide / ln1000(reflection.leftSide);
    distance.rightSide = 1000 * calibValue.rightSide / ln1000(reflection.rightSide);
    distance.rightFront = 1000 * calibValue.rightFront / ln1000(reflection.rightFront);

    bool leftWall = distance.leftSide <= threshold.left;
    bool rightWall = distance.rightSide <= threshold.right;

    transitionDetected = walls.left && !leftWall && (middleValue.leftSide - distance.leftSide <= MAX_ERROR);
    transitionDetected |= walls.right && !rightWall && (middleValue.rightSide - distance.rightSide <= MAX_ERROR);

    walls.front = (distance.leftFront + distance.rightFront) / 2 <= threshold.front;
    walls.left = leftWall;
    walls.right = rightWall;
}

static void UpdateForCalibration(void)
{
    SensorData reflection;

    MeasureReflection(&reflection);

    distance.leftFront = ln1000(reflection.leftFront);
    distance.leftSide = ln1000(reflection.leftSide);
    distance.rightSide = ln1000(reflection.rightSide);
    distance.rightFront = ln1000(reflection.rightFront);

    walls.left = 0;
    walls.right = 0;
    walls.front = 0;
    transitionDetected = false;
}

static void UpdateWithoutLightening(void)
{
    Micros_Wait(250);

    distance.leftFront = 10000;
    distance.leftSide = 10000;
    distance.rightSide = 10000;
    distance.rightFront = 10000;

    walls.left = 0;
    walls.right = 0;
    walls.front = 0;
}

void (*Sensors_Update)(void) = Update;

void Sensors_SetState(FunctionalState state)
{
    Sensors_Update = state ? Update : UpdateWithoutLightening;
}

FunctionalState Sensors_GetState(void)
{
    return Sensors_Update == Update ? ENABLE : DISABLE;
}

void Sensors_ReadDistance(struct SensorsDistance *d)
{
    SysTick_DisableInterrupt();
    *d = distance;
    SysTick_EnableInterrupt();
}

void Sensors_ReadWalls(struct SensorsWalls *w)
{
    SysTick_DisableInterrupt();
    *w = walls;
    SysTick_EnableInterrupt();
}

bool Sensors_DetectFinger(void)
{
    struct SensorsDistance d;

    Sensors_ReadDistance(&d);
    return d.rightFront <= FINGER_THRESHOLD;
}

bool Sensors_DetectTransition(void)
{
    return transitionDetected;
}

int32_t Sensors_GetStraightDeviation(void)
{
    int32_t leftError = middleValue.leftSide - distance.leftSide;
    int32_t rightError = distance.rightSide - middleValue.rightSide;

    if (walls.left && walls.right)
        return leftError + rightError;
    else if (walls.left)
        return 2 * leftError;
    else if (walls.right)
        return 2 * rightError;
    else
        return 0;
}

int32_t Sensors_GetDiagonalDeviation(void)
{
    if (distance.leftFront < diagonalMiddleMinimum.left)
        return diagonalMiddleMinimum.left - distance.leftFront;
    else if (distance.rightFront < diagonalMiddleMinimum.right)
        return distance.rightFront - diagonalMiddleMinimum.right;
    else
        return 0;
}

void Sensors_GetTrimmingErrors(int32_t *transError, int32_t *rotError)
{
    if (walls.front) {
        *transError = distance.leftFront - middleValue.leftFront + distance.rightFront - middleValue.rightFront;
        *rotError = middleValue.leftFront - distance.leftFront + distance.rightFront - middleValue.rightFront;
    }
    else {
        *transError = *rotError = 0;
    }
}

static void PrintTelemetry(void)
{
    struct SensorsDistance d;
    struct SensorsWalls w;

    if (telemetryMode == TelemetryMode_DISTANCES) {
        Sensors_ReadDistance(&d);
        printf("LF:%-5ld\tLS:%-5ld\tRS:%-5ld\tRF:%-5ld\n", d.leftFront, d.leftSide, d.rightSide, d.rightFront);
    }
    else if (telemetryMode == TelemetryMode_WALLS) {
        Sensors_ReadWalls(&w);
        printf("L:%-5d\tF:%-5d\tR:%-5d\n", w.left, w.front, w.right);
    }
}

static int execute(int argc, char *argv[])
{
    if (argc < 1)
        return -1;

    if (!strcmp(argv[0], "lon"))
        Sensors_SetState(ENABLE);
    else if (!strcmp(argv[0], "loff"))
        Sensors_SetState(DISABLE);
    else if (!strcmp(argv[0], "lcal"))
        Sensors_Update = UpdateForCalibration;
    else if (!strcmp(argv[0], "tm") && argc == 2)
        telemetryMode = (enum TelemetryMode)atoi(argv[1]);
    else if (!strcmp(argv[0], "thr") && argc == 4) {
        threshold.left = atoi(argv[1]);
        threshold.front = atoi(argv[2]);
        threshold.right = atoi(argv[3]);
    }
    else if (!strcmp(argv[0], "mid") && argc == 5) {
        middleValue.leftFront = atoi(argv[1]);
        middleValue.leftSide = atoi(argv[2]);
        middleValue.rightSide = atoi(argv[3]);
        middleValue.rightFront = atoi(argv[4]);
    }
    else if (!strcmp(argv[0], "cal") && argc == 5) {
        calibValue.leftFront = atoi(argv[1]);
        calibValue.leftSide = atoi(argv[2]);
        calibValue.rightSide = atoi(argv[3]);
        calibValue.rightFront = atoi(argv[4]);
    }
    else if (!strcmp(argv[0], "dmm") && argc == 3) {
        diagonalMiddleMinimum.left = atoi(argv[1]);
        diagonalMiddleMinimum.right = atoi(argv[2]);
    }
    else if (!strcmp(argv[0], "ps")) {
        printf("Sensors settings:\n"
               "thresh: %ld %ld %ld\n"
               "cal: %ld %ld %ld %ld\n"
               "mid: %ld %ld %ld %ld\n"
               "dmm: %ld %ld\n",
               threshold.left, threshold.front, threshold.right,
               calibValue.leftFront, calibValue.leftSide, calibValue.rightSide, calibValue.rightFront,
               middleValue.leftFront, middleValue.leftSide, middleValue.rightSide, middleValue.rightFront,
               diagonalMiddleMinimum.left, diagonalMiddleMinimum.right);
    }
    else
        return -2;

    return 0;
}

static void load(const uint8_t *buffer)
{
    memcpy(&threshold, buffer, sizeof(threshold));
    buffer += sizeof(threshold);
    memcpy(&calibValue, buffer, sizeof(calibValue));
    buffer += sizeof(calibValue);
    memcpy(&middleValue, buffer, sizeof(middleValue));
    buffer += sizeof(middleValue);
    memcpy(&diagonalMiddleMinimum, buffer, sizeof(diagonalMiddleMinimum));
}

static void save(uint8_t *buffer)
{
    memcpy(buffer, &threshold, sizeof(threshold));
    buffer += sizeof(threshold);
    memcpy(buffer, &calibValue, sizeof(calibValue));
    buffer += sizeof(calibValue);
    memcpy(buffer, &middleValue, sizeof(middleValue));
    buffer += sizeof(middleValue);
    memcpy(buffer, &diagonalMiddleMinimum, sizeof(diagonalMiddleMinimum));
}

const struct Settings SETT_Sensors = {
    .dataSize = sizeof(threshold) + sizeof(calibValue) + sizeof(middleValue) + sizeof(diagonalMiddleMinimum),
    .load = load,
    .save = save
};

struct SchedulerTask TASK_TmSensors = {
    .execute = PrintTelemetry,
    .period = 300
};

const struct ShellCommand CMD_Sensors = {
    .name = "sens",
    .execute = execute,
};
