#ifndef _INC_ENCODERS_H
#define _INC_ENCODERS_H

#include "module.h"

int Encoders_Init(void);
void Encoders_Update(void);
void Encoders_Reset(void);
void Encoders_GetCounts(int32_t *left, int32_t *right);
void Encoders_GetDelta(int32_t *left, int32_t *right);

extern struct Module Encoders_module;

#endif // _INC_ENCODERS_H