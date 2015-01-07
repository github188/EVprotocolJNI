#ifndef _SMART210_UART_H_
#define _SMART210_UART_H_

int uart_open(char *port);
void uart_clear(int fd);
int uart_read(int fd,char *buf,int len);
int uart_write(int fd,char *buf,int len);
int uart_close(int fd);
int uart_setBaud(int fd,int baud);
int uart_setParity(int fd,int databits,int stopbits,char parity);
int uart_isNotEmpty(int fd);
#endif