#include <wiringPi.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>  
#include <unistd.h>
#include <sched.h>

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

struct timespec sleeper, sleeper2, dummy ;

void print_led(int mrx[]) {
    int i,j,t;
    for (j=0;j<16;j++){
        t = mrx[j];
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

int cry[] = {
0b0000000000000000,
0b0000000000000000,
0b0000100001110000,
0b0000010001110000,
0b0000001001110000,
0b0000001000000000,
0b0000001000000000,
                  
0b0000001000000000,
0b0000001000000000,
0b0000001001110000,
0b0000010001110000,
0b0000100001110000,
0b0000000000000000,
0b0000000000000000,
0b0000000000000000,
0b0000000000000000
};

int right[] = {
0b0000000000000000,
0b0000000000000000,
0b0000000000000000,
0b0000000000000000,
0b0000000000000000,
0b0000000100000000,
0b0000001000000000,
0b0000010000000000,

0b0000001000000000,
0b0000000100000000,
0b0000000010000000,
0b0000000001000000,
0b0000000000100000,
0b0000000000000000,
0b0000000000000000,
0b0000000000000000
};

int stop[] = {
0b0000000000000000,
0b0000000000000000,
0b0000000000000000,
0b0000011111110000,
0b0000011111110000,
0b0000000000000000,
0b0000000000000000,
0b0000000000000000,

0b0000011111110000,
0b0000011111110000,
0b0000000000000000,
0b0000000000000000,
0b0000000000000000,
0b0000000000000000,
0b0000000000000000,
0b0000000000000000
};

int qrcode[] = {
0b1001111001111111,
0b0111010001000001,
0b0010011101011101,
0b0000011101011101,
0b0111100001011101,
0b1000011001000001,
0b0101010101111111,
0b0011111100000000,

0b1101001111010000,
0b0000001010010101,
0b0111101001100011,
0b0100010110100000,
0b1010001101101101,
0b0100111110110111,
0b1001001011001001,
0b0100000100101111
};

void print_loading(int n) {
  int i,j_,t;
  j_ = n % 18;
  
  t = j_ <= 15 ? 0x7F0 : 0x0;
  digitalWrite(P_LAT,0);
  digitalWrite(P_CLK,0);
  for (i=0;i<16;i++) {
      digitalWrite(P_DATA, !(t & 1));
      digitalWrite(P_CLK,1);
      digitalWrite(P_CLK,0);
      t >>= 1;
  }
  digitalWrite(P_OE,1);
  digitalWrite(ADDR_A,j_ & 0x1);
  digitalWrite(ADDR_B,j_ & 0x2);
  digitalWrite(ADDR_C,j_ & 0x4);
  digitalWrite(ADDR_D,j_ & 0x8);
  
  digitalWrite(P_LAT,1);
  digitalWrite(P_OE,0);
  digitalWrite(P_LAT,0);
  nanosleep(&sleeper2, &dummy);
}

int numbers[][4] = {
//0
{
0b0111110,
0b1000001,
0b1000001,
0b0111110
},
//1
{
0b0000000,
0b1000010,
0b1111111,
0b1000000
},
//2
{
0b1100110,
0b1010001,
0b1001001,
0b1000110
},
//3
{
0b0100010,
0b1001001,
0b1001001,
0b0110110
},
//4
{
0b0011100,
0b0010010,
0b1111111,
0b0010000
},
//5
{
0b0100111,
0b1000101,
0b1000101,
0b0111001
},
//6
{
0b0111100,
0b1001010,
0b1001001,
0b0110000
},
//7
{
0b0000001,
0b1110001,
0b0001001,
0b0000111
},
//8
{
0b0110110,
0b1001001,
0b1001001,
0b0110110
},
//9
{
0b0000110,
0b1001001,
0b0101001,
0b0011110
},
//!
{
0b0000000,
0b1011111,
0b1011111,
0b0000000
}
};

void print_time(int n){
  int i,j,t;
  if (n < 0) n = 0;
  int a = n / 60;
  int b = (n % 60) / 10;
  int c = n % 10;
  if (a > 10) a = 10;
  
  for (j=0;j<16;j++){
      if (j == 4 || j == 6 || j == 11) {
        t = 0x0;
      }else if (j<4) {
        t = numbers[a][j] << 4;
      }else if (j == 5) {
        t = 0x140;
      }else if (j < 11) {
        t = numbers[b][j-7] << 4;
      }else if (j < 16) {
        t = numbers[c][j-12] << 4;
      }
      
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

void main(){
    if (wiringPiSetup() == -1) {printf("pi setup error\n");exit(1);}
    init_led_pins();
    sleeper.tv_sec  = 0;
    sleeper.tv_nsec  = 1 *   1000000 ;
    sleeper2.tv_sec  = 0;
    sleeper2.tv_nsec = 1 * 200000000 ;
    
    unsigned char c_ne;
    unsigned char c_nw;
    unsigned char c_se;
    unsigned char c_sw;
    
    int rc,old_scheduler_policy;
    struct sched_param my_params;

    old_scheduler_policy=sched_getscheduler(0);

    my_params.sched_priority=sched_get_priority_max(SCHED_RR);
    rc=sched_setscheduler(0,SCHED_RR,&my_params);
    if(rc<0){
       perror("sched_setscheduler to SCHED_RR error");
       exit(0);
    }
    old_scheduler_policy=sched_getscheduler(0);
    rc=sched_setscheduler(0,SCHED_FIFO,&my_params);
    if(rc<0){
       perror("sched_setscheduler to SCHED_FIFO error");
       exit(0);
    }
    old_scheduler_policy=sched_getscheduler(0);
    
    FILE *f;
    int i = 0;
    int rest = 0;
    char buf[255];
    int status = 0;  // 0:loading, 1:playing_time, 2:stop, 3:cry, 4:qrcode, 5:qr_success
    int time = 0;
    while (1) {
        if (rest <= 0) status = 0;
        
        if(status == 0 && (f=fopen("/dev/shm/led_camera.buf","rb"))!=NULL){
          fgets(buf, 255, f);
          if (buf[0] == '2') {
            status = 5;
          } else if (buf[0] == '1' ) {
            status = 4;
          }
          fclose(f);
        }
        
        if(status == 0 && (f=fopen("/dev/shm/led_network.buf","rb"))!=NULL){
          fgets(buf, 255, f);
          if (buf[0] == '1' ) {
            status = 3;
          }
          fclose(f);
        }
        
        if(status == 0 && (f=fopen("/dev/shm/led_ce.buf","rb"))!=NULL){
          fgets(buf, 255, f);
          if (strlen(buf) > 0) {
            if (buf[0] == 'p'){
              status = 2;
            }else{
              status = 1;
              sscanf(buf, "%d", &time);
            }
          }
          fclose(f);
        }
        
        switch(status){
        case 0:
          print_loading(i);
          break;
        case 1:
          print_time(time);
          if (rest <= 0) rest = 20;
          break;
        case 2:
          print_led(stop);
          if (rest <= 0) rest = 20;
          break;
        case 3:
          print_led(cry);
          if (rest <= 0) rest = 20;
          break;
        case 4:
          print_led(qrcode);
          if (rest <= 0) rest = 20;
          break;
        case 5:
          print_led(right);
          if (rest <= 0) rest = 20;
          break;
        }
        
        i++;
        rest--;
    }
}

void load_ESWN(){
  FILE *f;
  int j;
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