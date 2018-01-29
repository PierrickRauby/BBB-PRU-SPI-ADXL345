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
uint32_t spiCommand;
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
					//—————————————————————————SPI Capture Loop———————————————————————————
					//——————————————————Initializing the SPI communication————————————————
					uint32_t data = 0x00000000; // Incoming data stored here.
					                          	//  The data out line is connected to R30 bit 1.
					uint16_t dataCounter = 0; // Used to load data transmission buffer payloadOut;
					__R30 = 0x00000000;//  Clear the output pin.

					__R30 = __R30 | (1 << 5); // Initialize chip select HIGH. __R30 = 0x00010000;
					__delay_cycles(100000000); //  Allow chip to stabilize.
					//————————————Transmitting the address we want to receive data from———
					//spiCommand is the MOSI preamble; must be reset for each sample.
					    spiCommand = 0x00000000;    // All 0 to have the Device ID
					    data = 0x00000000;          //  Initialize data.
					    __R30 = __R30 & 0xFFFFFFDF; //  Chip select to LOW P9.27
					/* au dessus: 0xFFFFFFDF => que des 1 sauf en bit n°5 ou il y a un 0 donc avec
					l'opérateur & on obient  0x00000000 => le bit 5 de __R30 est passé à LOW */
					    __R30 = __R30 & 0xFFFFFFFB; //  Clock to LOW   P9.30
					//  Start-bit is HIGH.  The Start-bit is manually clocked in here.
					/* au dessus: 0xFFFFFFFB => que des 1 sauf en bit n°2 où il y a un 0 donc avec
					l'opérateur & on obtient 0x00000000 => le bit 5 de _R30 passe à 0 */
					    __R30 = __R30 | (1 << 1);   //  Set a 0x10 on MOSI (the Start-bit)
					/* On met le bit 1 du registre _R30 sur 1 pour lancer MOSI*/
					    __delay_cycles(PULSEWIDTH); //  Delay to allow settling.
					    __R30 = __R30 | 0x00000004; //  Clock goes high; latch in start bit.
					/* passe le bit n°2 a 1 ce qui active la clock */
					    __delay_cycles(PULSEWIDTH); //  Delay to allow settling.
					    __R30 = __R30 & 0xFFFFFFFB; //  Clock to LOW   P9.30
					// The Start-bit cycle is completed.
					//——————————–––——Receiving data from the address given above——————————
					for (i = 0; i < 16; i = i + 1) { //  Inner single sample loop
					     //  The first action will be to transmit on MOSI by shifting out
					     //  spiCommand variable.
					     if (spiCommand >> 31)       //  If the MSB is 1
					       __R30 = __R30 | (1 << 1); //  Set a 0x10 on MOSI
					     else
					       __R30 = __R30 & (0xFFFFFFFD); //  All 1s except bit 1
					     spiCommand = spiCommand << 1;   // Shift left one bit (for next cycle).
					     __delay_cycles(PULSEWIDTH);
					     __R30 = __R30 | 0x00000004; //  Clock goes high; bit set on MOSI.
					     __delay_cycles(PULSEWIDTH); //  Delay to allow settling.
					     //  The data needs to be "shifted" into the data variable.
					     data = data << 1;           // Shift left; insert 0 at lsb.
					     __R30 = __R30 & 0xFFFFFFFB; //  Clock to LOW   P9.30
					     if (__R31 & (1 << 3)) //  Probe MISO data from ADC.
					       data = data | 1; //if one received LSB set to 1
					     else
					       data = data & 0xFFFFFFFE; //else LSB set to 0
					   } // *****End of 16 cycles loop****
				 __R30 = __R30 | 1 << 5; //  Chip select to HIGH
				 payload[dataCounter]=(int8_t)data;
				 dataCounter = dataCounter + 1;
				 if (dataCounter == 1) {
				   pru_rpmsg_send(&transport, dst, src, payload, 490);
				   dataCounter = 0;
					   }
				}
    }
  }
}
