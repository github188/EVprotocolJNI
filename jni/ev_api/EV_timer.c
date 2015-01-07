#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h> //定时器
#include <string.h>
#include <fcntl.h>
#include "EV_timer.h"

	
//给定5定时器 最多可以注册5个定时器
#define EV_TIMER_SUM	5


typedef struct _st_timer_{
	unsigned char startFlag;
	int timerId; //定时器ID
	EV_timerISR timerIsr;//定时器服务函数
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

//注册定时器500ms的定时器  成功返回定时器ID号  失败返回 -1
int EV_timer_register(EV_timerISR timerISR)
{
	int i;
	
	for(i = 0;i < EV_TIMER_SUM;i++)
	{
		if(EV_timer[i].timerId == 0)//未注册定时器
		{
			EV_timer[i].timerId = i;
			EV_timer[i].timerIsr = timerISR;
			EV_timer[i].startFlag = 0;
			EV_timer[i].timerTick = 0;
			if(i == 0)//第一个定时器 需要创建
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
		if(i == EV_timer[i].timerId)//找到了该定时器
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
		if(EV_timer[i].timerId != 0) return; //还有定时器
	}
	//否则 要关掉定时器
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



void EV_msleep(unsigned long msec)//毫秒睡眠
{

	if(msec) 	
		usleep(msec * 1000);
}



