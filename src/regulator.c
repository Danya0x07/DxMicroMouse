#include "regulator.h"

#define MAX_OUTPUT  1000000

void Regulator_Setup(struct Regulator *reg, int32_t kP, int32_t kI, int32_t kD)
{
    reg->kP = kP;
    reg->kI = kI;
    reg->kD = kD;
}

void Regulator_Reset(struct Regulator *reg)
{
    reg->prevError = 0;
    reg->intError = 0;
}

int64_t Regulator_Output(struct Regulator *reg, int32_t target, int32_t feedback)
{
    int32_t error = target - feedback;
    int32_t diffError = error - reg->prevError;

    reg->prevError = error;

    int64_t output = reg->kP * error + reg->kI * reg->intError + reg->kD * diffError;

    if (output > MAX_OUTPUT && error > 0)
        return MAX_OUTPUT;
    else if (output < -MAX_OUTPUT && error < 0)
        return -MAX_OUTPUT;

    reg->intError += error;
    return output;
}