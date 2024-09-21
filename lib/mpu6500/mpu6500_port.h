#ifndef _INC_MPU6500_PORT_H
#define _INC_MPU6500_PORT_H

#include "mcu.h"

#define CS_LOW()    GPIO_ResetBits(IMU_CS_GPIO, IMU_CS_PIN)
#define CS_HIGH()   GPIO_SetBits(IMU_CS_GPIO, IMU_CS_PIN)

#endif // _INC_MPU6500_PORT_H