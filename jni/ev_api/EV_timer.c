#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h> //��ʱ��
#include <string.h>
#include <fcntl.h>
#include "EV_timer.h"

	
//����5��ʱ�� ������ע��5����ʱ��
#define EV_TIMER_SUM	5


typedef struct _st_timer_{
	unsigned char startFlag;
	int timerId; //��ʱ��ID
	EV_timerISR timerIsr;//��ʱ��������
	unsigned int timerTick;
	
}ST_TIMER;

static ST_TIMER EV_timer[EV_TIMER_SUM] = {0};



void EV_timer_ISR()
{
	int i = 0;
	EV_timerISR timerisr = NULL;
	for(i = 0;i < EV_TIMER_SUM;i++)
	{
		if(EV_timer[i].timerId == 0) continue;
		if(EV_timer[i].timerTick )EV_timer[i].timerTick--;
		else
		{
			if(EV_timer[i].timerIsr != NULL && EV_timer[i].startFlag)
			{		
				timerisr = EV_timer[i].timerIsr;
				EV_timer[i].startFlag = 0;
				timerisr();
				
			}
		}
			
	}	
}

//ע�ᶨʱ��500ms�Ķ�ʱ��  �ɹ����ض�ʱ��ID��  ʧ�ܷ��� -1
int EV_timer_register(EV_timerISR timerISR)
{
	int i;
	
	for(i = 0;i < EV_TIMER_SUM;i++)
	{
		if(EV_timer[i].timerId == 0)//δע�ᶨʱ��
		{
			EV_timer[i].timerId = i;
			EV_timer[i].timerIsr = timerISR;
			EV_timer[i].startFlag = 0;
			EV_timer[i].timerTick = 0;
			if(i == 0)//��һ����ʱ�� ��Ҫ����
			{
				signal(SIGALRM, EV_timer_ISR);
			}
			struct itimerval itv;
			itv.it_value.tv_sec=0;
			itv.it_value.tv_usec=500000;
			itv.it_interval.tv_sec=0;
			itv.it_interval.tv_usec=500000;
			setitimer(ITIMER_REAL, &itv, NULL); 
			
			return i;//timerid
			
		}
	}
	return -1;
}


void EV_timer_release(int timerId)
{
	int i ;
	struct itimerval itv;
	if(timerId < 0) return;
	for(i = 0;i < EV_TIMER_SUM;i++)
	{
		if(i = EV_timer[i].timerId)//�ҵ��˸ö�ʱ��
		{
			EV_timer[i].timerId = 0;
			EV_timer[i].timerIsr = NULL;
			EV_timer[i].startFlag = 0;
			EV_timer[i].timerTick = 0;
			break;
		}
	}
	for(i = 0;i < EV_TIMER_SUM;i++)
	{
		if(EV_timer[i].timerId != 0) return; //���ж�ʱ��
	}
	//���� Ҫ�ص���ʱ��
	itv.it_value.tv_sec=0;
	itv.it_value.tv_usec=0;
	itv.it_interval.tv_sec=0;
	itv.it_interval.tv_usec=0;
	setitimer(ITIMER_REAL, &itv, NULL); 

}


void EV_timer_stop(int timerId)
{
	if(timerId < 0) return;
	EV_timer[timerId].startFlag = 0;
}


unsigned char EV_timer_start(int timerId,unsigned int sec)
{
	if(timerId < 0) return 0;
	EV_timer[timerId].timerTick = (sec * 2);
	EV_timer[timerId].startFlag = 1;
}



void EV_msleep(unsigned long msec)//����˯��
{
	unsigned int second = msec / 1000;
	unsigned int msecond = msec % 1000;
	if(second) 		sleep(second);
	if(msecond) 	usleep(msecond * 1000);
}



