#ifndef _INC_MODULE_H
#define _INC_MODULE_H

#include "mcu.h"

#define TELEMETRY_STRING_SIZE   80
#define TELEMETRY_DEFAULT_INTERVAL  200

#define SETTINGS_BUFFER_SIZE 1024

struct ModuleTelemetry {
    uint32_t _lastTime;
    const uint32_t interval;
    bool enabled;
    void (*write)(char [TELEMETRY_STRING_SIZE]);
};

struct ModuleSettings {
    const uint32_t dataSize;
    void (*const load)(const uint8_t *buffer);
    void (*const save)(uint8_t *buffer);
};

struct Module {
    const char *const name;
    int (*const execute)(int argc, char *argv[]);
    struct ModuleTelemetry *const telemetry;
    struct ModuleSettings *const settings;
};

extern struct Module *modules[]; // Should be declared in main.c and end with NULL.

void Modules_SendTelemetry(void);
void Modules_LoadSettings(void);
void Modules_SaveSettings(void);
struct Module *Module_FindByName(const char *name);
void Modules_Print(void);

#endif // _INC_MODULE_H