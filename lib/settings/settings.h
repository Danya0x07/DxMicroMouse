#ifndef _INC_SETTINGS_H
#define _INC_SETTINGS_H

#include <stdint.h>
#include "settings_conf.h"

struct Settings {
    unsigned dataSize;
    void (*load)(const uint8_t *buffer);
    void (*save)(uint8_t *buffer);
};

#define SETTINGS_OK 0
#define SETTINGS_EREAD  (-1)
#define SETTINGS_ECRC   (-2)
#define SETTINGS_EWRITE (-3)
#define SETTINGS_ESIZE  (-4)

int Settings_Load(const struct Settings *const settings[]);
int Settings_Save(const struct Settings *const settings[]);
void Settings_Export(uint8_t buffer[MEMORY_SIZE]);

#endif // _INC_SETTINGS_H