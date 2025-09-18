#ifndef _INC_GPIO_CTL_H
#define _INC_GPIO_CTL_H

#include "mcu.h"

typedef GPIO_TypeDef *GPIOCTL_Port_t;
typedef uint16_t GPIOCTL_Pin_t;

struct GPIOCTL_Line {
    const GPIOCTL_Port_t port;
    const GPIOCTL_Pin_t pin;
};

static inline void GPIOCTL_High(const struct GPIOCTL_Line *line)
{
    GPIO_SetBits(line->port, line->pin);
}

static inline void GPIOCTL_Low(const struct GPIOCTL_Line *line)
{
    GPIO_ResetBits(line->port, line->pin);
}

static inline bool GPIOCTL_IsHigh(const struct GPIOCTL_Line *line)
{
    return GPIO_ReadInputDataBit(line->port, line->pin);
}

#endif // _INC_GPIO_CTL_H