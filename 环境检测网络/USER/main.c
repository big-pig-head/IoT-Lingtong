#include "stm32f10x.h"
#include "led.h"
#include "usart.h"
#include "delay.h"
#include "oled.h"
#include "mq2.h"
#include "adcx.h"


u16 value;
u8 buff[30];//参数显示缓存数组
float ppm;

int main(void)
{ 
	
    SystemInit();//配置系统时钟为72M	
	delay_init(72);
	LED_Init();
	LED_On();
	MQ2_Init();
	USART1_Config();//串口初始化
	
	OLED_Init();
	printf("Start \n");
	delay_ms(1000);
	
	OLED_Clear();
	
	
	
	
	
	//显示“烟雾浓度:”
	OLED_ShowChinese(0,16,7,16,1);
	OLED_ShowChinese(16,16,1,16,1);
	//OLED_ShowChinese(32,0,2,16,1);
	//OLED_ShowChinese(48,0,3,16,1);
	//OLED_ShowChinese(32,0,3,16,1);

	OLED_ShowChar(32,16,':',16,1);
	
	

  while (1)
  {
		//LED_Toggle();
		value = MQ2_GetData();						//获取MQ2传感器的原始数据，支持两种工作模式。  
		
		printf("烟雾浓度: %d\r\n",value);
		//OLED_ShowNum(80,0,value,4,16,1);
		
		ppm = MQ2_GetData_PPM();					//将ADC值转换为气体浓度（ppm），仅适用于模拟模式（MODE=1）。
		sprintf((char*)buff, "%.2fppm    ",ppm);
		//OLED_ShowString(48,16,buff,16,1);
		OLED_ShowString(48,16,buff,16,1);
	  
	  
	  
	  
		
//		if(value)
//		{
//			OLED_ShowChinese(48,32,4,16,1);	//异
//			OLED_ShowChinese(64,32,6,16,1);	//常
//		}
//		else
//		{
//			OLED_ShowChinese(48,32,5,16,1);	//正
//			OLED_ShowChinese(64,32,6,16,1);	//常
//		}

		delay_ms(200);

  }
	
}


