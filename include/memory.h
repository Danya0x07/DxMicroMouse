#ifndef _INC_MEMORY_H
#define _INC_MEMORY_H

#include "module.h"

int Memory_Init(void);

#define MEMORY_HOLD_TRANSACTION()   GPIO_ResetBits(MEM_HOLD_GPIO, MEM_HOLD_PIN)
#define MEMORY_UNHOLD_TRANSACTION() GPIO_SetBits(MEM_HOLD_GPIO, MEM_HOLD_PIN)

extern struct Module Memory_module;

#endif // _INC_MEMORY_H