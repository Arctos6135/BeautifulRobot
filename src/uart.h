#ifndef __UART_H__
#define __UART_H__

#ifndef NULL
	#define NULL ((void*) 0)
#endif

void UART_Init(void);
void UART_InterruptInit(void);
	
void UART_SendByte(unsigned char);

extern unsigned long UART_Buffer;

#endif
