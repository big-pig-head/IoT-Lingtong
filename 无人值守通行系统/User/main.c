#include "stm32f10x.h"        // 标准外设库：寄存器操作函数
#include "Delay.h"            // 延时函数（Delay_us / Delay_ms）
#include "OLED.h"             // OLED 显示驱动
#include "string.h"           // 字符串处理函数
#include "Serial.h"           // 串口模块（帧接收/解析/车牌处理）
#include "Servo.h"            // 舵机驱动
#include "LightSensor.h"      // 光敏传感器
#include "HCSR04.h"           // 超声波测距
#include "stm32f10x_iwdg.h"   // 独立看门狗 IWDG

/* ============================ 类型定义 ============================ */
// 道闸状态枚举：整个系统只保留两个业务状态，用标志承载"动作中/超时/异常"
typedef enum {
    GATE_CLOSED,   // 道闸关闭
    GATE_OPENED    // 道闸已打开
} GateState;

/* ============================ 全局变量 ============================ */
volatile uint32_t SystemTick = 0;   // 系统时间戳（1ms 单位），由 TIM4 中断累加
int Int_Spaces = 5;                 // 剩余车位数量
extern uint32_t distance;           // 引用 HCSR04.c 的测距结果（供显示与道闸判断）
extern uint32_t Use_Time;           // 引用 HCSR04.c 的定义（本文件不再定义）

GateState gate_state = GATE_CLOSED; // 当前道闸状态
uint8_t  gate_busy = 0;            // 道闸动作进行中标志：1=正在抬/落杆（不阻塞主循环）
uint8_t  gate_target;              // 动作完成后要进入的状态（GATE_OPENED / GATE_CLOSED）
uint32_t gate_action_start = 0;    // 动作开始时刻（GetTick，ms），用于判断到位/超时
uint8_t  gate_error = 0;           // 动作/舵机异常标志（超时未到位置 1）
uint8_t  valid_plate = 0;          // 有效车牌标志（1=有效，0=无效）

// ===== 道闸动作阈值（依据 SG90 标称与代码）=====
#define SERVO_MOVE_MS     500   /* 舵机正常到位时间：SG90 0°↔90° 带杆约 0.5s */
#define SERVO_TIMEOUT_MS 3000   /* 舵机超时：超过 3s 判定异常，回归安全位 */
#define ANTI_COLLISION_MM 300   /* 防砸阈值：落杆中杆下障碍 < 300mm 立刻重新抬杆 */

/* ==================== 函数前置声明（main 靠前，实现后置） ==================== */
uint32_t GetTick(void);         // 获取系统时间（ms）
void SystemTick_Init(void);     // 系统时基初始化（TIM4 1ms 中断）
void TIM4_IRQHandler(void);     // TIM4 更新中断：SystemTick++（中断函数，由向量表调用）
void IWDG_Init(void);           // 独立看门狗初始化
void IWDG_Refresh(void);        // 刷新看门狗（主循环喂狗）
void UpdateDisplay(void);       // 刷新 OLED 显示（车位/车牌/距离）
void ProcessGate(void);         // 道闸状态机（非阻塞 + 超时 + 防砸）

/* ============================ 主入口 ============================ */
int main(void)
{
    /* ---- 硬件初始化：先建时基、开看门狗，再初始化各外设 ---- */
    SystemTick_Init();   // 先建立 1ms 系统时基（时间戳，供状态机/超时使用）
    IWDG_Init();         // 独立看门狗初始化（整机死机兜底）
    OLED_Init();         // OLED 显示初始化
    Serial_Init();       // 串口初始化（UART+DMA+IDLE 接收车牌）
    Servo_Init();        // 舵机（PWM）初始化
    LightSensor_Init();  // 光敏传感器初始化（当前未使用）
    HCSR04_Init();       // 超声波初始化（EXTI+TIM 测距）

    /* ---- 初始显示 ---- */
    OLED_Clear();        // 清屏
    UpdateDisplay();     // 显示初始状态（车位/无车牌/距离）
    Servo_SetAngle(0);   // 舵机回 0 度（道闸初始关闭）

    /* ---- 主循环：周期调度各业务模块 ---- */
    while(1)
    {
        IWDG_Refresh();          // 喂看门狗（放主循环，任何状态都执行）
        HCSR04_GetDistance();    // 超声波测距：更新 distance（供道闸判断与防砸）
        UpdateDisplay();         // 刷新 OLED
        Serial_ProcessFrame();   // 帧解析：校验通过才置 rx_flag（DMA+IDLE 已收整帧）
        ProcessLicensePlate();   // 处理车牌：判断有效/无效 + 本地去重
        ProcessGate();           // 道闸状态机：非阻塞 + 超时 + 防砸
        UpdateDisplay();         // 再次刷新（车牌/道闸变化后即时显示）
    }
}

/* ==================== 函数实现（后置） ==================== */

// 获取系统时间（单位：ms）—— 非阻塞超时、心跳都基于它
uint32_t GetTick(void)
{
    return SystemTick;
}

// 系统时基初始化：TIM4 产生 1ms 中断，每毫秒 SystemTick++。
// 为什么不用 SysTick：stm32f10x_it.c 已定义空 SysTick_Handler，且 Delay_us/ms 占用 SysTick 做阻塞延时，
// 直接启用 SysTick 会重复定义 / 相互干扰。改用独立 TIM4，SysTick 留给延时，互不冲突。
void SystemTick_Init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);   // 开 TIM4 时钟

    TIM_TimeBaseInitTypeDef t;
    t.TIM_ClockDivision = TIM_CKD_DIV1;     // 采样时钟不分频
    t.TIM_CounterMode = TIM_CounterMode_Up; // 向上计数
    t.TIM_Period = 1000 - 1;                // ARR=999：计数 1000 次溢出一次
    t.TIM_Prescaler = 72 - 1;               // PSC=71：72MHz/72=1MHz，每 1us 计一次
    t.TIM_RepetitionCounter = 0;            // 仅高级定时器用，填 0
    TIM_TimeBaseInit(TIM4, &t);             // 写寄存器 → 1MHz 下计 1000 次 = 1ms 溢出

    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE); // 使能更新中断（溢出即 1ms）

    NVIC_InitTypeDef n;
    n.NVIC_IRQChannel = TIM4_IRQn;              // TIM4 中断
    n.NVIC_IRQChannelCmd = ENABLE;              // 使能
    n.NVIC_IRQChannelPreemptionPriority = 2;    // 抢占优先级 2
    n.NVIC_IRQChannelSubPriority = 2;           // 子优先级 2
    NVIC_Init(&n);

    TIM_Cmd(TIM4, ENABLE);                      // 启动 TIM4
}

// TIM4 更新中断：每 1ms 进来一次，系统时间戳 +1
void TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)   // 确实是更新事件
    {
        SystemTick++;                                    // 时间戳累加
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);      // 清中断标志
    }
}

// 独立看门狗初始化：LSI 时钟 + 预分频 + 重载 + 使能
void IWDG_Init(void)
{
    RCC_LSICmd(ENABLE);                                        // 启用 LSI 内部低速时钟（看门狗专用）
    while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);        // 等待 LSI 就绪

    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);              // 允许写看门狗配置寄存器
    IWDG_SetPrescaler(IWDG_Prescaler_256);                     // 预分频 256
    IWDG_SetReload(0xFFF);                                     // 重载值（溢出约 26s，兜底整机死机）

    IWDG_ReloadCounter();                                      // 先刷新一次
    IWDG_Enable();                                             // 启动看门狗
}

// 刷新看门狗：主循环定期喂狗，任何状态都执行
void IWDG_Refresh(void)
{
    IWDG_ReloadCounter();
}

// 刷新 OLED 显示：车位 / 车牌 / 距离 三块信息
void UpdateDisplay(void)
{
    // ---- 第 1/3 行：剩余车位（>0 欢迎，==0 禁止）----
    if(Int_Spaces > 0)
    {
        OLED_ShowString(0, 0, "欢迎光临!");             // 欢迎语
        OLED_ShowString(0, 32, "剩余车位:");            // 标签
        OLED_ShowNum(72, 32, Int_Spaces, 3, OLED_8X16); // 显示剩余车位数字
        OLED_Update();                                  // 刷新
    }
    else
    {
        OLED_ShowString(0, 0, "车位不足禁止通行");      // 无车位提示
        OLED_ShowString(0, 32, "剩余车位:");
        OLED_ShowNum(72, 32, Int_Spaces, 3, OLED_8X16);
        OLED_Update();
    }

    // ---- 第 2 行：车牌状态 ----
    if(valid_plate)
    {
        OLED_ShowString(0, 16, rx_buffer);   // 显示识别到的车牌
        OLED_Update();
    }
    else
    {
        OLED_ShowString(0, 16, "No Plate    ");  // 无车牌提示
        OLED_Update();
    }

    // ---- 第 4 行：距离 ----
    OLED_ShowString(0, 48, "Dist:");                // 距离标签
    OLED_ShowNum(48, 48, distance, 4, OLED_8X16);   // 距离值(mm)
    OLED_ShowString(88, 48, "mm");                  // 单位
    OLED_Update();                                  // 刷新屏幕
}

// 道闸状态机：两个状态 + 非阻塞 + 超时 + 防砸
void ProcessGate(void)
{
    switch(gate_state)
    {
        case GATE_CLOSED:
            // 空闲：有效车牌 + 杆前有车 + 有车位 → 抬杆（非阻塞，不再 Delay）
            if(!gate_busy)
            {
                if(valid_plate && (distance < 100) && Int_Spaces > 0)
                {
                    Servo_SetAngle(90);             // 舵机90度抬杆，发 PWM 后不阻塞
                    gate_busy = 1;                  // 标记抬杆动作进行中
                    gate_target = GATE_OPENED;      // 动作完成后进入"已开"
                    gate_action_start = GetTick();  // 记录开始时刻（非阻塞计时）
                }
            }
            else
            {
                // 抬杆进行中：主循环照常跑（能读超声波/收串口/刷OLED）
                if(GetTick() - gate_action_start >= SERVO_MOVE_MS)
                {
                    gate_busy = 0;                  // 正常到位
                    gate_state = GATE_OPENED;
                }
                else if(GetTick() - gate_action_start >= SERVO_TIMEOUT_MS)
                {
                    // 超时未到位 = 舵机异常：回归安全位 + 报错，不卡死
                    gate_error = 1;
                    Servo_SetAngle(0);              // 回安全关闭位
                    gate_busy = 0;
                }
            }
            break;

        case GATE_OPENED:
            // 杆已全开：杆前无车 → 落杆（非阻塞，不再 Delay）
            if(!gate_busy)
            {
                if(distance > 100)
                {
                    Servo_SetAngle(0);              // 舵机0度落杆，发 PWM 后不阻塞
                    gate_busy = 1;
                    gate_target = GATE_CLOSED;
                    gate_action_start = GetTick();
                }
            }
            else
            {
                // 落杆中防砸：杆下障碍 < 300mm → 立刻重新抬杆
                if(gate_target == GATE_CLOSED && distance < ANTI_COLLISION_MM)
                {
                    Servo_SetAngle(90);             // 重新抬杆
                    gate_target = GATE_OPENED;      // 目标改为"重新打开"
                    gate_action_start = GetTick();  // 重新计时
                    break;
                }
                if(GetTick() - gate_action_start >= SERVO_MOVE_MS)
                {
                    gate_busy = 0;                  // 动作到位
                    if(gate_target == GATE_OPENED)
                        gate_state = GATE_OPENED;   // 防砸重抬到位，保持打开
                    else
                    {
                        gate_state = GATE_CLOSED;   // 正常落杆到位
                        if(Int_Spaces > 0) Int_Spaces--; // 减少车位
                        valid_plate = 0;            // 重置车牌
                        gate_error = 0;
                        UpdateDisplay();
                    }
                }
                else if(GetTick() - gate_action_start >= SERVO_TIMEOUT_MS)
                {
                    // 动作超时 = 异常：回归安全位 + 报错
                    gate_error = 1;
                    Servo_SetAngle(90);             // 异常时回到抬杆安全位，防砸优先
                    gate_busy = 0;
                }
            }
            break;
    }
}
