#include "module.h"
#include "uart.h"
#include "memory.h"

#include <string.h>

#define MEMORY_BUFFER_SIZE  1000

#define FOR_EACH_MODULE(addr)  for (struct Module **addr = &modules[0]; *(addr); addr++)

static uint8_t memoryBuffer[MEMORY_BUFFER_SIZE];

void Modules_SendTelemetry(void)
{
    FOR_EACH_MODULE(m) {
        struct ModuleTelemetry *telemetry = (*m)->telemetry;

        if (telemetry) {
            if (telemetry->enabled == false)
                continue;

            if (Millis_Get() - telemetry->_lastTime >= telemetry->interval) {
                static char telemetryString[TELEMETRY_STRING_SIZE];

                telemetry->write(telemetryString);
                UART_SendString(telemetryString);
                telemetry->_lastTime = Millis_Get();
            }
        }
    }
}

void Modules_LoadSettings(void)
{
    struct ModuleSettings *settings;
    uint8_t *ptr = memoryBuffer;

    if (Memory_LoadBuffer(memoryBuffer, MEMORY_BUFFER_SIZE) < 0) {
        UART_SendString("Failed to load settings\n");
        return;
    }

    FOR_EACH_MODULE(m) {
        settings = (*m)->settings;

        if (settings) {
            settings->load(ptr);
            ptr += settings->dataSize;
        }
    }
}

void Modules_SaveSettings(void)
{
    struct ModuleSettings *settings;
    uint8_t *ptr = memoryBuffer;

    FOR_EACH_MODULE(m) {
        settings = (*m)->settings;

        if (settings) {
            settings->load(ptr);
            ptr += settings->dataSize;
        }
    }
    printf("Saving %d bytes to EEPROM\n", ptr - memoryBuffer);

    if (Memory_SaveBuffer(memoryBuffer, MEMORY_BUFFER_SIZE) < 0) {
        printf("Failed to save settings\n");
    }
}

struct Module *Module_FindByName(const char *name)
{
    struct Module *m;

    FOR_EACH_MODULE(addr) {
        m = *addr;
        if (!strcmp(name, m->name))
            return m;
    }
    return NULL;
}

void Modules_Print(void)
{
    struct Module *m;

    UART_SendString("Modules:\n");
    FOR_EACH_MODULE(addr) {
        m = *addr;
        UART_SendString(m->name);
        if (m->telemetry)
            UART_SendChar('*');
        UART_SendChar(' ');
    }
    UART_SendChar('\n');
}
