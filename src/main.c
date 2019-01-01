#include <stc/STC12C2052AD.h>
#include "delay.h"
#include "ws2812b.h"

sbit status = P1 ^ 1;

void SPICallback(unsigned char dat) {
	LED_SendColor(dat, 0, 0);
	LED_Latch();
	status = !status;
}

int main(void) {
	delay(200);
	
	while(1) {
		
	}
}
