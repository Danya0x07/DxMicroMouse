#include "mcu.h"

RCC_ClocksTypeDef MCU_rccClocks;
volatile uint32_t MCU_millis;

static int16_t adcCalibrationValue;

static inline void initRCC(void)
{
    // Убрать делитель HSI на 2 перед PLL
    EXTEN->EXTEN_CTR |= EXTEN_PLL_HSI_PRE;

    /*
     * "When the prescale factor of the AHB clock source is
     * greater than 1, the prefetch buffer must be switched on."
     */
    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
    FLASH_SetLatency(FLASH_Latency_2);  // Для 48..72 МГц

    RCC_HCLKConfig(RCC_SYSCLK_Div1);
    RCC_PCLK1Config(RCC_HCLK_Div1);
    RCC_PCLK2Config(RCC_HCLK_Div1);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6); // 12 МГц для АЦП

    RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMULL9);
    RCC_PLLCmd(ENABLE);
    while (!RCC_GetFlagStatus(RCC_FLAG_PLLRDY))
        ;


    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    while (RCC_GetSYSCLKSource() != 0x08)
        ;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2
            | RCC_APB1Periph_TIM2
            | RCC_APB1Periph_TIM3
            | RCC_APB1Periph_TIM4, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1
            | RCC_APB2Periph_ADC1
            | RCC_APB2Periph_AFIO
            | RCC_APB2Periph_GPIOA
            | RCC_APB2Periph_GPIOB
            | RCC_APB2Periph_GPIOC
            | RCC_APB2Periph_GPIOD
            | RCC_APB2Periph_TIM1, ENABLE);

    RCC_GetClocksFreq(&MCU_rccClocks);
    SystemCoreClock = MCU_rccClocks.HCLK_Frequency;
}

static inline void initGPIO(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    // LEDS
    GPIO_PinRemapConfig(GPIO_Remap_PD01, ENABLE);
    GPIO_InitStructure.GPIO_Pin = LED0_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(LED0_GPIO, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = LED1_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(LED1_GPIO, &GPIO_InitStructure);

    // UART
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // SPI_MOSI
    GPIO_InitStructure.GPIO_Pin = SPI_MOSI_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(SPI_MOSI_GPIO, &GPIO_InitStructure);

    // SPI_MISO
    GPIO_InitStructure.GPIO_Pin = SPI_MISO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(SPI_MISO_GPIO, &GPIO_InitStructure);

    // SPI_SCK
    GPIO_InitStructure.GPIO_Pin = SPI_SCK_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(SPI_SCK_GPIO, &GPIO_InitStructure);

    // MEM_CS
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = MEM_CS_PIN;
    GPIO_Init(MEM_CS_GPIO, &GPIO_InitStructure);
    GPIO_SetBits(MEM_CS_GPIO, MEM_CS_PIN);

    // MEM_HOLD
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = MEM_HOLD_PIN;
    GPIO_Init(MEM_HOLD_GPIO, &GPIO_InitStructure);
    GPIO_SetBits(MEM_HOLD_GPIO, MEM_HOLD_PIN);

    // IMU_CS
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = IMU_CS_PIN;
    GPIO_Init(IMU_CS_GPIO, &GPIO_InitStructure);
    GPIO_SetBits(IMU_CS_GPIO, IMU_CS_PIN);

    // IMU_INT
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin = IMU_INT_PIN;
    GPIO_Init(IMU_INT_GPIO, &GPIO_InitStructure);

    // ENCL_CS
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = ENCL_CS_PIN;
    GPIO_Init(ENCL_CS_GPIO, &GPIO_InitStructure);
    GPIO_SetBits(ENCL_CS_GPIO, ENCL_CS_PIN);

    // ENCR_CS
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = ENCR_CS_PIN;
    GPIO_Init(ENCR_CS_GPIO, &GPIO_InitStructure);
    GPIO_SetBits(ENCR_CS_GPIO, ENCR_CS_PIN);

    // BUTTON
    GPIO_InitStructure.GPIO_Pin = BUTTON_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(BUTTON_GPIO, &GPIO_InitStructure);
    //~ GPIO_EXTILineConfig(GPIO_PortSourceGPIOD, GPIO_PinSource1);

    // BUZZER
    GPIO_PinRemapConfig(GPIO_FullRemap_TIM2, ENABLE);
    GPIO_InitStructure.GPIO_Pin = BUZZER_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(BUZZER_GPIO, &GPIO_InitStructure);

    // FAN
#ifdef FAN_PWM_MAX
    GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
#else
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
#endif
    GPIO_InitStructure.GPIO_Pin = FAN_PIN;
    GPIO_Init(MOTORS_GPIO, &GPIO_InitStructure);

    // MOTORS
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = DRV_AIN1_PIN;
    GPIO_Init(MOTORS_GPIO, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = DRV_AIN2_PIN;
    GPIO_Init(MOTORS_GPIO, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = DRV_BIN1_PIN;
    GPIO_Init(MOTORS_GPIO, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = DRV_BIN2_PIN;
    GPIO_Init(MOTORS_GPIO, &GPIO_InitStructure);

    // EMITTERS
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = EMITTER_LF_PIN;
    GPIO_Init(EMITTER_LF_GPIO, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = EMITTER_LS_PIN;
    GPIO_Init(EMITTER_LS_GPIO, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = EMITTER_F_PIN;
    GPIO_Init(EMITTER_F_GPIO, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = EMITTER_RS_PIN;
    GPIO_Init(EMITTER_RS_GPIO, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = EMITTER_RF_PIN;
    GPIO_Init(EMITTER_RF_GPIO, &GPIO_InitStructure);

    // RECEIVERS
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Pin = RECEIVER_LF_PIN;
    GPIO_Init(RECEIVER_LF_GPIO, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = RECEIVER_LS_PIN;
    GPIO_Init(RECEIVER_LS_GPIO, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = RECEIVER_RS_PIN;
    GPIO_Init(RECEIVER_RS_GPIO, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = RECEIVER_RF_PIN;
    GPIO_Init(RECEIVER_RF_GPIO, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = RECEIVER_F_PIN;
    GPIO_Init(RECEIVER_F_GPIO, &GPIO_InitStructure);

    // Battery
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Pin = BATTERY_PIN;
    GPIO_Init(BATTERY_GPIO, &GPIO_InitStructure);
}

//~ static inline void initEXTI(void)
//~ {
    //~ EXTI_InitTypeDef EXTI_InitStructure = {0};

    //~ // BUTTON
    //~ EXTI_InitStructure.EXTI_Line = EXTI_Line1;
    //~ EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    //~ EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    //~ EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    //~ EXTI_Init(&EXTI_InitStructure);
//~ }

static inline void initTimers(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {0};
    TIM_OCInitTypeDef TIM_OCInitStructure = {0};

    // Microseconds
    TIM_TimeBaseInitStructure.TIM_Period = 0xFFFF;
    TIM_TimeBaseInitStructure.TIM_Prescaler = MCU_rccClocks.PCLK2_Frequency / (uint32_t)1000000 - 1;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);
    TIM_ARRPreloadConfig(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);

    // Buzzer
    TIM_TimeBaseInitStructure.TIM_Period = 160;
    TIM_TimeBaseInitStructure.TIM_Prescaler = 5;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Toggle;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 160;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
    TIM_OC1Init(TIM2, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Disable);
    TIM_ARRPreloadConfig(TIM2, ENABLE);

    // Motors
    TIM_TimeBaseInitStructure.TIM_Period = MOTOR_PWM_MAX;
    TIM_TimeBaseInitStructure.TIM_Prescaler = MCU_rccClocks.PCLK1_Frequency / MOTOR_FREQ / MOTOR_PWM_MAX - 1;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStructure);

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
    TIM_OC1Init(TIM4, &TIM_OCInitStructure);
    TIM_OC2Init(TIM4, &TIM_OCInitStructure);
    TIM_OC3Init(TIM4, &TIM_OCInitStructure);
    TIM_OC4Init(TIM4, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable);
    TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable);
    TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM4, ENABLE);
    TIM_CtrlPWMOutputs(TIM4, ENABLE);
    TIM_Cmd(TIM4, ENABLE);

#ifdef FAN_PWM_MAX
    // Fan
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Disable);
    TIM_ARRPreloadConfig(TIM3, ENABLE);
    TIM_CtrlPWMOutputs(TIM3, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
#endif

    // SysTick (interrupt every 1ms)
    SysTick->CTLR = 0;

    SysTick->CNTL0 = 0;
    SysTick->CNTL1 = 0;
    SysTick->CNTL2 = 0;
    SysTick->CNTL3 = 0;
    SysTick->CNTH0 = 0;
    SysTick->CNTH1 = 0;
    SysTick->CNTH2 = 0;
    SysTick->CNTH3 = 0;

    SysTick->CMPLR0 = (uint8_t)(((MCU_rccClocks.HCLK_Frequency >> 3) / 1000 - 1) >> 0);
    SysTick->CMPLR1 = (uint8_t)(((MCU_rccClocks.HCLK_Frequency >> 3) / 1000 - 1) >> 8);
    SysTick->CMPLR2 = (uint8_t)(((MCU_rccClocks.HCLK_Frequency >> 3) / 1000 - 1) >> 16);
    SysTick->CMPLR3 = (uint8_t)(((MCU_rccClocks.HCLK_Frequency >> 3) / 1000 - 1) >> 24);
    SysTick->CMPHR0 = 0;
    SysTick->CMPHR1 = 0;
    SysTick->CMPHR2 = 0;
    SysTick->CMPHR3 = 0;

    SysTick->CTLR = 1;
}

static inline void initUART(uint32_t baudrate)
{
    USART_InitTypeDef USART_InitStructure = {0};

    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &USART_InitStructure);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);
}

static inline void initSPI(void)
{
    SPI_InitTypeDef  SPI_InitStructure = {0};

    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI2, &SPI_InitStructure);

    SPI_Cmd(SPI2, ENABLE);
}

static inline void initADC(void)
{
    ADC_InitTypeDef ADC_InitStructure = {0};

    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);
    ADC_Cmd(ADC1, ENABLE);

    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1))
        ;
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1))
        ;
    adcCalibrationValue = Get_CalibrationValue(ADC1);
}

static inline void initPFIC(void)
{
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

    // BUTTON
    NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&NVIC_InitStructure);

    // UART
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // SysTick
    NVIC_InitStructure.NVIC_IRQChannel = SysTicK_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_EnableIRQ(SysTicK_IRQn);
}

void MCU_Init(void)
{
    initRCC();
    initGPIO();
    initTimers();
    initUART(115200);
    initSPI();
    initADC();
    initPFIC();
}

void Micros_Wait(uint16_t us)
{
    Micros_Reset();
    while (Micros_Get() < us)
        ;
}

void Millis_Wait(uint32_t ms)
{
    uint32_t current = MCU_millis;
    while (MCU_millis - current < ms)
        ;
}

uint8_t SPI_TransferByte(uint8_t data)
{
    while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET)
        ;
    SPI_I2S_SendData(SPI2, data);
    while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET)
        ;
    return SPI_I2S_ReceiveData(SPI2);
}

void SPI_TransferBytes(uint8_t *in, const uint8_t *out, uint16_t len)
{
    if (in == NULL) {
        while (len--)
            SPI_TransferByte(*out++);
    } else if (out == NULL) {
        while (len--)
            *in++ = SPI_TransferByte(0);
    } else {
        while (len--)
            *in++ = SPI_TransferByte(*out++);
    }
}

void SPI_SetSpeedToNormal(void)
{
    SPI_InitTypeDef  SPI_InitStructure = {0};

    SPI_Cmd(SPI2, DISABLE);
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI2, &SPI_InitStructure);

    SPI_Cmd(SPI2, ENABLE);
}

static uint16_t CalcCalibrated(int16_t val)
{
    if((val + adcCalibrationValue) < 0 || val == 0)
        return 0;
    if((adcCalibrationValue + val) > 4095 || val == 4095)
        return 4095;
    return val + adcCalibrationValue;
}

uint16_t ADC_Read(uint8_t ch)
{
    uint16_t val;

	ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_28Cycles5);
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);

	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC))
        ;
	val = ADC_GetConversionValue(ADC1);

	return CalcCalibrated(val);
}

void *memcpy_v2n(void *_dest, const volatile void *_src, uint16_t n)
{
    uint8_t *dest = _dest;
    const volatile uint8_t *src = _src;

    while (n--) {
        *dest++ = *src++;
    }
    return dest;
}

volatile void *memcpy_n2v(volatile void *_dest, const void *_src, uint16_t n)
{
    volatile uint8_t *dest = _dest;
    const uint8_t *src = _src;

    while (n--) {
        *dest++ = *src++;
    }
    return dest;
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
