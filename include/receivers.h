#ifndef _INC_RECEIVERS_H
#define _INC_RECEIVERS_H

#include "mcu.h"

enum ReceiverChannel {
    ReceiverChannel_LeftFront,
    ReceiverChannel_LeftSide,
    ReceiverChannel_RightSide,
    ReceiverChannel_RightFront,
    ReceiverChannel_Front,
};

uint16_t Receivers_ReadChannel(enum ReceiverChannel channel);

#endif // _INC_RECEIVERS_H