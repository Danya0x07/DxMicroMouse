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

static volatile bool btnFlag = 0;

struct Module *modules[] = {
    &Sensors_module,
    &IMU_module,
    NULL
};

int main(void)
{
    int retcode;

    MCU_Init();
    LED0_Blink(2, 150);

    printf("DxMicroMouse mk1 Firmware " FIRMWARE_VERSION "\n");
    printf("Clock is: %ld\n\n", SystemCoreClock);

    if ((retcode = M95256_Init()) != 0) {
        printf("M95256 init failed: %d", retcode);
        Buzzer_Sing((uint16_t []){1200, 800}, 2, 100);
    }

    IMU_Init();

    Buzzer_Sing((uint16_t []){1200, 1500, 2000}, 3, 100);

    for (;;) {
        Shell_Spin();
    }
}

__attribute__((interrupt()))
void SysTick_Handler(void)
{
    MCU_millis++;

    SysTick->CTLR = 0;
    SysTick->CNTL0 = 0;
    SysTick->CNTL1 = 0;
    SysTick->CNTL2 = 0;
    SysTick->CNTL3 = 0;
    SysTick->CTLR = 1;

    Sensors_Update();
    IMU_Update();
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
