#ifndef _INC_MODULE_H
#define _INC_MODULE_H

#include "telemetry.h"

struct Module {
    const char *const name;
    int (*const execute)(int argc, char *argv[]);
    struct TelemetryControlBlock *const telemetry;
};

extern struct Module *modules[]; // Should be declared in main.c and end with NULL.

#define FOR_EACH_MODULE(addr)  for (struct Module **addr = &modules[0]; *(addr); addr++)

#endif // _INC_MODULE_H