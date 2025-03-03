#ifndef _INC_LEDS_H
#define _INC_LEDS_H

#include "mcu.h"

#define LED0_ON()   GPIO_SetBits(LED0_GPIO, LED0_PIN)
#define LED0_OFF()  GPIO_ResetBits(LED0_GPIO, LED0_PIN)
#define LED1_ON()   GPIO_SetBits(LED1_GPIO, LED1_PIN)
#define LED1_OFF()  GPIO_ResetBits(LED1_GPIO, LED1_PIN)

void LED0_Blink(uint16_t times, uint16_t duration);
void LED1_Blink(uint16_t times, uint16_t duration);

#endif // _INC_LEDS_H