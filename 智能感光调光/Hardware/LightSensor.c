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

