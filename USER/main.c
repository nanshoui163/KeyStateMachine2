#include "delay.h"
#include "sys.h"
#include "usart.h"
#include "timer.h"
#include "key.h"

int main(void)
{	

	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	uart_init(115200); // ���ڲ鿴���
	TIM3_Int_Init(200-1,7200-1); //���ö�ʱ��ʹ��20ms����һ���ж�
	//������ʼ������
	KEY_Init();
	while(1);

}     




