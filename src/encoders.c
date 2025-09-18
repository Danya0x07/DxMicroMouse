#include "encoders.h"
#include "mcu.h"
#include <as5048.h>
#include <stdio.h>
#include <string.h>

struct AS5048_Device encoderLeft = {
    .bus = SPI_Bus_PH1,
    .cs = {
        .port = ENCL_CS_GPIO,
        .pin = ENCL_CS_PIN
    }
};

struct AS5048_Device encoderRight = {
    .bus = SPI_Bus_PH1,
    .cs = {
        .port = ENCR_CS_GPIO,
        .pin = ENCR_CS_PIN
    }
};

struct EncoderCounts current, previous, delta;

int Encoders_Init(void)
{
    int retcode = 0;
    struct AS5048_DiagnosticsData diagnosticsDataLeft, diagnosticsDataRight;

    SysTick_DisableInterrupt();

    // To clear initial errors
    (void)AS5048_GetErrors(&encoderLeft);
    (void)AS5048_GetErrors(&encoderRight);

    for (int i = 0; i < 5; i++) {
        AS5048_GetDiagnosticsData(&encoderLeft, &diagnosticsDataLeft);
        AS5048_GetDiagnosticsData(&encoderRight, &diagnosticsDataRight);
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

    union AS5048_Errors errorsLeft = AS5048_GetErrors(&encoderLeft);
    union AS5048_Errors errorsRight = AS5048_GetErrors(&encoderRight);
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

    uint16_t magnitudeLeft = AS5048_GetMagnitudeRaw(&encoderLeft);
    uint16_t magnitudeRight = AS5048_GetMagnitudeRaw(&encoderRight);
    printf("Magnetic field magnitude:\nL:%d\tR:%d\n", magnitudeLeft, magnitudeRight);

    Encoders_Reset();
    SysTick_EnableInterrupt();

    return retcode;
}

void Encoders_Update(void)
{
    int32_t leftPrev = previous.left;
    int32_t rightPrev = previous.right;
    int32_t left = AS5048_GetAngleRaw(&encoderLeft) >> 2;
    int32_t right = AS5048_GetAngleRaw(&encoderRight) >> 2;

    int32_t diff = left - leftPrev;
    if (diff >= 0x07FF || diff <= -0x07FF) {
        if (diff < 0)
            diff = (int32_t)0x0FFF + diff;
        else
            diff = -((int32_t)0x0FFF - diff);
    }
    current.left += diff;
    delta.left = diff;

    diff = right - rightPrev;
    if (diff >= 0x07FF || diff <= -0x07FF) {
        if (diff < 0)
            diff = (int32_t)0x0FFF + diff;
        else
            diff = -((int32_t)0x0FFF - diff);
    }
    current.right -= diff; // inverse diff for right encoder due to the way pcb is mounted
    delta.right = -diff;

    previous.left = left;
    previous.right = right;
}

void Encoders_Reset(void)
{
    AS5048_SetZero(&encoderLeft, 0);
    AS5048_SetZero(&encoderRight, 0);
    AS5048_SetZero(&encoderLeft, AS5048_GetAngleRaw(&encoderLeft));
    AS5048_SetZero(&encoderRight, AS5048_GetAngleRaw(&encoderRight));
    current = previous = delta = (struct EncoderCounts){0};
}

void Encoders_GetCounts(struct EncoderCounts *c)
{
    SysTick_DisableInterrupt();
    *c = current;
    SysTick_EnableInterrupt();
}

void Encoders_GetDelta(struct EncoderCounts *d)
{
    SysTick_DisableInterrupt();
    *d = delta;
    SysTick_EnableInterrupt();
}

static void PrintTelemetry(void)
{
    struct EncoderCounts c, d;
    Encoders_GetCounts(&c);
    Encoders_GetDelta(&d);

    printf("L:%-10ld\tR:%-10ld\tdL:%-10ld\tdR:%-10ld\n", c.left, c.right, d.left, d.right);
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

struct SchedulerTask TASK_TmEncoders = {
    .execute = PrintTelemetry,
    .period = 350
};

const struct ShellCommand CMD_Encoders = {
    .name = "encs",
    .execute = execute
};