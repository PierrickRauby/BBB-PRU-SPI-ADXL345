/*
 * Source Modified by Pierrick Rauby <PierrickRauby - pierrick.rauby@gmail.com>
*/
#include <stdint.h>
#include <stdio.h>
#include <rsc_types.h>
#include <stdlib.h>
#include <spi.h> // spi.h is in ../include/spi

#define TWAIT 100000000 // long delay for stabilization
#define TDELAY 2 // delay for tdelay=2*5ns > 5ns=tdelay_min
#define TSETUP 2 // delay for tsetup=2*5ns > 5ns=tsetup_min
#define THOLD 2 // delay for thold=2*5ns > 5ns=thold_min
#define TSCLK 45 // delay for tsclk=45*5ns > 200ns=tsclk_min
#define TQUIET 2 // delay for tquiet=2*5ns > 5ns=tquiet_min
#define TCSDIS 35 // delay for tcsdis=35*5ns > 150ns=tcsdis_min


void spiWrite(uint8_t Register, uint8_t Value){
	int i;
	uint16_t Transfer=((((uint16_t)(Register & 0x7F))<<8)+Value);
	//initialization
	__R30 |= (1<<5); //CS -> HIGH //Maybe these lines can be deleted ?
	__R30 |= (1<<2); //Clock -> HIGH
	__delay_cycles(TWAIT);
	__R30 &= 0xFFFFFFDF; //CS -> LOW
	__delay_cycles(TDELAY);
	__R30 &= 0xFFFFFFFB; //Clock -> LOW
	//for loop that transfers bit W, MB, Address and D7 to D1 (D0 sent later)
	for (i=0; i<15; i++){
		if(Transfer &(1<<15)){
			__R30 |= (1<<1); //MOSI -> HIGH
		}
		else{
			__R30 &= 0xFFFFFFFD; //MOSI -> LOW
		}
		__delay_cycles(TSETUP);
		__R30 |= (1<<2); //Clock -> High
		__delay_cycles(THOLD);
		__R30 &= 0xFFFFFFFB; //Clock -> LOW
		__delay_cycles(TSCLK-TSETUP-THOLD-5);
		Transfer<<=1;
	} //end of the for loop D0 still needs to be transmited
	if (Transfer & (1<<15)){
		__R30 |=(1<<1); //MOSI -> HIGH
	}
	else{
		__R30 &= 0xFFFFFFFD; //MOSI -> LOW
	}
	__delay_cycles(TSETUP);
	__R30 |= (1<<2); //Clock -> HIGH
	__delay_cycles(TQUIET);
	__R30 |= (1<<5); //CS -> HIGH
	__delay_cycles(TCSDIS);
	__R30 &= 0xFFFFFFDF; //CS -> LOW
	//return save;
}

uint8_t spiRead(uint8_t Register){ // returns the values from a given register
	int i;
	uint8_t data=0x00;
	Register |= (1<<7); //adding the R bit as the MSB of the adress
	//initialization
	__R30 |= (1<<5); //CS -> HIGH #seems to be useless
 	__R30 |= (1<<2); //Clock -> HIGH
 	__delay_cycles(TWAIT);
 	__R30 &= 0xFFFFFFDF; //CS -> LOW
 	__delay_cycles(TDELAY);
 	__R30 &= 0xFFFFFFFB; //Clock -> LOW
	//for loop that transfers bit R, MB, Address and D7 to D1 (D0 received later)
	for (i=0; i<15; i++){
		if (Register & (1<<7)){
			__R30 |= (1<<1); // MOSI -> HIGH
		}
		else{
		__R30 &= 0xFFFFFFFD; // MOSI -> LOW
		}
		__delay_cycles(TSETUP);
		__R30 |= (1<<2); //Clock -> HIGH
		if(__R31 & (1<<3)){
			data |= 1; //1 in data's LSB
		}
		__delay_cycles(THOLD);
		__R30 &= 0xFFFFFFFB; //Clock -> LOW
		//__R30 &= 0xFFFFFFFD; //MOSI -> LOW useless already low => MUST BE VERIFIED
		__delay_cycles(TSCLK-TSETUP-THOLD-8);
		Register <<=1; //shifting the adress of 1 bit
		data <<=1; //shifting data of 1 bit
	} //end of the for loop D0 still needs to be transmited
	__delay_cycles(TSETUP);
	__R30 |= (1<<2); //Clock -> HIGH
	if (__R31 & (1<<3)){
		data|= 1; //reading bit D0
	}
	__delay_cycles(TQUIET);
	__R30 |= (1<<5); //CS -> HIGH
	__delay_cycles(TCSDIS);
	__R30 &= 0xFFFFFFDF; //CS -> LOW

	return data;
}
