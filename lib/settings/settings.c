#include "settings.h"
#include "settings_port.h"

#include <string.h>

#define AVAILABLE_SIZE  (MEMORY_SIZE - 2)

#ifndef FOR_EACH_PP
#   define FOR_EACH_PP(parr)   for (void **pp = (void **)(parr); *pp; pp++)
#endif

int Settings_Load(const struct Settings *const settings[])
{
    int retcode;
    struct Settings *s;
    uint8_t *ptr = memory;

    memset(memory, 0, MEMORY_SIZE);
    if ((retcode = LoadBuffer()) != SETTINGS_OK)
        return retcode;

    if (CRC16(memory, AVAILABLE_SIZE) != *((uint16_t *)&memory[MEMORY_SIZE - 2]))
        return SETTINGS_ECRC;

    FOR_EACH_PP(settings) {
        s = *pp;
        if (ptr - memory > AVAILABLE_SIZE)
            return SETTINGS_ESIZE;
        s->load(ptr);
        ptr += s->dataSize;
    }

    return SETTINGS_OK;
}

int Settings_Save(const struct Settings *const settings[])
{
    struct Settings *s;
    uint8_t *ptr = memory;

    memset(memory, 0, MEMORY_SIZE);

    FOR_EACH_PP(settings) {
        s = *pp;
        if (ptr - memory > AVAILABLE_SIZE)
            return SETTINGS_ESIZE;
        s->save(ptr);
        ptr += s->dataSize;
    }
    *((uint16_t *)&memory[MEMORY_SIZE - 2]) = CRC16(memory, AVAILABLE_SIZE);

    return SaveBuffer();
}

void Settings_Export(uint8_t buffer[MEMORY_SIZE])
{
    memcpy(buffer, memory, MEMORY_SIZE);
}
