#ifndef __WIFI_H__
#define __WIFI_H__
int wifi_available_len(void);
u8 wifi_receive(void);
void init_fetch(void);
void store_buffer(void);
void send_fetch_play_list(void);
u8* send_dnslookup(char *s,int len);
char *next_song(void);
#endif