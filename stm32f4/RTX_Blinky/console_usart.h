#ifndef __CONSOLE_USART_H__
#define __CONSOLE_USART_H__

#include <stdint.h>

void send_char(uint8_t ch) ;

void init_usart();
void println(char* s);

#endif
