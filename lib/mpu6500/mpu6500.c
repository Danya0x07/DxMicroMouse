#include "mpu6500.h"

#define REG_SELF_TEST_X_GYRO    0x00
#define REG_SELF_TEST_Y_GYRO    0x01
#define REG_SELF_TEST_Z_GYRO    0x02
#define REG_SELF_TEST_X_ACCEL   0x0D
#define REG_SELF_TEST_Y_ACCEL   0x0E
#define REG_SELF_TEST_Z_ACCEL   0x0F

#define REG_XG_OFFSET_H 0x13
#define REG_XG_OFFSET_L 0x14
#define REG_YG_OFFSET_H 0x15
#define REG_YG_OFFSET_L 0x16
#define REG_ZG_OFFSET_H 0x17
#define REG_ZG_OFFSET_L 0x18

#define REG_SMPLRT_DIV  0x19

#define REG_CONFIG      0x1A
#define BS_FIFO_MODE    6
#define BS_EXT_SYNC_SET 3
#define BS_DPLF_CFG     0

#define REG_GYRO_CONFIG     0x1B
#define BS_XG_ST        7
#define BS_YG_ST        6
#define BS_ZG_ST        5
#define BS_GYRO_FS_SEL  3
#define BS_FCHOICE_B    0

#define REG_ACCEL_CONFIG    0x1C
#define BS_XA_ST        7
#define BS_YA_ST        6
#define BS_ZA_ST        5
#define BS_ACCEL_FS_SEL 3

#define REG_ACCEL_CONFIG_2  0x1D
#define BS_ACCEL_FCHOICE_B  3
#define BS_A_DPLF_CFG       0

#define REG_LP_ACCEL_ODR    0x1E
#define BS_LPOSC_CLKSEL     0

#define REG_WOM_THR     0x1F

#define REG_FIFO_EN     0x23
#define BS_TEMP_FIFO_EN 7
#define BS_GYRO_XOUT    6
#define BS_GYRO_YOUT    5
#define BS_GYRO_ZOUT    4
#define BS_ACCEL        3
#define BS_SLV2     2
#define BS_SLV1     1
#define BS_SLV0     0

#define REG_I2C_MST_CTRL    0x24
#define BS_MULT_MST_EN  7
#define BS_WAIT_FOR_ES  6
#define BS_SLV3_FIFO_EN 5
#define BS_I2C_MST_P_NSR    4
#define BS_I2C_MST_CLK  0

#define REG_I2C_SLV0_ADDR   0x25
#define REG_I2C_SLV0_REG    0x26
#define REG_I2C_SLV0_CTRL   0x27
#define BS_I2C_SLV0_EN  7
#define BS_I2C_SLV0_BYTE_SW 6
#define BS_I2C_SLV0_REG_DIS 5
#define BS_I2C_SLV0_GRP     4
#define BS_I2C_SLV0_LENG    0

#define REG_I2C_SLV1_ADDR   0x28
#define REG_I2C_SLV1_REG    0x29
#define REG_I2C_SLV1_CTRL   0x2A
#define BS_I2C_SLV1_EN  7
#define BS_I2C_SLV1_BYTE_SW 6
#define BS_I2C_SLV1_REG_DIS 5
#define BS_I2C_SLV1_GRP     4
#define BS_I2C_SLV1_LENG    0

#define REG_I2C_SLV2_ADDR   0x2B
#define REG_I2C_SLV2_REG    0x2C
#define REG_I2C_SLV2_CTRL   0x2D
#define BS_I2C_SLV2_EN  7
#define BS_I2C_SLV2_BYTE_SW 6
#define BS_I2C_SLV2_REG_DIS 5
#define BS_I2C_SLV2_GRP     4
#define BS_I2C_SLV2_LENG    0

#define REG_I2C_SLV3_ADDR   0x2E
#define REG_I2C_SLV3_REG    0x2F
#define REG_I2C_SLV3_CTRL   0x30
#define BS_I2C_SLV3_EN  7
#define BS_I2C_SLV3_BYTE_SW 6
#define BS_I2C_SLV3_REG_DIS 5
#define BS_I2C_SLV3_GRP     4
#define BS_I2C_SLV3_LENG    0

#define REG_I2C_SLV4_ADDR   0x31
#define REG_I2C_SLV4_REG    0x32
#define REG_I2C_SLV4_DO     0x33
#define REG_I2C_SLV4_CTRL   0x34
#define BS_I2C_SLV4_EN  7
#define BS_SLV4_DONE_INT_EN 6
#define BS_I2C_SLV4_REG_DIS 5
#define BS_I2C_MST_DLY  0

#define REG_I2C_SLV4_DI     0x35

#define REG_MST_STATUS      0x36
#define BS_PASS_THROUGH     7
#define BS_I2C_SLV4_DONE    6
#define BS_I2C_LOST_ARB     5
#define BS_I2C_SLV4_NACK    4
#define BS_I2C_SLV3_NACK    3
#define BS_I2C_SLV2_NACK    2
#define BS_I2C_SLV1_NACK    1
#define BS_I2C_SLV0_NACK    0

#define REG_INT_PIN_CFG     0x37
#define BS_ACTL     7
#define BS_OPEN     6
#define BS_LATCH_INT_EN     5
#define BS_INT_ANYRD_2CLEAR 4
#define BS_ACTL_FSYNC       3
#define BS_FSYNC_INT_MODE_EN    2
#define BS_BYPASS_EN        1

#define REG_INT_ENABLE      0x38
#define BS_WOM_EN   6
#define BS_FIFO_OFLOW_EN    4
#define BS_FSYNC_INT_EN     3
#define BS_RAW_RDY_EN       0

#define REG_INT_STATUS      0x3A
#define BS_WOM_INT  6
#define BS_FIFO_OFLOW_INT   4
#define BS_FSYNC_INT        3
#define BS_DMP_INT  1
#define BS_RAW_DATA_RDY_INT 0

#define REG_ACCEL_XOUT_H    0x3B
#define REG_ACCEL_XOUT_L    0x3C
#define REG_ACCEL_YOUT_H    0x3D
#define REG_ACCEL_YOUT_L    0x3E
#define REG_ACCEL_ZOUT_H    0x3F
#define REG_ACCEL_ZOUT_L    0x40
#define REG_TEMP_OUT_H      0x41
#define REG_TEMP_OUT_L      0x42
#define REG_GYRO_XOUT_H     0x43
#define REG_GYRO_XOUT_L     0x44
#define REG_GYRO_YOUT_H     0x45
#define REG_GYRO_YOUT_L     0x46
#define REG_GYRO_ZOUT_H     0x47
#define REG_GYRO_ZOUT_L     0x48

#define REG_EXT_SENS_DATA_00    0x49
#define REG_EXT_SENS_DATA_01    0x4A
#define REG_EXT_SENS_DATA_02    0x4B
#define REG_EXT_SENS_DATA_03    0x4C
#define REG_EXT_SENS_DATA_04    0x4D
#define REG_EXT_SENS_DATA_05    0x4E
#define REG_EXT_SENS_DATA_06    0x4F
#define REG_EXT_SENS_DATA_07    0x50
#define REG_EXT_SENS_DATA_08    0x51
#define REG_EXT_SENS_DATA_09    0x52
#define REG_EXT_SENS_DATA_10    0x53
#define REG_EXT_SENS_DATA_11    0x54
#define REG_EXT_SENS_DATA_12    0x55
#define REG_EXT_SENS_DATA_13    0x56
#define REG_EXT_SENS_DATA_14    0x57
#define REG_EXT_SENS_DATA_15    0x58
#define REG_EXT_SENS_DATA_16    0x59
#define REG_EXT_SENS_DATA_17    0x5A
#define REG_EXT_SENS_DATA_18    0x5B
#define REG_EXT_SENS_DATA_19    0x5C
#define REG_EXT_SENS_DATA_20    0x5D
#define REG_EXT_SENS_DATA_21    0x5E
#define REG_EXT_SENS_DATA_22    0x5F
#define REG_EXT_SENS_DATA_23    0x60

#define REG_I2C_SLV0_DO     0x63
#define REG_I2C_SLV1_DO     0x64
#define REG_I2C_SLV2_DO     0x65
#define REG_I2C_SLV3_DO     0x66

#define REG_I2C_MST_DELAY_CTRL  0x67
#define BS_DELAY_ES_SHADOW  7
#define BS_I2C_SLV4_DLY_EN  4
#define BS_I2C_SLV3_DLY_EN  3
#define BS_I2C_SLV2_DLY_EN  2
#define BS_I2C_SLV1_DLY_EN  1
#define BS_I2C_SLV0_DLY_EN  0

#define REG_SIGNAL_PATH_RESET   0x68
#define BS_GYRO_RST     2
#define BS_ACCEL_RST    1
#define BS_TEMP_RST     0

#define REG_ACCEL_INTEL_CTRL    0x69
#define BS_ACCEL_INTEL_EN   7
#define BS_ACCEL_INTEL_MODE 6

#define REG_USER_CTRL       0x6A
#define BS_DMP_EN       7
#define BS_FIFO_EN      6
#define BS_I2C_MST_EN   5
#define BS_I2C_IF_DIS   4
#define BS_DMP_RST      3
#define BS_FIFO_RST     2
#define BS_I2C_MST_RST  1
#define BS_SIG_COND_RST 0

#define REG_PWR_MGMT_1      0x6B
#define BS_DEVICE_RESET 7
#define BS_SLEEP        6
#define BS_CYCLE        5
#define BS_GYRO_STANDBY 4
#define BS_TEMP_DIS     3
#define BS_CLKSEL       0

#define REG_PWR_MGMT_2      0x6C
#define BS_DIS_XA       5
#define BS_DIS_YA       4
#define BS_DIS_ZA       3
#define BS_DIS_XG       2
#define BS_DIS_YG       1
#define BS_DIS_ZG       0

#define REG_FIFO_COUNT_H    0x72
#define REG_FIFO_COUNT_L    0x73
#define REG_FIFO_R_W        0x74

#define REG_WHO_AM_I        0x75

#define REG_XA_OFFSET_H     0x77
#define REG_XA_OFFSET_L     0x78
#define REG_YA_OFFSET_H     0x7A
#define REG_YA_OFFSET_L     0x7B
#define REG_ZA_OFFSET_H     0x7D
#define REG_ZA_OFFSET_L     0x7E

static void WriteRegister(struct MPU6500_Device *dev, uint8_t reg, uint8_t data)
{
    uint8_t out[2] = {reg & 0x7F, data};

    SPI_BeginTransfer(dev->bus);
    GPIOCTL_Low(&dev->cs);
    SPI_TransferData(dev->bus, NULL, out, 2);
    GPIOCTL_High(&dev->cs);
    SPI_EndTransfer(dev->bus);
}

static uint8_t ReadRegister(struct MPU6500_Device *dev, uint8_t reg)
{
    uint8_t out = 0x80 | reg;
    uint8_t in;

    SPI_BeginTransfer(dev->bus);
    GPIOCTL_Low(&dev->cs);
    SPI_TransferData(dev->bus, NULL, &out, 1);
    SPI_TransferData(dev->bus, &in, NULL, 1);
    GPIOCTL_High(&dev->cs);
    SPI_EndTransfer(dev->bus);

    return in;
}

static void WriteBits(struct MPU6500_Device *dev, uint8_t reg, uint8_t maskDisable, uint8_t maskEnable)
{
    uint8_t data = ReadRegister(dev, reg);
    data &= ~maskDisable;
    data |= maskEnable;
    WriteRegister(dev, reg, data);
}

static void SetBits(struct MPU6500_Device *dev, uint8_t reg, uint8_t mask, FunctionalState state)
{
    if (state == ENABLE)
        WriteBits(dev, reg, 0, mask);
    else
        WriteBits(dev, reg, mask, 0);
}

static void ReadBuffer(struct MPU6500_Device *dev, uint8_t reg, uint8_t *buff, uint8_t len)
{
    reg |= 0x80;

    SPI_BeginTransfer(dev->bus);
    GPIOCTL_Low(&dev->cs);
    SPI_TransferData(dev->bus, NULL, &reg, 1);
    SPI_TransferData(dev->bus, buff, NULL, len);
    GPIOCTL_High(&dev->cs);
    SPI_EndTransfer(dev->bus);
}

static void WriteBuffer(struct MPU6500_Device *dev, uint8_t reg, const uint8_t *buff, uint8_t len)
{
    reg &= 0x7F;

    SPI_BeginTransfer(dev->bus);
    GPIOCTL_Low(&dev->cs);
    SPI_TransferData(dev->bus, NULL, &reg, 1);
    SPI_TransferData(dev->bus, NULL, buff, len);
    GPIOCTL_High(&dev->cs);
    SPI_EndTransfer(dev->bus);
}

uint8_t MPU6500_ReadID(struct MPU6500_Device *dev)
{
    return ReadRegister(dev, REG_WHO_AM_I);
}

void MPU6500_Configure(struct MPU6500_Device *dev, const struct MPU6500_Configuration *cfg)
{
    uint8_t tmp;

    WriteRegister(dev, REG_CONFIG,
        cfg->fifo.mode << BS_FIFO_MODE
        | cfg->fsyncPosition << BS_EXT_SYNC_SET
        | cfg->gyro.bandwidth << BS_DPLF_CFG
    );

    tmp = cfg->gyro.bandwidth == MPU6500_GYRO_BANDWIDTH_3600Hz_0Ms11 ? 2 :
            cfg->gyro.bandwidth == MPU6500_GYRO_BANDWIDTH_8800Hz_0Ms064 ? 1 : 0;
    WriteRegister(dev, REG_GYRO_CONFIG, cfg->gyro.range << BS_GYRO_FS_SEL | tmp << BS_FCHOICE_B);

    WriteRegister(dev, REG_ACCEL_CONFIG, cfg->accel.range << BS_ACCEL_FS_SEL);
    WriteRegister(dev, REG_ACCEL_CONFIG_2,
        (cfg->accel.bandwidth == MPU6500_ACCEL_BANDWIDTH_1130Hz_0Ms75) << BS_ACCEL_FCHOICE_B
        | cfg->accel.bandwidth << BS_A_DPLF_CFG
    );
    WriteRegister(dev, REG_LP_ACCEL_ODR, cfg->accel.lpfrequency << BS_LPOSC_CLKSEL);

    WriteRegister(dev, REG_FIFO_EN,
        cfg->fifo.writeTemp << BS_TEMP_FIFO_EN
        | cfg->fifo.writeGyroX << BS_GYRO_XOUT
        | cfg->fifo.writeGyroY << BS_GYRO_YOUT
        | cfg->fifo.writeGyroZ << BS_GYRO_ZOUT
        | cfg->fifo.writeAccelXYZ << BS_ACCEL
        | cfg->fifo.writeSlave2 << BS_SLV2
        | cfg->fifo.writeSlave1 << BS_SLV1
        | cfg->fifo.writeSlave0 << BS_SLV0
    );
    SetBits(dev, REG_I2C_MST_CTRL, 1 << BS_SLV3_FIFO_EN, (FunctionalState)cfg->fifo.writeSlave3);
}

void MPU6500_GetSensorData(struct MPU6500_Device *dev, struct MPU6500_SensorData *data)
{
    uint8_t buff[14];

    ReadBuffer(dev, REG_ACCEL_XOUT_H, buff, sizeof(buff));
    data->accelX = (int16_t)buff[0] << 8 | buff[1];
    data->accelY = (int16_t)buff[2] << 8 | buff[3];
    data->accelZ = (int16_t)buff[4] << 8 | buff[5];
    data->temp  = (int16_t)buff[6] << 8 | buff[7];
    data->gyroX = (int16_t)buff[8] << 8 | buff[9];
    data->gyroY = (int16_t)buff[10] << 8 | buff[11];
    data->gyroZ = (int16_t)buff[12] << 8 | buff[13];
}

void MPU6500_SetClockSource(struct MPU6500_Device *dev, enum MPU6500_ClockSource clk)
{
    WriteBits(dev, REG_PWR_MGMT_1, 0x7 << BS_CLKSEL, clk << BS_CLKSEL);
}

void MPU6500_SetSampleRateDivider(struct MPU6500_Device *dev, uint8_t div)
{
    WriteRegister(dev, REG_SMPLRT_DIV, div);
}

void MPU6500_SetPowerMode(struct MPU6500_Device *dev, enum MPU6500_PowerMode mode)
{
    struct MPU6500_SensorPower power;

    switch (mode) {

    case MPU6500_PowerMode_SLEEP:
        power = (struct MPU6500_SensorPower){DISABLE};
        MPU6500_SetSensorsPower(dev, &power);
        MPU6500_SetDmpState(dev, DISABLE);
        SetBits(dev, REG_PWR_MGMT_1, 1 << BS_SLEEP, ENABLE);
        SetBits(dev, REG_PWR_MGMT_1, 1 << BS_GYRO_STANDBY, DISABLE);
        SetBits(dev, REG_PWR_MGMT_1, 1 << BS_CYCLE, DISABLE);
        break;

    case MPU6500_PowerMode_GYRO_STANDBY:
        power.tempSensor = DISABLE;
        power.accelX = power.accelY = power.accelZ = DISABLE;
        power.gyroX = power.gyroY = power.gyroZ = ENABLE;
        MPU6500_SetSensorsPower(dev, &power);
        MPU6500_SetDmpState(dev, DISABLE);
        SetBits(dev, REG_PWR_MGMT_1, 1 << BS_SLEEP, DISABLE);
        SetBits(dev, REG_PWR_MGMT_1, 1 << BS_GYRO_STANDBY, ENABLE);
        SetBits(dev, REG_PWR_MGMT_1, 1 << BS_CYCLE, DISABLE);
        break;

    case MPU6500_PowerMode_ACCEL_LOWPOWER:
        power.tempSensor = DISABLE;
        power.accelX = power.accelY = power.accelZ = ENABLE;
        power.gyroX = power.gyroY = power.gyroZ = DISABLE;
        MPU6500_SetSensorsPower(dev, &power);
        MPU6500_SetDmpState(dev, DISABLE);
        SetBits(dev, REG_PWR_MGMT_1, 1 << BS_SLEEP, DISABLE);
        SetBits(dev, REG_PWR_MGMT_1, 1 << BS_GYRO_STANDBY, DISABLE);
        SetBits(dev, REG_PWR_MGMT_1, 1 << BS_CYCLE, ENABLE);
        break;

    case MPU6500_PowerMode_ACCEL_LOWNOISE:
        power.tempSensor = ENABLE;
        power.accelX = power.accelY = power.accelZ = ENABLE;
        power.gyroX = power.gyroY = power.gyroZ = DISABLE;
        MPU6500_SetSensorsPower(dev, &power);
        MPU6500_SetDmpState(dev, DISABLE);
        SetBits(dev, REG_PWR_MGMT_1, 1 << BS_SLEEP, DISABLE);
        SetBits(dev, REG_PWR_MGMT_1, 1 << BS_GYRO_STANDBY, DISABLE);
        SetBits(dev, REG_PWR_MGMT_1, 1 << BS_CYCLE, DISABLE);
        break;

    case MPU6500_PowerMode_GYRO_ONLY:
        power.tempSensor = ENABLE;
        power.accelX = power.accelY = power.accelZ = DISABLE;
        power.gyroX = power.gyroY = power.gyroZ = ENABLE;
        MPU6500_SetSensorsPower(dev, &power);
        SetBits(dev, REG_PWR_MGMT_1, 1 << BS_SLEEP, DISABLE);
        SetBits(dev, REG_PWR_MGMT_1, 1 << BS_GYRO_STANDBY, DISABLE);
        SetBits(dev, REG_PWR_MGMT_1, 1 << BS_CYCLE, DISABLE);
        break;

    case MPU6500_PowerMode_6AXIS:
        power.tempSensor = ENABLE;
        power.accelX = power.accelY = power.accelZ = ENABLE;
        power.gyroX = power.gyroY = power.gyroZ = ENABLE;
        MPU6500_SetSensorsPower(dev, &power);
        SetBits(dev, REG_PWR_MGMT_1, 1 << BS_SLEEP, DISABLE);
        SetBits(dev, REG_PWR_MGMT_1, 1 << BS_GYRO_STANDBY, DISABLE);
        SetBits(dev, REG_PWR_MGMT_1, 1 << BS_CYCLE, DISABLE);
        break;
    }
}

void MPU6500_SetSensorsPower(struct MPU6500_Device *dev, const struct MPU6500_SensorPower *pwr)
{
    WriteRegister(dev, REG_PWR_MGMT_2,
        !pwr->accelX << BS_DIS_XA
        | !pwr->accelY << BS_DIS_YA
        | !pwr->accelZ << BS_DIS_ZA
        | !pwr->gyroX << BS_DIS_XG
        | !pwr->gyroY << BS_DIS_YG
        | !pwr->gyroZ << BS_DIS_ZG
    );
    SetBits(dev, REG_PWR_MGMT_1, 1 << BS_TEMP_DIS, (FunctionalState)!pwr->tempSensor);
}

void MPU6500_GetSensorsPower(struct MPU6500_Device *dev, struct MPU6500_SensorPower *pwr)
{
    uint8_t value = ReadRegister(dev, REG_PWR_MGMT_1);
    pwr->tempSensor = (FunctionalState)((value & 1 << BS_TEMP_DIS) == 0);

    value = ReadRegister(dev, REG_PWR_MGMT_2);
    pwr->accelX = (FunctionalState)((value & 1 << BS_DIS_XA) == 0);
    pwr->accelY = (FunctionalState)((value & 1 << BS_DIS_YA) == 0);
    pwr->accelZ = (FunctionalState)((value & 1 << BS_DIS_ZA) == 0);
    pwr->gyroX  = (FunctionalState)((value & 1 << BS_DIS_XG) == 0);
    pwr->gyroY  = (FunctionalState)((value & 1 << BS_DIS_YG) == 0);
    pwr->gyroZ  = (FunctionalState)((value & 1 << BS_DIS_ZG) == 0);
}

void MPU6500_ConfigureInterruptPin(struct MPU6500_Device *dev, const struct MPU6500_InterruptPinConfiguration *cfg)
{
    WriteBits(dev, REG_INT_PIN_CFG, 0xFC,
        cfg->activeLevelLow << BS_ACTL
        | cfg->openDrain << BS_OPEN
        | cfg->latchUntilClear << BS_LATCH_INT_EN
        | cfg->anyReadClears << BS_INT_ANYRD_2CLEAR
        | cfg->fsyncTransitionInterrupt << BS_FSYNC_INT_MODE_EN
        | cfg->fsyncActiveLevelLow << BS_ACTL_FSYNC
    );
}

void MPU6500_ConfigureInterrupt(struct MPU6500_Device *dev, const struct MPU6500_InterruptConfiguration *cfg)
{
    WriteBits(dev, REG_INT_ENABLE, 0x59,
        cfg->wakeOnMotion << BS_WOM_EN
        | cfg->fifoOverflow << BS_FIFO_OFLOW_EN
        | cfg->fsyncTransition << BS_FSYNC_INT_EN
        | cfg->rawDataReady << BS_RAW_RDY_EN
    );
}

MPU6500_InterruptStatus MPU6500_GetInterruptStatus(struct MPU6500_Device *dev)
{
    uint8_t value = ReadRegister(dev, REG_INT_STATUS);
    MPU6500_InterruptStatus status;

    status.wakeOnMotion = !!(value & 1 << BS_WOM_INT);
    status.fifoOverflow = !!(value & 1 << BS_FIFO_OFLOW_INT);
    status.fsyncTransition = !!(value & 1 << BS_FSYNC_INT);
    status.dmp = !!(value & 1 << BS_DMP_INT);
    status.rawDataReady = !!(value & 1 << BS_RAW_DATA_RDY_INT);

    return status;
}

void MPU6500_SetWakeOnMotionThreshold(struct MPU6500_Device *dev, uint8_t threshold)
{
    WriteRegister(dev, REG_WOM_THR, threshold);
}

void MPU6500_SetupWakeOnMotionDetection(struct MPU6500_Device *dev, FunctionalState state,
                                        bool compareWithPreviousSample)
{
    WriteRegister(dev, REG_ACCEL_INTEL_CTRL,
        state << BS_ACCEL_INTEL_EN
        | compareWithPreviousSample << BS_ACCEL_INTEL_MODE
    );
}

void MPU6500_SetFifoState(struct MPU6500_Device *dev, FunctionalState state)
{
    SetBits(dev, REG_USER_CTRL, 1 << BS_FIFO_EN, state);
}

uint16_t MPU6500_GetFifoCount(struct MPU6500_Device *dev)
{
    uint8_t buff[2];
    ReadBuffer(dev, REG_FIFO_COUNT_H, buff, sizeof(buff));
    return (uint16_t)buff[0] << 8 | buff[1];
}

uint8_t MPU6500_GetFifoData(struct MPU6500_Device *dev)
{
    return ReadRegister(dev, REG_FIFO_R_W);
}

void MPU6500_SelfTestOn(struct MPU6500_Device *dev, uint8_t selfTestMask)
{
    uint8_t gyroMask = selfTestMask & 0xE0;
    uint8_t accelMask = selfTestMask << 3 & 0xE0;
    WriteBits(dev, REG_GYRO_CONFIG, 0xE0, gyroMask);
    WriteBits(dev, REG_ACCEL_CONFIG, 0xE0, accelMask);
}

void MPU6500_GetSelfTestData(struct MPU6500_Device *dev, struct MPU6500_SelfTestData *selfTestData)
{
    uint8_t buff[3];

    ReadBuffer(dev, REG_SELF_TEST_X_GYRO, buff, sizeof(buff));
    selfTestData->gyroX = buff[0];
    selfTestData->gyroY = buff[1];
    selfTestData->gyroZ = buff[2];

    ReadBuffer(dev, REG_SELF_TEST_X_ACCEL, buff, sizeof(buff));
    selfTestData->accelX = buff[0];
    selfTestData->accelY = buff[1];
    selfTestData->accelZ = buff[2];
}

void MPU6500_GetOffset(struct MPU6500_Device *dev, struct MPU6500_SensorData *offset)
{
    uint8_t buff[2];

    ReadBuffer(dev, REG_XG_OFFSET_H, buff, sizeof(buff));
    offset->gyroX = (int16_t)buff[0] << 8 | buff[1];

    ReadBuffer(dev, REG_YG_OFFSET_H, buff, sizeof(buff));
    offset->gyroY = (int16_t)buff[0] << 8 | buff[1];

    ReadBuffer(dev, REG_ZG_OFFSET_H, buff, sizeof(buff));
    offset->gyroZ = (int16_t)buff[0] << 8 | buff[1];

    ReadBuffer(dev, REG_XA_OFFSET_H, buff, sizeof(buff));
    offset->accelX = (int16_t)buff[0] << 8 | buff[1];

    ReadBuffer(dev, REG_YA_OFFSET_H, buff, sizeof(buff));
    offset->accelY = (int16_t)buff[0] << 8 | buff[1];

    ReadBuffer(dev, REG_ZA_OFFSET_H, buff, sizeof(buff));
    offset->accelZ = (int16_t)buff[0] << 8 | buff[1];

    offset->temp = 0;
}

void MPU6500_SetOffset(struct MPU6500_Device *dev, const struct MPU6500_SensorData *offset)
{
    uint8_t buff[6];

    buff[0] = (uint8_t)(offset->gyroX >> 8);
    buff[1] = (uint8_t)(offset->gyroX & 0xFF);
    buff[2] = (uint8_t)(offset->gyroY >> 8);
    buff[3] = (uint8_t)(offset->gyroY & 0xFF);
    buff[4] = (uint8_t)(offset->gyroZ >> 8);
    buff[5] = (uint8_t)(offset->gyroZ & 0xFF);
    WriteBuffer(dev, REG_XG_OFFSET_H, buff, sizeof(buff));

    buff[0] = (uint8_t)(offset->accelX >> 8);
    buff[1] = (uint8_t)(offset->accelX & 0xFF);
    buff[2] = (uint8_t)(offset->accelY >> 8);
    buff[3] = (uint8_t)(offset->accelY & 0xFF);
    buff[4] = (uint8_t)(offset->accelZ >> 8);
    buff[5] = (uint8_t)(offset->accelZ & 0xFF);
    WriteRegister(dev, REG_XA_OFFSET_H, buff[0]);
    WriteBits(dev, REG_XA_OFFSET_L, 0xFE, buff[1] & 0xFE);
    WriteRegister(dev, REG_YA_OFFSET_H, buff[2]);
    WriteBits(dev, REG_YA_OFFSET_L, 0xFE, buff[3] & 0xFE);
    WriteRegister(dev, REG_ZA_OFFSET_H, buff[4]);
    WriteBits(dev, REG_ZA_OFFSET_L, 0xFE, buff[5] & 0xFE);
}

void MPU6500_ConfigureAuxMaster(struct MPU6500_Device *dev, const struct MPU6500_AuxMasterConfiguration *cfg)
{
    WriteBits(dev, REG_I2C_MST_CTRL, ~(1 << BS_SLV3_FIFO_EN),
        cfg->multimaster << BS_MULT_MST_EN
        | cfg->waitForExternalData << BS_WAIT_FOR_ES
        | cfg->stopBetweenReads << BS_I2C_MST_P_NSR
        | cfg->clock << BS_I2C_MST_CLK
    );

    WriteBits(dev, REG_I2C_SLV4_CTRL, 0x1F, cfg->accessDelay & 0x1F);
    SetBits(dev, REG_I2C_MST_DELAY_CTRL, 1 << BS_DELAY_ES_SHADOW, (FunctionalState)cfg->delayShadow);
}

void MPU6500_SetAuxMasterState(struct MPU6500_Device *dev, FunctionalState state)
{
    SetBits(dev, REG_USER_CTRL, 1 << BS_I2C_MST_EN, state);
}

void MPU6500_SetAuxBypassState(struct MPU6500_Device *dev, FunctionalState state)
{
    SetBits(dev, REG_INT_PIN_CFG, 1 << BS_BYPASS_EN, state);
}

MPU6500_AuxMasterStatus MPU6500_GetAuxMasterStatus(struct MPU6500_Device *dev)
{
    MPU6500_AuxMasterStatus status;

    uint8_t value = ReadRegister(dev, REG_MST_STATUS);
    status.passThrough = !!(value & 1 << BS_PASS_THROUGH);
    status.slave4TransferFinished = !!(value & 1 << BS_I2C_SLV4_DONE);
    status.lostArbitration = !!(value & 1 << BS_I2C_LOST_ARB);
    status.slave4Nack = !!(value & 1 << BS_I2C_SLV4_NACK);
    status.slave3Nack = !!(value & 1 << BS_I2C_SLV3_NACK);
    status.slave2Nack = !!(value & 1 << BS_I2C_SLV2_NACK);
    status.slave1Nack = !!(value & 1 << BS_I2C_SLV1_NACK);
    status.slave0Nack = !!(value & 1 << BS_I2C_SLV0_NACK);

    return status;
}

void MPU6500_ConfigureAuxSlave(struct MPU6500_Device *dev, enum MPU6500_AuxSlave slave,
                               const struct MPU6500_AuxSlaveConfiguration *cfg)
{
    uint8_t regAddr = REG_I2C_SLV0_ADDR + 3 * slave;
    uint8_t regReg = regAddr + 1;
    uint8_t regCtrl = regAddr + 2;

    WriteRegister(dev, regAddr, cfg->address | cfg->transferDirection << 7);
    WriteRegister(dev, regReg, cfg->regaddr);
    WriteRegister(dev, regCtrl,
        cfg->byteSwap << BS_I2C_SLV0_BYTE_SW
        | cfg->skipReg << BS_I2C_SLV0_REG_DIS
        | cfg->byteGrouping << BS_I2C_SLV0_GRP
        | cfg->dataLen << BS_I2C_SLV0_LENG
    );
    SetBits(dev, REG_I2C_MST_DELAY_CTRL, 1 << slave, cfg->delay);
}

void MPU6500_SetAuxSlaveState(struct MPU6500_Device *dev, enum MPU6500_AuxSlave slave, FunctionalState state)
{
    uint8_t regCtrl = REG_I2C_SLV0_ADDR + 3 * slave + 2;
    SetBits(dev, regCtrl, 1 << BS_I2C_SLV0_EN, state);
}

void MPU6500_SetAuxSlaveOutData(struct MPU6500_Device *dev, enum MPU6500_AuxSlave slave, uint8_t data)
{
    uint8_t regDataOut = REG_I2C_SLV0_DO + slave;
    WriteRegister(dev, regDataOut, data);
}

void MPU6500_GetAuxSensorData(struct MPU6500_Device *dev, uint8_t *data, uint8_t len)
{
    ReadBuffer(dev, REG_EXT_SENS_DATA_00, data, len);
}

void MPU6500_ConfigureAuxSlave4(struct MPU6500_Device *dev, const struct MPU6500_AuxSlave4Configuration *cfg)
{
    WriteRegister(dev, REG_I2C_SLV4_ADDR, cfg->address);
    SetBits(dev, REG_I2C_MST_DELAY_CTRL, 1 << BS_I2C_SLV4_DLY_EN, cfg->delay);
}

void MPU6500_RequestAuxSlave4Transfer(struct MPU6500_Device *dev, MPU6500_AuxDirection dir,
        uint8_t regAddr, bool skipReg, uint8_t outData, bool intOnFinish)
{
    SetBits(dev, REG_I2C_SLV4_ADDR, 1 << 7, (FunctionalState)dir);
    WriteRegister(dev, REG_I2C_SLV4_REG, regAddr);
    WriteRegister(dev, REG_I2C_SLV4_DO, outData);
    WriteBits(dev, REG_I2C_SLV4_CTRL, 0xE0, 0x80 | skipReg << BS_I2C_SLV4_REG_DIS | intOnFinish << BS_SLV4_DONE_INT_EN);
}

uint8_t MPU6500_GetAuxSlave4Data(struct MPU6500_Device *dev)
{
    return ReadRegister(dev, REG_I2C_SLV4_DI);
}

void MPU6500_ResetSignalPath(struct MPU6500_Device *dev, bool accel, bool gyro, bool temp)
{
    WriteRegister(dev, REG_SIGNAL_PATH_RESET, gyro << BS_GYRO_RST | accel << BS_ACCEL_RST | temp << BS_TEMP_RST);
}

void MPU6500_ResetSensors(struct MPU6500_Device *dev)
{
    SetBits(dev, REG_USER_CTRL, 1 << BS_SIG_COND_RST, ENABLE);
}

void MPU6500_ResetFIFO(struct MPU6500_Device *dev)
{
    SetBits(dev, REG_USER_CTRL, 1 << BS_FIFO_RST, ENABLE);
}

void MPU6500_ResetPrimaryI2C(struct MPU6500_Device *dev)
{
    SetBits(dev, REG_USER_CTRL, 1 << BS_I2C_IF_DIS, ENABLE);
}

void MPU6500_ResetAuxilaryI2C(struct MPU6500_Device *dev)
{
    SetBits(dev, REG_USER_CTRL, 1 << BS_I2C_MST_RST, ENABLE);
}

void MPU6500_ResetDMP(struct MPU6500_Device *dev)
{
    SetBits(dev, REG_USER_CTRL, 1 << BS_DMP_RST, ENABLE);
}

void MPU6500_ResetDevice(struct MPU6500_Device *dev)
{
    SetBits(dev, REG_PWR_MGMT_1, 1 << BS_DEVICE_RESET, ENABLE);
}

void MPU6500_SetDmpState(struct MPU6500_Device *dev, FunctionalState state)
{
    SetBits(dev, REG_USER_CTRL, 1 << BS_DMP_EN, state);
}
