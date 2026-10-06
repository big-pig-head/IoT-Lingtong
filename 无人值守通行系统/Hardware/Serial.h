#ifndef __SERIAL_H
#define __SERIAL_H

#include <stdio.h>

/* USART1 接收方案：UART + DMA(循环) + IDLE 空闲线中断
 * 自定义帧格式：[AA][55][CMD][LEN][DATA][累加和][0D][0A]
 * 双缓冲职责分离：dma_rx_buf(DMA独占写) + rx_parse_buf(CPU解析专用)，解析结果存 rx_buffer */

#define RX_BUFFER_SIZE 128     /* DMA 循环接收缓冲区大小（容纳最长帧） */
#define FRAME_MAX_LEN  96      /* 单帧最大字节数（含帧头帧尾） */
#define PLATE_MAX_LEN  16      /* 车牌字符串最大长度（本地去重缓存用） */

extern uint8_t  dma_rx_buf[RX_BUFFER_SIZE];   /* DMA 独占写入缓冲区（第一块缓冲，CPU 不在其中解析） */
extern uint8_t  rx_parse_buf[FRAME_MAX_LEN];  /* CPU 解析专用缓冲区（第二块缓冲，中断拷入整帧，主循环解析） */
extern volatile uint16_t frame_valid_len;     /* 本次收到帧的有效字节数（中断里算出） */
extern volatile uint8_t  frame_ready;         /* 帧就绪标志：1=收到完整帧待解析，主循环处理完后清 0 */

extern char rx_buffer[RX_BUFFER_SIZE];        /* 解析后的有效数据（车牌字符串），OLED 显示源 */
extern volatile uint8_t rx_flag;              /* 有合法数据帧待处理 */
extern uint8_t valid_plate;                   /* 车牌有效标志（1=有效，0=无效），定义见 main.c */
extern char last_show_plate[PLATE_MAX_LEN];   /* 上次成功显示的车牌，本地去重缓存（配合 PC 端节流） */

void Serial_Init(void);
void Serial_ProcessFrame(void);               /* 主循环中调用：帧校验与解析 */
void ProcessLicensePlate(void);               /* 处理解析后的车牌（有效/无效判断 + 本地去重 + 刷新显示） */
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array, uint16_t Length);
void Serial_SendString(const char *String);
void Serial_SendNumber(uint32_t Number, uint8_t Length);
void Serial_Printf(char *format, ...);

#endif
