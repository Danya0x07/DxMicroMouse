#include "odometry.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define COUNTS_TO_TRANSITION    6198

static struct Odometry {
    int distance;
    int angle;
} prediction;

static int64_t angleInmLsb;
static int32_t distanceInCounts;

static int32_t coeffAlpha = 0;

void Odometry_Reset()
{
    prediction = (struct Odometry){0};
    angleInmLsb = 0;
    distanceInCounts = 0;
}

void Odometry_SetReckon(int distanceInMm, int angleInDeg)
{
    distanceInCounts = (int32_t)distanceInMm * COUNTS_PER_MM;
    angleInmLsb = ((int64_t)angleInDeg << 15) / 2;
}

void Odometry_UpdateReckon(int32_t transInCounts, int32_t rotInMimuUnits)
{
    angleInmLsb += rotInMimuUnits;
    distanceInCounts += transInCounts;
}

void Odometry_GetReckon(int *distanceInMm, int *angleInDeg)
{
    *distanceInMm = distanceInCounts / COUNTS_PER_MM;
    *angleInDeg = NormalizeAngleDegrees(angleInmLsb * 2 / 32768);
}

void Odometry_SnapReckon(void)
{
    int traversedCells = distanceInCounts / COUNTS_PER_CELL;
    distanceInCounts = (int32_t)COUNTS_PER_CELL * traversedCells + COUNTS_TO_TRANSITION;
}

void Odometry_SetPrediction(int distanceInMm, int angleInDeg)
{
    prediction.distance = distanceInMm;
    prediction.angle = angleInDeg;
}

void Odometry_UpdatePrediction(int transInMm, int rotInDeg)
{
    prediction.distance += transInMm;
    prediction.angle += rotInDeg;
}

void Odometry_GetPrediction(int *distanceInMm, int *angleInDeg)
{
    *distanceInMm = prediction.distance;
    *angleInDeg = prediction.angle;
}

void Odometry_GetFusion(int *distanceInMm, int *angleInDeg)
{
    int reckonDistance, reckonAngle;
    int32_t fusionDistance, fusionAngle;

    Odometry_GetReckon(&reckonDistance, &reckonAngle);

    fusionDistance = ((int32_t)coeffAlpha * prediction.distance + (1000 - coeffAlpha) * reckonDistance) / 100;
    fusionDistance += fusionDistance > 0 ? 5 : -5;
    fusionDistance /= 10;

    fusionAngle = (coeffAlpha * prediction.angle + (1000 - coeffAlpha) * reckonAngle) / 100;
    fusionAngle += fusionAngle > 0 ? 5 : -5;
    fusionAngle /= 10;

    *distanceInMm = fusionDistance;
    *angleInDeg = fusionAngle;
}

static int execute(int argc, char *argv[])
{
    if (argc < 1)
        return -1;

    if (!strcmp(argv[0], "rst")) {
        Odometry_Reset();
    }
    else if (!strcmp(argv[0], "a")) {
        if (argc != 2)
            return -1;
        coeffAlpha = atoi(argv[1]);
    }
    else if (!strcmp(argv[0], "ps")) {
        printf("Odometry settings:\n"
               "cA: %ld\n",
               coeffAlpha);
    }
    else
        return -2;

    return 0;
}

static void WriteTelemetry(char out[TELEMETRY_STRING_SIZE])
{
    int distance, angle;

    Odometry_GetFusion(&distance, &angle);
    snprintf(out, TELEMETRY_STRING_SIZE, "d: %d\tang: %d\n", distance, angle);
}

static struct ModuleTelemetry telemetry = {
    .interval = 250,
    .write = WriteTelemetry
};

static void load(const uint8_t *buffer)
{
    memcpy(&coeffAlpha, buffer, sizeof(coeffAlpha));
}

static void save(uint8_t *buffer)
{
    memcpy(buffer, &coeffAlpha, sizeof(coeffAlpha));
}

static struct ModuleSettings settings = {
    .dataSize = sizeof(coeffAlpha),
    .load = load,
    .save = save
};

struct Module Odometry_module = {
    .name = "odom",
    .execute = execute,
    .telemetry = &telemetry,
    .settings = &settings
};