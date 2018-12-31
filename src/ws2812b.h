#ifndef __WS2812B_H__
#define __WS2812B_H__

typedef struct _RGBColor {
	unsigned char R;
	unsigned char G;
	unsigned char B;

} RGBColor;

void LED__SendZero__(void);
void LED__SendOne__(void);
void LED__SendByte__(unsigned char);
void LED_Latch(void);

void LED_SendRGBColor(RGBColor*);
void LED_SendColor(unsigned char, unsigned char, unsigned char);
void LED_SendRGBData(RGBColor*, unsigned short);

#endif
