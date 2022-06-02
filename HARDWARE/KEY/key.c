#include "stm32f10x.h"
#include "key.h"
#include "sys.h" 
#include "delay.h"
#include "stdio.h"

/**************************************************************************************************** 
*                             长按、单击、双击定义
* 长按事件：任何大于 KEY_LONG_PRESS_TIME 
* 单击事件：按下时间不超过 KEY_LONG_PRESS_TIME 且 释放后 KEY_WAIT_DOUBLE_TIME 内无再次按下的操作
* 双击事件：俩次短按时间间隔小于KEY_WAIT_DOUBLE_TIME，俩次短按操作合并为一次双击事件。
* 特殊说明：
*          1.短按和长按时间间隔小于 KEY_WAIT_DOUBLE_TIME，响应一次单击和长按事件，不响应双击事件
*          2.连续2n次短按，且时间间隔小于 KEY_WAIT_DOUBLE_TIME，响应为n次双击
*          3.连续2n+1次短按，且时间间隔小于 KEY_WAIT_DOUBLE_TIME，且最后一次KEY_WAIT_DOUBLE_TIME内无操作，
*				响应为n次双击 和 一次单击事件
****************************************************************************************************/
#define KEY_LONG_PRESS_TIME    50 // 20ms*50 = 1s
#define KEY_WAIT_DOUBLE_TIME   25 // 20ms*25 = 500ms

#define KEY_PRESSED_LEVEL      0  // 按键按下是电平为低
/**************************************************************************************************** 
*                             局部函数定义
****************************************************************************************************/
static KEY_PinLevel_TypeDef KEY_ReadPin(void);   // 按键读取按键的电平函数
static void KEY_GetAction_PressOrRelease(void); // 获取按键是按下还是释放，保存到结构体

/**************************************************************************************************** 
*                             全局变量
****************************************************************************************************/
KEY_Configure_TypeDef KeyCfg={		
		0,						//按键长按计数
		KEY_Action_Release,		//虚拟当前IO电平，按下1，抬起0
		KEY_Status_Idle,        //按键状态
		KEY_Event_Null,         //按键事件
		KEY_ReadPin             //读IO电平函数
};
/**************************************************************************************************** 
*                             函数定义
****************************************************************************************************/
// 按键读取按键的电平函数，更具实际情况修改
static KEY_PinLevel_TypeDef KEY_ReadPin(void) //按键读取函数
{
  return (KEY_PinLevel_TypeDef) GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0);
}
// 获取按键是按下还是释放，保存到结构体
static void KEY_GetAction_PressOrRelease(void) // 根据实际按下按钮的电平去把它换算成虚拟的结果
{
	if(KeyCfg.KEY_ReadPin_Fcn() == KEY_PRESSED_LEVEL)
	{
		KeyCfg.KEY_Action = KEY_Action_Press;
	}
	else
	{
		KeyCfg.KEY_Action =  KEY_Action_Release;
	}
}

//按键初始化函数
void KEY_Init(void) //IO初始化
{ 
 	GPIO_InitTypeDef GPIO_InitStructure;
 
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);//使能PORTA时钟
	//初始化 WK_UP-->GPIOA.0	  
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //设置成上拉输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA.0
}

/**************************************************************************************************** 
*                             读取按键状态机
****************************************************************************************************/
void KEY_ReadStateMachine(void)
{
    KEY_GetAction_PressOrRelease();
	
	switch(KeyCfg.KEY_Status)
	{
		//状态：没有按键按下
		case KEY_Status_Idle:
			if(KeyCfg.KEY_Action == KEY_Action_Press)
			{
				KeyCfg.KEY_Status = KEY_Status_Debounce;
				KeyCfg.KEY_Event = KEY_Event_Null;
			}
			else
			{
				KeyCfg.KEY_Status = KEY_Status_Idle;
				KeyCfg.KEY_Event = KEY_Event_Null;
			}
			break;
			
		//状态：消抖
		case KEY_Status_Debounce:
			if(KeyCfg.KEY_Action == KEY_Action_Press)
			{
				KeyCfg.KEY_Status = KEY_Status_ConfirmPress;
				KeyCfg.KEY_Event = KEY_Event_Null;
			}
			else
			{
				KeyCfg.KEY_Status = KEY_Status_Idle;
				KeyCfg.KEY_Event = KEY_Event_Null;
			}
			break;	


		//状态：继续按下
		case KEY_Status_ConfirmPress:
			if( (KeyCfg.KEY_Action == KEY_Action_Press) && ( KeyCfg.KEY_Count >= KEY_LONG_PRESS_TIME))
			{
				KeyCfg.KEY_Status = KEY_Status_ConfirmPressLong;
				KeyCfg.KEY_Event = KEY_Event_Null;
				KeyCfg.KEY_Count = 0;
			}
			else if( (KeyCfg.KEY_Action == KEY_Action_Press) && (KeyCfg.KEY_Count < KEY_LONG_PRESS_TIME))
			{
				KeyCfg.KEY_Count++;
				KeyCfg.KEY_Status = KEY_Status_ConfirmPress;
				KeyCfg.KEY_Event = KEY_Event_Null;
			}
			else
			{
				KeyCfg.KEY_Count = 0;
				KeyCfg.KEY_Status = KEY_Status_WaiteAgain;// 按短了后释放
				KeyCfg.KEY_Event = KEY_Event_Null;

			}
			break;	
			
		//状态：一直长按着
		case KEY_Status_ConfirmPressLong:
			if(KeyCfg.KEY_Action == KEY_Action_Press) 
			{   // 一直等待其放开
				KeyCfg.KEY_Status = KEY_Status_ConfirmPressLong;
				KeyCfg.KEY_Event = KEY_Event_Null;
				KeyCfg.KEY_Count = 0;
			}
			else
			{
				KeyCfg.KEY_Status = KEY_Status_Idle;
				KeyCfg.KEY_Event = KEY_Event_LongPress;
				KeyCfg.KEY_Count = 0;
			}
			break;	
			
		//状态：等待是否再次按下
		case KEY_Status_WaiteAgain:
			if((KeyCfg.KEY_Action != KEY_Action_Press) && ( KeyCfg.KEY_Count >= KEY_WAIT_DOUBLE_TIME))
			{   // 第一次短按,且释放时间大于KEY_WAIT_DOUBLE_TIME
				KeyCfg.KEY_Count = 0;
				KeyCfg.KEY_Status = KEY_Status_Idle;  
				KeyCfg.KEY_Event = KEY_Event_SingleClick;// 响应单击
				
			}
			else if((KeyCfg.KEY_Action != KEY_Action_Press) && ( KeyCfg.KEY_Count < KEY_WAIT_DOUBLE_TIME))
			{// 第一次短按,且释放时间还没到KEY_WAIT_DOUBLE_TIME
				KeyCfg.KEY_Count ++;
				KeyCfg.KEY_Status = KEY_Status_WaiteAgain;// 继续等待
				KeyCfg.KEY_Event = KEY_Event_Null;
				
			}
			else // 第一次短按,且还没到KEY_WAIT_DOUBLE_TIME 第二次被按下
			{
				KeyCfg.KEY_Count = 0;
				KeyCfg.KEY_Status = KEY_Status_SecondPress;// 第二次按下
				KeyCfg.KEY_Event = KEY_Event_Null;
			}
			break;		
		case KEY_Status_SecondPress:
			if( (KeyCfg.KEY_Action == KEY_Action_Press) && ( KeyCfg.KEY_Count >= KEY_LONG_PRESS_TIME))
			{
				KeyCfg.KEY_Status = KEY_Status_ConfirmPressLong;// 第二次按的时间大于 KEY_LONG_PRESS_TIME
				KeyCfg.KEY_Event = KEY_Event_SingleClick; // 先响应单击
				KeyCfg.KEY_Count = 0;
			}
			else if( (KeyCfg.KEY_Action == KEY_Action_Press) && ( KeyCfg.KEY_Count < KEY_LONG_PRESS_TIME))
			{
                KeyCfg.KEY_Count ++;
				KeyCfg.KEY_Status = KEY_Status_SecondPress;
				KeyCfg.KEY_Event = KEY_Event_Null;
			}
            else 
            {// 第二次按下后在 KEY_LONG_PRESS_TIME内释放
                KeyCfg.KEY_Count = 0;
				KeyCfg.KEY_Status = KEY_Status_Idle;
				KeyCfg.KEY_Event = KEY_Event_DoubleClick; // 响应双击
            }
			break;	
		default:
			break;
	}

}
