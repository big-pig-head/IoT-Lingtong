#ifndef __HCSR04_H__
#define __HCSR04_H__
#include "stm32f10x.h"

/**HC-SR04超声模块初始化*/
void Hcsr04_Init(void);
/**获取距离，单位CM*/
float vGetDistance(float dis,float temp);

#endif

