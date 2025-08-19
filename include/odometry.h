#ifndef _INC_ODOMETRY_H
#define _INC_ODOMETRY_H

#include "module.h"

#define COUNTS_PER_MM    63
#define COUNTS_PER_CELL 11116

void Odometry_Reset(void);

void Odometry_SetReckon(int distanceInMm, int angleInDeg);
void Odometry_UpdateReckon(int32_t transInCounts, int32_t rotInMimuUnits);
void Odometry_GetReckon(int *distanceInMm, int *angleInDeg);
void Odometry_SnapReckon(void);

void Odometry_SetPrediction(int distanceInMm, int angleInDeg);
void Odometry_UpdatePrediction(int transInMm, int rotInDeg);
void Odometry_GetPrediction(int *distanceInMm, int *angleInDeg);

void Odometry_GetFusion(int *distanceInMm, int *angleInDeg);

extern struct Module Odometry_module;

#endif // _INC_ODOMETRY_H