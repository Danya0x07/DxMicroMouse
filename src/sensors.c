#include "sensors.h"
#include "emitters.h"
#include "receivers.h"

#include <string.h>
#include <stdlib.h>

#define FINGER_THRESHOLD    3000

static struct SensorsDistance currentDistance;
static struct SensorsWalls currentWalls;

static struct {
    int32_t left;
    int32_t front;
    int32_t right;
} threshold = {2000, 2000, 2000};

static enum TelemetryMode {
    TelemetryMode_DISTANCES,
    TelemetryMode_WALLS
} telemetryMode;

static void update(void)
{
    Emitters_LeftFrontOn();
    Emitters_RightFrontOn();
    Emitters_FrontOn();
    Micros_Wait(60);

    currentDistance.leftFront = Receivers_ReadChannel(ReceiverChannel_LF);
    currentDistance.rightFront = Receivers_ReadChannel(ReceiverChannel_RF);
    currentDistance.front = Receivers_ReadChannel(ReceiverChannel_F);

    Emitters_LeftFrontOff();
    Emitters_RightFrontOff();
    Emitters_FrontOff();
    Micros_Wait(60);

    currentDistance.leftFront -= Receivers_ReadChannel(ReceiverChannel_LF);
    currentDistance.rightFront -= Receivers_ReadChannel(ReceiverChannel_RF);
    currentDistance.front -= Receivers_ReadChannel(ReceiverChannel_F);

    Emitters_LeftSideOn();
    Emitters_RightSideOn();
    Micros_Wait(60);

    currentDistance.leftSide = Receivers_ReadChannel(ReceiverChannel_LS);
    currentDistance.rightSide = Receivers_ReadChannel(ReceiverChannel_RS);
    Emitters_LeftSideOff();
    Emitters_RightSideOff();
    Micros_Wait(60);

    currentDistance.leftSide -= Receivers_ReadChannel(ReceiverChannel_LS);
    currentDistance.rightSide -= Receivers_ReadChannel(ReceiverChannel_RS);

    currentWalls.left = currentDistance.leftSide >= threshold.left;
    currentWalls.right = currentDistance.rightSide >= threshold.right;
    currentWalls.front = (currentDistance.leftFront + currentDistance.rightFront) / 2 >= threshold.front;
}

static void updateWithoutLightening(void)
{
    Micros_Wait(250);
    currentDistance.leftFront = Receivers_ReadChannel(ReceiverChannel_LF);
    currentDistance.leftSide = Receivers_ReadChannel(ReceiverChannel_LS);
    currentDistance.rightSide = Receivers_ReadChannel(ReceiverChannel_RS);
    currentDistance.rightFront = Receivers_ReadChannel(ReceiverChannel_RF);
    currentDistance.front = Receivers_ReadChannel(ReceiverChannel_F);
}

void (*Sensors_Update)(void) = update;

void Sensors_SetLightening(FunctionalState state)
{
    Sensors_Update = state ? update : updateWithoutLightening;
}

void Sensors_ReadDistance(struct SensorsDistance *distance)
{
    SysTick_DisableInterrupt();
    *distance = currentDistance;
    SysTick_EnableInterrupt();
}

void Sensors_ReadWalls(struct SensorsWalls *walls)
{
    SysTick_DisableInterrupt();
    *walls = currentWalls;
    SysTick_EnableInterrupt();
}

bool Sensors_DetectFinger(void)
{
    struct SensorsDistance distance;

    Sensors_ReadDistance(&distance);
    return distance.rightFront <= FINGER_THRESHOLD;
}

int32_t Sensors_GetSteeringError(void)
{
    int32_t error = 0;

    if (currentWalls.left && currentWalls.right)
        error = currentDistance.leftSide - currentDistance.rightSide;
    else if (currentWalls.left)
        error = 2 * (currentDistance.leftSide - threshold.left);
    else if (currentWalls.right)
        error = 2 * (threshold.right - currentDistance.rightSide);

    return error;
}

static void WriteTelemetry(char out[TELEMETRY_STRING_SIZE])
{
    struct SensorsDistance distance;
    struct SensorsWalls walls;

    if (telemetryMode == TelemetryMode_DISTANCES) {
        Sensors_ReadDistance(&distance);
        snprintf(out, TELEMETRY_STRING_SIZE, "LF:%-5ld\tLS:%-5ld\tF:%-5ld\tRS:%-5ld\tRF:%-5ld\n",
                distance.leftFront, distance.leftSide, distance.front, distance.rightSide, distance.rightFront);
    }
    else if (telemetryMode == TelemetryMode_WALLS) {
        Sensors_ReadWalls(&walls);
        snprintf(out, TELEMETRY_STRING_SIZE, "L:%-5d\tF:%-5d\tR:%-5d\n",
                walls.left, walls.front, walls.right);
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
    else if (!strcmp(argv[0], "tm") && argc == 2)
        telemetryMode = (enum TelemetryMode)atoi(argv[1]);
    else if (!strcmp(argv[0], "thr") && argc == 4) {
        threshold.left = atoi(argv[1]);
        threshold.front = atoi(argv[2]);
        threshold.right = atoi(argv[3]);
    }
    else if (!strcmp(argv[0], "ps")) {
        printf("Sensors settings:\n"
               "thresh: %ld %ld %ld\n",
               threshold.left, threshold.front, threshold.right);
    }
    else
        return -2;

    return 0;
}

static void load(const uint8_t *buffer)
{
    memcpy(&threshold, buffer, sizeof(threshold));
}

static void save(uint8_t *buffer)
{
    memcpy(buffer, &threshold, sizeof(threshold));
}

static struct ModuleSettings settings = {
    .dataSize = sizeof(threshold),
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
