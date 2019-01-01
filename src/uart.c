#include "uart.h"
#include <stc/STC12C2052AD.h>

void UART_Init() {
	PCON &= 0x7F;		//Baudrate no doubled
	SCON = 0x50;		//8bit and variable baudrate
	AUXR |= 0x40;		//Timer1's clock is Fosc (1T)
	AUXR &= 0xFE;		//Use Timer1 as baudrate generator
	TMOD &= 0x0F;		//Mask Timer1 mode bit
	TMOD |= 0x20;		//Set Timer1 as 8-bit auto reload mode
	TL1 = 0xD9;		//Initial timer value
	TH1 = 0xD9;		//Set reload value
	ET1 = 0;		//Disable Timer1 interrupt
	TR1 = 1;		//Timer1 running
}

void UART_InterruptInit() {
	EA = 1;
	ES = 1;
}

void (*UART_InterruptCallback) (unsigned char) = NULL;

void UART_InterruptRoutine() interrupt 4 using 1 {
	//Clear receive flag
	RI = 0;
	
	if(UART_InterruptCallback) {
		UART_InterruptCallback(SBUF);
	}
}

void UART_SendByte(unsigned char dat) {
	SBUF = dat;
	
	while(!TI);
	TI = 0;
}
