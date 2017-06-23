/*
 * LCD.h
 *
 *  Created on: Jun 21, 2017
 *      Author: mitchellorsucci
 */

#include "xio.h"
#ifndef SRC_LCD_H_
#define SRC_LCD_H_

/************************************************************
 * DEFINES
*************************************************************/
#define LCD_DATA	0x44A10000
#define LCD_CONTROL	0x44A10004
#define RS					0x01
#define RW					0x02
#define EN					0x04
/************************************************************/

void instLCD();
void writeChar(unsigned char letter);
void pulseEnable(unsigned char command);
void writeCommandandData(unsigned char command, unsigned char mask);
//void commandLCD(char data);
void writeSequence(char * data);
void writeGyroData(char * axis, char * value);
void clrAndReturnHome();
//void writeSequence_n(char * data, int numBytes);
//void displayControl(char display, char cursor, char blink);
//void shiftLCD(char cursorORdisplay, char direction);


#endif /* SRC_LCD_H_ */
