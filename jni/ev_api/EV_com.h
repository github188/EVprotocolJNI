#ifndef _EV_COM_H_
#define _EV_COM_H_

#define HEAD_EF     0xE5
#define HEAD_LEN    5
#define VER_F0_1    0x41
#define VER_F0_0    0x40




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







//VMC当前状态
#define EV_DISCONNECT	0    //离线
#define EV_INITTING		1    //正在初始化
#define EV_NORMAL		2    //正常
#define EV_FAULT		3    //故障
#define EV_MANTAIN		4    //维护


#define PC_REQ_IDLE		0
#define PC_REQ_SENDING	1
#define PC_REQ_HANDLING	2




typedef enum{
	EV_SETUP_REQ,//初始化请求 
	EV_SETUP_RPT,//初始化结果返回
	EV_INFO_REQ,
	EV_INFO_RPT,
	EV_CONTROL_REQ,
	EV_CONTROL_RPT,
	EV_TRADE_REQ,
	EV_TRADE_RPT,
	EV_STATE_REQ,
	EV_STATE_RPT,
	EV_ACTION_REQ,
	EV_ACTION_RPT,
	EV_ENTER_MANTAIN,
	EV_EXIT_MANTAIN,
	EV_RESTART,
	EV_INITING,//初始化标志
	EV_OFFLINE,//离线标志
	EV_ONLINE,//在线标志
	EV_NAK,
	EV_TIMEOUT//请求超时


}EV_TYPE_REQ;



#define EV_TIMEROUT_VMC  10  //10秒超时
#define EV_TIMEROUT_PC   10  //30秒超时
#define EV_TIMEROUT_PC_LONG   90  //30秒超时



typedef void (*EV_callBack)(const int,const void *);
int EV_closeSerialPort();
int EV_openSerialPort(char *portName,int baud,int databits,char parity,int stopbits);
int EV_register(EV_callBack callBack);
int EV_release();

int EV_initFlow(const unsigned char type,const unsigned char *data,
		const unsigned char len);
int EV_mainFlow(const unsigned char type,const unsigned char *data,
		const unsigned char len);
int32_t	EV_vmcRpt(const uint8_t type,const uint8_t *data,const uint8_t len);
void EV_setVmcState(const uint8_t type)	;



void EV_task(int fd);
#endif
