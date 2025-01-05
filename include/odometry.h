#ifndef INC_ODOMETRY_H
#define INC_ODOMETRY_H

#include "module.h"

void Odometry_Reset();
void Odometry_Update(float deltaPos, float deltaAng);
void Odometry_Get(float *x, float *y, float *ang);

extern struct Module Odometry_module;

#endif // INC_ODOMETRY_H