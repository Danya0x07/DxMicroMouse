#ifndef _INC_AS5048_H
#define _INC_AS5048_H

#include <spi_io.h>
#include <gpio_ctl.h>
#include "as5048_conf.h"

#if defined(AS5048A) && defined(AS5048B) || !defined(AS5048A) && !defined(AS5048B)
#   error "Either AS5048A or AS5048B must be defined."
#endif

struct AS5048_Device {
    const SPI_Bus bus;
    const struct GPIOCTL_Line cs;
};

union AS5048_Errors {
    struct {
        uint8_t parity: 1;
        uint8_t command: 1;
        uint8_t framing: 1;
    };
    uint32_t status;
};

struct AS5048_DiagnosticsData {
    bool compHigh;
    bool compLow;
    bool cordicOverflow;
    bool offsetCompensationFinished;
    uint8_t agc;
};

union AS5048_Errors AS5048_GetErrors(struct AS5048_Device *dev);
bool AS5048_BurnFuses(struct AS5048_Device *dev);
uint16_t AS5048_GetZero(struct AS5048_Device *dev);
void AS5048_SetZero(struct AS5048_Device *dev, uint16_t zero);
void AS5048_GetDiagnosticsData(struct AS5048_Device *dev, struct AS5048_DiagnosticsData *data);
uint16_t AS5048_GetMagnitudeRaw(struct AS5048_Device *dev);
uint16_t AS5048_GetAngleRaw(struct AS5048_Device *dev);


#endif // _INC_AS5048_H