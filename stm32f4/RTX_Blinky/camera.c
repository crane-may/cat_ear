#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_i2c.h"
#include <zbar.h>
#include <image_data.h>
#include <stdio.h>
#include <stm32f4xx_usart.h>
#include "Sensor.h"
#include "console_usart.h"
#include "camera.h"

void qrcode (){
    int width = 160, height = 120;
    zbar_image_scanner_t *scanner = 0;
	  int n ;
	  zbar_image_t *image;
	  const zbar_symbol_t *symbol;
		printf("start qrcode ...\n\r");
    scanner = zbar_image_scanner_create();

    zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_ENABLE, 1);

    image = zbar_image_create();
    zbar_image_set_format(image, *(int*)"Y800");
    zbar_image_set_size(image, width, height);
    zbar_image_set_data(image, image_data, width * height, zbar_image_free_data);

		printf("start scan\n\r");
    n = zbar_scan_image(scanner, image);
		printf("scan over\n\r");

    symbol = zbar_image_first_symbol(image);
    for(; symbol; symbol = zbar_symbol_next(symbol)) {
        /* do something useful with results */
        zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
        const char *data = zbar_symbol_get_data(symbol);
        printf("decoded %s symbol \"%s\"\r\n",
               zbar_get_symbol_name(typ), data);
    }

    /* clean up */
//    zbar_image_destroy(image);
    zbar_image_scanner_destroy(scanner);
}

#define SLAVE_ADDRESS 0x42 // the slave address 


#define FIFO_CS_BIT     GPIO_Pin_13  // 
#define FIFO_RRST_BIT   GPIO_Pin_11 // 
#define FIFO_RD_BIT     GPIO_Pin_10  // 
#define FIFO_WE_BIT     GPIO_Pin_12  //


#define FIFO_CS_H()    GPIOC->BSRRL  =FIFO_CS_BIT
#define FIFO_CS_L()    GPIOC->BSRRH  =FIFO_CS_BIT


#define FIFO_RRST_H()  GPIOC->BSRRL =FIFO_RRST_BIT
#define FIFO_RRST_L()  GPIOC->BSRRH  =FIFO_RRST_BIT

#define FIFO_RD_H()    GPIOC->BSRRL =FIFO_RD_BIT
#define FIFO_RD_L()    GPIOC->BSRRH  =FIFO_RD_BIT

#define FIFO_WE_H()    GPIOC->BSRRL =FIFO_WE_BIT
#define FIFO_WE_L()    GPIOC->BSRRH  =FIFO_WE_BIT


int shot() {
	char *p ,*pp;
	int sum=0;
	int clock =0;
	int t1,t2,t3,i;
	uint32_t c_data,r,g,b;
	GPIO_InitTypeDef GPIO_InitStruct;
	
//   I2C_LowLevel_Init();
// 	for (i=0;i<OV7670_REG_NUM;i++){
// 		I2C_WrBuf(0x42,(uint8_t *)OV7670_reg[i],2);
// 	}
	
	init_I2C1();
 	for (i=0;i<OV7670_REG_NUM;i++){
		I2C_start(I2C1, SLAVE_ADDRESS, I2C_Direction_Transmitter); // start a transmission in Master transmitter mode
		I2C_write(I2C1, OV7670_reg[i][0]); // write one byte to the slave
		I2C_write(I2C1, OV7670_reg[i][1]); // write another byte to the slave
		I2C_stop(I2C1); // stop the transmission
 	}
	
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	
	GPIO_InitStruct.GPIO_Pin =	GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | 
															GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 |
															GPIO_Pin_8;		  // we want to configure PA0
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN; 	  // we want it to be an input
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;//this sets the GPIO modules clock speed
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;   // this sets the pin type to push / pull (as opposed to open drain)
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;   // this enables the pulldown resistor --> we want to detect a high level
	GPIO_Init(GPIOC, &GPIO_InitStruct);
	
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13; // we want to configure all LED GPIO pins
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT; 		// we want the pins to be an output
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz; 	// this sets the GPIO modules clock speed
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP; 	// this sets the pin type to push / pull (as opposed to open drain)
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL; 	// this sets the pullup / pulldown resistors to be inactive
	GPIO_Init(GPIOC, &GPIO_InitStruct); 			// this finally passes all the values to the GPIO_Init function which takes care of setting the corresponding bits.
	
	FIFO_CS_L();
  FIFO_WE_L();
	
	FIFO_RRST_L(); 
  FIFO_RD_L();
  FIFO_RD_H();
  FIFO_RD_L();
  FIFO_RRST_H();
  FIFO_RD_H();
	delay(5);
	
	
  while (GPIOC->IDR & 0x0100);
	while (! (GPIOC->IDR & 0x0100));
  
	FIFO_WE_H();
	while (GPIOC->IDR & 0x0100);
  FIFO_WE_L();
  while (! (GPIOC->IDR & 0x0100));
	
	FIFO_RRST_L(); 
  FIFO_RD_L();
  FIFO_RD_H();
  FIFO_RD_L();
  FIFO_RRST_H();
  FIFO_RD_H();
	
	printf("start shot...\r\n");
	
	t1 = 0;
  for(i=0;i<76800;i++){
		FIFO_RD_L();
		c_data = (GPIOC->IDR & 0x00ff) << 8; 
		send_char((uint8_t)GPIOC->IDR&0x00ff);
		FIFO_RD_H();
		FIFO_RD_L();
		c_data |= GPIOC->IDR &0x00ff; 
		send_char((uint8_t)GPIOC->IDR&0x00ff);
		FIFO_RD_H();
		
		r = (c_data & 0xf800) >> 8;
		b = (c_data & 0x07e0) >> 3;
		g = (c_data & 0x001f) << 3;
		
		c_data = (r*76 + g*150 + b*30) >> 8;
		
 		if ( (i % 2 == 0)&&
 			   ( (i/320) % 2 == 0 ) ) {
			//send_char((uint8_t)c_data);
			//image_data[t1++] = (uint8_t)c_data;
		}
	}
	
	printf("shot over\r\n");
	
  //qrcode();
}