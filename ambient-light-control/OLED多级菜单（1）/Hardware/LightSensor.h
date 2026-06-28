#ifndef __LIGHT_SENSOR_H
#define __LIGHT_SENSOR_H

void LightSensor_Init(void);

uint8_t LightSensor_Get(void);
void ADC1_Init(void);
void TIM3_PWM_Init(void);
void RTC_Init(void);
#endif
