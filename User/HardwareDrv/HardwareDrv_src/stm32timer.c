/**
 * stm32timer.c file for STM32F103.
 * Describtion:  STM32 ��ʱ��
 * Author: qinfei 2015.04.02
 * Version: GatewayV1.0
 * Support:qf.200806@163.com
 */

#include "stm32timer.h"
#include "leds.h"
#include "usart.h"

#include "myportmacro.h"//�������ͺ궨��


static uint32 SystemNowtime;//ϵͳ��ǰʱ�䵥λ10ms
u32 uip_timer=0;//uip��ʱ����ÿ10ms����1.


/* Timer6 ����ϵͳʱ----------------------------------------------------------*/
/* ��ʱ��6�жϷ������ */ 
void TIM6_IRQHandler(void)
{ 
    /*���ָ����TIM�жϷ������:TIM �ж�Դ */
    if (TIM_GetITStatus(TIM6, TIM_IT_Update) != RESET)
    {
      uip_timer++;//uip��ʱ������1
      Timing_Increase();
    } 
    
    /*���TIMx���жϴ�����λ:TIM �ж�Դ*/
    TIM_ClearITPendingBit(TIM6, TIM_IT_Update);	    		  			    	    
}

/**
 * ������ʱ��6�жϳ�ʼ��
 * ����ʱ��ѡ��ΪAPB1��2������APB1Ϊ36M
 * arr���Զ���װֵ��
 * psc��ʱ��Ԥ��Ƶ��
 * ����ʹ�õ��Ƕ�ʱ��3!
 * TIM6_Int_Init(1000,719); 100Khz����Ƶ�ʣ�������1000Ϊ10ms 
 */
void TIM6_Int_Init(u16 arr,u16 psc)
{	
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);//ʱ��ʹ��

    TIM_TimeBaseStructure.TIM_Period = arr;//��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ,������5000Ϊ500ms
    TIM_TimeBaseStructure.TIM_Prescaler = psc;//����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ,10Khz�ļ���Ƶ��  
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;//����ʱ�ӷָ�:TDTS = Tck_tim
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;//TIM���ϼ���ģʽ
    TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);//����TIM_TimeBaseInitStruct��ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
    TIM_ITConfig( TIM6,TIM_IT_Update|TIM_IT_Trigger,ENABLE);//ʹ�ܶ�ʱ��6���´����ж�

    TIM_Cmd(TIM6, ENABLE);//ʹ��TIMx����

    NVIC_InitStructure.NVIC_IRQChannel = TIM6_IRQn;//TIM6�ж�
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;//��ռ���ȼ�0��
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;//�����ȼ�3��
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//IRQͨ����ʹ��
    NVIC_Init(&NVIC_InitStructure);//����NVIC_InitStruct��ָ���Ĳ�����ʼ������NVIC�Ĵ��� 								 
}

/*
 * ��������Timing_Increase
 * ����  ��ϵͳʱ�������
 * ����  ����
 * ���  ��ϵͳ��ǰʱ��
 * ����  ���� SysTick �жϺ��� SysTick_Handler()����
 */  
void Timing_Increase(void)
{   
    /*��Χ��0-4294967295(4 Bytes)�ۼӵ������Լ��Ҫ3.6Сʱ*/
    SystemNowtime++;
}

/*
 * ��������GetSystemNowtime
 * ����  ����ȡϵͳ��ǰʱ��
 * ����  ����
 * ���  ��ϵͳ��ǰʱ��
 * ����  ���� SysTick �жϺ��� SysTick_Handler()����
 */  
uint32 GetSystemNowtime(void)
{  
    return SystemNowtime;//����ϵͳ��ǰʱ��
}


/* Timer3��DNS����Ҫ�Ķ�ʱ��---------------------------------------------------*/
/**
  * @brief  ��ʼ����ʱ���ж�
  * @param  None
  * @retval None
  */
void Timer_Interrupts_Config(void)
{
    NVIC_InitTypeDef  NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1; 
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/**
  * @brief  ��ʼ����ʱ�� 
  * @param  None
  * @retval None
  */
void Timer_Config(void)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); 

    TIM_DeInit(TIM3);//��λTIM3��ʱ��
                    
    /* TIM3 configuration */
    TIM_TimeBaseStructure.TIM_Period = 200;// 100ms    
    TIM_TimeBaseStructure.TIM_Prescaler = 36000;// ��Ƶ36000      
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;// ʱ�ӷ�Ƶ 
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;//�����������ϼ���
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    /* Clear TIM3 update pending flag[���TIM3����жϱ�־] */
    TIM_ClearFlag(TIM3, TIM_FLAG_Update);

    /* Enable TIM3 Update interrupt [TIM3����ж�����]*/
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE); 
    /* TIM3����������*/
    TIM3->CNT=0;
    /* TIM3 enable counter [����TIM3����]*/
    TIM_Cmd(TIM3, DISABLE);  
    /*Config interrupts*/
    Timer_Interrupts_Config();
}

/**
  * @brief  �����ʱ���������Ĵ�������ֵ��������ʱ��
  * @param  None
  * @retval None
  */
void Timer_Start(void)
{
    TIM3->CNT=0;//����������Ĵ�����ֵ�����Լ�С��֡�����
    /* Enable the TIM Counter */
    TIM_Cmd(TIM3, ENABLE); 
}

/**
  * @brief  ֹͣ��ʱ���������ʱ���ļ���ֵ
  * @param  None
  * @retval None
  */
void Timer_Stop(void)
{ 
    /* Disable the TIM Counter */
    TIM_Cmd(TIM3, DISABLE);
}


