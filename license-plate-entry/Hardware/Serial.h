#ifndef __SERIAL_H
#define __SERIAL_H

#include <stdio.h>

extern char Serial_RxPacket[];
extern uint8_t Serial_RxFlag;

#define RX_BUFFER_SIZE 128
//作用：定义串口接收缓冲区的最大长度
//典型值选择：根据最长车牌号长度设定（中文车牌通常8字符，含汉字需要更大缓冲区）

extern volatile uint16_t rx_index ;
//volatile：防止编译器优化，确保中断与主程序都能看到最新值
//用途：记录当前缓冲区写入位置，范围0~127

extern volatile uint8_t rx_flag ;

extern char rx_buffer[RX_BUFFER_SIZE];
//作用：声明接收缓冲区数组
//内存分配：占用128字节的RAM空间，用于临时存储接收的原始数据

void Serial_Init(void);
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array, uint16_t Length);
void Serial_SendString(const char *String);
void Serial_SendNumber(uint32_t Number, uint8_t Length);
void Serial_Printf(char *format, ...);
void JR6001_USART_Init(void);
void JR6001_PlayWelcome(void);
void Serial_SendString_USART2(const char *String);
void JR6001_PlayTrack2(void);




#endif
