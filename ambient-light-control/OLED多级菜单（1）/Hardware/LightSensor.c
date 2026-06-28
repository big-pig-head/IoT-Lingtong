#include "stm32f10x.h"                  // Device header

void LightSensor_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//选择GPIO的模式
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;//选择哪个引脚来初始化
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//速度
	GPIO_Init(GPIOB,&GPIO_InitStructure);//取地址，第二个参数
	

} 

uint8_t LightSensor_Get(void)
{
	
	return GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_13);
	
}

// 在main.c中添加以下ADC初始化函数
//void ADC1_Init(void) {
//    GPIO_InitTypeDef GPIO_InitStruct;
//    ADC_InitTypeDef ADC_InitStruct;

//    // 使能时钟
//    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA, ENABLE);
//    
//    // 配置PA0为模拟输入
//    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
//    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;
//    GPIO_Init(GPIOA, &GPIO_InitStruct);

//    // ADC参数配置
//    ADC_InitStruct.ADC_Mode = ADC_Mode_Independent;
//    ADC_InitStruct.ADC_ScanConvMode = DISABLE;
//    ADC_InitStruct.ADC_ContinuousConvMode = ENABLE;
//    ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
//    ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
//    ADC_InitStruct.ADC_NbrOfChannel = 1;
//    ADC_Init(ADC1, &ADC_InitStruct);

//    // 校准并启动
//    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);
//    ADC_Cmd(ADC1, ENABLE);
//    ADC_ResetCalibration(ADC1);
//    while(ADC_GetResetCalibrationStatus(ADC1));
//    ADC_StartCalibration(ADC1);
//    while(ADC_GetCalibrationStatus(ADC1));
//    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
//}

// 在main.c中添加PWM初始化函数
//void TIM3_PWM_Init(void) {
//    GPIO_InitTypeDef GPIO_InitStruct;
//    TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
//    TIM_OCInitTypeDef TIM_OCInitStruct;

//    // 使能时钟
//    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
//    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

//    // 配置PA6为复用推挽输出
//    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6;
//    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
//    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(GPIOA, &GPIO_InitStruct);

//    // 定时器基础配置（1kHz PWM）
//    TIM_TimeBaseStruct.TIM_Period = 999;      // ARR值
//    TIM_TimeBaseStruct.TIM_Prescaler = 71;    // 72MHz/(71+1)=1MHz
//    TIM_TimeBaseStruct.TIM_ClockDivision = 0;
//    TIM_TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
//    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStruct);

//    // PWM通道配置
//    TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
//    TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
//    TIM_OCInitStruct.TIM_Pulse = 0;          // 初始占空比0%
//    TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
//    TIM_OC1Init(TIM3, &TIM_OCInitStruct);

//    TIM_Cmd(TIM3, ENABLE);
//}

void RTC_Init(void) {
    // 1. 启用备份域访问（必要步骤）
    PWR_BackupAccessCmd(ENABLE);
    
    // 2. 启用LSE外部低速晶振（32.768kHz）
    RCC_LSEConfig(RCC_LSE_ON);
    while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);
    
    // 3. 配置RTC时钟源为LSE
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
    RCC_RTCCLKCmd(ENABLE);
    
    // 4. 配置RTC预分频器（32768Hz → 1Hz）
    RTC_SetPrescaler(32768 - 1); // 分频值=时钟频率-1
    
    // 5. 初始化时间（示例：2023年10月1日 00:00:00）
    RTC_SetCounter(0); // 从0秒开始计时
}

