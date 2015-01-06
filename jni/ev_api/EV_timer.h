#ifndef _EV_TIMER_H_
#define _EV_TIMER_H_

typedef void (*EV_timerISR)(void);




int EV_timer_register(EV_timerISR timerISR);
void EV_timer_release(int timerId);
unsigned char EV_timer_start(int timerId,unsigned int sec);
void EV_timer_stop(int timerId);

void EV_msleep(unsigned long msec);//∫¡√ÎÀØ√ﬂ
#endif