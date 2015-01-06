#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h> //��ʱ��
#include <string.h>
#include <fcntl.h>
#include "EV_com.h"
#include "../ev_driver/smart210_uart.h"
#include "EV_timer.h"


static unsigned char recvbuf[512],sendbuf[512];
static unsigned char snNo = 0;




volatile unsigned char pcReqLock;//PC���󻥳���
volatile unsigned char pcReqType;//PC�������� 0��ʾ�������־����
volatile unsigned char pcReqFlag;//VMC 0���� 1��Ҫ��������  2���ڴ�������

static unsigned char vmcState = EV_DISCONNECT,lastVmcState = EV_DISCONNECT;



EV_callBack EV_callBack_fun = NULL;



/*********************************************************************************************************
**��ʱ�������� ������ʱ�� 1��VMCͨ�ų�ʱ��ʱ�� 2PC����ʱ��ʱ��
*********************************************************************************************************/
static int timerId_vmc = 0,timerId_pc = 0;

void EV_heart_ISR(void)
{
	EV_setVmcState(EV_DISCONNECT);
}


void EV_pcTimer_ISR(void)//PC����ʱ����
{
	EV_setVmcState(EV_DISCONNECT);
	pcReqFlag = PC_REQ_IDLE;
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
	int ret = 0;
	ret = uart_read(ch,1);
	return ret;
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
	uart_clear();
	uart_write((char *)buf,ix);
	//logSerialStr(2,buf);
	EV_callBack_fun(2,(void *)buf);
}



//0��ʱ 1ACK  2NAK
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
** Descriptions		:		PC���ʹ����������ݰ�
** input parameters	:   
** output parameters:		��
** Returned value	:		0 ʧ�� 1�ɹ�  
*********************************************************************************************************/
int EV_sendReq()
{
	unsigned char i;
	unsigned char	len = sendbuf[1];//�ض�λ���Ͱ�У��ֵ
	
	sendbuf[2] = snNo;
	unsigned short crc = EV_crcCheck(sendbuf,len);
	sendbuf[len + 0] = crc / 256;
	sendbuf[len + 1] = crc % 256;
	
	for(i = 0;i < 3;)
	{
		uart_write(sendbuf,len + 2);
		//logSerialStr(2,sendbuf);//���������־
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
		else if(rAck == NAK_RPT)//vmc�ܾ���������
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
** Descriptions:      PC������������
** input parameters:  type ���͵������������� ����Ϊ��   
** output parameters:   ��
** Returned value:    0 ʧ�� 1�ɹ�  
*********************************************************************************************************/

int32_t	EV_pcReqSend(uint8_t type,unsigned char ackBack,uint8_t *data,uint8_t len)
{
	uint8_t	ix = 0;
	int i;
	
	if(pcReqFlag != PC_REQ_IDLE)
	{
		EV_callBack_fun(3,"EV_pcReqSend is not PC_REQ_IDLE:\n");
		return 0;//������������˳�
	}

	sendbuf[ix++] = HEAD_EF;
	sendbuf[ix++] = len + HEAD_LEN;
	sendbuf[ix++] = 0;//Ԥ��
	sendbuf[ix++] = (ackBack == 1) ? VER_F0_1 : VER_F0_0;
	sendbuf[ix++] = type;
	for(i = 0;i < len;i++)
	{
		sendbuf[ix++] = data[i];
	}
	EV_setReqType(type);
	
	pcReqFlag = PC_REQ_SENDING;

	EV_callBack_fun(3,"EV_pcReqSend:\n");

	if(type == VENDOUT_IND)//�������� ��ʱ1����30��
		EV_timer_start(timerId_pc,EV_TIMEROUT_PC_LONG);
	else						//һ��Ϊ3��
		EV_timer_start(timerId_pc,EV_TIMEROUT_PC);


	return 1;

}





/*********************************************************************************************************
** Function name	:		send
** Descriptions		:		PC��Ӧ���ݰ��������յ������ݰ�
** input parameters	:   
** output parameters:		��
** Returned value	:		0 ʧ�� 1�ɹ�  
*********************************************************************************************************/
int EV_send()
{
	
	unsigned char MT = recvbuf[4];

	if(snNo == recvbuf[2])//�ذ�
    {
		if(recvbuf[3] == VER_F0_1)
			EV_replyACK(1);
        return 1;
    }
	
	snNo = recvbuf[2];
	//���Է�������  VMC���� POLL �� 0501
	if((MT == POLL) || (MT == ACTION_RPT && recvbuf[HEAD_LEN] == 5 && recvbuf[HEAD_LEN + 1]))
	{
		if(EV_getVmcState() == EV_MANTAIN && MT == POLL)
		{
			EV_initFlow(EV_EXIT_MANTAIN,NULL,0);//�˳�ά��ģʽ
		}
		else if(EV_getVmcState() != EV_MANTAIN && MT == ACTION_RPT)
		{
			EV_initFlow(EV_ENTER_MANTAIN,NULL,0);//����ά��ģʽ
		}
		else if(pcReqFlag == PC_REQ_SENDING)//PC��������
		{
			return EV_sendReq();
		}
	}

	if(recvbuf[3] == VER_F0_1)
	{
		EV_replyACK(1);
	}

	switch(MT)
	{
		case ACTION_RPT:
			EV_vmcRpt(EV_ACTION_RPT,recvbuf,recvbuf[1]);
			break;
		case VMC_SETUP://��ʼ��������Ϣ
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
	unsigned char ch,ix = 0,len,i;
	unsigned short crc;
	if(!uart_isNotEmpty())
		return 0;
	EV_getCh((char *)&ch);//HEAD ���ܰ�ͷ
	if(ch != HEAD_EF)
    {
        //ev_log.out("Head = %x is not e5",ch);
        //clear();
        return 0;
    }
	recvbuf[ix++] = ch;//recvbuf[0]	
	for(i = 0;i < 10;i++)
	{
		if(uart_isNotEmpty())	
			break;
		else	
			EV_msleep(5);		
	}
	EV_getCh((char *)&ch);//len ���ճ���
	if(ch < HEAD_LEN)
    {
        //ev_log.out("Len = %d < 5",ch);
        return 0;
    }
	recvbuf[ix++] = ch;//recvbuf[1]
	len = ch;
	//sn:recvbuf[2] + VER_F:recvbuf[3] + MT:recvbuf[4] + data + crc
	int rcx = 40;
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
        return 0;
    }
	
	crc = EV_crcCheck(recvbuf,len);
    if(crc/256 != recvbuf[len] || crc % 256 != recvbuf[len + 1])
    {	
		//ev_log.out("EV_recv:crc Err!");
		return 0;
    }
	
	EV_callBack_fun(1,(void *)recvbuf);
	
	EV_send();	//���յ� ��ȷ�İ�	

	
    return 1;
	
}





/*********************************************************************************************************
** Function name:     	EV_task
** Descriptions:	    PC��VMCͨ��������ӿ�
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void EV_task()
{
	
	if(EV_recv())
	{
		EV_timer_start(timerId_vmc,EV_TIMEROUT_VMC);
		if(EV_getVmcState() == EV_DISCONNECT)
		{
			EV_callBack_fun(3,"EV_INITTING........\n");
			EV_setVmcState(EV_INITTING);
			EV_initFlow(EV_SETUP_REQ, NULL,0);
		}
		EV_callBack_fun(3,"EV_recv\n");
	}
	else
	{
		EV_msleep(20);
	}

	if(EV_getVmcState() == EV_DISCONNECT)
	{
		EV_initFlow(EV_OFFLINE, NULL,0);
		EV_callBack_fun(3,"EV_OFFLINE........\n");
	}
		
	
}



/*********************************************************************************************************
** Function name	:		EV_initFlow
** Descriptions		:		��ʼ��Э�����̺���
** input parameters	:   
** output parameters:		��
** Returned value	:		��  
*********************************************************************************************************/
int EV_initFlow(const unsigned char type,const unsigned char *data,
		const unsigned char len)
{

	unsigned char	buf[256] = {0},callType = 0;
	EV_timer_stop(timerId_pc);
	EV_setReqType(0);
	
	switch(type)
	{
        case EV_SETUP_REQ:	//	1 ��ʼ�� GET_SETUP
        	EV_callBack_fun(3,"EV_SETUP_REQ\n");
			EV_pcReqSend(GET_SETUP,0,NULL,0);
			break;
		case EV_SETUP_RPT://��ʼ��������Ϣ
			EV_callBack_fun(3,"EV_SETUP_RPT\n");
		case EV_CONTROL_REQ: // 2��ʼ����ɱ�־	
			EV_callBack_fun(3,"EV_CONTROL_REQ\n");
			buf[0] = 19;
			buf[1] = 0x00;
			EV_pcReqSend(CONTROL_IND,1,buf,2);
			
			break;
		case EV_CONTROL_RPT:			
		case EV_STATE_REQ: // 3.��ȡ�ۻ���״̬
			EV_callBack_fun(3,"EV_STATE_REQ\n");
			EV_pcReqSend(GET_STATUS,0,NULL,0);
			break;
		case EV_STATE_RPT:
			EV_callBack_fun(3,"EV_STATE_RPT\n");
			if(EV_getLastVmcState() == EV_MANTAIN)
				callType = EV_EXIT_MANTAIN;
			else
				callType = EV_ONLINE;
			if(data[HEAD_LEN] & 0x03)
				EV_setVmcState(EV_FAULT);
			else
				EV_setVmcState(EV_NORMAL);

		case EV_ONLINE://����
			//ev_vm_state.state = getVmcState();
			//ev_callBack(callType,(void *)&ev_vm_state);
				
			break;
		case EV_OFFLINE://����
			//ev_vm_state.state = getVmcState();
			//ev_callBack(EV_OFFLINE,(void *)&ev_vm_state);
			break;

		case EV_ENTER_MANTAIN:
			EV_setVmcState(EV_MANTAIN);
			//ev_callBack(EV_ENTER_MANTAIN,NULL);
			EV_callBack_fun(3,"EV_ENTER_MANTAIN\n");
			break;
		case EV_EXIT_MANTAIN:
			EV_setVmcState(EV_INITTING);
			EV_callBack_fun(3,"EV_EXIT_MANTAIN\n");
			if(EV_getLastVmcState() == EV_INITTING || EV_getLastVmcState() == EV_DISCONNECT) //ϵͳ����ʼ��
			{
				EV_pcReqSend(GET_SETUP,0,NULL,0);
			}
			else//�˳�ά��ģʽ �ָ�ԭ����״̬ ǩ����ʼ��
			{
				buf[0] = 19; buf[1] = 0x00;
				EV_pcReqSend(CONTROL_IND,1,buf,2);
			}
			
			break;
		case EV_ACTION_RPT:	//������־
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
** Descriptions		:		PC�������������̽ӿ�
** input parameters	:   
** output parameters:		��
** Returned value	:		��  
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
		case EV_ONLINE://����
			//ev_vm_state.state = getVmcState();
			//ev_callBack(callType,(void *)&ev_vm_state);
				
			break;
		case EV_OFFLINE://����
			//ev_vm_state.state = getVmcState();
			//ev_callBack(EV_OFFLINE,(void *)&ev_vm_state);
			break;

		case EV_ENTER_MANTAIN:
			EV_setVmcState(EV_MANTAIN);
			//ev_callBack(EV_ENTER_MANTAIN,NULL);
			break;
		case EV_EXIT_MANTAIN:
			EV_setVmcState(EV_INITTING);
			if(EV_getLastVmcState() == EV_INITTING || EV_getLastVmcState() == EV_DISCONNECT) //ϵͳ����ʼ��
			{
				EV_pcReqSend(GET_SETUP,0,NULL,0);
			}
			else//�˳�ά��ģʽ �ָ�ԭ����״̬ ǩ����ʼ��
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
** Descriptions		:		VMC��Ӧ���ݰ��ӿ� ���л�Ӧ�����ݾ���ͨ���˽ӿڹ���
** input parameters	:   	type :MT ������
** output parameters:		��
** Returned value	:		��  
*********************************************************************************************************/
int32_t	EV_vmcRpt(const uint8_t type,const uint8_t *data,const uint8_t len)
{
	if(type == EV_ACTION_RPT && data[HEAD_LEN] == 7)//������־
	{
		EV_setVmcState(EV_INITTING);
		return EV_initFlow(EV_SETUP_REQ,data,len);
	}
	
	if(EV_getVmcState()== EV_INITTING)//���ڳ�ʼ�� 
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
	ret = uart_setParity(databits,stopbits,parity);
	ret = uart_setBaud(baud);
	return fd;
}



int EV_closeSerialPort()
{

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
}

int EV_release()
{
	EV_timer_release(timerId_vmc);
	EV_timer_release(timerId_pc);
	uart_close();
	EV_callBack_fun = NULL;
}



