/*
 * CAN.h
 *
 *  Created on: 2 Sep 2018
 *      Author: pedro
 */

#ifndef CAN_H_
#define CAN_H_

#include "MKE06Z4.h"
#include "GPIO.h"

/* All CAN-bus related definitions and functions come here */

/* Defines start here */

/* Functions start here */
void CheckCANAddr(){
	/* Variable CAN address detected from 4-bit resistor code on PCB
	 * Each resistor is pulled-up to 5V, if no resistor pin reads low
	 */

	uint16_t canaddr = mGetAddrBit0();
	canaddr |= ((mGetAddrBit1() << 1) | (mGetAddrBit2() << 2) | (mGetAddrBit3() << 3));
	// TODO Attribute Slave or Master mode
}
void GetCANMsg(){

}
void SendCANMsg(){

}

#endif /* CAN_H_ */
