#ifndef _INC_BUZZER_H
#define _INC_BUZZER_H

#include "mcu.h"

void Buzzer_SetFrequency(uint16_t freq);
void Buzzer_Start(void);
void Buzzer_Stop(void);
void Buzzer_Blink(uint16_t times, uint16_t freq, uint16_t duration);
void Buzzer_Sing(uint16_t *freqs, uint16_t len, uint16_t duration);

#endif // _INC_BUZZER_H