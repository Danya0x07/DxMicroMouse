#ifndef _INC_BUTTON_H
#define _INC_BUTTON_H

#include <stdbool.h>

typedef enum {
    ButtonEvent_NOTHING,
    ButtonEvent_PRESS,
    ButtonEvent_RELEASE
} ButtonEvent;

bool Button_IsPressed(void);
ButtonEvent Button_GetEvent(void);

#endif // _INC_BUTTON_H