#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_spi.h"
#include <misc.h>
#include <stdio.h>
#include <stm32f4xx_usart.h>
#include "timer.h"

static volatile uint32_t millisecondCounter;
void initSystick() {
  millisecondCounter=0;
  SysTick_Config(SystemCoreClock/1000);
}

void delay(uint32_t millis) {
  uint32_t target;
 
  target=millisecondCounter+millis;
  while(millisecondCounter<target) __NOP();
}

void SysTick_Handler(void) {
  millisecondCounter++;
}

uint32_t get_millisecond(){
	return millisecondCounter;
}