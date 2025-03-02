#include "receivers.h"

uint16_t Receivers_ReadChannel(enum ReceiverChannel channel)
{
    static const uint8_t CHANNEL_TABLE[5] = {
        [ReceiverChannel_LF]    = RECEIVER_LF_CH,
        [ReceiverChannel_LS]    = RECEIVER_LS_CH,
        [ReceiverChannel_RS]    = RECEIVER_RS_CH,
        [ReceiverChannel_RF]    = RECEIVER_RF_CH,
        [ReceiverChannel_F]     = RECEIVER_F_CH
    };

    return ADC_Read(CHANNEL_TABLE[channel]);
}