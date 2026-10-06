#include "stm32f10x.h"                  // Device header —— 标准外设库头文件，提供所有寄存器操作函数
#include "Delay.h"                       // 延时函数头文件（提供 Delay_us / Delay_ms）

/* ============ 全局变量：跨多个函数共用，先统一声明在文件顶部 ============ */
uint8_t flag=0;                          // 状态标记：flag=0 表示"等待上升沿(待触发)"，flag=1 表示"计时中"
uint32_t number=0;                       // 记录 TIM2 溢出的次数：每溢出一次表示计时多走了 100us
uint32_t times=0;                        // 最终算出的 Echo 高电平持续时间（us 量级）
uint32_t Use_Time=0;                     // 临时变量：存放"本次测距读到的时长"，供 HCSR04_GetDistance 使用
uint32_t distance = 0;                   // 测距结果，单位：毫米（mm）

/* ============ 1. 初始化函数：把测距要用到的硬件全部配好 ============ */
void HCSR04_Init(void)
{
    /* ---- 第一步：配 GPIO。Trig(PA6)=推挽输出，Echo(PA7)=浮空输入 ---- */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);   // 开 GPIOA 的时钟：引脚必须先有时钟才能工作
    GPIO_InitTypeDef itd;                                   // 定义一个 GPIO 配置结构体
    itd.GPIO_Mode=GPIO_Mode_Out_PP;                         // 推挽输出：既能拉高又能拉低，用来做 Trig（主控要主动"发"脉冲），这个推挽输出是主控的gpio口设置的，主控主动输出高低电平发信号给trig，然后trig怎么操作我们不需要理
    itd.GPIO_Pin=GPIO_Pin_6;                                // 选中 PA6 作为 Trig 引脚
    itd.GPIO_Speed=GPIO_Speed_50MHz;                        // 输出速度 50MHz（越高翻转越快）
    GPIO_Init(GPIOA,&itd);                                  // 把上面的配置真正写入寄存器

    itd.GPIO_Mode=GPIO_Mode_IN_FLOATING;                    // 浮空输入：只"读"外部进来的电平，用来做 Echo（等传感器给信号）
    itd.GPIO_Pin=GPIO_Pin_7;                                // 选中 PA7 作为 Echo 引脚
    GPIO_Init(GPIOA,&itd);                                  // 写寄存器

    /* ---- 第二步：配 EXTI。让 Echo(PA7) 的电平跳变能触发 CPU 中断 ---- */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);    // 开 AFIO 时钟：外部中断靠它把"引脚"连接到"中断线"
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource7); // ⚠️注意：这行把 PA1 连到 EXTI_Line1，而 Echo 实际在 PA7。功能上能否正常需核对实际接线，严谨应改为 GPIO_PinSource7 对齐下方 EXTI_Line7

    EXTI_InitTypeDef itd1;
    itd1.EXTI_Line=EXTI_Line7;                              // 使用第 7 号中断线（PA7 属于这条线）
    itd1.EXTI_LineCmd=ENABLE;                               // 使能这条中断线
    itd1.EXTI_Mode=EXTI_Mode_Interrupt;                     // 中断模式（不是事件模式）
    itd1.EXTI_Trigger=EXTI_Trigger_Rising_Falling;          // 上升沿+下降沿都触发：因为"开始"和"结束"都要知道
    EXTI_Init(&itd1);                                       // 写寄存器

    /* ---- 第三步：配 NVIC。给外部中断定优先级并打开 ---- */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);         // 中断分组方式：抢占优先级 2 位 + 子优先级 2 位
    NVIC_InitTypeDef itd2;
    itd2.NVIC_IRQChannel=EXTI9_5_IRQn;                      // PA7 属于第 5~9 号中断线，统一走 EXTI9_5 这个中断号
    itd2.NVIC_IRQChannelCmd=ENABLE;                         // 使能该中断
    itd2.NVIC_IRQChannelPreemptionPriority=2;               // 抢占优先级 2（数字越小越优先，2 属中等）
    itd2.NVIC_IRQChannelSubPriority=2;                      // 子优先级 2
    NVIC_Init(&itd2);                                       // 写寄存器

    /* ---- 第四步：配 TIM2。用来"计时"，量 Echo 高电平持续了多久 ---- */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);    // 开 TIM2 的时钟
    TIM_TimeBaseInitTypeDef itd3;
    itd3.TIM_ClockDivision=TIM_CKD_DIV1;                    // 采样时钟不分频
    itd3.TIM_CounterMode=TIM_CounterMode_Up;                // 向上计数：0→71→溢出回 0
    // 72MHz 主频 + PSC=99 → 计数时钟 = 72MHz/100 = 720kHz，即每 1.389us 计一次；
    // ARR=71 → 每 72 个数 = 72×1.389us ≈ 100us 溢出一次，溢出一次就触发一次更新中断（number++）
    itd3.TIM_Period=72-1;                                   // ARR=71：计数器计到 71 就溢出，重载系数
    itd3.TIM_Prescaler=100-1;                               // PSC=99：预分频系数
    itd3.TIM_RepetitionCounter=0;                           // 仅高级定时器使用，这里填 0
    TIM_TimeBaseInit(TIM2,&itd3);                           // 写寄存器
    TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);                // 使能"更新中断"：每次溢出都进一次中断（number++）
    TIM_InternalClockConfig(TIM2);                          // 选择内部时钟驱动定时器

    /* ---- 第五步：配 NVIC。TIM2 的中断优先级（比 EXTI 高，因为计时不能丢） ---- */
    NVIC_InitTypeDef itd4;
    itd4.NVIC_IRQChannel=TIM2_IRQn;                         // TIM2 的中断通道
    itd4.NVIC_IRQChannelCmd=ENABLE;                         // 使能该中断
    itd4.NVIC_IRQChannelPreemptionPriority=1;               // 抢占优先级 1（比 EXTI 的 2 更高）
    itd4.NVIC_IRQChannelSubPriority=1;                      // 子优先级 1
    NVIC_Init(&itd4);                                       // 写寄存器
}

/* ============ 2. 定时器中断服务函数：TIM2 每溢出一次（100us）就进来，number+1 ============ */
void TIM2_IRQHandler(void){
    if(SET==TIM_GetITStatus(TIM2,TIM_FLAG_Update)){        // 先确认确实是"更新(溢出)"事件来了
        number++;                                           // 每溢出一次 = 计时多走了 100us
        TIM_ClearITPendingBit(TIM2,TIM_FLAG_Update);        // 清掉中断标志，否则会一直重复进中断
    }
}

/* ============ 3. 外部中断服务函数：Echo 电平跳变（上升/下降沿）就进来 —— 测距核心 ============ */
void EXTI9_5_IRQHandler(void){
    if(SET==EXTI_GetITStatus(EXTI_Line7)){                  // 是 Echo 所在的第 7 号中断线触发吗
        if(flag==0){
            // flag==0 → 这次是"上升沿"：回声刚开始，开始计时
            number=0;                                       // 溢出计数归零
            flag=1;                                         // 标记"现在处于计时中"
            TIM_SetCounter(TIM2,0);                         // TIM2 计数器也归零
            TIM_Cmd(TIM2,ENABLE);                           // 启动 TIM2，开始计时 ← 起点

        }else{
            // flag==1 → 这次是"下降沿"：回声结束，停止计时
            TIM_Cmd(TIM2,DISABLE);                          // 停表 ← 终点
            flag=0;                                         // 状态复位为"等待下一次上升沿"
            times=number*100+TIM_GetCounter(TIM2);          // 总时长 = 溢出次数×100 + 当前计数（us 量级）
        }
        EXTI_ClearITPendingBit(EXTI_Line7);                 // 清掉中断标志
    }
}

/* ============ 4. 读取函数：把最后一次量到的 Echo 高电平时长交出去 ============ */
uint32_t GetTimes(void)
{
    return times;
}

/* ============ 5. 测距函数：触发 5 次测量，取平均得到距离（毫米） ============ */
uint32_t HCSR04_GetDistance(void) {
    for(int i=0;i<5;++i){                                   // 循环 5 次测距取平均，减小随机误差
        GPIO_SetBits(GPIOA,GPIO_Pin_6);                     // Trig 拉高
        Delay_us(15);                                       // 保持 15us（说明书要求至少 10us）
        GPIO_ResetBits(GPIOA,GPIO_Pin_6);                   // Trig 拉低 → 传感器收到"开始"命令，开始发波
        Delay_ms(65);                                       // 等 65ms，给声波一个完整的往返时间
        Use_Time=GetTimes();                                // 读取刚量好的 Echo 高电平时长
        distance+=(Use_Time/5.8);                           // 时长÷5.8≈毫米（声速340m/s→0.34mm/us，往返÷2，即÷5.88 近似 5.8），每计数1us说明波传播了0.34mm
    }
    distance/=5;                                            // 取平均
    return distance;                                        // 返回毫米数
}
