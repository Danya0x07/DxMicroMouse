#include "odometry.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static float currentX, currentY, currentAng;

void Odometry_Reset()
{
    currentX = currentY = currentAng = 0;
}

void Odometry_Update(float deltaPos, float deltaAng)
{
    float angRadians = currentAng * M_PI / 180;

    currentY += deltaPos * cos(angRadians);
    currentX += deltaPos * -sin(angRadians);
    currentAng += deltaAng;
    if (currentAng > 180)
        currentAng -= 360;
    if (currentAng < -180)
        currentAng += 360;
}

void Odometry_Get(float *x, float *y, float *ang)
{
    *x = currentX;
    *y = currentY;
    *ang = currentAng;
}

static int execute(int argc, char *argv[])
{
    if (argc < 1)
        return -1;

    if (!strcmp(argv[0], "reset")) {
        Odometry_Reset();
    }
    return 0;
}

static void WriteTelemetry(char out[TELEMETRY_STRING_SIZE])
{
    snprintf(out, TELEMETRY_STRING_SIZE, "x:%d\ty:%d\tAng:%d\n",
            (int16_t)currentX, (int16_t)currentY, (int16_t)currentAng);
}

static struct TelemetryControlBlock telemetryControlBlock = {
    .interval = 250,
    .write = WriteTelemetry
};

struct Module Odometry_module = {
    .name = "odom",
    .execute = execute,
    .telemetry = &telemetryControlBlock
};