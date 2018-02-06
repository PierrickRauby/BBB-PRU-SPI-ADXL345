/*
 * Source Modified by Pierrick Rauby <PierrickRauby - pierrick.rauby@gmail.com>
 * Based on the examples distributed by TI
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *	* Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 *
 *	* Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the
 *	  distribution.
 *
 *	* Neither the name of Texas Instruments Incorporated nor the names of
 *	  its contributors may be used to endorse or promote products derived
 *	  from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stdio.h>
#include <pru_cfg.h>
#include <pru_intc.h>
#include <rsc_types.h>
#include <pru_rpmsg.h>
#include <stdlib.h>
#include <string.h>
//#include <ADXL345_SPI.h> // spi.h is in ../include/spi
#include "resource_table_0.h"


uint8_t spiCommand;
#define PULSEWIDTH 300 //old delay



#define TWAIT 100000000 // long delay for stabilization
#define TDELAY 2 // delay for tdelay=2*5ns > 5ns=tdelay_min
#define TSETUP 2 // delay for tsetup=2*5ns > 5ns=tsetup_min
#define THOLD 2 // delay for thold=2*5ns > 5ns=thold_min
#define TSCLK 45 // delay for tsclk=45*5ns > 200ns=tsclk_min
#define TQUIET 2 // delay for tquiet=2*5ns > 5ns=tquiet_min
#define TCSDIS 35 // delay for tcsdis=35*5ns > 150ns=tcsdis_min

volatile register uint32_t __R30;
volatile register uint32_t __R31;

/* Host-0 Interrupt sets bit 30 in register R31 */
#define HOST_INT			((uint32_t) 1 << 30)

/* The PRU-ICSS system events used for RPMsg are defined in the Linux device tree
 * PRU0 uses system event 16 (To ARM) and 17 (From ARM)
 * PRU1 uses system event 18 (To ARM) and 19 (From ARM)
 */
#define TO_ARM_HOST			16
#define FROM_ARM_HOST			17
#define CHAN_NAME			"rpmsg-pru"
#define CHAN_DESC			"Channel 30"
#define CHAN_PORT			30

#define VIRTIO_CONFIG_S_DRIVER_OK	4

uint8_t payload[RPMSG_BUF_SIZE];

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

void main(void)
{
	struct pru_rpmsg_transport transport;
	uint16_t src, dst, len;
	volatile uint8_t *status;
	/* Allow OCP master port access by the PRU so the PRU can read external memories */
	CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;
	/* Clear the status of the PRU-ICSS system event that the ARM will use to 'kick' us */
	CT_INTC.SICR_bit.STS_CLR_IDX = FROM_ARM_HOST;
	/* Make sure the Linux drivers are ready for RPMsg communication */
	status = &resourceTable.rpmsg_vdev.status;
	while (!(*status & VIRTIO_CONFIG_S_DRIVER_OK));
	/* Initialize the RPMsg transport structure */
	pru_rpmsg_init(&transport, &resourceTable.rpmsg_vring0, &resourceTable.rpmsg_vring1, TO_ARM_HOST, FROM_ARM_HOST);
	/* Create the RPMsg channel between the PRU and ARM user space using the transport structure. */
	while (pru_rpmsg_channel(RPMSG_NS_CREATE, &transport, CHAN_NAME, CHAN_DESC, CHAN_PORT) != PRU_RPMSG_SUCCESS);

	while (1) {
	/* Check bit 30 of register R31 to see if the ARM has kicked us */
	if (__R31 & HOST_INT) {
		/* Clear the event status */
		CT_INTC.SICR_bit.STS_CLR_IDX = FROM_ARM_HOST;
		/* Receive all available messages, multiple messages can be sent per kick */
		while (pru_rpmsg_receive(&transport, &src, &dst, payload, &len) == PRU_RPMSG_SUCCESS) {
					//variables initialization
					uint8_t data = 0x00; // Incoming data stored here.
					spiCommand = 0x2D;
					__R30 = 0x00000000;//  Clear the output pin.
					spiWrite(0x2D,0x2B);
					data=spiRead(0x2D);
				 	payload[0]= (uint16_t)data;
				 	pru_rpmsg_send(&transport, dst, src, payload, 2);
				}
    }
  }
}
