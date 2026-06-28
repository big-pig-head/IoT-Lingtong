#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "string.h"
#include "Serial.h"
#include "Servo.h"
#include "LightSensor.h"
#include "HCSR04.h"
#include "stm32f10x_iwdg.h"      //独立看门狗（IWDG），防止程序死锁


 //道闸状态枚举
typedef enum {
    GATE_CLOSED,    // 道闸关闭
    //GATE_OPENING,   // 道闸正在打开
    GATE_OPENED     // 道闸已打开
} GateState;

 //全局变量
volatile uint32_t SystemTick = 0;  // 系统时间戳（1ms单位）
int Int_Spaces = 5;               // 剩余车位数量
uint32_t distance = 0;             // 超声波测量的距离（毫米）
GateState gate_state = GATE_CLOSED;// 当前道闸状态
uint8_t valid_plate = 0;           // 有效车牌标志（1=有效，0=无效）
//static uint32_t gate_timer = 0;    // 道闸状态切换计时器
uint32_t Use_Time=0;			//计算时间
//uint32_t distance=0;				//超声波传感器和障碍物的距离

// 获取系统时间（单位：ms）
uint32_t GetTick(void) {
    return SystemTick;
}

//void SysTick_Handler(void) {
//    SystemTick++;
//}

// ///系统时钟初始化（配置SysTick）
//void SystemClock_Init(void) {
//    SysTick_Config(SystemCoreClock / 1000); // 1ms中断一次
//}

// 独立看门狗初始化
void IWDG_Init(void) {
    // 启用LSI时钟（内部低速时钟，看门狗专用）
    RCC_LSICmd(ENABLE);
    while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET); // 等待LSI就绪
 
    // 设置看门狗预分频器和重载值
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);   // 允许写配置
    IWDG_SetPrescaler(IWDG_Prescaler_256);          // 预分频系数256
    IWDG_SetReload(0xFFF);                          // 重载值（超时时间 ≈ 1s）
    
    // 启动看门狗
    IWDG_ReloadCounter();  // 先刷新一次
    IWDG_Enable();         // 启动看门狗
}

// 刷新看门狗（需在主循环中定期调用）
void IWDG_Refresh(void) {
    IWDG_ReloadCounter();
}



//取5次为平均值
uint32_t HCSR04_GetDistance(void) {

	for(int i=0;i<5;++i){             					 //每次取10次测距数据,取平均值减少误差
            GPIO_SetBits(GPIOA,GPIO_Pin_6);
            Delay_us(15);                 				  	//根据说明书,需要提供至少10us的高电平
            GPIO_ResetBits(GPIOA,GPIO_Pin_6);
            Delay_ms(65); 									//根据说明书,每个周期至少需要等待60ms
			Use_Time=GetTimes();
            distance+=(Use_Time/5.8);          				//根据说明书提供的公式,获取单位为mm的距离
        }
        distance/=5;
		return distance;
}


 //更新显示函数
void UpdateDisplay(void) {
    //OLED_ShowString(0, 0, "Welcome!");          // 首行显示欢迎语
    //OLED_ShowString(0, 32, "Spaces:");          // 第三行显示车位标签
    //OLED_ShowNum(72, 32, Int_Spaces, 3, OLED_8X16); // 显示剩余车位
    //OLED_Update();
	if(Int_Spaces > 0)
	{
	OLED_ShowString(0, 0, "欢迎光临!");
	OLED_ShowString(0, 32, "剩余车位:");
	OLED_ShowNum(72, 32, Int_Spaces, 3,OLED_8X16);	//OLED显示剩余车位
	OLED_Update();
	}
	else {
	
	OLED_ShowString(0, 0, "车位不足禁止通行");
	OLED_ShowString(0, 32, "剩余车位:");
	OLED_ShowNum(72, 32, Int_Spaces, 3,OLED_8X16);	//OLED显示剩余车位
	OLED_Update();
		
	}
	
    // 第二行显示车牌状态
    if(valid_plate) {
        //OLED_ShowString(0, 16, "Plate:");       // 显示车牌标签
        //OLED_ShowString(48, 16, rx_buffer);     // 显示车牌内容
		OLED_ShowString(0, 16, rx_buffer);  			// 显示车牌
		OLED_Update();
    } else {
        OLED_ShowString(0, 16, "No Plate    "); // 无车牌提示
		OLED_Update();
    }
    
    // 第四行显示距离
    OLED_ShowString(0, 48, "Dist:");            // 距离标签
    OLED_ShowNum(48, 48, distance, 4, OLED_8X16); // 显示距离值
    OLED_ShowString(88, 48, "mm");              // 单位标识
    OLED_Update();                              // 刷新屏幕
}

 //处理车牌数据（新增关键逻辑）
void ProcessLicensePlate(void) {
    if(rx_flag) { // 串口接收到数据
        // 检测是否为无效车牌
        if(strncmp(rx_buffer, "No Plate", 8) == 0) {
            valid_plate = 0;            // 标记无效
            memset(rx_buffer, 0, sizeof(rx_buffer)); // 清缓冲区
        } else {
            valid_plate = 1;            // 标记有效车牌
        }
        UpdateDisplay();                // 更新显示
        rx_flag = 0;                    // 清除接收标志
    }
}

// 改进的道闸状态机
void ProcessGate(void) {    
    switch(gate_state) {
        case GATE_CLOSED:
            // 有效车牌且距离<1米时抬杆
            if(valid_plate && (distance < 100) && Int_Spaces > 0) {
				
				
                Servo_SetAngle(90);      // 舵机90度（抬杆）
                //gate_state = GATE_OPENING;
				Delay_ms(2500);
                gate_state = GATE_OPENED;
                //gate_timer = GetTick(); // 记录抬杆开始时间
            }
//		 if(valid_plate  && Int_Spaces > 0) {
//				
//				
//                Servo_SetAngle(90);      // 舵机90度（抬杆）
//                //gate_state = GATE_OPENING;
//				Delay_ms(2500);
//                gate_state = GATE_OPENED;
//                //gate_timer = GetTick(); // 记录抬杆开始时间
//            }
            break;
            
//        case GATE_OPENING:
//            // 抬杆动作持续500ms后进入开启状态
//            if(GetTick() - gate_timer > 500) {
//                gate_state = GATE_OPENED;
//                gate_timer = GetTick(); // 记录完全开启时间
//				
//				//实验
//				Int_Spaces++;
//				
//            }
//				if(distance < 1000) {
//				
//				Delay_ms(500);
//                gate_state = GATE_OPENED;
//                //gate_timer = GetTick(); // 记录完全开启时间
//            }
//            break;
            
        case GATE_OPENED:
            // 距离>1米且持续2秒后落杆
            if((distance > 100)  ) { //&& (GetTick() - gate_timer > 2000)
                Servo_SetAngle(0);       // 舵机0度（落杆）
                gate_state = GATE_CLOSED;
                if(Int_Spaces > 0) Int_Spaces--; // 减少车位
                valid_plate = 0;         // 重置车牌状态
                UpdateDisplay();          // 更新显示
            }
            break;
    }
}

int main(void) {
	
    // 硬件初始化
    //SystemClock_Init();  // 系统时钟
	IWDG_Init();          // 新增看门狗初始化
    OLED_Init();         // OLED
    Serial_Init();       // 串口
    Servo_Init();        // 舵机
    LightSensor_Init();  // 光敏传感器（代码中未使用）
    HCSR04_Init();       // 超声波
	//JR6001_USART_Init(); // JR6001语音模块初始化
    
    // 初始显示
    OLED_Clear();
    UpdateDisplay();
	//JR6001_PlayWelcome();              // 播报“欢迎光临,请小心行驶”
	Servo_SetAngle(0);
	//JR6001_PlayTrack2();  // 发送"A7:00002\n"
	
    while(1) {
		IWDG_Refresh();   // 必须定期刷新看门狗
		
        // 每200ms更新一次距离
       // static uint32_t dist_timer = 0;
       // if(GetTick() - dist_timer > 200) {
            distance = HCSR04_GetDistance();
        //    dist_timer = GetTick();
       // }
        
		UpdateDisplay();
        // 处理车牌和道闸逻辑
        ProcessLicensePlate();						//处理车牌数据（新增关键逻辑）
        ProcessGate();								// 改进的道闸状态机
        UpdateDisplay();
        // 安全检测：10秒强制落杆
//        if(gate_state == GATE_OPENED && (GetTick() - gate_timer) > 10000) {
//            Servo_SetAngle(0);
//            gate_state = GATE_CLOSED;
//            UpdateDisplay();
//        }
		
		
		 // 处理车牌
//        if (rx_flag) {
//            ProcessLicensePlate();
//            rx_flag = 0;
//        }
//        
//        // 触发JR6001播放
//        if (need_play_voice) {
//            uint8_t cmd[] = {0xAA, 0x07, 0x02, 0xBB};
//            JR6001_SendCmd(cmd, sizeof(cmd));
//            need_play_voice = 0;
//        }
//		
		
		
		
		
    }
}


































//#include "stm32f10x.h"                  // Device header
//#include "Delay.h"
//#include "OLED.h"
//#include "string.h"
//#include "Serial.h"						//串口
//#include "Servo.h"						//舵机
//#include "LightSensor.h"				//光敏传感器
//#include "HCSR04.h"						//超声波传感器

//float Angle=0;					//定义角度变量
//int Int_Spaces=10;				//定义剩余停车位
//int Int_PanDuanSpaces_One=0;    //用来辅助剩余停车位的计数
//int Int_PanDuanSpaces_Two=0;	//用来辅助剩余停车位的计数
//int Int_WhileOne=0;

//uint32_t Use_Time=0;			//计算时间
//uint32_t distance=0;				//超声波传感器和障碍物的距离



//int main(void)
//{
//	/*OLED初始化*/
//	OLED_Init();
//	Serial_Init();		//串口初始化
//	Servo_Init();		//舵机初始化
//	LightSensor_Init();//光敏传感器
//	JR6001_USART_Init(); // JR6001语音模块初始化
//	HCSR04_Init();			//超声波传感器，//初始化GPIO口,Trig使用推挽输出,Echo使用浮空输入
//	
//	// 初始显示
//	//OLED_ShowString(0, 1, "Spaces:");				//(列，行)
//	OLED_ShowString(0, 0, "欢迎光临!");
//	OLED_ShowString(0, 32, "剩余车位:");
//	OLED_ShowNum(72, 32, Int_Spaces, 3,OLED_8X16);	//OLED显示角度变量
//	OLED_Update();
//	
//	JR6001_PlayWelcome();              // 播报“欢迎光临”
//	
//	
//	while (1)
//	{
//		
//		//超声波控制舵机，显示距离
//		for(int i=0;i<10;++i){             					 //每次取10次测距数据,取平均值减少误差
//            GPIO_SetBits(GPIOA,GPIO_Pin_6);
//            Delay_us(15);                 				  	//根据说明书,需要提供至少10us的高电平
//            GPIO_ResetBits(GPIOA,GPIO_Pin_6);
//            Delay_ms(65); 									//根据说明书,每个周期至少需要等待60ms
//			Use_Time=GetTimes();
//            distance+=(Use_Time/5.8);          				//根据说明书提供的公式,获取单位为mm的距离
//        }
//        distance/=10;										//平均数，单位是mm
//		//distance/=1000;										//单位变换，从mm变到m
//        OLED_ShowNum(0,48,distance,4,OLED_8X16);	
//		//OLED_ShowChar(8,48,'m',OLED_8X16);
//		//OLED_ShowChar(16, 0, 'A', OLED_8X16);	
//		OLED_Update();
//		
//		
//		
//		//控制杆子的逻辑，以光敏电阻控制舵机的代码进行修改

////		if(LightSensor_Get() == 1)
////		{
////			Int_WhileOne=1;
////		}
//		//Servo_SetAngle(Angle);	//设置舵机的角度为角度变量
//		
////		if (LightSensor_Get() == 1)				//当有东西遮挡光敏传感器的时候触发
////		{
////			
////			Angle = 90;
////			
////			
////			Int_PanDuanSpaces_One=1;
////			Servo_SetAngle(Angle);	//设置舵机的角度为角度变量
////			
////			if(LightSensor_Get() == 0)
////			{
////			Int_WhileOne=0;
////			Angle = 0;
////			Servo_SetAngle(Angle);	//设置舵机的角度为角度变量
////			Int_PanDuanSpaces_Two=1;
////			//break;
////			}
////			
////		}

//		//Servo_SetAngle(Angle);			//设置舵机的角度为角度变量
//		//Delay_ms(500);					//延时500ms，看成车辆通过闸口的时间，一过，车位加一
//		//if (LightSensor_Get() == 0)				//按键1按下
//		//{
//			
//		//	Angle = 0;
//			
//		//	Int_PanDuanSpaces_Two=1;
//		//}
//		
//		//Delay_ms(500);	
//		
////		if(Int_PanDuanSpaces_One ==1 && Int_PanDuanSpaces_Two ==1)
////		{
////			if(Int_Spaces !=0)
////			{Int_Spaces--;}
////			
////			OLED_ShowNum(72, 32, Int_Spaces, 3,OLED_8X16);
////            OLED_Update();			//OLED显示角度变量
////			Int_PanDuanSpaces_One =0;
////			Int_PanDuanSpaces_Two =0;
////		}
//		
//	
//		//超声波控制舵机，显示距离
////		for(int i=0;i<10;++i){             					 //每次取10次测距数据,取平均值减少误差
////            GPIO_SetBits(GPIOA,GPIO_Pin_6);
////            Delay_us(15);                 				  	//根据说明书,需要提供至少10us的高电平
////            GPIO_ResetBits(GPIOA,GPIO_Pin_6);
////            Delay_ms(65); 									//根据说明书,每个周期至少需要等待60ms
////			Use_Time=GetTimes();
////            distance+=(Use_Time/5.8);          				//根据说明书提供的公式,获取单位为mm的距离
////        }
////        distance/=10;										//平均数
////		//distance/=1000;										//单位变换，从mm变到m
////        OLED_ShowNum(0,48,distance,4,OLED_8X16);	
////		//OLED_ShowChar(8,48,'m',OLED_8X16);
////		//OLED_ShowChar(16, 0, 'A', OLED_8X16);	
////		OLED_Update();
////		
//		
//		
//	 if (rx_flag)
//	 {
//        //OLED_Clear();
//		OLED_ShowString(0, 0, "欢迎光临!");
//		OLED_ShowString(0, 32, "剩余车位:");
//		OLED_ShowString(0, 16, "        "); 			// 清空车牌行
//		OLED_ShowNum(72, 32, Int_Spaces, 3,OLED_8X16);	//OLED显示剩余车位
//        OLED_ShowString(0, 16, rx_buffer);  			// 显示车牌
//		OLED_Update();									//*调用OLED_Update函数，将OLED显存数组的内容更新到OLED硬件进行显示*/
//		 
//		JR6001_PlayWelcome();              // 播报“欢迎光临”
//        rx_flag = 0;
//		 
////		 while (distance<=1000)					//当超声波传感器识别出来的距离小于1000mm（即1m）的时候触发
////												//相当于车辆未完全通过杆子，需要开杆子
////		{
////			
////			Angle = 90;
////			Servo_SetAngle(Angle);	//设置舵机的角度为角度变量
////			
////			Int_PanDuanSpaces_One=1;
//////			if(LightSensor_Get() == 0)
//////			{
//////			//Int_WhileOne=0;
//////			Angle = 0;
//////			Servo_SetAngle(Angle);	//设置舵机的角度为角度变量
//////			Int_PanDuanSpaces_Two=1;
//////			//break;
//////			}
////			
////		}
////		 
////		 if(distance>1000)
////			{
////			
////			Angle = 0;
////			Servo_SetAngle(Angle);	//设置舵机的角度为角度变量
////			Int_PanDuanSpaces_Two=1;
////			
////			}
////		 
////		 /*调用OLED_Update函数，将OLED显存数组的内容更新到OLED硬件进行显示*/
////		//OLED_Update();
////    }

////	//更新剩余车位的信息
////		if(Int_PanDuanSpaces_One ==1 && Int_PanDuanSpaces_Two ==1)
////		{
////			if(Int_Spaces !=0)
////			{Int_Spaces--;}
////			
////			OLED_ShowNum(72, 32, Int_Spaces, 3,OLED_8X16);			//OLED显示剩余车位数量
////            OLED_Update();			
////			Int_PanDuanSpaces_One =0;
////			Int_PanDuanSpaces_Two =0;
////		}
//	
//	
//	}
//}
//}






