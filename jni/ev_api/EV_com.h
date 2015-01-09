#ifndef _EV_COM_H_
#define _EV_COM_H_

#define HEAD_EF     0xE5
#define HEAD_LEN    5
#define VER_F0_1    0x41
#define VER_F0_0    0x40


#define HEAD   0
#define LEN    1
#define SN     2
#define VF     3  
#define MT     4

//VMC-->PC
#define ACK_RPT    0x01
#define NAK_RPT    0x02
#define POLL       0x03
#define VMC_SETUP  0x05
#define PAYIN_RPT 0x06
#define HUODAO_RPT 0x0E
#define VENDOUT_RPT 0x08
#define INFO_RPT 0x11
#define ACTION_RPT 0x0B
#define BUTTON_RPT  0x0C
#define STATUS_RPT  0x0D

//PC-->VMC
#define ACK      0x80
#define NAK      0x81
#define GET_SETUP   0x90
#define GET_HUODAO   0x8A
#define HUODAO_IND  0x87
#define SALEPRICE_IND 0x8E
#define HUODAO_SET_IND 0x8F
#define PRICE_SET_IND  0x8E
#define VENDOUT_IND   0x83
#define CONTROL_IND   0x85
#define GET_INFO 0x8C
#define GET_INFO_EXP 0x92
#define SET_HUODAO  0x93
#define GET_STATUS  0x86







//VMC��ǰ״̬
#define EV_STATE_DISCONNECT		0    //�Ͽ�����
#define EV_STATE_INITTING		1    //���ڳ�ʼ��
#define EV_STATE_NORMAL			2    //����
#define EV_STATE_FAULT			3    //����
#define EV_STATE_MANTAIN		4    //ά��


#define PC_REQ_IDLE		0
#define PC_REQ_SENDING	1
#define PC_REQ_HANDLING	2







typedef enum{
	EV_NA					=	0x00,
	EV_SETUP_REQ 			= 	GET_SETUP,//��ʼ������ 
	EV_SETUP_RPT 			= 	VMC_SETUP,//��ʼ���������
	EV_INFO_REQ 			=  	GET_INFO,
	EV_INFO_RPT 			=  	INFO_RPT,
	EV_ACK_PC				=  	ACK,   //PC��ӦACK
	EV_NAK_PC   			=  	NAK,  //PC��ӦNAK
	EV_ACK_VM				=  	ACK_RPT,
	EV_NAK_VM				=	NAK_RPT,
	EV_POLL					=	POLL,
	EV_PAYIN_RPT			=	PAYIN_RPT,//Ͷ�ұ���
	EV_COLUMN_REQ			=	GET_HUODAO,//��ȡ����
	EV_COLUMN_RPT			=	HUODAO_RPT,//��������
	EV_TRADE_REQ			=	VENDOUT_IND,
	EV_TRADE_RPT			=	VENDOUT_RPT,
	EV_ACTION_RPT			=	ACTION_RPT,	
	EV_STATE_REQ			=	GET_STATUS,
	EV_STATE_RPT			=	STATUS_RPT,
	EV_BUTTON_RPT			=	BUTTON_RPT,
	EV_CONTROL_REQ			=	CONTROL_IND,
	EV_CONTROL_RPT			=	0xA0,
	EV_ACTION_REQ,
	EV_ENTER_MANTAIN,
	EV_EXIT_MANTAIN,
	EV_RESTART,//VMC����������־
	EV_OFFLINE,//���߱�־
	EV_ONLINE,//���߱�־
	EV_TIMEOUT,//����ʱ
	EV_FAIL  //����ʧ��


}EV_TYPE_REQ;



#define EV_TIMEROUT_VMC  10  //10�볬ʱ
#define EV_TIMEROUT_PC   10  //30�볬ʱ
#define EV_TIMEROUT_PC_LONG   90  //30�볬ʱ


/*********************************************************************************************************
**����ͨ�ú꺯��
*********************************************************************************************************/

#define HUINT16(v)   	(((v) >> 8) & 0x0F)
#define LUINT16(v)   	((v) & 0x0F)
#define INTEG16(h,l)  	(((unsigned int)h << 8) | l)




typedef void (*EV_callBack)(const int,const void *);
int EV_closeSerialPort();
int EV_openSerialPort(char *portName,int baud,int databits,char parity,int stopbits);
int EV_register(EV_callBack callBack);
int EV_release();
int EV_vmMainFlow(const unsigned char type,const unsigned char *data,const unsigned char len);
int32_t	EV_vmRpt(const uint8_t type,const uint8_t *data,const uint8_t len);




void EV_task(int fd);
#endif
