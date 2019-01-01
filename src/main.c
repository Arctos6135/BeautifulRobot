#include <stc/STC12C2052AD.h>
#include "delay.h"
#include "ws2812b.h"
#include "spi.h"

#define LED_COUNT 30

sbit status = P1 ^ 1;

void SPICallback(unsigned char dat) {
	LED_SendColor(dat, 0, 0);
	LED_Latch();
	status = !status;
}

int main(void) {
	unsigned char i;
	LED_Data = 0;
	delay(200);
	
	SPI_SlaveInit();
	SPI_InterruptInit();
	SPI_InterruptCallback = SPICallback;
	
	for(i = 0; i < LED_COUNT; i ++) {
		LED_SendColor(0, 0, 0);
	}
	LED_Latch();
	
	while(1) {
		
	}
}
