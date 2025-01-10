#include "sensors.h"
#include "emitters.h"
#include "receivers.h"
#include <string.h>

static volatile uint16_t currentValues[5];

static void update(void)
{
    uint16_t rawValues[5] = {0};
    uint16_t frontLightValue, frontDarkValue;

    Emitters_LeftFrontOn();
    Emitters_RightFrontOn();
    Micros_Wait(60);
    rawValues[ReceiverChannel_LeftFront] = Receivers_ReadChannel(ReceiverChannel_LeftFront);
    rawValues[ReceiverChannel_RightFront] = Receivers_ReadChannel(ReceiverChannel_RightFront);
    frontLightValue = Receivers_ReadChannel(ReceiverChannel_Front);
    Emitters_LeftFrontOff();
    Emitters_RightFrontOff();
    Micros_Wait(60);
    rawValues[ReceiverChannel_LeftFront] -= Receivers_ReadChannel(ReceiverChannel_LeftFront);
    rawValues[ReceiverChannel_RightFront] -= Receivers_ReadChannel(ReceiverChannel_RightFront);

    frontDarkValue = Receivers_ReadChannel(ReceiverChannel_Front);
    if (frontDarkValue > frontLightValue)
        frontDarkValue = frontLightValue;
    rawValues[ReceiverChannel_Front] = frontLightValue - frontDarkValue;

    Emitters_LeftSideOn();
    Emitters_RightSideOn();
    Micros_Wait(60);
    rawValues[ReceiverChannel_LeftSide] = Receivers_ReadChannel(ReceiverChannel_LeftSide);
    rawValues[ReceiverChannel_RightSide] = Receivers_ReadChannel(ReceiverChannel_RightSide);
    Emitters_LeftSideOff();
    Emitters_RightSideOff();
    Micros_Wait(60);
    rawValues[ReceiverChannel_LeftSide] -= Receivers_ReadChannel(ReceiverChannel_LeftSide);
    rawValues[ReceiverChannel_RightSide] -= Receivers_ReadChannel(ReceiverChannel_RightSide);

    // processing rawValues

    memcpy_n2v(currentValues, rawValues, sizeof(currentValues));
}

static void updateWithoutLightening(void)
{
    uint16_t rawValues[5] = {0};

    rawValues[ReceiverChannel_LeftFront] = Receivers_ReadChannel(ReceiverChannel_LeftFront);
    rawValues[ReceiverChannel_RightFront] = Receivers_ReadChannel(ReceiverChannel_RightFront);
    rawValues[ReceiverChannel_LeftSide] = Receivers_ReadChannel(ReceiverChannel_LeftSide);
    rawValues[ReceiverChannel_RightSide] = Receivers_ReadChannel(ReceiverChannel_RightSide);
    rawValues[ReceiverChannel_Front] = Receivers_ReadChannel(ReceiverChannel_Front);

    // processing rawValues

    memcpy_n2v(currentValues, rawValues, sizeof(currentValues));
}

void (*Sensors_Update)(void) = update;

void Sensors_SetLightening(FunctionalState state)
{
    Sensors_Update = state ? update : updateWithoutLightening;
}

void Sensors_ReadToBuffer(uint16_t buffer[5])
{
    SysTick_DisableInterrupt();
    memcpy_v2n(buffer, currentValues, sizeof(currentValues));
    SysTick_EnableInterrupt();
}

static void WriteTelemetry(char out[TELEMETRY_STRING_SIZE])
{
    uint16_t values[5];

    Sensors_ReadToBuffer(values);
    snprintf(out, TELEMETRY_STRING_SIZE,
            "LF:%-5d\tLS:%-5d\tF:%-5d\tRS:%-5d\tRF:%-5d\n",
            values[0], values[1], values[4], values[2], values[3]);
}

static int execute(int argc, char *argv[])
{
    if (argc != 1)
        return -1;

    if (!strcmp(argv[0], "lon"))
        Sensors_SetLightening(ENABLE);
    else if (!strcmp(argv[0], "loff"))
        Sensors_SetLightening(DISABLE);
    else
        return -2;

    return 0;
}

static struct TelemetryControlBlock telemetryControlBlock = {
    .interval = 300,
    .write = WriteTelemetry
};

struct Module Sensors_module = {
    .name = "sensors",
    .execute = execute,
    .telemetry = &telemetryControlBlock
};
