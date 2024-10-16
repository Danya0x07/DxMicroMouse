#include "memory.h"
#include <m95256.h>
#include <stdlib.h>
#include <string.h>
#include "uart.h"

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
    .execute = execute,
    .telemetry = NULL
};