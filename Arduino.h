/*
 * Arduino.h
 *
 *  Created on: Jan 12, 2023
 *      Author: larry
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"

#ifndef USER_ARDUINO_H_
#define USER_ARDUINO_H_

#define BITBANG

// GPIO pin states
enum {
	OUTPUT = 0,
	INPUT,
	INPUT_PULLUP,
	INPUT_PULLDOWN
};

#define PROGMEM
#define memcpy_P memcpy
#define pgm_read_byte(s) *(uint8_t *)s
#define pgm_read_word(s) *(uint16_t *)s

// Wrapper methods
void delay(int i);
//
// Digital pin functions use a numbering scheme to make it easier to map the
// pin number to a port name and number
// The GPIO ports A-D become the most significant nibble of the pin number
// for example, to use Port C pin 7 (C7), use the pin number 0xC7
//
void pinMode(uint8_t u8Pin, int iMode);
uint8_t digitalRead(uint8_t u8Pin);
void digitalWrite(uint8_t u8Pin, uint8_t u8Value);

// The Wire library is a C++ class; I've created a work-alike to my
// BitBang_I2C API which is a set of C functions to simplify I2C
void I2CSetSpeed(int iSpeed);
int I2CReadRegister(uint8_t iAddr, uint8_t u8Register, uint8_t *pData, int iLen);
void I2CInit(uint8_t u8SDA, uint8_t u8SCL, int iSpeed);

void I2CWrite(uint8_t u8Addr, uint8_t *pData, int iLen);
void I2CRead(uint8_t u8Addr, uint8_t *pData, int iLen);
int I2CTest(uint8_t u8Addr);

// SPI1 (polling mode)
void SPI_write(uint8_t *pData, int iLen);
void SPI_begin(int iSpeed, int iMode);

// USART1 (receive only)
void UART_Init(int iBaud);
void UART_DeInit(void);
int UART_Read(void);
#define UART_TIMEOUT 10000

// Random stuff
void Standby82ms(uint8_t iTicks);
void breatheLED(uint8_t u8Pin, int iPeriod);


#endif /* USER_ARDUINO_H_ */
