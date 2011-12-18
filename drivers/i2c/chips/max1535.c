/*
    This driver supports the MAX 1535B/1535C/1535D SMBus Battery Chargers.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/proc_fs.h>

#define MAX1535_CHARGER_SPEC_INFO		0x11
#define MAX1535_CHARGER_STATUS			0x13
#define MAX1535_MANUFACTURER_ID			0xFE
#define MAX1535_DEVICE_ID			0xFF
#define MAX1535_INPUT_CURRENT_LIMIT		0x3F
#define MAX1535_INPUT_CURRENT_LIMIT_DEF		0x0800	/* 4A */

static const struct i2c_device_id max1535_id[] = {
     { "max1535_i2c", 0 },
     { }
};

struct i2c_client *max1535_client = NULL;

int charger_proc_write(struct file *file, const char *buffer,unsigned long count, void *data)
{	
	int cmd;
	int val;

	sscanf(buffer, "%x %x", &cmd, &val);
	printk("Writing to 0x%x 0x%x\n", (u8)cmd, (u16)val);
	i2c_smbus_write_word_data(max1535_client, (u8)cmd, (u16)val);
	
	return count;
}

int charger_proc_read(char* page, char** start, off_t off, int count,int* eof, void* data)
{
	int len = 0;
	s32 rd;

	if ((rd = (int)i2c_smbus_read_word_data(max1535_client, MAX1535_CHARGER_SPEC_INFO)) < 0) {
          	return -EIO;
     	}
	len += sprintf(page+len, "Charger Spec Info (0x11) = %04x\n", rd);

	if ((rd = (int)i2c_smbus_read_word_data(max1535_client, MAX1535_CHARGER_STATUS)) < 0) {
          	return -EIO;
     	}
	len += sprintf(page+len, "Charger Status (0x13) = %04x\n", rd);

	if ((rd = (int)i2c_smbus_read_word_data(max1535_client, MAX1535_MANUFACTURER_ID)) < 0) {
          	return -EIO;
     	}
	len += sprintf(page+len, "Manufacturer ID (0xFE) = %04x\n", rd);

	if ((rd = (int)i2c_smbus_read_word_data(max1535_client, MAX1535_DEVICE_ID)) < 0) {
          	return -EIO;
     	}
	len += sprintf(page+len, "Device ID (0xFF) = %04x\n", rd);

	len += sprintf(page+len, "\n\nUsage:\n");
	len += sprintf(page+len, "  echo <reg> <val> > /proc/charger\n");
	return len;
}

static struct proc_dir_entry *charger_proc_entry;
extern struct proc_dir_entry proc_root;
static int charger_proc_init(void)
{
	charger_proc_entry = create_proc_entry("charger", 0666, &proc_root);
	charger_proc_entry->read_proc = charger_proc_read;
	charger_proc_entry->write_proc = charger_proc_write;
	return 0;
}

static int __devinit max1535_probe(struct i2c_client *client,
			       const struct i2c_device_id *id)
{
     if (!i2c_check_functionality(to_i2c_adapter(client->dev.parent),
				  I2C_FUNC_SMBUS_EMUL))
	  return -EIO;

     /* Raise the charge current */
     if (i2c_smbus_write_word_data(client, MAX1535_INPUT_CURRENT_LIMIT, 
					MAX1535_INPUT_CURRENT_LIMIT_DEF) < 0) {
          dev_err(&client->dev, "Failed To initialize Current Limit in SMBus Charger");
          return -EIO;
     }

     max1535_client = client;

     charger_proc_init();

     printk("MAX1535 SMBus Battery Charger driver loaded successfully\n");
     return 0;
}
static int __devexit max1535_remove(struct i2c_client *client)
{
     return 0;
}


static struct i2c_driver max1535_driver = {
     .driver = {
	  .name   = "max1535_i2c",
	  .owner  = THIS_MODULE,
     },
     .probe          = max1535_probe,
     .remove         = __devexit_p(max1535_remove),
     .id_table       = max1535_id,
};

static int __init max1535_init(void)
{
	return i2c_add_driver(&max1535_driver);
}

static void __exit max1535_exit(void)
{
	i2c_del_driver(&max1535_driver);
}

MODULE_AUTHOR("Tawfik Bayouk <tawfik@marvell.com>");
MODULE_DESCRIPTION("MAX1535 SMBus Battery Charger Driver");
MODULE_LICENSE("GPL");

module_init(max1535_init);
module_exit(max1535_exit);
