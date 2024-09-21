#include "mpu6500.h"
#include "mpu6500_port.h"

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
#define BS_DES_ZG       0

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
