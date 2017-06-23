/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * GyroDisp.c: Uses the RGB leds on the ARTY and the PMODCLP 16x2 character display
 * 	to print information about the acceleration of the board
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "platform.h"	// for USB-uart
#include "LCD.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xio.h"		// for register writes
#include "PmodGYRO.h"
#include <String.h> 	// for strcat


/************************************************************
 * DEFINES
*************************************************************/

#define PWMS		0x44A00000
#define LCD_DATA	0x44A10000
#define LCD_CONTROL	0x44A10004
#define SWITCH		0x40000000
#define KEY			0x40000008
#define AVG_BUFF_SIZE 50
#define THRESHHOLD	200
#define SHAMT		4


/************************************************************
 * GLOBAL VARIABLES
*************************************************************/

int xVals[AVG_BUFF_SIZE];
int yVals[AVG_BUFF_SIZE];
int zVals[AVG_BUFF_SIZE];

int xAvg = 0;
int yAvg = 0;
int zAvg = 0;


int main(){
    init_platform();

    instLCD();
    xil_printf("LCD instantiated\n\r");
    writeSequence("Welcome");
    writeCommandandData(0x00, 0x40 | 0x80); // Set address to 0x40 -- second line
    writeSequence("This is LEDGyro.");
    writeCommandandData(0x00, 0x0A | 0x80); // Set address to 0x10 -- first line, space 10
    writeSequence("Press BTN0 to continue");

    xil_printf("Welcome to LEDgyro\n\r");
    xil_printf("press BTN0 to continue\n\r");

    xil_printf("Once the program has started, you may press BTN1\n\r");
    xil_printf("At any time to stop the program\n\r");

    u8 button, exit;
    button = Xil_In8(KEY);
    while(!(button & 0x01)) {
    	writeCommandandData(0x00, 0x18);
    	usleep(12500);
    	button = Xil_In8(KEY);
    }
    clrAndReturnHome();

    PmodGYRO gyro;
    GYRO_begin(&gyro, XPAR_PMODGYRO_0_AXI_LITE_SPI_BASEADDR, XPAR_PMODGYRO_0_AXI_LITE_GPIO_BASEADDR);

    GYRO_enableInt1(&gyro, INT1_XHIE);
    GYRO_enableInt2(&gyro, REG3_I2_DRDY);

    int16_t xAxis=0, yAxis=0, zAxis=0;

    /* Tunes the sensitivity of the sensors */
    GYRO_setThsXH(&gyro, 0x00);
    GYRO_setThsYH(&gyro, 0x0F);
	GYRO_setThsZH(&gyro, 0x0F);


	u8 trig = 0;
    u8 index = 0;
    print("Starting...\n\r");
    u8 swValue;

    while(1){
        char axis[20] = "      ";
        char data[20] = "";
        swValue = Xil_In8(SWITCH);

    	if(index >= AVG_BUFF_SIZE) {
        	index = 0;
        }

    	if(GYRO_Int1Status(&gyro) != 0){
            trig = 1;
        }
        if(GYRO_Int2Status(&gyro) != 0){
            if(trig == 1) {
                trig = 0;
            }

            xAxis = GYRO_getX(&gyro);
            yAxis = GYRO_getY(&gyro);
            zAxis = GYRO_getZ(&gyro);



             /* Normalize the values */
            xAxis = xAxis - 450;
            if(xAxis < 0) {
            	xAxis = -xAxis;
            }
            if(yAxis < 0) {
            	yAxis = -yAxis;
            }
            if(zAxis < 0) {
            	zAxis = -zAxis;
            }

            /* updates the running averages */
            xVals[index] = xAxis;
			yVals[index] = yAxis;
			zVals[index] = zAxis;
			int sum = 0;
			for (int i = 0; i < AVG_BUFF_SIZE; i++) {
				sum += xVals[i];
			}
			xAvg = sum/AVG_BUFF_SIZE;
			sum = 0;
			for (int i = 0; i < AVG_BUFF_SIZE; i++) {
				sum += yVals[i];
			}
			yAvg = sum/AVG_BUFF_SIZE;
			sum = 0;
			for (int i = 0; i < AVG_BUFF_SIZE; i++) {
				sum += zVals[i];
			}
			zAvg = sum/AVG_BUFF_SIZE;


			/* Selects which data to display */
            switch (swValue) {
            	case 1: // X == Green
            		yAvg = 0;
            		zAvg = 0;
            		strcat(axis, "X-Axis:");
            		sprintf(data, "%d", xAvg);
            		writeGyroData(axis, data);
            		break;
            	case 2: // Y == Blue
            		xAvg = 0;
            		zAvg = 0;
            		strcat(axis, "Y-Axis:");
            		sprintf(data, "%d", yAvg);
            		writeGyroData(axis, data);
            		break;
            	case 3: // Z == Red
            		xAvg = 0;
            		yAvg = 0;
            		strcat(axis, "Z-Axis:");
            		sprintf(data, "%d", zAvg);
            		writeGyroData(axis, data);
            		break;
            	default:
                    if(xAvg > yAvg && xAvg > zAvg)  {
                    	yAvg = 0;
                    	zAvg = 0;
                		strcat(axis, "X-Axis:");
                		sprintf(data, "%d", xAvg);
                		writeGyroData(axis, data);
                    } else if (yAvg > xAvg && yAvg > zAvg) {
                    	xAvg = 0;
                    	zAvg = 0;
                		strcat(axis, "Y-Axis:");
                		sprintf(data, "%d", yAvg);
                		writeGyroData(axis, data);
                    } else if (zAvg > xAvg && zAvg > yAvg) {
                    	yAvg = 0;
                    	xAvg = 0;
                		strcat(axis, "Z-Axis:");
                		sprintf(data, "%d", zAvg);
                		writeGyroData(axis, data);
                    }
            		break;
            }

            xAvg = xAvg < THRESHHOLD ? 0 : xAvg;
            yAvg = yAvg < THRESHHOLD ? 0 : yAvg;
            zAvg = zAvg < THRESHHOLD ? 0 : zAvg;

            /* Un-commenting this line may make your code a little buggy,
             * But can be used to print axis data to the serial terminal
             *
             * If you decide to use it, I recommend increasing the delay
             * at the bottom of the while loop
             * to give the UART time to catch up
             */
            //xil_printf("X Axis: %d\tY Axis: %d\tZ Axis: %d\t\n\r", xAxis, yAxis, zAxis);
            Xil_Out32(PWMS, xAvg >> 1); 		// 0 -- B
            Xil_Out32(PWMS + 4, yAvg >> 1);		// 0 -- G
            Xil_Out32(PWMS + 8, zAvg >> 1);		// 0 -- R
            Xil_Out32(PWMS + 12, xAvg >> 4);	// 1 -- B
            Xil_Out32(PWMS + 16, yAvg >> 4); 	// 1 -- G
            Xil_Out32(PWMS + 20, zAvg >> 4); 	// 1 -- R
            Xil_Out32(PWMS + 24, xAvg >> 7);	// 2 -- B
            Xil_Out32(PWMS + 28, yAvg >> 7);	// 2 -- G
            Xil_Out32(PWMS + 32, zAvg >> 7);	// 2 -- R
            Xil_Out32(PWMS + 36, xAvg >> 8);	// 3 -- B
            Xil_Out32(PWMS + 40, yAvg >> 8); 	// 3 -- G
            Xil_Out32(PWMS + 44, zAvg >> 8); 	// 3 -- R

            index++;
            exit = Xil_In8(KEY);
            if(exit & 0x02) {
            	clrAndReturnHome();
            	for(int i = 0; i < SHAMT; i++) {
            		writeCommandandData(0x00, 0x1C); // shift entire display right
            	}
            	writeSequence("Have a nice day!");
            	break;
            }
            usleep(1000);
        }
    }

    cleanup_platform();
    return 0;
}
