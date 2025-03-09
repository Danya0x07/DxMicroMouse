#include "mcu.h"
#include "uart.h"
#include "leds.h"
#include "button.h"
#include "buzzer.h"
#include <m95256.h>
#include "motors.h"
#include "fan.h"
#include "sensors.h"
#include "shell.h"
#include "imu.h"
#include "encoders.h"
#include "memory.h"
#include "battery.h"
#include "speedctl.h"
#include "odometry.h"
#include "motion.h"
#include "router.h"

struct Module *modules[] = {
    &Sensors_module,
    &IMU_module,
    &Motors_module,
    &Fan_module,
    &Encoders_module,
    &Memory_module,
    &Battery_module,
    &SpeedCtl_module,
    &Odometry_module,
    &Buzzer_module,
    &Motion_module,
    &Router_module,
    NULL
};

static RouterRunType runType = RouterRunType_SEARCH;

static void InitModules(void)
{
    int retcode;

    if ((retcode = Memory_Init()) != 0) {
        printf("Memory retcode: %d\n", retcode);
        Buzzer_Blink(1, 1000, 300);
    }
    if (!Button_IsPressed())
        Modules_LoadSettings();
    else
        printf("Skip loading settings\n");

    if ((retcode = Encoders_Init()) != 0) {
        printf("Encoders retcode: %d\n", retcode);
        Buzzer_Blink(2, 1000, 300);
    }
    Encoders_Reset();

    if ((retcode = IMU_Init(ImuConfiguration_APP)) != 0) {
        printf("IMU retcode: %d\n", retcode);
        Buzzer_Blink(3, 1000, 300);
    }

    SPI_SetSpeedToNormal();

    Odometry_Reset();
    SpeedCtl_Setup();
    Router_Setup();

    printf("======= INITIALIZATION FINISHED =======\n");
}

static void CheckBattery(void)
{
    BatteryStatus batteryStatus = Battery_GetStatus();

    printf("Battery status: %s\n", Battery_StatusToStr(batteryStatus));
    if (batteryStatus <= BatteryStatus_LOW) {
        printf("WARNING: low power!\n");
        Buzzer_Blink(3, 600, 80);
    }
}

static bool GetPress(void)
{
    bool press = false;
    while (Button_IsPressed()) {}

    for (int i = 0; i < 10; i++) {
        LED1_OFF();
        Millis_Wait(100);
        if (Button_GetEvent() == ButtonEvent_PRESS) {
            press = true;
            break;
        }
        LED1_ON();
        Millis_Wait(100);
    }
    LED1_OFF();
    Millis_Wait(200);
    return press;
}

static void WaitForFinger(void)
{
    while (Sensors_DetectFinger()) {}

    printf("Waiting for your finger... ;)\n");
    Buzzer_Blink(1, 1200, 50);

    LED0_ON();
    while (!Sensors_DetectFinger()) {
        if (Button_GetEvent() == ButtonEvent_PRESS) {
            if (runType == RouterRunType_SEARCH) {
                runType = RouterRunType_RUSH;
                LED1_ON();
            }
            else {
                runType = RouterRunType_SEARCH;
                LED1_OFF();
            }
        }
        Millis_Wait(200);
    }
    LED0_OFF();
    Buzzer_Blink(1, 2000, 50);

    printf("Now remove\n");
    while (Sensors_DetectFinger()) {}
    Millis_Wait(300);
    Buzzer_Blink(1, 3000, 50);
}

static void ShowHappiness(void)
{
    Millis_Wait(300);
    LED0_ON();
    Millis_Wait(100);
    LED1_ON();
    Buzzer_Sing((uint16_t []){2600, 2900, 3100, 3300, 3600, 3900}, 6, 70);
    LED1_OFF();
    Millis_Wait(100);
    LED0_OFF();
    Millis_Wait(300);
}

int main(void)
{
    MCU_Init();
    Sensors_SetLightening(DISABLE);
    LED0_Blink(2, 150);
    printf("\nDxMicroMouse mk1 Firmware " FIRMWARE_VERSION "\n");

    InitModules();

    Millis_Wait(1000);
    CheckBattery();

    bool setupMode = GetPress();

    if (setupMode) {
        Buzzer_Sing((uint16_t []){1200, 1500, 2000}, 3, 50);
        printf("Setup mode\n");
        Motion_SetDiscreteMotion(ENABLE);

        if (GetPress()) {
            Router_EraseMaze();
            Modules_SaveSettings();
            printf("Maze erased from RAM\n");
            Buzzer_Sing((uint16_t []){2000, 1800}, 2, 50);
        }

        for (;;) {
            Shell_Spin();
            if (Button_GetEvent() == ButtonEvent_PRESS) {
                SpeedCtl_SetState(DISABLE);
                Sensors_SetLightening(DISABLE);
                Fan_Off();
                Buzzer_Blink(2, 900, 80);
            }
        }
    }
    else {
        Buzzer_Sing((uint16_t []){2800, 2800, 3300, 4000}, 4, 50);
        printf("Run mode\n");
        SpeedCtl_Reset();
        SpeedCtl_SetState(ENABLE);
        Sensors_SetLightening(ENABLE);

        for (;;) {
            WaitForFinger();
            Router_RunToFinish(runType);
            ShowHappiness();
            Modules_SaveSettings();  // to save known maze
            Router_RunToStart();
            Modules_SaveSettings();  // to save known maze
            CheckBattery();
        }
    }
}

__attribute__((interrupt()))
void SysTick_Handler(void)
{
    MCU_millis++;

    SysTick_Reset();

    Sensors_Update();
    MEMORY_HOLD_TRANSACTION();
    Encoders_Update();
    IMU_Update();
    MEMORY_UNHOLD_TRANSACTION();
    Battery_Update();
    Buzzer_Update();

    Motion_Update();
    SpeedCtl_Update();
}

//~ __attribute__((interrupt()))
//~ void EXTI1_IRQHandler(void)
//~ {
    //~ if(EXTI_GetITStatus(EXTI_Line1) != RESET) {
        //~ SpeedCtl_SetState(DISABLE);
        //~ Motors_SetPwm(0, 0);
        //~ EXTI_ClearITPendingBit(EXTI_Line1);
    //~ }
//~ }
