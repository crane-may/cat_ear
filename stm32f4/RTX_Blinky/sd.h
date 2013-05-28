#ifndef __SD_H__
#define __SD_H__

#define CMD0 0x00
#define CMD1 0x01
#define CMD8 0x08
#define CMD9 0x09
#define CMD10 0x0A
#define CMD12 0x0C
#define CMD13 0x0D
#define CMD16 0x10
#define CMD17 0x11
#define CMD24 0x18
#define CMD32 0x20
#define CMD33 0x21
#define CMD38 0x26
#define CMD41 0x29
#define CMD55 0x37
#define CMD58 0x3A
#define CMD59 0x3B

void SPI_SetSpeed(u8 SpeedSet);
void initSpi(void) ;
u8 SD_Init();
uint8_t writeBlock(uint32_t blockNumber, const uint8_t* src);
uint8_t readBlock(uint32_t block, uint8_t* dst);

#endif