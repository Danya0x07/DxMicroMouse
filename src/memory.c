#include "memory.h"
#include "uart.h"
#include "utils.h"
#include <m95256.h>

#include <stdlib.h>
#include <string.h>

#define MEMORY_START_ADDRESS    0x100
#define NUM_RETRIES 3

#define SHADOW_SIZE (MEMORY_MAX_BUFFER_SIZE + 2)
static uint8_t shadowBuffer[SHADOW_SIZE];

int Memory_Init(void)
{
    const char *const msgs[3] = {
        "locked",
        "timed out",
        "fucked itself somehow"
    };

    M95_Error_t err = M95256_Init();
    if (err != M95_Error_NONE) {
        printf("Memory %s\n", msgs[err - 1]);
        return -1;
    }
    return 0;
}

int Memory_SaveBuffer(const uint8_t *buffer, unsigned size)
{
    if (size > MEMORY_MAX_BUFFER_SIZE)
        return -1;

    memcpy(shadowBuffer, buffer, size);
    *((uint16_t *)&shadowBuffer[SHADOW_SIZE - 2]) = Crc16(buffer, size);

    M95_Error_t err;
    for (int i = 0; i < NUM_RETRIES; i++) {
        err = M95256_WriteArray(MEMORY_START_ADDRESS, shadowBuffer, SHADOW_SIZE);
        if (err == M95_Error_NONE)
            break;
    }

    if (err != M95_Error_NONE) {
        printf("M95 error on write: %d\n", err);
        return -2;
    }
    return 0;
}

int Memory_LoadBuffer(uint8_t *buffer, unsigned size)
{
    if (size > MEMORY_MAX_BUFFER_SIZE)
        return -1;

    memset(shadowBuffer, 0, SHADOW_SIZE);

    M95_Error_t err;
    for (int i = 0; i < NUM_RETRIES; i++) {
        err = M95256_ReadArray(MEMORY_START_ADDRESS, shadowBuffer, SHADOW_SIZE);
        if (err == M95_Error_NONE)
            break;
    }

    if (err != M95_Error_NONE) {
        printf("M95 error on read: %d\n", err);
        return -2;
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
    M95_Error_t err;

    if (argc != 3)
        goto invalid_call;

    uint16_t address = atoi(argv[0]);
    uint16_t len = 0;

    if (argv[1][0] == 'r') {
        len = atoi(argv[2]);
        if (address + len - 1 > 32767)
            goto out_of_mem;

        char buff[128] = {0};
        if (len > sizeof(buff))
            len = sizeof(buff);
        err = M95256_ReadArray(address, (uint8_t *)buff, len);
        for (uint_fast8_t i = 0; i < len; i++)
            UART_SendChar(buff[i]);
    }
    else if (argv[1][0] == 'w') {
        len = strlen(argv[2]);
        if (address + len - 1 > 32767)
            goto out_of_mem;

        err = M95256_WriteArray(address, (uint8_t *)argv[2], len);
    }
    else {
        goto invalid_call;
    }

    if (err != M95_Error_NONE)
        goto bad_status;

    printf("\n\nAccessed %d bytes\n", len);
    return 0;

invalid_call:
    printf("Usage: mem ADDR r|w RLEN|WDATA\n");
    return -1;
out_of_mem:
    printf("Address range: 0-32767\n");
    return -2;
bad_status:
    printf("Mem errcode: %d\n", err);
    return -3;
}

struct Module Memory_module = {
    .name = "mem",
    .execute = execute
};