/*
 *  linux/include/asm-arm/arch-dove/pxa2xx-gpio.h
 *
 *  Author:	Shadi Ammouri
 *  Created:	Jun 10, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */


/*
 * Dummy values for GPIO constants used by the PXA AC'97 driver.
 * TODO: Revisit to make sure that these are still needed in the future.
 */
#define GPIO31_SYNC_AC97_MD		0
#define GPIO30_SDATA_OUT_AC97_MD	0
#define GPIO28_BITCLK_AC97_MD		0
#define GPIO29_SDATA_IN_AC97_MD		0


/*
 * Stubs for PXA GPIO functions used by the PXA AC'97 driver.
 */
static inline int pxa_gpio_mode(int gpio_mode)
{
	return 0;
}

