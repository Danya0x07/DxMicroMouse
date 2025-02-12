#ifndef _INC_MCU_H
#define _INC_MCU_H

#include <stdbool.h>
#include <stddef.h>

#include <ch32v10x.h>
#include <debug.h>

/* ============= Pin configuration ============= */
#define LED0_GPIO   GPIOC
#define LED0_PIN    GPIO_Pin_15

#define LED1_GPIO   GPIOD
#define LED1_PIN    GPIO_Pin_0

#define BUTTON_GPIO GPIOD
#define BUTTON_PIN  GPIO_Pin_1

#define BUZZER_GPIO GPIOA
#define BUZZER_PIN  GPIO_Pin_15

#define MEM_CS_GPIO GPIOB
#define MEM_CS_PIN  GPIO_Pin_11

#define MEM_HOLD_GPIO   GPIOB
#define MEM_HOLD_PIN    GPIO_Pin_10

#define IMU_CS_GPIO     GPIOB
#define IMU_CS_PIN      GPIO_Pin_12
#define IMU_INT_GPIO    GPIOA
#define IMU_INT_PIN     GPIO_Pin_8

#define ENCL_CS_GPIO    GPIOA
#define ENCL_CS_PIN     GPIO_Pin_12
#define ENCR_CS_GPIO    GPIOC
#define ENCR_CS_PIN     GPIO_Pin_14

#define SPI_MOSI_GPIO   GPIOB
#define SPI_MOSI_PIN    GPIO_Pin_15

#define SPI_MISO_GPIO   GPIOB
#define SPI_MISO_PIN    GPIO_Pin_14

#define SPI_SCK_GPIO    GPIOB
#define SPI_SCK_PIN     GPIO_Pin_13

#define MOTORS_GPIO GPIOB
#define DRV_AIN1_PIN    GPIO_Pin_6
#define DRV_AIN2_PIN    GPIO_Pin_7
#define DRV_BIN1_PIN    GPIO_Pin_8
#define DRV_BIN2_PIN    GPIO_Pin_9

#define FAN_PIN     GPIO_Pin_4

#define EMITTER_LF_GPIO GPIOB
#define EMITTER_LF_PIN  GPIO_Pin_2
#define EMITTER_LS_GPIO GPIOB
#define EMITTER_LS_PIN  GPIO_Pin_0
#define EMITTER_F_GPIO  GPIOA
#define EMITTER_F_PIN   GPIO_Pin_5
#define EMITTER_RS_GPIO GPIOA
#define EMITTER_RS_PIN  GPIO_Pin_3
#define EMITTER_RF_GPIO GPIOA
#define EMITTER_RF_PIN  GPIO_Pin_2

#define RECEIVER_LF_GPIO    GPIOB
#define RECEIVER_LF_PIN     GPIO_Pin_1
#define RECEIVER_LF_CH      ADC_Channel_9
#define RECEIVER_LS_GPIO    GPIOA
#define RECEIVER_LS_PIN     GPIO_Pin_7
#define RECEIVER_LS_CH      ADC_Channel_7
#define RECEIVER_F_GPIO     GPIOA
#define RECEIVER_F_PIN      GPIO_Pin_6
#define RECEIVER_F_CH       ADC_Channel_6
#define RECEIVER_RS_GPIO    GPIOA
#define RECEIVER_RS_PIN     GPIO_Pin_4
#define RECEIVER_RS_CH      ADC_Channel_4
#define RECEIVER_RF_GPIO    GPIOA
#define RECEIVER_RF_PIN     GPIO_Pin_1
#define RECEIVER_RF_CH      ADC_Channel_1

#define BATTERY_GPIO    GPIOA
#define BATTERY_PIN     GPIO_Pin_0
#define BATTERY_CH      ADC_Channel_0

#define MOTOR_FREQ  20000
#define MOTOR_PWM_MAX    3600

#define FAN_PWM_MAX 360

extern RCC_ClocksTypeDef MCU_rccClocks;
extern volatile uint32_t MCU_millis;

void MCU_Init(void);

static inline void Micros_Reset(void)
{
    TIM_SetCounter(TIM1, 0);
}

static inline uint16_t Micros_Get(void)
{
    return TIM_GetCounter(TIM1);
}

void Micros_Wait(uint16_t us);

static inline void Micros_WaitMillis(uint16_t ms)
{
    while (ms--)
        Micros_Wait(1000);
}

static inline uint32_t Millis_Get(void)
{
    return MCU_millis;
}

void Millis_Wait(uint32_t ms);

static inline void SysTick_Reset(void)
{
    SysTick->CTLR = 0;
    SysTick->CNTL0 = 0;
    SysTick->CNTL1 = 0;
    SysTick->CNTL2 = 0;
    SysTick->CNTL3 = 0;
    SysTick->CTLR = 1;
}

uint8_t SPI_TransferByte(uint8_t data);
void SPI_TransferBytes(uint8_t *in, const uint8_t *out, uint16_t len);
void SPI_SetSpeedToNormal(void);

uint16_t ADC_Read(uint8_t ch);

void *memcpy_v2n(void *dest, const volatile void *src, uint16_t n);
volatile void *memcpy_n2v(volatile void *dest, const void *src, uint16_t n);

#define SysTick_EnableInterrupt()   NVIC_EnableIRQ(SysTicK_IRQn)
#define SysTick_DisableInterrupt()   NVIC_DisableIRQ(SysTicK_IRQn)

#endif // _INC_MCU_H