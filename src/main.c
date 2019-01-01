#include <stc/STC12C2052AD.h>
#include "delay.h"
#include "ws2812b.h"
#include "uart.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

/*
 * Commands are composed of 16 bits each, with the first 8 bits denoting the action and the second 8 the parameter.
 */
//Display enable/disable
//Param: whether the display is on. 1 - on, 0 - off. Default: on.
#define CMD_ENABLE 0x01
//Percentage brightness
//Param: an integer in the range 0 - 100, the percentage brightness. Default: 100%.
#define CMD_BRIGHTNESS 0x02

#define LED_COUNT 50

uint8_t brightness = 100;
bit dispOn = 0;

void processCmd(uint16_t cmdBuf) {
	uint8_t cmd = cmdBuf >> 8;
	uint8_t param = cmdBuf & 0x00FF;
	
	//UART_SendByte(cmd);
	//UART_SendByte(param);
	
	//Look at the first byte
	switch(cmd) {
	case CMD_ENABLE:
		dispOn = param;
		break;
	case CMD_BRIGHTNESS:
		brightness = param;
		break;
	default: break;
	}
}

xdata RGBColor colors[LED_COUNT] = { 0 };
void Timer0Init(void) { //30ms@12.000MHz
	//Enable interrupts
	EA = 1;
	ET0 = 1;
	AUXR &= 0x7F;		//Timer clock is 12T mode
	TMOD &= 0xF0;		//Set timer work mode
	TMOD |= 0x01;		//Set timer work mode
	TL0 = 0xD0;		//Initial timer value
	TH0 = 0x8A;		//Initial timer value
	TF0 = 0;		//Clear TF0 flag
	TR0 = 1;		//Timer0 start run
}
void Timer0Routine(void) interrupt 1 {
	unsigned char i;
	//Clear flag
	TF0 = 0;
	//Reset timer
	TL0 = 0xD0;
	TH0 = 0x8A;
	
	if(dispOn) {
		LED_SendRGBData(colors, LED_COUNT);
	}
	else {
		for(i = 0; i < LED_COUNT; i ++) {
			LED_SendColor(0, 0, 0);
		}
		LED_Latch();
	}
}



int main(void) {
	unsigned char i;
	LED_Data = 0;
	delay(200);
	
	UART_Init();
	UART_InterruptInit();
	
	for(i = 0; i < LED_COUNT; i ++) {
		LED_SendColor(0, 0, 0);
	}
	LED_Latch();
	
	Timer0Init();
	
	colors[0].R = 0x80;
	colors[0].B = 0x80;
	colors[1].G = 0x80;
	
	while(1) {
		
		//When UART_Buffer > 0xFF, at least one bit in the first byte must be 1. Therefore 2 bytes have been received.
		if(UART_Buffer > 0xFF) {
			processCmd(UART_Buffer);
			UART_Buffer = 0;
		}
	}
}
