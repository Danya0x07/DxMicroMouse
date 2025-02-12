#include "odometry.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

struct Odometry {
    int64_t lsbAng;
    int64_t countsX, countsY;
    int32_t degAng;
    int32_t vTransInCountsPerS, vRotInImuUnitsPerS;
};

static struct Odometry odometry;

void Odometry_Reset()
{
    odometry = (struct Odometry){0};
}

void Odometry_Update(int32_t transInCounts, int32_t deltaAngInMimuUnits)
{
    odometry.vTransInCountsPerS = transInCounts * 1000;
    odometry.vRotInImuUnitsPerS = deltaAngInMimuUnits; /* / 1ms */
    odometry.lsbAng += deltaAngInMimuUnits;

    odometry.degAng = ((odometry.lsbAng * 2000 / 32768) / 100 + 5) / 10;
    odometry.degAng = NormalizeAngleDegrees(odometry.degAng);

    odometry.countsY += transInCounts * Cos100000(odometry.degAng);
    odometry.countsX += transInCounts * -Sin100000(odometry.degAng);
}

void Odometry_GetPosition(int32_t *mmX, int32_t *mmY, int32_t *degAng)
{
    *mmX = odometry.countsX / (COUNTS_PER_MM * 100000 + 2500);
    *mmY = odometry.countsY / (COUNTS_PER_MM * 100000 + 2500);
    *degAng = odometry.degAng;
}

void Odometry_GetVelocity(int16_t *vTransInMmPerS, int16_t *vRotInDegPerS)
{
    *vTransInMmPerS = odometry.vTransInCountsPerS / COUNTS_PER_MM;
    *vRotInDegPerS = (odometry.vRotInImuUnitsPerS * 2000 / 16384 + 1) / 2;;
}

static int execute(int argc, char *argv[])
{
    if (argc < 1)
        return -1;

    if (!strcmp(argv[0], "rst")) {
        Odometry_Reset();
    }
    return 0;
}

static void WriteTelemetry(char out[TELEMETRY_STRING_SIZE])
{
    int32_t mmX, mmY, degAng;
    int16_t vTransInMmPerS, vRotInDegPerS;

    Odometry_GetPosition(&mmX, &mmY, &degAng);
    Odometry_GetVelocity(&vTransInMmPerS, &vRotInDegPerS);
    snprintf(out, TELEMETRY_STRING_SIZE, "x:%ld\ty:%ld\tAng:%ld\tv:%d\tw:%d\n",
            mmX, mmY, degAng, vTransInMmPerS, vRotInDegPerS);
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