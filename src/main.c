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
    NULL
};

int main(void)
{
    int retcode;

    MCU_Init();
    LED0_Blink(2, 150);

    printf("DxMicroMouse mk1 Firmware " FIRMWARE_VERSION "\n");
    printf("Clock is: %ld\n\n", SystemCoreClock);

    if ((retcode = Memory_Init()) != 0) {
        printf("Memory retcode: %d\n", retcode);
        Buzzer_Sing((uint16_t []){1200, 800}, 2, 100);
        LED0_Blink(1, 200);
    }

    if ((retcode = Encoders_Init()) != 0) {
        printf("Encoders retcode: %d\n", retcode);
        Buzzer_Sing((uint16_t []){1200, 800}, 2, 100);
        LED0_Blink(2, 200);
    }
    Encoders_Reset();

    IMU_Init(ImuConfiguration_TEST);
    IMU_Test();
    IMU_Init(ImuConfiguration_CALIBRATION);
    IMU_Calibrate(5);
    if ((retcode = IMU_Init(ImuConfiguration_APP)) != 0) {
        printf("IMU retcode: %d\n", retcode);
        Buzzer_Sing((uint16_t []){1200, 800}, 2, 100);
        LED0_Blink(3, 200);
    }

    SPI_SetSpeedToNormal();
    Sensors_SetLightening(DISABLE);
    Buzzer_Sing((uint16_t []){1200, 1500, 2000}, 3, 100);

    Odometry_Reset();
    SpeedCtl_Reset();
    SpeedCtl_Setup(7000, 15, 100000, 30000);
    //SpeedCtl_SetState(ENABLE);
    Button_EnableInterrupt();
    printf("======= INITIALIZATION FINISHED =======\n");

    for (;;) {
        Shell_Spin();
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

    SpeedCtl_Update();
}

__attribute__((interrupt("WCH-Interrupt-fast")))
void EXTI1_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line1) != RESET) {
        SpeedCtl_SetState(DISABLE);
        Motors_SetPwm(0, 0);
        EXTI_ClearITPendingBit(EXTI_Line1);
    }
}
