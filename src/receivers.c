#include "receivers.h"

#define FRONT_SENSOR_TIMEOUT    50000

uint16_t Receivers_ReadChannel(enum ReceiverChannel channel)
{
    static const uint8_t CHANNEL_TABLE[5] = {
        [ReceiverChannel_LeftFront]     = RECEIVER_LF_CH,
        [ReceiverChannel_LeftSide]      = RECEIVER_LS_CH,
        [ReceiverChannel_RightSide]     = RECEIVER_RS_CH,
        [ReceiverChannel_RightFront]    = RECEIVER_RF_CH,
        [ReceiverChannel_Front]         = RECEIVER_F_CH
    };

    return ADC_Read(CHANNEL_TABLE[channel]);
}