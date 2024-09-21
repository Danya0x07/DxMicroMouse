#include "receivers.h"

uint16_t Receivers_ReadChannel(enum ReceiverChannel channel)
{
    if (channel == ReceiverChannel_Front) {  // Особый случай
        GPIO_InitTypeDef GPIO_InitStructure = {0};
        //~ RECEIVER_F_GPIO->CFGLR = RECEIVER_F_GPIO->CFGLR & ~(0b1111 << 20) | (0b0011 << 20);
        //~ GPIOC->BSHR = RECEIVER_F_PIN;
        GPIO_InitStructure.GPIO_Pin = RECEIVER_F_PIN;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_Init(RECEIVER_F_GPIO, &GPIO_InitStructure);
        GPIO_SetBits(RECEIVER_F_GPIO, RECEIVER_F_PIN);
        Micros_Wait(10);
        //~ RECEIVER_F_GPIO->CFGLR = RECEIVER_F_GPIO->CFGLR & ~(0b1111 << 20) | (0b0100 << 20);
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_Init(RECEIVER_F_GPIO, &GPIO_InitStructure);
        Micros_Reset();
        while (GPIO_ReadInputDataBit(RECEIVER_F_GPIO, RECEIVER_F_PIN) == 1) {
            if (Micros_Get() >= 50000)
                break;
        }
        return Micros_Get();
    }
    else {
        static const uint8_t CHANNEL_TABLE[4] = {
            [ReceiverChannel_LeftFront]     = RECEIVER_LF_CH,
            [ReceiverChannel_LeftSide]      = RECEIVER_LS_CH,
            [ReceiverChannel_RightSide]     = RECEIVER_RS_CH,
            [ReceiverChannel_RightFront]    = RECEIVER_RF_CH
        };

        return ADC_Read(CHANNEL_TABLE[channel]);
    }
}