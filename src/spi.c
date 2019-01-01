#include "spi.h"
#include <stc/STC12C2052AD.h>

void SPI_SlaveInit(void) {
	//SSIG = 0 - Don't ignore SS (Single master, multi slave mode)
	//SPEN = 1 - Enable SPI
	//DORD = 0 - MSB first
	//MSTR = 0 - Slave mode
	
	//CPOL = 1 - Clock idle high (Active low)
	//CPHA = 0 - Sample on clock leading edge
	//SPR1 = 0 - Speed: CPU_CLK / 4
	//SPR0 = 0 - Speed: CPU_CLK / 4
	SPCTL = 0x48; //0100 1000
}

void SPI_InterruptInit(void) {
	//Master interrupt enable
	EA = 1;
	//ADC/SPI interrupt enable
	EADC_SPI = 1;
}

void SPI_InterruptRoutine(void) interrupt 5 {
	//Clear status registers
	//Note: the bits are cleared when 1 is written.
	SPSTAT = 0xC0; //1100 0000
	//Call the callback
	if(SPI_InterruptCallback) {
		SPI_InterruptCallback(SPDAT);
	}
}
