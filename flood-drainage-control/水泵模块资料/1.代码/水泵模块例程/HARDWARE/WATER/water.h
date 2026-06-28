#ifndef __WATER_H
#define	__WATER_H
#include "stm32f10x.h"
#include "adcx.h"
#include "delay.h"
#include "math.h"

/*****************辰哥单片机设计******************
											STM32
 * 文件			:	水位传感器h文件                   
 * 版本			: V1.0
 * 日期			: 2025.1.27
 * MCU			:	STM32F103C8T6
 * 接口			:	见代码							
 * BILIBILI	:	辰哥单片机设计
 * CSDN			:	辰哥单片机设计
 * 作者			:	辰哥

**********************BEGIN***********************/

#define WATER_READ_TIMES	10  //WATER传感器ADC循环读取次数

//模式选择	
//模拟AO:	1             这个可以显示数值，传感器的数值
//数字DO:	0             这个只能显示0/1
#define	WATER_MODE 	1

/***************根据自己需求更改****************/
// WATER GPIO宏定义
#if WATER_MODE
#define		WATER_AO_GPIO_CLK								RCC_APB2Periph_GPIOA
#define 	WATER_AO_GPIO_PORT							GPIOA
#define 	WATER_AO_GPIO_PIN								GPIO_Pin_3
#define   	WATER_ADC_CHANNEL                			ADC_Channel_3	// ADC 通道宏定义

#else
#define		WATER_DO_GPIO_CLK								RCC_APB2Periph_GPIOA
#define 	WATER_DO_GPIO_PORT							GPIOA
#define 	WATER_DO_GPIO_PIN								GPIO_Pin_1			

#endif
/*********************END**********************/


void WATER_Init(void);
uint16_t WATER_GetData(void);

#endif /* __WATER_H */

