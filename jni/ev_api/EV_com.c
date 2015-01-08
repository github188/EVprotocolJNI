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
#include "../ev_driver/ev_config.h"

static unsigned char recvbuf[512],sendbuf[512];
static unsigned char snNo = 0;

#define LOG_STYLES_HELLO	( LOG_STYLE_DATETIMEMS | LOG_STYLE_LOGLEVEL | LOG_STYLE_PID | LOG_STYLE_TID | LOG_STYLE_SOURCE | LOG_STYLE_FORMAT | LOG_STYLE_NEWLINE )



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

//ÉèÖÃPCÀàÐÍ
void EV_setReqType(const uint8_t type)	
{ 
	
	if(type == 0)
	{
		EV_timer_stop(timerId_pc);
		pcReqFlag = PC_REQ_IDLE;
	}
	pcReqType = type;
		
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






void EV_heart_ISR(void)
{
	EV_setVmcState(EV_DISCONNECT);
	EV_LOGI1("EV_disconnected......\n");
	EV_initFlow(EV_OFFLINE, NULL,0);	
}


void EV_pcTimer_ISR(void)//PCÇëÇó³¬Ê±º¯Êý
{

	pcReqFlag = PC_REQ_IDLE;
	EV_LOGI1("PC request timeout...!!\n");
	if(EV_getVmcState() == EV_INITTING)
	{
		EV_initFlow(EV_OFFLINE, NULL,0);
		EV_setVmcState(EV_DISCONNECT);
	}
	else
	{	
		EV_mainFlow(EV_TIMEOUT, NULL,0);	
	}
	
}






void EV_callbackhandle(int type,void *ptr)
{
	if(EV_callBack_fun != NULL)
		EV_callBack_fun(type,ptr);
}



void EV_COMLOG(int type ,char *data)
{
	char buf[256] = {0};
	unsigned int i;
	for(i = 0;i < data[1] + 2;i++)
		sprintf(&buf[i*3],"%02x ",data[i]);
	if(type == 1)
		EV_LOGCOM("VM-->PC[%d]:%s\n",data[1],buf);
	else 
		EV_LOGCOM("PC-->VM[%d]:%s\n",data[1],buf);
}




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
	uart_write(vmc_fd,(char *)buf,ix);
	EV_COMLOG(2,buf);
}



//0³¬Ê± 1ACK  2NAK
uint8_t EV_recvACK()
{
	uint8_t ix = 0,buf[HEAD_LEN + 2],ch;//400ms
	uint16_t crc,timeout = 400;//4   
	while(timeout)
	{
		if(EV_getCh((char *)&ch))
        {
           buf[ix++] = ch;
           if(ix >= (HEAD_LEN + 2))
		   {
				if(buf[4] == POLL)
				{
					EV_COMLOG(1, buf);
					EV_replyACK(1);ix = 0;
				}	
				else
					break;
		   }    
        }
		else
		{
			EV_msleep(10);
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
	EV_COMLOG(1, buf);
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
	for(i = 0;i < 3;i--)
	{
		uart_write(vmc_fd,sendbuf,len + 2);
		EV_COMLOG(2,sendbuf);//Êä³ö·¢ËÍÈÕÖ¾

		if(sendbuf[VF] == VER_F0_1) //ÐèÒª»ØÓ¦ACK
		{
			uint8_t rAck = EV_recvACK();
			if(rAck == ACK_RPT)//¶ÔÓÚ¶ÌÊ±¼äµÄ ACK´¦Àí
			{
				pcReqFlag = PC_REQ_HANDLING;
				EV_vmcRpt(EV_ACK,NULL,0);
				return 1;
			}
			else if(rAck == NAK_RPT) //vmc¾Ü¾ø½ÓÊÜÃüÁî
			{
				EV_vmcRpt(EV_NAK,NULL,0);
				return 1;
			}
		}
		else
			return 1;
		EV_msleep(500);
		
	}	
	
	return 0;
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
		EV_callbackhandle(EV_FAIL,"EV_pcReqSend is not PC_REQ_IDLE:\n");
		EV_LOGW("EV_pcReqSend send failed,anther request...");
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

	EV_LOGTASK("EV_pcReqSend:MT =%x\n",type);

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
	unsigned char mt = recvbuf[MT];

	if(snNo == recvbuf[SN])//ÖØ°ü
    {
		if(recvbuf[VF] == VER_F0_1)
			EV_replyACK(1);
        return 1;
    }
	snNo = recvbuf[SN];
	//¿ÉÒÔ·¢ËÍÇëÇó  VMC·¢ËÍ POLL ºÍ 0501
	if((mt == POLL) || (mt == ACTION_RPT && recvbuf[HEAD_LEN] == 5 && recvbuf[HEAD_LEN + 1] == 0x01))
	{
		//EV_LOGTASK("EV_can send req:MT =%x pcreq= %x\n",mt,pcReqFlag);
		if(pcReqFlag == PC_REQ_SENDING)
			EV_sendReq();
		else if(recvbuf[3] == VER_F0_1)
			EV_replyACK(1);	
			
		if(EV_getVmcState() == EV_MANTAIN && mt == POLL)
		{
			EV_initFlow(EV_EXIT_MANTAIN,NULL,0);//ÍË³öÎ¬»¤Ä£Ê½
		}
		else if(EV_getVmcState() != EV_MANTAIN && mt == ACTION_RPT)
		{
			EV_initFlow(EV_ENTER_MANTAIN,NULL,0);//½øÈëÎ¬»¤Ä£Ê½
		}
		
	}
	else 
	{
		if(recvbuf[3] == VER_F0_1)
			EV_replyACK(1);	
	}
		
	switch(mt)
	{
		case ACTION_RPT:
			EV_vmcRpt(EV_ACTION_RPT,recvbuf,recvbuf[1]);
			break;
		case VMC_SETUP://³õÊ¼»¯·µ»ØÐÅÏ¢
			if(EV_getReqType() == GET_SETUP) //Ö¸Áî»Ø¸´³É¹¦
				EV_vmcRpt(EV_SETUP_RPT,recvbuf,recvbuf[1]);
			break;
		case INFO_RPT:
			if(EV_getReqType() == GET_INFO)
				EV_vmcRpt(EV_INFO_RPT,recvbuf,recvbuf[1]);
			break;
		case NAK_RPT:
			EV_vmcRpt(EV_NAK,NULL,0);
			break;
		case ACK_RPT: //¶ÔÓÚ³¤Ê±¼äµÄACK»ØÓ¦´¦Àí
			EV_vmcRpt(EV_ACK,NULL,0);
			break;
		case STATUS_RPT:
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
	
	//if(!uart_isNotEmpty(vmc_fd))
	if(!EV_getCh((char *)&ch))
		return 0;
	//EV_getCh((char *)&ch);//HEAD ½ÓÊÜ°üÍ
	if(ch != HEAD_EF)
    {
    	EV_LOGCOM("EV_recv:%02x != %02x\n",ch,HEAD_EF);
		uart_clear(vmc_fd);
        return 0;
    }
	recvbuf[ix++] = ch;//recvbuf[0]	
	EV_getCh((char *)&ch);//len ½ÓÊÕ³¤¶È
	if(ch < HEAD_LEN)
    {
        EV_LOGCOM("EV_recv:len = %d < %d\n",ch,HEAD_LEN);
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
		EV_LOGCOM("EV_recv:timeout!\n");
        return 0;
    }
	
	crc = EV_crcCheck(recvbuf,len);
    if(crc/256 != recvbuf[len] || crc % 256 != recvbuf[len + 1])
    {	
		EV_LOGCOM("EV_recv:crc = Err\n");
		return 0;
    }
	EV_COMLOG(1,recvbuf);
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
			EV_LOGTASK("EV_connected,start to init....\n");
			EV_setVmcState(EV_INITTING);
			EV_initFlow(EV_SETUP_REQ, NULL,0);
		}
	}
	else
	{
		EV_msleep(20);
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
	//EV_timer_stop(timerId_pc);
	//EV_setReqType(0);
	
	switch(type)
	{
        case EV_SETUP_REQ:	//	1 ³õÊ¼»¯ GET_SETUP
        	EV_callbackhandle(EV_SETUP_REQ,recvbuf);
			EV_LOGFLOW("EV_SETUP_REQ\n");
			EV_pcReqSend(GET_SETUP,0,NULL,0);
			break;
		case EV_SETUP_RPT://³õÊ¼»¯·µ»ØÐÅÏ¢
			EV_setReqType(0);
			EV_LOGFLOW("EV_SETUP_RPT\n");
			EV_callbackhandle(EV_SETUP_RPT,recvbuf);
		case EV_CONTROL_REQ: // 2³õÊ¼»¯Íê³É±êÖ¾	
			EV_LOGFLOW("EV_CONTROL_REQ\n");
			EV_callbackhandle(EV_CONTROL_REQ,recvbuf);
			buf[0] = 19;
			buf[1] = 0x00;
			EV_pcReqSend(CONTROL_IND,1,buf,2);
			
			break;
		case EV_CONTROL_RPT:
			EV_setReqType(0);
		case EV_STATE_REQ: // 3.»ñÈ¡ÊÛ»õ»ú×´Ì¬
			EV_LOGFLOW("EV_STATE_REQ\n");
			EV_callbackhandle(EV_STATE_REQ,recvbuf);
			EV_pcReqSend(GET_STATUS,0,NULL,0);
			break;
		case EV_STATE_RPT:
			if(data[HEAD_LEN] & 0x03)
				EV_setVmcState(EV_FAULT);
			else
				EV_setVmcState(EV_NORMAL);
			EV_LOGFLOW("EV_STATE_RPT\n");
			if(EV_getReqType() == GET_STATUS)
				EV_setReqType(0);
			EV_callbackhandle(EV_STATE_RPT,recvbuf);
			if(EV_getLastVmcState() == EV_MANTAIN)
			{
				callType = EV_EXIT_MANTAIN;
				break;
			}	
			else if(EV_getLastVmcState() == EV_INITTING)
				callType = EV_ONLINE;			
			else
				break;
		case EV_ONLINE://ÔÚÏß
			EV_callbackhandle(EV_ONLINE,recvbuf);
			EV_LOGFLOW("EV_conneced online....\n");	
			break;
		case EV_OFFLINE://ÀëÏß
			EV_LOGFLOW("EV_OFFLINE\n");	
			EV_callbackhandle(EV_OFFLINE,recvbuf);
			break;

		case EV_ENTER_MANTAIN:
			EV_setVmcState(EV_MANTAIN);
			EV_LOGFLOW("EV_ENTER_MANTAIN\n");	
			EV_callbackhandle(EV_ENTER_MANTAIN,recvbuf);
			
			break;
		case EV_EXIT_MANTAIN:
			EV_setVmcState(EV_INITTING);
			EV_LOGFLOW("EV_EXIT_MANTAIN\n");	
			EV_callbackhandle(EV_EXIT_MANTAIN,recvbuf);
			EV_pcReqSend(GET_SETUP,0,NULL,0);
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
	//pcReqFlag = PC_REQ_IDLE;
	//EV_timer_stop(timerId_pc);
	uint8_t rptType = 0;
	switch(type)
	{
		case EV_ACK:
			if(EV_getReqType() == CONTROL_IND)
				{
					EV_setReqType(0);
					EV_vmcRpt(EV_CONTROL_RPT,recvbuf,recvbuf[1]);
				}	
			break;
		case EV_NAK:
			EV_LOGTASK("EV_NAK\n");
			break;
		case EV_TRADE_RPT:
			EV_setReqType(0);
			EV_LOGTASK("EV_TRADE_RPT\n");
			EV_callbackhandle(EV_TRADE_RPT,recvbuf);
			break;


		case EV_STATE_REQ: // 3.»ñÈ¡ÊÛ»õ»ú×´Ì¬
			EV_LOGFLOW("EV_STATE_REQ\n");
			EV_callbackhandle(EV_STATE_REQ,recvbuf);
			EV_pcReqSend(GET_STATUS,0,NULL,0);
			break;
		case EV_STATE_RPT:
			if(data[HEAD_LEN] & 0x03)
				EV_setVmcState(EV_FAULT);
			else
				EV_setVmcState(EV_NORMAL);
			EV_LOGFLOW("EV_STATE_RPT\n");
			if(EV_getReqType() == GET_STATUS)
				EV_setReqType(0);
			EV_callbackhandle(EV_STATE_RPT,recvbuf);
			if(EV_getLastVmcState() == EV_MANTAIN)
			{
				//callType = EV_EXIT_MANTAIN;
				break;
			}			
			else
				break;

		case EV_FAIL:
		case EV_TIMEOUT:
			EV_LOGTASK("EV_FAIL:type = %d\n",EV_getReqType());
			EV_callbackhandle(EV_FAIL,recvbuf);
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
	unsigned char ev_type = type;
	if(type == EV_ACTION_RPT && data[HEAD_LEN] == 7)//ÖØÆô±êÖ¾
	{
		EV_setVmcState(EV_INITTING);
		return EV_initFlow(EV_SETUP_REQ,data,len);
	}
	else if(type == EV_ACK)
	{
		switch(EV_getReqType())
		{
			case CONTROL_IND://·¢ËÍ¿ØÖÆÃüÁî
				ev_type = EV_CONTROL_RPT;
				break;
			default:
				break;
		}
		
	}
	else if(type == EV_NAK)
	{
		if(EV_getReqType() != 0) //¾Ü¾øÇëÇó
		{
			EV_setReqType(0);
			EV_LOGTASK("PC request is rejected....");
			EV_callbackhandle(type,(void *)data);
			return 0;
		}
	}
		
	
	if(EV_getVmcState()== EV_INITTING)//ÕýÔÚ³õÊ¼»¯ 
		return EV_initFlow(ev_type,data,len);
	else
		return EV_mainFlow(ev_type,data,len);
}
	


int EV_openSerialPort(char *portName,int baud,int databits,char parity,int stopbits)
{
	int ret;
	int fd = uart_open(portName);
	if (fd <0){
			EV_LOGE("EV_openSerialPort:oepn %s failed\n",portName);
			return -1;
	}
	ret = uart_setParity(fd,databits,stopbits,parity);
	if(ret == 0)
	{
		EV_LOGW("EV_openSerialPort:setParity failed\n");
	}
	ret = uart_setBaud(fd,baud);
	if(ret == 0)
	{
		EV_LOGW("EV_openSerialPort:setBaud failed\n");
	}
	uart_clear(fd);
	vmc_fd = fd;

	EV_LOGI4("EV_openSerialPort:Serial[%s] open suc..fd = %d\n",portName,fd);
	return fd;
}



int EV_closeSerialPort(int fd)
{
	EV_LOGI4("EV_closeSerialPort:closed...\n");
	uart_close(fd);
}



int EV_register(EV_callBack callBack)
{
	if(callBack == NULL)
	{
		EV_LOGW("The callback is NULL.....\n");
	}
	EV_callBack_fun = callBack;
	timerId_vmc = EV_timer_register((EV_timerISR)EV_heart_ISR);
	if(timerId_vmc < 0)
	{
		EV_LOGE("Timer[timerId_vmc] register failed.....");
		return -1;
	}
		
	timerId_pc = EV_timer_register((EV_timerISR)EV_pcTimer_ISR);
	if(timerId_pc < 0)
	{
		EV_LOGE("Timer[timerId_pc] register failed.....");
		return -1;
	}
		
	EV_setVmcState(EV_DISCONNECT);
	EV_LOGI7("EV_register OK.....");


	
#if 0
	//´´½¨ÎÄ¼þ
	log_fd = CreateLogHandle() ;
	if( log_fd == NULL )
	{
		EV_callBack_fun(3,"CreateLogHandle fail/////////////////////");
		return -1;
	}
	EV_callBack_fun(3,"CreateLogHandle:suc111111111111111111111111111111111");
	SetLogOutput(log_fd, LOG_OUTPUT_FILE , "./ev_test.log" , LOG_NO_OUTPUTFUNC );
	SetLogLevel( log_fd , LOG_LEVEL_INFO );
	SetLogStyles( log_fd , LOG_STYLES_HELLO , LOG_NO_STYLEFUNC );

	DebugLog( log_fd , __FILE__ , __LINE__ , "hello DEBUG" );
	InfoLog( log_fd , __FILE__ , __LINE__ , "hello INFO" );
	WarnLog( log_fd , __FILE__ , __LINE__ , "hello WARN" );
	ErrorLog( log_fd , __FILE__ , __LINE__ , "hello ERROR" );
	FatalLog( log_fd , __FILE__ , __LINE__ , "hello FATAL" );
	DestroyLogHandle( log_fd );
#endif	
	
	return 1;
}

int EV_release()
{
	EV_timer_release(timerId_vmc);
	EV_timer_release(timerId_pc);
	EV_closeSerialPort(vmc_fd);
	EV_callBack_fun = NULL;
	EV_LOGI7("EV_release OK.....");
	return 1;
}



