#include "stm32f10x.h"
#include "Delay.h"
#include "LightSensor.h"
#include <math.h>

// PWM初始化（TIM3_CH1→PA6）
void TIM3_PWM_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
    TIM_OCInitTypeDef TIM_OCInitStruct;

    // 使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    // 配置PA6为复用推挽输出
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 定时器基础配置（1kHz PWM）
    TIM_TimeBaseStruct.TIM_Period = 999;      // ARR=999 → 1kHz
    TIM_TimeBaseStruct.TIM_Prescaler = 71;     // 72MHz/(71+1)=1MHz
    TIM_TimeBaseStruct.TIM_ClockDivision = 0;
    TIM_TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStruct);

    // PWM通道配置
    TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStruct.TIM_Pulse = 0;            // 初始占空比0%
    TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM3, &TIM_OCInitStruct);

    TIM_Cmd(TIM3, ENABLE);
}

// ADC初始化（PA0）
void ADC1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct;
    ADC_InitTypeDef ADC_InitStruct;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA, ENABLE);
    
    // 配置PA0为模拟输入
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // ADC参数配置
    ADC_InitStruct.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStruct.ADC_ScanConvMode = DISABLE;
    ADC_InitStruct.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStruct.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStruct);

    // 启动ADC
    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);
    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

// 修改后的亮度计算函数
uint8_t Calculate_Brightness(uint16_t adc_val) {
    if (adc_val < 500) {         // 强光环境
        return 0;              // LED关闭
    } else if (adc_val > 3500) {  // 全暗环境
        return 100;                // LED全亮
    } else {
        float normalized = (adc_val - 500.0f) / 3000.0f;
		//float normalized = (500.0f-adc_val ) / 3000.0f;
        //return (uint8_t)((1.0f - pow(normalized, 2.2)) * 100);
		return (uint8_t)(pow(normalized, 2.2) * 100);
    }
}

// 设置PWM占空比
void Set_LED_Brightness(uint8_t percent) {
    uint16_t pulse = (uint16_t)(percent * 9.99f); // 映射到0-999
    TIM_SetCompare1(TIM3, pulse);
}

int main(void) {
    SystemInit();
    ADC1_Init();        // 初始化光敏传感器ADC
    TIM3_PWM_Init();    // 初始化PWM调光
    
    while(1) {
        // 1. 读取光敏传感器ADC值
        uint16_t adc_value = ADC_GetConversionValue(ADC1);
        
        // 2. 计算目标亮度（非线性映射）
        uint8_t brightness = Calculate_Brightness(adc_value);
        
        // 3. 设置LED亮度
        Set_LED_Brightness(brightness);
        
        // 4. 延时控制采样率（200ms）
        Delay_ms(200);
    }
}
