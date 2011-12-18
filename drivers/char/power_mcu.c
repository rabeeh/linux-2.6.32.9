
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/string.h>
#include "power_mcu.h"
#include <linux/delay.h>
//#include <mach/gpio.h>
//#include <mach/mfp.h>
//#include <mach/mfp-pxa168.h>
//#include <plat/mfp.h>
//#include <mach/gpio_ec.h>

static struct i2c_client *g_client;

int power_mcu_read(unsigned char reg, void *pval)
{
	int ret, status = -EIO;
	
	//printk("power_mcu: power_mcu_read\n");

	if (g_client == NULL)
		return -EFAULT;

	if ((reg < MCU_OFFSET_WORD ) && (reg >= MCU_OFFSET_BYTE ))
	{
		//if ( set_bus_busy() == -1 )
		//	return -EINVAL;
		ret = i2c_smbus_read_byte_data( g_client , reg );
		//set_bus_free();
		if ( ret >= 0 )
		{
			*((unsigned char *)pval) = (unsigned char )ret;
			status = 0;
		}
	}
	else if (( reg >= MCU_OFFSET_WORD ) && ( reg < MCU_END ))
	{
		//if ( set_bus_busy() == -1 )
		//	return -EINVAL;
		ret = i2c_smbus_read_word_data( g_client , reg );
		//set_bus_free();

		if ( ret >= 0 )
		{
			*((unsigned short *)pval) = (unsigned short )ret;
			status=0;
		}
	}
	else
		return -EINVAL;
	
	return status;
}


EXPORT_SYMBOL(power_mcu_read);

int power_mcu_write_word(unsigned char reg, unsigned short val)
{
	//unsigned char data;
	int ret;
	
	//printk("power_mcu: power_mcu_write_word\n");
	if (g_client == NULL)
		return -EFAULT;
	if (reg >= MCU_END )
		return -EINVAL;
	
	//if ( set_bus_busy() == -1 )
	//	return -EINVAL;
	ret = i2c_smbus_write_word_data(g_client, reg, val);
	//set_bus_free();

	return ret;
}

int power_mcu_write_byte(unsigned char reg, unsigned char val)
{
	//unsigned char data;
	int ret;
	
	//printk("power_mcu: power_mcu_write_byte\n");
	if (g_client == NULL)
		return -EFAULT;
    
	if (reg >= MCU_END )
		return -EINVAL;

	//if ( set_bus_busy() == -1 )
	//	return -EINVAL; 
	ret = i2c_smbus_write_byte_data(g_client, reg, val );
	//set_bus_free();

	return ret;
}

EXPORT_SYMBOL(power_mcu_write_byte);
EXPORT_SYMBOL(power_mcu_write_word);

void power_mcu_write_rtc( u32 rtc )
{
	power_mcu_write_word( MCU_RTC_LSB , rtc );
	power_mcu_write_word( MCU_RTC_MSB , rtc >> 16 );
}

void power_mcu_read_rtc( u32 *rtc )
{
	power_mcu_read( MCU_RTC_LSB , rtc );
	power_mcu_read( MCU_RTC_MSB , ((u8 *)rtc + 2 ));

}

EXPORT_SYMBOL(power_mcu_write_rtc);
EXPORT_SYMBOL(power_mcu_read_rtc);

static int __devinit power_i2c_probe(struct i2c_client *client)
{
	int i;
	int nretry = 25;
	unsigned char version;
	unsigned char battery_absent;
        g_client = client;
	
	//printk("power_mcu: power_i2c_probe start\n");
	for( i = 0 ; i < nretry ; i++ )
	{
		if ( power_mcu_read( MCU_VERSION , &version ) == 0 )
        	{
			printk("MCU_VERSION==%d\n",version);
			if ( power_mcu_read( MCU_BATT_ABSENT , &battery_absent ) == 0 )
        		{
				printk("MCU_BATT_ABSENT==%d\n",battery_absent);
				return 0;
			}
			//return 0;
		}
	}
	printk("power_mcu: probe failed\n");
	g_client = NULL;
	return -1;
        
}

static int __devexit power_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static struct i2c_device_id power_mcu_idtable[]=
{
	{"power_mcu",0},
	{},
};

static struct i2c_driver power_mcu_driver = {
	.driver = {
		.name	= "power_mcu",
	},
	.id_table		= power_mcu_idtable,
	.probe			= &power_i2c_probe,
	.remove			= &power_i2c_remove,
};


static int __init power_mcu_init(void)
{
    	int ret;

 //      printk("power_mcu_init\n");
	if ( (ret = i2c_add_driver(&power_mcu_driver)) ) {
		printk("power mcu Driver registration failed\n");
		return ret;
	}

	return 0;
}

static void __exit power_mcu_exit(void)
{
	i2c_del_driver(&power_mcu_driver);
}

module_init(power_mcu_init);
module_exit(power_mcu_exit);


