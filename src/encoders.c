#include "encoders.h"
#include <stdio.h>
#include <as5048.h>
#include "telemetry.h"
#include "mcu.h"

static volatile struct Encoders_Data currentData = {0};
static volatile struct Encoders_Data prevData = {0};

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
    int32_t ticksLeftPrev = prevData.ticksLeft;
    int32_t ticksRightPrev = prevData.ticksRight;
    int32_t ticksLeft = AS5048_GetAngleRaw(AS5048_Handle_LEFT);
    int32_t ticksRight = AS5048_GetAngleRaw(AS5048_Handle_RIGHT);

    int32_t delta = ticksLeft - ticksLeftPrev;
    if (delta >= 0x1FFF || delta <= -0x1FFF) {
        if (delta < 0)
            delta = (int32_t)0x3FFF - ticksLeftPrev + ticksLeft;
        else
            delta = -((int32_t)0x3FFF - ticksLeft + ticksLeftPrev);
    }
    currentData.ticksLeft += delta;

    delta = ticksRight - ticksRightPrev;
    if (delta >= 0x1FFF || delta <= -0x1FFF) {
        if (delta < 0)
            delta = (int32_t)0x3FFF - ticksRightPrev + ticksRight;
        else
            delta = -((int32_t)0x3FFF - ticksRight + ticksRightPrev);
    }
    currentData.ticksRight -= delta; // inverse delta for right encoder due to the way pcb is mounted

    prevData.ticksLeft = ticksLeft;
    prevData.ticksRight = ticksRight;
}

void Encoders_Reset(void)
{
    AS5048_SetZero(AS5048_Handle_LEFT, 0);
    AS5048_SetZero(AS5048_Handle_RIGHT, 0);
    AS5048_SetZero(AS5048_Handle_LEFT, AS5048_GetAngleRaw(AS5048_Handle_LEFT));
    AS5048_SetZero(AS5048_Handle_RIGHT, AS5048_GetAngleRaw(AS5048_Handle_RIGHT));
    currentData = (struct Encoders_Data){0};
    prevData = (struct Encoders_Data){0};
}

void Encoders_GetData(struct Encoders_Data *data)
{
    SysTick_DisableInterrupt();
    memcpy_v2n(data, &currentData, sizeof(struct Encoders_Data));
    SysTick_EnableInterrupt();
}

static void WriteTelemetry(char out[TELEMETRY_STRING_SIZE])
{
    struct Encoders_Data encodersData;
    Encoders_GetData(&encodersData);

    snprintf(out, TELEMETRY_STRING_SIZE, "tL:%-10ld\ttR:%-10ld\n", encodersData.ticksLeft, encodersData.ticksRight);
}

static struct TelemetryControlBlock telemetryControlBlock = {
    .interval = 350,
    .write = WriteTelemetry
};

struct Module Encoders_module = {
    .name = "encoders",
    .execute = NULL,
    .telemetry = &telemetryControlBlock
};