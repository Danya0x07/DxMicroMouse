#include "battery.h"
#include "mcu.h"
#include <stdio.h>

#define BATTERY_CHECK_PERIOD    1000

static const char *STATUS_TXT[] = {
    "DEAD", "LOW", "MEDIUM", "HIGH", "FULL"
};

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
     * 8.0 3680
     * 7.4 3391
     * 7.1 3292
     * 6.4 2963
     * 5.8 2660
     */
    static const uint16_t thresholds[5] = {
        [BatteryStatus_DEAD]    = 2660,
        [BatteryStatus_LOW]     = 2963,
        [BatteryStatus_MEDIUM]  = 3292,
        [BatteryStatus_HIGH]    = 3391,
        [BatteryStatus_FULL]    = 3680
    };

    unsigned level = batteryLevel;

    for (BatteryStatus bs = BatteryStatus_DEAD; bs <= BatteryStatus_FULL; bs++) {
        if (level <= thresholds[bs])
            return bs;
    }

    return BatteryStatus_FULL;
}

const char *Battery_StatusToStr(BatteryStatus status)
{
    return STATUS_TXT[status];
}

static void PrintTelemetry(void)
{
    printf("BATTERY:%d => %s\n", batteryLevel, Battery_StatusToStr(Battery_GetStatus()));
}

struct SchedulerTask TASK_TmBattery = {
    .execute = PrintTelemetry,
    .period = 1000
};
