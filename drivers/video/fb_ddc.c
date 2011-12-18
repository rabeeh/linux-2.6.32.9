/*
 * driver/vide/fb_ddc.c - DDC/EDID read support.
 *
 *  Copyright (C) 2006 Dennis Munsie <dmunsie@cecropia.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fb.h>
#include <linux/i2c-algo-bit.h>

#include "edid.h"

#define DDC_ADDR	0x50

static unsigned char *fb_do_probe_ddc_edid(struct i2c_adapter *adapter)
{
	unsigned char start = 0x0;
	unsigned char *buf = kmalloc(EDID_LENGTH, GFP_KERNEL);
	struct i2c_msg msgs[] = {
		{
			.addr	= DDC_ADDR,
			.flags	= 0,
			.len	= 1,
			.buf	= &start,
		}, {
			.addr	= DDC_ADDR,
			.flags	= I2C_M_RD,
			.len	= EDID_LENGTH,
			.buf	= buf,
		}
	};

	if (!buf) {
		dev_warn(&adapter->dev, "unable to allocate memory for EDID "
			 "block.\n");
		return NULL;
	}
	if (adapter->algo->master_xfer) {
		if (i2c_transfer(adapter, msgs, 2) == 2)
			return buf;
	} else {
		int i;
		dev_info(&adapter->dev, "use SMBUS adapter for reading EDID\n");
		if (0 != i2c_smbus_xfer(adapter, msgs[0].addr, msgs[0].flags,
					I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE_DATA, buf)) {
                        printk("%s: failed for address 0\n", __func__);
			return NULL;
                }
		for (i=0; i < EDID_LENGTH; i++) {
			if (0 != i2c_smbus_xfer(adapter, msgs[1].addr, 0,
						I2C_SMBUS_READ, i, I2C_SMBUS_BYTE_DATA, 
						buf + i)) {
				printk("%s: failed for address %d\n", __func__, i);
				return NULL;
			}
			
		}
		return buf;
	}

	dev_warn(&adapter->dev, "unable to read EDID block.\n");
	kfree(buf);
	return NULL;
}

unsigned char *fb_ddc_read(struct i2c_adapter *adapter)
{
	struct i2c_algo_bit_data *algo_data = adapter->algo_data;
	unsigned char *edid = NULL;
	int i, j;

	if (!algo_data)	{
		/* No direct control on I2C bus */
		for (i = 0; i < 3; i++) {
			edid = fb_do_probe_ddc_edid(adapter);
			if (edid)
				break;
		}
		return edid;
	}
	algo_data->setscl(algo_data->data, 1);

	for (i = 0; i < 3; i++) {
		/* For some old monitors we need the
		 * following process to initialize/stop DDC
		 */
		algo_data->setsda(algo_data->data, 1);
		msleep(13);

		algo_data->setscl(algo_data->data, 1);
		for (j = 0; j < 5; j++) {
			msleep(10);
			if (algo_data->getscl(algo_data->data))
				break;
		}
		if (j == 5)
			continue;

		algo_data->setsda(algo_data->data, 0);
		msleep(15);
		algo_data->setscl(algo_data->data, 0);
		msleep(15);
		algo_data->setsda(algo_data->data, 1);
		msleep(15);

		/* Do the real work */
		edid = fb_do_probe_ddc_edid(adapter);
		algo_data->setsda(algo_data->data, 0);
		algo_data->setscl(algo_data->data, 0);
		msleep(15);

		algo_data->setscl(algo_data->data, 1);
		for (j = 0; j < 10; j++) {
			msleep(10);
			if (algo_data->getscl(algo_data->data))
				break;
		}

		algo_data->setsda(algo_data->data, 1);
		msleep(15);
		algo_data->setscl(algo_data->data, 0);
		algo_data->setsda(algo_data->data, 0);
		if (edid)
			break;
	}
	/* Release the DDC lines when done or the Apple Cinema HD display
	 * will switch off
	 */
	algo_data->setsda(algo_data->data, 1);
	algo_data->setscl(algo_data->data, 1);

	adapter->class |= I2C_CLASS_DDC;
	return edid;
}

EXPORT_SYMBOL_GPL(fb_ddc_read);

MODULE_AUTHOR("Dennis Munsie <dmunsie@cecropia.com>");
MODULE_DESCRIPTION("DDC/EDID reading support");
MODULE_LICENSE("GPL");
