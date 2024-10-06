#ifndef _INC_TELEMETRY_H
#define _INC_TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define TELEMETRY_STRING_SIZE   80
#define TELEMETRY_DEFAULT_INTERVAL  200

struct TelemetryControlBlock {
    uint32_t _lastTime;
    const uint16_t interval;
    bool enabled;
    void (*write)(char [TELEMETRY_STRING_SIZE]);
};

void Telemetry_Send(struct TelemetryControlBlock *ctlb);

#endif // _INC_TELEMETRY_H