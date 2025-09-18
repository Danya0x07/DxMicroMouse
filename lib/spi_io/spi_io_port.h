#ifndef _INC_SPI_IO_PORT_H
#define _INC_SPI_IO_PORT_H

#include "spi_io.h"
#include "mcu.h"

struct SPI_BusCtl {
    int (*begin)(void);
    int (*transfer)(uint8_t *byteRx, uint8_t byteTx);
    void (*end)(void);
};

static int _CommonTransfer(uint8_t *byteRx, uint8_t byteTx)
{
    while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET)
        ;

    SPI_I2S_SendData(SPI2, byteTx);

    while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET)
        ;

    *byteRx = SPI_I2S_ReceiveData(SPI2);
    return SPI_OK;
}

static int _Begin_PH0(void)
{
    return SPI_OK;
}

static void _End_PH0(void) {}

static int _Begin_PH1(void)
{
    SPI2->CTLR1 &= ~SPI_CTLR1_SPE;
    SPI2->CTLR1 |= SPI_CTLR1_CPHA;
    SPI2->CTLR1 |= SPI_CTLR1_SPE;
    return SPI_OK;
}

static void _End_PH1(void)
{
    SPI2->CTLR1 &= ~SPI_CTLR1_SPE;
    SPI2->CTLR1 &= ~SPI_CTLR1_CPHA;
    SPI2->CTLR1 |= SPI_CTLR1_SPE;
}

static const struct SPI_BusCtl BUSES[] = {
    [SPI_Bus_PH0] = {
        .begin = _Begin_PH0,
        .transfer = _CommonTransfer,
        .end = _End_PH0
    },
    [SPI_Bus_PH1] = {
        .begin = _Begin_PH1,
        .transfer = _CommonTransfer,
        .end = _End_PH1
    },
};

#endif // _INC_SPI_IO_PORT_H