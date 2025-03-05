#ifndef _INC_UTILS_H
#define _INC_UTILS_H

#include <stdint.h>

int32_t NormalizeAngleDegrees(int32_t degAng);
int32_t Sin100000(int32_t degAng);
int32_t Cos100000(int32_t degAng);
int32_t ln1000(int32_t adc12BitValue);

uint32_t SquareRootRounded(uint32_t input);
uint16_t Crc16(const uint8_t *data, unsigned len);

#endif // _INC_UTILS_H