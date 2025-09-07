#include "button.h"
#include "mcu.h"

bool Button_IsPressed(void)
{
    return GPIO_ReadInputDataBit(BUTTON_GPIO, BUTTON_PIN) == 0;
}

ButtonEvent Button_GetEvent(void)
{
    static bool prevState = 0;
    static uint32_t prevCheckTime = 0;
    ButtonEvent event = ButtonEvent_NOTHING;
    bool state = Button_IsPressed();

    if (state != prevState && Millis_Get() - prevCheckTime > 10) {
        state = Button_IsPressed();
        prevCheckTime = Millis_Get();

        if (prevState == 0 && state == 1)
            event = ButtonEvent_PRESS;
        else if (prevState == 1 && state == 0)
            event = ButtonEvent_RELEASE;

        prevState = state;
    }

    return event;
}
