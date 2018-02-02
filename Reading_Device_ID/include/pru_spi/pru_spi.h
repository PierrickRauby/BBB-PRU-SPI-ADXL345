/*
*/
#ifndef _PRU_SPI_H_
#define _PRU_SPI_H_

//prototypes for the spiRead and spiWrite function
uint8_t spiRead(uint8_t Register);
void spiWrite(uint8_t Register, uint8_t Value);
