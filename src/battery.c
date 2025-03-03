#include "battery.h"
#include "mcu.h"
#include <stdlib.h>

#define BATTERY_CHECK_PERIOD    1000

static unsigned batteryLevel;

void Battery_Update(void)
{
    static uint32_t lastTime = 0;

    if (Millis_Get() - lastTime >= BATTERY_CHECK_PERIOD) {
        batteryLevel = ADC_Read(BATTERY_CH);
        lastTime = Millis_Get();
    }
}

BatteryStatus Battery_GetStatus(void)
{
    /*
     * 4.0 3900
     * 3.8 3800
     * 3.6 3550
     * 3.5 3450
     * 3.3 3300
     */
    static const unsigned thresholds[5] = {
        [BatteryStatus_DEAD]    = 3300,
        [BatteryStatus_LOW]     = 3450,
        [BatteryStatus_MEDIUM]  = 3550,
        [BatteryStatus_HIGH]    = 3800,
        [BatteryStatus_FULL]    = 3900
    };

    unsigned level = batteryLevel;

    for (BatteryStatus bs = BatteryStatus_DEAD; bs <= BatteryStatus_FULL; bs++) {
        if (level <= thresholds[bs])
            return bs;
    }

    return BatteryStatus_FULL;
}

static void WriteTelemetry(char out[TELEMETRY_STRING_SIZE])
{
    static const char *STATUS_TXT[] = {
        "DEAD", "LOW", "MEDIUM", "HIGH", "FULL"
    };

    snprintf(out, TELEMETRY_STRING_SIZE, "BATTERY:%d => %s\n", batteryLevel, STATUS_TXT[Battery_GetStatus()]);
}

static struct ModuleTelemetry telemetry = {
    .interval = 1000,
    .write = WriteTelemetry
};

struct Module Battery_module = {
    .name = "battery",
    .telemetry = &telemetry
};