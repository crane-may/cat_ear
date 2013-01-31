#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define CHANGE_REG_NUM 49
#define n_NUM 2

const int change_reg[CHANGE_REG_NUM][n_NUM] = {
       {0x01, 0x0001},     /* select IFP address space */ 
        
       {0x05, 0x000b},  //  
       {0x06, 0x500c},  
       {0x25, 0x4912},     
       {0x62, 0x0000},   
       {0x08, 0xd800},         //rgb    
       {0x21, 0x5000},//0x21,
       {0xA7, 320    },//
       {0xAA, 240    },//
       {0x3a, 0x0000 },//
       
       {0x62, 0x0000 },// 
       {0x67, 0x4010 },//
       
       {0x34, 0x0000 },//
       {0x35, 0xff00 },//
        { 0x37, 0x0080 },//
        {0x2e, 0x1055 },//
      { 51, 5137 }, 
       { 57, 290  }, 
       { 59, 1068 },
       { 62, 4095 }, 
       { 89, 504  }, 
       { 90, 605  },
       { 92, 8222 }, 
       { 93, 10021}, 
       {100, 4477 },  /*  */
       {0x01, 0x0004},     /* select sensor address space */ 
       // {0x20, 0xffff},
       { 51, 132 }, 
       { 6, 10 }, 
       {0x16, 0x0000}, 
       {0x17, 0x0000}, 
       {0x18, 0x0000}, 
       {0x19, 0x0000}, 
       {0x1a, 0x0000}, 
       {0x1b, 0x0000}, 
       {0x1c, 0x0000}, 
       {0x1d, 0x0000}, 
       {0x30, 0x0000}, 
       {0x30, 0x0005}, 
       {0x31, 0x0000}, 
 
       {0x2b, 0x00a0},    
       {0x2c, 0x0020},     

       {0x2d, 0x0020},     
       {0x2e, 0x00a0},     
       {0x2f,  63414},//
       
       {0x07, 0x3002},//
       {0x09, 280   },//
       {0x21, 0xe401},
       {0x2f, 0xe7b6},/**/
};

int main(int argc, char **argv)
{
	int fd;
	char *fileName = "/dev/i2c-1";
	int  address = 0x48;
	unsigned char t;
    int i;
	
	if ((fd = open(fileName, O_RDWR)) < 0) {
		printf("Failed to open i2c port\n");
		exit(1);
	}
	if (ioctl(fd, I2C_SLAVE, address) < 0) {
		printf("Unable to get bus access to talk to slave\n");
		exit(1);
	}
    
	for (i=0;i<CHANGE_REG_NUM;i++){
        t = change_reg[i][0];
        if ((write(fd, &t, 1)) != 1) {	printf("Error writing to i2c slave\n");exit(1);}
        t = (change_reg[i][1] & 0xf0) >> 8;
        if ((write(fd, &t, 1)) != 1) {	printf("Error writing to i2c slave\n");exit(1);}
        t = change_reg[i][1] & 0x0f;
        if ((write(fd, &t, 1)) != 1) {	printf("Error writing to i2c slave\n");exit(1);}
	}
	return 0;
}