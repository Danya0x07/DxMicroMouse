#include "mcu.h"
#include "uart.h"
#include "leds.h"
#include "button.h"
#include "buzzer.h"
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
#include "maneuver.h"
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
    &Maneuver_module,
    &Router_module,
    NULL
};

static bool runFast = false;

static void InitModules(void)
{
    int retcode;

    if (!Button_IsPressed()) {
        if ((retcode = Modules_LoadSettings()) < 0)
            Buzzer_BlinkInitError(1);
    }
    else {
        printf("Skip loading settings\n");
    }

    if ((retcode = Encoders_Init()) != 0) {
        printf("Encoders retcode: %d\n", retcode);
        Buzzer_BlinkInitError(2);
    }
    Encoders_Reset();

    if ((retcode = IMU_Init(ImuConfiguration_APP)) != 0) {
        printf("IMU retcode: %d\n", retcode);
        Buzzer_BlinkInitError(3);
    }

    SPI_SetSpeedToNormal();

    Odometry_Reset();
    SpeedCtl_Setup();

    printf("======= INITIALIZATION FINISHED =======\n");
}

static void CheckBattery(void)
{
    BatteryStatus batteryStatus = Battery_GetStatus();

    printf("Battery status: %s\n", Battery_StatusToStr(batteryStatus));
    if (batteryStatus <= BatteryStatus_LOW) {
        printf("WARNING: low power!\n");
        Buzzer_BlinkLowBattery();
    }
}

static bool GetPress(unsigned blinkFreq)
{
    bool press = false;
    while (Button_IsPressed()) {}

    uint32_t t = Millis_Get();
    while (Millis_Get() - t < 3000) {
        LED1_OFF();
        Millis_Wait(1000 / blinkFreq);
        if (Button_GetEvent() == ButtonEvent_PRESS) {
            press = true;
            break;
        }
        LED1_ON();
        Millis_Wait(1000 / blinkFreq);
    }
    LED1_OFF();
    Millis_Wait(200);
    return press;
}

static void WaitForFinger(void)
{
    while (Sensors_DetectFinger()) {}

    printf("Waiting for your finger... ;)\n");
    Buzzer_BlinkWaitingFinger(1);

    runFast = false;
    LED0_ON();
    while (!Sensors_DetectFinger()) {
        if (Button_GetEvent() == ButtonEvent_PRESS) {
            if (runFast == false) {
                runFast = true;
                LED1_ON();
            }
            else {
                runFast = false;
                LED1_OFF();
            }
        }
        Millis_Wait(200);
    }
    LED0_OFF();
    Buzzer_BlinkWaitingFinger(2);

    printf("Now remove\n");
    while (Sensors_DetectFinger()) {}
    Millis_Wait(300);
    Buzzer_BlinkWaitingFinger(3);
}

static void ShowHappiness(void)
{
    Millis_Wait(300);
    LED0_ON();
    Millis_Wait(100);
    LED1_ON();
    Buzzer_SingHappy();
    LED1_OFF();
    Millis_Wait(100);
    LED0_OFF();
    Millis_Wait(300);
}

static void StopActivity(void)
{
    SpeedCtl_SetState(DISABLE);
    Sensors_SetState(DISABLE);
    Fan_Off();
}

static void executeSetupMode(void)
{
    Buzzer_SingSetupMode();
    printf("Setup mode\n");

    if (GetPress(10)) {
        Router_EraseMaze();
        Modules_SaveSettings();
        printf("Maze erased from RAM\n");
        Buzzer_SingErazeMaze();
    }

    for (;;) {
        Shell_Spin();
        if (Button_GetEvent() == ButtonEvent_PRESS) {
            if (SpeedCtl_GetState() == ENABLE || Sensors_GetState() == ENABLE || Fan_IsOn()) {
                StopActivity();
                Buzzer_BlinkStopActivity();
            }
            else {
                break;
            }
        }
    }
}

static void executeRunMode(void)
{
    Buzzer_SingRunMode();
    printf("Run mode\n");
    if (GetPress(10)) {
        Router_ChangeStartDirection();
        Millis_Wait(2000);
    }
    SpeedCtl_Reset();
    SpeedCtl_SetState(ENABLE);
    Sensors_SetState(ENABLE);

    bool success;
    for (;;) {
        WaitForFinger();
        Router_TargetFinish();

        if (runFast)
            success = Router_RunFast();
        else
            success = Router_RunSearch();

        if (success) {
            ShowHappiness();
            Modules_SaveSettings();  // to save known maze
            Router_TargetStart();
            success = Router_RunSearch();
            if (success)
                Modules_SaveSettings();  // to save known maze
            else
                break;
        }
        else {
            break;
        }
        CheckBattery();
    }
}

int main(void)
{
    MCU_Init();
    Sensors_SetState(DISABLE);
    LED0_Blink(2, 150);
    printf("\nDxMicroMouse mk1 Firmware " FIRMWARE_VERSION "\n");

    InitModules();
    Millis_Wait(1000);

    bool setupMode = GetPress(5);
    for (;;) {
        Router_Setup();
        CheckBattery();
        if (setupMode) {
            executeSetupMode();
            setupMode = false;
        }
        else {
            executeRunMode();
            setupMode = true;
        }
        StopActivity();
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
