#include "fan.h"
#include "mcu.h"
#include <stdlib.h>

#ifdef FAN_PWM

void Fan_On(void)
{
    Fan_SetDuty(MOTOR_DUTY_MAX >> 1);
}

void Fan_Off(void)
{
    Fan_SetDuty(0);
}

void Fan_SetDuty(uint16_t duty)
{
    TIM_SetCompare1(TIM3, duty);
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

void Fan_SetDuty(uint16_t duty)
{
    (void)duty;
}

#endif

static int execute(int argc, char *argv[])
{
    if (argc != 1)
        return -1;

    uint16_t duty = atoi(argv[0]);
    if (duty <= MOTOR_DUTY_MAX >> 1) {
        Fan_SetDuty(duty);
        return 0;
    }
    return -2;
}

struct Module Fan_module = {
    .name = "fan",
    .execute = execute,
    .telemetry = NULL
};