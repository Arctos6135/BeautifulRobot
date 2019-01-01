#include "ws2812b.h"
#include <intrins.h>

#ifndef MAX
	#define MAX(a, b) ((a < b) ? b : a)
#endif
#ifndef MIN
	#define MIN(a, b) ((a > b) ? b : a)
#endif

/*
 * Assumes a 12MHz clock. 
 */

void LED__SendZero_(void) {
	LED_Data = 1;
	_nop_();
	LED_Data = 0;
	_nop_();
	_nop_();
}
void LED__SendOne_(void) {
	LED_Data = 1;
	_nop_();
	_nop_();
	LED_Data = 0;
	_nop_();
}
void LED_Latch(void) {
	short a = 10000;
	LED_Data = 0;
	//Each loop should produce 3 instructions: decrement, comparison, and jmp. 
	//At least 600 instructions are needed for 50us.
	while(a--);
}
void LED__SendByte_(unsigned char dat) {
	if(dat & 0x80) LED__SendOne_(); else LED__SendZero_();   
	if(dat & 0x40) LED__SendOne_(); else LED__SendZero_();   
	if(dat & 0x20) LED__SendOne_(); else LED__SendZero_();   
	if(dat & 0x10) LED__SendOne_(); else LED__SendZero_();   
	if(dat & 0x08) LED__SendOne_(); else LED__SendZero_();   
	if(dat & 0x04) LED__SendOne_(); else LED__SendZero_();   
	if(dat & 0x02) LED__SendOne_(); else LED__SendZero_();   
	if(dat & 0x01) LED__SendOne_(); else LED__SendZero_();   
}
void LED_SendRGBColor(RGBColor *color) {
	LED__SendByte_((*color).G);
   	LED__SendByte_((*color).R);
	LED__SendByte_((*color).B);
}
void LED_SendColor(unsigned char R, unsigned char G, unsigned char B) {
	LED__SendByte_(G);
	LED__SendByte_(R);
	LED__SendByte_(B);
}
void LED_SendRGBData(RGBColor *colors, unsigned short count) {
	unsigned short i;
	for(i = 0; i < count; i ++) {
		LED_SendRGBColor(&colors[i]);
	}
	LED_Latch();
}
