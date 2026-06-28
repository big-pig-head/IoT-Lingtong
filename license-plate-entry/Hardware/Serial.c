#include "stm32f10x.h"                  // Device header
#include <stdio.h>
#include <stdarg.h>
//#include <Serial.h>

//char Serial_RxPacket[100];				//定义接收数据包数组，数据包格式"@MSG\r\n"
//uint8_t Serial_RxFlag;					//定义接收数据包标志位

#define RX_BUFFER_SIZE 128
//作用：定义串口接收缓冲区的最大长度
//典型值选择：根据最长车牌号长度设定（中文车牌通常8字符，含汉字需要更大缓冲区）

char rx_buffer[RX_BUFFER_SIZE];
//作用：声明接收缓冲区数组
//内存分配：占用128字节的RAM空间，用于临时存储接收的原始数据

volatile uint16_t rx_index = 0;
//volatile：防止编译器优化，确保中断与主程序都能看到最新值
//用途：记录当前缓冲区写入位置，范围0~127

volatile uint8_t rx_flag = 0;

volatile uint8_t jr6001_cmd_flag = 0;  // JR6001指令接收标志
//utf-8编码
// 在全局添加
typedef enum {
    UTF8_ASCII = 0,
    UTF8_2BYTE,
    UTF8_3BYTE
} UTF8_State;
 
volatile UTF8_State utf8_state = UTF8_ASCII;
volatile uint8_t utf8_counter = 0;



/**
  * 函    数：串口初始化
  * 参    数：无
  * 返 回 值：无
  */
void Serial_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);	//开启USART1的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//开启GPIOA的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将PA9引脚初始化为复用推挽输出
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将PA10引脚初始化为上拉输入
	
	/*USART初始化*/
	USART_InitTypeDef USART_InitStructure;					//定义结构体变量
	USART_InitStructure.USART_BaudRate = 9600;				//波特率
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	//硬件流控制，不需要
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;	//模式，发送模式和接收模式均选择
	USART_InitStructure.USART_Parity = USART_Parity_No;		//奇偶校验，不需要
	USART_InitStructure.USART_StopBits = USART_StopBits_1;	//停止位，选择1位
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;		//字长，选择8位
	USART_Init(USART1, &USART_InitStructure);				//将结构体变量交给USART_Init，配置USART1
	
	/*中断输出配置*/
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);			//开启串口接收数据的中断
	
	/*NVIC中断分组*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);			//配置NVIC为分组2
	
	/*NVIC配置*/
	NVIC_InitTypeDef NVIC_InitStructure;					//定义结构体变量
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;		//选择配置NVIC的USART1线
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//指定NVIC线路使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;		//指定NVIC线路的抢占优先级为1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		//指定NVIC线路的响应优先级为1
	NVIC_Init(&NVIC_InitStructure);							//将结构体变量交给NVIC_Init，配置NVIC外设
	
	/*USART使能*/
	USART_Cmd(USART1, ENABLE);								//使能USART1，串口开始运行
}

/**
  * 函    数：串口发送一个字节
  * 参    数：Byte 要发送的一个字节
  * 返 回 值：无
  */
void Serial_SendByte(uint8_t Byte)
{
	USART_SendData(USART1, Byte);		//将字节数据写入数据寄存器，写入后USART自动生成时序波形
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);	//等待发送完成
	/*下次写入数据寄存器会自动清除发送完成标志位，故此循环后，无需清除标志位*/
}

/**
  * 函    数：串口发送一个数组
  * 参    数：Array 要发送数组的首地址
  * 参    数：Length 要发送数组的长度
  * 返 回 值：无
  */
void Serial_SendArray(uint8_t *Array, uint16_t Length)
{
	uint16_t i;
	for (i = 0; i < Length; i ++)		//遍历数组
	{
		Serial_SendByte(Array[i]);		//依次调用Serial_SendByte发送每个字节数据
	}
}

/**
  * 函    数：串口发送一个字符串
  * 参    数：String 要发送字符串的首地址
  * 返 回 值：无
  */
void Serial_SendString(const char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i ++)//遍历字符数组（字符串），遇到字符串结束标志位后停止
	{
		Serial_SendByte(String[i]);		//依次调用Serial_SendByte发送每个字节数据
	}
}

/**
  * 函    数：次方函数（内部使用）
  * 返 回 值：返回值等于X的Y次方
  */
uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;	//设置结果初值为1
	while (Y --)			//执行Y次
	{
		Result *= X;		//将X累乘到结果
	}
	return Result;
}

/**
  * 函    数：串口发送数字
  * 参    数：Number 要发送的数字，范围：0~4294967295
  * 参    数：Length 要发送数字的长度，范围：0~10
  * 返 回 值：无
  */
void Serial_SendNumber(uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i ++)		//根据数字长度遍历数字的每一位
	{
		Serial_SendByte(Number / Serial_Pow(10, Length - i - 1) % 10 + '0');	//依次调用Serial_SendByte发送每位数字
	}
}

/**
  * 函    数：使用printf需要重定向的底层函数
  * 参    数：保持原始格式即可，无需变动
  * 返 回 值：保持原始格式即可，无需变动
  */
int fputc(int ch, FILE *f)
{
	Serial_SendByte(ch);			//将printf的底层重定向到自己的发送字节函数
	return ch;
}

/**
  * 函    数：自己封装的prinf函数
  * 参    数：format 格式化字符串
  * 参    数：... 可变的参数列表
  * 返 回 值：无
  */
void Serial_Printf(char *format, ...)
{
	char String[100];				//定义字符数组
	va_list arg;					//定义可变参数列表数据类型的变量arg
	va_start(arg, format);			//从format开始，接收参数列表到arg变量
	vsprintf(String, format, arg);	//使用vsprintf打印格式化字符串和参数列表到字符数组中
	va_end(arg);					//结束变量arg
	Serial_SendString(String);		//串口发送字符数组（字符串）
}

/**
  * 函    数：USART1中断函数
  * 参    数：无
  * 返 回 值：无
  * 注意事项：此函数为中断函数，无需调用，中断触发后自动执行
  *           函数名为预留的指定名称，可以从启动文件复制
  *           请确保函数名正确，不能有任何差异，否则中断函数将不能进入
  */
//void USART1_IRQHandler(void)
//{
//	static uint8_t RxState = 0;		//定义表示当前状态机状态的静态变量
//	static uint8_t pRxPacket = 0;	//定义表示当前接收数据位置的静态变量
//	if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)	//判断是否是USART1的接收事件触发的中断，!= RESET = == SET
//	{
////////		作用：检查是否发生"接收寄存器非空"中断
////////		USART_IT_RXNE：接收中断标志位，当DR寄存器中有数据时触发
////////		返回值判断：RESET表示标志位未置位，SET表示需要处理
//		
//		
//		
//		uint8_t RxData = USART_ReceiveData(USART1);			//读取数据寄存器，存放在接收的数据变量
////		操作过程：
////			读取USART1的DR寄存器值
////			自动清除RXNE标志位
////			注意：必须读取DR寄存器才能清除中断标志
//		
//		
//		/*使用状态机的思路，依次处理数据包的不同部分*/
//		
//		/*当前状态为0，接收数据包包头*/
//		if (RxState == 0)
//		{
//			if (RxData == '@' && Serial_RxFlag == 0)		//如果数据确实是包头，并且上一个数据包已处理完毕
//			{
//				RxState = 1;			//置下一个状态
//				pRxPacket = 0;			//数据包的位置归零
//			}
//		}
//		/*当前状态为1，接收数据包数据，同时判断是否接收到了第一个包尾*/
//		else if (RxState == 1)
//		{
//			if (RxData == '\r')			//如果收到第一个包尾
//			{
//				RxState = 2;			//置下一个状态
//			}
//			else						//接收到了正常的数据
//			{
//				Serial_RxPacket[pRxPacket] = RxData;		//将数据存入数据包数组的指定位置
//				pRxPacket ++;			//数据包的位置自增
//			}
//		}
//		/*当前状态为2，接收数据包第二个包尾*/
//		else if (RxState == 2)
//		{
//			if (RxData == '\n')			//如果收到第二个包尾
//			{
//				RxState = 0;			//状态归0
//				Serial_RxPacket[pRxPacket] = '\0';			//将收到的字符数据包添加一个字符串结束标志
//				Serial_RxFlag = 1;		//接收数据包标志位置1，成功接收一个数据包
//			}
//		}
//		
//		USART_ClearITPendingBit(USART1, USART_IT_RXNE);		//清除标志位
//	}
//}

//void USART1_IRQHandler(void)
//{
//    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
//    {
//        char ch = USART_ReceiveData(USART1);
//        
//        /* 移除非必需的状态机逻辑 */
//        if (rx_index < RX_BUFFER_SIZE - 1)
//        {
//            if (ch == '\n')     // 仅用\n作为结束符
//            {
//                rx_buffer[rx_index] = '\0';
//                rx_flag = 1;    // 通知主程序
//                rx_index = 0;
//            }
//            else
//            {
//                // UTF-8字符处理（支持中文）
//                if ((ch & 0x80) != 0)  // 检测多字节字符
//                {
//                    // 需要连续接收3字节（UTF-8中文）
//                    // 此处需添加多字节处理逻辑（见下方说明）
//					
//					 if ((ch & 0xE0) == 0xC0) {
//						utf8_state = UTF8_2BYTE;
//						utf8_counter = 1;
//				} else if ((ch & 0xF0) == 0xE0) {
//						utf8_state = UTF8_3BYTE;
//						utf8_counter = 2;
//						}
//				}
//				else
//			{
//						utf8_state = UTF8_ASCII;
//					
//             }
//			
//			 //// 正常存储字节
//                rx_buffer[rx_index++] = ch;
//			 
//			 // 处理多字节计数
//if (utf8_state != UTF8_ASCII)
//{
//    if (utf8_counter > 0) {
//        utf8_counter--;
//    } else {
//        utf8_state = UTF8_ASCII;
//    }
//}
//			 
//            }
//        }
//        else
//        {
//            rx_index = 0; // 溢出时重置
//        }
//        
//        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
//    }
//}

void USART1_IRQHandler(void) {
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET) {
        char ch = USART_ReceiveData(USART1);
        
		/* 处理JR6001二进制指令 */
//        if (jr6001_cmd_flag) {
//            // 解析JR6001响应（根据手册实现）
//            // ...
//        } 
//        /* 处理车牌文本数据 */
//        else if (ch == 0xAA) {  // JR6001指令头
//            jr6001_cmd_flag = 1;
//            // 启动JR6001指令解析缓冲区
//        } else {
        if (rx_index < RX_BUFFER_SIZE - 1) {
            if (ch == '\n') {  // 检测换行符
                rx_buffer[rx_index] = '\0';  // 终止字符串
                rx_flag = 1;    // 通知主程序
                rx_index = 0;
            } else {
                rx_buffer[rx_index++] = ch;
            }
        } else {
            rx_index = 0;  // 溢出处理
        }
        
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
  }
//}

// 在Serial.h中添加JR6001的USART初始化函数
//void JR6001_USART_Init(void) {
//    GPIO_InitTypeDef GPIO_InitStructure;
//    USART_InitTypeDef USART_InitStructure;

//    // 使能时钟
//    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

//    // 配置USART1_TX (PA9) 为复用推挽输出
//    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(GPIOA, &GPIO_InitStructure);

//    // 配置USART1_RX (PA10) 为浮空输入
//    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
//    GPIO_Init(GPIOA, &GPIO_InitStructure);

//    // USART参数配置
//    USART_InitStructure.USART_BaudRate = 9600;  // JR6001默认波特率
//    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
//    USART_InitStructure.USART_StopBits = USART_StopBits_1;
//    USART_InitStructure.USART_Parity = USART_Parity_No;
//   USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
//    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
//    USART_Init(USART1, &USART_InitStructure);

//    USART_Cmd(USART1, ENABLE);
//}

// 发送指定语音段（假设“欢迎光临”预存在第1段）
void JR6001_PlayWelcome(void) {
    // JR6001播放指令格式：0xAA 0x07 0x01 0xBB
    // 0x07: 播放命令，0x01: 语音段号
    uint8_t cmd[] = {0xAA, 0x07, 0x02, 0xBB};
    
    for (int i = 0; i < sizeof(cmd); i++) {
        USART_SendData(USART1, cmd[i]);
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    }
}                                                                                                                                                                                                              

 //JR6001使用USART2初始化
//void JR6001_USART_Init(void) {
//    GPIO_InitTypeDef GPIO_InitStruct;
//    USART_InitTypeDef USART_InitStruct;
//	NVIC_InitTypeDef NVIC_InitStruct;
// 
//    // 使能时钟
//    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
//    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
// 
//    // 配置PA2为USART2_TX（复用推挽输出）
//    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2;
//    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
//    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(GPIOA, &GPIO_InitStruct);
// 
//    // 配置PA3为USART2_RX（浮空输入）
//    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3;
//    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
//    GPIO_Init(GPIOA, &GPIO_InitStruct);
// 
//    // USART2参数配置
//    USART_InitStruct.USART_BaudRate = 9600;
//    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
//    USART_InitStruct.USART_StopBits = USART_StopBits_1;
//    USART_InitStruct.USART_Parity = USART_Parity_No;
//    USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
//    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
//    USART_Init(USART2, &USART_InitStruct);
//	
//	// 启用USART2接收中断（关键！）
//    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
// 
//    // 配置USART2中断优先级
//    NVIC_InitStruct.NVIC_IRQChannel = USART2_IRQn;
//    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2;
//    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
//    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
//    NVIC_Init(&NVIC_InitStruct);
//	
//    USART_Cmd(USART2, ENABLE);
//}
// 
//// 修改JR6001发送函数
//void JR6001_PlayWelcome(void) {
//    uint8_t cmd[] = {0xAA, 0x07, 0x02, 0xBB};
//    for (int i = 0; i < sizeof(cmd); i++) {
//        USART_SendData(USART2, cmd[i]);  // 使用USART2发送
//        while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
//    }
//}

// 在Serial.c中实现
//void Serial_SendString_USART2(const char *String) {
//    USART_ITConfig(USART2, USART_IT_RXNE, DISABLE); // 发送前禁用接收中断
//    
//    uint8_t i;
//    for (i = 0; String[i] != '\0'; i++) {
//        USART_SendData(USART2, String[i]);         // 使用USART2发送
//        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
//    }
//    
//    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE); // 重新启用接收中断
//}

//// 播放第2首曲目（根据文档7.3节）
//void JR6001_PlayTrack2(void) {
//    // 正确指令格式：A7:00002\n
//    const char *cmd = "A7:00002\n"; 
//    Serial_SendString_USART2(cmd);
//}





