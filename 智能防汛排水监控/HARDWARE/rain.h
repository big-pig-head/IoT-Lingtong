#ifndef __RAIN_H
#define	__RAIN_H
#include "stm32f10x.h"
#include "adcx.h"
#include "delay.h"
#include "math.h"



#define RAIN_READ_TIMES	10  //雨滴传感器ADC循环读取次数

//模式选择	
//模拟AO:	1
//数字DO:	0
#define	MODE 1

/***************修改后的引脚配置****************/
// 将雨滴传感器AO口从PA0改为PA2
// RAIN GPIO宏定义                                                                          
#if MODE
#define		RAIN_AO_GPIO_CLK								RCC_APB2Periph_GPIOA
#define 	RAIN_AO_GPIO_PORT								GPIOA
#define 	RAIN_AO_GPIO_PIN								GPIO_Pin_2  // 从Pin_0改为Pin_2
#define   	RAIN_ADC_CHANNEL               			ADC_Channel_2	// ADC通道从0改为2

#else
#define		RAIN_DO_GPIO_CLK								RCC_APB2Periph_GPIOA
#define 	RAIN_DO_GPIO_PORT								GPIOA
#define 	RAIN_DO_GPIO_PIN								GPIO_Pin_3  // 如果需要DO也可相应调整			

#endif
/*********************END**********************/


void RAIN_Init(void);
uint16_t RAIN_GetData(void);

#endif /* __RAIN_H */
