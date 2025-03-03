#include "imu.h"
#include "mcu.h"
#include <mpu6500.h>
#include "leds.h"
#include <stdlib.h>
#include <string.h>

#define CALIB_BUFFSIZE  1000

static const uint16_t mpu6500SelfTestTable[256] = {
    2620,2646,2672,2699,2726,2753,2781,2808,
    2837,2865,2894,2923,2952,2981,3011,3041,
    3072,3102,3133,3165,3196,3228,3261,3293,
    3326,3359,3393,3427,3461,3496,3531,3566,
    3602,3638,3674,3711,3748,3786,3823,3862,
    3900,3939,3979,4019,4059,4099,4140,4182,
    4224,4266,4308,4352,4395,4439,4483,4528,
    4574,4619,4665,4712,4759,4807,4855,4903,
    4953,5002,5052,5103,5154,5205,5257,5310,
    5363,5417,5471,5525,5581,5636,5693,5750,
    5807,5865,5924,5983,6043,6104,6165,6226,
    6289,6351,6415,6479,6544,6609,6675,6742,
    6810,6878,6946,7016,7086,7157,7229,7301,
    7374,7448,7522,7597,7673,7750,7828,7906,
    7985,8065,8145,8227,8309,8392,8476,8561,
    8647,8733,8820,8909,8998,9088,9178,9270,
    9363,9457,9551,9647,9743,9841,9939,10038,
    10139,10240,10343,10446,10550,10656,10763,10870,
    10979,11089,11200,11312,11425,11539,11654,11771,
    11889,12008,12128,12249,12371,12495,12620,12746,
    12874,13002,13132,13264,13396,13530,13666,13802,
    13940,14080,14221,14363,14506,14652,14798,14946,
    15096,15247,15399,15553,15709,15866,16024,16184,
    16346,16510,16675,16842,17010,17180,17352,17526,
    17701,17878,18057,18237,18420,18604,18790,18978,
    19167,19359,19553,19748,19946,20145,20347,20550,
    20756,20963,21173,21385,21598,21814,22033,22253,
    22475,22700,22927,23156,23388,23622,23858,24097,
    24338,24581,24827,25075,25326,25579,25835,26093,
    26354,26618,26884,27153,27424,27699,27976,28255,
    28538,28823,29112,29403,29697,29994,30294,30597,
    30903,31212,31524,31839,32157,32479,32804,33132
};

static volatile struct IMU_Data currentData;
static struct MPU6500_SensorData sensorOffset = {0};

struct ImuAverage {
    int32_t aX, aY, aZ;
    int32_t gX, gY, gZ;
};
typedef struct ImuAverage ImuDeviation;

static void GetAverages(struct ImuAverage *avg, uint16_t numToAverage, uint16_t numToSkip)
{
    struct MPU6500_SensorData sensorData;
    int32_t avgAccelX = 0, avgAccelY = 0, avgAccelZ = 0, avgGyroX = 0, avgGyroY = 0, avgGyroZ = 0;

    for (int i = 0; i < numToSkip + numToAverage; i++) {
        MPU6500_GetSensorData(&sensorData);
        Micros_WaitMillis(2);
        if (i < numToSkip)
            continue;
        avgAccelX += sensorData.accelX;
        avgAccelY += sensorData.accelY;
        avgAccelZ += sensorData.accelZ;
        avgGyroX += sensorData.gyroX;
        avgGyroY += sensorData.gyroY;
        avgGyroZ += sensorData.gyroZ;
    }
    avg->aX = avgAccelX / numToAverage;
    avg->aY = avgAccelY / numToAverage;
    avg->aZ = avgAccelZ / numToAverage;
    avg->gX = avgGyroX / numToAverage;
    avg->gY = avgGyroY / numToAverage;
    avg->gZ = avgGyroZ / numToAverage;
}

static void GetDeviationFromFactoryTrim(const struct ImuAverage *testOn,
                                        const struct ImuAverage *testOff,
                                        ImuDeviation *deviation)
{
    struct MPU6500_SelfTestData selfTestData;
    MPU6500_GetSelfTestData(&selfTestData);

    const int32_t factoryTrimAccelX = mpu6500SelfTestTable[selfTestData.accelX];
    const int32_t factoryTrimAccelY = mpu6500SelfTestTable[selfTestData.accelY];
    const int32_t factoryTrimAccelZ = mpu6500SelfTestTable[selfTestData.accelZ];
    const int32_t factoryTrimGyroX = mpu6500SelfTestTable[selfTestData.gyroX];
    const int32_t factoryTrimGyroY = mpu6500SelfTestTable[selfTestData.gyroY];
    const int32_t factoryTrimGyroZ = mpu6500SelfTestTable[selfTestData.gyroZ];

    deviation->aX = 100L * (testOn->aX - testOff->aX - factoryTrimAccelX) / factoryTrimAccelX;
    deviation->aY = 100L * (testOn->aY - testOff->aY - factoryTrimAccelY) / factoryTrimAccelY;
    deviation->aZ = 100L * (testOn->aZ - testOff->aZ - factoryTrimAccelZ) / factoryTrimAccelZ;
    deviation->gX = 100L * (testOn->gX - testOff->gX - factoryTrimGyroX) / factoryTrimGyroX;
    deviation->gY = 100L * (testOn->gY - testOff->gY - factoryTrimGyroY) / factoryTrimGyroY;
    deviation->gZ = 100L * (testOn->gZ - testOff->gZ - factoryTrimGyroZ) / factoryTrimGyroZ;
}

static void ApplyOffsetsByAverage(const struct ImuAverage *avg)
{
    MPU6500_GetOffset(&sensorOffset);

    sensorOffset.accelX -= (int16_t)avg->aX;
    sensorOffset.accelY -= (int16_t)avg->aY;
    //~ sensorOffset.accelZ -= (int16_t)(avg->aZ > 0 ? avg->aZ + 2048 : avg->aZ - 2048);
    sensorOffset.accelZ -= (int16_t)avg->aZ;
    sensorOffset.gyroX = -(int16_t)avg->gX;
    sensorOffset.gyroY = -(int16_t)avg->gY;
    sensorOffset.gyroZ = -(int16_t)avg->gZ;

    MPU6500_SetOffset(&sensorOffset);
}

int IMU_Init(enum ImuConfiguration configuration)
{
    int retcode = 0;

    struct MPU6500_Configuration config = {
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
            .range = MPU6500_GYRO_RANGE_2000DPS
        },
        .accel = {
            .bandwidth = MPU6500_ACCEL_BANDWIDTH_460Hz_1Ms94,
            .lpfrequency = MPU6500_ACCEL_LPFREQUENCY_250Hz,
            .range = MPU6500_ACCEL_RANGE_4G
        }
    };
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

    SysTick_DisableInterrupt();

    if (configuration == ImuConfiguration_CALIBRATION) {
        config.gyro.bandwidth = MPU6500_GYRO_BANDWIDTH_250Hz_0Ms97;
        config.gyro.range = MPU6500_GYRO_RANGE_1000DPS;
        config.accel.bandwidth = MPU6500_ACCEL_BANDWIDTH_1130Hz_0Ms75;
        config.accel.range = MPU6500_ACCEL_RANGE_16G;
    }
    else if (configuration == ImuConfiguration_TEST) {
        config.gyro.bandwidth = MPU6500_GYRO_BANDWIDTH_92Hz_3Ms9;
        config.gyro.range = MPU6500_GYRO_RANGE_250DPS;
        config.accel.bandwidth = MPU6500_ACCEL_BANDWIDTH_92Hz_7Ms8;
        config.accel.range = MPU6500_ACCEL_RANGE_2G;
    }

    // Perform reset
    MPU6500_ResetDevice();
    Micros_WaitMillis(100);
    MPU6500_ResetSignalPath(true, true, true);
    Micros_WaitMillis(100);

    // Check ID
    uint8_t id;
    if ((id = MPU6500_ReadID()) != 0x70) {
        printf("MPU6500 ID mismatch: expected 0x70, got 0x%x\n", id);
        retcode = -1;
    }

    MPU6500_Configure(&config);
    MPU6500_ConfigureInterruptPin(&intPinConfig);
    MPU6500_SetPowerMode(MPU6500_PowerMode_6AXIS);
    MPU6500_SetClockSource(MPU6500_ClockSource_AUTOPLL);
    MPU6500_ConfigureInterrupt(&intConfig);

    if (configuration == ImuConfiguration_APP) {
        MPU6500_SetOffset(&sensorOffset);
    }
    Micros_WaitMillis(100);

    SysTick_EnableInterrupt();
    return retcode;
}

int IMU_Test(void)
{
    int retcode = 0;
    struct ImuAverage testOn, testOff;

    SysTick_DisableInterrupt();
    LED1_ON();

    MPU6500_SelfTestOn(
        MPU6500_SELFTEST_XA | MPU6500_SELFTEST_YA | MPU6500_SELFTEST_ZA
        | MPU6500_SELFTEST_XG | MPU6500_SELFTEST_YG | MPU6500_SELFTEST_ZG
    );
    Micros_WaitMillis(25);
    GetAverages(&testOn, 1000, 100);

    MPU6500_SelfTestOff();
    Micros_WaitMillis(25);
    GetAverages(&testOff, 1000, 100);

    ImuDeviation deviation;
    GetDeviationFromFactoryTrim(&testOn, &testOff, &deviation);

    printf("MPU6500 factory trim deviation:\n");
    printf("aX:%ld%%\taY:%ld%%\taZ:%ld%%\tgX:%ld%%\tgY:%ld%%\tgZ:%ld%%\n",
        deviation.aX, deviation.aY, deviation.aZ,
        deviation.gX, deviation.gY, deviation.gZ
    );
    if (deviation.aX > 10 || deviation.aY > 10 || deviation.aZ > 10
            || deviation.gX > 10 || deviation.gY > 10 || deviation.gZ > 10) {
        retcode = -2;
    }

    LED1_OFF();
    SysTick_EnableInterrupt();

    return retcode;
}

void IMU_Calibrate(unsigned numIterations)
{
    SysTick_DisableInterrupt();
    LED1_ON();

    struct ImuAverage avg;
    GetAverages(&avg, 1000, 100);

    for (unsigned i = 1; i <= numIterations; i++) {
        ApplyOffsetsByAverage(&avg);
        GetAverages(&avg, 1000, 100);

        printf("MPU6500 averages, iteration %d:\n", i);
        printf("aX:%ld\taY:%ld\taZ:%ld\tgX:%ld\tgY:%ld\tgZ:%ld\n",
            avg.aX, avg.aY, avg.aZ,
            avg.gX, avg.gY, avg.gZ
        );
    }

    LED1_OFF();
    SysTick_EnableInterrupt();
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
    SysTick_DisableInterrupt();
    memcpy_v2n(data, &currentData, sizeof(struct IMU_Data));
    SysTick_EnableInterrupt();
}

static int execute(int argc, char *argv[])
{
    if (argc != 1)
        return -1;

    if (!strcmp(argv[0], "cal")) {
        IMU_Init(ImuConfiguration_CALIBRATION);
        IMU_Calibrate(5);
        IMU_Init(ImuConfiguration_APP);
    }
    else if (!strcmp(argv[0], "tst")) {
        IMU_Init(ImuConfiguration_TEST);
        IMU_Test();
        IMU_Init(ImuConfiguration_APP);
    }
    else if (!strcmp(argv[0], "ps")) {
        printf("IMU offsets:\n"
               "Accel: %d %d %d\n"
               "Gyro: %d %d %d\n",
               sensorOffset.accelX, sensorOffset.accelY, sensorOffset.accelZ,
               sensorOffset.gyroX, sensorOffset.gyroY, sensorOffset.gyroZ);
    }
    else
        return -2;

    return 0;
}

static void WriteTelemetry(char out[TELEMETRY_STRING_SIZE])
{
    struct IMU_Data imuData;
    IMU_GetData(&imuData);

    snprintf(out, TELEMETRY_STRING_SIZE,
        "gX:%-5d\tgY:%-5d\tgZ:%-5d\taX:%-5d\taY:%-5d\taZ:%-5d\n",
        imuData.gyroX, imuData.gyroY, imuData.gyroZ,
        imuData.accelX, imuData.accelY, imuData.accelZ
    );
}

static struct ModuleTelemetry telemetry = {
    .interval = 200,
    .write = WriteTelemetry
};

static void load(const uint8_t *buffer)
{
    memcpy(&sensorOffset, buffer, sizeof(sensorOffset));
}

static void save(uint8_t *buffer)
{
    memcpy(buffer, &sensorOffset, sizeof(sensorOffset));
}

static struct ModuleSettings settings = {
    .dataSize = sizeof(sensorOffset),
    .load = load,
    .save = save
};

struct Module IMU_module = {
    .name = "imu",
    .execute = execute,
    .telemetry = &telemetry,
    .settings = &settings
};