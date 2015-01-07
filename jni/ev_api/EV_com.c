#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h> //¶¨Ê±Æ÷
#include <string.h>
#include <fcntl.h>
#include "EV_com.h"
#include "../ev_driver/smart210_uart.h"
#include "EV_timer.h"


static unsigned char recvbuf[512],sendbuf[512];
static unsigned char snNo = 0;




volatile unsigned char pcReqLock;//PCÇëÇó»¥³âËø
volatile unsigned char pcReqType;//PCÇëÇóÀàÐÍ 0±íÊ¾ÎÞÇëÇó±êÖ¾¿ÕÏÐ
volatile unsigned char pcReqFlag;//VMC 0¿ÕÏÐ 1ÐèÒª·¢ËÍÇëÇó  2ÕýÔÚ´¦ÀíÇëÇó

static unsigned char vmcState = EV_DISCONNECT,lastVmcState = EV_DISCONNECT;

static int vmc_fd = -1;

EV_callBack EV_callBack_fun = NULL;



/*********************************************************************************************************
**¶¨Ê±Æ÷·þÎñº¯Êý Á½¸ö¶¨Ê±Æ÷ 1ÓëVMCÍ¨ÐÅ³¬Ê±¶¨Ê±Æ÷ 2PCÇëÇó³¬Ê±¶¨Ê±Æ÷
*********************************************************************************************************/
static int timerId_vmc = 0,timerId_pc = 0;

void EV_heart_ISR(void)
{
	EV_setVmcState(EV_DISCONNECT);
	EV_callBack_fun(3,"EV_heart_ISR:timeout!!");
}


void EV_pcTimer_ISR(void)//PCÇëÇó³¬Ê±º¯Êý
{
	EV_setVmcState(EV_DISCONNECT);
	pcReqFlag = PC_REQ_IDLE;
	EV_callBack_fun(3,"EV_pcTimer_ISR:timeout!!");
}










void EV_setReqType(const uint8_t type)	
{ 
	pcReqType = type;
	if(type == 0)
		pcReqFlag = PC_REQ_IDLE;
}



uint8_t	EV_getReqType() 
{return pcReqType;}



void EV_setVmcState(const uint8_t type)	
{
	lastVmcState = vmcState; 
	vmcState = type;
}


uint8_t	EV_getVmcState()				
{return vmcState;}


uint8_t	EV_getLastVmcState()
{return lastVmcState;}



int EV_getCh(unsigned char *ch)
{
	return uart_read(vmc_fd,ch,1);
}


unsigned short EV_crcCheck(unsigned char *msg,unsigned char len)
{
    unsigned short i, j, crc = 0, current = 0;
        for(i=0;i<len;i++) {
            current = msg[i] << 8;
            for(j=0;j<8;j++) {
                if((short)(crc^current)<0)
                    crc = (crc<<1)^0x1021;
                else
                    crc <<= 1;
                current <<= 1;
            }
        }
        return crc;

}

void EV_replyACK(const unsigned char flag)
{
	unsigned char pc_ack,ix = 0,buf[10] = {0};
    pc_ack = (flag == 1) ? ACK : NAK;
    buf[ix++] = HEAD_EF;
	buf[ix++] = HEAD_LEN;
	buf[ix++] = snNo;
	buf[ix++] = VER_F0_0;
	buf[ix++] = pc_ack;
	unsigned short crc = EV_crcCheck(buf,ix);
	buf[ix++] = crc / 256;
	buf[ix++] = crc % 256;
	//uart_clear();
	uart_write(vmc_fd,(char *)buf,ix);
	//logSerialStr(2,buf);
	EV_callBack_fun(2,(void *)buf);
}



//0³¬Ê± 1ACK  2NAK
uint8_t EV_recvACK()
{
	uint8_t ix = 0,buf[HEAD_LEN + 2],ch;//400ms
	uint16_t crc,timeout = 400;
	while(timeout)
	{
		if(EV_getCh((char *)&ch))
        {
           buf[ix++] = ch;
           if(ix >= (HEAD_LEN + 2))
		   {
				if(buf[4] == POLL)
				{
					EV_replyACK(1);ix = 0;
				}	
				else
					break;
		   }    
        }
		else
		{
			EV_msleep(5);
			timeout--;
		}
	}
	if(timeout == 0)
    {
        return 0;
    }
	
	crc = EV_crcCheck(buf,HEAD_LEN);
    if(crc/256 != buf[HEAD_LEN] || crc % 256 != buf[HEAD_LEN + 1])
    {	
		return 0;
    }
	return buf[4];
}




/*********************************************************************************************************
** Function name	:		EV_sendReq
** Descriptions		:		PC·¢ËÍ´®¿ÚÇëÇóÊý¾Ý°ü
** input parameters	:   
** output parameters:		ÎÞ
** Returned value	:		0 Ê§°Ü 1³É¹¦  
*********************************************************************************************************/
int EV_sendReq()
{
	unsigned char i;
	unsigned char	len = sendbuf[1];//ÖØ¶¨Î»·¢ËÍ°üÐ£ÑéÖµ
	
	sendbuf[2] = snNo;
	unsigned short crc = EV_crcCheck(sendbuf,len);
	sendbuf[len + 0] = crc / 256;
	sendbuf[len + 1] = crc % 256;
	
	for(i = 0;i < 3;)
	{
		uart_write(vmc_fd,sendbuf,len + 2);
		//logSerialStr(2,sendbuf);//Êä³ö·¢ËÍÈÕÖ¾
		EV_callBack_fun(2,(void *)sendbuf);
		if(EV_getReqType() != VENDOUT_IND)
		{
			pcReqFlag = PC_REQ_HANDLING;
			return 1;
		}
		uint8_t rAck = EV_recvACK();
		if(rAck == ACK_RPT)
		{
			pcReqFlag = PC_REQ_HANDLING;
			return 1;
		}
		else if(rAck == NAK_RPT)//vmc¾Ü¾ø½ÓÊÜÃüÁî
		{
			EV_vmcRpt(EV_NAK,NULL,0);
			return 1;
		}
		else
		{
			usleep(500000);i--;
		}
	}	

	//EV_callBack_fun(2,(void *)sendbuf);
	//pcReqTimerHandler(this);
	return 1;
}


/*********************************************************************************************************
** Function name:     pcReqSend
** Descriptions:      PC·¢ËÍÊý¾ÝÇëÇó
** input parameters:  type ·¢ËÍµÄÊý¾ÝÇëÇóÀàÐÍ ²»ÄÜÎªÁã   
** output parameters:   ÎÞ
** Returned value:    0 Ê§°Ü 1³É¹¦  
*********************************************************************************************************/

int32_t	EV_pcReqSend(uint8_t type,unsigned char ackBack,uint8_t *data,uint8_t len)
{
	uint8_t	ix = 0;
	int i;
	
	if(pcReqFlag != PC_REQ_IDLE)
	{
		EV_callBack_fun(3,"EV_pcReqSend is not PC_REQ_IDLE:\n");
		return 0;//Èç¹ûÓÐÇëÇóÔòÍË³ö
	}

	sendbuf[ix++] = HEAD_EF;
	sendbuf[ix++] = len + HEAD_LEN;
	sendbuf[ix++] = 0;//Ô¤Áô
	sendbuf[ix++] = (ackBack == 1) ? VER_F0_1 : VER_F0_0;
	sendbuf[ix++] = type;
	for(i = 0;i < len;i++)
	{
		sendbuf[ix++] = data[i];
	}
	EV_setReqType(type);
	
	pcReqFlag = PC_REQ_SENDING;

	EV_callBack_fun(3,"EV_pcReqSend:\n");

	if(type == VENDOUT_IND)//³ö»õÃüÁî ³¬Ê±1·ÖÖÓ30Ãë
		EV_timer_start(timerId_pc,EV_TIMEROUT_PC_LONG);
	else						//Ò»°ãÎª3Ãë
		EV_timer_start(timerId_pc,EV_TIMEROUT_PC);


	return 1;

}





/*********************************************************************************************************
** Function name	:		send
** Descriptions		:		PC»ØÓ¦Êý¾Ý°ü²¢½âÎöÊÕµ½µÄÊý¾Ý°ü
** input parameters	:   
** output parameters:		ÎÞ
** Returned value	:		0 Ê§°Ü 1³É¹¦  
*********************************************************************************************************/
int EV_send()
{
	
	unsigned char MT = recvbuf[4];

	if(snNo == recvbuf[2])//ÖØ°ü
    {
		if(recvbuf[3] == VER_F0_1)
			EV_replyACK(1);
        return 1;
    }
	
	snNo = recvbuf[2];
	//¿ÉÒÔ·¢ËÍÇëÇó  VMC·¢ËÍ POLL ºÍ 0501
	if((MT == POLL) || (MT == ACTION_RPT && recvbuf[HEAD_LEN] == 5 && recvbuf[HEAD_LEN + 1]))
	{

		if(pcReqFlag == PC_REQ_SENDING)
			EV_sendReq();
		else if(recvbuf[3] == VER_F0_1)
			EV_replyACK(1);	
			
		if(EV_getVmcState() == EV_MANTAIN && MT == POLL)
		{
			EV_initFlow(EV_EXIT_MANTAIN,NULL,0);//ÍË³öÎ¬»¤Ä£Ê½
		}
		else if(EV_getVmcState() != EV_MANTAIN && MT == ACTION_RPT)
		{
			EV_initFlow(EV_ENTER_MANTAIN,NULL,0);//½øÈëÎ¬»¤Ä£Ê½
		}
		
	}
	else 
	{
		if(recvbuf[3] == VER_F0_1)
			EV_replyACK(1);	
	}
		


	
	switch(MT)
	{
		case ACTION_RPT:
			EV_vmcRpt(EV_ACTION_RPT,recvbuf,recvbuf[1]);
			break;
		case VMC_SETUP://³õÊ¼»¯·µ»ØÐÅÏ¢
			if(EV_getReqType() == GET_SETUP)
			EV_vmcRpt(EV_SETUP_RPT,recvbuf,recvbuf[1]);
			break;
		case INFO_RPT:
			if(EV_getReqType() == GET_INFO)
				EV_vmcRpt(EV_INFO_RPT,recvbuf,recvbuf[1]);
			break;
		case ACK_RPT: case NAK_RPT:
			if(EV_getReqType() == CONTROL_IND)
				EV_vmcRpt(EV_CONTROL_RPT,recvbuf,recvbuf[1]);
			break;
		case STATUS_RPT:
			
			if(EV_getReqType() == GET_STATUS)
			EV_vmcRpt(EV_STATE_RPT,recvbuf,recvbuf[1]);
			break;
		case VENDOUT_RPT:
			if(EV_getReqType() == VENDOUT_IND)
			EV_vmcRpt(EV_TRADE_RPT,recvbuf,recvbuf[1]);
			break;

		default:
			break;
	}
	return 1;


}

int EV_recv()
{
	unsigned char ch,ix = 0,len,i,head = 0;
	unsigned short crc;
	if(!uart_isNotEmpty(vmc_fd))
		return 0;
	EV_getCh((char *)&ch);//HEAD ½ÓÊÜ°üÍ
	if(ch != HEAD_EF)
    {
		uart_clear(vmc_fd);
        return 0;
    }
	recvbuf[ix++] = ch;//recvbuf[0]	
	EV_getCh((char *)&ch);//len ½ÓÊÕ³¤¶È
	if(ch < HEAD_LEN)
    {
        //ev_log.out("Len = %d < 5",ch);
        return 0;
    }
	recvbuf[ix++] = ch;//recvbuf[1]
	
	len = ch;
	//sn:recvbuf[2] + VER_F:recvbuf[3] + MT:recvbuf[4] + data + crc
	int rcx = 20;
    while(rcx > 0) //200ms
    {
        if(EV_getCh((char *)&ch))
        {
           recvbuf[ix++] = ch;		  
           if(ix >= (len + 2))
               break;
        }
		else
		{
			EV_msleep(5);	
			rcx--;
		}
    }
	if(rcx <= 0)
    {
		//ev_log.out("EV_recv:timeout!");
		//EV_callBack_fun(3,"EV_recv:timeout!!!!!!!\n");
        return 0;
    }
	
	crc = EV_crcCheck(recvbuf,len);
    if(crc/256 != recvbuf[len] || crc % 256 != recvbuf[len + 1])
    {	
		//ev_log.out("EV_recv:crc Err!");
		return 0;
    }
	EV_callBack_fun(1,(void *)recvbuf);	
	EV_send();	//½ÓÊÕµ½ ÕýÈ·µÄ°ü	
    return 1;
	
}





/*********************************************************************************************************
** Function name:     	EV_task
** Descriptions:	    PCÓëVMCÍ¨ÐÅÖ÷ÈÎÎñ½Ó¿Ú
** input parameters:    ÎÞ
** output parameters:   ÎÞ
** Returned value:      ÎÞ
*********************************************************************************************************/
void EV_task(int fd)
{
	if(vmc_fd != fd)vmc_fd = fd;
	if(EV_recv())
	{
		EV_timer_start(timerId_vmc,EV_TIMEROUT_VMC);
		if(EV_getVmcState() == EV_DISCONNECT)
		{
			EV_callBack_fun(3,"EV_INITTING........\n");
			EV_setVmcState(EV_INITTING);
			EV_initFlow(EV_SETUP_REQ, NULL,0);
		}
		//EV_callBack_fun(3,"EV_recv\n");
	}
	else
	{
		EV_msleep(20);
	}

	if(EV_getVmcState() == EV_DISCONNECT && EV_getLastVmcState() != EV_DISCONNECT)
	{
		
		EV_initFlow(EV_OFFLINE, NULL,0);
		EV_callBack_fun(3,"EV_OFFLINE........\n");
	}
		
	
}



/*********************************************************************************************************
** Function name	:		EV_initFlow
** Descriptions		:		³õÊ¼»¯Ð­ÒéÁ÷³Ìº¯Êý
** input parameters	:   
** output parameters:		ÎÞ
** Returned value	:		ÎÞ  
*********************************************************************************************************/
int EV_initFlow(const unsigned char type,const unsigned char *data,
		const unsigned char len)
{

	unsigned char	buf[256] = {0},callType = 0;

		case EV_CONTROL_RPT:			
		case EV_STATE_REQ: // 3.»ñÈ¡ÊÛ»õ»ú×´Ì¬
			EV_callBack_fun(3,"EV_STATE_REQ\n");
			EV_pcReqSend(GET_STATUS,0,NULL,0);
			break;
		case EV_STATE_RPT:
			EV_callBack_fun(3,"EV_STATE_RPT\n");
			if(EV_ge
			EV_setVmcState(EV_MANTAIN);
			//ev_callBack(EV_ENTER_MANTAIN,NULL);
			EV_callBack_fun(3,"EV_ENTER_MANTAIN\n");
= EV_INITTING || EV_getLastVmcState() == EV_DISCONNECT) //ÏµÍ³¼¶³õÊ¼»¯
			{
				EV_pcReqSend(GET_SETUP,0,NULL,0);
			}
			else//ÍË³öÎ¬»¤Ä£Ê½ »Ö¸´Ô­À´µÄ×´Ì¬ Ç©µ½³õÊ¼»¯
			{
				buf[0] = 19; buf[1] = 0x00;
				EV_pcReqSend(CONTROL_IND,1,buf,2);
			}
			
			break;
		case EV_ACTION_RPT:	//ÖØÆô±êÖ¾
			EV_callBack_fun(3,"EV_ACTION_RPT\n");
			break;	
		case EV_TIMEOUT:	
			break;
		default:
			break;
	}
	return 1;

}


/*********************************************************************************************************
** Function name	:		EV_mainFlow
** Descriptions		:		PCÕý³£ÔËÐÐÖ÷Á÷³Ì½Ó¿Ú
** input parameters	:   
** output parameters:		ÎÞ
** Returned value	:		ÎÞ  
*********************************************************************************************************/
int32_t	EV_mainFlow(const uint8_t type,const uint8_t *data,const uint8_t len)
{
	char buf[20] = {0};
	pcReqFlag = PC_REQ_IDLE;
	EV_timer_stop(timerId_pc);
	uint8_t rptType = 0;
	switch(type)
	{
		case EV_NAK:
			//ev_callBack(EV_NAK,NULL);
			break;
		case EV_TRADE_RPT:
			EV_callBack_fun(3,"EV_TRADE_RPT");
			//ev_callBack(EV_TRADE_RPT,NULL);
			break;

		case EV_STATE_RPT:
			if(data[HEAD_LEN] & 0x03)
				EV_setVmcState(EV_FAULT);
			else
				EV_setVmcState(EV_NORMAL);
			//ev_vm_state.state = getVmcState();
			//ev_callBack(EV_STATE_RPT,(void *)&ev_vm_state);
			break;
		case EV_ONLINE://ÔÚÏß
			//ev_vm_state.state = getVmcState();
			//ev_callBack(callType,(void *)&ev_vm_state);
				
			break;
		case EV_OFFLINE://ÀëÏß
			//ev_vm_state.state = getVmcState();
			//ev_callBack(EV_OFFLINE,(void *)&ev_vm_state);
			break;

		case EV_ENTER_MANTAIN:
			EV_setVmcState(EV_MANTAIN);
			//ev_callBack(EV_ENTER_MANTAIN,NULL);
			break;
		case EV_EXIT_MANTAIN:
			EV_setVmcState(EV_INITTING);
			if(EV_getLastVmcState() == EV_INITTING || EV_getLastVmcState() == EV_DISCONNECT) //ÏµÍ³¼¶³õÊ¼»¯
			{
				EV_pcReqSend(GET_SETUP,0,NULL,0);
			}
			else//ÍË³öÎ¬»¤Ä£Ê½ »Ö¸´Ô­À´µÄ×´Ì¬ Ç©µ½³õÊ¼»¯
			{
				buf[0] = 19; buf[1] = 0x00;
				EV_pcReqSend(CONTROL_IND,1,buf,2);
			}
			
			break;
		case EV_TIMEOUT:
			EV_setReqType(0);
			//ev_callBack(EV_TIMEOUT,NULL);
			break;
	}
	
	return 1;
}


/*********************************************************************************************************
** Function name	:		EV_vmcRpt
** Descriptions		:		VMC»ØÓ¦Êý¾Ý°ü½Ó¿Ú ËùÓÐ»ØÓ¦µÄÊý¾Ý¾ùÐèÍ¨¹ý´Ë½Ó¿Ú¹ýÂË
** input parameters	:   	type :MT °üÀàÐÍ
** output parameters:		ÎÞ
** Returned value	:		ÎÞ  
*********************************************************************************************************/
int32_t	EV_vmcRpt(const uint8_t type,const uint8_t *data,const uint8_t len)
{
	if(type == EV_ACTION_RPT && data[HEAD_LEN] == 7)//ÖØÆô±êÖ¾
	{
		EV_setVmcState(EV_INITTING);
		return EV_initFlow(EV_SETUP_REQ,data,len);
	}
	
	if(EV_getVmcState()== EV_INITTING)//ÕýÔÚ³õÊ¼»¯ 
		return EV_initFlow(type,data,len);
	else
		return EV_mainFlow(type,data,len);
}
	


int EV_openSerialPort(char *portName,int baud,int databits,char parity,int stopbits)
{
	int ret;
	int fd = uart_open(portName);
	if (fd <0){

			return -1;
	}
	ret = uart_setParity(fd,databits,stopbits,parity);
	ret = uart_setBaud(fd,baud);
	uart_clear(fd);
	vmc_fd = fd;
	return fd;
}



int EV_closeSerialPort(int fd)
{
	uart_close(fd);
}



int EV_register(EV_callBack callBack)
{
	EV_callBack_fun = callBack;
	timerId_vmc = EV_timer_register((EV_timerISR)EV_heart_ISR);
	if(timerId_vmc < 0)		
		return -1;
	timerId_pc = EV_timer_register((EV_timerISR)EV_pcTimer_ISR);
	if(timerId_pc < 0)
		return -1;

	EV_setVmcState(EV_DISCONNECT);
	return 1;
}

int EV_release()
{
	EV_timer_release(timerId_vmc);
	EV_timer_release(timerId_pc);
	EV_closeSerialPort(vmc_fd);
	EV_callBack_fun = NULL;
	return 1;
}



