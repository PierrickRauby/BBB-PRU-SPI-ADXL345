/*Define delays for the ADXL 345 SPI communication*/


#define TWAIT 100000000 // long delay for stabilization
#define TDELAY 2 // delay for tdelay=2*5ns > 5ns=tdelay_min
#define TSETUP 2 // delay for tsetup=2*5ns > 5ns=tsetup_min
#define THOLD 2 // delay for thold=2*5ns > 5ns=thold_min
#define TSCLK 45 // delay for tsclk=45*5ns > 200ns=tsclk_min
#define TQUIET 2 // delay for tquiet=2*5ns > 5ns=tquiet_min
#define TCSDIS 35 // delay for tcsdis=35*5ns > 150ns=tcsdis_min

volatile register uint32_t __R30;
volatile register uint32_t __R31;

uint8_t spiRead(uint8_t Register);
void spiWrite(uint8_t Register, uint8_t Value);
