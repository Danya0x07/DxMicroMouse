#include "fan.h"
#include "mcu.h"

void Fan_On(void)
{
    GPIO_SetBits(MOTORS_GPIO, FAN_PIN);
}

void Fan_Off(void)
{
    GPIO_ResetBits(MOTORS_GPIO, FAN_PIN);
}