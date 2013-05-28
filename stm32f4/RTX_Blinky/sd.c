#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_spi.h"
#include <stdio.h>
#include <stm32f4xx_usart.h>
#include "console_usart.h"
#include "sd.h"

uint8_t inBlock_ = 0;
uint32_t block_;
uint16_t offset_;
uint8_t status_;
uint8_t type_ = 0;

void SPI_SetSpeed(u8 SpeedSet){
  SPI_InitTypeDef SPI_InitStructure;
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	
  SPI_InitStructure.SPI_BaudRatePrescaler = SpeedSet ? SPI_BaudRatePrescaler_8 : SPI_BaudRatePrescaler_256;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI1, &SPI_InitStructure);
  return;
}

void initSpi(void) {
    GPIO_InitTypeDef GPIO_InitStruct;
	
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
		
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1);
	
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_SetBits(GPIOA, GPIO_Pin_8);

    SPI_I2S_DeInit(SPI1);
		SPI_SetSpeed(0);
    SPI_Cmd(SPI1, ENABLE);
		
}

uint8_t SPI_ReadWriteByte(uint8_t TxData){
    uint8_t RxData = 0;
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI1, TxData);
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    RxData = SPI_I2S_ReceiveData(SPI1);
    return (uint8_t)RxData;
}


static void spiSend(uint8_t b) {
	SPI_ReadWriteByte(b);
}
static  uint8_t spiRec(void) {
	return SPI_ReadWriteByte(0XFF);
}
void chipSelectHigh(void) {GPIO_SetBits(GPIOA, GPIO_Pin_8);}
void chipSelectLow(void) {GPIO_ResetBits(GPIOA, GPIO_Pin_8);}

void readEnd(void) {
  if (inBlock_) {
    while (offset_++ < 514) spiRec();
    chipSelectHigh();
    inBlock_ = 0;
  }
}

uint8_t waitNotBusy()  {
	u8 i;
  do {
		i = spiRec();
    if (i == 0XFF) return 1;
  } while (1);
  return 0;
}

uint8_t cardCommand(uint8_t cmd, uint32_t arg) {
	int8_t s,i;
  uint8_t crc = 0XFF;
	
  readEnd();
  chipSelectLow();
  waitNotBusy();
  spiSend(cmd | 0x40);
  for (s = 24; s >= 0; s -= 8) 
	  spiSend(arg >> s);

  if (cmd == CMD0) crc = 0X95;  // correct crc for CMD0 with arg 0
  if (cmd == CMD8) crc = 0X87;  // correct crc for CMD8 with arg 0X1AA
  spiSend(crc);

  for (i = 0; ((status_ = spiRec()) & 0X80) && i != 0XFF; i++);
  return status_;
}


uint8_t cardAcmd(uint8_t cmd, uint32_t arg) {
  cardCommand(CMD55, 0);
  return cardCommand(cmd, arg);
}

u8 SD_Init() {
	unsigned char temp,i,status_,type;
	int arg,time;
  inBlock_ = type_ = 0;
	
	chipSelectHigh();
  for (i = 0; i < 20; i++) spiSend(0XFF);
  chipSelectLow();
	
	time = 0;
	while ((status_ = cardCommand(CMD0, 0)) != 0X01) {
    if (time++ > 200) {
			println("Error: CMD0");
			goto fail;
    }
  }
	
	if ((cardCommand(CMD8, 0x1AA) & 0X04)) {
    println("SD_CARD_TYPE_SD1");
		type = 1;
  } else {
    for (i = 0; i < 4; i++) {
			status_ = spiRec();
		}
    if (status_ != 0XAA) {
      println("Error: CMD8");
			goto fail;
    }
		println("SD_CARD_TYPE_SD2");
		type = 2;
  }
  chipSelectHigh();spiRec();spiRec();spiRec();delay(1);
  
	arg = (type == 2) ? 0X40000000 : 0;
  time=0;
	while ((status_ = cardAcmd(CMD41, arg)) != 0X00) {
    time++;
		if (time == 2000) {
			println("Error: ACMD41");
			goto fail;
    }
  }
  chipSelectHigh();spiRec();spiRec();delay(1);
  
	if (type == 2) {
    if (cardCommand(CMD58, 0)) {
			printf("Error: CMD58\r\n");
			goto fail;
    }
    if ((spiRec() & 0XC0) == 0XC0) 
		printf("SD_CARD_TYPE_SDHC\r\n");
    // discard rest of ocr - contains allowed voltage range
    for (i = 0; i < 3; i++) spiRec();
  }
  chipSelectHigh();

	SPI_SetSpeed(1);
  return 0;

 fail:
  chipSelectHigh();
  println("Error: init");
  return 1;
}

uint8_t writeData(uint8_t token, const uint8_t* src) {
  uint16_t i;
	spiSend(token);
  for (i = 0; i < 512; i++) {
    spiSend(src[i]);
  }
  spiSend(0xff);
  spiSend(0xff);

  status_ = spiRec();
  if ((status_ & 0X1F) != 0X05) {
    chipSelectHigh();
    printf("Error: writeData() 0x%x\r\n",status_);
    return 0;
  }
  return 1;
}

uint8_t writeBlock(uint32_t blockNumber, const uint8_t* src) {
  if (cardCommand(CMD24, blockNumber)) {
		println("Error: CMD24");
		goto fail;
  }
  if (!writeData(0XFE, src))
	  goto fail;

  if (!waitNotBusy()) {
    println("Error: Write timeout");
    goto fail;
  }
  if (cardCommand(CMD13, 0) || spiRec()) {
    println("Error: Write programming");
    goto fail;
  }
  chipSelectHigh();
  return 0;

 fail:
  chipSelectHigh();
  printf("Error: Sd2Card::writeBlock %d\r\n",blockNumber);
  return -1;
}

uint8_t waitStartBlock(void) {
  while ((status_ = spiRec()) == 0XFF);
  if (status_ != 0XFE) {
    println("Error: Read");
    goto fail;
  }
  return 1;

 fail:
  chipSelectHigh();
  println("Error: Sd2Card::waitStartBlock()");
  return 0;
}

uint8_t readData(uint32_t block,
        uint16_t offset, uint16_t count, uint8_t* dst) {
  uint16_t n,i;
  if (count == 0) return -1;
  if ((count + offset) > 512) goto fail;
  if (!inBlock_ || block != block_ || offset < offset_) {
    block_ = block;
    if (cardCommand(CMD17, block)) {
      println("Error: CMD17");
      goto fail;
    }
    if (!waitStartBlock()) goto fail;
		
    offset_ = 0;
    inBlock_ = 1;
  }

  for (;offset_ < offset; offset_++) spiRec();
	
  for (i = 0; i < count; i++) {
    dst[i] = spiRec();
		// printf("0x%02x ",dst[i]);
  }

  offset_ += count;
  if (offset_ >= 512) readEnd();
  
	return 0;

 fail:
  chipSelectHigh();
  println("Error: readData()");
  return -1;
}

uint8_t readBlock(uint32_t block, uint8_t* dst) {
//	printf("r b %d\r\n",block);
  return readData(block, 0, 512, dst);
}