#include "bump.h"

/*****************辰哥单片机设计******************
											STM32
 * 文件			:	5V水泵模块c文件                   
 * 版本			: V1.0
 * 日期			: 2024.9.22
 * MCU			:	STM32F103C8T6
 * 接口			:	见代码							
 * BILIBILI	:	辰哥单片机设计
 * CSDN			:	辰哥单片机设计
 * 作者			:	辰哥

**********************BEGIN***********************/


void BUMP_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(BUMP_CLK, ENABLE ); //配置时钟
	
	GPIO_InitStructure.GPIO_Pin = BUMP_GPIO_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(BUMP_GPIO_PROT,&GPIO_InitStructure);

	BUMP_OFF;
}

