#include "sensors.h"
#include "emitters.h"
#include "receivers.h"

static volatile uint16_t currentValues[5];

void Sensors_Update(void)
{
    uint16_t rawValues[5] = {0};

    Emitters_LeftFrontOn();
    rawValues[ReceiverChannel_LeftFront] = Receivers_ReadChannel(ReceiverChannel_LeftFront);
    Emitters_LeftFrontOff();

    Emitters_LeftSideOn();
    rawValues[ReceiverChannel_LeftSide] = Receivers_ReadChannel(ReceiverChannel_LeftSide);
    Emitters_LeftSideOff();

    Emitters_RightSideOn();
    rawValues[ReceiverChannel_RightSide] = Receivers_ReadChannel(ReceiverChannel_RightSide);
    Emitters_RightSideOff();

    Emitters_RightFrontOn();
    rawValues[ReceiverChannel_RightFront] = Receivers_ReadChannel(ReceiverChannel_RightFront);
    Emitters_RightFrontOff();

    /*Emitters_FrontOn();
    rawValues[ReceiverChannel_Front] = Receivers_ReadChannel(ReceiverChannel_Front);
    Emitters_FrontOff();*/

    // processing rawValues

    memcpy_n2v(currentValues, rawValues, sizeof(currentValues));
}

void Sensors_ReadToBuffer(uint16_t buffer[5])
{
    memcpy_v2n(buffer, currentValues, sizeof(currentValues));
}

static void WriteTelemetry(char out[TELEMETRY_STRING_SIZE])
{
    uint16_t values[5];

    Sensors_ReadToBuffer(values);
    snprintf(out, TELEMETRY_STRING_SIZE,
            "Sensors:\tLF:%d\tLS:%d\tF:%d\tRS:%d\tRF:%d\n",
            values[0], values[1], values[4], values[2], values[3]);
}

static struct TelemetryControlBlock telemetryControlBlock = {
    .interval = 300,
    .write = WriteTelemetry
};

struct Module Sensors_module = {
    .name = "sensors",
    .execute = NULL,
    .telemetry = &telemetryControlBlock
};
