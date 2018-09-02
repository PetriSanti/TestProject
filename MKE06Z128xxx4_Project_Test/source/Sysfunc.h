/*
 * Sysfunc.h
 *
 *  Created on: 2 Sep 2018
 *      Author: pedro
 */

#ifndef SYSFUNC_H_
#define SYSFUNC_H_

/* All System user-made definitions and functions come here */

/* Defines start here */
#define SysClock 				48000000							// 48 MHz Clock

/* Functions start here */
void delay(uint32_t delaytime)
{
    volatile uint32_t i = 0;
    for (i = 0; i < delaytime; ++i)
    {
        __asm("NOP"); /* delay, does nothing */
    }
    // TODO: Translate in ms according to system clock
}

#endif /* SYSFUNC_H_ */
