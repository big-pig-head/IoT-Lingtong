#include "stm32f10x.h"
#include "led.h"
#include "delay.h"
#include "oled.h"
#include "rain.h"
#include "water.h"
#include "bump.h"
#include <stdio.h>  // 包含标准输入输出函数声明
#include "Buzzer.h"


// 传感器阈值设置 (根据广东雨季特点调整)
#define RAIN_THRESHOLD      800    // 雨量阈值(低于此值表示大雨)
#define WATER_THRESHOLD     1500   // 水位阈值(高于此值表示危险水位)
#define SAFE_WATER_LEVEL    500    // 安全水位线(降至此值关闭水泵)

// 系统状态变量
uint16_t rain_value = 0;      // 雨量值
uint16_t water_value = 0;    // 水位值
uint8_t pump_state = 0;      // 水泵状态(0:关闭, 1:开启)

// OLED显示缓存
u8 display_buf[20];  // 修改为u8类型解决警告

int main(void)
{ 
    // 初始化所有外设
    SystemInit();                   // 配置系统时钟为72M
    delay_init(72);
    LED_Init();
    BUMP_Init();                    // 水泵初始化
    RAIN_Init();                    // 雨滴传感器初始化
    WATER_Init();                   // 水位传感器初始化
    OLED_Init();                    // OLED初始化
    Buzzer_Init();                  //蜂鸣器初始化
		
	
    delay_ms(1000);
    
    OLED_Clear();
    
    // 显示固定标题
    OLED_ShowChinese(0, 0, 6, 16, 1);  // 雨
    OLED_ShowChinese(16, 0, 7, 16, 1); // 量
    OLED_ShowChar(32, 0, ':', 16, 1);
    
    OLED_ShowChinese(0, 16, 8, 16, 1);  // 水
    OLED_ShowChinese(16, 16, 9, 16, 1); // 位
    OLED_ShowChar(32, 16, ':', 16, 1);
    
    OLED_ShowChinese(0, 32, 0, 16, 1);  // 水
    OLED_ShowChinese(16, 32, 1, 16, 1); // 泵
    OLED_ShowChinese(32, 32, 2, 16, 1); // 状
    OLED_ShowChinese(48, 32, 3, 16, 1); // 态
    OLED_ShowChar(64, 32, ':', 16, 1);

    while (1)
    {
        // 1. 读取传感器数据
        rain_value = RAIN_GetData();
        water_value = WATER_GetData();
        
        // 2. 自动排水控制逻辑 (广东雨季专用)
        if (!pump_state) {
            // 当雨量大且水位达到警戒线时启动水泵
            if (rain_value > RAIN_THRESHOLD && water_value > WATER_THRESHOLD) {
                //BUMP_ON; 
				BUMP_OFF;  //这两个ON/OFF好像搞反了，好像off才能使水泵开，on才能使水泵关
                pump_state = 1;
                //LED_On();
				Buzzer_ON();
            }
        } else {
            // 水位降至安全线以下才关闭水泵
            if (water_value < SAFE_WATER_LEVEL) {
                //BUMP_OFF;
				BUMP_ON;
                pump_state = 0;
                //LED_Off();
				Buzzer_OFF();
            }
        }
        
        // 3. OLED显示更新
        // 雨量值显示
        sprintf((char*)display_buf, "%4d", rain_value);
        OLED_ShowString(48, 0, display_buf, 16, 1);
        
        // 水位值显示
        sprintf((char*)display_buf, "%4d", water_value);
        OLED_ShowString(48, 16, display_buf, 16, 1);
        
        // 水泵状态显示
        if (pump_state) {
            OLED_ShowChinese(80, 32, 10, 16, 1);  // 开
        } else {
            OLED_ShowChinese(80, 32, 11, 16, 1);  // 关
        }
        
        // 4. LED心跳指示
        //LED_Toggle();
        delay_ms(500);
    }
}

