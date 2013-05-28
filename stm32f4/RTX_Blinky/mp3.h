#ifndef __MP3_H__
#define __MP3_H__

#define xCs GPIO_Pin_2
#define xReset GPIO_Pin_1
#define dReq GPIO_Pin_15
#define xDcs GPIO_Pin_0

uint8_t SPI3_ReadWriteByte(uint8_t TxData);
void init_mp3();
void Mp3Reset();
int mp3_need_data();
void mp3_input_data(u8* s);
void mute();
int calc_frame_size(u8* header);
#endif