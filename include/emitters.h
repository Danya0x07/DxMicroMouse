#ifndef _INC_EMITTERS_H
#define _INC_EMITTERS_H

#include "mcu.h"

static inline void Emitters_LeftFrontOn(void)
{
    GPIO_SetBits(EMITTER_LF_GPIO, EMITTER_LF_PIN);
}

static inline void Emitters_LeftFrontOff(void)
{
    GPIO_ResetBits(EMITTER_LF_GPIO, EMITTER_LF_PIN);
}

static inline void Emitters_LeftSideOn(void)
{
    GPIO_SetBits(EMITTER_LS_GPIO, EMITTER_LS_PIN);
}

static inline void Emitters_LeftSideOff(void)
{
    GPIO_ResetBits(EMITTER_LS_GPIO, EMITTER_LS_PIN);
}

static inline void Emitters_FrontOn(void)
{
    GPIO_SetBits(EMITTER_F_GPIO, EMITTER_F_PIN);
}

static inline void Emitters_FrontOff(void)
{
    GPIO_ResetBits(EMITTER_F_GPIO, EMITTER_F_PIN);
}

static inline void Emitters_RightSideOn(void)
{
    GPIO_SetBits(EMITTER_RS_GPIO, EMITTER_RS_PIN);
}

static inline void Emitters_RightSideOff(void)
{
    GPIO_ResetBits(EMITTER_RS_GPIO, EMITTER_RS_PIN);
}

static inline void Emitters_RightFrontOn(void)
{
    GPIO_SetBits(EMITTER_RF_GPIO, EMITTER_RF_PIN);
}

static inline void Emitters_RightFrontOff(void)
{
    GPIO_ResetBits(EMITTER_RF_GPIO, EMITTER_RF_PIN);
}

#endif // _INC_EMITTERS_H