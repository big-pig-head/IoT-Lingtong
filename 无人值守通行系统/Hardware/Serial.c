#include "stm32f10x.h"     // 标准外设库头文件：提供所有寄存器操作函数
#include <stdio.h>          // 标准输入输出，供 fputc / Serial_Printf 使用
#include <stdarg.h>         // 变长参数处理，供 Serial_Printf 使用
#include <string.h>         // 字符串处理（memcpy / strncmp / memset）
#include "Serial.h"         // 本模块对外接口 + 共享变量声明（rx_buffer/rx_flag/valid_plate/宏）

extern void UpdateDisplay(void);  /* 引用 main.c 里定义的显示刷新函数（ProcessLicensePlate 会调用它） */

/* =====================================================================
 * 车辆信息交互 —— USART1 接收方案
 * 方案：UART + DMA（循环模式）+ IDLE 空闲线中断
 * ---------------------------------------------------------------------
 * 旧方案痛点：RXNE 逐字节中断，CPU 每收到 1 字节进一次中断，中断开销
 * 大；PC 端持续下发时若 CPU 未及时消费，数据会被冲掉导致乱码。
 * 改进方案：DMA1_Channel5 以循环模式把 USART1->DR 自动搬入 dma_rx_buf，
 *           一帧发完总线空闲触发 1 次 IDLE 中断，用 CNDTR 算出本帧长度，
 *           中断里把整帧拷到 rx_parse_buf（第二块缓冲，CPU 解析专用）并置标志，
 *           主循环按帧格式做校验与解析，解析结果存 rx_buffer。
 *           双缓冲职责分离：DMA 只写 dma_rx_buf，CPU 只解析 rx_parse_buf，
 *           不冲突、无需乒乓切换。CPU 每帧仅中断一次，帧边界由硬件空闲线
 *           对齐；ProcessLicensePlate 做本地去重，车牌没变不刷新 OLED（配合 PC 节流）。
 *
 * 自定义帧格式（与简历一致）：
 *   [AA][55][CMD][LEN][DATA(LEN字节)][累加和][0D][0A]
 *   帧头AA55 | 指令1B | 数据长度1B | 数据 | 累加和校验1B | 帧尾0D0A
 * ===================================================================== */

/* ---------- 缓冲区大小与帧格式常量 ---------- */
#define FRAME_MAX_LEN  96    /* 单帧最大字节数（含帧头帧尾），宏 RX_BUFFER_SIZE 来自 Serial.h */
#define FRAME_HEAD1    0xAA  /* 帧头第 1 字节：AA */
#define FRAME_HEAD2    0x55  /* 帧头第 2 字节：55 */
#define FRAME_TAIL1    0x0D  /* 帧尾第 1 字节：回车 */
#define FRAME_TAIL2    0x0A  /* 帧尾第 2 字节：换行 */
#define FRAME_CMD_PLATE 0x01 /* 指令码：车牌数据（PC 端下发车牌帧时 CMD 固定为 0x01） */

/* ---------- 接收相关变量（定义在本文件，头文件里用 extern 声明供 main 引用） ---------- */
uint8_t  dma_rx_buf[RX_BUFFER_SIZE];      /* DMA 循环接收缓冲区：DMA 把串口收到的字节按顺序写进来（第一块缓冲，DMA 独占） */
volatile uint16_t dma_fill_prev = 0;      /* 上次已处理到的填充字节数（用于算出"这次新增了多少字节"） */

uint8_t  rx_parse_buf[FRAME_MAX_LEN];     /* 解析专用缓冲区：把本帧从循环缓冲拷贝到这里再解析（第二块缓冲，CPU 独占） */
volatile uint16_t frame_valid_len = 0;    /* 本次收到帧的有效字节数（中断里算出） */
volatile uint8_t  frame_ready = 0;        /* 帧就绪标志：1=收到完整帧待解析（中断置 1），主循环解析后清 0 */

char rx_buffer[RX_BUFFER_SIZE];           /* 解析后的有效数据（车牌字符串），OLED 显示源 */
volatile uint8_t rx_flag = 0;             /* 有合法数据帧待处理标志（校验通过才置 1） */
char last_show_plate[PLATE_MAX_LEN] = {0}; /* 上次成功显示的车牌，本地去重缓存（配合 PC 节流，车牌没变不刷屏） */

/**
  * 串口1初始化：GPIO + USART1 + DMA(循环) + IDLE中断
  * 作用：把接收链路配通——引脚、波特率、DMA 搬运、空闲线中断、NVIC 全部打开。
  */
void Serial_Init(void)
{
	/* ---- 1. 开时钟：USART1 和 GPIOA ---- */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

	/* ---- 2. 配引脚：PA9=TX(推挽复用)，PA10=RX(上拉输入) ---- */
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;   /* 复用推挽输出：PA9 交给 USART1 做发送 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;     /* 上拉输入：PA10 接收数据，上拉保证空闲为高电平 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* ---- 3. 配 USART1：9600，8 位，无校验，1 停止位，收发都开 ---- */
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 9600;                                   /* 波特率 9600（和 PC 端一致） */
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; /* 无硬件流控 */
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;              /* 收发都使能 */
	USART_InitStructure.USART_Parity = USART_Parity_No;                          /* 无校验位 */
	USART_InitStructure.USART_StopBits = USART_StopBits_1;                       /* 1 个停止位 */
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;                  /* 8 位数据 */
	USART_Init(USART1, &USART_InitStructure);

	/* ---- 4. 配 DMA1 通道5：把 USART1_RX 收到的数据自动搬进内存，循环模式 ---- */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);   /* 开 DMA1 时钟 */

	DMA_InitTypeDef DMA_InitStructure;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR; /* 源：串口数据寄存器（外设） */
	DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t)dma_rx_buf; /* 目的：内存缓冲区 */
	DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralSRC;/* 方向：外设→内存 */
	DMA_InitStructure.DMA_BufferSize         = RX_BUFFER_SIZE;       /* 一次搬多少字节（=缓冲大小） */
	DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable; /* 外设地址不递增（始终是 DR） */
	DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;     /* 内存地址递增（依次写入缓冲） */
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; /* 外设数据宽度：字节 */
	DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;     /* 内存数据宽度：字节 */
	DMA_InitStructure.DMA_Mode               = DMA_Mode_Circular;           /* 循环模式：填满从头再填，环形 */
	DMA_InitStructure.DMA_Priority           = DMA_Priority_High;           /* DMA 优先级高 */
	DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;             /* 非内存到内存 */
	DMA_Init(DMA1_Channel5, &DMA_InitStructure);
	DMA_Cmd(DMA1_Channel5, ENABLE);         /* 启动 DMA 搬运 */

	/* ---- 5. 把 USART1 的"接收请求"交给 DMA：收到字节就自动触发 DMA 搬一次 ---- */
	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);

	/* ---- 6. 使能 IDLE 空闲线中断：一帧发完总线空闲时触发一次（而不是每字节一次） ---- */
	USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);

	/* ---- 7. 配 NVIC：打开 USART1 中断，设优先级 ---- */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);   /* 中断分组：抢占 2 位 + 子 2 位 */
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;            /* USART1 的中断号 */
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;              /* 使能 */
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;    /* 抢占优先级 1 */
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;           /* 子优先级 1 */
	NVIC_Init(&NVIC_InitStructure);

	USART_Cmd(USART1, ENABLE);   /* 最后使能 USART1（收发真正开始工作） */
}

/**
  * USART1 中断：IDLE 空闲线中断（接收帧的关键函数）
  * 作用：当一帧发完、总线出现空闲时触发一次。
  *       1) 读 SR/DR 清掉 IDLE 标志；
  *       2) 用 CNDTR 算出 DMA 目前已填充的字节数，减去上次的 = 本次新增的帧长；
  *       3) 把这一帧从循环缓冲按序拷到 rx_parse_buf，置 frame_ready，等主循环解析。
  * 中断里只做"收帧"，不做业务解析（解析在主循环 Serial_ProcessFrame，避免阻塞中断）。
  */
void USART1_IRQHandler(void)
{
	if (USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)   /* 确实是"空闲线"事件吗 */
	{
		/* 清 IDLE 标志：标准做法是先读 SR 再读 DR（读操作本身清除标志） */
		(void)USART1->SR;
		(void)USART1->DR;

		/* 当前 DMA 已经填了多少字节：缓冲总大小 - 剩余未用计数（CNDTR 寄存器） */
		uint16_t fill = (uint16_t)(RX_BUFFER_SIZE - DMA_GetCurrDataCounter(DMA1_Channel5));
		/* 本次新增字节数：用取模算，能正确处理环形缓冲"回绕"的情况 */
		uint16_t n = (uint16_t)((fill - dma_fill_prev + RX_BUFFER_SIZE) % RX_BUFFER_SIZE);

		if (n != 0 && n <= FRAME_MAX_LEN)   /* 有新增且长度在合理范围 */
		{
			/* 把这 n 个字节从循环缓冲按序拷到 rx_parse_buf（下标取模，支持跨边界自动拼接） */
			for (uint16_t i = 0; i < n; i++)
			{
				rx_parse_buf[i] = dma_rx_buf[(dma_fill_prev + i) % RX_BUFFER_SIZE];
			}
			frame_valid_len = n;  /* 记录本帧字节数 */
			frame_ready = 1;      /* 置"一帧已收齐"标志，通知主循环去解析 */
		}
		dma_fill_prev = fill;   /* 更新"已处理到的位置"，为下一帧做准备 */
	}
}

/**
  * 帧解析（主循环中调用，接收/解析的核心函数）
  * 作用：拿到 rx_parse_buf 里的一帧，依次校验【帧头→长度→帧尾→累加和】，
  *       全部通过才把数据区拷进 rx_buffer 并置 rx_flag；任何一步不过就丢弃。
  * 帧格式：[AA][55][CMD][LEN][DATA][累加和][0D][0A]
  */
void Serial_ProcessFrame(void)
{
	if (!frame_ready) return;   /* 没有收到完整帧就退出 */
	frame_ready = 0;            /* 清接收标志（本次帧已开始处理） */

	uint8_t *p = rx_parse_buf;      /* 指向解析缓冲区（中断已把整帧拷到这里） */
	uint16_t len = frame_valid_len; /* 本帧字节数 */

	/* 最短帧：帧头2 + 指令1 + 长度1 + 校验1 + 帧尾2 = 7 字节，少于就丢弃 */
	if (len < 7) return;

	/* 帧头校验：必须是 AA 55 */
	if (p[0] != FRAME_HEAD1 || p[1] != FRAME_HEAD2) return;

	uint8_t cmd  = p[2];   /* 指令：0x01 代表车牌数据 */
	uint8_t dlen = p[3];   /* 数据长度：DATA 区有几个字节 */

	/* 指令码校验：当前只支持车牌指令 0x01，其他类型帧直接丢弃（让 CMD 字段真正生效） */
	if (cmd != FRAME_CMD_PLATE) return;

	/* 长度越界保护：4(头+指令+长度) + 数据 + 2(帧尾) 必须在帧内，防止越界读 */
	if ((uint16_t)(4 + dlen + 2) >= len) return;

	/* 帧尾校验：数据区后面必须是 0D 0A */
	if (p[4 + dlen + 1] != FRAME_TAIL1 || p[4 + dlen + 2] != FRAME_TAIL2) return;

	/* 累加和校验：对帧头到数据末尾所有字节求和，结果必须等于校验字节 */
	uint8_t sum = 0;
	for (uint16_t i = 0; i < (uint16_t)(4 + dlen); i++) sum += p[i];
	if (p[4 + dlen] != sum) return;   /* 累加和不符：数据传输出错，丢弃 */

	/* 校验全通过：把数据区(DATA)拷到 rx_buffer，补上结束符，置 rx_flag 通知业务 */
	if (dlen > 0 && dlen < RX_BUFFER_SIZE)
	{
		memcpy(rx_buffer, &p[4], dlen);   /* 拷贝 dlen 个有效数据（车牌字符串） */
		rx_buffer[dlen] = '\0';           /* 手动补字符串结束符 */
		rx_flag = 1;                      /* 置位：通知主循环有合法车牌待处理 */
	}
}

/**
  * 处理车牌数据（主循环中调用）
  * 作用：检查 rx_flag，判断解析出的 rx_buffer 是有效车牌还是"无车牌"标志，
  *       更新 valid_plate 状态并刷新 OLED 显示，最后清 rx_flag 等待下一帧。
  * 依赖：rx_buffer/rx_flag(本文件)、valid_plate(main.c，经 Serial.h 声明)、UpdateDisplay(main.c)
  */
void ProcessLicensePlate(void) {
    if(rx_flag) {                                    /* 串口收到一帧合法数据（Serial_ProcessFrame 校验通过后置位） */
        /* 检测是否为无效车牌：PC 端模型识别不到车牌时会下发"No Plate"标志 */
        if(strncmp(rx_buffer, "No Plate", 8) == 0) {
            valid_plate = 0;                         /* 标记为无效 */
            memset(rx_buffer, 0, sizeof(rx_buffer)); /* 清空车牌缓冲，避免残留旧值 */
            last_show_plate[0] = '\0';               /* 重置本地去重缓存：进入无车牌状态 */
            UpdateDisplay();                         /* 刷新 OLED 显示（清屏显示无车） */
        } else {
            valid_plate = 1;                         /* 是真实车牌，标记有效 */
            /* 本地去重：车牌没变则不刷新 OLED（配合 PC 端节流，避免反复刷屏、省 CPU） */
            if(strcmp(rx_buffer, last_show_plate) != 0) {
                strncpy(last_show_plate, rx_buffer, PLATE_MAX_LEN - 1); /* 更新去重缓存为当前车牌 */
                last_show_plate[PLATE_MAX_LEN - 1] = '\0';              /* 确保字符串结束符 */
                UpdateDisplay();                     /* 刷新 OLED 显示（车牌/剩余车位/距离） */
            }
        }
        rx_flag = 0;                                 /* 清除接收标志，等待下一帧 */
    }
}

/* ============================ 发送部分（保持不变） ============================ */

/* 发送单个字节：写进 DR，等发送寄存器空（TXE） */
void Serial_SendByte(uint8_t Byte)
{
	USART_SendData(USART1, Byte);
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

/* 发送一数组字节：逐个调用 Serial_SendByte */
void Serial_SendArray(uint8_t *Array, uint16_t Length)
{
	uint16_t i;
	for (i = 0; i < Length; i ++)
	{
		Serial_SendByte(Array[i]);
	}
}

/* 发送字符串：遇到 '\0' 结束 */
void Serial_SendString(const char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i ++)
	{
		Serial_SendByte(String[i]);
	}
}

/* 求 10 的幂，供发送数字用（如 Serial_Pow(10,3)=1000） */
uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y --)
	{
		Result *= X;
	}
	return Result;
}

/* 按位发送一个数字（定长，高位不足补 '0'） */
void Serial_SendNumber(uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i ++)
	{
		Serial_SendByte(Number / Serial_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/* 重定向 printf：让 printf 输出到串口 */
int fputc(int ch, FILE *f)
{
	Serial_SendByte(ch);
	return ch;
}

/* 格式化打印：把格式串按变长参数拼成字符串再经串口发出（串口版 printf） */
void Serial_Printf(char *format, ...)
{
	char String[100];
	va_list arg;
	va_start(arg, format);
	vsprintf(String, format, arg);
	va_end(arg);
	Serial_SendString(String);
}
