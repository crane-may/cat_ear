#include <wiringPi.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>  
#include <unistd.h>

#define ADDR_A  2
#define ADDR_B  0
#define ADDR_C  7
#define ADDR_D  9
#define P_LAT   14
#define P_CLK   13
#define P_DATA  12
#define P_OE    3

void init_led_pins(){
    pinMode(ADDR_A, OUTPUT);
    pinMode(ADDR_B, OUTPUT);
    pinMode(ADDR_C, OUTPUT);
    pinMode(ADDR_D, OUTPUT);
    pinMode(P_LAT , OUTPUT);
    pinMode(P_CLK , OUTPUT);
    pinMode(P_OE  , OUTPUT);
    pinMode(P_DATA, OUTPUT);
}

struct timespec sleeper, dummy ;

void print_led(int mrx[]) {
    int i,j,t;
    for (j=0;j<16;j++){
        t = mrx[15 - j];
        digitalWrite(P_LAT,0);
        digitalWrite(P_CLK,0);
        for (i=0;i<16;i++) {
            digitalWrite(P_DATA, !(t & 1));
            digitalWrite(P_CLK,1);
            digitalWrite(P_CLK,0);
            t >>= 1;
        }
        digitalWrite(P_OE,1);
        digitalWrite(ADDR_A,j & 0x1);
        digitalWrite(ADDR_B,j & 0x2);
        digitalWrite(ADDR_C,j & 0x4);
        digitalWrite(ADDR_D,j & 0x8);
        
        digitalWrite(P_LAT,1);
        digitalWrite(P_OE,0);
        digitalWrite(P_LAT,0);
        nanosleep(&sleeper, &dummy);
    }
}

int NE[] = {
0b00000000,
0b01111110,
0b01100010,
0b01010010,
0b01001010,
0b01000110,
0b01111110,
0b00000000
};
int NW[] = {
0b00000000,
0b01111110,
0b01100010,
0b01010010,
0b01001010,
0b01000110,
0b01111110,
0b00000000
};
int SE[] = {
0b00000000,
0b01111110,
0b01100010,
0b01010010,
0b01001010,
0b01000110,
0b01111110,
0b00000000
};
int SW[] = {
0b00000000,
0b01111110,
0b01100010,
0b01010010,
0b01001010,
0b01000110,
0b01111110,
0b00000000
};

int B_table[] = {
0b00000001,
0b00000010,
0b00000100,
0b00001000,
0b00010000,
0b00100000,
0b01000000,
0b10000000,
};

void print_ESWN() {
    int i,j;
    for (j=0;j<16;j++){
        digitalWrite(P_LAT,0);
        digitalWrite(P_CLK,0);
        for (i=0;i<16;i++) {
            if     (i < 8 && j < 8){ digitalWrite(P_DATA, ! (NE[i  ] & B_table[ 7-j]));}
            else if(i < 8 && j >=8){ digitalWrite(P_DATA, ! (NW[i  ] & B_table[15-j]));}
            else if(i >=8 && j < 8){ digitalWrite(P_DATA, ! (SE[i-8] & B_table[ 7-j]));}
            else if(i >=8 && j >=8){ digitalWrite(P_DATA, ! (SW[i-8] & B_table[15-j]));}
            
            digitalWrite(P_CLK,1);
            digitalWrite(P_CLK,0);
        }
        digitalWrite(P_OE,1);
        digitalWrite(ADDR_A,j & 0x1);
        digitalWrite(ADDR_B,j & 0x2);
        digitalWrite(ADDR_C,j & 0x4);
        digitalWrite(ADDR_D,j & 0x8);
        
        digitalWrite(P_LAT,1);
        digitalWrite(P_OE,0);
        digitalWrite(P_LAT,0);
        nanosleep(&sleeper, &dummy);
    }
}

int gra2[] = {
0b0000000011100000,
0b0000000100010000,
0b0000001000001000,
0b0000010000000100,
0b0000100000000100,
0b0001000000000100,
0b0010000000001000,
0b0100000000010000,

0b1100000000110000,
0b0010000000001000,
0b0001000000000100,
0b0000100000000100,
0b0000010000000100,
0b0000001000001000,
0b0000000100010000,
0b0000000011100000,
};

void main(){
    if (wiringPiSetup() == -1) {printf("pi setup error\n");exit(1);}
    init_led_pins();
    sleeper.tv_sec  = 0;
    sleeper.tv_nsec = 1 * 1000000 ;
    
    FILE *f;
    unsigned char c_ne;
    unsigned char c_nw;
    unsigned char c_se;
    unsigned char c_sw;
    
    int i = 0,j;
    while (1) {
        //print_led(gra2);
        print_ESWN();
        
        if (i % 10 == 0) {
            if((f=fopen("/dev/shm/led_ne.buf","rb"))!=NULL){
                for (j=0;j<8;j++) NE[j] = fgetc(f);
                fclose(f);
            }
            if((f=fopen("/dev/shm/led_nw.buf","rb"))!=NULL){
                for (j=0;j<8;j++) NW[j] = fgetc(f);
                fclose(f);
            }
            if((f=fopen("/dev/shm/led_se.buf","rb"))!=NULL){
                for (j=0;j<8;j++) SE[j] = fgetc(f);
                fclose(f);
            }
            if((f=fopen("/dev/shm/led_sw.buf","rb"))!=NULL){
                for (j=0;j<8;j++) SW[j] = fgetc(f);
                fclose(f);
            }
        }
        i++;
    }
}