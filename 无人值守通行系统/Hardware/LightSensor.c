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
