#ifndef _INC_M95256_PORT_H
#define _INC_M95256_PORT_H

#include "mcu.h"

#define S_LOW()     GPIO_ResetBits(MEM_CS_GPIO, MEM_CS_PIN)
#define S_HIGH()    GPIO_SetBits(MEM_CS_GPIO, MEM_CS_PIN)
#define W_IS_LOW()  0
#define H_LOW()     GPIO_ResetBits(MEM_HOLD_GPIO, MEM_HOLD_PIN)
#define H_HIGH()    GPIO_SetBits(MEM_HOLD_GPIO, MEM_HOLD_PIN)

#define MS_GET()    (Micros_Get() >> 10)
#define MS_WAIT(ms) Delay_Ms((ms))

#endif // _INC_M95256_PORT_H