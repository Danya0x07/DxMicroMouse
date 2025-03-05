#include "receivers.h"

uint32_t Receivers_ReadChannel(enum ReceiverChannel channel)
{
    static const unsigned CHANNEL_TABLE[5] = {
        [ReceiverChannel_LF]    = RECEIVER_LF_CH,
        [ReceiverChannel_LS]    = RECEIVER_LS_CH,
        [ReceiverChannel_RS]    = RECEIVER_RS_CH,
        [ReceiverChannel_RF]    = RECEIVER_RF_CH,
        [ReceiverChannel_F]     = RECEIVER_F_CH
    };

    uint32_t adcValue = ADC_Read(CHANNEL_TABLE[channel]);

    if (channel == ReceiverChannel_F)
        adcValue = 4095 - adcValue;

    return adcValue;
}