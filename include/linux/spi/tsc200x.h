/*
 *	Driver Header for TI Touch Screen Controller TSC2005/6 
 *	 
 * 
 * TSC Interfaced with Atmel AT91sam9261ek board,
 * with PENIRQ connected to AT91_PIN_PB0 and on
 * SPI0.2 (spi0 chip_select2)
 * modalias: "tsc2006" or "tsc2005"
 * 
 * 
 * Author	: Pavan Savoy (pavan_savoy@mindtree.com)
 * Date		: 15-05-2008
 */

#ifndef TSC200X_H_
#define TSC200X_H_


#include <linux/init.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <asm/mach-types.h>

struct tsc2005_platform_data
{
	u16	model;
	u16	x_plate_ohms;
	u16	y_plate_ohms;

};

#endif /*TSC200X_H_*/

