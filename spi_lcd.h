/*
 * spi_lcd.h
 *
 *  Created on: Sep 11, 2023
 *      Author: larry
 */

#ifndef USER_SPI_LCD_H_
#define USER_SPI_LCD_H_

#define LCD_WIDTH 160
#define LCD_PITCH (LCD_WIDTH*2)
#define LCD_HEIGHT 80
#define CACHED_LINES 16
// enough memory to hold 16 lines of the display for fast character drawing
#define CACHE_SIZE (LCD_PITCH*CACHED_LINES)
// memory offset of visible area (80x160 out of 240x320)
#define LCD_XOFFSET 0
#define LCD_YOFFSET 24

// Proportional font data taken from Adafruit_GFX library
/// Font data stored PER GLYPH
typedef struct {
  uint16_t bitmapOffset; ///< Pointer into GFXfont->bitmap
  uint8_t width;         ///< Bitmap dimensions in pixels
  uint8_t height;        ///< Bitmap dimensions in pixels
  uint8_t xAdvance;      ///< Distance to advance cursor (x axis)
  int8_t xOffset;        ///< X dist from cursor pos to UL corner
  int8_t yOffset;        ///< Y dist from cursor pos to UL corner
} GFXglyph;

typedef struct {
  uint8_t *bitmap;  ///< Glyph bitmaps, concatenated
  GFXglyph *glyph;  ///< Glyph array
  uint8_t first;    ///< ASCII extents (first char)
  uint8_t last;     ///< ASCII extents (last char)
  uint8_t yAdvance; ///< Newline distance (y axis)
} GFXfont;

void lcdFill(uint16_t u16Color);
void lcdInit(uint32_t u32Speed, uint8_t u8CSPin, uint8_t u8DCPin, uint8_t u8RSTPin, uint8_t u8BLPin);
void lcdWriteCMD(uint8_t ucCMD);
void lcdWriteDATA(uint8_t *pData, int iLen);
int lcdWriteString(int x, int y, char *szMsg, uint16_t usFGColor, uint16_t usBGColor, int iFontSize);
int lcdDrawTile(int x, int y, int iTileWidth, int iTileHeight, unsigned char *pTile, int iPitch);
int lcdWriteStringCustom(GFXfont *pFont, int x, int y, char *szMsg, uint16_t usFGColor, uint16_t usBGColor, int bBlank);

#define COLOR_BLACK 0
#define COLOR_WHITE 0xffff
#define COLOR_RED 0xf800
#define COLOR_GREEN 0x7e0
#define COLOR_BLUE 0x1f
#define COLOR_MAGENTA 0xf81f
#define COLOR_CYAN 0x7ff
#define COLOR_YELLOW 0xffe0

enum {
	FONT_6x8 = 0,
	FONT_8x8,
	FONT_12x16,
	FONT_COUNT
};

#endif /* USER_SPI_LCD_H_ */
