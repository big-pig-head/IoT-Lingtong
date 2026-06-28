#include "stm32f10x.h"                  // Device header
//#include "hc_sr04.h"
#include "delay.h"

//端口宏定义
#define Tring_GPIO GPIOB
#define Tring_PIN  GPIO_Pin_12		//Trig：超声信号触发引脚
#define Echo_GPIO GPIOB			
#define Echo_PIN GPIO_Pin_13		//Echo：超声信号接收引脚

void Hcsr04_Init(void)
{
    //使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
    
    /**初始化GPIO端口*/
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = Tring_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(Tring_GPIO,&GPIO_InitStruct);
    
    GPIO_InitStruct.GPIO_Pin = Echo_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(Echo_GPIO,&GPIO_InitStruct);
    
    /**初始化TIM2，进行计时*/
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
    TIM_TimeBaseInitStruct.TIM_CounterMode=TIM_CounterMode_Up;
    TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStruct.TIM_Period=0xFFFF;
    TIM_TimeBaseInitStruct.TIM_Prescaler=72-1; //1us计数一次
    TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStruct);

    TIM_Cmd(TIM2,ENABLE);//使能/开启定时器
}

//一个指向浮点数的指针dis（用于存储计算出的距离）
//和一个浮点数temp（表示当前温度，因为声速随温度变化）。

float vGetDistance(float dis,float temp)				//不用温度传感器前，温度参数先设置为20度
{
	
	Delay_us(10);
    u16 time = 0;
														//用于存储超声波从发射到接收的时间。

														/**Tring引脚拉高10us的TTL电平，使模块发送超声波*/
    GPIO_SetBits(Tring_GPIO,Tring_PIN);
														//通过GPIO设置函数，将名为Tring的GPIO端口的Tring_PIN引脚置高电平。这通常用于触发超声波传感器发送超声波脉冲。
														//vDelayUs(10);										//微妙延迟
														//Delay_ms(10);										//毫秒延迟
	
	Delay_us(10);	
    GPIO_ResetBits(Tring_GPIO,Tring_PIN);
														//通过GPIO重置函数，将Tring端口的Tring_PIN引脚置低电平，停止超声波的发送。
														/*获取回波时间，高电平持续时间38ms为无回波时返回时间*/
	
	
    while(GPIO_ReadInputDataBit(Echo_GPIO,Echo_PIN) == 0);
														//这是一个空循环，它会一直执行直到Echo_GPIO端口的Echo_PIN引脚检测到高电平。这表示超声波已经遇到障碍物并反射回来，开始被接收。
    TIM_SetCounter(TIM2,0);
														//将定时器2（TIM2）的计数器清零。这是为了准备测量超声波回波的时间。
    while(GPIO_ReadInputDataBit(Echo_GPIO,Echo_PIN) == 1);
														//这是另一个空循环，它会一直执行直到Echo_GPIO端口的Echo_PIN引脚检测到低电平。这表示超声波回波已经被完全接收。
    time = TIM_GetCounter(TIM2);
														// 读取定时器2的计数器值，并将其存储在变量time中。这个值表示超声波从发送到接收的时间。
														//计算距离
	
	
    dis = time*(334.1+0.6*temp) / 20000;				//这里dis的单位是m
														//声速随温度每升高1°C增加约0.6m/s，20°C时声速约为343m/s
														//vDelayMs(5); //等待下一次开始，官方建议60ms采样周期，自己根据情况修改
														//Delay_ms(10);
	//Delay_ms(30);	
	//Delay_us(10);	
	
	return dis;
}

