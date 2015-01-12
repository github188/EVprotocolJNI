#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h> //定时器
#include <string.h>
#include <fcntl.h>
#include "EV_bento.h"
#include "../ev_driver/smart210_uart.h"
#include "EV_timer.h"
#include "../ev_driver/ev_config.h"




static int bento_fd = -1;

int EV_bento_openSerial(char *portName,int baud,int databits,char parity,int stopbits)
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
	bento_fd = fd;

	EV_LOGI4("EV_openSerialPort:Serial[%s] open suc..fd = %d\n",portName,fd);
	return fd;
}

int EV_bento_closeSerial(int fd)
{
	EV_LOGI4("EV_closeSerialPort:closed...\n");
	uart_close(fd);
}


unsigned char EV_bento_recv(unsigned char *rdata,unsigned char *rlen)
{
	unsigned char timeout = 100,buf[10]= {0},len = 0,temp,startFlag = 0;
	unsigned short crc;
	*rlen = 0;
	while(timeout--)
	{
		if(uart_read(bento_fd,&temp,1) > 0)
		{
			if(temp == EV_BENTO_HEAD + 1)
			{
				startFlag = 1;
				
			}
			if(startFlag == 1)
			{
				buf[len++] = temp;
				if(len >= (buf[1] + 2))
				{
					crc = EV_crcCheck(buf,6);
					if(crc == INTEG16(buf[6],buf[7]))
					{
						if(rdata != NULL)
							memcpy(rdata,buf,8);
						*rlen = 8;
						return 1;
					}
					else
						return 0;
					
				}
			}
			else
				EV_msleep(2);
		}
		else
			EV_msleep(2);
	}
	return 0;
	
}


int EV_bento_send(unsigned char cmd,unsigned char cabinet,unsigned char arg,unsigned char *data)
{
	unsigned char buf[20] = {0},len = 0,ret,rbuf[20] = {0};
	unsigned short crc;
	buf[len++] = EV_BENTO_HEAD;
	buf[len++] = 0x07;
	buf[len++] = cabinet - 1;
	buf[len++] = cmd;
	buf[len++] = cabinet - 1;
	buf[len++] = cabinet - 1;//0x08;
	buf[len++] = arg;//0x00;	
	crc = EV_crcCheck(buf,len);
	buf[len++] = HUINT16(crc);
	buf[len++] = LUINT16(crc);
	uart_clear(bento_fd);
	uart_write(bento_fd,buf,len);
	
	ret = EV_bento_recv(rbuf,&len);
	
	if(ret == 1)
	{
		if(cmd == EV_BENTO_TYPE_OPEN)
		{
			if(rbuf[3] == EV_BENTO_TYPE_OPEN_ACK)
			{
				return 1;//开门成功
			}
		}
		else if(cmd == EV_BENTO_TYPE_CHECK)
		{
			if(rbuf[3] == EV_BENTO_TYPE_CHECK_ACK)
			{
				if(data != NULL)
					memcpy(data,rbuf,rbuf[1]);
				return 1;
			}
		}
		else if(cmd == EV_BENTO_TYPE_LIGHT)
		{
			if(rbuf[3] == EV_BENTO_TYPE_LIGHT_ACK)
				return 1;
			else
				return 0;
		}
	}
	return 0;
	
}


int EV_bento_open(int cabinet,int box)
{
	int ret = 0;
	if(cabinet <= 0 || box <= 0)
		return 0;
	
	ret = EV_bento_send(EV_BENTO_TYPE_OPEN,cabinet&0xFF,box &0xFF,NULL);
	return ret;
}


int EV_bento_light(int cabinet,unsigned char flag)
{
	int ret = 0;
	if(cabinet <= 0)
		return 0;
	ret = EV_bento_send(EV_BENTO_TYPE_LIGHT,cabinet&0xFF,flag &0xFF,NULL);
	return ret;
}


int EV_bento_check(int cabinet,ST_BENTO_FEATURE *st_bento)
{
	int ret = 0;
	unsigned char buf[20] = {0},i;
	if(st_bento == NULL) return 0;
	if(cabinet <= 0)
		return 0;

	ret = EV_bento_send(EV_BENTO_TYPE_CHECK,cabinet&0xFF,0x00,buf);
	if(ret == 1)
	{
		st_bento->boxNum = buf[6];
		st_bento->ishot = (buf[8] & 0x01);
		st_bento->iscool = ((buf[8] >> 1) & 0x01);
		st_bento->islight = ((buf[8] >> 2) & 0x01);
		for(i = 0;i < 7;i++)
		{
			st_bento->id[i] = buf[9 + i];
		}
		return 1;
		
	}
	return ret;
}









