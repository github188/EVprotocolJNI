#include "smart210_uart.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <termios.h>


static struct timeval timeout={0,1};

int baud_arr[] = {115200,57600, 38400,  19200, 9600,  4800,  2400,  1200,  300,
				  38400,  19200,  9600, 4800, 2400,1200,  300, };


static int uart_fd = -1;

/*********************************************************************************************************
** Function name:     	uart_open
** Descriptions:	    ���ڴ򿪺���
** input parameters:    port ���ں�
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
int uart_open(char *port)
{
	uart_fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);         //| O_NOCTTY 
	if (uart_fd < 0)
	{ 
		printf("Can't Open Serial Port:%s\n",port);
		return (-1);
	}	
	return uart_fd;
}



/*********************************************************************************************************
** Function name:     	uart_setParity
** Descriptions:	    �������ò���
** input parameters:    port ���ں�
** output parameters:   
** Returned value:      1:���óɹ� 0����ʧ��
*********************************************************************************************************/
int uart_setParity(int databits,int stopbits,char parity)
{
	if(uart_fd < 0) return 0;
	struct termios options;
 	if ( tcgetattr( uart_fd,&options)  !=  0) {
  		perror("SetupSerial 1");
  		return(0);
	}
  	options.c_cflag &= ~CSIZE;
  	switch (databits){
  		case 7:
  			options.c_cflag |= CS7;
  			break;
  		case 8:
			options.c_cflag |= CS8;
			break;
		default:
			fprintf(stderr,"Unsupported data size\n");
			return (0);
	}
 	switch (parity){
  		case 'n':
		case 'N':
			options.c_cflag &= ~PARENB;   /* Clear parity enable */
			options.c_iflag &= ~INPCK;     /* Enable parity checking */
			options.c_iflag &= ~(ICRNL|IGNCR);
			options.c_lflag &= ~(ICANON );
			break;
		case 'o':
		case 'O':
			options.c_cflag |= (PARODD | PARENB);  /* ����Ϊ��Ч��*/ 
			options.c_iflag |= INPCK;             /* Disnable parity checking */
			break;
		case 'e':
		case 'E':
			options.c_cflag |= PARENB;     /* Enable parity */
			options.c_cflag &= ~PARODD;   /* ת��ΪżЧ��*/  
			options.c_iflag |= INPCK;       /* Disnable parity checking */
			break;
		case 'S':
		case 's':  /*as no parity*/
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
			break;
		default:
			fprintf(stderr,"Unsupported parity\n");
			return (0);
	}
  /* ����ֹͣλ*/   
  	switch (stopbits){
  		case 1:
  			options.c_cflag &= ~CSTOPB;
			break;
		case 2:
			options.c_cflag |= CSTOPB;
			break;
		default:
			fprintf(stderr,"Unsupported stop bits\n");
			return (0);
	}
  /* Set input parity option */
  	if (parity != 'n'){
  		options.c_iflag |= INPCK;
  	}
    	options.c_cc[VTIME] = 0; // 50ms��ʱʱ��
    	options.c_cc[VMIN] = 0;   //��С��ȡλ

	tcflush(uart_fd,TCIOFLUSH); /* Update the options and do it NOW */
 	if (tcsetattr(uart_fd,TCSANOW,&options) != 0){
  		perror("SetupSerial 3");
		return (0);
	}
  	return (1);
}


/*********************************************************************************************************
** Function name:     	uart_setBaud
** Descriptions:	    �������ò�����
** input parameters:    port ���ں�
** output parameters:   baud ������
** Returned value:      1:���óɹ� 0����ʧ��
*********************************************************************************************************/

int uart_setBaud(int baud)
{
  	int   i;
  	int   status;
  	struct termios   Opt;
	if(uart_fd < 0) return 0;
  	tcgetattr(uart_fd, &Opt);
  	for (i = 0;  i < sizeof(baud_arr) / sizeof(int);  i++){
   		if(baud_arr[i] == baud)
		{
			tcflush(uart_fd, TCIOFLUSH);
    		cfsetispeed(&Opt, baud_arr[i]);
    		cfsetospeed(&Opt, baud_arr[i]);
    		status = tcsetattr(uart_fd, TCSANOW, &Opt);
    		if (status != 0)
			{
				perror("tcsetattr fd1");
    		}
			return 1;
     	}
   		tcflush(uart_fd,TCIOFLUSH);
   	}
	return 0;
}


/*********************************************************************************************************
** Function name:     	uart_close
** Descriptions:	    ���ڹرպ���
** input parameters:    
** output parameters:   
** Returned value:      1:���óɹ� 0����ʧ��
*********************************************************************************************************/

int uart_close()
{
	if(uart_fd < 0) return 1;
	close(uart_fd);
	uart_fd = -1;
	return 1;
}

/*********************************************************************************************************
** Function name:     	uart_write
** Descriptions:	    ���ڷ�������
** input parameters:    port ���ں�
** output parameters:   buf ����ָ�� len���ͳ���
** Returned value:      1:���óɹ� 0����ʧ��
*********************************************************************************************************/
int uart_write(char *buf,int len)
{
	int ret ;
	if(uart_fd < 0) return 0;

	uart_clear();
	ret = write(uart_fd , buf , len);  //д����
	if(ret <0)
	{
		perror("write serial");
		return 0;
	}
	return  ret;
}



/*********************************************************************************************************
** Function name:     	uart_isNotEmpty
** Descriptions:	    �����Ƿ������ݿɶ�
** input parameters:    
** output parameters:   
** Returned value:      1�ɶ� 0������
*********************************************************************************************************/
int uart_isNotEmpty()
{
	int ret;
	if(uart_fd < 0) return 0;
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(uart_fd,&readfds);
	
	ret = select(uart_fd + 1,&readfds,NULL,NULL,&timeout);
	if(ret < 0) return 0;
	else
	{
		if(FD_ISSET(uart_fd,&readfds))
			return 1;
	}
	return 0;
}

/*********************************************************************************************************
** Function name:     	uart_read
** Descriptions:	    �������ݶ�ȡ
** input parameters:    buf Ҫ��ȡ������ָ��  len Ҫ��ȡ�ó���
** output parameters:   
** Returned value:      1�ɶ� 0������
*********************************************************************************************************/

int uart_read(char *buf,int len)
{
	int ret ;
	if(len <= 0) return 0;
	ret = read(uart_fd , buf , len); //��ȡ����
	if(ret<0){
		perror("read");		
		return 0;
	}
	return ret;
}





/*********************************************************************************************************
** Function name:     	uart_clear
** Descriptions:	    ��մ��ڻ�����
** input parameters:    
** output parameters:   
** Returned value:      
*********************************************************************************************************/

void uart_clear()
{
	if(uart_fd < 0) return;
	if(tcflush(uart_fd,TCIOFLUSH)<0)//��մ��ڻ�����
	{  
		fprintf(stderr,"flush error\n");
	}

	
}
