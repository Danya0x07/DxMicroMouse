#include "motors.h"
#include "mcu.h"
#include <stdlib.h>

static int16_t ConstrainPwm(int16_t pwm)
{
    return pwm > MOTOR_PWM_MAX ? MOTOR_PWM_MAX :
            pwm < -MOTOR_PWM_MAX ? -MOTOR_PWM_MAX : pwm;
}

static void SetLeftPwm(int16_t pwm)
{
    pwm = ConstrainPwm(pwm);
    if (pwm > 0) {
        TIM_SetCompare2(TIM4, 0);
        TIM_SetCompare1(TIM4, pwm);
    } else {
        TIM_SetCompare1(TIM4, 0);
        TIM_SetCompare2(TIM4, -pwm);
    }
}

static void SetRightPwm(int16_t pwm)
{
    pwm = ConstrainPwm(pwm);
    if (pwm > 0) {
        TIM_SetCompare3(TIM4, 0);
        TIM_SetCompare4(TIM4, pwm);
    } else {
        TIM_SetCompare4(TIM4, 0);
        TIM_SetCompare3(TIM4, -pwm);
    }
}

void Motors_SetPwm(int16_t left, int16_t right)
{
    SetLeftPwm(left);
    SetRightPwm(right);
}

void Motors_GetPwm(int16_t *left, int16_t *right)
{
    *left = (int16_t)TIM_GetCapture2(TIM4) - (int16_t)TIM_GetCapture1(TIM4);
    *right = (int16_t)TIM_GetCapture3(TIM4) - (int16_t)TIM_GetCapture4(TIM4);
}

static int execute(int argc, char *argv[])
{
    if (argc != 2)
        return -1;

    int16_t pwmLeft = atoi(argv[0]);
    int16_t pwmRight = atoi(argv[1]);

    Motors_SetPwm(pwmLeft, pwmRight);
    return 0;
}

static void WriteTelemetry(char out[TELEMETRY_STRING_SIZE])
{
    int16_t pwmLeft, pwmRight;

    Motors_GetPwm(&pwmLeft, &pwmRight);
    snprintf(out, TELEMETRY_STRING_SIZE, "L:%d\tR:%d\n", pwmLeft, pwmRight);
}

static struct TelemetryControlBlock telemetryControlBlock = {
    .interval = 50,
    .write = WriteTelemetry
};

struct Module Motors_module = {
    .name = "motors",
    .execute = execute,
    .telemetry = &telemetryControlBlock
};