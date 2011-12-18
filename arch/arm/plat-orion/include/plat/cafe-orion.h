/*
 * include/asm-arm/plat-orion/cafe-orion.h
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __ASM_PLAT_ORION_CAFE_ORION_H
#define __ASM_PLAT_ORION_CAFE_ORION_H

struct cafe_cam_platform_data {
	int power_down; /* value to set into GPR to power down the sensor*/ 
	int reset;	/* value to set into GPR to reset the senser */
	int numbered_i2c_bus;
	unsigned int i2c_bus_id;
};
#endif
