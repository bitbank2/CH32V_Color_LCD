/*
 * spi_lcd.c
 * a ST7735S LCD display library
 *  Created on: Sep 11, 2023
 *      written by Larry Bank
 */
#include "Arduino.h"
#include "spi_lcd.h"

static uint8_t u8CS, u8DC, u8BL;
static int iCursorX, iCursorY;
// Kludge - the system is reporting the wrong amount of SRAM (10K) when it is 20K, so hard code the addresses
#define u8Cache0 (SRAM_BASE+0x2800)
#define u8Cache1 (SRAM_BASE+0x3c00)
static uint8_t *pCache0 = (uint8_t *)u8Cache0;
static uint8_t *pCache1 = (uint8_t *)u8Cache1;
//static uint8_t u8Cache0[CACHE_SIZE]; // ping-pong data buffers
//static uint8_t u8Cache1[CACHE_SIZE];
//static uint8_t *pCache0 = u8Cache0, *pCache1 = u8Cache1;
volatile int bDMA = 0;

const unsigned char uc80InitList[] = {
    2, 0x3a, 0x05,    // pixel format RGB565
    2, 0x36, 0x68, // MADCTL (0/90/180/270 and color/inversion)
    17, 0xe0, 0x09, 0x16, 0x09,0x20,
    0x21,0x1b,0x13,0x19,
    0x17,0x15,0x1e,0x2b,
    0x04,0x05,0x02,0x0e, // gamma sequence
    17, 0xe1, 0x0b,0x14,0x08,0x1e,
    0x22,0x1d,0x18,0x1e,
    0x1b,0x1a,0x24,0x2b,
    0x06,0x06,0x02,0x0f,
    1, 0x20,    // display inversion off
	1, 0x29, // display on
    0
};

const uint8_t ucFont[]PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x5f,0x5f,0x06,0x00,
  0x00,0x07,0x07,0x00,0x07,0x07,0x00,0x14,0x7f,0x7f,0x14,0x7f,0x7f,0x14,
  0x24,0x2e,0x2a,0x6b,0x6b,0x3a,0x12,0x46,0x66,0x30,0x18,0x0c,0x66,0x62,
  0x30,0x7a,0x4f,0x5d,0x37,0x7a,0x48,0x00,0x04,0x07,0x03,0x00,0x00,0x00,
  0x00,0x1c,0x3e,0x63,0x41,0x00,0x00,0x00,0x41,0x63,0x3e,0x1c,0x00,0x00,
  0x08,0x2a,0x3e,0x1c,0x3e,0x2a,0x08,0x00,0x08,0x08,0x3e,0x3e,0x08,0x08,
  0x00,0x00,0x80,0xe0,0x60,0x00,0x00,0x00,0x08,0x08,0x08,0x08,0x08,0x08,
  0x00,0x00,0x00,0x60,0x60,0x00,0x00,0x60,0x30,0x18,0x0c,0x06,0x03,0x01,
  0x3e,0x7f,0x59,0x4d,0x47,0x7f,0x3e,0x40,0x42,0x7f,0x7f,0x40,0x40,0x00,
  0x62,0x73,0x59,0x49,0x6f,0x66,0x00,0x22,0x63,0x49,0x49,0x7f,0x36,0x00,
  0x18,0x1c,0x16,0x53,0x7f,0x7f,0x50,0x27,0x67,0x45,0x45,0x7d,0x39,0x00,
  0x3c,0x7e,0x4b,0x49,0x79,0x30,0x00,0x03,0x03,0x71,0x79,0x0f,0x07,0x00,
  0x36,0x7f,0x49,0x49,0x7f,0x36,0x00,0x06,0x4f,0x49,0x69,0x3f,0x1e,0x00,
  0x00,0x00,0x00,0x66,0x66,0x00,0x00,0x00,0x00,0x80,0xe6,0x66,0x00,0x00,
  0x08,0x1c,0x36,0x63,0x41,0x00,0x00,0x00,0x14,0x14,0x14,0x14,0x14,0x14,
  0x00,0x41,0x63,0x36,0x1c,0x08,0x00,0x00,0x02,0x03,0x59,0x5d,0x07,0x02,
  0x3e,0x7f,0x41,0x5d,0x5d,0x5f,0x0e,0x7c,0x7e,0x13,0x13,0x7e,0x7c,0x00,
  0x41,0x7f,0x7f,0x49,0x49,0x7f,0x36,0x1c,0x3e,0x63,0x41,0x41,0x63,0x22,
  0x41,0x7f,0x7f,0x41,0x63,0x3e,0x1c,0x41,0x7f,0x7f,0x49,0x5d,0x41,0x63,
  0x41,0x7f,0x7f,0x49,0x1d,0x01,0x03,0x1c,0x3e,0x63,0x41,0x51,0x33,0x72,
  0x7f,0x7f,0x08,0x08,0x7f,0x7f,0x00,0x00,0x41,0x7f,0x7f,0x41,0x00,0x00,
  0x30,0x70,0x40,0x41,0x7f,0x3f,0x01,0x41,0x7f,0x7f,0x08,0x1c,0x77,0x63,
  0x41,0x7f,0x7f,0x41,0x40,0x60,0x70,0x7f,0x7f,0x0e,0x1c,0x0e,0x7f,0x7f,
  0x7f,0x7f,0x06,0x0c,0x18,0x7f,0x7f,0x1c,0x3e,0x63,0x41,0x63,0x3e,0x1c,
  0x41,0x7f,0x7f,0x49,0x09,0x0f,0x06,0x1e,0x3f,0x21,0x31,0x61,0x7f,0x5e,
  0x41,0x7f,0x7f,0x09,0x19,0x7f,0x66,0x26,0x6f,0x4d,0x49,0x59,0x73,0x32,
  0x03,0x41,0x7f,0x7f,0x41,0x03,0x00,0x7f,0x7f,0x40,0x40,0x7f,0x7f,0x00,
  0x1f,0x3f,0x60,0x60,0x3f,0x1f,0x00,0x3f,0x7f,0x60,0x30,0x60,0x7f,0x3f,
  0x63,0x77,0x1c,0x08,0x1c,0x77,0x63,0x07,0x4f,0x78,0x78,0x4f,0x07,0x00,
  0x47,0x63,0x71,0x59,0x4d,0x67,0x73,0x00,0x7f,0x7f,0x41,0x41,0x00,0x00,
  0x01,0x03,0x06,0x0c,0x18,0x30,0x60,0x00,0x41,0x41,0x7f,0x7f,0x00,0x00,
  0x08,0x0c,0x06,0x03,0x06,0x0c,0x08,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
  0x00,0x00,0x03,0x07,0x04,0x00,0x00,0x20,0x74,0x54,0x54,0x3c,0x78,0x40,
  0x41,0x7f,0x3f,0x48,0x48,0x78,0x30,0x38,0x7c,0x44,0x44,0x6c,0x28,0x00,
  0x30,0x78,0x48,0x49,0x3f,0x7f,0x40,0x38,0x7c,0x54,0x54,0x5c,0x18,0x00,
  0x48,0x7e,0x7f,0x49,0x03,0x06,0x00,0x98,0xbc,0xa4,0xa4,0xf8,0x7c,0x04,
  0x41,0x7f,0x7f,0x08,0x04,0x7c,0x78,0x00,0x44,0x7d,0x7d,0x40,0x00,0x00,
  0x60,0xe0,0x80,0x84,0xfd,0x7d,0x00,0x41,0x7f,0x7f,0x10,0x38,0x6c,0x44,
  0x00,0x41,0x7f,0x7f,0x40,0x00,0x00,0x7c,0x7c,0x18,0x78,0x1c,0x7c,0x78,
  0x7c,0x78,0x04,0x04,0x7c,0x78,0x00,0x38,0x7c,0x44,0x44,0x7c,0x38,0x00,
  0x84,0xfc,0xf8,0xa4,0x24,0x3c,0x18,0x18,0x3c,0x24,0xa4,0xf8,0xfc,0x84,
  0x44,0x7c,0x78,0x4c,0x04,0x0c,0x18,0x48,0x5c,0x54,0x74,0x64,0x24,0x00,
  0x04,0x04,0x3e,0x7f,0x44,0x24,0x00,0x3c,0x7c,0x40,0x40,0x3c,0x7c,0x40,
  0x1c,0x3c,0x60,0x60,0x3c,0x1c,0x00,0x3c,0x7c,0x60,0x30,0x60,0x7c,0x3c,
  0x44,0x6c,0x38,0x10,0x38,0x6c,0x44,0x9c,0xbc,0xa0,0xa0,0xfc,0x7c,0x00,
  0x4c,0x64,0x74,0x5c,0x4c,0x64,0x00,0x08,0x08,0x3e,0x77,0x41,0x41,0x00,
  0x00,0x00,0x00,0x77,0x77,0x00,0x00,0x41,0x41,0x77,0x3e,0x08,0x08,0x00,
  0x02,0x03,0x01,0x03,0x02,0x03,0x01,0x70,0x78,0x4c,0x46,0x4c,0x78,0x70};
  // 5x7 font (in 6x8 cell)
const uint8_t ucSmallFont[] PROGMEM = {
0x00,0x00,0x00,0x00,0x00,
0x00,0x06,0x5f,0x06,0x00,
0x07,0x03,0x00,0x07,0x03,
0x24,0x7e,0x24,0x7e,0x24,
0x24,0x2b,0x6a,0x12,0x00,
0x63,0x13,0x08,0x64,0x63,
0x36,0x49,0x56,0x20,0x50,
0x00,0x07,0x03,0x00,0x00,
0x00,0x3e,0x41,0x00,0x00,
0x00,0x41,0x3e,0x00,0x00,
0x08,0x3e,0x1c,0x3e,0x08,
0x08,0x08,0x3e,0x08,0x08,
0x00,0xe0,0x60,0x00,0x00,
0x08,0x08,0x08,0x08,0x08,
0x00,0x60,0x60,0x00,0x00,
0x20,0x10,0x08,0x04,0x02,
0x3e,0x51,0x49,0x45,0x3e,
0x00,0x42,0x7f,0x40,0x00,
0x62,0x51,0x49,0x49,0x46,
0x22,0x49,0x49,0x49,0x36,
0x18,0x14,0x12,0x7f,0x10,
0x2f,0x49,0x49,0x49,0x31,
0x3c,0x4a,0x49,0x49,0x30,
0x01,0x71,0x09,0x05,0x03,
0x36,0x49,0x49,0x49,0x36,
0x06,0x49,0x49,0x29,0x1e,
0x00,0x6c,0x6c,0x00,0x00,
0x00,0xec,0x6c,0x00,0x00,
0x08,0x14,0x22,0x41,0x00,
0x24,0x24,0x24,0x24,0x24,
0x00,0x41,0x22,0x14,0x08,
0x02,0x01,0x59,0x09,0x06,
0x3e,0x41,0x5d,0x55,0x1e,
0x7e,0x11,0x11,0x11,0x7e,
0x7f,0x49,0x49,0x49,0x36,
0x3e,0x41,0x41,0x41,0x22,
0x7f,0x41,0x41,0x41,0x3e,
0x7f,0x49,0x49,0x49,0x41,
0x7f,0x09,0x09,0x09,0x01,
0x3e,0x41,0x49,0x49,0x7a,
0x7f,0x08,0x08,0x08,0x7f,
0x00,0x41,0x7f,0x41,0x00,
0x30,0x40,0x40,0x40,0x3f,
0x7f,0x08,0x14,0x22,0x41,
0x7f,0x40,0x40,0x40,0x40,
0x7f,0x02,0x04,0x02,0x7f,
0x7f,0x02,0x04,0x08,0x7f,
0x3e,0x41,0x41,0x41,0x3e,
0x7f,0x09,0x09,0x09,0x06,
0x3e,0x41,0x51,0x21,0x5e,
0x7f,0x09,0x09,0x19,0x66,
0x26,0x49,0x49,0x49,0x32,
0x01,0x01,0x7f,0x01,0x01,
0x3f,0x40,0x40,0x40,0x3f,
0x1f,0x20,0x40,0x20,0x1f,
0x3f,0x40,0x3c,0x40,0x3f,
0x63,0x14,0x08,0x14,0x63,
0x07,0x08,0x70,0x08,0x07,
0x71,0x49,0x45,0x43,0x00,
0x00,0x7f,0x41,0x41,0x00,
0x02,0x04,0x08,0x10,0x20,
0x00,0x41,0x41,0x7f,0x00,
0x04,0x02,0x01,0x02,0x04,
0x80,0x80,0x80,0x80,0x80,
0x00,0x03,0x07,0x00,0x00,
0x20,0x54,0x54,0x54,0x78,
0x7f,0x44,0x44,0x44,0x38,
0x38,0x44,0x44,0x44,0x28,
0x38,0x44,0x44,0x44,0x7f,
0x38,0x54,0x54,0x54,0x08,
0x08,0x7e,0x09,0x09,0x00,
0x18,0xa4,0xa4,0xa4,0x7c,
0x7f,0x04,0x04,0x78,0x00,
0x00,0x00,0x7d,0x40,0x00,
0x40,0x80,0x84,0x7d,0x00,
0x7f,0x10,0x28,0x44,0x00,
0x00,0x00,0x7f,0x40,0x00,
0x7c,0x04,0x18,0x04,0x78,
0x7c,0x04,0x04,0x78,0x00,
0x38,0x44,0x44,0x44,0x38,
0xfc,0x44,0x44,0x44,0x38,
0x38,0x44,0x44,0x44,0xfc,
0x44,0x78,0x44,0x04,0x08,
0x08,0x54,0x54,0x54,0x20,
0x04,0x3e,0x44,0x24,0x00,
0x3c,0x40,0x20,0x7c,0x00,
0x1c,0x20,0x40,0x20,0x1c,
0x3c,0x60,0x30,0x60,0x3c,
0x6c,0x10,0x10,0x6c,0x00,
0x9c,0xa0,0x60,0x3c,0x00,
0x64,0x54,0x54,0x4c,0x00,
0x08,0x3e,0x41,0x41,0x00,
0x00,0x00,0x77,0x00,0x00,
0x00,0x41,0x41,0x3e,0x08,
0x02,0x01,0x02,0x01,0x00,
0x3c,0x26,0x23,0x26,0x3c};

void DMA1_Channel3_IRQHandler(void) __attribute__((interrupt));

void DMA1_Channel3_IRQHandler(void)
{
	if(DMA_GetITStatus(DMA1_IT_TC3)) {
		DMA_ClearITPendingBit(DMA1_IT_TC3);
		DMA_Cmd(DMA1_Channel3, DISABLE);
		//Delay_Ms(4); // this comes right before the data is completely written
		digitalWrite(u8CS, 1); // de-activate CS
		bDMA = 0; // no longer active transaction
	}
	// clear all other flags
	DMA1->INTFCR = DMA1_IT_GL3;
}

void DMA_Tx_Init(DMA_Channel_TypeDef *DMA_CHx, u32 ppadr, u32 memadr, u16 bufsize)
{
    DMA_InitTypeDef DMA_InitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure={0};

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    // Enable DMA interrupt on channel 3
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&NVIC_InitStructure);
    NVIC_EnableIRQ( DMA1_Channel3_IRQn );

    DMA_Cmd(DMA1_Channel3, DISABLE);

    DMA_DeInit(DMA_CHx);
    DMA_InitStructure.DMA_PeripheralBaseAddr = ppadr;
    DMA_InitStructure.DMA_MemoryBaseAddr = memadr;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = bufsize;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA_CHx, &DMA_InitStructure);

    DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);
   	DMA_Cmd(DMA1_Channel3, ENABLE);
} /* DMA_Tx_Init() */

void lcdWriteCMD(uint8_t ucCMD)
{
	while (bDMA) {}; // wait for old transaction to complete
	bDMA = 1;
	digitalWrite(u8DC, 0);
	digitalWrite(u8CS, 0);
	SPI_write(&ucCMD, 1);
	digitalWrite(u8CS, 1);
	digitalWrite(u8DC, 1);
	bDMA = 0;

} /* lcdWriteCMD() */

void lcdWriteDATA(uint8_t *pData, int iLen)
{
	uint8_t *p;
	while (bDMA) {}; // wait for old transaction to complete
	if (iLen >= 320) {
		digitalWrite(u8CS, 0); // activate CS
		DMA1_Channel3->CNTR = iLen;
		DMA1_Channel3->MADDR = (uint32_t)pCache0;
		DMA_Cmd(DMA1_Channel3, ENABLE); // have DMA send the data
		bDMA = 1; // tell our code that DMA is currently active for next time
		// swap buffers
		p = pCache0;
		pCache0 = pCache1;
		pCache1 = p;
	} else {
		digitalWrite(u8CS, 0);
		SPI_write(pData, iLen);
		digitalWrite(u8CS, 1);
	}
} /* lcdWriteDATA() */

void lcdInit(uint32_t u32Speed, uint8_t u8CSPin, uint8_t u8DCPin, uint8_t u8RSTPin, uint8_t u8BLPin)
{
//    uint8_t iBGR = 0;
	uint8_t *s;
	int iCount;

	u8CS = u8CSPin;
	pinMode(u8CSPin, OUTPUT);
	digitalWrite(u8CSPin, 1);
	pinMode(u8RSTPin, OUTPUT);
	digitalWrite(u8RSTPin, 0); // reset the display controller
	Delay_Ms(100);
	digitalWrite(u8RSTPin, 1);
	Delay_Ms(200);

	SPI_begin(u32Speed, 0);
	u8DC = u8DCPin;
	pinMode(u8DCPin, OUTPUT);
	u8BL = u8BLPin;
	pinMode(u8BL, OUTPUT);
	digitalWrite(u8BL, 1); // turn on backlight
//    if (pLCD->iLCDFlags & FLAGS_SWAP_RB)
//        iBGR = 8;
	lcdWriteCMD(0x01); // SW reset
	Delay_Ms(200);
	lcdWriteCMD(0x11); // sleep out
	Delay_Ms(100);
    s = (unsigned char *)uc80InitList;
    iCount = 1;
     while (iCount)
     {
		 iCount = *s++;
		 if (iCount != 0)
		 {
			 lcdWriteCMD(s[0]);
			 lcdWriteDATA(&s[1], iCount-1);
			 s += iCount;
		 } // if count
     }// while
     DMA_Tx_Init(DMA1_Channel3, (u32)&SPI1->DATAR, (u32)pCache0, 0);

} /* lcdInit() */

void lcdSetPosition(int x, int y, int w, int h)
{
uint8_t ucBuf[8];

     x += LCD_XOFFSET;
     y += LCD_YOFFSET;
     ucBuf[0] = (unsigned char)(x >> 8);
     ucBuf[1] = (unsigned char)x;
     x = x + w - 1;
     ucBuf[2] = (unsigned char)(x >> 8);
     ucBuf[3] = (unsigned char)x;
     lcdWriteCMD(0x2a);
     lcdWriteDATA(ucBuf, 4);
     ucBuf[0] = (unsigned char)(y >> 8);
     ucBuf[1] = (unsigned char)y;
     y = y + h - 1;
     ucBuf[2] = (unsigned char)(y >> 8);
     ucBuf[3] = (unsigned char)y;
     lcdWriteCMD(0x2b);
     lcdWriteDATA(ucBuf, 4);
     lcdWriteCMD(0x2c); // RAMWR
} /* lcdSetPosition() */

//
// Draw a NxN RGB565 tile
// This reverses the pixel byte order and sets a memory "window"
// of pixels so that the write can occur in one shot
//
int lcdDrawTile(int x, int y, int iTileWidth, int iTileHeight, unsigned char *pTile, int iPitch)
{
    int i, j;
    uint16_t *s16, *d16;

    if (iTileWidth*iTileHeight*2 > CACHE_SIZE) {
        return -1; // tile must fit in SPI cache
    }
    // First convert to big-endian order
    d16 = (uint16_t *)pCache0;
    for (j=0; j<iTileHeight; j++)
    {
        s16 = (uint16_t*)&pTile[j*iPitch];
        for (i=0; i<iTileWidth; i++)
        {
            *d16++ = __builtin_bswap16(*s16++);
        } // for i;
    } // for j
    lcdSetPosition(x, y, iTileWidth, iTileHeight);
    lcdWriteDATA(pCache0, iTileWidth*iTileHeight*2);
    return 0;
} /* lcdDrawTile() */

void lcdFill(uint16_t usData)
{
	int cx, cy;
	uint16_t *d;

    usData = (usData >> 8) | (usData << 8); // swap hi/lo byte for LCD
    lcdSetPosition(0,0, LCD_WIDTH, LCD_HEIGHT);
    // fit within our temp buffer
    for (cy = 0; cy < LCD_HEIGHT; cy++) {
    	if (cy < 2) { // both buffers need a copy
    		d = (uint16_t *)pCache0; // pointer swapped after each write
    		for (cx = 0; cx < LCD_WIDTH; cx++) {
    			d[cx] = usData;
    		}
    	}
        lcdWriteDATA(pCache0, LCD_WIDTH*2); // fill with data words
   } // for y

} /* lcdFill() */

//
// Draw a 1-bpp pattern with the given color and translucency
// 1 bits are drawn as color, 0 are transparent
// The translucency value can range from 1 (barely visible) to 32 (fully opaque)
// If there is a backbuffer, the bitmap is draw only into memory
// If there is no backbuffer, the bitmap is drawn on the screen with a black background color
//
void spilcdDrawPattern(uint8_t *pPattern, int iSrcPitch, int iDestX, int iDestY, int iCX, int iCY, uint16_t usColor)
{
    int x, y;
    uint8_t *s, uc, ucMask;
    uint16_t *d, u16Clr;

     if (iDestX+iCX > LCD_WIDTH) // trim to fit on display
         iCX = (LCD_WIDTH - iDestX);
     if (iDestY+iCY > LCD_HEIGHT)
         iCY = (LCD_HEIGHT - iDestY);
     if (pPattern == NULL || iDestX < 0 || iDestY < 0 || iCX <=0 || iCY <= 0)
         return;
       u16Clr = (usColor >> 8) | (usColor << 8); // swap low/high bytes
       lcdSetPosition(iDestX, iDestY, iCX, iCY);
       for (y=0; y<iCY; y++)
       {
         s = &pPattern[y * iSrcPitch];
         ucMask = uc = 0;
         d = (uint16_t *)pCache0;
         for (x=0; x<iCX; x++)
         {
             ucMask >>= 1;
             if (ucMask == 0)
             {
                 ucMask = 0x80;
                 uc = *s++;
             }
             if (uc & ucMask) // active pixel
                *d++ = u16Clr;
             else
                *d++ = 0;
         } // for x
         lcdWriteDATA(pCache0, iCX*2);
       } // for y
} /* spilcdDrawPattern() */

//
// Draw a string of text with the built-in fonts
//
int lcdWriteString(int x, int y, char *szMsg, uint16_t usFGColor, uint16_t usBGColor, int iFontSize)
{
int i, j, k, iLen;
int iStride;
uint8_t *s;
uint16_t usFG = (usFGColor >> 8) | ((usFGColor & -1)<< 8);
uint16_t usBG = (usBGColor >> 8) | ((usBGColor & -1)<< 8);
uint16_t *usD;
int cx;
uint8_t *pFont;

    if (iFontSize < 0 || iFontSize >= FONT_COUNT)
        return -1; // invalid size
    if (x == -1)
        x = iCursorX;
    if (y == -1)
        y = iCursorY;
    if (x < 0) return -1;
    iLen = strlen(szMsg);

    if (iFontSize == FONT_12x16) {
        if ((12*iLen) + x > LCD_WIDTH) iLen = (LCD_WIDTH - x)/12; // can't display it all
        if (iLen < 0) return -1;
        iStride = iLen*12;
        lcdSetPosition(x, y, iStride, 16);
        usD = (uint16_t *)pCache0;
        for (i=0; i<iStride*16; i++)
           usD[i] = usBG; // set to background color first
        for (k = 0; k<8; k++) { // create a pair of scanlines from each original
           uint8_t ucMask = (1 << k);
           usD = (unsigned short *)&pCache0[k*iStride*4];
           for (i=0; i<iLen; i++)
           {
               uint8_t c0, c1;
               s = (uint8_t *)&ucSmallFont[((unsigned char)szMsg[i]-32) * 5];
               for (j=1; j<6; j++)
               {
                   uint8_t ucMask1 = ucMask << 1;
                   uint8_t ucMask2 = ucMask >> 1;
                   c0 = s[j-1];
                   if (c0 & ucMask)
                      usD[0] = usD[1] = usD[iStride] = usD[iStride+1] = usFG;
                   // test for smoothing diagonals
                   if (j < 5) {
                      c1 = s[j];
                      if ((c0 & ucMask) && (~c1 & ucMask) && (~c0 & ucMask1) && (c1 & ucMask1)) { // first diagonal condition
                          usD[iStride+2] = usFG;
                      } else if ((~c0 & ucMask) && (c1 & ucMask) && (c0 & ucMask1) && (~c1 & ucMask1)) { // second condition
                          usD[iStride+1] = usFG;
                      }
                      if ((c0 & ucMask2) && (~c1 & ucMask2) && (~c0 & ucMask) && (c1 & ucMask)) { // repeat for previous line
                          usD[1] = usFG;
                      } else if ((~c0 & ucMask2) && (c1 & ucMask2) && (c0 & ucMask) && (~c1 & ucMask)) {
                          usD[2] = usFG;
                      }
                   }
                   usD+=2;
               } // for j
               usD += 2; // leave "6th" column blank
            } // for each character
        } // for each scanline
        lcdWriteDATA(pCache0, iStride*32);
        return 0;
    } // 12x16

    cx = (iFontSize == FONT_8x8) ? 8:6;
    pFont = (iFontSize == FONT_8x8) ? (uint8_t *)ucFont : (uint8_t *)ucSmallFont;
    if ((cx*iLen) + x > LCD_WIDTH) iLen = (LCD_WIDTH - x)/cx; // can't display it all
    iStride = iLen * cx*2;
    for (i=0; i<iLen; i++)
    {
        s = &pFont[((unsigned char)szMsg[i]-32) * (cx-1)];
        uint8_t ucMask = 1;
        for (k=0; k<8; k++) // for each scanline
        {
            usD = (unsigned short *)&pCache0[(k*iStride) + (i * cx*2)];
            for (j=0; j<cx-1; j++)
            {
                if (s[j] & ucMask)
                    *usD++ = usFG;
                else
                    *usD++ = usBG;
            } // for j
	    *usD++ = usBG; // blank column
            ucMask <<= 1;
        } // for k
    } // for i
    // write the data in one shot
    lcdSetPosition(x, y, cx*iLen, 8);
    lcdWriteDATA(pCache0, iLen*cx*16);
    iCursorX = x + (cx*iLen);
    iCursorY = y;
    return 0;
} /* lcdWriteString() */

//
// Draw a string in a proportional font you supply
//
int lcdWriteStringCustom(GFXfont *pFont, int x, int y, char *szMsg, uint16_t usFGColor, uint16_t usBGColor, int bBlank)
{
int i, /*j, iLen, */ k, dx, dy, cx, cy, c, iBitOff;
int tx, ty;
uint8_t *s, bits, uc;
GFXfont font;
GFXglyph glyph, *pGlyph;
#define TEMP_BUF_SIZE 64
#define TEMP_HIGHWATER (TEMP_BUF_SIZE-8)
uint16_t *d;

   if (pFont == NULL)
      return -1;
    if (x == -1)
        x = iCursorX;
    if (y == -1)
        y = iCursorY;
    if (x < 0)
        return -1;
   // in case of running on AVR, get copy of data from FLASH
   memcpy(&font, pFont, sizeof(font));
   pGlyph = &glyph;
   usFGColor = (usFGColor >> 8) | (usFGColor << 8); // swap h/l bytes
   usBGColor = (usBGColor >> 8) | (usBGColor << 8);

   i = 0;
   while (szMsg[i] && x < LCD_WIDTH)
   {
      c = szMsg[i++];
      if (c < font.first || c > font.last) // undefined character
         continue; // skip it
      c -= font.first; // first char of font defined
      memcpy_P(&glyph, &font.glyph[c], sizeof(glyph));
      // set up the destination window (rectangle) on the display
      dx = x + pGlyph->xOffset; // offset from character UL to start drawing
      dy = y + pGlyph->yOffset;
      cx = pGlyph->width;
      cy = pGlyph->height;
      iBitOff = 0; // bitmap offset (in bits)
      if (dy + cy > LCD_HEIGHT)
         cy = LCD_HEIGHT - dy; // clip bottom edge
      else if (dy < 0) {
         cy += dy;
         iBitOff += (pGlyph->width * (-dy));
         dy = 0;
      }
      if (dx + cx > LCD_WIDTH)
         cx = LCD_WIDTH - dx; // clip right edge
      s = font.bitmap + pGlyph->bitmapOffset; // start of bitmap data
      // Bitmap drawing loop. Image is MSB first and each pixel is packed next
      // to the next (continuing on to the next character line)
      bits = uc = 0; // bits left in this font byte

      if (bBlank) { // erase the areas around the char to not leave old bits
         int miny, maxy;
         c = '0' - font.first;
         miny = y + pGlyph->yOffset;
         c = 'y' - font.first;
         maxy = miny + pGlyph->height;
         if (maxy > LCD_HEIGHT)
            maxy = LCD_HEIGHT;
         cx = pGlyph->xAdvance;
         if (cx + x > LCD_WIDTH) {
            cx = LCD_WIDTH - x;
         }
         lcdSetPosition(x, miny, cx, maxy-miny);
            // blank out area above character
//            cy = font.yAdvance - pGlyph->height;
//            for (ty=miny; ty<miny+cy && ty < maxy; ty++) {
//               for (tx=0; tx<cx; tx++)
//                  u16Temp[tx] = usBGColor;
//               myspiWrite(pLCD, (uint8_t *)u16Temp, cx*sizeof(uint16_t), MODE_DATA, iFlags);
//            } // for ty
            // character area (with possible padding on L+R)
            for (ty=0; ty<pGlyph->height && ty+miny < maxy; ty++) {
               d = (uint16_t *)pCache0;
               for (tx=0; tx<pGlyph->xOffset && tx < cx; tx++) { // left padding
                  *d++ = usBGColor;
               }
            // character bitmap (center area)
               for (tx=0; tx<pGlyph->width; tx++) {
                  if (bits == 0) { // need more data
                     uc = s[iBitOff>>3];
                     bits = 8;
                     iBitOff += bits;
                  }
                  if (tx + pGlyph->xOffset < cx) {
                     *d++ = (uc & 0x80) ? usFGColor : usBGColor;
                  }
                  bits--;
                  uc <<= 1;
               } // for tx
               // right padding
               k = pGlyph->xAdvance - (int)(d - (uint16_t*)pCache0); // remaining amount
               for (tx=0; tx<k && (tx+pGlyph->xOffset+pGlyph->width) < cx; tx++)
                  *d++ = usBGColor;
               lcdWriteDATA(pCache0, cx*sizeof(uint16_t));
            } // for ty
            // padding below the current character
            ty = y + pGlyph->yOffset + pGlyph->height;
            for (; ty < maxy; ty++) {
            	d = (uint16_t *)pCache0;
               for (tx=0; tx<cx; tx++)
                  d[tx] = usBGColor;
               lcdWriteDATA(pCache0, cx*sizeof(uint16_t));
            } // for ty
      } else if (usFGColor == usBGColor) { // transparent
          int iCount; // opaque pixel count
          d = (uint16_t*)pCache0;
          for (iCount=0; iCount < cx; iCount++)
              d[iCount] = usFGColor; // set up a line of solid color
          iCount = 0; // number of sequential opaque pixels
             for (ty=0; ty<cy; ty++) {
             for (tx=0; tx<pGlyph->width; tx++) {
                if (bits == 0) { // need to read more font data
                   uc = s[iBitOff>>3]; // get more font bitmap data
                   bits = 8 - (iBitOff & 7); // we might not be on a byte boundary
                   iBitOff += bits; // because of a clipped line
                   uc <<= (8-bits);
                } // if we ran out of bits
                if (tx < cx) {
                    if (uc & 0x80) {
                        iCount++; // one more opaque pixel
                    } else { // any opaque pixels to write?
                        if (iCount) {
                            lcdSetPosition(dx+tx-iCount, dy+ty, iCount, 1);
                       d = (uint16_t *)pCache0; // point to start of output buffer
                          lcdWriteDATA(pCache0, iCount*sizeof(uint16_t));
                            iCount = 0;
                        } // if opaque pixels to write
                    } // if transparent pixel hit
                }
                bits--; // next bit
                uc <<= 1;
             } // for tx
             } // for ty
       // quicker drawing
      } else { // just draw the current character box fast
         lcdSetPosition(dx, dy, cx, cy);
            d = (uint16_t *)pCache0; // point to start of output buffer
            for (ty=0; ty<cy; ty++) {
            for (tx=0; tx<pGlyph->width; tx++) {
               if (bits == 0) { // need to read more font data
                  uc = s[iBitOff>>3]; // get more font bitmap data
                  bits = 8 - (iBitOff & 7); // we might not be on a byte boundary
                  iBitOff += bits; // because of a clipped line
                  uc <<= (8-bits);
                  k = (int)(d-(uint16_t*)pCache0); // number of words in output buffer
                  if (k >= TEMP_HIGHWATER) { // time to write it
                     lcdWriteDATA(pCache0, k*sizeof(uint16_t));
                     d = (uint16_t*)pCache0;
                  }
               } // if we ran out of bits
               if (tx < cx) {
                  *d++ = (uc & 0x80) ? usFGColor : usBGColor;
               }
               bits--; // next bit
               uc <<= 1;
            } // for tx
            } // for ty
            k = (int)(d-(uint16_t*)pCache0);
            if (k) // write any remaining data
               lcdWriteDATA(pCache0, k*sizeof(uint16_t));
      } // quicker drawing
      x += pGlyph->xAdvance; // width of this character
   } // while drawing characters
    iCursorX = x;
    iCursorY = y;
   return 0;
} /* lcdWriteStringCustom() */

