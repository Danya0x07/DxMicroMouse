#ifndef _INC_ODOMETRY_H
#define _INC_ODOMETRY_H

#include "module.h"

#define COUNTS_PER_MM    60

void Odometry_Reset(void);
void Odometry_Update(int32_t transInCounts, int32_t deltaAngInMimuUnits);
void Odometry_GetPosition(int32_t *mmX, int32_t *mmY, int32_t *degAng);
void Odometry_GetVelocity(int16_t *vTransInMmPerS, int16_t *vRotInDegPerS);

extern struct Module Odometry_module;

#endif // _INC_ODOMETRY_H