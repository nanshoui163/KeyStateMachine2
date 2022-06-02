#include "stm32f10x.h"
#include "key.h"
#include "sys.h" 
#include "delay.h"
#include "stdio.h"

/**************************************************************************************************** 
*                             ������������˫������
* �����¼����κδ��� KEY_LONG_PRESS_TIME 
* �����¼�������ʱ�䲻���� KEY_LONG_PRESS_TIME �� �ͷź� KEY_WAIT_DOUBLE_TIME �����ٴΰ��µĲ���
* ˫���¼������ζ̰�ʱ����С��KEY_WAIT_DOUBLE_TIME�����ζ̰������ϲ�Ϊһ��˫���¼���
* ����˵����
*          1.�̰��ͳ���ʱ����С�� KEY_WAIT_DOUBLE_TIME����Ӧһ�ε����ͳ����¼�������Ӧ˫���¼�
*          2.����2n�ζ̰�����ʱ����С�� KEY_WAIT_DOUBLE_TIME����ӦΪn��˫��
*          3.����2n+1�ζ̰�����ʱ����С�� KEY_WAIT_DOUBLE_TIME�������һ��KEY_WAIT_DOUBLE_TIME���޲�����
*				��ӦΪn��˫�� �� һ�ε����¼�
****************************************************************************************************/
#define KEY_LONG_PRESS_TIME    50 // 20ms*50 = 1s
#define KEY_WAIT_DOUBLE_TIME   25 // 20ms*25 = 500ms

#define KEY_PRESSED_LEVEL      0  // ���������ǵ�ƽΪ��
/**************************************************************************************************** 
*                             �ֲ���������
****************************************************************************************************/
static KEY_PinLevel_TypeDef KEY_ReadPin(void);   // ������ȡ�����ĵ�ƽ����
static void KEY_GetAction_PressOrRelease(void); // ��ȡ�����ǰ��»����ͷţ����浽�ṹ��

/**************************************************************************************************** 
*                             ȫ�ֱ���
****************************************************************************************************/
KEY_Configure_TypeDef KeyCfg={		
		0,						//������������
		KEY_Action_Release,		//���⵱ǰIO��ƽ������1��̧��0
		KEY_Status_Idle,        //����״̬
		KEY_Event_Null,         //�����¼�
		KEY_ReadPin             //��IO��ƽ����
};
/**************************************************************************************************** 
*                             ��������
****************************************************************************************************/
// ������ȡ�����ĵ�ƽ����������ʵ������޸�
static KEY_PinLevel_TypeDef KEY_ReadPin(void) //������ȡ����
{
  return (KEY_PinLevel_TypeDef) GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0);
}
// ��ȡ�����ǰ��»����ͷţ����浽�ṹ��
static void KEY_GetAction_PressOrRelease(void) // ����ʵ�ʰ��°�ť�ĵ�ƽȥ�������������Ľ��
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

//������ʼ������
void KEY_Init(void) //IO��ʼ��
{ 
 	GPIO_InitTypeDef GPIO_InitStructure;
 
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);//ʹ��PORTAʱ��
	//��ʼ�� WK_UP-->GPIOA.0	  
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //���ó���������
	GPIO_Init(GPIOA, &GPIO_InitStructure);//��ʼ��GPIOA.0
}

/**************************************************************************************************** 
*                             ��ȡ����״̬��
****************************************************************************************************/
void KEY_ReadStateMachine(void)
{
    KEY_GetAction_PressOrRelease();
	
	switch(KeyCfg.KEY_Status)
	{
		//״̬��û�а�������
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
			
		//״̬������
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


		//״̬����������
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
				KeyCfg.KEY_Status = KEY_Status_WaiteAgain;// �����˺��ͷ�
				KeyCfg.KEY_Event = KEY_Event_Null;

			}
			break;	
			
		//״̬��һֱ������
		case KEY_Status_ConfirmPressLong:
			if(KeyCfg.KEY_Action == KEY_Action_Press) 
			{   // һֱ�ȴ���ſ�
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
			
		//״̬���ȴ��Ƿ��ٴΰ���
		case KEY_Status_WaiteAgain:
			if((KeyCfg.KEY_Action != KEY_Action_Press) && ( KeyCfg.KEY_Count >= KEY_WAIT_DOUBLE_TIME))
			{   // ��һ�ζ̰�,���ͷ�ʱ�����KEY_WAIT_DOUBLE_TIME
				KeyCfg.KEY_Count = 0;
				KeyCfg.KEY_Status = KEY_Status_Idle;  
				KeyCfg.KEY_Event = KEY_Event_SingleClick;// ��Ӧ����
				
			}
			else if((KeyCfg.KEY_Action != KEY_Action_Press) && ( KeyCfg.KEY_Count < KEY_WAIT_DOUBLE_TIME))
			{// ��һ�ζ̰�,���ͷ�ʱ�仹û��KEY_WAIT_DOUBLE_TIME
				KeyCfg.KEY_Count ++;
				KeyCfg.KEY_Status = KEY_Status_WaiteAgain;// �����ȴ�
				KeyCfg.KEY_Event = KEY_Event_Null;
				
			}
			else // ��һ�ζ̰�,�һ�û��KEY_WAIT_DOUBLE_TIME �ڶ��α�����
			{
				KeyCfg.KEY_Count = 0;
				KeyCfg.KEY_Status = KEY_Status_SecondPress;// �ڶ��ΰ���
				KeyCfg.KEY_Event = KEY_Event_Null;
			}
			break;		
		case KEY_Status_SecondPress:
			if( (KeyCfg.KEY_Action == KEY_Action_Press) && ( KeyCfg.KEY_Count >= KEY_LONG_PRESS_TIME))
			{
				KeyCfg.KEY_Status = KEY_Status_ConfirmPressLong;// �ڶ��ΰ���ʱ����� KEY_LONG_PRESS_TIME
				KeyCfg.KEY_Event = KEY_Event_SingleClick; // ����Ӧ����
				KeyCfg.KEY_Count = 0;
			}
			else if( (KeyCfg.KEY_Action == KEY_Action_Press) && ( KeyCfg.KEY_Count < KEY_LONG_PRESS_TIME))
			{
                KeyCfg.KEY_Count ++;
				KeyCfg.KEY_Status = KEY_Status_SecondPress;
				KeyCfg.KEY_Event = KEY_Event_Null;
			}
            else 
            {// �ڶ��ΰ��º��� KEY_LONG_PRESS_TIME���ͷ�
                KeyCfg.KEY_Count = 0;
				KeyCfg.KEY_Status = KEY_Status_Idle;
				KeyCfg.KEY_Event = KEY_Event_DoubleClick; // ��Ӧ˫��
            }
			break;	
		default:
			break;
	}

}
