#ifndef _INC_RECEIVERS_H
#define _INC_RECEIVERS_H

#include "mcu.h"

enum ReceiverChannel {
    ReceiverChannel_LF,
    ReceiverChannel_LS,
    ReceiverChannel_RS,
    ReceiverChannel_RF,
    ReceiverChannel_F,
};

uint16_t Receivers_ReadChannel(enum ReceiverChannel channel);

#endif // _INC_RECEIVERS_H