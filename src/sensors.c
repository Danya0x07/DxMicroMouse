#include "sensors.h"
#include "emitters.h"
#include "receivers.h"
#include "utils.h"

#include <string.h>
#include <stdlib.h>

#define FINGER_THRESHOLD    950
#define MAX_ALLOWED_ERROR   30

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

static struct {
    int32_t left;
    int32_t front;
    int32_t right;
} threshold = {1000, 980, 1000}, middle = {914, 980, 903};

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

    transitionDetected = walls.left && !leftWall && (middle.left - distance.leftSide <= MAX_ALLOWED_ERROR);
    transitionDetected |= walls.right && !rightWall && (middle.right - distance.rightSide <= MAX_ALLOWED_ERROR);

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

void Sensors_SetLightening(FunctionalState state)
{
    Sensors_Update = state ? Update : UpdateWithoutLightening;
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

int32_t Sensors_GetSteeringError(void)
{
    int32_t error = 0;

    if (walls.left && walls.right)
        error = distance.rightSide - distance.leftSide;
    else if (distance.leftSide <= middle.left)
        error = 2 * (middle.left - distance.leftSide);
    else if (distance.rightSide <= middle.right)
        error = 2 * (distance.rightSide - middle.right);

    return error;
}

static void WriteTelemetry(char out[TELEMETRY_STRING_SIZE])
{
    struct SensorsDistance d;
    struct SensorsWalls w;

    if (telemetryMode == TelemetryMode_DISTANCES) {
        Sensors_ReadDistance(&d);
        snprintf(out, TELEMETRY_STRING_SIZE, "LF:%-5ld\tLS:%-5ld\tRS:%-5ld\tRF:%-5ld\n",
                d.leftFront, d.leftSide, d.rightSide, d.rightFront);
    }
    else if (telemetryMode == TelemetryMode_WALLS) {
        Sensors_ReadWalls(&w);
        snprintf(out, TELEMETRY_STRING_SIZE, "L:%-5d\tF:%-5d\tR:%-5d\n",
                w.left, w.front, w.right);
    }
}

static int execute(int argc, char *argv[])
{
    if (argc < 1)
        return -1;

    if (!strcmp(argv[0], "lon"))
        Sensors_SetLightening(ENABLE);
    else if (!strcmp(argv[0], "loff"))
        Sensors_SetLightening(DISABLE);
    else if (!strcmp(argv[0], "lcal"))
        Sensors_Update = UpdateForCalibration;
    else if (!strcmp(argv[0], "tm") && argc == 2)
        telemetryMode = (enum TelemetryMode)atoi(argv[1]);
    else if (!strcmp(argv[0], "thr") && argc == 4) {
        threshold.left = atoi(argv[1]);
        threshold.front = atoi(argv[2]);
        threshold.right = atoi(argv[3]);
    }
    else if (!strcmp(argv[0], "mid") && argc == 4) {
        middle.left = atoi(argv[1]);
        middle.front = atoi(argv[2]);
        middle.right = atoi(argv[3]);
    }
    else if (!strcmp(argv[0], "cal") && argc == 5) {
        calibValue.leftFront = atoi(argv[1]);
        calibValue.leftSide = atoi(argv[2]);
        calibValue.rightSide = atoi(argv[3]);
        calibValue.rightFront = atoi(argv[4]);
    }
    else if (!strcmp(argv[0], "ps")) {
        printf("Sensors settings:\n"
               "thresh: %ld %ld %ld\n"
               "cal: %ld %ld %ld %ld\n"
               "mid: %ld %ld %ld\n",
               threshold.left, threshold.front, threshold.right,
               calibValue.leftFront, calibValue.leftSide, calibValue.rightSide, calibValue.rightFront,
               middle.left, middle.front, middle.right);
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
    memcpy(&middle, buffer, sizeof(middle));
}

static void save(uint8_t *buffer)
{
    memcpy(buffer, &threshold, sizeof(threshold));
    buffer += sizeof(threshold);
    memcpy(buffer, &calibValue, sizeof(calibValue));
    buffer += sizeof(calibValue);
    memcpy(buffer, &middle, sizeof(middle));
}

static struct ModuleSettings settings = {
    .dataSize = sizeof(threshold) + sizeof(calibValue) + sizeof(middle),
    .load = load,
    .save = save
};

static struct ModuleTelemetry telemetry = {
    .interval = 300,
    .write = WriteTelemetry
};

struct Module Sensors_module = {
    .name = "sensors",
    .execute = execute,
    .telemetry = &telemetry,
    .settings = &settings
};
