#include "as5048.h"

// ================= Control and Error Registers ===================
#define REG_NOP     0x0000

#define REG_ERROR   0x0001
#define BIT_ERROR_PARITY     (1 << 2)
#define BIT_ERROR_CMDINV     (1 << 1)
#define BIT_ERROR_FRAMING    (1 << 0)

#define REG_PROGCTL 0x0003
#define BIT_PROGEN  (1 << 0)
#define BIT_BURN    (1 << 3)
#define BIT_VERIFY  (1 << 6)

// ================ Programmable Customer Settings =================
#define REG_ZEROH   0x0016
#define REG_ZEROL   0x0017

// =============== Readout Registers ===============================
#define REG_DIAGNOSTICS 0x3FFD
#define BIT_OCF (1 << 8)
#define BIT_COF (1 << 9)
#define BIT_COMPL   (1 << 10)
#define BIT_COMPH   (1 << 11)

#define REG_MAGNITUDE   0x3FFE
#define REG_ANGLE       0x3FFF

static uint16_t CalcEvenParity(uint16_t value){
	uint16_t cnt = 0;

	for (uint_fast8_t i = 0; i < 15; i++) {
		if (value & 1)
			cnt++;
		value >>= 1;
	}
	return cnt & 1;
}

static uint16_t ReadRegister(struct AS5048_Device *dev, uint16_t reg)
{
    uint16_t msg = (1 << 14) | (reg & 0x3FFF);
    msg |= CalcEvenParity(msg) << 15;

    uint8_t in[2], out[2] = {msg >> 8, msg & 0xFF};

    SPI_BeginTransfer(dev->bus);
    GPIOCTL_Low(&dev->cs);
    SPI_TransferData(dev->bus, NULL, out, 2);
    GPIOCTL_High(&dev->cs);
    SPI_EndTransfer(dev->bus);

    SPI_BeginTransfer(dev->bus);
    GPIOCTL_Low(&dev->cs);
    SPI_TransferData(dev->bus, in, NULL, 2);
    GPIOCTL_High(&dev->cs);
    SPI_EndTransfer(dev->bus);

    uint16_t resp = (uint16_t)in[0] << 8 | in[1];
    resp &= 0x3FFF; // remove parity and error flag bits
    return resp;
}

static uint16_t WriteRegister(struct AS5048_Device *dev, uint16_t reg, uint16_t value)
{
    uint16_t msg = reg & 0x3FFF;
    msg |= CalcEvenParity(msg) << 15;

    uint8_t in[2], out[2] = {msg >> 8, msg & 0xFF};

    value &= 0x3FFF;
    value |= CalcEvenParity(value) << 15;

    SPI_BeginTransfer(dev->bus);
    GPIOCTL_Low(&dev->cs);
    SPI_TransferData(dev->bus, NULL, out, 2);
    GPIOCTL_High(&dev->cs);
    SPI_EndTransfer(dev->bus);

    out[0] = value >> 8;
    out[1] = value & 0xFF;
    SPI_BeginTransfer(dev->bus);
    GPIOCTL_Low(&dev->cs);
    SPI_TransferData(dev->bus, in, out, 2);
    GPIOCTL_High(&dev->cs);
    SPI_EndTransfer(dev->bus);

    SPI_BeginTransfer(dev->bus);
    GPIOCTL_Low(&dev->cs);
    SPI_TransferData(dev->bus, in, NULL, 2);
    GPIOCTL_High(&dev->cs);
    SPI_EndTransfer(dev->bus);

    uint16_t new = (uint16_t)in[0] << 8 | in[1];
    new &= 0x3FFF; // remove parity and error flag bits
    return new;
}

union AS5048_Errors AS5048_GetErrors(struct AS5048_Device *dev)
{
    uint16_t data = ReadRegister(dev, REG_ERROR);

    union AS5048_Errors errors = {
        .parity = !!(data & BIT_ERROR_PARITY),
        .command = !!(data & BIT_ERROR_CMDINV),
        .framing = !!(data & BIT_ERROR_FRAMING)
    };
    return errors;
}

bool AS5048_BurnFuses(struct AS5048_Device *dev)
{
    bool success = true;

    WriteRegister(dev, REG_PROGCTL, BIT_PROGEN);
    WriteRegister(dev, REG_PROGCTL, BIT_PROGEN | BIT_BURN);
    if (ReadRegister(dev, REG_ANGLE) != 0)
        success = false;
    WriteRegister(dev, REG_PROGCTL, BIT_PROGEN | BIT_BURN | BIT_VERIFY);
    if (ReadRegister(dev, REG_ANGLE) != 0)
        success = false;
    return success;
}

uint16_t AS5048_GetZero(struct AS5048_Device *dev)
{
    uint16_t zero = (ReadRegister(dev, REG_ZEROH) & 0x00FF) << 6;
    zero |= ReadRegister(dev, REG_ZEROL) & 0x003F;
    return zero;
}

void AS5048_SetZero(struct AS5048_Device *dev, uint16_t zero)
{
    zero &= 0x3FFF;
    WriteRegister(dev, REG_ZEROH, zero >> 6);
    WriteRegister(dev, REG_ZEROL, zero & 0x003F);
}

void AS5048_GetDiagnosticsData(struct AS5048_Device *dev, struct AS5048_DiagnosticsData *data)
{
    uint16_t value = ReadRegister(dev, REG_DIAGNOSTICS);

    data->compHigh = !!(value & BIT_COMPH);
    data->compLow = !!(value & BIT_COMPL);
    data->cordicOverflow = !!(value & BIT_COF);
    data->offsetCompensationFinished = !!(value & BIT_OCF);
    data->agc = value & 0xFF;
}

uint16_t AS5048_GetMagnitudeRaw(struct AS5048_Device *dev)
{
    return ReadRegister(dev, REG_MAGNITUDE);
}

uint16_t AS5048_GetAngleRaw(struct AS5048_Device *dev)
{
    return ReadRegister(dev, REG_ANGLE);
}