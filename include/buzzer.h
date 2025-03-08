#ifndef _INC_BUZZER_H
#define _INC_BUZZER_H

#include "module.h"

void Buzzer_SetFrequency(unsigned freq);
void Buzzer_Start(void);
void Buzzer_Stop(void);
void Buzzer_Blink(unsigned times, unsigned freq, unsigned duration);
void Buzzer_Sing(uint16_t *freqs, unsigned len, unsigned duration);

void Buzzer_BeepAsync(unsigned freq, unsigned duration);
void Buzzer_Update(void);

extern struct Module Buzzer_module;

#endif // _INC_BUZZER_H