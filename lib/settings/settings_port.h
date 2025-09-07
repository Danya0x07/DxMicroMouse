#ifndef _INC_SETTINGS_PORT_H
#define _INC_SETTINGS_PORT_H

#include "settings.h"
#include "mcu.h"
#include "utils.h"

#define START_ADDRESS   (FLASH_BASE + 0xFFFF - MEMORY_SIZE + 1)

static uint8_t memory[MEMORY_SIZE];

static int SaveBuffer(void)
{
    int retcode = SETTINGS_OK;

    SysTick_DisableInterrupt();

    FLASH_Status status = FLASH_ROM_ERASE(START_ADDRESS, MEMORY_SIZE);

    if (status == FLASH_COMPLETE) {
        status = FLASH_ROM_WRITE(START_ADDRESS, (uint32_t *)memory, MEMORY_SIZE);
    }

    if (status != FLASH_COMPLETE) {
        retcode = SETTINGS_EWRITE;
    }

    SysTick_EnableInterrupt();

    return retcode;
}

static int LoadBuffer(void)
{
    SysTick_DisableInterrupt();

    volatile uint8_t *m = (volatile uint8_t *)START_ADDRESS;

    for (int i = 0; i < MEMORY_SIZE; i++) {
        memory[i] = *m++;
    }
    SysTick_EnableInterrupt();

    return SETTINGS_OK;
}

#define CRC16(data, len)    Crc16(data, len)

#endif // _INC_SETTINGS_PORT_H