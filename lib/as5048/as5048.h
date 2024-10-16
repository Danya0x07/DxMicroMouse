#ifndef _INC_AS5048_H
#define _INC_AS5048_H

#include <stdint.h>
#include <stdbool.h>
#include "as5048_conf.h"

#if defined(AS5048A) && defined(AS5048B) || !defined(AS5048A) && !defined(AS5048B)
#   error "Either AS5048A or AS5048B must be defined."
#endif

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

union AS5048_Errors AS5048_GetErrors(AS5048_Handle h);
bool AS5048_BurnFuses(AS5048_Handle h);
uint16_t AS5048_GetZero(AS5048_Handle h);
void AS5048_SetZero(AS5048_Handle h, uint16_t zero);
void AS5048_GetDiagnosticsData(AS5048_Handle h, struct AS5048_DiagnosticsData *data);
uint16_t AS5048_GetMagnitudeRaw(AS5048_Handle h);
uint16_t AS5048_GetAngleRaw(AS5048_Handle h);


#endif // _INC_AS5048_H