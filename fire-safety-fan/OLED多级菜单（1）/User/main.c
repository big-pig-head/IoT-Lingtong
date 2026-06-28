#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Motor.h"
#include "Key.h"
#include <math.h>
#include "Buzzer.h"


uint8_t KeyNum;		//定义用于接收按键键码的变量
int8_t Speed;		//定义速度变量

#define ALARM_TEMP 24.0 // 报警阈值

void ADC1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct;
    ADC_InitTypeDef ADC_InitStruct;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    
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
 
    // 校准并启动ADC
    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_239Cycles5);
    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}
 
float Get_Temperature(void) {
    static uint16_t buffer[10];
    uint32_t sum = 0;
    
    // 滑动平均滤波
    for(uint8_t i=0; i<10; i++) {
        buffer[i] = ADC_GetConversionValue(ADC1);
        sum += buffer[i];
        Delay_ms(5);
    }
    float adc_value = sum / 10.0f;
    
    // NTC温度转换（需根据实际型号校准）
    float R = 10000.0 * (4095.0 / adc_value - 1.0); // 10K分压电阻
    float temp = 1.0 / (log(R/10000.0)/3950.0 + 1.0/298.15) - 273.15;
    
    return temp;
}




int main(void)
{
	/*模块初始化*/
	SystemInit();
	OLED_Init();		//OLED初始化
	Motor_Init();		//直流电机初始化
	Key_Init();			//按键初始化
	ADC1_Init();
	Buzzer_Init();
	/*显示静态字符串*/
	OLED_ShowString(1, 1, "Speed:");		//1行1列显示字符串Speed:
	
	while (1)
	{
		float temp = Get_Temperature();
		OLED_ShowNum(11, 6, (uint16_t)temp, 2);
        OLED_ShowString(11, 8, "."); 
        OLED_ShowNum(11, 9, (uint16_t)(temp*10)%10, 1);
        OLED_ShowString(11, 10, "C");
		
		
		
		
		KeyNum = Key_GetNum();				//获取按键键码
		if (temp > ALARM_TEMP)					//按键1按下
		{
			Speed = 40;					//速度变量自增20
			//if (Speed > 100)				//速度变量超过100后
			//{
			//	Speed = -100;				//速度变量变为-100
			Buzzer_ON();								//此操作会让电机旋转方向突然改变，可能会因供电不足而导致单片机复位
											//若出现了此现象，则应避免使用这样的操作
			//}
			Motor_SetSpeed(Speed);				//设置直流电机的速度为速度变量
			//OLED_ShowString(3, 1, "ALARM!>99C ");
		}
		else {
		
		Speed = 0;	
		Buzzer_OFF();			//到时候分温度梯度来定风的转速
		Motor_SetSpeed(Speed);
		}
		//Motor_SetSpeed(Speed);				//设置直流电机的速度为速度变量
		OLED_ShowSignedNum(1, 7, Speed, 3);	//OLED显示速度变量
		
		Delay_ms(500);
		
	}
}
