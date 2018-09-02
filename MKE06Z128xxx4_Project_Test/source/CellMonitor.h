/*
 * CellMonitor.h
 *
 *  Created on: 2 Sep 2018
 *      Author: pedro
 */

#ifndef CELLMONITOR_H_
#define CELLMONITOR_H_

#include "MKE06Z4.h"

/* All Cell-Monitor IC definitions and functions come here */

/* Defines start here */

/* Battery characteristics */
#define CELL_NUMBER				6										// Actual number of cells in series
#define CELL_NUMBER_MIN			3										// Minimal number of cells in series
#define CELL_NUMBER_MAX			6										// Maximal number of cells in series
#define CELL_VOLTAGE_MIN		2.5										// Minimal cell voltage
#define CELL_VOLTAGE_MAX		4.2										// Maximal cell voltage
#define VBAT_MIN				CELL_NUMBER * CELL_VOLTAGE_MIN			// Minimal Battery Pack Voltage
#define VBAT_MAX				CELL_NUMBER * CELL_VOLTAGE_MAX			// Maximal Battery Pack Voltage
#define TBAT_MIN 				0 										// Minimal cell working temperature ºC
#define TBAT_MAX				60 										// Maximal cell working temperature ºC

/* BQ76PL536 IC definitions */

/* Functions start here */

#endif /* CELLMONITOR_H_ */
