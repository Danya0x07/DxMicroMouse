#ifndef _INC_ENCODERS_H
#define _INC_ENCODERS_H

#include "module.h"

struct Encoders_Data {
    int32_t ticksLeft;
    int32_t ticksRight;
};

int Encoders_Init(void);
void Encoders_Update(void);
void Encoders_Reset(void);
void Encoders_GetData(struct Encoders_Data *data);

extern struct Module Encoders_module;

#endif // _INC_ENCODERS_H