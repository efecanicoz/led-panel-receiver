#include "hal.h"
#include "ch.h"


/*Macros*/
#define SOH 0x01
#define EOT 0x04

/*Prototypes*/


/*
 * Low speed SPI configuration (140.625kHz, CPHA=0, CPOL=0, MSb first).
 */
 //gpiob ike çalışıyomuş ne iş ?
static const SPIConfig ls_spicfg = {
  NULL,
  GPIOA,
  12,
  SPI_CR1_BR_2 | SPI_CR1_BR_1,
  SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0
};


static uint8_t txbuf[32];
static uint8_t msgArr[6] = {0,0,0,0,0,0};
//static uint8_t rxbuf[32];

int main(void)
{
	halInit();
	chSysInit();
	
	uint8_t packet[3];
	uint8_t sender, data, calc_parity, last_data;
	palClearPad(GPIOF, 0);
	palSetPadMode(GPIOF, 0, PAL_MODE_OUTPUT_PUSHPULL);
	palSetPadMode(GPIOA, GPIOA_USART_TX, PAL_MODE_ALTERNATE(1)); // used function : USART1_TX
	palSetPadMode(GPIOA, GPIOA_USART_RX, PAL_MODE_ALTERNATE(1)); // used function : USART1_RX
	palSetPadMode(GPIOA, GPIOA_PIN4, PAL_MODE_ALTERNATE(0) | PAL_STM32_OSPEED_HIGHEST); //spi nss
	palSetPadMode(GPIOA, GPIOA_PIN5, PAL_MODE_ALTERNATE(0) | PAL_STM32_OSPEED_HIGHEST); //spi sck
	palSetPadMode(GPIOA, GPIOA_PIN6, PAL_MODE_ALTERNATE(0) | PAL_STM32_OSPEED_HIGHEST); //spi miso
	palSetPadMode(GPIOA, GPIOA_PIN7, PAL_MODE_ALTERNATE(0) | PAL_STM32_OSPEED_HIGHEST); //spi mosi
	
	sdStart(&SD1, NULL);
	spiStart(&SPID1, &ls_spicfg);
    
	while(!0)
	{
		//try to catch a valid telegram start byte
		do
		{
			sdRead(&SD1, &packet[0], 1U);
		}while(packet[0] != SOH);
		
		sdRead(&SD1, &packet[1], 2U);
				
		if(packet[2] != EOT)
			continue;
		calc_parity = packet[1] ^ (packet[1]>>1);
		calc_parity ^= calc_parity>>2;
		calc_parity ^= calc_parity>>4;
		calc_parity &= 1;
		
		if(calc_parity != 0)
			continue;
			
		//valid telegram
		sender = (packet[1] & 0xF0) >> 4;
		data = (packet[1] & 0x0E) >> 1;
		
		if(data > 5 || data == last_data)
			continue;
		
		msgArr[data]++;
		
		if(msgArr[data] > 5)
		{
			
			palSetPad(GPIOF, 0);
			last_data = data;
			txbuf[0] = data;
			txbuf[30] = sender;
			
			spiSelect(&SPID1);
			spiSend(&SPID1, 32, txbuf);
			spiUnselect(&SPID1);
			//find more efficent way
			msgArr[0] = 0;
			msgArr[1] = 0;
			msgArr[2] = 0;
			msgArr[3] = 0;
			msgArr[4] = 0;
			msgArr[5] = 0;
			
			palClearPad(GPIOF, 0);
		}
		
		chThdSleepMilliseconds(10);
	}
	return 0;
}
