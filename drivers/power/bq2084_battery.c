/*
 * Driver for batteries with BQ2084 chips inside.
 *
 * Copyright © 2009 Marvell semiconductors
 *
 * Author:  Tawfik Bayouk <tawfik@marvell.com>
 *	    March 2009
 * Based on the driver DS2760 battery driver.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/proc_fs.h>

struct bq2084_device_info {
	struct device *dev;

	/* Data is valid after calling bq2084_battery_read_status() */
	unsigned long update_time;	/* jiffies when data read */
	int charge_status;
	int temp_tK;
	int current_mV;
	int current_mA;
	int average_mA;
	int capacity_P;
	int current_mAh;
	int absolute_mAh;	
	int current_empty_time_Min;
	int average_empty_time_Min;
	int average_full_time_Min;
	int design_mAh;

	struct power_supply bat;
	struct device *w1_dev;
	struct workqueue_struct *monitor_wqueue;
	struct delayed_work monitor_work;
};

static unsigned int cache_time = 1000;
module_param(cache_time, uint, 0644);
MODULE_PARM_DESC(cache_time, "cache time in milliseconds");

static struct bq2084_device_info *di_proc;
int bat_proc_write(struct file *file, const char *buffer,unsigned long count, void *data)
{	
	return count;
}

int bat_proc_read(char* page, char** start, off_t off, int count,int* eof,
		    void* data)
{
	int len = 0;

	len += sprintf(page+len, "Charge_status = %d\n", di_proc->charge_status);
	len += sprintf(page+len, "temp_tK = %d\n", di_proc->temp_tK);
	len += sprintf(page+len, "current_mV = %d\n", di_proc->current_mV);
	len += sprintf(page+len, "current_mA = %d\n", di_proc->current_mA);
	len += sprintf(page+len, "average_mA = %d\n", di_proc->average_mA);
	len += sprintf(page+len, "capacity_P = %d\n", di_proc->capacity_P);
	len += sprintf(page+len, "current_mAh = %d\n", di_proc->current_mAh);
	len += sprintf(page+len, "absolute_mAh = %d\n", di_proc->absolute_mAh);
	len += sprintf(page+len, "current_empty_time_Min = %d\n", di_proc->current_empty_time_Min);
	len += sprintf(page+len, "average_empty_time_Min = %d\n", di_proc->average_empty_time_Min);
	len += sprintf(page+len, "average_full_time_Min = %d\n", di_proc->average_full_time_Min);
	len += sprintf(page+len, "design_mAh = %d\n", di_proc->design_mAh);
	return len;
}

static struct proc_dir_entry *bat_proc_entry;
extern struct proc_dir_entry proc_root;
static int battery_proc_init(void)
{
	bat_proc_entry = create_proc_entry("battery", 0666, &proc_root);
	bat_proc_entry->read_proc = bat_proc_read;
	bat_proc_entry->write_proc = bat_proc_write;
	return 0;
}

/*
 * Gauge Driver
 */

struct i2c_client *bq2084_client = NULL;

#define	PS_TEMP			0x08
#define	PS_VOLTAGE_NOW		0x09
#define PS_CURRENT_NOW		0x0A
#define PS_CURRENT_AVG 		0x0B
#define PS_CAPACITY 		0x0D
#define PS_CHARGE_NOW 		0x0F
#define PS_CHARGE_FULL		0x10
#define PS_TIME_TO_EMPTY_NOW 	0x11
#define PS_TIME_TO_EMPTY_AVG 	0x12
#define PS_TIME_TO_FULL_AVG 	0x13
#define PS_CHARGE_FULL_DESIGN	0x18

static const struct i2c_device_id bq2084_gauge_id[] = {
     { "bq2084_i2c", 0 },
     { }
};

static int __devinit bq2084_gauge_probe(struct i2c_client *client,
			       const struct i2c_device_id *id)
{
     bq2084_client = client;

     if (!i2c_check_functionality(to_i2c_adapter(client->dev.parent),
				  I2C_FUNC_SMBUS_EMUL))
	  return -EIO;

     printk("TI BQ2084 Battery Gauge driver loaded successfully\n");
     return 0;
}
static int __devexit bq2084_gauge_remove(struct i2c_client *client)
{
     return 0;
}


static struct i2c_driver bq2084_driver = {
     .driver = {
	  .name   = "bq2084_i2c",
	  .owner  = THIS_MODULE,
     },
     .probe          = bq2084_gauge_probe,
     .remove         = __devexit_p(bq2084_gauge_remove),
     .id_table       = bq2084_gauge_id,
};

/*
 * Battery Driver
 */

static int bq2084_battery_read_status(struct bq2084_device_info *di)
{
	int old_charge_status = di->charge_status;

	/* Poll the device at most once in a cache_time */
	if (time_before(jiffies, di->update_time + msecs_to_jiffies(cache_time)))
		return 0;

	/* Read the Gauge status through the I2C */
	di->update_time = jiffies;		/* last update time */

	if ((di->temp_tK = (int)i2c_smbus_read_word_data(bq2084_client, PS_TEMP)) < 0) {
		dev_err(&bq2084_client->dev, "i2c_smbus_read_word_data: PS_TEMP");
          	return -EIO;
     	}

	if ((di->current_mV = (int)i2c_smbus_read_word_data(bq2084_client, PS_VOLTAGE_NOW)) < 0) {
		dev_err(&bq2084_client->dev, "i2c_smbus_read_word_data: PS_VOLTAGE_NOW");
          	return -EIO;
     	}

	if ((di->current_mA = (int)i2c_smbus_read_word_data(bq2084_client, PS_CURRENT_NOW)) < 0) {
		dev_err(&bq2084_client->dev, "i2c_smbus_read_word_data: PS_CURRENT_NOW");
          	return -EIO;
     	}

	if ((di->average_mA = (int)i2c_smbus_read_word_data(bq2084_client, PS_CURRENT_AVG)) < 0) {
		dev_err(&bq2084_client->dev, "i2c_smbus_read_word_data: PS_CURRENT_AVG");
          	return -EIO;
     	}

	if ((di->current_mAh = (int)i2c_smbus_read_word_data(bq2084_client, PS_CHARGE_NOW)) < 0) {
		dev_err(&bq2084_client->dev, "i2c_smbus_read_word_data: PS_CAPACITY");
          	return -EIO;
     	}

	if ((di->absolute_mAh = (int)i2c_smbus_read_word_data(bq2084_client, PS_CHARGE_FULL)) < 0) {
		dev_err(&bq2084_client->dev, "i2c_smbus_read_word_data: PS_CHARGE_NOW");
          	return -EIO;
     	}

	if ((di->capacity_P = (int)i2c_smbus_read_word_data(bq2084_client, PS_CAPACITY)) < 0) {
		dev_err(&bq2084_client->dev, "i2c_smbus_read_word_data: PS_CHARGE_FULL");
          	return -EIO;
     	}

	if ((di->current_empty_time_Min = (int)i2c_smbus_read_word_data(bq2084_client, PS_TIME_TO_EMPTY_NOW)) < 0) {
		dev_err(&bq2084_client->dev, "i2c_smbus_read_word_data: PS_TIME_TO_EMPTY_NOW");
          	return -EIO;
     	}

	if ((di->average_empty_time_Min = (int)i2c_smbus_read_word_data(bq2084_client, PS_TIME_TO_EMPTY_AVG)) < 0) {
		dev_err(&bq2084_client->dev, "i2c_smbus_read_word_data: PS_TIME_TO_EMPTY_AVG");
          	return -EIO;
     	}

	if ((di->average_full_time_Min = (int)i2c_smbus_read_word_data(bq2084_client, PS_TIME_TO_FULL_AVG)) < 0) {
		dev_err(&bq2084_client->dev, "i2c_smbus_read_word_data: PS_TIME_TO_FULL_AVG");
          	return -EIO;
     	}

	if ((di->design_mAh = (int)i2c_smbus_read_word_data(bq2084_client, PS_CHARGE_FULL_DESIGN)) < 0) {
		dev_err(&bq2084_client->dev, "i2c_smbus_read_word_data: PS_CHARGE_FULL_DESIGN");
          	return -EIO;
     	}

	if (di->current_mA & 0x8000)
		di->charge_status = POWER_SUPPLY_STATUS_DISCHARGING;
	else
		di->charge_status = POWER_SUPPLY_STATUS_CHARGING;

	if ((di->current_mA & ~0x8000) < 10)
		di->charge_status = POWER_SUPPLY_STATUS_FULL;

	if (di->charge_status != old_charge_status)
	{
		power_supply_changed(&di->bat);
		cancel_delayed_work(&di->monitor_work);
		queue_delayed_work(di->monitor_wqueue, &di->monitor_work, HZ/10);		
	}

	dev_dbg(di->dev, "Charge_status = %d\n", di->charge_status);
	dev_dbg(di->dev, "temp_tK = %d\n", di->temp_tK);
	dev_dbg(di->dev, "current_mV = %d\n", di->current_mV);
	dev_dbg(di->dev, "current_mA = %d\n", di->current_mA);
	dev_dbg(di->dev, "average_mA = %d\n", di->average_mA);
	dev_dbg(di->dev, "capacity_P = %d\n", di->capacity_P);
	dev_dbg(di->dev, "current_mAh = %d\n", di->current_mAh);
	dev_dbg(di->dev, "absolute_mAh = %d\n", di->absolute_mAh);
	dev_dbg(di->dev, "current_empty_time_Min = %d\n", di->current_empty_time_Min);
	dev_dbg(di->dev, "average_empty_time_Min = %d\n", di->average_empty_time_Min);
	dev_dbg(di->dev, "average_full_time_Min = %d\n", di->average_full_time_Min);
	dev_dbg(di->dev, "design_mAh = %d\n", di->design_mAh);

	return 0;
}

static void bq2084_battery_work(struct work_struct *work)
{
	struct bq2084_device_info *di = container_of(work,
		struct bq2084_device_info, monitor_work.work);
	const int interval = HZ * 60;
	
	dev_dbg(di->dev, "%s\n", __func__);

	bq2084_battery_read_status(di);
	queue_delayed_work(di->monitor_wqueue, &di->monitor_work, interval);
}

#define to_bq2084_device_info(x) container_of((x), struct bq2084_device_info, \
					      bat);

static void bq2084_battery_external_power_changed(struct power_supply *psy)
{
	struct bq2084_device_info *di = to_bq2084_device_info(psy);

	dev_dbg(di->dev, "%s\n", __func__);

	cancel_delayed_work(&di->monitor_work);
	queue_delayed_work(di->monitor_wqueue, &di->monitor_work, HZ/10);
}

static int bq2084_battery_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	struct bq2084_device_info *di = to_bq2084_device_info(psy);

	bq2084_battery_read_status(di);

	/*
	 * All voltages, currents, charges, energies, time and temperatures in uV,
	 * µA, µAh, µWh, seconds and tenths of degree Celsius
	 */

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = di->charge_status;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		/* convert Kelvin to tenths of degree Celsius */
		val->intval = (di->temp_tK - 2730); 
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = (di->current_mV * 1000);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = (di->current_mA * 1000);
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		val->intval = (di->average_mA * 1000);
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = di->capacity_P;
		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		val->intval = (di->current_mAh * 1000);
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		val->intval = (di->absolute_mAh * 1000);
		break;	
	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW:
		val->intval = (di->current_empty_time_Min * 60); 
		break;
	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG:
		val->intval = (di->average_empty_time_Min * 60);
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_AVG:
		val->intval = (di->average_full_time_Min * 60);
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		val->intval = (di->design_mAh * 1000);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static enum power_supply_property bq2084_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,		/* based on current */
	POWER_SUPPLY_PROP_TEMP,			/* SMB 0x08 */
	POWER_SUPPLY_PROP_VOLTAGE_NOW,		/* SMB 0x09 */
	POWER_SUPPLY_PROP_CURRENT_NOW,		/* SMB 0x0A */
	POWER_SUPPLY_PROP_CURRENT_AVG, 		/* SMB 0x0B */
	POWER_SUPPLY_PROP_CAPACITY, 		/* SMB 0x0D */
	POWER_SUPPLY_PROP_CHARGE_NOW, 		/* SMB 0x0F */
	POWER_SUPPLY_PROP_CHARGE_FULL,		/* SMB 0x10 */		
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW, 	/* SMB 0x11 */
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG, 	/* SMB 0x12 */
	POWER_SUPPLY_PROP_TIME_TO_FULL_AVG, 	/* SMB 0x13 */
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,	/* SMB 0x18 */
};

static int bq2084_battery_probe(struct platform_device *pdev)
{
	int retval = 0;
	struct bq2084_device_info *di;
	struct bq2084_platform_data *pdata;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di) {
		retval = -ENOMEM;
		goto di_alloc_failed;
	}

	platform_set_drvdata(pdev, di);

	pdata 			= pdev->dev.platform_data;
	di->dev			= &pdev->dev;
	di->w1_dev	     	= pdev->dev.parent;
	di->update_time 	= jiffies;
	di->bat.name	   	= dev_name(&pdev->dev);
	di->bat.type	   	= POWER_SUPPLY_TYPE_BATTERY;
	di->bat.properties     	= bq2084_battery_props;
	di->bat.num_properties 	= ARRAY_SIZE(bq2084_battery_props);
	di->bat.get_property   	= bq2084_battery_get_property;
	di->bat.external_power_changed =
				  bq2084_battery_external_power_changed;

	di->charge_status = POWER_SUPPLY_STATUS_UNKNOWN;

	retval = power_supply_register(&pdev->dev, &di->bat);
	if (retval) {
		dev_err(di->dev, "failed to register battery\n");
		goto batt_failed;
	}

	INIT_DELAYED_WORK(&di->monitor_work, bq2084_battery_work);
	di->monitor_wqueue = create_singlethread_workqueue(dev_name(&pdev->dev));
	if (!di->monitor_wqueue) {
		retval = -ESRCH;
		goto workqueue_failed;
	}
	queue_delayed_work(di->monitor_wqueue, &di->monitor_work, HZ * 1);

	di_proc = di;

	printk("TI BQ2084 Power Supply driver loaded successfully. \n");
	goto success;

workqueue_failed:
	power_supply_unregister(&di->bat);
batt_failed:
	kfree(di);
di_alloc_failed:
success:
	return retval;
}

static int bq2084_battery_remove(struct platform_device *pdev)
{
	struct bq2084_device_info *di = platform_get_drvdata(pdev);

	cancel_rearming_delayed_workqueue(di->monitor_wqueue,
					  &di->monitor_work);
	destroy_workqueue(di->monitor_wqueue);
	power_supply_unregister(&di->bat);

	return 0;
}

#ifdef CONFIG_PM

static int bq2084_battery_suspend(struct platform_device *pdev,
				  pm_message_t state)
{
	struct bq2084_device_info *di = platform_get_drvdata(pdev);

	di->charge_status = POWER_SUPPLY_STATUS_UNKNOWN;

	return 0;
}

static int bq2084_battery_resume(struct platform_device *pdev)
{
	struct bq2084_device_info *di = platform_get_drvdata(pdev);

	di->charge_status = POWER_SUPPLY_STATUS_UNKNOWN;
	power_supply_changed(&di->bat);

	cancel_delayed_work(&di->monitor_work);
	queue_delayed_work(di->monitor_wqueue, &di->monitor_work, HZ);

	return 0;
}

#else

#define bq2084_battery_suspend NULL
#define bq2084_battery_resume NULL

#endif /* CONFIG_PM */

static struct platform_driver bq2084_battery_driver = {
	.driver = {
		.name = "bq2084-battery",
	},
	.probe	  = bq2084_battery_probe,
	.remove   = bq2084_battery_remove,
	.suspend  = bq2084_battery_suspend,
	.resume	  = bq2084_battery_resume,
};

static int __init bq2084_battery_init(void)
{
	int ret;
	if ((ret = i2c_add_driver(&bq2084_driver)) < 0)
		return ret;

	battery_proc_init();

	return platform_driver_register(&bq2084_battery_driver);
}

static void __exit bq2084_battery_exit(void)
{
	platform_driver_unregister(&bq2084_battery_driver);
	i2c_del_driver(&bq2084_driver);
}

module_init(bq2084_battery_init);
module_exit(bq2084_battery_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tawfik Bayouk <tawfik@marvell.com>");
MODULE_DESCRIPTION("bq2084 battery driver");
