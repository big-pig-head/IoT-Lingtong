#include "stm32f10x.h"                  // Device header
#include "Delay.h"           //使用的是b站江科大的延时函数,可以自己用别的替代延时功能
#include "OLED.h"            //使用的是b站江科大的OLED驱动代码,用于展示测距结果,可以在相应的地方更换为串口通信展示到电脑的串口助手上
#include "HCSR04.h"
#include "Servo.h"
#include "Key.h"
#include "LightSensor.h"
#include "LED.h"
#include <math.h>
#include "Buzzer.h"


uint32_t Use_Time=0;			//计算时间
uint32_t distance=0;				//超声波传感器和障碍物的距离
int a=0;

uint32_t distance1 = 0, distance2 = 0;
 
// 防死锁状态标志
volatile uint8_t conflict_flag = 0; 


volatile uint8_t active_oled = 0;  // 当前活动屏幕：0-OLED1，1-OLED2
uint32_t last_switch_time = 0;
#define SWITCH_INTERVAL 500       // 切换间隔500ms

volatile uint32_t sys_tick = 0;
 
//void SysTick_Handler(void) {
//    sys_tick++;  // 每1ms自动递增
//}
 
uint32_t Get_Tick(void) {
    return sys_tick;
}




// OLED显示切换函数
void Switch_OLED_Display(void) {
    if (Get_Tick() - last_switch_time > SWITCH_INTERVAL) {
        active_oled = !active_oled;
        last_switch_time = Get_Tick();
        
        // 切换时清空非活动屏幕
        if (active_oled == 0) {
            OLED2_Clear();
        } else {
            OLED_Clear();
        }
    }
}
 
// 显示内容更新函数
void Update_Display(void) {
    if (active_oled == 0) { // OLED1活动
        OLED_ShowString(4, 1, "distance:");
        OLED_ShowNum(4,10, distance1, 4);
        OLED_ShowString(4, 14, "mm");
    } else { // OLED2活动
        OLED2_ShowString(4, 1, "distance:");
        OLED2_ShowNum(4,10, distance2, 4);
        OLED2_ShowString(4, 14, "mm");
    }
}




//取5次为平均值 ,超声波传感器1
uint32_t HCSR04_1_GetDistance(void) {

	for(int i=0;i<5;++i){             					 //每次取10次测距数据,取平均值减少误差
            GPIO_SetBits(GPIOA,GPIO_Pin_6);
            Delay_us(15);                 				  	//根据说明书,需要提供至少10us的高电平
            GPIO_ResetBits(GPIOA,GPIO_Pin_6);
            Delay_ms(65); 									//根据说明书,每个周期至少需要等待60ms
			Use_Time=GetTimes1();
            distance+=(Use_Time/5.8);          				//根据说明书提供的公式,获取单位为mm的距离
        }
        distance/=5;
		return distance;
}

//取5次为平均值 ,超声波传感器2
uint32_t HCSR04_2_GetDistance(void) {

	for(int i=0;i<5;++i){             					 //每次取10次测距数据,取平均值减少误差
            GPIO_SetBits(GPIOA,GPIO_Pin_6);
            Delay_us(15);                 				  	//根据说明书,需要提供至少10us的高电平
            GPIO_ResetBits(GPIOA,GPIO_Pin_6);
            Delay_ms(65); 									//根据说明书,每个周期至少需要等待60ms
			Use_Time=GetTimes2();
            distance+=(Use_Time/5.8);          				//根据说明书提供的公式,获取单位为mm的距离
        }
        distance/=5;
		return distance;
}



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



int main(void){
	
		/*模块初始化*/
	SystemInit();
	SysTick_Config(SystemCoreClock / 1000);
	OLED_Init();			//OLED初始化
	OLED2_Init();			//OLED初始化
	
	HCSR04_2_Init();			//超声波传感器，//初始化GPIO口,Trig使用推挽输出,Echo使用浮空输入
    HCSR04_1_Init();
	
	Buzzer_Init();

	
	
	
	OLED_ShowString(1, 1, "System Ready");
    OLED2_ShowString(1, 1, "System Ready");
    Delay_ms(1000);
    OLED_Clear();
    OLED2_Clear();

    while(1){
		
		 /* 获取两个传感器的距离 */
        distance1 = HCSR04_1_GetDistance();  // 传感器1距离（单位：mm）
        distance2 = HCSR04_2_GetDistance();  // 传感器2距离（单位：mm）
		
		
//        //distance=0;
//        //HCSR04_GetDistance();					//显示超声波传感器和障碍物的距离 ,distance
//		OLED_ShowString(4, 1, "distance:");
//		OLED_ShowNum(4,10,HCSR04_1_GetDistance(),4);													//distance的单位是mm
//		OLED_ShowString(4, 14, "mm");
//		
//		OLED2_ShowString(4, 1, "distance:");
//		OLED2_ShowNum(4,10,HCSR04_2_GetDistance(),4);													//distance的单位是mm
//		OLED2_ShowString(4, 14, "mm");
		
		
		/* 切换显示控制 */
        Switch_OLED_Display();
        
        /* 更新当前活动屏幕内容 */
        Update_Display();


		
		
		 /* 会车逻辑控制 */
        if (distance1 <= 1000 || distance2 <= 1000) {
            Buzzer_ON();
            Delay_ms(200);
            Buzzer_OFF();
            
            if (active_oled == 0) { // OLED1显示控制
                if (distance1 <= 1000 && distance2 > 1000) {
                    OLED_ShowString(1, 1, "Car Detected  ");
                    OLED_ShowString(2, 1, "Direction: GO ");
                    OLED2_Clear();
                } 
                else if (distance2 <= 1000 && distance1 > 1000) {
                    OLED2_ShowString(1, 1, "Car Detected  ");
                    OLED2_ShowString(2, 1, "Direction: GO ");
                    OLED_Clear();
                }
                else if (distance1 <= 1000 && distance2 <= 1000) {
                    OLED_ShowString(1, 1, "!CONFLICT!    ");
                    OLED_ShowString(2, 1, "Priority:SIDE2");
                    OLED2_ShowString(1, 1, "GO            ");
                    OLED2_ShowString(2, 1, "SIDE1 WAIT    ");
                    // 冲突处理逻辑...
					
					
					while (distance1 <= 1000 && distance2 <= 1000) {
                    Buzzer_ON();
                    distance1 = HCSR04_1_GetDistance();
                    distance2 = HCSR04_2_GetDistance();
                    Delay_ms(100);
                }
                Buzzer_OFF();
                conflict_flag = 0;
					
                }
            } 
            else { // OLED2显示控制
                if (distance2 <= 1000 && distance1 > 1000) {
                    OLED2_ShowString(1, 1, "Car Detected  ");
                    OLED2_ShowString(2, 1, "Direction: GO ");
                    OLED_Clear();
                } 
                else if (distance1 <= 1000 && distance2 > 1000) {
                    OLED_ShowString(1, 1, "Car Detected  ");
                    OLED_ShowString(2, 1, "Direction: GO ");
                    OLED2_Clear();
                }
                else if (distance1 <= 1000 && distance2 <= 1000) {
                    OLED2_ShowString(1, 1, "!CONFLICT!    ");
                    OLED2_ShowString(2, 1, "Priority:SIDE1");
                    OLED_ShowString(1, 1, "GO            ");
                    OLED_ShowString(2, 1, "SIDE2 WAIT    ");
                    // 冲突处理逻辑...
					
					
					while (distance1 <= 1000 && distance2 <= 1000) {
                    Buzzer_ON();
                    distance1 = HCSR04_1_GetDistance();
                    distance2 = HCSR04_2_GetDistance();
                    Delay_ms(100);
                }
                Buzzer_OFF();
                conflict_flag = 0;
					
                }
            }
        } 
        else { // 空闲状态
            if (active_oled == 0) {
                OLED_ShowString(1, 1, "Status: Free  ");
                OLED_ShowString(2, 1, "SIDE1 Ready   ");
            } else {
                OLED2_ShowString(1, 1, "Status: Free  ");
                OLED2_ShowString(2, 1, "SIDE2 Ready   ");
            }
        }
        
        Delay_ms(100); // 整体刷新率控制
		
		
		
		
		
		
		
		
		
		
    }
}
 

