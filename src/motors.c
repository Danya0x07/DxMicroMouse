#include "motors.h"
#include "mcu.h"
#include <stdlib.h>

static volatile int16_t targetL, targetR;

void Motors_SetDutyLeft(int16_t duty)
{
    duty = duty > MOTOR_DUTY_MAX ? MOTOR_DUTY_MAX :
            duty < -MOTOR_DUTY_MAX ? -MOTOR_DUTY_MAX : duty;
    if (duty > 0) {
        TIM_SetCompare1(TIM4, 0);
        TIM_SetCompare2(TIM4, duty);
    } else {
        TIM_SetCompare2(TIM4, 0);
        TIM_SetCompare1(TIM4, -duty);
    }
}

void Motors_SetDutyRight(int16_t duty)
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

void Motors_SetTargetDuty(int16_t left, int16_t right)
{
    targetL = left;
    targetR = right;
}

void Motors_Update(void)
{
    int16_t currentL = (int16_t)TIM_GetCapture2(TIM4) - (int16_t)TIM_GetCapture1(TIM4);
    int16_t currentR = (int16_t)TIM_GetCapture3(TIM4) - (int16_t)TIM_GetCapture4(TIM4);
    int16_t tgtL = targetL, tgtR = targetR;

    if (currentL < tgtL)
        currentL++;
    else if (currentL > tgtL)
        currentL--;

    if (currentR < tgtR)
        currentR++;
    else if (currentR > tgtR)
        currentR--;

    Motors_SetDutyLeft(currentL);
    Motors_SetDutyRight(currentR);
}

static int execute(int argc, char *argv[])
{
    if (argc != 2)
        return -1;

    int16_t dutyL = atoi(argv[0]);
    int16_t dutyR = atoi(argv[1]);

    if (abs(dutyL) <= MOTOR_DUTY_MAX && abs(dutyR) <= MOTOR_DUTY_MAX) {
        Motors_SetTargetDuty(dutyL, dutyR);
        return 0;
    }
    return -2;
}

struct Module Motors_module = {
    .name = "motors",
    .execute = execute,
    .telemetry = NULL
};