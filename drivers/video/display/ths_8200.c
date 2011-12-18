#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/param.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>

//#define DEBUG

#ifdef DEBUG
#include <linux/debugfs.h>
#include <asm/uaccess.h>
#endif

static const struct i2c_device_id ths8200_register_id[] = {
	{ "ths8200_i2c", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ths8200_register_id);


static int ths8200_i2c_remove(struct i2c_client *client)
{
        return 0;
}

static int ths8200_i2c_probe(struct i2c_client *client,
                        const struct i2c_device_id *id)
{
        int rc;

	printk ("Probing in %s, name %s, addr 0x%x\n",__FUNCTION__,client->name,client->addr);

        if (!i2c_check_functionality(client->adapter,
                                     I2C_FUNC_SMBUS_I2C_BLOCK)) {
                dev_err(&client->dev, "i2c bus does not support the THS8200.\n");
                rc = -ENODEV;
                goto exit;
        }

	i2c_smbus_write_byte_data(client, 0x1C, 0x30);
	i2c_smbus_write_byte_data(client, 0x82, 0x5C);

        return 0;

 exit:
        return rc;
}

static struct i2c_driver ths8200_driver = {
     .driver = {
          .name   = "ths8200_i2c",
          .owner  = THIS_MODULE,
     },
     .probe          = ths8200_i2c_probe,
     .remove         = ths8200_i2c_remove,
     .id_table       = ths8200_register_id,
};

static int __init ths8200_init(void)
{
        int ret;

	printk ("Initializing %s\n",__FUNCTION__);

        if ((ret = i2c_add_driver(&ths8200_driver)) < 0)
        {
                return ret;
        }
        return ret;
}

static void __exit ths8200_exit(void)
{
	i2c_del_driver(&ths8200_driver);
}

module_init(ths8200_init);
module_exit(ths8200_exit);

MODULE_AUTHOR("shadi@marvell.com");
MODULE_DESCRIPTION("THS8200 D2A converter.");
MODULE_LICENSE("GPL");

