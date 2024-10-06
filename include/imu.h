#ifndef _INC_IMU_H
#define _INC_IMU_H

#include <module.h>

struct IMU_Data {
    int16_t accelX;
    int16_t accelY;
    int16_t accelZ;
    int16_t gyroX;
    int16_t gyroY;
    int16_t gyroZ;
};

void IMU_Init(void);
void IMU_Update(void);
void IMU_GetData(struct IMU_Data *data);

extern struct Module IMU_module;

#endif // _INC_IMU_H