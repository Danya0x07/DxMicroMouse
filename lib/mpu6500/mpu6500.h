#ifndef _INC_MPU6500_H
#define _INC_MPU6500_H

#include <spi_io.h>
#include <gpio_ctl.h>
#include "mpu6500_conf.h"

struct MPU6500_Device {
    const SPI_Bus bus;
    const struct GPIOCTL_Line cs;
};

struct MPU6500_SelfTestData {
    uint8_t gyroX;
    uint8_t gyroY;
    uint8_t gyroZ;
    uint8_t accelX;
    uint8_t accelY;
    uint8_t accelZ;
};

struct MPU6500_SensorData {
    int16_t accelX;
    int16_t accelY;
    int16_t accelZ;
    int16_t temp;   // 333.87 LSB/degC, offset 21deg
    int16_t gyroX;
    int16_t gyroY;
    int16_t gyroZ;
};

struct MPU6500_Configuration {
    struct {
        PACKED enum {
            MPU6500_FIFO_MODE_REPLACING = 0,
            MPU6500_FIFO_MODE_KEEP
        } mode;
        bool writeTemp;
        bool writeGyroX;
        bool writeGyroY;
        bool writeGyroZ;
        bool writeAccelXYZ;
        bool writeSlave3;
        bool writeSlave2;
        bool writeSlave1;
        bool writeSlave0;
    } fifo;

    PACKED enum {
        MPU6500_FSYNCPOS_DISABLED = 0,
        MPU6500_FSYNCPOS_TEMP_L0,
        MPU6500_FSYNCPOS_GYRO_X_L0,
        MPU6500_FSYNCPOS_GYRO_Y_L0,
        MPU6500_FSYNCPOS_GYRO_Z_L0,
        MPU6500_FSYNCPOS_ACCEL_X_L0,
        MPU6500_FSYNCPOS_ACCEL_Y_L0,
        MPU6500_FSYNCPOS_ACCEL_Z_L0
    } fsyncPosition;

    struct {
        PACKED enum {
            MPU6500_GYRO_BANDWIDTH_250Hz_0Ms97 = 0, // Fs = 8kHz
            MPU6500_GYRO_BANDWIDTH_184Hz_2Ms9, // Fs = 1kHz
            MPU6500_GYRO_BANDWIDTH_92Hz_3Ms9, // Fs = 1kHz
            MPU6500_GYRO_BANDWIDTH_41Hz_5Ms9, // Fs = 1kHz
            MPU6500_GYRO_BANDWIDTH_20Hz_9Ms9, // Fs = 1kHz
            MPU6500_GYRO_BANDWIDTH_10Hz_17Ms85, // Fs = 1kHz
            MPU6500_GYRO_BANDWIDTH_5Hz_33Ms48, // Fs = 1kHz
            MPU6500_GYRO_BANDWIDTH_3600Hz_0Ms17, // Fs = 8kHz

            // If bypassing filter (Fs = 32kHz).
            MPU6500_GYRO_BANDWIDTH_8800Hz_0Ms064,
            MPU6500_GYRO_BANDWIDTH_3600Hz_0Ms11
        } bandwidth;

        PACKED enum {
            MPU6500_GYRO_RANGE_250DPS,  // 131 LSB / (deg/s)
            MPU6500_GYRO_RANGE_500DPS,  // 65.5 LSB / (deg/s)
            MPU6500_GYRO_RANGE_1000DPS, // 32.8 LSB / (deg/s)
            MPU6500_GYRO_RANGE_2000DPS  // 16.4 LSB / (deg/s)
        } range;
    } gyro;

    struct {
        PACKED enum {
            // Fs = 1 kHz
            MPU6500_ACCEL_BANDWIDTH_460Hz_1Ms94 = 0,
            MPU6500_ACCEL_BANDWIDTH_184Hz_5Ms8,
            MPU6500_ACCEL_BANDWIDTH_92Hz_7Ms8,
            MPU6500_ACCEL_BANDWIDTH_41Hz_11Ms8,
            MPU6500_ACCEL_BANDWIDTH_20Hz_19Ms8,
            MPU6500_ACCEL_BANDWIDTH_10Hz_35Ms7,
            MPU6500_ACCEL_BANDWIDTH_5Hz_66Ms96,

            // If bypassing filter (Fs = 4kHz).
            MPU6500_ACCEL_BANDWIDTH_1130Hz_0Ms75
        } bandwidth;

        PACKED enum {
            MPU6500_ACCEL_LPFREQUENCY_0Hz24 = 0,
            MPU6500_ACCEL_LPFREQUENCY_0Hz49,
            MPU6500_ACCEL_LPFREQUENCY_0Hz98,
            MPU6500_ACCEL_LPFREQUENCY_1Hz95,
            MPU6500_ACCEL_LPFREQUENCY_3Hz91,
            MPU6500_ACCEL_LPFREQUENCY_7Hz81,
            MPU6500_ACCEL_LPFREQUENCY_15Hz63,
            MPU6500_ACCEL_LPFREQUENCY_31Hz25,
            MPU6500_ACCEL_LPFREQUENCY_62Hz50,
            MPU6500_ACCEL_LPFREQUENCY_125Hz,
            MPU6500_ACCEL_LPFREQUENCY_250Hz,
            MPU6500_ACCEL_LPFREQUENCY_500Hz
        } lpfrequency;

        PACKED enum {
            MPU6500_ACCEL_RANGE_2G, // 16384 LSB / g
            MPU6500_ACCEL_RANGE_4G, // 8192 LSB / g
            MPU6500_ACCEL_RANGE_8G, // 4096 LSB / g
            MPU6500_ACCEL_RANGE_16G // 2048 LSB / g
        } range;
    } accel;
};

struct MPU6500_AuxMasterConfiguration {
    bool multimaster;
    bool waitForExternalData;
    bool stopBetweenReads; // false => restart between reads
    bool delayShadow;

    PACKED enum {
        MPU6500_AUXCLK_348kHz = 0,
        MPU6500_AUXCLK_333kHz,
        MPU6500_AUXCLK_320kHz,
        MPU6500_AUXCLK_308kHz,
        MPU6500_AUXCLK_296kHz,
        MPU6500_AUXCLK_286kHz,
        MPU6500_AUXCLK_276kHz,
        MPU6500_AUXCLK_267kHz,
        MPU6500_AUXCLK_258kHz,
        MPU6500_AUXCLK_500kHz,
        MPU6500_AUXCLK_471kHz,
        MPU6500_AUXCLK_444kHz,
        MPU6500_AUXCLK_421kHz,
        MPU6500_AUXCLK_400kHz,
        MPU6500_AUXCLK_381kHz,
        MPU6500_AUXCLK_364kHz
    } clock;

    // Slave with delay enabled will be accessed not every time (with sample rate) but every (1 + masterDelay) time.
    uint8_t accessDelay; // 0 - 31
};

typedef struct {
    uint8_t passThrough: 1;
    uint8_t slave4TransferFinished: 1;
    uint8_t lostArbitration: 1;
    uint8_t slave4Nack: 1;
    uint8_t slave3Nack: 1;
    uint8_t slave2Nack: 1;
    uint8_t slave1Nack: 1;
    uint8_t slave0Nack: 1;
} MPU6500_AuxMasterStatus;

enum MPU6500_AuxSlave {
    MPU6500_AUXSLAVE0 = 0,
    MPU6500_AUXSLAVE1,
    MPU6500_AUXSLAVE2,
    MPU6500_AUXSLAVE3,
};

typedef enum PACKED {
    MPU6500_AUXDIR_WRITE = 0,
    MPU6500_AUXDIR_READ
} MPU6500_AuxDirection;

struct MPU6500_AuxSlaveConfiguration {
    uint8_t address;
    MPU6500_AuxDirection transferDirection;
    uint8_t regaddr;
    bool byteSwap;
    bool skipReg;
    PACKED enum {
        MPU6500_BYTEGROUPING_ODD = 0, // 0 & 1, 2 & 3, ...
        MPU6500_BYTEGROUPING_EVEN, // 1 & 2, 3 & 4, ...
    } byteGrouping;
    uint8_t dataLen; // 0 - 15
    FunctionalState delay;
};

struct MPU6500_AuxSlave4Configuration {
    uint8_t address;
    FunctionalState delay;
};

struct MPU6500_InterruptPinConfiguration {
    bool activeLevelLow;
    bool openDrain; // otherwise push-pull
    bool latchUntilClear; // otherwise interrupt signal will be 50us pulse of active level
    bool anyReadClears; // otherwise interrupt clears only by MPU6500_ReadInterruptStatus().
    bool fsyncTransitionInterrupt;
    bool fsyncActiveLevelLow;
};

struct MPU6500_InterruptConfiguration {
    FunctionalState wakeOnMotion;
    FunctionalState fifoOverflow;
    FunctionalState fsyncTransition;
    FunctionalState rawDataReady;
};

typedef struct {
    uint8_t wakeOnMotion: 1;
    uint8_t fifoOverflow: 1;
    uint8_t fsyncTransition: 1;
    uint8_t dmp: 1;
    uint8_t rawDataReady: 1;
} MPU6500_InterruptStatus;

enum MPU6500_PowerMode {
    MPU6500_PowerMode_SLEEP,
    MPU6500_PowerMode_GYRO_STANDBY,
    MPU6500_PowerMode_ACCEL_LOWPOWER,
    MPU6500_PowerMode_ACCEL_LOWNOISE,
    MPU6500_PowerMode_GYRO_ONLY,
    MPU6500_PowerMode_6AXIS
};

struct MPU6500_SensorPower {
    FunctionalState tempSensor;
    FunctionalState accelX;
    FunctionalState accelY;
    FunctionalState accelZ;
    FunctionalState gyroX;
    FunctionalState gyroY;
    FunctionalState gyroZ;
};

enum MPU6500_ClockSource {
    MPU6500_ClockSource_INTERNAL_20MHZ = 0,
    MPU6500_ClockSource_AUTOPLL,
    MPU6500_ClockSource_STOP = 7
};

/** ID and General configuration and features. --------------------------------------------------- */
uint8_t MPU6500_ReadID(struct MPU6500_Device *dev); // Should be 0x70
void MPU6500_Configure(struct MPU6500_Device *dev, const struct MPU6500_Configuration *cfg);
void MPU6500_GetSensorData(struct MPU6500_Device *dev, struct MPU6500_SensorData *data);

/** Clock configuration. ------------------------------------------------------------------------- */
void MPU6500_SetClockSource(struct MPU6500_Device *dev, enum MPU6500_ClockSource clk);
// SAMPLE_RATE = 1kHz / (div + 1) */
void MPU6500_SetSampleRateDivider(struct MPU6500_Device *dev, uint8_t div);

/** Power configuration. ------------------------------------------------------------------------- */
void MPU6500_SetPowerMode(struct MPU6500_Device *dev, enum MPU6500_PowerMode mode);
void MPU6500_SetSensorsPower(struct MPU6500_Device *dev, const struct MPU6500_SensorPower *pwr);
void MPU6500_GetSensorsPower(struct MPU6500_Device *dev, struct MPU6500_SensorPower *pwr);

/** Interrupt feature configuration. ------------------------------------------------------------- */
void MPU6500_ConfigureInterruptPin(struct MPU6500_Device *dev, const struct MPU6500_InterruptPinConfiguration *cfg);
void MPU6500_ConfigureInterrupt(struct MPU6500_Device *dev, const struct MPU6500_InterruptConfiguration *cfg);
MPU6500_InterruptStatus MPU6500_GetInterruptStatus(struct MPU6500_Device *dev);

/** Wake on motion detection setup. -------------------------------------------------------------- */
void MPU6500_SetWakeOnMotionThreshold(struct MPU6500_Device *dev, uint8_t threshold); // LSB = 4mg
void MPU6500_SetupWakeOnMotionDetection(struct MPU6500_Device *dev, FunctionalState state,
                                        bool compareWithPreviousSample);

/** FIFO configuration. -------------------------------------------------------------------------- */
void MPU6500_SetFifoState(struct MPU6500_Device *dev, FunctionalState state);
uint16_t MPU6500_GetFifoCount(struct MPU6500_Device *dev);
uint8_t MPU6500_GetFifoData(struct MPU6500_Device *dev);

/** Calibration and self test features. ---------------------------------------------------------- */
#define MPU6500_SELFTEST_XG (1 << 7)
#define MPU6500_SELFTEST_YG (1 << 6)
#define MPU6500_SELFTEST_ZG (1 << 5)
#define MPU6500_SELFTEST_XA (1 << 4)
#define MPU6500_SELFTEST_YA (1 << 3)
#define MPU6500_SELFTEST_ZA (1 << 2)

void MPU6500_SelfTestOn(struct MPU6500_Device *dev, uint8_t selfTestMask);
#define MPU6500_SelfTestOff(dev)    MPU6500_SelfTestOn(dev, 0);

void MPU6500_GetSelfTestData(struct MPU6500_Device *dev, struct MPU6500_SelfTestData *selfTestData);
void MPU6500_GetOffset(struct MPU6500_Device *dev, struct MPU6500_SensorData *offset);
void MPU6500_SetOffset(struct MPU6500_Device *dev, const struct MPU6500_SensorData *offset);

/** Auxilary I2C master interface setup and features. -------------------------------------------- */
void MPU6500_ConfigureAuxMaster(struct MPU6500_Device *dev, const struct MPU6500_AuxMasterConfiguration *cfg);
void MPU6500_SetAuxMasterState(struct MPU6500_Device *dev, FunctionalState state);
void MPU6500_SetAuxBypassState(struct MPU6500_Device *dev, FunctionalState state);
MPU6500_AuxMasterStatus MPU6500_GetAuxMasterStatus(struct MPU6500_Device *dev);

/** Auxilary I2C slaves setup and features. ------------------------------------------------------ */
void MPU6500_ConfigureAuxSlave(struct MPU6500_Device *dev, enum MPU6500_AuxSlave slave,
                               const struct MPU6500_AuxSlaveConfiguration *cfg);
void MPU6500_SetAuxSlaveState(struct MPU6500_Device *dev, enum MPU6500_AuxSlave slave, FunctionalState state);
void MPU6500_SetAuxSlaveOutData(struct MPU6500_Device *dev, enum MPU6500_AuxSlave slave, uint8_t data); // when transferDirection == WRITE
void MPU6500_GetAuxSensorData(struct MPU6500_Device *dev, uint8_t *data, uint8_t len);

void MPU6500_ConfigureAuxSlave4(struct MPU6500_Device *dev, const struct MPU6500_AuxSlave4Configuration *cfg);
void MPU6500_RequestAuxSlave4Transfer(struct MPU6500_Device *dev, MPU6500_AuxDirection dir,
        uint8_t regAddr, bool skipReg, uint8_t outData, bool intOnFinish);
uint8_t MPU6500_GetAuxSlave4Data(struct MPU6500_Device *dev);

/** Reset functionality. ------------------------------------------------------------------------- */
void MPU6500_ResetSignalPath(struct MPU6500_Device *dev, bool accel, bool gyro, bool temp);
void MPU6500_ResetSensors(struct MPU6500_Device *dev);
void MPU6500_ResetFIFO(struct MPU6500_Device *dev);
void MPU6500_ResetPrimaryI2C(struct MPU6500_Device *dev);
void MPU6500_ResetAuxilaryI2C(struct MPU6500_Device *dev);
void MPU6500_ResetDMP(struct MPU6500_Device *dev);

/* When using SPI interface, user should use PWR_MGMT_1 (register 107) as well as
 * SIGNAL_PATH_RESET (register 104) to ensure the reset is performed properly. The sequence
 * used should be:
 *  MPU6500_ResetDevice();
 *  Wait 100 ms;
 *  MPU6500_ResetSignalPath(1, 1, 1);
 *  Wait 100 ms;
 */
void MPU6500_ResetDevice(struct MPU6500_Device *dev);

/** DMP features (currently not supported, may be continued) */
void MPU6500_SetDmpState(struct MPU6500_Device *dev, FunctionalState state);

#endif // _INC_MPU6500_H