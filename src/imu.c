#include "imu.h"
#include "mcu.h"
#include <mpu6500.h>

static volatile struct IMU_Data currentData;

void IMU_Init(void)
{
    const struct MPU6500_Configuration config = {
        .fifo = {
            .mode = MPU6500_FIFO_MODE_REPLACING,
            .writeTemp = false,
            .writeGyroX = false,
            .writeGyroY = false,
            .writeGyroZ = false,
            .writeAccelXYZ = false,
            .writeSlave3 = false,
            .writeSlave2 = false,
            .writeSlave1 = false,
            .writeSlave0 = false
        },
        .fsyncPosition = MPU6500_FSYNCPOS_DISABLED,
        .gyro = {
            .bandwidth = MPU6500_GYRO_BANDWIDTH_250Hz_0Ms97,
            .range = MPU6500_GYRO_RANGE_500DPS
        },
        .accel = {
            .bandwidth = MPU6500_ACCEL_BANDWIDTH_460Hz_1Ms94,
            .lpfrequency = MPU6500_ACCEL_LPFREQUENCY_250Hz,
            .range = MPU6500_ACCEL_RANGE_4G
        }
    };

    // Perform reset
    MPU6500_ResetDevice();
    Millis_Wait(100);
    MPU6500_ResetSignalPath(true, true, true);
    Millis_Wait(100);

    // Check ID
    uint8_t id = MPU6500_ReadID();
    printf("MPU6500 ID: %x\n", id);

    MPU6500_Configure(&config);
    MPU6500_SetPowerMode(MPU6500_PowerMode_6AXIS);
    MPU6500_SetClockSource(MPU6500_ClockSource_AUTOPLL);
    Millis_Wait(100);

    // Check self-test data
    struct MPU6500_SelfTestData selfTestData;
    MPU6500_GetSelfTestData(&selfTestData);
    printf("MPU6500 self-test manufacturing data:\n");
    printf("\tgX:%d\tgY:%d\tgZ:%d\taX:%d\taY:%d\taZ:%d\n",
        selfTestData.gyroX, selfTestData.gyroY, selfTestData.gyroZ,
        selfTestData.accelX, selfTestData.accelY, selfTestData.accelZ
    );

    // Check offsets
    struct MPU6500_SensorData sensorData;
    MPU6500_GetOffset(&sensorData);
    printf("MPU6500 initial offsets:\n");
    printf("\tgX:%d\tgY:%d\tgZ:%d\taX:%d\taY:%d\taZ:%d\n",
        sensorData.gyroX, sensorData.gyroY, sensorData.gyroZ,
        sensorData.accelX, sensorData.accelY, sensorData.accelZ
    );

    // Configure interrupt
    const struct MPU6500_InterruptPinConfiguration intPinConfig = {
        .activeLevelLow = false,
        .openDrain = false,
        .latchUntilClear = true,
        .anyReadClears = true,
        .fsyncTransitionInterrupt = false,
        .fsyncActiveLevelLow = false
    };
    const struct MPU6500_InterruptConfiguration intConfig = {
        .wakeOnMotion = DISABLE,
        .fifoOverflow = DISABLE,
        .fsyncTransition = DISABLE,
        .rawDataReady = ENABLE
    };
    MPU6500_ConfigureInterruptPin(&intPinConfig);
    MPU6500_ConfigureInterrupt(&intConfig);
}

void IMU_Update(void)
{
    if (GPIO_ReadInputDataBit(IMU_INT_GPIO, IMU_INT_PIN) == 1) {
        struct MPU6500_SensorData sensorData;
        MPU6500_GetSensorData(&sensorData);

        currentData.accelX = sensorData.accelX;
        currentData.accelY = sensorData.accelY;
        currentData.accelZ = sensorData.accelZ;
        currentData.gyroX = sensorData.gyroX;
        currentData.gyroY = sensorData.gyroY;
        currentData.gyroZ = sensorData.gyroZ;
    }
}

void IMU_GetData(struct IMU_Data *data)
{
    memcpy_v2n(data, &currentData, sizeof(struct IMU_Data));
}

static void WriteTelemetry(char out[TELEMETRY_STRING_SIZE])
{
    struct IMU_Data imuData;
    IMU_GetData(&imuData);

    snprintf(out, TELEMETRY_STRING_SIZE,
        "gX:%d\tgY:%d\tgZ:%d\taX:%d\taY:%d\taZ:%d\n",
        imuData.gyroX, imuData.gyroY, imuData.gyroZ,
        imuData.accelX, imuData.accelY, imuData.accelZ
    );
}


static struct TelemetryControlBlock telemetryControlBlock = {
    .interval = 200,
    .write = WriteTelemetry
};

struct Module IMU_module = {
    .name = "imu",
    .execute = NULL,
    .telemetry = &telemetryControlBlock
};