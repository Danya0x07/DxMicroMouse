#include "encoders.h"
#include <as5048.h>
#include "mcu.h"
#include <string.h>

struct EncoderCounts current, previous, delta;

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
    int32_t leftPrev = previous.left;
    int32_t rightPrev = previous.right;
    int32_t left = AS5048_GetAngleRaw(AS5048_Handle_LEFT) >> 2;
    int32_t right = AS5048_GetAngleRaw(AS5048_Handle_RIGHT) >> 2;

    int32_t diff = left - leftPrev;
    if (diff >= 0x07FF || diff <= -0x07FF) {
        if (diff < 0)
            diff = (int32_t)0x0FFF - leftPrev + left;
        else
            diff = -((int32_t)0x0FFF - left + leftPrev);
    }
    current.left += diff;
    delta.left = diff;

    diff = right - rightPrev;
    if (diff >= 0x07FF || diff <= -0x07FF) {
        if (diff < 0)
            diff = (int32_t)0x0FFF - rightPrev + right;
        else
            diff = -((int32_t)0x0FFF - right + rightPrev);
    }
    current.right -= diff; // inverse diff for right encoder due to the way pcb is mounted
    delta.right = -diff;

    previous.left = left;
    previous.right = right;
}

void Encoders_Reset(void)
{
    AS5048_SetZero(AS5048_Handle_LEFT, 0);
    AS5048_SetZero(AS5048_Handle_RIGHT, 0);
    AS5048_SetZero(AS5048_Handle_LEFT, AS5048_GetAngleRaw(AS5048_Handle_LEFT));
    AS5048_SetZero(AS5048_Handle_RIGHT, AS5048_GetAngleRaw(AS5048_Handle_RIGHT));
    current.left = current.right = previous.left = previous.right = delta.left = delta.right = 0;
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

static void WriteTelemetry(char out[TELEMETRY_STRING_SIZE])
{
    struct EncoderCounts c, d;
    Encoders_GetCounts(&c);
    Encoders_GetDelta(&d);

    snprintf(out, TELEMETRY_STRING_SIZE,
            "L:%-10ld\tR:%-10ld\tdL:%-10ld\tdR:%-10ld\n", c.left, c.right, d.left, d.right);
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

static struct ModuleTelemetry telemetry = {
    .interval = 350,
    .write = WriteTelemetry
};

struct Module Encoders_module = {
    .name = "encoders",
    .execute = execute,
    .telemetry = &telemetry
};