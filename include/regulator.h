#ifndef _INC_REGULATOR_H
#define _INC_REGULATOR_H

#include <stdint.h>

struct Regulator {
    int32_t kP, kI, kD;
    int32_t prevError;
    int64_t intError;
};

void Regulator_Setup(struct Regulator *reg, int32_t kP, int32_t kI, int32_t kD);
void Regulator_Reset(struct Regulator *reg);
int64_t Regulator_Output(struct Regulator *reg, int32_t target, int32_t feedback);

#endif // _INC_REGULATOR_H