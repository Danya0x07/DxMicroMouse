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

static volatile bool btnFlag = 0;

struct Module *modules[] = {
    &Sensors_module,
    &IMU_module,
    &Motors_module,
    &Fan_module,
    &Encoders_module,
    &Memory_module,
    &Battery_module,
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

    if ((retcode = IMU_Init()) != 0) {
        printf("IMU retcode: %d\n", retcode);
        Buzzer_Sing((uint16_t []){1200, 800}, 2, 100);
        LED0_Blink(3, 200);
    }

    SPI_SetSpeedToNormal();
    printf("======= INITIALIZATION FINISHED =======\n");
    Buzzer_Sing((uint16_t []){1200, 1500, 2000}, 3, 100);

    for (;;) {
        Shell_Spin();
    }
}

__attribute__((interrupt()))
void SysTick_Handler(void)
{
    MCU_millis++;

    SysTick_Reset();

    MEMORY_HOLD_TRANSACTION();

    Sensors_Update();
    Encoders_Update();
    IMU_Update();
    Battery_Update();

    Motors_Update();

    MEMORY_UNHOLD_TRANSACTION();
}

__attribute__((interrupt("WCH-Interrupt-fast")))
void EXTI1_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line1) != RESET) {
        btnFlag = 1;
        EXTI_ClearITPendingBit(EXTI_Line1);
    }
}

__attribute__((interrupt("WCH-Interrupt-fast")))
void NMI_Handler(void)
{

}

__attribute__((interrupt("WCH-Interrupt-fast")))
void HardFault_Handler(void)
{
    for (;;) {

    }
}
