/*
 * GPIO.h
 *
 *  Created on: 2 Sep 2018
 *      Author: pedro
 */

#ifndef GPIO_H_
#define GPIO_H_

#include "MKE06Z4.h"

/* All GPIO related definitions and functions come here */

/* Defines start here */
#define GPIO_Clock 				48000000							// 48 MHz Clock

/* PTI4,PTB5,PTC3 and PTC2 are CAN-BIT ADDRESS */
#define mGetAddrBit0()			0									// Position 0 State, PTI4, Pin 11
#define mGetAddrBit1()			0									// Position 1 State, PTB5, Pin 12
#define mGetAddrBit2()			0									// Position 2 State, PTC3, Pin 14
#define mGetAddrBit3()			0									// Position 3 State, PTC2, Pin 15

/* Functions start here */

void InitGPIO(void){

	/* Start of GPIO initialisation */
	// Enable the GPIOA Clock
	// Enable the GPIOB Clock

	/* I/O mode configuration */

	/* I/O output type configuration */
	// PTI4 configured to push-pull mode


	/* I/O output speed configuration */
	// PTI4 configured to high speed mode
	// TODO review to lower consumption

	/* I/O output pull-up/pull-down configuration */
	// PTI4 configured to pull-down mode

	/* End of GPIO initialisation */
}


#endif /* GPIO_H_ */
