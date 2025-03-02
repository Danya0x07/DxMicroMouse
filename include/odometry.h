#ifndef _INC_ODOMETRY_H
#define _INC_ODOMETRY_H

#include "module.h"

#define COUNTS_PER_MM    62

void Odometry_Reset(void);

void Odometry_SetReckon(int32_t distanceInMm, int32_t angleInDeg);
void Odometry_UpdateReckon(int32_t transInCounts, int32_t rotInMimuUnits);
void Odometry_GetReckon(int32_t *distanceInMm, int32_t *angleInDeg);

void Odometry_SetPrediction(int32_t distanceInMm, int32_t angleInDeg);
void Odometry_UpdatePrediction(int32_t transInMm, int32_t rotInDeg);
void Odometry_GetPrediction(int32_t *distanceInMm, int32_t *angleInDeg);

void Odometry_GetFusion(int32_t *distanceInMm, int32_t *angleInDeg);

extern struct Module Odometry_module;

#endif // _INC_ODOMETRY_H