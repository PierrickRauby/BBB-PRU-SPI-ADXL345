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
#include "resource_table_0.h"

volatile register uint32_t __R30;
volatile register uint32_t __R31;
uint8_t spiCommand;
#define PULSEWIDTH 300

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

void main(void)
{
	struct pru_rpmsg_transport transport;
	uint16_t src, dst, len;
	volatile uint8_t *status;
  int i;
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
					uint16_t data = 0x00; // Incoming data stored here.
					spiCommand = 0x30;
					// int RW= 1; // RW=1 for reading
					// if (RW){
					// spiCommand= spiCommand | (1<<7);
					// }
					__R30 = 0x00000000;//  Clear the output pin.
					//start R/W bit
					__R30 = __R30 | (1 << 5); //chip select HIGH. __R30 = 0x00100000;
					__delay_cycles(100000000); // Delay
					__R30 = __R30 & 0xFFFFFFDF; //  CS-> LOW P9.27 r30_5
					__R30 = __R30 & 0xFFFFFFFB; //  Clock -> LOW P9.30 r30_2
					__R30 = __R30 | (1 << 1);   //  MOSI -> HIGH P9.29 r30_1
					__delay_cycles(PULSEWIDTH); //  Delay
					__R30 = __R30 | (1<<2); //  Clock -> HIGH P9.30 r30_2
					__delay_cycles(PULSEWIDTH); //  Delay
					__R30 = __R30 & 0xFFFFFFFB; //  Clock -> LOW P9.30 r30_2
					__R30 = __R30 & (0xFFFFFFFD);  //  MOSI -> LOW P9.29 r30_1
					//loop for 15 last bits
					for (i = 0; i < 15; i ++) {
								//shifting
								//data<<=1;
								//spiCommand <<=1;
								// Writting on MOSI
						    if (spiCommand & (1<<7)){  //  If the spiCommand MSB 1
						       __R30 = __R30 | (1 << 1); //  MOSI -> HIGH P9.29 r30_1
						    }
						    else{
						       __R30 = __R30 & (0xFFFFFFFD);  //  MOSI -> LOW P9.29 r30_1
						    }
								//Clock toggling
						    __delay_cycles(PULSEWIDTH); //  Delay
						    __R30 = __R30 | (1<<2); //  Clock -> HIGH P9.30 r30_2
								// Reading on MISO
								if (__R31 & (1 << 3)){ //MISO read P9.28 r31_3
									 data |= 1;} // 1-> data's LBS
								__delay_cycles(PULSEWIDTH); //  Delay
						    __R30 = __R30 & 0xFFFFFFFB; //  Clock -> LOW P9.30 r30_2
								//shifting
								data<<=1;
								spiCommand <<=1;
 							 }
					__delay_cycles(PULSEWIDTH); //  Delay
					__R30 = __R30 | (1<<2); //  Clock -> HIGH P9.30 r30_2
					if (__R31 & (1 << 3)){ //MISO read P9.28 r31_3
					 data |= 1;} // 1-> data's LBS
					__delay_cycles(PULSEWIDTH); //  Delay
				 	__R30 = __R30 | 1 << 5; //  Chip select to HIGH
				 	payload[0]= (uint8_t)data;
				 	pru_rpmsg_send(&transport, dst, src, payload, 2);
				}
    }
  }
}
