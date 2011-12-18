/*
 *
 *	Marvell Fuel Gauge IC driver
 *	Maxim DS2782 Fuel Gauge IC
 *
 *	Author: 
 *	Copyright (C) 2008 Marvell Ltd.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef MV_DS2782_DRV
#define MV_DS2782_DRV

#define DS2782_I2C_ADDR 	0x34

#define DS2782_STATUS_REG 	0x01
#define DS2782_VOLT_REG 	0x0c
#define DS2782_TEMP_REG 	0x0a
#define DS2782_CURRENT_REG 	0x0e
#define DS2782_ACR_REG 		0x10
#define DS2782_IAVG_REG 	0x08
#define DS2782_AC_REG 		0x62
#define DS2782_AS_REG 		0x14
#define DS2782_RSNSP_REG 	0x69
#define DS2782_VCHG_REG 	0x64
#define DS2782_IMIN_REG 	0x65
#define DS2782_VAE_REG 		0x66
#define DS2782_IAE_REG 		0x67
#define DS2782_FULL40_REG 	0x6a
#define DS2782_FULL_REG 	0x16
#define DS2782_AE_REG 		0x18
#define DS2782_SE_REG 		0x1a
#define DS2782_RAAC_REG 	0x02
#define DS2782_RSAC_REG 	0x04
#define DS2782_RARC_REG 	0x06
#define DS2782_RSRC_REG 	0x07

#define DS2782_VOLT_SHIFT_BIT 	 	5
#define DS2782_TEMP_SHIFT_BIT 		5
#define DS2782_CURRENT_SHIFT_BIT 	0
#define DS2782_ACR_SHIFT_BIT		0
#define DS2782_IAVG_SHIFT_BIT 		0
#define DS2782_AC_SHIFT_BIT 		0
#define DS2782_AS_SHIFT_BIT 		0
#define DS2782_RSNSP_SHIFT_BIT 		0
#define DS2782_VCHG_SHIFT_BIT 		0
#define DS2782_IMIN_SHIFT_BIT 		0
#define DS2782_VAE_SHIFT_BIT		0
#define DS2782_IAE_SHIFT_BIT		0
#define DS2782_FULL40_SHIFT_BIT 	0
#define DS2782_FULL_SHIFT_BIT 		0
#define DS2782_AE_SHIFT_BIT 		0
#define DS2782_SE_SHIFT_BIT 		0
#define DS2782_RAAC_SHIFT_BIT 		0
#define DS2782_RSAC_SHIFT_BIT 		0
#define DS2782_RARC_SHIFT_BIT 		0
#define DS2782_RSRC_SHIFT_BIT 		0

extern int ds2782_reg_get(int offset, unsigned char *pDataBuf, int datasize);
extern int ds2782_reg_set(int offset, unsigned char data);

#endif /* MV_DS2782_DRV */
