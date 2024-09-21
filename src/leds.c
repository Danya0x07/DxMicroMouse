#include "leds.h"

void LED0_Blink(uint16_t times, uint16_t duration)
{
    while (times--) {
        LED0_ON();
        Millis_Wait(duration);
        LED0_OFF();
        Millis_Wait(duration);
    }
}

void LED1_Blink(uint16_t times, uint16_t duration)
{
    while (times--) {
        LED1_ON();
        Millis_Wait(duration);
        LED1_OFF();
        Millis_Wait(duration);
    }
}