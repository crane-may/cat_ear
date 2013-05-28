#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_spi.h"
#include <misc.h>   
#include "console_usart.h"
#include <stm32f4xx_usart.h>
#include <stdio.h>
#include "wifi.h"
#include "ring_buffer.h"
#include "image_data.h"

// 0 - green 
// 1 - orange 
// 2 - red wifi lock
// 3 - blue usart receive
const uint16_t LEDS = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
const uint16_t LED[4] = {GPIO_Pin_12, GPIO_Pin_13, GPIO_Pin_14, GPIO_Pin_15};
void init_LEDs() {
    GPIO_InitTypeDef gpio;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    GPIO_StructInit(&gpio);
    gpio.GPIO_Mode = GPIO_Mode_OUT;
    gpio.GPIO_Pin = LEDS;
    GPIO_Init(GPIOD, &gpio);
		GPIO_ResetBits(GPIOD, LEDS);
}

void toggle_led(int n){
	(GPIOD->IDR & LED[n]) ? GPIO_ResetBits(GPIOD, LED[n]) :	GPIO_SetBits(GPIOD, LED[n]);
}
void set_led(int n){
	GPIO_SetBits(GPIOD, LED[n]);
}
void reset_led(int n){
	GPIO_ResetBits(GPIOD, LED[n]);
}

#define buf_size 1024
#define buf_size_line 1000
#define buf_size_bottom 100
static uint8_t received_buf[buf_size];

ring_buffer received_rb;
int counter_receive = 0;
int counter_used = 0;

void may_lock_wifi(){
	if (rb_full_count(&received_rb) > buf_size_line) {
		GPIO_SetBits(GPIOA, GPIO_Pin_13);
		set_led(2);
	}
}

void may_release_wifi(){
	if (rb_full_count(&received_rb) < buf_size_bottom ) {
		GPIO_ResetBits(GPIOA, GPIO_Pin_13);
		reset_led(2);
	}
}

void release_wifi(){
	GPIO_ResetBits(GPIOA, GPIO_Pin_13);
	reset_led(2);
}

void USART3_IRQHandler(void){
  if( USART_GetITStatus(USART3, USART_IT_RXNE) ){
		char c = USART3->DR;
// 		if (rb_is_full(&received_rb)){
// 			println("error full");while (1);
// 		}
		rb_insert(&received_rb,c);
		counter_receive++;
		may_lock_wifi();
		toggle_led(3);
  }
}

static int sd_writed = 0;
static int sd_used = 0;
void store_buffer(){
	int i;
	static u8 sd_write_buf[512];
	while ( rb_full_count(&received_rb) >= 512){
		//printf("write %d \r\n",sd_writed+1);
		for (i=0;i<512;i++){
// 			if (rb_is_empty(&received_rb)){
// 				println("error full--");while (1);
// 			}
			sd_write_buf[i] = rb_remove(&received_rb);
		}
		writeBlock(sd_writed+1,sd_write_buf);
		sd_writed++;
		release_wifi();
		// printf("%d\t%d\t%d\r\n",len,sd_write_idx,get_receive_sum());
	}
}

u8 wifi_receive(){
	static u8 sd_cur_buf[512];
	static int sd_cur_idx = 512;
	u8 c;
	
	if (sd_cur_idx < 512){
		counter_used++;
		return sd_cur_buf[sd_cur_idx++];
	} 
	else if (sd_writed > sd_used){
		//printf("read %d\r\n",sd_used+1);
		readBlock(sd_used+1,sd_cur_buf);
		sd_used++;
		sd_cur_idx = 0;
		counter_used++;
		return sd_cur_buf[sd_cur_idx++];
	}
	else if (rb_full_count(&received_rb) > 0){
		counter_used++;
		c = rb_remove(&received_rb);
		may_release_wifi();
		return c;
	}
	else{
		println("errrrrrrrrrrrrrrrrrrrrrrrrrrrror");
		while(1);
		return 0;
	}
}

int wifi_available_len(){
	return counter_receive - counter_used;
}


void init_wifi(uint32_t baudrate){
	GPIO_InitTypeDef GPIO_InitStruct; // this is for the GPIO pins used as TX and RX
	USART_InitTypeDef USART_InitStruct; // this is for the USART1 initilization
	NVIC_InitTypeDef NVIC_InitStructure; // this is used to configure the NVIC (nested vector interrupt controller)
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11; // Pins 6 (TX) and 7 (RX) are used
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF; 			// the pins are configured as alternate function so the USART peripheral has access to them
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;		// this defines the IO speed and has nothing to do with the baudrate!
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;			// this defines the output type as push pull mode (as opposed to open drain)
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;			// this activates the pullup resistors on the IO pins
	GPIO_Init(GPIOB, &GPIO_InitStruct);					// now all the values are passed to the GPIO_Init() function which sets the GPIO registers
	
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_USART3); //
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_USART3);
	
	USART_InitStruct.USART_BaudRate = baudrate;				// the baudrate is set to the value we passed into this init function
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;// we want the data frame size to be 8 bits (standard)
	USART_InitStruct.USART_StopBits = USART_StopBits_1;		// we want 1 stop bit (standard)
	USART_InitStruct.USART_Parity = USART_Parity_No;		// we don't want a parity bit (standard)
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // we don't want flow control (standard)
	USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; // we want to enable the transmitter and the receiver
	USART_Init(USART3, &USART_InitStruct);					// again all the properties are passed to the USART_Init function which takes care of all the bit setting
	
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE); // enable the USART1 receive interrupt 
	
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;		 // we want to configure the USART1 interrupts
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;// this sets the priority group of the USART1 interrupts
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		 // this sets the subpriority inside the group
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			 // the USART1 interrupts are globally enabled
	NVIC_Init(&NVIC_InitStructure);							 // the properties are passed to the NVIC_Init function which takes care of the low level stuff	
	
	rb_init(&received_rb,sizeof(received_buf),received_buf);
	printf("buf_len:%d\r\n",received_rb.size);
	USART_Cmd(USART3, ENABLE);
	
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14; 
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	init_LEDs();
}

////////////////////////////

void send_c(char c){
	while( !(USART3->SR & 0x00000040) ); 
  USART_SendData(USART3, c);
}
void send_s(char *s){
	while(*s){
		while( !(USART3->SR & 0x00000040) ); 
		USART_SendData(USART3, *s);
    *s++;
  }
}

void _send_cmd(volatile char *s, int n){
	int org_n = n;
	char c;
	printf("---\r\n%s",s);
	send_s(s);
	send_c('\r');
	send_c('\n');
	n = 1000000;
	while (n--){
		if (wifi_available_len()){
			while (wifi_available_len() >0 ){
				c = wifi_receive();
				printf("%c",c);
			};
			n = org_n;
		}
		delay(1);
	}
}
void send_cmd(volatile char *s) {_send_cmd(s,100);};
void send_query(volatile char *s) {_send_cmd(s,1000);};
void send_query_not_over(volatile char *s) {
	printf("---%s",s);
	send_s(s);
};

////////////////////////////
int insert_line_buf(u8 *buf,int buf_max,int idx,u8 c){
	if (idx >= buf_max) return idx;
	if (c == '\n'){
		buf[idx] = c;
		return 0;
	}else{
		buf[idx] = c;
		return idx+1;
	}
}

void send_fetch_song(char *url){
	int domain_end = 7;
	char *ip;
	
	while (url[domain_end] != '/') domain_end++;
	url[domain_end] = '\0';
//	ip = send_dnslookup(url+7,domain_end - 7);
	ip = send_dnslookup("au.ibooloo.com",14);
	
	printf("]]] %s\r\n",url+7);
	printf("]]] %s\r\n",url+domain_end);
	
	send_query_not_over("AT+NCTCP=");
	send_query_not_over(ip);
	send_query(",80");
	
	send_c(0x1b);
	send_c('S');
	send_c('0');
// 	send_s("GET /");
// 	send_s(url+(domain_end+1));
// 	send_s(" HTTP/1.1\r\n");
//  	send_s("Host: ");
// 	send_s(url+7);
// 	send_s("\r\n");
	send_s("GET /haihai.mp3?avthumb/mp3/ab/96k HTTP/1.1\r\n");
	send_s("Host: au.ibooloo.com\r\n");
	
 	send_s("Connection: close\r\n");
 	send_s("User-Agent: Mozilla/5.0 (Windows NT 6.2; WOW64) AppleWebKit/537.11 (KHTML, like Gecko) Chrome/23.0.1271.97 Safari/537.11\r\n");
 	send_s("Accept: */*;q=0.8\r\n");
	send_s("\r\n");
	send_s("\r\n");
	send_c(0x1b);
	send_c('E');
}

static char play_list[4][150];
static int play_list_len = 0;

char *next_song(){
	return "http://au.ibooloo.com/haihai.mp3?avthumb/mp3/ab/96k";
	
	if (play_list_len == 0){
		send_fetch_play_list();
	}
	
	return play_list[--play_list_len];
}

void send_fetch_play_list(){
	char *ip = send_dnslookup("douban.fm",9);
	send_query_not_over("AT+NCTCP=");
	send_query_not_over(ip);
	send_query(",80");
	
	send_c(0x1b);
	send_c('S');
	send_c('0');
	send_s("GET /j/mine/playlist?type=n&channel=0 HTTP/1.1\r\n");
 	send_s("Host: douban.fm\r\n");
 	send_s("Connection: close\r\n");
 	send_s("User-Agent: Mozilla/5.0 (Windows NT 6.2; WOW64) AppleWebKit/537.11 (KHTML, like Gecko) Chrome/23.0.1271.97 Safari/537.11\r\n");
 	send_s("Accept: */*;q=0.8\r\n");
	send_s("Accept-Encoding: deflate\r\n");
	send_s("\r\n");
	send_s("\r\n");
	send_c(0x1b);
	send_c('E');
	
	while(1){
		static int brace_level = 0;
		static int in_brace = 0;
		static int parenthesis_level = 0;
		static int in_parenthesis = 0;
		static int pl_idx = 0;
		static int pl_c_idx = 0;
		
		char c;
		
		if (wifi_available_len() == 0) continue;
		
		store_buffer();
		
		c = wifi_receive();
		print_c(c);
		
		if (c == '{'){
			brace_level++;
			in_brace = 1;
		}else if (c == '}'){
			brace_level--;
			if (brace_level == 0){
				play_list_len = pl_idx;
				in_brace = 0;
				
				printf("-- %s\r\n",play_list[0]);
				printf("-- %s\r\n",play_list[1]);
				printf("-- %s\r\n",play_list[2]);
				printf("-- %s\r\n",play_list[3]);
				
				return;
			}
		}
		
		if (in_brace){
			if (c == '"'){
				if (in_parenthesis){
					if (pl_c_idx > 10 &&
							play_list[pl_idx][pl_c_idx-1] == '3' &&
							play_list[pl_idx][pl_c_idx-2] == 'p' &&
							play_list[pl_idx][pl_c_idx-3] == 'm' &&
							play_list[pl_idx][pl_c_idx-4] == '.'){
						
						play_list[pl_idx][pl_c_idx] = '\0';
						pl_idx++;
					}
					pl_c_idx = 0;
				}
				in_parenthesis = 1 - in_parenthesis;
			}
			
			if (in_parenthesis && c != '"' && c != '\\'){
				if (pl_idx < 4) play_list[pl_idx][pl_c_idx++] = c;
			}
		}
	}
}

#define DNS_TABLE_LEN 10
#define DNS_ITEM_LEN 20
static char dns_table[DNS_TABLE_LEN][2][DNS_ITEM_LEN];
static int dns_idx = 0;
char *lookup(char *domain,int len){
	int i,j;
	for (i=0;i<dns_idx;i++){
		for (j=0;j<len;j++) {
			if (domain[j] != dns_table[i][0][j]){
				printf("%c vs %c",domain[j],dns_table[i][0][j]);
				break;
			}
		}
		if (j == len) {
			return dns_table[i][1];
		}
	}
	
	return 0;
}

u8* send_dnslookup(char *s,int len){
	int n;
	int org_n = 1000;
	char c;
	static u8 buf[30];
	static int buf_idx;
	u8 *ret;
	
	if ((ret=lookup(s,len)) != 0) {
		return ret;
	}
	
	printf("---\r\nAT+DNSLOOKUP=%s",s);
	send_s("AT+DNSLOOKUP=");
	send_s(s);
	send_c('\r');
	send_c('\n');
	
	n = 1000000;
	while (n--){
		if (wifi_available_len()){
			while (wifi_available_len() >0 ){
				c = wifi_receive();
				printf("%c",c);
				
				if ((buf_idx = insert_line_buf(buf,sizeof(buf),buf_idx,c)) == 0){
					if (buf[0] == 'I' && buf[1] == 'P' && buf[2] == ':'){
						int i = 3;
						while(buf[i] != '\r' && buf[i] != '\n'){
							dns_table[dns_idx][1][i-3] = buf[i];
							i++;
						}
						//printf("()()()%d\r\n",i);
						dns_table[dns_idx][1][i-3] = '\0';
						ret = dns_table[dns_idx][1];
						
						for(i=0;i<len;i++){
							dns_table[dns_idx][0][i] = s[i];
						}
						
						dns_idx++;
					}
				}
			};
			n = org_n;
		}
		delay(1);
	}
	
	return ret;
}

void init_fetch(){
	init_wifi(921600);
	
	GPIO_ResetBits(GPIOA, GPIO_Pin_14); 
	delay(300);
	GPIO_SetBits(GPIOA, GPIO_Pin_14);
	GPIO_ResetBits(GPIOA, GPIO_Pin_13); 
	
	send_cmd("AT");
	send_cmd("AT+WD");
	send_cmd("AT+NDHCP=1");
	send_cmd("ATE0");
	// send_cmd("ATB=921600");
	send_cmd("AT&R1");
	// send_cmd("AT&W0");
	// send_cmd("AT&Y0");
	// send_cmd("ATV1");
	
	// send_query("AT+WS");
	
	send_cmd("AT+WWPA=ashalahashalah");
	send_query("AT+WA=wowwowwow");
	
//	remain = 0; receive_sum = 0;
	//send_http();
	// send_query("AT+NSTAT=?");
	
// 	send_cmd("AT+HTTPCONF=20,Mozilla/5.0 (iPad; CPU OS 5_0 like Mac OS X) AppleWebKit/534.46 (KHTML, like Gecko) Version/5.1 Mobile/9A334 Safari/7534.48.3");
//   send_cmd("AT+HTTPCONF=3,close");
//   send_cmd("AT+HTTPCONF=11,192.168.19.115:8000");
//   send_query("AT+HTTPOPEN=192.168.19.115,8000");
//   
// 	
// 	send_play("AT+HTTPSEND=0,1,10,/in_my_song.mp3");
}