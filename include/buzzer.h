#ifndef _INC_BUZZER_H
#define _INC_BUZZER_H

#include "module.h"

void Buzzer_Blink(unsigned times, unsigned freq, unsigned duration);
void Buzzer_Sing(uint16_t *freqs, unsigned len, unsigned duration);
void Buzzer_BeepAsync(unsigned freq, unsigned duration);
void Buzzer_Update(void);

void Buzzer_BlinkInitError(unsigned step);
void Buzzer_BlinkLowBattery(void);
void Buzzer_BlinkWaitingFinger(unsigned step);
void Buzzer_BlinkStopActivity();
void Buzzer_BlinkManeuverFailed();
void Buzzer_SingHappy(void);
void Buzzer_SingSetupMode(void);
void Buzzer_SingErazeMaze(void);
void Buzzer_SingRunMode(void);
void Buzzer_BeepManeuverCompleted();
void Buzzer_BeepTurningBack(void);

extern struct Module Buzzer_module;

#endif // _INC_BUZZER_H