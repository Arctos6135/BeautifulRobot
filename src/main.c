#include <stc/STC12C2052AD.h>
#include "delay.h"
#include "ws2812b.h"
#include "uart.h"
#include <stdint.h>

/*
 * Commands are composed of 16 bits each, with the first 8 bits denoting the action and the second 8 the parameter.
 */
//Display enable/disable
//Param: whether the display is on. 1 - on, 0 - off. Default: on.
#define CMD_ENABLE 0x01
//Percentage brightness
//Param: an integer in the range 0 - 100, the percentage brightness. Default: 100%.
#define CMD_BRIGHTNESS 0x02

unsigned int LED_COUNT = 50;

//Command buffer
uint16_t cmdBuf = 0;
//New command flag
bit newCmd = 0;
void UART_InterruptHandler(unsigned char dat) {
	if(cmdBuf) {
		cmdBuf <<= 8;
		cmdBuf |= dat;
		newCmd = 1;
	}
	else {
		cmdBuf = dat;
	}
}

uint8_t brightness = 100;
bit dispOn = 0;

void processCmd() {
	uint8_t cmd = cmdBuf >> 8;
	uint8_t param = cmdBuf & 0x00FF;
	//Clear new command flag
	if(!newCmd)
		return;
	newCmd = 0;
	//Look at the first byte
	switch(cmd) {
	case CMD_ENABLE:
		dispOn = param ? 1 : 0;
		break;
	case CMD_BRIGHTNESS:
		brightness = param;
		break;
	default: break;
	}
}

int main(void) {
	unsigned char i;
	LED_Data = 0;
	delay(200);
	
	UART_Init();
	UART_InterruptInit();
	UART_InterruptCallback = UART_InterruptHandler;
	
	for(i = 0; i < LED_COUNT; i ++) {
		LED_SendColor(0, 0, 0);
	}
	LED_Latch();
	
	while(1) {
	}
}
