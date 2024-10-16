#ifndef _INC_AS5048_PORT_H
#define _INC_AS5048_PORT_H

#include "as5048_conf.h"
#include "mcu.h"

#ifndef AS5048A
#   error "Only AS5048A currently supported"
#endif

static GPIO_TypeDef *const _gpios[2] = {
    [AS5048_Handle_LEFT] = ENCL_CS_GPIO,
    [AS5048_Handle_RIGHT] = ENCR_CS_GPIO
};

static const uint16_t _pins[2] = {
    [AS5048_Handle_LEFT] = ENCL_CS_PIN,
    [AS5048_Handle_RIGHT] = ENCR_CS_PIN
};

static inline void cs_low(AS5048_Handle h)
{
    //~ SPI2->CTLR1 &= ~SPI_CTLR1_SPE;
    //~ SPI2->CTLR1 |= SPI_CTLR1_CPHA;
    //~ SPI2->CTLR1 |= SPI_CTLR1_SPE;
    GPIO_ResetBits(_gpios[h], _pins[h]);
}

static inline void cs_high(AS5048_Handle h)
{
    GPIO_SetBits(_gpios[h], _pins[h]);
    //~ SPI2->CTLR1 &= ~SPI_CTLR1_SPE;
    //~ SPI2->CTLR1 &= ~SPI_CTLR1_CPHA;
    //~ SPI2->CTLR1 |= SPI_CTLR1_SPE;
}

static inline void spi_begin(void)
{
    SPI2->CTLR1 &= ~SPI_CTLR1_SPE;
    SPI2->CTLR1 |= SPI_CTLR1_CPHA;
    SPI2->CTLR1 |= SPI_CTLR1_SPE;
}

static inline void spi_end(void)
{
    SPI2->CTLR1 &= ~SPI_CTLR1_SPE;
    SPI2->CTLR1 &= ~SPI_CTLR1_CPHA;
    SPI2->CTLR1 |= SPI_CTLR1_SPE;
}

#endif