#include "motors.h"
#include "mcu.h"

void Motors_SetL(int16_t duty)
{
    duty = duty > MOTOR_DUTY_MAX ? MOTOR_DUTY_MAX :
            duty < -MOTOR_DUTY_MAX ? -MOTOR_DUTY_MAX : duty;
    if (duty > 0) {
        TIM_SetCompare2(TIM4, 0);
        TIM_SetCompare1(TIM4, duty);
    } else {
        TIM_SetCompare1(TIM4, 0);
        TIM_SetCompare2(TIM4, -duty);
    }
}

void Motors_SetR(int16_t duty)
{
    duty = duty > MOTOR_DUTY_MAX ? MOTOR_DUTY_MAX :
            duty < -MOTOR_DUTY_MAX ? -MOTOR_DUTY_MAX : duty;
    if (duty > 0) {
        TIM_SetCompare4(TIM4, 0);
        TIM_SetCompare3(TIM4, duty);
    } else {
        TIM_SetCompare3(TIM4, 0);
        TIM_SetCompare4(TIM4, -duty);
    }
}