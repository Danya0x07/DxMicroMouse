#include "button.h"

bool Button_IsPressed(void)
{
    return GPIO_ReadInputDataBit(BUTTON_GPIO, BUTTON_PIN) == 0;
}

ButtonEvent_t Button_GetEvent(void)
{
    static bool prevState = 0;
    static uint16_t prevCheckTime = 0;

    ButtonEvent_t event = ButtonEvent_NOTHING;
    bool state = Button_IsPressed();

    if (state != prevState && (uint16_t)(Micros_Get() - prevCheckTime) > 5000) {
        prevCheckTime = Micros_Get();
        if (prevState == 0 && state == 1)
            event = ButtonEvent_PRESS;
        else if (prevState == 1 && state == 0)
            event = ButtonEvent_RELEASE;
        prevState = state;
    }

    return event;
}

ButtonEvent_t Button_GetNextEvent(void)
{
    ButtonEvent_t event = ButtonEvent_NOTHING;

    for (int i = 0; i < 200; i++) {
        Micros_Wait(1000);
        if (event == ButtonEvent_NOTHING)
            event = Button_GetEvent();
        else if (event == ButtonEvent_RELEASE && Button_GetEvent() == ButtonEvent_PRESS) {
            event = ButtonEvent_PRESS;
            break;
        }
    }
    return event;
}

void Button_EnableInterrupt(void)
{
    NVIC_EnableIRQ(EXTI1_IRQn);
}

void Button_DisableInterrupt(void)
{
    NVIC_DisableIRQ(EXTI1_IRQn);
}