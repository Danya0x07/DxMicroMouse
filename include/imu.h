#ifndef _INC_IMU_H
#define _INC_IMU_H

#include <shell.h>
#include <scheduler.h>
#include <settings.h>

struct IMU_Data {
    int16_t accelX;
    int16_t accelY;
    int16_t accelZ;
    int16_t gyroX;
    int16_t gyroY;
    int16_t gyroZ;
};

enum ImuConfiguration {
    ImuConfiguration_APP,
    ImuConfiguration_CALIBRATION,
    ImuConfiguration_TEST
};

int IMU_Init(enum ImuConfiguration configuration);
int IMU_Test(void);
void IMU_Calibrate(unsigned numIterations);
void IMU_Update(void);
void IMU_GetData(struct IMU_Data *data);

extern struct SchedulerTask TASK_TmImu;
extern const struct ShellCommand CMD_Imu;
extern const struct Settings SETT_Imu;

#endif // _INC_IMU_H