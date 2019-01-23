#include "delay.h"
#include "uart.h"
#include "ws2812b.h"
#include <stc/STC12C2052AD.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

// Thanks @Swordfish
#define FASTABS(c) (c ^ -(c < 0)) + (c < 0)

/*
 * Commands are composed of 16 bits each, with the first 8 bits denoting the
 * action and the second 8 the parameter. When they're first received, they also
 * contain a sync byte (see the main function).
 *
 * Please note that there is no error checking to make sure the parameters are
 * correct. Thus, if the incorrect parameter was sent, the behavior is
 * undefined.
 */
//	Display enable/disable
//	Param: an integer in the range 0 - 1, whether the display is on. 1 - on,
// 0 -
// off. Default: on.
#define CMD_ENABLE 0x01
//	Percentage brightness
//	Param: an integer in the range 0 - 100, the percentage brightness.
// Default:
// 100%.
#define CMD_BRIGHTNESS 0x02
//	Mode
//	Param: an integer in the range [], the id of the mode to set to.
// Default: 0. 	Mode 0: Solid 	Mode 1: Pulsating 	Mode 2: Rainbow 	Mode 3:
// Moving Pulse 	Mode 4: Status bar/Meter
#define CMD_MODE 0x03
//	Color
//	Param: an integer in the range 0 - 3, the team color. 0 - red, 1 - blue,
// 2 -
// green. Default: 2.
#define CMD_COLOR 0x04
//	Direction of pulse
//	Param: an integer in the range 0 - 1, the "direction" of the pulse.
// Default:
// 0 0 - The pulse appears to move away from the microcontroller and following
// the direction of the strip. 1 - The pulse appears to move towards the
// microcontroller, going against the direction of the strip.
#define CMD_DIRECTION 0x05
//	The number of LEDs
//	Param: an integer in the range 0 - 80, the number of LEDs in the strip.
// Default: 80. Note: This number is not intended to be changed normally. It
// should, ideally, be changed only when the LEDs are off, as changing the
// number from higher to lower does not turn off any LEDs.
#define CMD_COUNT 0x06
//	The speed of the patterns
//	Param: an integer in the range 0 - 255. The total speed is made of a
// high
// byte and a low byte. CMD_SPEEDHIGH changes the high byte, while CMD_SPEEDLOW
// changes the low byte.
#define CMD_SPEEDHIGH 0x07
#define CMD_SPEEDLOW 0x08
//	Resets the BeautifulRobot. All properties (e.g. brightness, color) will
// be
// reset back to the default. Param: none
#define CMD_RESET 0x09
// Set the value of the register with least significant and most significant
// bits
#define REG_SETVALUE_MSB 0x0A
#define REG_SETVALUE_LSB 0x0B

unsigned char LED_COUNT = 80;

volatile uint8_t brightness = 100;
volatile bit dispOn = 0;
volatile uint8_t mode = 0;
volatile uint8_t color = 2;
volatile bit direction = 0;
volatile uint8_t speedHigh = 0x01;
volatile uint8_t speedLow = 0x00;

volatile uint16_t reg = 0;

xdata volatile RGBColor colors[80] = {0};
xdata volatile unsigned short time = 0;
void processCmd(uint16_t cmdBuf) {
	uint8_t cmd = cmdBuf >> 8;
	uint8_t param = cmdBuf & 0x00FF;

	UART_SendByte(cmd);
	UART_SendByte(param);

	//	Look at the first byte
	switch (cmd) {
	case CMD_ENABLE:
		dispOn = param;
		break;
	case CMD_BRIGHTNESS:
		brightness = param;
		break;
	case CMD_MODE:
		mode = param;
		time = 0;
		break;
	case CMD_COLOR:
		color = param;
		break;
	case CMD_DIRECTION:
		direction = param;
		break;
	case CMD_COUNT:
		LED_COUNT = param;
		break;
	case CMD_SPEEDHIGH:
		speedHigh = param;
		break;
	case CMD_SPEEDLOW:
		speedLow = param;
		break;
	case REG_SETVALUE_LSB:
		reg = reg & 0xff00 | param;
		break;
	case REG_SETVALUE_MSB:
		reg = reg & 0x00ff | param;
		break;
	case CMD_RESET:
		//	Cast 0x0000 into a function pointer and call it to reset everything
		((void(code *)(void)) 0x0000)();
		break;
	default:
		break;
	}
}

void Timer0Init(void) { //	30ms@12.000MHz
	//	Enable interrupts
	EA = 1;
	ET0 = 1;
	AUXR &= 0x7F; //	Timer clock is 12T mode
	TMOD &= 0xF0; //	Set timer work mode
	TMOD |= 0x01; //	Set timer work mode
	TL0 = 0xD0;   //	Initial timer value
	TH0 = 0x8A;   //	Initial timer value
	TF0 = 0;      //	Clear TF0 flag
	TR0 = 1;      //	Timer0 start run
}
void Timer0Routine(void) interrupt 1 {
	unsigned char i;
	//	Reset timer
	TL0 = 0xD0;
	TH0 = 0x8A;
	//	Clear flag
	TF0 = 0;

	if (dispOn) {
		LED_SendRGBData(colors, LED_COUNT);
	} else {
		for (i = 0; i < LED_COUNT; i++) {
			LED_SendColor(0, 0, 0);
		}
		LED_Latch();
	}
}

#define BRIGHTNESS(x) ((x) *brightness / 100)
uint8_t generate1(unsigned short time) {
	if (time >= 0x8000) {
		return 0xFF - ((time >> 8) - 0x80) * 2;
	} else {
		return (time >> 8) * 2;
	}
}
RGBColor colorBuf;
RGBColor generate2(unsigned int time) {
	time /= 42;

	time = time > 0x5FF ? 0x5FF : time;

	if (time < 0x100) {
		colorBuf.R = 0xFF;
		colorBuf.G = time;
		colorBuf.B = 0;
		return colorBuf;
	}
	if (time < 0x200) {
		colorBuf.R = 0xFF - (time - 0x100);
		colorBuf.G = 0xFF;
		colorBuf.B = 0;
		return colorBuf;
	}
	if (time < 0x300) {
		colorBuf.R = 0;
		colorBuf.G = 0xFF;
		colorBuf.B = time - 0x200;
		return colorBuf;
	}
	if (time < 0x400) {
		colorBuf.R = 0;
		colorBuf.G = 0xFF - (time - 0x300);
		colorBuf.B = 0xFF;
		return colorBuf;
	}
	if (time < 0x500) {
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
uint8_t generate3(unsigned short time) {
	if (time >= 0xC000) {
		return 0xFF - ((time >> 8) - 0xC0) * 4;
	} else if (time >= 0x8000) {
		return ((time >> 8) - 0x80) * 4;
	} else {
		return 0;
	}
}

// Draws a point that fades to 0 at distance. It gets cut of by the ends
void drawPoint(unsigned char position, unsigned char distance) {
	unsigned char dropoff = 0xFF / distance;
	if (i > 0) {
		// I know it's beautiful
		for (char i = position - distance > position ? 0 : position - distance;
		     i < (position + distance < position ? LED_COUNT : position + distance); i++) {
			colors[i].R = color == 0 ? BRIGHTNESS(255 - dropoff * FASTABS(position - i)) : 0;
			colors[i].G = color == 2 ? BRIGHTNESS(255 - dropoff * FASTABS(position - i)) : 0;
			colors[i].B = color == 1 ? BRIGHTNESS(255 - dropoff * FASTABS(position - i)) : 0;
		}
	}
}

void generateColors(void) {
	unsigned char i;
	unsigned short t = time;
	//	Mode 0 - Solid
	if (mode == 0) {
		for (i = 0; i < LED_COUNT; i++) {
			colors[i].R = color == 0 ? BRIGHTNESS(0xFF) : 0;
			colors[i].G = color == 2 ? BRIGHTNESS(0xFF) : 0;
			colors[i].B = color == 1 ? BRIGHTNESS(0xFF) : 0;
		}
	}
	//	Mode 1 - Pulsating
	else if (mode == 1) {
		for (i = 0; i < LED_COUNT; i++) {
			colors[i].R = color == 0 ? BRIGHTNESS(generate1(t)) : 0;
			colors[i].G = color == 2 ? BRIGHTNESS(generate1(t)) : 0;
			colors[i].B = color == 1 ? BRIGHTNESS(generate1(t)) : 0;
		}
	}
	//	Mode 2 - Rainbow
	else if (mode == 2) {
		for (i = 0; i < LED_COUNT; i++) {
			generate2(t);
			colors[i].R = BRIGHTNESS(colorBuf.R);
			colors[i].G = BRIGHTNESS(colorBuf.G);
			colors[i].B = BRIGHTNESS(colorBuf.B);

			/*
			 * The "time" of each LED is slightly shifted to give the impression of a
			 * pulse, when in reality all LEDs are doing the same thing.
			 *
			 * Depending on the direction, a certain amount is added to or subtracted
			 * from the time of the previous LED to get the time of the next LED.
			 *
			 * When the direction is forwards, the amount is subtracted. This means
			 * that LEDs closer to the beginning of the strip have higher times, which
			 * means that the pulse will be experienced by LEDs closer to the
			 * beginning first, and then propagate down.
			 *
			 * When the direction is backwards, the amount is added. This has the
			 * opposite effect of subtracting the value.
			 */
			//	Ignore overflow/underflow
			//	According to the C standard, unsigned integer overflow is
			//	defined
			// behavior.
			t += direction ? 0x400 : -0x400;
		}
	}
	//	Mode 3 - Moving Pulse
	else if (mode == 3) {
		for (i = 0; i < LED_COUNT; i++) {
			colors[i].R = color == 0 ? BRIGHTNESS(generate3(t)) : 0;
			colors[i].G = color == 2 ? BRIGHTNESS(generate3(t)) : 0;
			colors[i].B = color == 1 ? BRIGHTNESS(generate3(t)) : 0;

			//	Ignore overflow/underflow
			//	According to the C standard, unsigned integer overflow is
			//	defined
			// behavior.
			t += direction ? 0x800 : -0x800;
		}
	}
	//	Mode 4 - Progress bar
	else if (mode == 4) {
		char ledsToLight = LED_COUNT / (reg >> 8);
		// Light all the LEDS up to that point using the normal brightness
		for (i = 0; i < ledsToLight; i++) {
			colors[i].R = color == 0 ? BRIGHTNESS(0xFF) : 0;
			colors[i].G = color == 2 ? BRIGHTNESS(0xFF) : 0;
			colors[i].B = color == 1 ? BRIGHTNESS(0xFF) : 0;
		}
		// Light the last LED up based on the progress
		colors[i].G = color == 2 ? BRIGHTNESS(reg & 0x00ff) : 0;
		colors[i].B = color == 1 ? BRIGHTNESS(reg & 0x00ff) : 0;
		colors[i].R = color == 0 ? BRIGHTNESS(reg & 0x00ff) : 0;
	}
	//	Ignore overflow
	//	According to the C standard, unsigned integer overflow is defined
	//	behavior.
	time += (speedHigh << 8) | speedLow;
}

int main(void) {
	unsigned char i;
	LED_Data = 0;
	delay(200);

	UART_Init();
	UART_InterruptInit();

	for (i = 0; i < LED_COUNT; i++) {
		LED_SendColor(0, 0, 0);
	}
	LED_Latch();

	Timer0Init();

	while (1) {

		generateColors();

		/*
		 * Check if enough bytes have been received for a valid command.
		 *
		 * All commands consist of 3 bytes:
		 * - The operation
		 * - The parameter
		 * - The sync byte (0xFF)
		 *
		 * The sync byte is used to make sure that if some bytes of a command were
		 * missed, the following commands would not be messed up due to the first
		 * command still being in the buffer.
		 *
		 * There has to be at least 3 nonzero bytes in the buffer, hence the
		 * buffer's value must be greater than 0xFFFF. Additionally, the buffer also
		 * has to end with the sync byte, which has a value of 0xFF. Due to the
		 * actual size of the buffer being 4 bytes, the leftmost byte in the buffer
		 * is ignored.
		 *
		 * Concept inspired by @mincrmatt12
		 */
		if ((UART_Buffer & 0x00FFFFFF) > 0xFFFF && (UART_Buffer & 0x000000FF) == 0xFF) {
			//	Right-shift 8 since we don't need the sync byte
			processCmd((UART_Buffer >> 8) & 0xFFFF);
			UART_Buffer = 0;
		}
	}
}
