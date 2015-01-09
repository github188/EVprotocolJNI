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
#include "../ev_driver/ev_config.h"

static unsigned char recvbuf[512],sendbuf[512];
static unsigned char snNo = 0;

#define LOG_STYLES_HELLO	( LOG_STYLE_DATETIMEMS | LOG_STYLE_LOGLEVEL | LOG_STYLE_PID | LOG_STYLE_TID | LOG_STYLE_SOURCE | LOG_STYLE_FORMAT | LOG_STYLE_NEWLINE )



volatile unsigned char pcLock;//PC���󻥳���
volatile unsigned char pcType,pcsubType;//PC�������� 0��ʾ�������־����
volatile unsigned char pcFlag;//VMC 0���� 1��Ҫ��������  2���ڴ�������

static unsigned char vm_state = EV_STATE_DISCONNECT,last_vm_state = EV_STATE_DISCONNECT;

static int vmc_fd = -1;


EV_callBack EV_callBack_fun = NULL;
/*********************************************************************************************************
**��ʱ�������� ������ʱ�� 1��VMCͨ�ų�ʱ��ʱ�� 2PC����ʱ��ʱ��
*********************************************************************************************************/
static int timerId_vmc = 0,timerId_pc = 0;

//����PC����
void EV_set_pc_cmd(const uint8_t type)	
{ 
	if(type == EV_NA)
	{
		EV_timer_stop(timerId_pc);
		pcFlag = PC_REQ_IDLE;
		pcType = EV_NA;
	}
	else
	{
		pcType = type;
		pcFlag = PC_REQ_SENDING;
	}		
}


uint8_t	EV_get_pc_cmd() 
{
	return pcType;
}



unsigned EV_setSubcmd(unsigned char type)
{
	pcsubType = type;
}


unsigned char EV_getSubcmd()
{
	return pcsubType;
}

void EV_set_pc_flag(unsigned char flag)
{
	pcFlag = flag;
}

unsigned char EV_get_pc_flag()
{
	return pcFlag;
}



void EV_setVmState(const unsigned char type)	
{
	last_vm_state = vm_state; 
	vm_state = type;
}


unsigned char EV_getVmState()				
{
	return vm_state;
}


unsigned char	EV_getLastVmState()
{
	return last_vm_state;
}






void EV_heart_ISR(void)
{
	EV_vmMainFlow(EV_OFFLINE, NULL,0);	
}


void EV_pcTimer_ISR(void)//PC����ʱ����
{

	EV_set_pc_flag(PC_REQ_IDLE);
	EV_LOGI1("PC request timeout...!!\n");
	if(EV_getVmState() == EV_STATE_INITTING)
	{
		EV_vmMainFlow(EV_OFFLINE, NULL,0);
	}
	else
	{	
		EV_vmMainFlow(EV_TIMEOUT, NULL,0);	
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



//0��ʱ 1ACK  2NAK
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
	for(i = 0;i < 3;i--)
	{
		uart_write(vmc_fd,sendbuf,len + 2);
		EV_COMLOG(2,sendbuf);//���������־

		if(sendbuf[VF] == VER_F0_1) //��Ҫ��ӦACK
		{
			uint8_t rAck = EV_recvACK();
			if(rAck == ACK_RPT)//���ڶ�ʱ��� ACK����
			{
				EV_set_pc_flag(PC_REQ_HANDLING);
				EV_vmRpt(EV_ACK_VM,NULL,0);
				return 1;
			}
			else if(rAck == NAK_RPT) //vmc�ܾ���������
			{
				EV_vmRpt(EV_NAK_VM,NULL,0);
				return 1;
			}
		}
		else
		{
			EV_set_pc_cmd(EV_NA);
			return 1;
		}
			
		EV_msleep(500);
		
	}	

	//����ʧ��
	EV_vmRpt(EV_NAK_VM,NULL,0);
	return 0;
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
	unsigned char mt = recvbuf[MT];
	if(snNo == recvbuf[SN])//�ذ� ֱ������
    {
		if(recvbuf[VF] == VER_F0_1)
			EV_replyACK(1);
        return 1;
    }
	snNo = recvbuf[SN];
	if((mt == POLL) || (mt == ACTION_RPT && recvbuf[HEAD_LEN] == 5 && recvbuf[HEAD_LEN + 1] == 0x01))
	{
		if(EV_get_pc_flag() == PC_REQ_SENDING)
			EV_sendReq();
		else
		if(recvbuf[3] == VER_F0_1)
			EV_replyACK(1);	
	}
	else 
	{
		if(recvbuf[3] == VER_F0_1)
			EV_replyACK(1);	
	}
	EV_vmRpt(mt,recvbuf,recvbuf[LEN]);

	return 1;


}

int EV_recv()
{
	unsigned char ch,ix = 0,len,i,head = 0;
	unsigned short crc;
	
	if(!uart_isNotEmpty(vmc_fd))
	//if(!EV_getCh((char *)&ch))
		return 0;
	EV_getCh((char *)&ch);//HEAD ���ܰ��
	if(ch != HEAD_EF)
    {
    	//EV_LOGCOM("EV_recv:%02x != %02x\n",ch,HEAD_EF);
		uart_clear(vmc_fd);
        return 0;
    }
	recvbuf[ix++] = ch;//recvbuf[0]	
	EV_getCh((char *)&ch);//len ���ճ���
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
	EV_send();	//���յ� ��ȷ�İ�	
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
	
	if(EV_get_pc_flag() != PC_REQ_IDLE)
	{
		EV_callbackhandle(EV_FAIL,"EV_pcReqSend is not PC_REQ_IDLE:\n");
		EV_LOGW("EV_pcReqSend send failed,anther request...");
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

	EV_set_pc_cmd(type);
	EV_LOGTASK("EV_pcReqSend:MT =%x\n",type);

	if(type == VENDOUT_IND)//�������� ��ʱ1����30��
		EV_timer_start(timerId_pc,EV_TIMEROUT_PC_LONG);
	else						//һ��Ϊ3��
		EV_timer_start(timerId_pc,EV_TIMEROUT_PC);


	return 1;

}




/*********************************************************************************************************
** Function name:     	EV_task
** Descriptions:	    PC��VMCͨ��������ӿ�
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void EV_task(int fd)
{
	if(vmc_fd != fd)vmc_fd = fd;
	if(EV_recv())
	{
		EV_timer_start(timerId_vmc,EV_TIMEROUT_VMC);
#if 0
		if(EV_getVmState() == EV_STATE_DISCONNECT)
		{
			EV_LOGTASK("EV_connected,start to init....\n");
			EV_initFlow(EV_SETUP_REQ, NULL,0);
		}
#endif
	}
	else
	{
		EV_msleep(50);
	}
		
	
}


/*********************************************************************************************************
** Function name	:		EV_vmMainFlow
** Descriptions		:		Э�������̺���
** input parameters	:   
** output parameters:		��
** Returned value	:		��  
*********************************************************************************************************/
int EV_vmMainFlow(const unsigned char type,const unsigned char *data,const unsigned char len)
{
	unsigned char	buf[256] = {0},callType = 0;
	switch(type)
	{
		case EV_NAK_VM:
			if(EV_get_pc_cmd() != EV_NA)//������ܾ�
			{
				EV_set_pc_cmd(EV_NA);
				EV_LOGFLOW("EV_NA\n");
				EV_callbackhandle(EV_NA,recvbuf);
			}
			break;
        case EV_SETUP_REQ:	//	1 ��ʼ�� GET_SETUP PC���ͳ�ʼ������
        	if(EV_getVmState() == EV_STATE_DISCONNECT)
        	{
        		EV_set_pc_cmd(EV_NA);
        		EV_setVmState(EV_STATE_INITTING);	
				EV_LOGFLOW("EV_connected,start to init....\n");
        	}
        	EV_callbackhandle(EV_SETUP_REQ,recvbuf);
			EV_LOGFLOW("EV_SETUP_REQ\n");
			EV_pcReqSend(GET_SETUP,0,NULL,0);
			break;
		case EV_SETUP_RPT://��ʼ��������Ϣ
			EV_LOGFLOW("EV_SETUP_RPT\n");
			EV_callbackhandle(EV_SETUP_RPT,recvbuf);
			EV_set_pc_cmd(EV_NA);
			if(EV_getVmState() != EV_STATE_INITTING) //�ж��Ƿ��ڳ�ʼ��
			{
				break;
			}//�ǳ�ʼ�� ֱ����һ��	
		
		case EV_CONTROL_REQ://���ͳ�ʼ����ɱ�־
			EV_LOGFLOW("EV_CONTROL_REQ\n");
			EV_callbackhandle(EV_CONTROL_REQ,recvbuf);
			buf[0] = 19;
			buf[1] = 0x00;
			EV_setSubcmd(19);
			EV_pcReqSend(CONTROL_IND,1,buf,2);
			break;
		case EV_CONTROL_RPT:
			if(EV_get_pc_cmd() == EV_CONTROL_REQ)
				EV_set_pc_cmd(EV_NA);
			if(EV_getSubcmd() == 19)//��ʼ����ɱ�־ 
			{
				if(EV_getVmState() != EV_STATE_INITTING)//���Զ���ʼ������ֱ����һ��
				{
					break;
				}
			}
			else
				break;
		case EV_STATE_REQ: // 3.��ȡ�ۻ���״̬
			EV_LOGFLOW("EV_STATE_REQ\n");
			EV_callbackhandle(EV_STATE_REQ,recvbuf);
			EV_pcReqSend(GET_STATUS,0,NULL,0);
			break;
		case EV_STATE_RPT://״̬����
			EV_LOGFLOW("EV_STATE_RPT\n");
			EV_callbackhandle(EV_STATE_RPT,recvbuf);
			if(EV_get_pc_cmd() == EV_STATE_REQ)
			{
				EV_set_pc_cmd(EV_NA);
			}

			if(EV_getVmState() == EV_STATE_INITTING)//��ʼ��״̬ ��ʾ��ʼ�����
			{
				if(data[HEAD_LEN] & 0x03)
					EV_setVmState(EV_STATE_FAULT);
				else
					EV_setVmState(EV_STATE_NORMAL);
			}
			else if(EV_getVmState() == EV_STATE_MANTAIN)//״̬��ά�� ������
			{
				break;
			}
			else
			{
				if(data[HEAD_LEN] & 0x03)
					EV_setVmState(EV_STATE_FAULT);
				else
					EV_setVmState(EV_STATE_NORMAL);

				break;
			}
		case EV_ONLINE://����
			EV_callbackhandle(EV_ONLINE,recvbuf);
			EV_LOGFLOW("EV_conneced online....\n");	
			break;
		case EV_OFFLINE://����
			EV_setVmState(EV_STATE_DISCONNECT);
			EV_LOGFLOW("EV_disconnected......\n");
			EV_callbackhandle(EV_OFFLINE,recvbuf);
			break;
		case EV_ACTION_REQ: // PC�������� Ŀǰ����
			
			break;
		case EV_ACTION_RPT: //VMC�������� 
			if(data[MT + 1] == 7)//VMC��������
			{
				EV_callbackhandle(EV_SETUP_REQ,recvbuf);
				EV_LOGFLOW("VMC is restarted......\n");
				EV_setVmState(EV_STATE_INITTING);
				EV_set_pc_cmd(EV_NA);
				EV_pcReqSend(GET_SETUP,0,NULL,0);
			}
			else if(data[MT + 1] == 5)
			{
				if(data[MT + 2] == 1)//��ʾ����ά��ģʽ
				{
					if(EV_getVmState() != EV_STATE_MANTAIN)
					{
						EV_set_pc_cmd(EV_NA);
						EV_setVmState(EV_STATE_MANTAIN);
						EV_LOGFLOW("EV_ENTER_MANTAIN\n");	
						EV_callbackhandle(EV_ENTER_MANTAIN,recvbuf);
					}
				}
			}
			else
			{
				EV_LOGFLOW("EV_ACTION_RPT\n");	
				EV_callbackhandle(EV_ACTION_RPT,recvbuf);
			}
			break;
		case EV_POLL:
			if(EV_getVmState() == EV_STATE_MANTAIN)//��ʾ�˳�ά��ģʽ
			{
				EV_setVmState(EV_STATE_INITTING);
				EV_LOGFLOW("EV_EXIT_MANTAIN\n");	
				EV_callbackhandle(EV_EXIT_MANTAIN,recvbuf);
				EV_pcReqSend(GET_SETUP,0,NULL,0);
			}
			break;
		case EV_ENTER_MANTAIN:
			
			break;
		case EV_EXIT_MANTAIN:
			break;
		
		case EV_TRADE_RPT: //�������淵��
			if(EV_get_pc_cmd() == EV_TRADE_REQ)
			{
				EV_set_pc_cmd(EV_NA);			
				EV_LOGFLOW("EV_TRADE_RPT\n");
				EV_callbackhandle(EV_TRADE_RPT,recvbuf);
			}
			
			break;
		case EV_TIMEOUT:
			EV_LOGFLOW("EV_TIMEOUT...cmd=%x\n",EV_get_pc_cmd());
			EV_callbackhandle(EV_TIMEOUT,recvbuf);
			break;

		
		default:
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
int32_t	EV_vmRpt(const uint8_t type,const uint8_t *data,const uint8_t len)
{
	unsigned char ev_type = type;

	if(type == EV_POLL)
	{
		if(EV_getVmState() == EV_STATE_DISCONNECT)//������������Զ������ʼ������
		{
			EV_vmMainFlow(EV_SETUP_REQ,NULL,0);
			return 1;
		}
	}
	else if(type == EV_ACK_VM)
	{
		if(EV_get_pc_cmd() == EV_CONTROL_REQ)
		{
			ev_type = EV_CONTROL_RPT;
			EV_vmMainFlow(ev_type,data,len);
			return 1;
		}
	}

	EV_vmMainFlow(type,data,len);
	return 1;

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
		
	EV_setVmState(EV_STATE_DISCONNECT);
	EV_LOGI7("EV_register OK.....");


	
#if 0
	//�����ļ�
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



