#ifndef _INC_BUTTON_H
#define _INC_BUTTON_H

#include "mcu.h"

typedef enum {
    ButtonEvent_NOTHING,
    ButtonEvent_PRESS,
    ButtonEvent_RELEASE
} ButtonEvent_t;

bool Button_IsPressed(void);
ButtonEvent_t Button_GetEvent(void);
//~ ButtonEvent_t Button_GetNextEvent(void);

//~ void Button_EnableInterrupt(void);
//~ void Button_DisableInterrupt(void);

#endif // _INC_BUTTON_H