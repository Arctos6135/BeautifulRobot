#ifndef __SPI_H__
#define __SPI_H__

#ifndef NULL
	#define NULL ((void*) 0)
#endif

void SPI_SlaveInit(void);
void SPI_InterruptInit(void);

extern void (*SPI_InterruptCallback) (unsigned char);

#endif
