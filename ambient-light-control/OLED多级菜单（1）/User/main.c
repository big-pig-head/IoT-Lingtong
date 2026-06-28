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

























//#include "stm32f10x.h"                  // Device header
//#include "stm32f10x_rtc.h"
//#include "Delay.h"
//#include "Buzzer.h"
//#include "LightSensor.h"
//#include <math.h>
////uint8_t KeyNum;
//// 非线性亮度计算函数
//uint8_t Calculate_Brightness(uint16_t adc_val) {
//    // ADC范围：0（全暗）~4095（全亮）
//    if(adc_val > 3500) return 100;    // 完全黑暗时全亮
//    else if(adc_val < 500) return 0;  // 强光环境关闭
//    else {
//        // 指数曲线映射（更符合人眼感知）
//        float normalized = (3500.0 - adc_val) / 3000.0;
//        return (uint8_t)(pow(normalized, 2.2) * 100);
//    }
//}
// 
//// 设置PWM占空比
//void Set_LED_Brightness(uint8_t percent) {
//    uint16_t pulse = (uint16_t)(percent * 9.99); // 映射到0-999
//    TIM_SetCompare1(TIM3, pulse);
//}
// 
//// 简化版节能模式（移除RTC依赖）
//uint8_t Is_EnergySaving_Mode(void) {
//    // 示例：模拟时间（实际需替换为其他计时方式）
//    static uint32_t fake_counter = 0;
//    fake_counter++;
//    uint8_t fake_hour = (fake_counter / 3600) % 24; // 每3600次循环模拟1小时
//    
//    if(fake_hour >= 23 || fake_hour < 6) {
//        return 1;
//    }
//    return 0;
//}
//int main(void)
//{
//	SystemInit();
//	RTC_Init();
//	Buzzer_Init();
//	LightSensor_Init();
//	ADC1_Init();        // 光敏传感器ADC
//    TIM3_PWM_Init();    // PWM调光初始化
//	//LED_Init();//这样就完成led的初始化了
//	//Key_Init();
//	
//	//GPIO_ResetBits(GPIOA,GPIO_Pin_0);//使灯泡亮
//	//GPIO_SetBits(GPIOA,GPIO_Pin_0);//使灯泡灭
//	//GPIO_WriteBit(GPIOA,GPIO_Pin_0,Bit_SET);
//	while(1)
//	{
//	//led闪烁逻辑，点亮一会，停止一会，然后循环
//	//	GPIO_WriteBit(GPIOA,GPIO_Pin_0,Bit_RESET);//点亮
//	//	Delay_ms(500);//毫秒延时函数,延时的时间越短，灯闪的越快
//	//	GPIO_WriteBit(GPIOA,GPIO_Pin_0,Bit_SET);//熄灭
//	//	Delay_ms(500);//毫秒延时函数
//		
//	//	LED1_ON();
//	//	LED2_OFF();
//	//	Delay_ms(500);
//	//	LED1_OFF();
//	//	LED2_ON();
//	//	Delay_ms(500);
//	
//	//	KeyNum = Key_GetNum();//不断读取按键码
//	//	if(KeyNum ==1 )
//	//	{
//	//		LED1_Turn();
//	//	}
//		
//	//	if(KeyNum ==2 )
//	//	{
//	//		LED2_Turn();
//	//	}
//		
//	//	Buzzer_ON();
//	//	Delay_ms(500);
//	//	Buzzer_OFF();
//	//	Delay_ms(500);
//	//	Buzzer_Turn();
//	//	Delay_ms(500);
//	//	Buzzer_Turn();
//	//	Delay_ms(500);
//	
//	
////	if(LightSensor_Get() == 1)//光线比较暗的情况下
////	{
////	
////		Buzzer_ON();
////		
////	}
////	else
////	{
////		
////	Buzzer_OFF();
////		
////	}
//	
//	/* 核心调光逻辑 */
//        uint16_t adc_value = ADC_GetConversionValue(ADC1);
//        uint8_t brightness = Calculate_Brightness(adc_value);
//        Set_LED_Brightness(brightness);
//        
//        /* 节能模式处理 */
//        if(Is_EnergySaving_Mode()) {
//            Set_LED_Brightness(20); // 夜间最低亮度
//        }
//        
//        Delay_ms(200);  // 控制采样频率
//	
//	}
//	
//	
//	// 临时修改main函数，测试LED硬件

////    GPIO_InitTypeDef GPIO_InitStruct;
////    
////    // 使能PA6时钟
////    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
////    
////    // 配置PA6为推挽输出
////    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6;
////    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
////    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
////    GPIO_Init(GPIOA, &GPIO_InitStruct);
////    
////    while(1) {
////        GPIO_SetBits(GPIOA, GPIO_Pin_6); // 亮
////        Delay_ms(500);
////        GPIO_ResetBits(GPIOA, GPIO_Pin_6); // 灭
////        Delay_ms(500);
////    }

//	
//}


