#include "delay.h"

/*
	Delay milliseconds function
*/
void delay (unsigned int a){
	unsigned int i;
	while( --a != 0){
		for(i = 0; i < 600; i++);
	}
}
void delay2(unsigned int a) {
	while(a--);
}
