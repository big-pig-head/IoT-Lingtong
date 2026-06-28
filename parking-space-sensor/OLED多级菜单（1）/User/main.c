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


//uint8_t KeyNum;					//定义用于接收键码的变量
//float Angle=0;					//定义角度变量
//int Int_Spaces=10;				//定义剩余停车位
//int Int_PanDuanSpaces_One=0;    //用来辅助剩余停车位的计数
//int Int_PanDuanSpaces_Two=0;	//用来辅助剩余停车位的计数
//int Int_WhileOne=0;
//float Float_dis=0.0;
//float Float_dis_real=0.0;
//float Float_temp;
uint32_t Use_Time=0;			//计算时间
uint32_t distance=0;				//超声波传感器和障碍物的距离
int a=0;

#define ALARM_TEMP 99.0// 报警阈值


//取5次为平均值
uint32_t HCSR04_GetDistance(void) {

	for(int i=0;i<5;++i){             					 //每次取10次测距数据,取平均值减少误差
            GPIO_SetBits(GPIOA,GPIO_Pin_6);
            Delay_us(15);                 				  	//根据说明书,需要提供至少10us的高电平
            GPIO_ResetBits(GPIOA,GPIO_Pin_6);
            Delay_ms(65); 									//根据说明书,每个周期至少需要等待60ms
			Use_Time=GetTimes();
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
	OLED_Init();			//OLED初始化
	//Servo_Init();			//舵机初始化
	//Key_Init();			//按键初始化
	//LightSensor_Init();		//光敏传感器DO初始化
	//ADC1_Init();        // 初始化光敏传感器ADC，光敏AO初始化
	HCSR04_Init();			//超声波传感器，//初始化GPIO口,Trig使用推挽输出,Echo使用浮空输入
	LED_Init2();			//红绿led初始化
	//TIM3_PWM_Init();    // 初始化PWM调光
	Buzzer_Init();
	ADC1_Init();
	
	
	
	/*显示静态字符串*/
	OLED_ShowString(1, 1, "Welcome!");	//1行1列显示字符串Angle:
	//OLED_ShowString(2, 1, "Spaces:");
	//OLED_ShowNum(2, 8, Int_Spaces, 3);	//OLED显示剩余车位数量
	//Servo_SetAngle(Angle);	//设置舵机的角度为角度变量
	 
	
	GPIO_ResetBits(GPIOA,GPIO_Pin_1);//一开始，使绿灯亮
	GPIO_SetBits(GPIOA,GPIO_Pin_2);//一开始，使红灯灭
	
    //uint32_t distance;
    while(1){
        //distance=0;
        //HCSR04_GetDistance();					//显示超声波传感器和障碍物的距离 ,distance
		OLED_ShowString(4, 1, "distance:");
		OLED_ShowNum(4,10,HCSR04_GetDistance(),4);													//distance的单位是mm
		OLED_ShowString(4, 14, "mm");
		//int a=0;

		 float temp = Get_Temperature();
		OLED_ShowString(3, 1, "heat:");
		OLED_ShowNum(3, 6, (uint16_t)temp, 2);
        OLED_ShowString(3, 8, "."); 
        OLED_ShowNum(3, 9, (uint16_t)(temp*10)%10, 1);
        OLED_ShowString(3, 10, "C");
		
		
		// 报警控制
        if(temp > ALARM_TEMP) {
            Buzzer_ON();
			
            OLED_ShowString(3, 1, "ALARM!>99C ");
        } else {
            Buzzer_OFF();
			
            float temp = Get_Temperature();
			OLED_ShowString(3, 1, "heat:");
			OLED_ShowNum(3, 6, (uint16_t)temp, 2);
			OLED_ShowString(3, 8, "."); 
			OLED_ShowNum(3, 9, (uint16_t)(temp*10)%10, 1);
			OLED_ShowString(3, 10, "C");
        }
        
        Delay_ms(500);
		
		
		
		
		
		
		
		if (HCSR04_GetDistance() <= 100)				//当超声波传感器测量出来的距离小于等于1m
		{
			
			//Angle = 90;
			
			GPIO_WriteBit(GPIOA,GPIO_Pin_2,Bit_SET);//点亮红灯，代表有车占位
			GPIO_WriteBit(GPIOA,GPIO_Pin_1,Bit_RESET);//熄灭绿灯
			
			
			OLED_ShowString(2, 1, "occupied");
			//Int_PanDuanSpaces_One=1;
			//Servo_SetAngle(Angle);	//设置舵机的角度为角度变量
			
			
//			if(distance > 1000)
//			{
//			a=0;
//			}
			
		}
			if(HCSR04_GetDistance() > 100)
			{
			//Int_WhileOne=0;
			//Angle = 0;
			
			GPIO_WriteBit(GPIOA,GPIO_Pin_1,Bit_SET);//点亮绿灯，代表没车占位
			GPIO_WriteBit(GPIOA,GPIO_Pin_2,Bit_RESET);//熄灭红灯	
				
			OLED_ShowString(2, 1, "free          ");
			//Servo_SetAngle(Angle);	//设置舵机的角度为角度变量     
			//Int_PanDuanSpaces_Two=1;
			//break;
			}
			
		
		//Servo_SetAngle(Angle);			//设置舵机的角度为角度变量
		//Delay_ms(500);					//延时500ms，看成车辆通过闸口的时间，一过，车位加一
		//if (LightSensor_Get() == 0)				//按键1按下
		//{
			
		//	Angle = 0;
			
		//	Int_PanDuanSpaces_Two=1;
		//}
		
		Delay_ms(500);	
//		if(Int_PanDuanSpaces_One ==1 && Int_PanDuanSpaces_Two ==1)
//		{
//			if(Int_Spaces !=0)
//			{Int_Spaces--;}
//			
//			OLED_ShowNum(2, 8, Int_Spaces, 3);	//OLED显示剩余车位
//			Int_PanDuanSpaces_One =0;
//			Int_PanDuanSpaces_Two =0;
//		}
		
		
		
    }
}
 

