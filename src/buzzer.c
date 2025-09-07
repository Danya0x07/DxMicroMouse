#include "buzzer.h"
#include "mcu.h"
#include <stdlib.h>

static void SetFrequency(unsigned freq)
{
    /* frequency = APB1_FREQ / ((prescaler + 1) * ARR * 2)
     */

    const uint32_t product = MCU_rccClocks.PCLK1_Frequency / 1000;
    uint32_t prescaler = product / (freq << 1) - 1;

    TIM_PrescalerConfig(TIM2, (uint16_t)prescaler, TIM_PSCReloadMode_Immediate);
    TIM_SetAutoreload(TIM2, 999);
    TIM_SetCompare1(TIM2, 999);
}

static void Start(void)
{
    TIM_CtrlPWMOutputs(TIM2, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
}

static void Stop(void)
{
    TIM_CtrlPWMOutputs(TIM2, DISABLE);
    TIM_Cmd(TIM2, DISABLE);
}

void Buzzer_Blink(unsigned times, unsigned freq, unsigned duration)
{
    Stop();
    SetFrequency(freq);
    while (times--) {
        Start();
        Millis_Wait(duration);
        Stop();
        Millis_Wait(duration);
    }
}

void Buzzer_Sing(uint16_t *freqs, unsigned len, unsigned duration)
{
    Stop();
    while (len--) {
        SetFrequency(*freqs++);
        Start();
        Millis_Wait(duration);
        Stop();
    }
}

static uint32_t endTime;

void Buzzer_BeepAsync(unsigned freq, unsigned duration)
{
    endTime = Millis_Get() + duration;
    SetFrequency(freq);
    Start();
}

void Buzzer_Update(void)
{
    if (endTime && Millis_Get() >= endTime) {
        Stop();
        endTime = 0;
    }
}

void Buzzer_BlinkInitError(unsigned step)
{
    Buzzer_Blink(step, 1000, 300);
}

void Buzzer_BlinkLowBattery(void)
{
    Buzzer_Blink(3, 600, 80);
}

void Buzzer_BlinkWaitingFinger(unsigned step)
{
    Buzzer_Blink(1, step * 1000, 50);
}

void Buzzer_BlinkStopActivity()
{
    Buzzer_Blink(2, 900, 80);
}

void Buzzer_BlinkManeuverFailed()
{
    Buzzer_Blink(6, 800, 80);
}

void Buzzer_SingHappy(void)
{
    Buzzer_Sing((uint16_t []){2600, 2900, 3100, 3300, 3600, 3900}, 6, 70);
}

void Buzzer_SingSetupMode(void)
{
    Buzzer_Sing((uint16_t []){1200, 1500, 2000}, 3, 50);
}

void Buzzer_SingErazeMaze(void)
{
    Buzzer_Sing((uint16_t []){2000, 1800}, 2, 50);
}

void Buzzer_SingRunMode(void)
{
    Buzzer_Sing((uint16_t []){2800, 2800, 3300, 4000}, 4, 50);
}

void Buzzer_BeepManeuverCompleted()
{
    Buzzer_BeepAsync(4000, 20);
}

void Buzzer_BeepTurningBack(void)
{
    Buzzer_BeepAsync(2600, 30);
}

static int execute(int argc, char *argv[])
{
    if (argc != 3)
        return -1;

    unsigned times = atoi(argv[0]);
    unsigned freq = atoi(argv[1]);
    unsigned duration = atoi(argv[2]);
    Buzzer_Blink(times, freq, duration);

    return 0;
}

const struct ShellCommand CMD_Buzzer = {
    .name = "bz",
    .execute = execute
};