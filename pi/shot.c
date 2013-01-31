#include <wiringPi.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>  
#include <unistd.h>

int timeval_subtract(struct timeval* result, struct timeval* x, struct timeval* y) {
    int nsec;   
    if (x->tv_sec>y->tv_sec) return -1;   
    if ((x->tv_sec==y->tv_sec) && (x->tv_usec>y->tv_usec)) return -1;   

    result->tv_sec = (y->tv_sec-x->tv_sec);   
    result->tv_usec = (y->tv_usec-x->tv_usec);   

    if (result->tv_usec<0){   
        result->tv_sec--;   
        result->tv_usec+=1000000;   
    }
    return 0;   
}

#define FIFO_CS_BIT     2  
#define FIFO_RRST_BIT   3 
#define FIFO_RD_BIT     7
#define FIFO_WE_BIT     0

#define FIFO_CS_H()    digitalWrite(FIFO_CS_BIT,1)
#define FIFO_CS_L()    digitalWrite(FIFO_CS_BIT,0)

#define FIFO_RRST_H()  digitalWrite(FIFO_RRST_BIT,1)
#define FIFO_RRST_L()  digitalWrite(FIFO_RRST_BIT,0)

#define FIFO_RD_H()    digitalWrite(FIFO_RD_BIT,1)
#define FIFO_RD_L()    digitalWrite(FIFO_RD_BIT,0)

#define FIFO_WE_H()    digitalWrite(FIFO_WE_BIT,1)
#define FIFO_WE_L()    digitalWrite(FIFO_WE_BIT,0)

#define VSYNC digitalRead(12)

#define D0 15
#define D1 16
#define D2 1
#define D3 4
#define D4 5
#define D5 6
#define D6 13
#define D7 14

void init(){
    pinMode(12, INPUT);
    pinMode(FIFO_CS_BIT, OUTPUT);
    pinMode(FIFO_RRST_BIT, OUTPUT);
    pinMode(FIFO_RD_BIT, OUTPUT);
    pinMode(FIFO_WE_BIT, OUTPUT);
    pinMode(D0, OUTPUT);
    pinMode(D1, OUTPUT);
    pinMode(D2, OUTPUT);
    pinMode(D3, OUTPUT);
    pinMode(D4, OUTPUT);
    pinMode(D5, OUTPUT);
    pinMode(D6, OUTPUT);
    pinMode(D7, OUTPUT);
}

uint8_t FIFO_BYTE(){
    return  (digitalRead(D7) << 7) | \
            (digitalRead(D6) << 6) | \
            (digitalRead(D5) << 5) | \
            (digitalRead(D4) << 4) | \
            (digitalRead(D3) << 3) | \
            (digitalRead(D2) << 2) | \
            (digitalRead(D1) << 1) | \
            digitalRead(D0);
}

int main() {
	uint32_t c_data,r,g,b,i;
    uint8_t buf[230400];
    
    if (wiringPiSetup() == -1) {
        printf("pi setup error\n");
        exit(1);
    }
    init();
    
    FIFO_CS_L();
    FIFO_WE_L();
	
	FIFO_RRST_L(); 
    FIFO_RD_L();
    FIFO_RD_H();
    FIFO_RD_L();
    FIFO_RRST_H();
    FIFO_RD_H();
	usleep(5000);
	
	
	
	FIFO_RRST_L(); 
    FIFO_RD_L();
    FIFO_RD_H();
    FIFO_RD_L();
    FIFO_RRST_H();
    FIFO_RD_H();
	
	printf("start shot...\n");
	
	for(i=0;i<76800;i++){
		FIFO_RD_L();
		c_data = FIFO_BYTE() << 8;
		FIFO_RD_H();
		FIFO_RD_L();
		c_data |= FIFO_BYTE();
		FIFO_RD_H();
		
		r = (c_data & 0xf800) >> 8;
		g = (c_data & 0x07e0) >> 3;
		b = (c_data & 0x001f) << 3;
		
        buf[i*3] = r;
        buf[i*3+1] = g;
        buf[i*3+2] = b;
        
		//c_data = (r*76 + g*150 + b*30) >> 8;
	}
    
    FILE *f;
    f = fopen("rgb_buf","wb");
    fwrite(buf,1,230400,f);
    fclose(f);
	
	printf("shot over\r\n");

    return 0 ;
}