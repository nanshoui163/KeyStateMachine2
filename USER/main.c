#include "delay.h"
#include "sys.h"
#include "usart.h"
#include "timer.h"
#include "key.h"

int main(void)
{	

	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	uart_init(115200); // 用于查看输出
	TIM3_Int_Init(200-1,7200-1); //调用定时器使得20ms产生一个中断
	//按键初始化函数
	KEY_Init();
	while(1);

}     




