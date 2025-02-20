#include "buzzer.h"

void Buzzer_SetFrequency(uint16_t freq)
{
    /* frequency = APB1_FREQ / ((prescaler + 1) * ARR * 2)
     */

    const uint32_t product = (MCU_rccClocks.PCLK1_Frequency / (uint32_t)freq) >> 1;
    uint32_t prescaler;
    uint32_t period;

    for (prescaler = 1; prescaler <= 0xFFFF; prescaler++) {
        period = product / (prescaler + 1);
        if (period <= 0xFFFF)
            break;
    }

    TIM_PrescalerConfig(TIM2, (uint16_t)prescaler, TIM_PSCReloadMode_Immediate);
    TIM_SetAutoreload(TIM2, period);
    TIM_SetCompare1(TIM2, period);
}

void Buzzer_Start(void)
{
    TIM_CtrlPWMOutputs(TIM2, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
}

void Buzzer_Stop(void)
{
    TIM_CtrlPWMOutputs(TIM2, DISABLE);
    TIM_Cmd(TIM2, DISABLE);
}

void Buzzer_Blink(uint16_t times, uint16_t freq, uint16_t duration)
{
    Buzzer_Stop();
    Buzzer_SetFrequency(freq);
    while (times--) {
        Buzzer_Start();
        Millis_Wait(duration);
        Buzzer_Stop();
        Millis_Wait(duration);
    }
}

void Buzzer_Sing(uint16_t *freqs, uint16_t len, uint16_t duration)
{
    Buzzer_Stop();
    while (len--) {
        Buzzer_SetFrequency(*freqs++);
        Buzzer_Start();
        Millis_Wait(duration);
        Buzzer_Stop();
    }
}

static uint32_t endTime;

void Buzzer_BeepAsync(uint16_t freq, uint16_t duration)
{
    Buzzer_SetFrequency(freq);
    endTime = Millis_Get() + duration;
}

void Buzzer_Update(void)
{
    if (Millis_Get() >= endTime)
        Buzzer_Stop();
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

struct Module Buzzer_module = {
    .name = "bz",
    .execute = execute,
    .telemetry = NULL
};