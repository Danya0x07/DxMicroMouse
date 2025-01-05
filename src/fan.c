#include "fan.h"
#include "mcu.h"
#include <stdlib.h>

#ifdef FAN_PWM_MAX

void Fan_On(void)
{
    Fan_SetPwm(FAN_PWM_MAX);
}

void Fan_Off(void)
{
    Fan_SetPwm(0);
}

void Fan_SetPwm(uint16_t pwm)
{
    TIM_SetCompare1(TIM3, pwm > FAN_PWM_MAX ? FAN_PWM_MAX : pwm);
}

#else
#error "Fan motor is 4.2V maximum rated, we are using 8.4V maximum voltage."

void Fan_On(void)
{
    GPIO_SetBits(MOTORS_GPIO, FAN_PIN);
}

void Fan_Off(void)
{
    GPIO_ResetBits(MOTORS_GPIO, FAN_PIN);
}

void Fan_SetPwm(uint16_t pwm)
{
    (void)pwm;
}

#endif

static int execute(int argc, char *argv[])
{
    if (argc != 1)
        return -1;

    uint16_t pwm = atoi(argv[0]);
    Fan_SetPwm(pwm);
    return 0;
}

struct Module Fan_module = {
    .name = "fan",
    .execute = execute,
    .telemetry = NULL
};