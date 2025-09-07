#include "mcu.h"
#include "leds.h"
#include "button.h"
#include "buzzer.h"
#include "motors.h"
#include "fan.h"
#include "sensors.h"
#include "imu.h"
#include "encoders.h"
#include "battery.h"
#include "speedctl.h"
#include "odometry.h"
#include "motion.h"
#include "maneuver.h"
#include "memory.h"
#include "router.h"
#include "telemetry.h"

#include <uart_io.h>
#include <shell.h>
#include <scheduler.h>
#include <settings.h>

static int PrintSettingsMemory(int argc, char *argv[]);
static int SaveSettings(int argc, char *argv[]);

static const struct ShellCommand *const shellCommands[] = {
    &CMD_Buzzer,
    &CMD_Encoders,
    &CMD_Fan,
    &CMD_Imu,
    &CMD_Maneuver,
    &CMD_Motors,
    &CMD_Odometry,
    &CMD_Router,
    &CMD_Sensors,
    &CMD_SpeedCtl,
    &CMD_Telemetry,
    &(struct ShellCommand){.name = "mem", .execute = PrintSettingsMemory},
    &(struct ShellCommand){.name = "ss", .execute = SaveSettings},
    NULL
};

struct SchedulerTask *const schedulerTasks[] = {
    &TASK_TmBattery,
    &TASK_TmEncoders,
    &TASK_TmImu,
    &TASK_TmMotors,
    &TASK_TmOdometry,
    &TASK_TmSensors,
    &TASK_TmSpeedCtl,
    NULL
};
const char *const taskNames[sizeof(schedulerTasks) / sizeof(schedulerTasks[0]) - 1] = {
    "bat", "encs", "imu", "mot", "odom", "sens", "spctl"
};

static const struct Settings *const settings[] = {
    &SETT_Imu,
    &SETT_Maneuver,
    &SETT_Odometry,
    &SETT_Router,
    &SETT_Sensors,
    &SETT_SpeedCtl,
    NULL
};

static bool runFast = false;

static void InitModules(void)
{
    int retcode;

    if (!Button_IsPressed()) {
        if ((retcode = Settings_Load(settings)) != SETTINGS_OK) {
            printf("Settings err: %d\n", retcode);
            Buzzer_BlinkInitError(1);
        }
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
        Settings_Save(settings);
        printf("Maze erased from RAM\n");
        Buzzer_SingErazeMaze();
    }
    Router_Setup();

    for (;;) {
        Shell_Spin(shellCommands);
        Scheduler_SpinRegular(schedulerTasks);
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
    Router_Setup();

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
            SaveSettings(0, NULL);
            Router_TargetStart();
            success = Router_RunSearch();
            if (success)
                SaveSettings(0, NULL);
            else
                break;
        }
        else {
            break;
        }
        CheckBattery();
    }
    StopActivity();
    Buzzer_BlinkManeuverFailed();
}

int main(void)
{
    MCU_Init();
    Sensors_SetState(DISABLE);
    Scheduler_Setup(schedulerTasks);
    LED0_Blink(2, 150);
    printf("\nDxMicroMouse mk1 Firmware " FIRMWARE_VERSION "\n");

    InitModules();
    Millis_Wait(1000);

    bool setupMode = GetPress(5);
    for (;;) {
        CheckBattery();
        if (setupMode) {
            executeSetupMode();
            setupMode = false;
        }
        else {
            executeRunMode();
            setupMode = true;
        }
    }
}

__attribute__((interrupt()))
void SysTick_Handler(void)
{
    MCU_millis++;

    SysTick_Reset();

    Sensors_Update();
    Encoders_Update();
    IMU_Update();
    Battery_Update();
    Buzzer_Update();

    Motion_Update();
    SpeedCtl_Update();
}

static int PrintSettingsMemory(int argc, char *argv[])
{
    uint8_t buffer[MEMORY_SIZE];
    Settings_Export(buffer);
    for (int i = 0; i < MEMORY_SIZE; i++)
        UART_SendChar(buffer[i]);
    return 0;
}

static int SaveSettings(int argc, char *argv[])
{
    printf("Settings save ret: %d\n", Settings_Save(settings));
    return 0;
}
