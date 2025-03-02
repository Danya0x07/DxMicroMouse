#ifndef _INC_MEMORY_H
#define _INC_MEMORY_H

#include "module.h"

#define MEMORY_MAX_BUFFER_SIZE  1024

int Memory_Init(void);
int Memory_SaveBuffer(const uint8_t *buffer, unsigned size);
int Memory_LoadBuffer(uint8_t *buffer, unsigned size);

#define MEMORY_HOLD_TRANSACTION()   GPIO_ResetBits(MEM_HOLD_GPIO, MEM_HOLD_PIN)
#define MEMORY_UNHOLD_TRANSACTION() GPIO_SetBits(MEM_HOLD_GPIO, MEM_HOLD_PIN)

extern struct Module Memory_module;

#endif // _INC_MEMORY_H