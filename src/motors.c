#include "motors.h"
#include "mcu.h"
#include <stdlib.h>

static int ConstrainPwm(int pwm)
{
    return pwm > MOTOR_PWM_MAX ? MOTOR_PWM_MAX :
            pwm < -MOTOR_PWM_MAX ? -MOTOR_PWM_MAX : pwm;
}

static void SetLeftPwm(int pwm)
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

static void SetRightPwm(int pwm)
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

void Motors_SetPwm(int left, int right)
{
    SetLeftPwm(left);
    SetRightPwm(right);
}

void Motors_GetPwm(int *left, int *right)
{
    *left = (int)TIM_GetCapture2(TIM4) - (int)TIM_GetCapture1(TIM4);
    *right = (int)TIM_GetCapture3(TIM4) - (int)TIM_GetCapture4(TIM4);
}

static int execute(int argc, char *argv[])
{
    if (argc != 2)
        return -1;

    int pwmLeft = atoi(argv[0]);
    int pwmRight = atoi(argv[1]);

    Motors_SetPwm(pwmLeft, pwmRight);
    return 0;
}

static void WriteTelemetry(char out[TELEMETRY_STRING_SIZE])
{
    int pwmLeft, pwmRight;

    Motors_GetPwm(&pwmLeft, &pwmRight);
    snprintf(out, TELEMETRY_STRING_SIZE, "L:%d\tR:%d\n", pwmLeft, pwmRight);
}

static struct ModuleTelemetry telemetry = {
    .interval = 50,
    .write = WriteTelemetry
};

struct Module Motors_module = {
    .name = "motors",
    .execute = execute,
    .telemetry = &telemetry
};