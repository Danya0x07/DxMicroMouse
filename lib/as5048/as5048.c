#include "as5048.h"
#include "as5048_port.h"

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

static uint16_t ReadRegister(AS5048_Handle h, uint16_t reg)
{
    uint16_t msg = (1 << 14) | (reg & 0x3FFF);
    msg |= CalcEvenParity(msg) << 15;

    spi_begin();
    cs_low(h);
    SPI_TransferByte(msg >> 8);
    SPI_TransferByte(msg & 0xFF);
    cs_high(h);
    spi_end();

    uint16_t resp;
    spi_begin();
    cs_low(h);
    resp = SPI_TransferByte(0) << 8;
    resp |= SPI_TransferByte(0);
    cs_high(h);
    spi_end();

    resp &= 0x3FFF; // remove parity and error flag bits
    return resp;
}

static uint16_t WriteRegister(AS5048_Handle h, uint16_t reg, uint16_t value)
{
    uint16_t msg = reg & 0x3FFF;
    msg |= CalcEvenParity(msg) << 15;

    value &= 0x3FFF;
    value |= CalcEvenParity(value) << 15;

    spi_begin();
    cs_low(h);
    SPI_TransferByte(msg >> 8);
    SPI_TransferByte(msg & 0xFF);
    cs_high(h);
    spi_end();

    uint16_t old;
    spi_begin();
    cs_low(h);
    old = SPI_TransferByte(value >> 8) << 8;
    old |= SPI_TransferByte(value & 0xFF);
    cs_high(h);
    spi_end();

    uint16_t new;
    spi_begin();
    cs_low(h);
    new = SPI_TransferByte(0) << 8;
    new |= SPI_TransferByte(0);
    cs_high(h);
    spi_end();

    new &= 0x3FFF; // remove parity and error flag bits
    return new;
}

union AS5048_Errors AS5048_GetErrors(AS5048_Handle h)
{
    uint16_t data = ReadRegister(h, REG_ERROR);

    union AS5048_Errors errors = {
        .parity = !!(data & BIT_ERROR_PARITY),
        .command = !!(data & BIT_ERROR_CMDINV),
        .framing = !!(data & BIT_ERROR_FRAMING)
    };
    return errors;
}

bool AS5048_BurnFuses(AS5048_Handle h)
{
    bool success = true;

    WriteRegister(h, REG_PROGCTL, BIT_PROGEN);
    WriteRegister(h, REG_PROGCTL, BIT_PROGEN | BIT_BURN);
    if (ReadRegister(h, REG_ANGLE) != 0)
        success = false;
    WriteRegister(h, REG_PROGCTL, BIT_PROGEN | BIT_BURN | BIT_VERIFY);
    if (ReadRegister(h, REG_ANGLE) != 0)
        success = false;
    return success;
}

uint16_t AS5048_GetZero(AS5048_Handle h)
{
    uint16_t zero = (ReadRegister(h, REG_ZEROH) & 0x00FF) << 6;
    zero |= ReadRegister(h, REG_ZEROL) & 0x003F;
    return zero;
}

void AS5048_SetZero(AS5048_Handle h, uint16_t zero)
{
    zero &= 0x3FFF;
    WriteRegister(h, REG_ZEROH, zero >> 6);
    WriteRegister(h, REG_ZEROL, zero & 0x003F);
}

void AS5048_GetDiagnosticsData(AS5048_Handle h, struct AS5048_DiagnosticsData *data)
{
    uint16_t value = ReadRegister(h, REG_DIAGNOSTICS);

    data->compHigh = !!(value & BIT_COMPH);
    data->compLow = !!(value & BIT_COMPL);
    data->cordicOverflow = !!(value & BIT_COF);
    data->offsetCompensationFinished = !!(value & BIT_OCF);
    data->agc = value & 0xFF;
}

uint16_t AS5048_GetMagnitudeRaw(AS5048_Handle h)
{
    return ReadRegister(h, REG_MAGNITUDE);
}

uint16_t AS5048_GetAngleRaw(AS5048_Handle h)
{
    return ReadRegister(h, REG_ANGLE);
}