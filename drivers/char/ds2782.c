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

#include <linux/i2c.h>
#include "ds2782.h"

static struct i2c_driver ds2782_i2c_driver;
static	struct i2c_client *ds2782_i2c_client;
static struct i2c_client *get_ds2782_i2c_client(void);

int ds2782_reg_get(int offset, unsigned char *pDataBuf, int datasize)
{
	int rc;
	struct i2c_client *i2c_client;

	if((i2c_client = get_ds2782_i2c_client()))
	{
		rc = i2c_smbus_read_i2c_block_data(i2c_client, offset, datasize, pDataBuf);
		if (rc < 0)
			return -EIO;
	}else{
		return -ENODEV;
	}
    	return 0;	
}
EXPORT_SYMBOL(ds2782_reg_get);

int ds2782_reg_set(int offset, unsigned char data)
{
	struct i2c_client *i2c_client;

	if((i2c_client = get_ds2782_i2c_client()))
	{
		i2c_smbus_write_byte_data(i2c_client, offset, data);

	}else{
		return -ENODEV;
	}
    	return 0;	
}
EXPORT_SYMBOL(ds2782_reg_set);

static struct i2c_client *get_ds2782_i2c_client(void)
{
	return ds2782_i2c_client;
}


static int ds2782_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int rc;

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_I2C_BLOCK)) {
		dev_err(&client->dev, "i2c bus does not support the ds2782\n");
		rc = -ENODEV;
		goto exit;
	}

	ds2782_i2c_client = client;
	return 0;

 exit:
	return rc;
}

static int ds2782_remove(struct i2c_client *client)
{
	ds2782_i2c_client = NULL;
	return 0;
}



static const struct i2c_device_id ds2782_id[] = {
	{ "ds2782", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ds2782_id);

static struct i2c_driver ds2782_i2c_driver = {
	.driver = {
		.name	= "ds2782",
	},
	.probe = ds2782_probe,
	.remove = ds2782_remove,
	.id_table = ds2782_id,
};



static int __init ds2782_init(void)
{
	return i2c_add_driver(&ds2782_i2c_driver);
}

static void __exit ds2782_exit(void)
{
	i2c_del_driver(&ds2782_i2c_driver);
}


module_init(ds2782_init);
module_exit(ds2782_exit);


