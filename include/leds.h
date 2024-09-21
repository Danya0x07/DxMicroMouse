#ifndef _INC_LEDS_H
#define _INC_LEDS_H

#include "mcu.h"

#define LED0_ON()   GPIO_WriteBit(LED0_GPIO, LED0_PIN, 1)
#define LED0_OFF()  GPIO_WriteBit(LED0_GPIO, LED0_PIN, 0)
#define LED1_ON()   GPIO_WriteBit(LED1_GPIO, LED1_PIN, 1)
#define LED1_OFF()  GPIO_WriteBit(LED1_GPIO, LED1_PIN, 0)

void LED0_Blink(uint16_t times, uint16_t duration);
void LED1_Blink(uint16_t times, uint16_t duration);

#endif // _INC_LEDS_H