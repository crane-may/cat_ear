#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_spi.h"
#include <misc.h>   
#include <zbar.h>
#include <stdio.h>
#include <stm32f4xx_usart.h>
#include "console_usart.h"
#include "ring_buffer.h"
#include "mp3.h"
#include "timer.h"
#include "sd.h"
#include "wifi.h"

struct song{
	int content_length;
	int content_length_without_id3v2;
	
	int content_remain;
	
	int bitrate;
	int sampling_rate;
	int padding_bit;
	
	u8 first_head[4];
	int headed;
	int frame_size;
	int read_byte;
	
	u8 buffer[60];
	int buffer_idx;
	
	int cur_frame_remain;
};

struct song cur_song;

void print_c(u8 c){
	if (c == 0x1b)
		printf("<ESC>");
	else
		printf("%c",c);
}

void mp3_c(u8 c){
	int i;
	static int over=0;
	
	if (cur_song.content_length == 0 || cur_song.content_remain == -1){
		print_c(c);
		if (c != '\n'){
			if (cur_song.buffer_idx < sizeof(cur_song.buffer)){
				cur_song.buffer[cur_song.buffer_idx++] = c;
			}
		}else{
			char * s = cur_song.buffer;
			
			if (s[0] == 'C' &&
					s[1] == 'o' &&
					s[2] == 'n' &&
					s[3] == 't' &&
					s[4] == 'e' &&
					s[5] == 'n' &&
					s[6] == 't' &&
					s[7] == '-' &&
					s[8] == 'L' &&
					s[9] == 'e' &&
					s[10]== 'n' &&
					s[11]== 'g' &&
					s[12]== 't' &&
					s[13]== 'h' &&
					s[14]== ':'){
				i = 16;
				while (i < 50 && s[i] >= '0' && s[i] <= '9'){
					cur_song.content_length = cur_song.content_length * 10 + (s[i] - '0');
					i++;
				}
				printf("-----content_length: %d\r\n",cur_song.content_length);
			}
			
			if (s[0]='\r' && cur_song.buffer_idx ==1 && cur_song.content_length > 0){
				cur_song.content_remain = cur_song.content_length;
				println("content_start");
			}
			
			cur_song.buffer_idx = 0;
		}
	}else{
		if (cur_song.content_remain > 0){
			// printf("%d\r\n",cur_song.content_remain);
			cur_song.content_remain--;
			cur_song.buffer[cur_song.buffer_idx++] = c;
			
			if (cur_song.content_length_without_id3v2 == 0 && cur_song.buffer_idx > 9){
				if (cur_song.buffer[0] == 0x49 &&
					  cur_song.buffer[1] == 0x44 &&
					  cur_song.buffer[2] == 0x33 ){
					cur_song.content_length_without_id3v2 = cur_song.content_length - 
							((cur_song.buffer[6]&0x7F)*0x200000+(cur_song.buffer[7]&0x7F)*0x4000+(cur_song.buffer[8]&0x7F)*0x80+(cur_song.buffer[9]&0x7F) + 10);
				}else{
					cur_song.content_length_without_id3v2 = cur_song.content_length;
					cur_song.first_head[0] = cur_song.buffer[0];
					cur_song.first_head[1] = cur_song.buffer[1];
					cur_song.first_head[2] = cur_song.buffer[2];
					cur_song.first_head[3] = cur_song.buffer[3];
					cur_song.headed = 4;
				}
			}
			
			if (cur_song.headed < 4 && cur_song.content_length_without_id3v2 > cur_song.content_remain ){
				cur_song.first_head[cur_song.headed++] = c;
			}
			
			if (cur_song.headed >= 4 && cur_song.frame_size == 0){
				for(i=0;i<4;i++) printf("0x%02x ",cur_song.first_head[i]);
				cur_song.frame_size = calc_frame_size(cur_song.first_head);
				
				printf("frame_size: %d\r\n",cur_song.frame_size);
			}
			
		}else{
			println("shot over");
			printf("content_over %d\r\n",over++);
		}
	}
}


int mp3_buffer_len(){
	if (cur_song.content_length == 0 || cur_song.content_remain <= 0){
		return 0;
	}else{
		return cur_song.buffer_idx;
	}
}

int fill_mp3_buffer(){
	int i,j;
	int len = wifi_available_len();
	char c;
	static char tar[] = {0x1b,0x45,0x1b,0x53,0x30};
	static char buf[8];
	static int buf_idx = 0;

	if (len > 0){
		//printf("-%d\r\n",len);
		c = wifi_receive();
		buf[buf_idx++] = c;
		
		if (buf[buf_idx - 1] != tar[buf_idx - 1]){
			if (c != 0x1b){
				for (j=0;j<buf_idx;j++) mp3_c(buf[j]);
				buf_idx = 0;
			}else{
				for (j=0;j<buf_idx-1;j++) mp3_c(buf[j]);
				buf_idx = 0;
				buf[buf_idx++] = c;
			}
		}else{
			if (buf_idx >= 5) buf_idx = 0;
		}
	};
	
	return mp3_buffer_len();
}

static int is_playing = 0;

void loop(){
	int i;
	static u8 zero[32] = {0};
	
	if (is_playing){
		int buf_len = mp3_buffer_len();
		static int last_second = -1;
		int second;
		
		store_buffer();
		
		if (buf_len < 32){
			for (i=0;i<8;i++) fill_mp3_buffer();
		}else if (mp3_need_data()){
			if (wifi_available_len() < 30 && last_second < 2){
				printf("oooooverrrrrrrrr\r\n");
				while (wifi_available_len() > 0) wifi_receive();
				mp3_input_data(zero);
				
				cur_song_init();
				is_playing = 0;
				return;
			}
			
			mp3_input_data(cur_song.buffer);
			for (i=32;i<cur_song.buffer_idx;i++){
				cur_song.buffer[i-32] = cur_song.buffer[i];
			}
			cur_song.buffer_idx -= 32;
			
			
			if (cur_song.frame_size > 0){
				second = cur_song.content_remain / cur_song.frame_size * 26 /1000;
				if (second != last_second)	printf("time : -%d\r\n",second);
				last_second = second;
			}
		}
	}else{
		char * sg;
		sg = next_song();
	
		printf(">>> %s\r\n",sg);
		send_fetch_song(sg);
		
		is_playing = 1;
	}
}


int cur_song_init(){
	cur_song.content_length = 0;
	cur_song.buffer_idx = 0;
	cur_song.content_length_without_id3v2 = 0;
	cur_song.content_remain = -1;
	cur_song.frame_size = 0;
	cur_song.headed = 0;
}

int main() {
	int i=0;
	int ret,j;
	static u8 data[512];
	
	
	cur_song_init();
	init_usart();
	initSystick();
	delay(1000);
	printf("\r\n\r\n-------- start -------\r\n");
	
//	shot();
//	while (1);
	
	initSpi();
	SD_Init();
	readBlock(0,data);
	
	init_mp3();
	Mp3Reset();
	mute();
	init_fetch();
	
  //send_fetch_play_list();
  while (1) loop();

// 	println("start shot...");
// 	for(i=1;i<=3573;i++){
// 		readBlock(i,data);
// 		for(j=0;j<512;j++) {
// 			printf("%c",data[j]);
// 		}
// 	}
// 	println("shot over");

// 	println("--- 0");
//  	ret = get_millisecond();
//  	for(i=1;i<1000;i++)
//  		writeBlock(i,data);
// 	readBlock(990,data);
//  	printf("--- %d\r\n",get_millisecond() - ret);
}
