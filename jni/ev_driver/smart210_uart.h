#ifndef _SMART210_UART_H_
#define _SMART210_UART_H_

int uart_open(char *port);
void uart_clear();
int uart_read(char *buf,int len);
int uart_write(char *buf,int len);
int uart_close();
int uart_setBaud(int baud);
int uart_setParity(int databits,int stopbits,char parity);
int uart_isNotEmpty();
#endif