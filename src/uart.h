#ifndef __UART_H__
#define __UART_H__

#ifndef NULL
	#define NULL ((void*) 0)
#endif

void UART_Init(void);
void UART_InitInterrupt(void);

extern void (*UART_InterruptCallback) (unsigned char);

#endif
