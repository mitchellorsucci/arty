/*
 * LCD_driver.c
 *
 *  Created on: Jun 21, 2017
 *      Author: mitchellorsucci
 */

#include "LCD.h"
#include "xgpio.h"
#include "xtmrctr.h"
#include "xparameters.h"
#include "xio.h"
#include "xil_io.h"
XTmrCtr TimerCounter;

#define TIMER_BASE			XPAR_AXI_TIMER_0_BASEADDR
#define TMRCTR_DEVICE_ID	XPAR_TMRCTR_0_DEVICE_ID
#define LCD_DATA	0x44A10000
#define LCD_CONTROL	0x44A10004

int usleep(unsigned int useconds);

void instLCD() {
	/*Set up and configure counter for delays */
	int status;
	status = XTmrCtr_Initialize(&TimerCounter, TMRCTR_DEVICE_ID);
	if (status != XST_SUCCESS) {
		xil_printf("Timer Config Failed\n\r");
		return XST_FAILURE;
		}
	XTmrCtr_SetResetValue(&TimerCounter, 0, 0);
	XTmrCtr_SetOptions(&TimerCounter, 0, 0);


	/* Startup Sequence */
	usleep(45000); // Wait at least 30 ms
	writeCommandandData(0x00, 0x3C);
	usleep(1000); // wait at least 39 us

	writeCommandandData(0x00, 0x0C); // display on, cursor off, blink off
	usleep(1000); // wait another 39 us

	writeCommandandData(0x00, 0x01); // clear display
	usleep(2000); // wait for more than 1.53 ms

	writeCommandandData(0x00, 0x06); // set to increment and shift mode
	usleep(2000);



}

void writeCommandandData(unsigned char command, unsigned char mask) {
	Xil_Out32(LCD_DATA, mask);
	Xil_Out32(LCD_CONTROL, command);
	pulseEnable(command);
}

void pulseEnable(unsigned char command) {
	usleep(10);
	Xil_Out32(LCD_CONTROL, command | EN); // set enable high
	usleep(10);
	Xil_Out8(LCD_CONTROL, command & 0xFB); // set enable low
	usleep(10);
}

void writeChar(unsigned char letter) {
	writeCommandandData(0x01, letter);
}

void writeSequence(char * data) {
	while(*data) {
		writeChar(*data);
		data++;
	}
}

void writeGyroData(char * axis, char * value) {
	clrAndReturnHome();
	writeCommandandData(0x00, 0x06); // set to increment and shift mode
	writeCommandandData(0x00, 0x04 | 0x80); // Set address to 0x04
	writeSequence(axis);
	writeCommandandData(0x00, 0x46 | 0x80); // Set address to 0x42
	writeSequence(value);
}

void clrAndReturnHome() {
	writeCommandandData(0x00, 0x00 | 0x80); // Return home
	writeCommandandData(0x00, 0x01); // Clear the display
}
int usleep(unsigned int useconds) {
		u32 Value;
		u32 Compare = useconds * 1000;
		XTmrCtr_Reset(&TimerCounter, 0);
		/*
		 * Start the timer counter such that it's incrementing by default
		 */
		XTmrCtr_Start(&TimerCounter, 0);

		/*
		 * Read the value of the timer counter and wait for it to change,
		 * since it's incrementing it should change, if the hardware is not
		 * working for some reason, this loop could be infinite such that the
		 * function does not return
		 */
		while (1) {
			Value = XTmrCtr_GetValue(&TimerCounter, 0);
			if (Value == 0xFFFFFFFF) {
				/* Because of the timer overflow it stops after it passes zero. */
				break;
			} else if (Value > Compare) {
				break;
			}
		}
		/*
		 * Disable the timer counter such that it stops incrementing
		 */
		XTmrCtr_Stop(&TimerCounter, 0);

		return XST_SUCCESS;

}

