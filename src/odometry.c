#include "odometry.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static struct Odometry {
    int32_t distance;
    int32_t angle;
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

void Odometry_SetReckon(int32_t distanceInMm, int32_t angleInDeg)
{
    distanceInCounts = distanceInMm * COUNTS_PER_MM;
    angleInmLsb = (angleInDeg << 15) / 2;
}

void Odometry_UpdateReckon(int32_t transInCounts, int32_t rotInMimuUnits)
{
    angleInmLsb += rotInMimuUnits;
    distanceInCounts += transInCounts;
}

void Odometry_GetReckon(int32_t *distanceInMm, int32_t *angleInDeg)
{
    *distanceInMm = distanceInCounts / COUNTS_PER_MM;
    *angleInDeg = NormalizeAngleDegrees(angleInmLsb * 2 / 32768);
}

void Odometry_SetPrediction(int32_t distanceInMm, int32_t angleInDeg)
{
    prediction.distance = distanceInMm;
    prediction.angle = angleInDeg;
}

void Odometry_UpdatePrediction(int32_t transInMm, int32_t rotInDeg)
{
    prediction.distance += transInMm;
    prediction.angle += rotInDeg;
}

void Odometry_GetPrediction(int32_t *distanceInMm, int32_t *angleInDeg)
{
    *distanceInMm = prediction.distance;
    *angleInDeg = prediction.angle;
}

void Odometry_GetFusion(int32_t *distanceInMm, int32_t *angleInDeg)
{
    int32_t reckonDistance, reckonAngle;
    int32_t fusionDistance, fusionAngle;

    Odometry_GetReckon(&reckonDistance, &reckonAngle);

    fusionDistance = (coeffAlpha * prediction.distance + (1000 - coeffAlpha) * reckonDistance) / 100;
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
    return 0;
}

static void WriteTelemetry(char out[TELEMETRY_STRING_SIZE])
{
    int32_t distance, angle;

    Odometry_GetFusion(&distance, &angle);
    snprintf(out, TELEMETRY_STRING_SIZE, "d: %ld\tang: %ld\n", distance, angle);
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