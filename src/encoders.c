#include "encoders.h"
#include <as5048.h>
#include "telemetry.h"
#include "mcu.h"
#include <string.h>

static volatile int32_t currentCountsLeft, currentCountsRight;
static volatile int32_t prevCountsLeft, prevCountsRight;
static volatile int32_t deltaCountsLeft, deltaCountsRight;

int Encoders_Init(void)
{
    int retcode = 0;
    struct AS5048_DiagnosticsData diagnosticsDataLeft, diagnosticsDataRight;

    SysTick_DisableInterrupt();

    // To clear initial errors
    (void)AS5048_GetErrors(AS5048_Handle_LEFT);
    (void)AS5048_GetErrors(AS5048_Handle_RIGHT);

    for (int i = 0; i < 5; i++) {
        AS5048_GetDiagnosticsData(AS5048_Handle_LEFT, &diagnosticsDataLeft);
        AS5048_GetDiagnosticsData(AS5048_Handle_RIGHT, &diagnosticsDataRight);
        if (diagnosticsDataLeft.offsetCompensationFinished && diagnosticsDataRight.offsetCompensationFinished)
            break;
        Micros_Wait(10000);
    }
    if (!diagnosticsDataLeft.offsetCompensationFinished || !diagnosticsDataRight.offsetCompensationFinished) {
        printf("AS5048 diagnostics problem\n");
        printf("\tAS5048 Left:\ncompHigh: %d\ncompLow: %d\ncof: %d\nocf: %d\nagc: %d\n",
            diagnosticsDataLeft.compHigh,
            diagnosticsDataLeft.compLow,
            diagnosticsDataLeft.cordicOverflow,
            diagnosticsDataLeft.offsetCompensationFinished,
            diagnosticsDataLeft.agc
        );
        printf("\tAS5048 Right:\ncompHigh: %d\ncompLow: %d\ncof: %d\nocf: %d\nagc: %d\n",
            diagnosticsDataRight.compHigh,
            diagnosticsDataRight.compLow,
            diagnosticsDataRight.cordicOverflow,
            diagnosticsDataRight.offsetCompensationFinished,
            diagnosticsDataRight.agc
        );
        retcode = -1;
    }

    union AS5048_Errors errorsLeft = AS5048_GetErrors(AS5048_Handle_LEFT);
    union AS5048_Errors errorsRight = AS5048_GetErrors(AS5048_Handle_RIGHT);
    if (errorsLeft.status != 0 || errorsRight.status != 0) {
        printf("AS5048 errors during initialization\n");
        printf("\tAS5048 Left:\nparity: %d\ncommand: %d\nframing: %d\n",
            errorsLeft.parity,
            errorsLeft.command,
            errorsLeft.framing
        );
        printf("\tAS5048 Right:\nparity: %d\ncommand: %d\nframing: %d\n",
            errorsRight.parity,
            errorsRight.command,
            errorsRight.framing
        );
    }

    uint16_t magnitudeLeft = AS5048_GetMagnitudeRaw(AS5048_Handle_LEFT);
    uint16_t magnitudeRight = AS5048_GetMagnitudeRaw(AS5048_Handle_RIGHT);
    printf("Magnetic field magnitude:\nL:%d\tR:%d\n", magnitudeLeft, magnitudeRight);

    Encoders_Reset();
    SysTick_EnableInterrupt();

    return retcode;
}

void Encoders_Update(void)
{
    int32_t leftPrev = prevCountsLeft;
    int32_t rightPrev = prevCountsRight;
    int32_t left = AS5048_GetAngleRaw(AS5048_Handle_LEFT) >> 2;
    int32_t right = AS5048_GetAngleRaw(AS5048_Handle_RIGHT) >> 2;

    int32_t delta = left - leftPrev;
    if (delta >= 0x07FF || delta <= -0x07FF) {
        if (delta < 0)
            delta = (int32_t)0x0FFF - leftPrev + left;
        else
            delta = -((int32_t)0x0FFF - left + leftPrev);
    }
    currentCountsLeft += delta;
    deltaCountsLeft = delta;

    delta = right - rightPrev;
    if (delta >= 0x07FF || delta <= -0x07FF) {
        if (delta < 0)
            delta = (int32_t)0x0FFF - rightPrev + right;
        else
            delta = -((int32_t)0x0FFF - right + rightPrev);
    }
    currentCountsRight -= delta; // inverse delta for right encoder due to the way pcb is mounted
    deltaCountsRight = -delta;

    prevCountsLeft = left;
    prevCountsRight = right;
}

void Encoders_Reset(void)
{
    AS5048_SetZero(AS5048_Handle_LEFT, 0);
    AS5048_SetZero(AS5048_Handle_RIGHT, 0);
    AS5048_SetZero(AS5048_Handle_LEFT, AS5048_GetAngleRaw(AS5048_Handle_LEFT));
    AS5048_SetZero(AS5048_Handle_RIGHT, AS5048_GetAngleRaw(AS5048_Handle_RIGHT));
    currentCountsLeft = currentCountsRight = prevCountsLeft = prevCountsRight = deltaCountsLeft = deltaCountsRight = 0;
}

void Encoders_GetCounts(int32_t *left, int32_t *right)
{
    SysTick_DisableInterrupt();
    *left = currentCountsLeft;
    *right = currentCountsRight;
    SysTick_EnableInterrupt();
}

void Encoders_GetDelta(int32_t *left, int32_t *right)
{
    SysTick_DisableInterrupt();
    *left = deltaCountsLeft;
    *right = deltaCountsRight;
    SysTick_EnableInterrupt();
}

static void WriteTelemetry(char out[TELEMETRY_STRING_SIZE])
{
    int32_t left, right, deltaLeft, deltaRight;
    Encoders_GetCounts(&left, &right);
    Encoders_GetDelta(&deltaLeft, &deltaRight);

    snprintf(out, TELEMETRY_STRING_SIZE,
            "L:%-10ld\tR:%-10ld\tdL:%-10ld\tdR:%-10ld\n", left, right, deltaLeft, deltaRight);
}

static int execute(int argc, char *argv[])
{
    if (argc < 1)
        return -1;

    if (!strcmp(argv[0], "rst")) {
        Encoders_Reset();
    }
    return 0;
}

static struct TelemetryControlBlock telemetryControlBlock = {
    .interval = 350,
    .write = WriteTelemetry
};

struct Module Encoders_module = {
    .name = "encoders",
    .execute = execute,
    .telemetry = &telemetryControlBlock
};