#ifndef __HCSR04_H
#define __HCSR04_H


void HCSR04_Init(void);

//定时器中断函数;
void TIM2_IRQHandler(void);
 
//外部中断函数
void EXTI9_5_IRQHandler(void);

uint32_t GetTimes(void);

#endif
