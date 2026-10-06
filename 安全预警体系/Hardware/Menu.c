#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "Key.h"
#include "MyRTC.h"
#include "Menu.h"

uint8_t KeyNum; //用于储存键码值

uint8_t Ani_Dir_Flag = 1; //动画移动方向标志位(默认下移)

uint8_t setting_flag = 1; //时间设置标志位(1->正常读取、显示时间,2->暂停读取时间,进行设置)

int16_t Length,Angle,Speed,Direction; //风扇控制的步长、速度、角度、方向

uint8_t flag = 1; //所在行数
/***********************************************************************************************
	把一级菜单的变量flag放到全局变量,静态储存,不会丢失数据,所以从二级菜单回来以后,
	仍可以返回到一级菜单原来的那一行,而不是每次都回到一级菜单的第一行。
************************************************************************************************/

/* 一级菜单 */
int menu1(void) 
{
	while(1)
	{
		KeyNum = Key_GetNum();
		if(KeyNum == 2) //上一项
		{
			flag --;
			if(flag == 0) flag = 8;
			Ani_Dir_Flag = 2; //动画从下一项往上一项移动(上移)
			OLED_AniUpdate(); //按一下按键,刷新一次动画效果
		}
		
		if(KeyNum == 1) //下一项
		{
			flag ++;
			if(flag == 9) flag = 1;
			Ani_Dir_Flag = 1; //动画从上一项往下一项移动(下移)
			OLED_AniUpdate(); //按一下按键,刷新一次动画效果
		}
		
		if(KeyNum == 3) //确认
		{
			OLED_Clear();
			OLED_AniUpdate(); //按一下按键,刷新一次动画效果
			return flag; //跳出循环,返回主函数,把一级菜单的值返回给二级菜单变量menu2
		}
		
		switch(flag)
		{
			case 1:
			{
				//再次显示，确保反相前是亮着的状态(第一页内容)
				OLED_ShowString(0, 0, "风扇控制         ", OLED_8X16);
				OLED_ShowString(0, 16, "AD              ", OLED_8X16);
				OLED_ShowString(0, 32, "摇杆数据        ", OLED_8X16);
				OLED_ShowString(0, 48, "MPU6050         ", OLED_8X16);
				
				/* 刚开始下移时,是从起始位置(0,0,0,0)->第一行的目标位置 */
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 0, 0, 0, 0, 0, 64, 16);   //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 16, 16, 16, 0, 0, 64, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 2:
			{
				OLED_ShowString(0, 0, "风扇控制         ", OLED_8X16);
				OLED_ShowString(0, 16, "AD              ", OLED_8X16);
				OLED_ShowString(0, 32, "摇杆数据        ", OLED_8X16);
				OLED_ShowString(0, 48, "MPU6050         ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 0, 64, 16, 0, 16, 16, 16);  //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 32, 64, 16, 0, 16, 16, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 3:
			{
				OLED_ShowString(0, 0, "风扇控制         ", OLED_8X16);
				OLED_ShowString(0, 16, "AD              ", OLED_8X16);
				OLED_ShowString(0, 32, "摇杆数据        ", OLED_8X16);
				OLED_ShowString(0, 48, "MPU6050         ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 16, 16, 16, 0, 32, 64, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 48, 56, 16, 0, 32, 64, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 4:
			{
				OLED_ShowString(0, 0, "风扇控制         ", OLED_8X16);
				OLED_ShowString(0, 16, "AD              ", OLED_8X16);
				OLED_ShowString(0, 32, "摇杆数据        ", OLED_8X16);
				OLED_ShowString(0, 48, "MPU6050         ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 32, 64, 16, 0, 48, 56, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 0, 24, 16, 0, 48, 56, 16);  //动画从下一项往上一项移动(上移)
			}
			break;
			case 5:
			{
				//再次显示，确保反相前是亮着的状态(第二页内容)
				OLED_ShowString(0, 0, "PID              ", OLED_8X16);
				OLED_ShowString(0, 16, "时钟            ", OLED_8X16);
				OLED_ShowString(0, 32, "音乐            ", OLED_8X16);
				OLED_ShowString(0, 48, "设置            ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 48, 56, 16, 0, 0, 24, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 16, 32, 16, 0, 0, 24, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 6:
			{
				OLED_ShowString(0, 0, "PID              ", OLED_8X16);
				OLED_ShowString(0, 16, "时钟            ", OLED_8X16);
				OLED_ShowString(0, 32, "音乐            ", OLED_8X16);
				OLED_ShowString(0, 48, "设置            ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 0, 24, 16, 0, 16, 32, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 32, 32, 16, 0, 16, 32, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 7:
			{
				OLED_ShowString(0, 0, "PID              ", OLED_8X16);
				OLED_ShowString(0, 16, "时钟            ", OLED_8X16);
				OLED_ShowString(0, 32, "音乐            ", OLED_8X16);
				OLED_ShowString(0, 48, "设置            ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 16, 32, 16, 0, 32, 32, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 48, 32, 16, 0, 32, 32, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 8:
			{
				OLED_ShowString(0, 0, "PID              ", OLED_8X16);
				OLED_ShowString(0, 16, "时钟            ", OLED_8X16);
				OLED_ShowString(0, 32, "音乐            ", OLED_8X16);
				OLED_ShowString(0, 48, "设置            ", OLED_8X16);
				/*
					最后一行上移时,(0,64,64,16)->最后一行的目标位置
					(0,64,64,16)坐标前两个数代表最后一行下边的位置,0列64行
					后两个数代表第一行的宽度和高度(风扇控制：宽度64,高度16)
				*/
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 32, 32, 16, 0, 48, 32, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 64, 64, 16, 0, 48, 32, 16); //动画从下一项往上一项移动(上移)
			}
			break;
		}
	}
}

/* 二级菜单---风扇控制 */
int menu2_fan(void)
{
	uint8_t flag = 1;
	uint8_t menu3_flag;
	
	while(1)
	{
		KeyNum = Key_GetNum();
		if(KeyNum == 2) //上一项
		{
			flag --;
			if(flag == 0) flag = 5;
			Ani_Dir_Flag = 2;
			OLED_AniUpdate();
		}
		
		if(KeyNum == 1) //下一项
		{
			flag ++;
			if(flag == 6) flag = 1;
			Ani_Dir_Flag = 1;
			OLED_AniUpdate();
		}
		
		if(KeyNum == 3) //确认
		{
			OLED_Clear();
			OLED_AniUpdate();
			menu3_flag = flag; //把二级菜单的值传给三级菜单变量
		}
		
		if(menu3_flag == 1) return 0; //退出二级菜单的while循环,返回一级菜单
		
		/*
								《三级菜单控制》
			进入风扇控制的单次步长三级菜单,可以对三级菜单进行设置
			退出时,三级菜单返回0(return 0),menu3_flag = 0,这时候二级菜单的while循环还没有退出,
			继续显示相对应的二级菜单,就实现了三级菜单返回二级菜单的操作
		*/
		if(menu3_flag == 2) menu3_flag = menu3_SetLength();
		if(menu3_flag == 3) menu3_flag = menu3_SetAngle();
		if(menu3_flag == 4) menu3_flag = menu3_SetSpeed();
		if(menu3_flag == 5) menu3_flag = menu3_SetDirection();
		
		switch(flag)
		{
			case 1:
			{
				OLED_ShowString(0, 0, "<-                   ", OLED_8X16);
				OLED_Printf(0, 16, OLED_8X16, "单次步长:%d", Length);
				OLED_Printf(0, 32, OLED_8X16, "角度控制:%d", Angle);
				OLED_Printf(0, 48, OLED_8X16, "速度控制:%d", Speed);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 0, 0, 0, 0, 0, 16, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 16, 64, 16, 0, 0, 16, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 2:
			{
				OLED_ShowString(0, 0, "<-                   ", OLED_8X16);
				OLED_Printf(0, 16, OLED_8X16, "单次步长:%d", Length);
				OLED_Printf(0, 32, OLED_8X16, "角度控制:%d", Angle);
				OLED_Printf(0, 48, OLED_8X16, "速度控制:%d", Speed);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 0, 16, 16, 0, 16, 64, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 32, 64, 16, 0, 16, 64, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 3:
			{
				OLED_ShowString(0, 0, "<-                   ", OLED_8X16);
				OLED_Printf(0, 16, OLED_8X16, "单次步长:%d", Length);
				OLED_Printf(0, 32, OLED_8X16, "角度控制:%d", Angle);
				OLED_Printf(0, 48, OLED_8X16, "速度控制:%d", Speed);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 16, 64, 16, 0, 32, 64, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 48, 64, 16, 0, 32, 64, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 4:
			{
				OLED_ShowString(0, 0, "<-                   ", OLED_8X16);
				OLED_Printf(0, 16, OLED_8X16, "单次步长:%d", Length);
				OLED_Printf(0, 32, OLED_8X16, "角度控制:%d", Angle);
				OLED_Printf(0, 48, OLED_8X16, "速度控制:%d", Speed);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 32, 64, 16, 0, 48, 64, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 64, 64, 16, 0, 48, 64, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 5:
			{
				//第二页
				OLED_Printf(0, 0, OLED_8X16, "旋转方向:%d", Direction);
				OLED_ShowString(0, 16, "                        ", OLED_8X16);
				OLED_ShowString(0, 32, "                        ", OLED_8X16);
				OLED_ShowString(0, 48, "                        ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 48, 64, 16, 0, 0, 64, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 16, 64, 16, 0, 0, 64, 16); //动画从下一项往上一项移动(上移)
			}
			break;
		}
	}
}

/* 三级菜单---风扇控制-单次步长 */
int menu3_SetLength(void)
{
	while(1)
	{
		KeyNum = Key_GetNum();
		if(KeyNum == 2) //步长减
		{
			Length --;
			if(Length < 0) Length = 30;
			OLED_AniUpdate();
		}
		
		if(KeyNum == 1) //步长加
		{
			Length ++;
			if(Length > 30) Length = 0;
			OLED_AniUpdate();
		}
		
		if(KeyNum == 3) //确认
		{
			OLED_AniUpdate(); //按一下按键,刷新一次动画效果
			return 0; //退出三级菜单，返回二级菜单
		}
		
		OLED_ShowString(0, 0, "<-                   ", OLED_8X16);
		OLED_Printf(0, 16, OLED_8X16, "单次步长:%d", Length);
		OLED_Printf(0, 32, OLED_8X16, "角度控制:%d", Angle);
		OLED_Printf(0, 48, OLED_8X16, "速度控制:%d", Speed);
		/*
										《Q弹效果》
			三级菜单为最后的菜单,不需要判断上移下移,只需要有一个动画效果即可
			因为二分法是一点一点的到目标位置的，前面的"单次步长"比"00"显示的宽度大(宽度64->16)，
			所以会先直接平移过去64,然后根据二分法一点一点变小,缩短到目标位置,
			这个是需要一点时间的,所以会显示出一种Q弹的效果。
		*/
		if(Length < 10) 
		{
			OLED_ClearArea(80, 16, 8, 16); //当显示一位数时,把第二位数给清除掉
			OLED_Animation(0, 16, 64, 16, 72, 16, 8, 16);
		}
		else OLED_Animation(0, 16, 64, 16, 72, 16, 16, 16);
		
	}
}

/* 三级菜单---风扇控制-角度控制 */
int menu3_SetAngle(void)
{
	while(1)
	{
		KeyNum = Key_GetNum();
		if(KeyNum == 2) //角度减
		{
			Angle = Angle - Length; //缩写(Angle -= Length)
			if(Angle < 0) Angle = 180;
			OLED_AniUpdate();
		}
		
		if(KeyNum == 1) //角度加
		{
			Angle = Angle + Length; //缩写(Angle += Length)
			if(Angle > 180) Angle = 0;
			OLED_AniUpdate();
		}
		
		if(KeyNum == 3) //确认
		{
			OLED_AniUpdate();
			return 0; //退出三级菜单，返回二级菜单
		}
		
		OLED_ShowString(0, 0, "<-                   ", OLED_8X16);
		OLED_Printf(0, 16, OLED_8X16, "单次步长:%d", Length);
		OLED_Printf(0, 32, OLED_8X16, "角度控制:%d", Angle);
		OLED_Printf(0, 48, OLED_8X16, "速度控制:%d", Speed);
		if(Angle < 10) 
		{
			OLED_ClearArea(80, 32, 16, 16); //当显示一位数时,把第二、三位数给清除掉
			OLED_Animation(0, 32, 64, 16, 72, 32, 8, 16);
		}
		else if(Angle >= 10 && Angle < 100) 
		{
			OLED_ClearArea(88, 32, 8, 16); //当显示两位数时,把第三位数给清除掉
			OLED_Animation(0, 32, 64, 16, 72, 32, 16, 16);
		}
		else OLED_Animation(0, 32, 64, 16, 72, 32, 24, 16);
	}
}

/* 三级菜单---风扇控制-速度控制 */
int menu3_SetSpeed(void)
{
	while(1)
	{
		KeyNum = Key_GetNum();
		if(KeyNum == 2) //速度减
		{
			Speed = Speed - Length;
			if(Speed < 0) Speed = 100;
			OLED_AniUpdate();
		}
		
		if(KeyNum == 1) //速度加
		{
			Speed = Speed + Length;
			if(Speed > 100) Speed = 0;
			OLED_AniUpdate();
		}
		
		if(KeyNum == 3) //确认
		{
			OLED_AniUpdate();
			return 0; //退出三级菜单，返回二级菜单
		}
		
		OLED_ShowString(0, 0, "<-                   ", OLED_8X16);
		OLED_Printf(0, 16, OLED_8X16, "单次步长:%d", Length);
		OLED_Printf(0, 32, OLED_8X16, "角度控制:%d", Angle);
		OLED_Printf(0, 48, OLED_8X16, "速度控制:%d", Speed);
		if(Speed < 10) 
		{
			OLED_ClearArea(80, 48, 16, 16); //当显示一位数时,把第二、三位数给清除掉
			OLED_Animation(0, 48, 64, 16, 72, 48, 8, 16);
		}
		else if(Speed >= 10 && Speed < 100) 
		{
			OLED_ClearArea(88, 48, 8, 16); //当显示两位数时,把第三位数给清除掉
			OLED_Animation(0, 48, 64, 16, 72, 48, 16, 16);
		}
		else OLED_Animation(0, 48, 64, 16, 72, 48, 24, 16);
	}
}

/* 三级菜单---风扇控制-方向控制 */
int menu3_SetDirection(void)
{
	while(1)
	{
		KeyNum = Key_GetNum();
		if(KeyNum == 2) //方向减
		{
			Direction = Direction - Length;
			if(Direction < 0) Direction = 180;
			OLED_AniUpdate();
		}
		
		if(KeyNum == 1) //方向加
		{
			Direction = Direction + Length;
			if(Direction > 180) Direction = 0;
			OLED_AniUpdate();
		}
		
		if(KeyNum == 3) //确认
		{
			OLED_AniUpdate();
			return 0; //退出三级菜单，返回二级菜单
		}
		
		//第二页
		OLED_Printf(0, 0, OLED_8X16, "旋转方向:%d", Direction);
		OLED_ShowString(0, 16, "                        ", OLED_8X16);
		OLED_ShowString(0, 32, "                        ", OLED_8X16);
		OLED_ShowString(0, 48, "                        ", OLED_8X16);
		if(Direction < 10) 
		{
			OLED_ClearArea(80, 0, 16, 16); //当显示一位数时,把第二、三位数给清除掉
			OLED_Animation(0, 0, 64, 16, 72, 0, 8, 16);
		}
		else if(Direction >= 10 && Direction < 100) 
		{
			OLED_ClearArea(88, 0, 8, 16); //当显示两位数时,把第三位数给清除掉
			OLED_Animation(0, 0, 64, 16, 72, 0, 16, 16);
		}
		else OLED_Animation(0, 0, 64, 16, 72, 0, 24, 16);
	}
}

/* 二级菜单---AD控制 */
int menu2_AD(void)
{
	uint8_t flag = 1;
	uint8_t menu3_flag;
	
	while(1)
	{
		KeyNum = Key_GetNum();
		if(KeyNum == 2) //上一项
		{
			flag --;
			if(flag == 0) flag = 4;
			Ani_Dir_Flag = 2;
			OLED_AniUpdate();
		}
		
		if(KeyNum == 1) //下一项
		{
			flag ++;
			if(flag == 5) flag = 1;
			Ani_Dir_Flag = 1;
			OLED_AniUpdate();
		}
		
		if(KeyNum == 3) //确认
		{
			OLED_Clear();
			OLED_AniUpdate();
			menu3_flag = flag; //把二级菜单的值传给三级菜单变量
		}
		
		if(menu3_flag == 1) return 0; //返回一级菜单
//		if(menu3_flag == 2) return 0;
//		if(menu3_flag == 3) return 0;
//		if(menu3_flag == 4) return 0;
		
		switch(flag)
		{
			case 1:
			{
				OLED_ShowString(0, 0, "<-                   ", OLED_8X16);
				OLED_ShowString(0, 16, "GPIO设置            ", OLED_8X16);
				OLED_ShowString(0, 32, "Pin设置             ", OLED_8X16);
				OLED_ShowString(0, 48, "数据                ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 0, 0, 0, 0, 0, 16, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 16, 64, 16, 0, 0, 16, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 2:
			{
				OLED_ShowString(0, 0, "<-                   ", OLED_8X16);
				OLED_ShowString(0, 16, "GPIO设置            ", OLED_8X16);
				OLED_ShowString(0, 32, "Pin设置             ", OLED_8X16);
				OLED_ShowString(0, 48, "数据                ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 0, 16, 16, 0, 16, 64, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 32, 56, 16, 0, 16, 64, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 3:
			{
				OLED_ShowString(0, 0, "<-                   ", OLED_8X16);
				OLED_ShowString(0, 16, "GPIO设置            ", OLED_8X16);
				OLED_ShowString(0, 32, "Pin设置             ", OLED_8X16);
				OLED_ShowString(0, 48, "数据                ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 16, 64, 16, 0, 32, 56, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 48, 32, 16, 0, 32, 56, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 4:
			{
				OLED_ShowString(0, 0, "<-                   ", OLED_8X16);
				OLED_ShowString(0, 16, "GPIO设置            ", OLED_8X16);
				OLED_ShowString(0, 32, "Pin设置             ", OLED_8X16);
				OLED_ShowString(0, 48, "数据                ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 32, 56, 16, 0, 48, 32, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 64, 16, 16, 0, 48, 32, 16); //动画从下一项往上一项移动(上移)
			}
			break;
		}
	}
}

/* 二级菜单---摇杆控制 */
int menu2_rocker(void)
{
	uint8_t flag = 1;
	uint8_t menu3_flag;
	
	while(1)
	{
		KeyNum = Key_GetNum();
		if(KeyNum == 2) //上一项
		{
			flag --;
			if(flag == 0) flag = 4;
			Ani_Dir_Flag = 2;
			OLED_AniUpdate();
		}
		
		if(KeyNum == 1) //下一项
		{
			flag ++;
			if(flag == 5) flag = 1;
			Ani_Dir_Flag = 1;
			OLED_AniUpdate();
		}
		
		if(KeyNum == 3) //确认
		{
			OLED_Clear();
			OLED_AniUpdate();
			menu3_flag = flag; //把二级菜单的值传给三级菜单变量
		}
		
		if(menu3_flag == 1) return 0; //返回一级菜单
//		if(menu3_flag == 2) return 0;
//		if(menu3_flag == 3) return 0;
//		if(menu3_flag == 4) return 0;
		
		switch(flag)
		{
			case 1:
			{
				OLED_ShowString(0, 0, "<-                  ", OLED_8X16);
				OLED_ShowString(0, 16, "VRX                ", OLED_8X16);
				OLED_ShowString(0, 32, "VRY                ", OLED_8X16);
				OLED_ShowString(0, 48, "SW                 ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 0, 0, 0, 0, 0, 16, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 16, 24, 16, 0, 0, 16, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 2:
			{
				OLED_ShowString(0, 0, "<-                  ", OLED_8X16);
				OLED_ShowString(0, 16, "VRX                ", OLED_8X16);
				OLED_ShowString(0, 32, "VRY                ", OLED_8X16);
				OLED_ShowString(0, 48, "SW                 ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 0, 16, 16, 0, 16, 24, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 32, 24, 16, 0, 16, 24, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 3:
			{
				OLED_ShowString(0, 0, "<-                  ", OLED_8X16);
				OLED_ShowString(0, 16, "VRX                ", OLED_8X16);
				OLED_ShowString(0, 32, "VRY                ", OLED_8X16);
				OLED_ShowString(0, 48, "SW                 ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 16, 24, 16, 0, 32, 24, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 48, 16, 16, 0, 32, 24, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 4:
			{
				OLED_ShowString(0, 0, "<-                  ", OLED_8X16);
				OLED_ShowString(0, 16, "VRX                ", OLED_8X16);
				OLED_ShowString(0, 32, "VRY                ", OLED_8X16);
				OLED_ShowString(0, 48, "SW                 ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 32, 24, 16, 0, 48, 16, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 64, 16, 16, 0, 48, 16, 16); //动画从下一项往上一项移动(上移)
			}
			break;
		}
	}
}

/* 二级菜单---mpu6050控制 */
int menu2_mpu6050(void)
{
	uint8_t flag = 1;
	uint8_t menu3_flag;
	
	while(1)
	{
		KeyNum = Key_GetNum();
		if(KeyNum == 2) //上一项
		{
			flag --;
			if(flag == 0) flag = 4;
			Ani_Dir_Flag = 2;
			OLED_AniUpdate();
		}
		
		if(KeyNum == 1) //下一项
		{
			flag ++;
			if(flag == 5) flag = 1;
			Ani_Dir_Flag = 1;
			OLED_AniUpdate();
		}
		
		if(KeyNum == 3) //确认
		{
			OLED_Clear();
			OLED_AniUpdate();
			menu3_flag = flag; //把二级菜单的值传给三级菜单变量
		}
		
		if(menu3_flag == 1) return 0; //返回一级菜单
//		if(menu3_flag == 2) return 0;
//		if(menu3_flag == 3) return 0;
//		if(menu3_flag == 4) return 0;
		
		switch(flag)
		{
			case 1:
			{
				OLED_ShowString(0, 0, "<-                  ", OLED_8X16);
				OLED_ShowString(0, 16, "俯仰角             ", OLED_8X16);
				OLED_ShowString(0, 32, "偏航角             ", OLED_8X16);
				OLED_ShowString(0, 48, "横滚角             ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 0, 0, 0, 0, 0, 16, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 16, 48, 16, 0, 0, 16, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 2:
			{
				OLED_ShowString(0, 0, "<-                  ", OLED_8X16);
				OLED_ShowString(0, 16, "俯仰角             ", OLED_8X16);
				OLED_ShowString(0, 32, "偏航角             ", OLED_8X16);
				OLED_ShowString(0, 48, "横滚角             ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 0, 16, 16, 0, 16, 48, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 32, 48, 16, 0, 16, 48, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 3:
			{
				OLED_ShowString(0, 0, "<-                  ", OLED_8X16);
				OLED_ShowString(0, 16, "俯仰角             ", OLED_8X16);
				OLED_ShowString(0, 32, "偏航角             ", OLED_8X16);
				OLED_ShowString(0, 48, "横滚角             ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 16, 48, 16, 0, 32, 48, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 48, 48, 16, 0, 32, 48, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 4:
			{
				OLED_ShowString(0, 0, "<-                  ", OLED_8X16);
				OLED_ShowString(0, 16, "俯仰角             ", OLED_8X16);
				OLED_ShowString(0, 32, "偏航角             ", OLED_8X16);
				OLED_ShowString(0, 48, "横滚角             ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 32, 48, 16, 0, 48, 48, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 64, 16, 16, 0, 48, 48, 16); //动画从下一项往上一项移动(上移)
			}
			break;
		}
	}
}

/* 二级菜单---PID控制 */
int menu2_PID(void)
{
	uint8_t flag = 1;
	uint8_t menu3_flag;
	
	while(1)
	{
		KeyNum = Key_GetNum();
		if(KeyNum == 2) //上一项
		{
			flag --;
			if(flag == 0) flag = 4;
			Ani_Dir_Flag = 2;
			OLED_AniUpdate();
		}
		
		if(KeyNum == 1) //下一项
		{
			flag ++;
			if(flag == 5) flag = 1;
			Ani_Dir_Flag = 1;
			OLED_AniUpdate();
		}
		
		if(KeyNum == 3) //确认
		{
			OLED_Clear();
			OLED_AniUpdate();
			menu3_flag = flag; //把二级菜单的值传给三级菜单变量
		}
		
		if(menu3_flag == 1) return 0; //返回一级菜单
//		if(menu3_flag == 2) return 0;
//		if(menu3_flag == 3) return 0;
//		if(menu3_flag == 4) return 0;
		
		switch(flag)
		{
			case 1:
			{
				OLED_ShowString(0, 0, "<-                  ", OLED_8X16);
				OLED_ShowString(0, 16, "直立环             ", OLED_8X16);
				OLED_ShowString(0, 32, "速度环             ", OLED_8X16);
				OLED_ShowString(0, 48, "转向环             ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 0, 0, 0, 0, 0, 16, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 16, 48, 16, 0, 0, 16, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 2:
			{
				OLED_ShowString(0, 0, "<-                  ", OLED_8X16);
				OLED_ShowString(0, 16, "直立环             ", OLED_8X16);
				OLED_ShowString(0, 32, "速度环             ", OLED_8X16);
				OLED_ShowString(0, 48, "转向环             ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 0, 0, 0, 0, 16, 48, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 32, 48, 16, 0, 16, 48, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 3:
			{
				OLED_ShowString(0, 0, "<-                  ", OLED_8X16);
				OLED_ShowString(0, 16, "直立环             ", OLED_8X16);
				OLED_ShowString(0, 32, "速度环             ", OLED_8X16);
				OLED_ShowString(0, 48, "转向环             ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 16, 48, 16, 0, 32, 48, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 48, 48, 16, 0, 32, 48, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 4:
			{
				OLED_ShowString(0, 0, "<-                  ", OLED_8X16);
				OLED_ShowString(0, 16, "直立环             ", OLED_8X16);
				OLED_ShowString(0, 32, "速度环             ", OLED_8X16);
				OLED_ShowString(0, 48, "转向环             ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 32, 48, 16, 0, 48, 48, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 64, 16, 16, 0, 48, 48, 16); //动画从下一项往上一项移动(上移)
			}
			break;
		}
	}
}

/* 二级菜单---时钟控制 */
int menu2_clock(void)
{
	uint8_t flag = 1;
	uint8_t menu3_flag;
	
	while(1)
	{
		if(setting_flag == 1) //正常读取、显示时间
		{
			MyRTC_ReadTime(); //RTC读取时间，最新的时间存储到MyRTC_Time数组中
			OLED_UpdateArea(0, 16, 128, 32); //对时间和日期显示部分进行刷新显示
		}
		
		KeyNum = Key_GetNum(); //获取键码值
		if(KeyNum == 2) //上一项
		{
			flag --;
			if(flag == 0) flag = 2;
			OLED_AniUpdate();
			Ani_Dir_Flag = 2;
		}
		
		if(KeyNum == 1) //下一项
		{
			flag ++;
			if(flag == 3) flag = 1;
			OLED_AniUpdate();
			Ani_Dir_Flag = 2;
		}
		
		if(KeyNum == 3) //确认
		{
			OLED_Clear();
			OLED_AniUpdate();
			menu3_flag = flag; //把二级菜单的值传给三级菜单变量
		}
		
		if(menu3_flag == 1) return 0; //返回一级菜单
//		if(menu3_flag == 2) menu3_flag = menu3_clock_setting();
		
		switch(flag)
		{
			case 1:
			{
				/*
					通过使用%02d格式化符号,可以确保时间显示为两位数格式(例如01),
					结合OLED_Printf函数,可以方便地在OLED屏幕上显示格式化的日期和时间
				*/
				OLED_ShowString(0, 0, "<-               ", OLED_8X16);
				OLED_Printf(0, 16, OLED_8X16, "Data：%04d-%02d-%02d", MyRTC_Time[0], MyRTC_Time[1], MyRTC_Time[2]);
				OLED_Printf(0, 32, OLED_8X16, "Time：%02d:%02d:%02d", MyRTC_Time[3], MyRTC_Time[4], MyRTC_Time[5]);
				OLED_ShowString(0, 48, "            设置", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 0, 0, 0, 0, 0, 16, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(96, 48, 32, 16, 0, 0, 16, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 2:
			{
				OLED_ShowString(0, 0, "<-               ", OLED_8X16);
				OLED_Printf(0, 16, OLED_8X16, "Data：%04d-%02d-%02d", MyRTC_Time[0], MyRTC_Time[1], MyRTC_Time[2]);
				OLED_Printf(0, 32, OLED_8X16, "Time：%02d:%02d:%02d", MyRTC_Time[3], MyRTC_Time[4], MyRTC_Time[5]);
				OLED_ShowString(0, 48, "            设置", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 0, 16, 16, 96, 48, 32, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 0, 16, 16, 96, 48, 32, 16); //动画从下一项往上一项移动(上移)
			}
			break;
		}
	}
}

/* 三级菜单---时钟控制-设置 */
//int menu3_clock_setting(void)
//{
//	while(1)
//	{
//		if()
//	}
//}

/* 二级菜单---音乐控制 */
int menu2_music(void)
{
	uint8_t flag = 1;
	uint8_t menu3_flag;
	
	while(1)
	{
		KeyNum = Key_GetNum();
		if(KeyNum == 2) //上一项
		{
			flag --;
			if(flag == 0) flag = 6;
			OLED_AniUpdate();
			Ani_Dir_Flag = 2;
		}
		
		if(KeyNum == 1) //下一项
		{
			flag ++;
			if(flag == 7) flag = 1;
			OLED_AniUpdate();
			Ani_Dir_Flag = 1;
		}
		
		if(KeyNum == 3) //确认
		{
			OLED_Clear();
			OLED_AniUpdate();
			menu3_flag = flag; //把二级菜单的值传给三级菜单变量
		}
		
		if(menu3_flag == 1) return 0; //返回一级菜单
//		if(menu3_flag == 2) return 0;
//		if(menu3_flag == 3) return 0;
//		if(menu3_flag == 4) return 0;
		
		switch(flag)
		{
			case 1:
			{
				OLED_ShowString(0, 0, "<-                  ", OLED_8X16);
				OLED_ShowString(0, 16, "老男孩             ", OLED_8X16);
				OLED_ShowString(0, 32, "稻香               ", OLED_8X16);
				OLED_ShowString(0, 48, "生如夏花           ", OLED_8X16);
				
				/* 刚开始下移时,是从起始位置(0,0,0,0)->第一行的目标位置 */
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 0, 0, 0, 0, 0, 16, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 16, 48, 16, 0, 0, 16, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 2:
			{
				OLED_ShowString(0, 0, "<-                  ", OLED_8X16);
				OLED_ShowString(0, 16, "老男孩             ", OLED_8X16);
				OLED_ShowString(0, 32, "稻香               ", OLED_8X16);
				OLED_ShowString(0, 48, "生如夏花           ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 0, 16, 16, 0, 16, 48, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 32, 32, 16, 0, 16, 48, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 3:
			{
				OLED_ShowString(0, 0, "<-                  ", OLED_8X16);
				OLED_ShowString(0, 16, "老男孩             ", OLED_8X16);
				OLED_ShowString(0, 32, "稻香               ", OLED_8X16);
				OLED_ShowString(0, 48, "生如夏花           ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 16, 48, 16, 0, 32, 32, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 48, 64, 16, 0, 32, 32, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 4:
			{
				OLED_ShowString(0, 0, "<-                  ", OLED_8X16);
				OLED_ShowString(0, 16, "老男孩             ", OLED_8X16);
				OLED_ShowString(0, 32, "稻香               ", OLED_8X16);
				OLED_ShowString(0, 48, "生如夏花           ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 32, 32, 16, 0, 48, 64, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 0, 64, 16, 0, 48, 64, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 5:
			{
				OLED_ShowString(0, 0, "这就是命            ", OLED_8X16);
				OLED_ShowString(0, 16, "异客               ", OLED_8X16);
				OLED_ShowString(0, 32, "                   ", OLED_8X16);
				OLED_ShowString(0, 48, "                   ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 48, 64, 16, 0, 0, 64, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 16, 32, 16, 0, 0, 64, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 6:
			{
				OLED_ShowString(0, 0, "这就是命            ", OLED_8X16);
				OLED_ShowString(0, 16, "异客               ", OLED_8X16);
				OLED_ShowString(0, 32, "                   ", OLED_8X16);
				OLED_ShowString(0, 48, "                   ", OLED_8X16);
				/*
					最后一行上移时,(0,32,16,16)->最后一行的目标位置
					(0,32,16,16)坐标前两个数代表最后一行下边的位置,0列32行
					后两个数代表第一行的宽度和高度(风扇控制：宽度16,高度16)
				*/
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 0, 64, 16, 0, 16, 32, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 32, 16, 16, 0, 16, 32, 16); //动画从下一项往上一项移动(上移)
			}
			break;
		}
	}
}

/* 二级菜单---设置控制 */
int menu2_set(void)
{
	uint8_t flag = 1;
	uint8_t menu3_flag;
	
	while(1)
	{
		KeyNum = Key_GetNum();
		if(KeyNum == 2) //上一项
		{
			flag --;
			if(flag == 0) flag = 6;
			OLED_AniUpdate();
			Ani_Dir_Flag = 2;
		}
		
		if(KeyNum == 1) //下一项
		{
			flag ++;
			if(flag == 7) flag = 1;
			OLED_AniUpdate();
			Ani_Dir_Flag = 1;
		}
		
		if(KeyNum == 3) //确认
		{
			OLED_Clear();
			OLED_AniUpdate();
			menu3_flag = flag; //把二级菜单的值传给三级菜单变量
		}
		
		if(menu3_flag == 1) return 0; //返回一级菜单
//		if(menu3_flag == 2) return 0;
//		if(menu3_flag == 3) return 0;
//		if(menu3_flag == 4) return 0;
		
		switch(flag)
		{
			case 1:
			{
				OLED_ShowString(0, 0, "<-                  ", OLED_8X16);
				OLED_ShowString(0, 16, "模式               ", OLED_8X16);
				OLED_ShowString(0, 32, "动画设置           ", OLED_8X16);
				OLED_ShowString(0, 48, "语言               ", OLED_8X16);
				
				/* 刚开始下移时,是从起始位置(0,0,0,0)->第一行的目标位置 */
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 0, 0, 0, 0, 0, 16, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 16, 32, 16, 0, 0, 16, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 2:
			{
				OLED_ShowString(0, 0, "<-                  ", OLED_8X16);
				OLED_ShowString(0, 16, "模式               ", OLED_8X16);
				OLED_ShowString(0, 32, "动画设置           ", OLED_8X16);
				OLED_ShowString(0, 48, "语言               ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 0, 16, 16, 0, 16, 32, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 32, 64, 16, 0, 16, 32, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 3:
			{
				OLED_ShowString(0, 0, "<-                  ", OLED_8X16);
				OLED_ShowString(0, 16, "模式               ", OLED_8X16);
				OLED_ShowString(0, 32, "动画设置           ", OLED_8X16);
				OLED_ShowString(0, 48, "语言               ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 16, 32, 16, 0, 32, 64, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 48, 32, 16, 0, 32, 64, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 4:
			{
				OLED_ShowString(0, 0, "<-                  ", OLED_8X16);
				OLED_ShowString(0, 16, "模式               ", OLED_8X16);
				OLED_ShowString(0, 32, "动画设置           ", OLED_8X16);
				OLED_ShowString(0, 48, "语言               ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 32, 64, 16, 0, 48, 32, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 0, 32, 16, 0, 48, 32, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 5:
			{
				OLED_ShowString(0, 0, "亮度                ", OLED_8X16);
				OLED_ShowString(0, 16, "版本               ", OLED_8X16);
				OLED_ShowString(0, 32, "                   ", OLED_8X16);
				OLED_ShowString(0, 48, "                   ", OLED_8X16);
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 48, 32, 16, 0, 0, 32, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 16, 32, 16, 0, 0, 32, 16); //动画从下一项往上一项移动(上移)
			}
			break;
			case 6:
			{
				OLED_ShowString(0, 0, "亮度                ", OLED_8X16);
				OLED_ShowString(0, 16, "版本               ", OLED_8X16);
				OLED_ShowString(0, 32, "                   ", OLED_8X16);
				OLED_ShowString(0, 48, "                   ", OLED_8X16);
				/*
					最后一行上移时,(0,32,16,16)->最后一行的目标位置
					(0,32,16,16)坐标前两个数代表最后一行下边的位置,0列32行
					后两个数代表第一行的宽度和高度(风扇控制：宽度16,高度16)
				*/
				if(Ani_Dir_Flag == 1) OLED_Animation(0, 0, 32, 16, 0, 16, 32, 16); //动画从上一项往下一项移动(下移)
				if(Ani_Dir_Flag == 2) OLED_Animation(0, 32, 16, 16, 0, 16, 32, 16); //动画从下一项往上一项移动(上移)
			}
			break;
		}
	}
}
