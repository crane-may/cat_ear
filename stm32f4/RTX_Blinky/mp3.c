#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_spi.h"
#include <misc.h>   
#include <stdio.h>
#include <stm32f4xx_usart.h>
#include "console_usart.h"
#include "mp3.h"

int volume=0x10;//set volume here 

uint8_t SPI3_ReadWriteByte(uint8_t TxData){
    uint8_t RxData = 0;
    while(SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI3, TxData);
    while(SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE) == RESET);
    RxData = SPI_I2S_ReceiveData(SPI3);
    return (uint8_t)RxData;
}

void set_mp3_speed(int s){
	SPI_InitTypeDef SPI_InitStructure;
	
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	
  SPI_InitStructure.SPI_BaudRatePrescaler = s ? SPI_BaudRatePrescaler_4 : SPI_BaudRatePrescaler_32;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI3, &SPI_InitStructure);
}

void init_mp3(){
	GPIO_InitTypeDef GPIO_InitStruct;
	
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

  GPIO_StructInit(&GPIO_InitStruct);
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
  GPIO_Init(GPIOB, &GPIO_InitStruct);

  
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_SPI3);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_SPI3);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_SPI3);
  
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStruct.GPIO_Pin = xCs | xReset | xDcs;
  GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStruct.GPIO_Pin = dReq;
  GPIO_Init(GPIOA, &GPIO_InitStruct);
  
	SPI_I2S_DeInit(SPI3);	
	set_mp3_speed(0);
  SPI_Cmd(SPI3, ENABLE);
}

void commad(unsigned char addr,unsigned char hdat,unsigned char ldat ){  
  while(! GPIOA->IDR & dReq);
    GPIO_ResetBits(GPIOB,xCs);
    SPI3_ReadWriteByte(0X02);
    SPI3_ReadWriteByte(addr);
    SPI3_ReadWriteByte(hdat);
    SPI3_ReadWriteByte(ldat);    
    GPIO_SetBits(GPIOB,xCs);
}

void Mp3Reset(){
	GPIO_ResetBits(GPIOB,xReset);
  delay(100);
  GPIO_SetBits(GPIOB,xCs);
  GPIO_SetBits(GPIOB,xDcs);
  GPIO_SetBits(GPIOB,xReset);
  delay(100);
  commad(0X00,0X08,0X04);
  delay(10);

  commad(0X03,0XC0,0X00);//??VS1003???
  delay(10);
  commad(0X05,0XBB,0X81);//??VS1003???? 44kps ???
  delay(10);
  commad(0X02,0X00,0X55);//????
  delay(10);
  commad(0X0B,volume,volume);//????0x0000????0xFEFE
  delay(10); 
	
  SPI3_ReadWriteByte(0);
  SPI3_ReadWriteByte(0);
  SPI3_ReadWriteByte(0);
  SPI3_ReadWriteByte(0);
  GPIO_SetBits(GPIOB,xCs);
  GPIO_SetBits(GPIOB,xReset);
  GPIO_SetBits(GPIOB,xDcs);
	
	set_mp3_speed(1);
	delay(100);
}

int mp3_need_data(){
	return GPIOA->IDR & dReq;
}

void mp3_input_data(u8* s){
	int i;
	GPIO_ResetBits(GPIOB,xDcs);
	for(i=0;i<32;i++){
 		SPI3_ReadWriteByte(s[i]);
 	}
	GPIO_SetBits(GPIOB,xDcs);
}

void mute(){
	static u8 dd[32] = {0};
	if (mp3_need_data()) mp3_input_data(dd);
}

static int bitrate[2][3][16] = {  
{ // Version 1.0  
{ // Layer I 
0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, (-1) 
}, 
{ // Layer II 
0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, (-1) 
}, 
{ // Layer III 
0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, (-1) 
} 
},

{ // Version 2.0  
{ // Layer I 
0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, (-1) 
}, 
{ // Layer II 
0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, (-1) 
}, 
{ // Layer III 
0, 8, 16, 24, 32, 64, 80, 56, 64, 128, 160, 112, 128, 256, 320, (-1) 
}
} 
};

static int sampling[4][4] = {
	{11025,12000,8000,0},
	{0,0,0,0},
	{22050,24000,16000,0},
	{44100,48000,32000,0}
};

int calc_frame_size(u8* header){
	u8 ver;
	u8 layer;
	int br_;
	int br;
	u8 smp_;
	int smp;
	int padding;
	
	ver = (header[1] & 0x18) >> 3;
	layer = (header[1] & 0x6) >> 1;
	br_ = (header[2] & 0xf0) >> 4;
	smp_ = (header[2] & 0xc) >> 2;
	
	if (header[0] != 0xff || ver == 0x01 || layer == 0x0 || br_ == 0xff)
		return -1;
	
	br = bitrate[ ver == 3 ? 0 : 1 ][ 3 - layer ][ br_ ];
	smp = sampling[ ver ][ smp_ ];
	
	if (br == -1 || smp == 0)
		return -1;
	
	padding = (header[2] & 0x2) >> 1;
	
	printf("v:%x,b:%d,s:%d\r\n",ver,br,smp);
	
	return ((( ver == 3 ? 144:72 ) * br * 1000 ) / smp ) + padding;
}

void play(){
	//  	while(! GPIOA->IDR & dReq) __NOP();
//  	for(i=0;i<32;i++){
//  		GPIO_ResetBits(GPIOB,xDcs);
//  		if (can_fetch_data()){
// 			SPI3_ReadWriteByte(fetch_data());
// 			toggle_led(0);
// 		}	else {
// 			toggle_led(2);
// 		}
//  		GPIO_SetBits(GPIOB,xDcs);
//  	}
}