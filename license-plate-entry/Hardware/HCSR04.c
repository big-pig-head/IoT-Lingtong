#include "stm32f10x.h"                  // Device header
uint8_t flag=0;				//用于记录中断信号是上升沿还是下降沿
uint32_t number=0;			//记录定时器中断的次数
uint32_t times=0;			//记录回响信号的持续时间

void HCSR04_Init(void)
{

//初始化GPIO口,Trig使用推挽输出,Echo使用浮空输入
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);        //使能GPIOA的外设时钟
    GPIO_InitTypeDef itd;
    itd.GPIO_Mode=GPIO_Mode_Out_PP;                             //选择推挽输出模式
    itd.GPIO_Pin=GPIO_Pin_6;                                    //选择GPIO_Pin_6
    itd.GPIO_Speed=GPIO_Speed_50MHz;                            //默认选择50MHz
    GPIO_Init(GPIOA,&itd);
    
    itd.GPIO_Mode=GPIO_Mode_IN_FLOATING;                        //选择浮空输入模式
    itd.GPIO_Pin=GPIO_Pin_7;                                    //选择GPIO_Pin_7
    GPIO_Init(GPIOA,&itd);
    
    //AFIO映射中断引脚
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);         //使能AFIO的外设时针
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource1);  //选择外部中断源和中断通道
    
    //EXTI中断配置
    EXTI_InitTypeDef itd1;
    itd1.EXTI_Line=EXTI_Line7;                                  //echo使用的端口7,因此选择7号中断线
    itd1.EXTI_LineCmd=ENABLE;
    itd1.EXTI_Mode=EXTI_Mode_Interrupt;
    itd1.EXTI_Trigger=EXTI_Trigger_Rising_Falling;              //上升沿和下降沿都触发中断
    EXTI_Init(&itd1);
    
    //NVIC分配外部中断的中断优先级
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);             //指定中断分组
    NVIC_InitTypeDef itd2;
    itd2.NVIC_IRQChannel=EXTI9_5_IRQn;                          //使用的端口7,因此选择这个参数
    itd2.NVIC_IRQChannelCmd=ENABLE;
    itd2.NVIC_IRQChannelPreemptionPriority=2;                   //抢占优先级
    itd2.NVIC_IRQChannelSubPriority=2;                          //响应优先级 
    NVIC_Init(&itd2);
    
    //配置定时器
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
    TIM_TimeBaseInitTypeDef itd3;
    itd3.TIM_ClockDivision=TIM_CKD_DIV1;                        //使用时钟分频1
    itd3.TIM_CounterMode=TIM_CounterMode_Up;                    //向上计数
    //72MHz/72/100=1000,每秒定时器计数1000个,因此每个计数为100us
    itd3.TIM_Period=72-1;                                       //预分频系数
    itd3.TIM_Prescaler=100-1;                                   //自动重装器
    itd3.TIM_RepetitionCounter=0;                               //该参数仅给高级定时器使用
    TIM_TimeBaseInit(TIM2,&itd3);
    TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);                    //使能中断输出信号
    TIM_InternalClockConfig(TIM2);                              //选择内部时钟    
    
    //NVIC分配定时器的中断优先级
    NVIC_InitTypeDef itd4;
    itd4.NVIC_IRQChannel=TIM2_IRQn;                             //指定Tim2的中断通道
    itd4.NVIC_IRQChannelCmd=ENABLE;
    itd4.NVIC_IRQChannelPreemptionPriority=1;                   //抢占优先级
    itd4.NVIC_IRQChannelSubPriority=1;                          //响应优先级
    NVIC_Init(&itd4);
}


//定时器中断函数
void TIM2_IRQHandler(void){
    if(SET==TIM_GetITStatus(TIM2,TIM_FLAG_Update)){
        number++;                                   //每次中断将次数++
        TIM_ClearITPendingBit(TIM2,TIM_FLAG_Update);
    }
}
 
//外部中断函数
void EXTI9_5_IRQHandler(void){
    if(SET==EXTI_GetITStatus(EXTI_Line7)){
        if(flag==0){
            //上升沿即回响电平开始,打开计数器
            number=0;flag=1;
            TIM_SetCounter(TIM2,0);
            TIM_Cmd(TIM2,ENABLE);
            
        }else{
            //下降沿即回响电平结束,统计高电平持续时长
            TIM_Cmd(TIM2,DISABLE);
            flag=0;
            times=number*100+TIM_GetCounter(TIM2);  //得到回响的高电平持续的us
        }
        EXTI_ClearITPendingBit(EXTI_Line7);
    }
}
uint32_t GetTimes(void)
{
return times;
}

// 超声波模块距离获取函数，ds生成
//uint32_t HCSR04_GetDistance(void) {
//    uint32_t timeout = 0;
//    uint32_t echo_time = 0;
//    
//    // 发送触发脉冲
//    GPIO_SetBits(GPIOA, GPIO_Pin_6);    // Trig引脚置高
//    Delay_us(15);                       // 保持15us高电平
//    GPIO_ResetBits(GPIOA, GPIO_Pin_6);  // Trig引脚置低

//    // 等待回响信号开始（上升沿）
//    while(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7) == RESET) {
//        if(++timeout > 100000) return 0xFFFFFFFF; // 超时返回错误值
//    }
//    
//    // 测量高电平持续时间
//    uint32_t start_time = GetTick();
//    while(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7) != RESET) {
//        if(GetTick() - start_time > 30) break; // 最大测量距离约5米（30ms超时）
//    }
//    echo_time = GetTick() - start_time;

//    // 计算距离（单位：毫米）
//    // 声速340m/s = 0.34mm/us → 公式：距离(mm) = 时间(us)*0.34/2
//    // 由于我们以ms为单位，转换为：距离(mm) = 时间(ms)*340/2 = 时间*170
//    return (echo_time * 170); 
//}




