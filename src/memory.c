#include "memory.h"
#include "uart.h"
#include "utils.h"

#include <string.h>

#define SHADOW_SIZE (MEMORY_MAX_BUFFER_SIZE + 2)
#define MEMORY_START_ADDRESS    (FLASH_BASE + 0xFFFF - SHADOW_SIZE + 1)

static uint8_t shadowBuffer[SHADOW_SIZE];

int Memory_SaveBuffer(const uint8_t *buffer, unsigned size)
{
    if (size > MEMORY_MAX_BUFFER_SIZE)
        return -1;

    memcpy(shadowBuffer, buffer, size);
    *((uint16_t *)&shadowBuffer[SHADOW_SIZE - 2]) = Crc16(buffer, size);

    FLASH_Status status = FLASH_ROM_ERASE(MEMORY_START_ADDRESS, SHADOW_SIZE);

    if (status != FLASH_COMPLETE) {
        printf("Flash erase error: %d\n", status);
        return -2;
    }

    status = FLASH_ROM_WRITE(MEMORY_START_ADDRESS, (uint32_t *)shadowBuffer, SHADOW_SIZE);

    if (status != FLASH_COMPLETE) {
        printf("Flash write error: %d\n", status);
        return -2;
    }
    return 0;
}

int Memory_LoadBuffer(uint8_t *buffer, unsigned size)
{
    if (size > MEMORY_MAX_BUFFER_SIZE)
        return -1;

    memset(shadowBuffer, 0, SHADOW_SIZE);

    volatile uint8_t *memory = (volatile uint8_t *)MEMORY_START_ADDRESS;

    for (int i = 0; i < SHADOW_SIZE; i++) {
        shadowBuffer[i] = *memory++;
    }

    if (Crc16(shadowBuffer, size) != *((uint16_t *)&shadowBuffer[SHADOW_SIZE - 2])) {
        printf("CRC error\n");
        return -3;
    }

    memcpy(buffer, shadowBuffer, size);
    return 0;
}

static int execute(int argc, char *argv[])
{
    for (int i = 0; i < SHADOW_SIZE; i++) {
        UART_SendChar(shadowBuffer[i]);
    }
    return 0;
}

struct Module Memory_module = {
    .name = "mem",
    .execute = execute
};