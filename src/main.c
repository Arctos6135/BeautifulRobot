#include <stc/STC12C2052AD.h>
#include "delay.h"
#include "ws2812b.h"
#include "uart.h"
#include <string.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

/*
 * Commands are composed of 16 bits each, with the first 8 bits denoting the action and the second 8 the parameter.
 */
//Display enable/disable
//Param: whether the display is on. 1 - on, 0 - off. Default: on.
#define CMD_ENABLE 		0x01
//Percentage brightness
//Param: an integer in the range 0 - 100, the percentage brightness. Default: 100%.
#define CMD_BRIGHTNESS 	0x02
//Mode
//Param: an integer in the range [], the id of the mode to set to. Default: 0.
//Mode 0: Solid color
//Mode 1: Pulsating color
//Mode 2: Rainbow
#define CMD_MODE 		0x03
//Color
//Param: an integer in the range 0 - 3, the team color. 0 - red, 1 - blue, 2 - green. Default: 2.
#define CMD_COLOR 		0x04

#define LED_COUNT 60

uint8_t brightness = 100;
bit dispOn = 0;
uint8_t mode = 0;
uint8_t color = 2;

xdata volatile RGBColor colors[LED_COUNT] = { 0 };
xdata volatile unsigned short times[LED_COUNT] = { 0 };
void addOverflow(unsigned int*, unsigned int, unsigned int);
void processCmd(uint16_t cmdBuf) {
	uint8_t cmd = cmdBuf >> 8;
	uint8_t param = cmdBuf & 0x00FF;
	
	UART_SendByte(cmd);
	UART_SendByte(param);
	
	//Look at the first byte
	switch(cmd) {
	case CMD_ENABLE:
		dispOn = param;
		break;
	case CMD_BRIGHTNESS:
		brightness = param;
		break;
	case CMD_MODE:
		mode = param;
		if(mode == 1) {
			memset(times, 0, sizeof(unsigned short));
		}
		else if(mode == 2) {
			unsigned short t = 0;
			unsigned int i;
			for(i = 0; i < LED_COUNT; i ++) {
				times[LED_COUNT - 1 - i] = t;

				addOverflow(&t, 0x400, 0xFFFF);
			}
		}
		break;
	case CMD_COLOR:
		color = param;
		break;
	default: break;
	}
}

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
	//Reset timer
	TL0 = 0xD0;
	TH0 = 0x8A;
	//Clear flag
	TF0 = 0;
	
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

#define BRIGHTNESS(x) ((x) * brightness / 100)
void addOverflow(unsigned int *a, unsigned int b, unsigned int max) {
	//a + b > max
	//Doing it like this prevents integer overflow
	if(max - b < *a) {
		//a + b - max
		//a + (b - max)
		//a - (max - b)
		*a -= max - b;	
	}
	else {
		*a += b;
	}
}
uint8_t generate1(unsigned short time) {
	if(time >= 0x8000) {
		return 0xFF - ((time >> 8) - 0x80) * 2;
	}
	else {
		return (time >> 8) * 2;
	}
}
RGBColor colorBuf;
RGBColor generate2(unsigned int time) {
	time /= 42;

	time = time > 0x5FF ? 0x5FF : time;

	if(time < 0x100) {
		colorBuf.R = 0xFF;
		colorBuf.G = time;
		colorBuf.B = 0;
		return colorBuf;
	}
	if(time < 0x200) {
		colorBuf.R = 0xFF - (time - 0x100);
		colorBuf.G = 0xFF;
		colorBuf.B = 0;
		return colorBuf;
	}
	if(time < 0x300) {
		colorBuf.R = 0;
		colorBuf.G = 0xFF;
		colorBuf.B = time - 0x200;	
		return colorBuf;
	}
	if(time < 0x400) {
		colorBuf.R = 0;
		colorBuf.G = 0xFF - (time - 0x300);
		colorBuf.B = 0xFF;
		return colorBuf;
	}
	if(time < 0x500) {
		colorBuf.R = time - 0x400;
		colorBuf.G = 0;
		colorBuf.B = 0xFF;
		return colorBuf;
	}
	colorBuf.R = 0xFF;
	colorBuf.G = 0;
	colorBuf.B = 0xFF - (time - 0x500);
	return colorBuf;
}
void generateColors(void) {
	unsigned char i;
	//Mode 0 - Solid color
	if(mode == 0) {
		for(i = 0; i < LED_COUNT; i ++) {
			colors[i].R = color == 0 ? BRIGHTNESS(0xFF) : 0;
			colors[i].G = color == 2 ? BRIGHTNESS(0xFF) : 0;
			colors[i].B = color == 1 ? BRIGHTNESS(0xFF) : 0;
		}
	}
	//Mode 1 - Pulsating color
	else if(mode == 1) {
		for(i = 0; i < LED_COUNT; i ++) {
			colors[i].R = color == 0 ? BRIGHTNESS(generate1(times[i])) : 0;
			colors[i].G = color == 2 ? BRIGHTNESS(generate1(times[i])) : 0;
			colors[i].B = color == 1 ? BRIGHTNESS(generate1(times[i])) : 0;
		}
	}
	//Mode 3 - Rainbow
	else if(mode == 2) {
		for(i = 0; i < LED_COUNT; i ++) {
			generate2(times[i]);
			colors[i].R = BRIGHTNESS(colorBuf.R);
			colors[i].G = BRIGHTNESS(colorBuf.G);
			colors[i].B = BRIGHTNESS(colorBuf.B);
		}
	}
	
	for(i = 0; i < LED_COUNT; i ++) {
		addOverflow(times + i, mode == 1 | mode == 2 ? 0x100 : 0, 0xFF);
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
	
	while(1) {
		
		generateColors();
		
		//When UART_Buffer > 0xFF, at least one bit in the first byte must be 1. Therefore 2 bytes have been received.
		if(UART_Buffer > 0xFF) {
			processCmd(UART_Buffer);
			UART_Buffer = 0;
		}
	}
}
