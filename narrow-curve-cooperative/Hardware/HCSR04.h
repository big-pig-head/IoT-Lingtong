#ifndef __HCSR04_H
#define __HCSR04_H


//void HCSR04_Init(void);

////定时器中断函数;
//void TIM2_IRQHandler(void);
// 
////外部中断函数
//void EXTI9_5_IRQHandler(void);

//uint32_t GetTimes(void);


void HCSR04_1_Init(void);

// 初始化第二个超声波传感器
void HCSR04_2_Init(void);

// 定时器2中断函数，对应第一个超声波传感器
void TIM2_IRQHandler(void);

// 定时器3中断函数，对应第二个超声波传感器
void TIM3_IRQHandler(void);

// 外部中断函数，对应第一个超声波传感器
void EXTI9_5_IRQHandler(void);

// 外部中断函数，对应第二个超声波传感器
void EXTI1_IRQHandler(void);

uint32_t GetTimes1(void);

uint32_t GetTimes2(void);





#endif
