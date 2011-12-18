/*
 * Driver for keys on GPIO lines capable of generating interrupts.
 *
 * Copyright 2005 Phil Blundell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <linux/module.h>
#include <linux/version.h>


#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include "mv_ec_key.h"
#include <linux/i2c.h>
#include <asm/gpio.h>


//#define DEBUG 1

/////////////////////////////////////////////////////////////////////////////////////
//ec key
/////////////////////////////////////////////////////////////////////////////////////
#define NUM_EC_KEY 8
#define EC_KEY_SLAVE_ADDR 0x2c
#define EC_KEY_INTR_STATUS_OFFSET 0x40
#define EC_KEY_DOWN_STATUS_OFFSET 0x41
#define EC_KEY_UP_STATUS_OFFSET 0x42

#define KEY_PRESS_INTR_CLR 0x0
#define KEY_RELEASE_INTR_CLR 0x0
#define KEY_PRESS_INTR 0x1
#define KEY_RELEASE_INTR 0x2

#define EC_KEY_PRESSING 0x01
#define EC_KEY_RELEASED 0x10

//#define KEY_DATA_COMMAND 0x01
//#define FW_VERSION_COMMAND 0x16
//#define TOUCH_KEY_ID 0x61
//#define DUMMY_DATA 0x00


#define POLLING_TIMER 50 //msec


static const int ec_key_flag[NUM_EC_KEY]=
{
	0x01,
	0x02,
	0x04,
	0x08,
	0x10,
	0x20,
	0x40,
	0x80,
};

static const int ec_key_code[NUM_EC_KEY]=
{
	KEY_HOME,
	KEY_BACK,
	KEY_UP,
	KEY_DOWN,
	KEY_POWER,
	KEY_MENU,
	KEY_F1,
	KEY_F2,
};

#ifdef DEBUG
static const char * ec_key_string[NUM_EC_KEY]=
{
	"KEY_HOME",
	"KEY_BACK",
	"KEY_UP",
	"KEY_DOWN",
	"KEY_POWER",
	"KEY_MENU",
	"KEY_F1",
	"KEY_F2",

};
#endif

struct ec_key
{
	struct input_dev *input_dev;

	int ec_key_active;
	int irq;

	struct workqueue_struct *workqueue;
	struct work_struct work;
	struct timer_list polling_timer;

	u8 key_pressing;

	struct i2c_adapter      *i2c_adapter;
	struct i2c_client *i2c_client;
};


/////////////////////////////////////////////////////////////////////////////////////
//touch key
/////////////////////////////////////////////////////////////////////////////////////

static irqreturn_t ec_key_isr(int irq, void *dev_id)
{
	struct ec_key *ec_key = (struct ec_key *)dev_id;

#ifdef DEBUG
	printk("%s\n",__FUNCTION__);
#endif

	mod_timer(&ec_key->polling_timer, jiffies);
	disable_irq_nosync(irq);
	
	return IRQ_HANDLED;
}

static void polling_timer_handler(unsigned long handle)
{ 
	struct ec_key *ec_key = (void *)handle;
#ifdef DEBUG
	printk("%s\n",__FUNCTION__);
#endif
	queue_work(ec_key->workqueue, &ec_key->work);
}


static void mv_ec_key_work(struct work_struct *work)
{

	struct ec_key *ec_key = container_of(work, struct ec_key, work);
	unsigned char wr_cmd;
	unsigned char wr_data;
	unsigned char rd_data;
	unsigned char key_status;

	wr_cmd = EC_KEY_INTR_STATUS_OFFSET;
	rd_data = i2c_smbus_read_byte_data(ec_key->i2c_client, wr_cmd);
	if(rd_data < 0)
	{
		printk("EC_KEY Driver: Fail to Read I2C\n");
		return;
	}

	if((rd_data & KEY_PRESS_INTR) || (rd_data & KEY_RELEASE_INTR)) {

		if(rd_data & KEY_PRESS_INTR)
			wr_data = KEY_PRESS_INTR_CLR;
		else
			wr_data = KEY_RELEASE_INTR_CLR;

		// clear interrupt
		if(i2c_smbus_write_byte_data(ec_key->i2c_client, wr_cmd, wr_data) != 0)
		{
			printk("EC_KEY Driver: Fail to Write I2C\n");
			//return;
		}

		key_status = rd_data;

		// key pressed
		if(key_status & KEY_PRESS_INTR) {
			int i = 0;
			int key_code;

			// check if ec_key still pressing
			if(ec_key->key_pressing & EC_KEY_PRESSING) {
#ifdef DEBUG
				printk("ec_key still pressing\n");
#endif
				enable_irq(ec_key->irq);
				return;
			}

			// read key code
			wr_cmd = EC_KEY_DOWN_STATUS_OFFSET;
			rd_data = i2c_smbus_read_byte_data(ec_key->i2c_client, wr_cmd);
			if(rd_data < 0)
			{
				printk("EC_KEY Driver: Fail to Read I2C\n");
				return;
			}
			wr_data = 0;
			if(i2c_smbus_write_byte_data(ec_key->i2c_client, wr_cmd, wr_data) != 0)
			{
				printk("EC_KEY Driver: Fail to Write I2C\n");
				//return;
			}

			key_code = rd_data;
#ifdef DEBUG
			printk("ec_key press read: %x\n",rd_data);
#endif

			// check which key been pressed
			for (i=0; i<NUM_EC_KEY; i++) {
				if(key_code == ec_key_flag[i]) {
#ifdef DEBUG
					printk("ec_key press: %s\n", ec_key_string[i]);
#endif
					input_report_key(ec_key->input_dev, ec_key_code[i], 1);
					input_sync(ec_key->input_dev);
					ec_key->key_pressing = EC_KEY_PRESSING;
					break;
				}
			}
		}

		// key released
		if(key_status & KEY_RELEASE_INTR) {
			int i = 0;
			int key_code;

			// read key code
			wr_cmd = EC_KEY_UP_STATUS_OFFSET;
			rd_data = i2c_smbus_read_byte_data(ec_key->i2c_client, wr_cmd);
			if(rd_data < 0)
			{
				printk("EC_KEY Driver: Fail to Read I2C\n");
				return;
			}
			wr_data = 0;
			if(i2c_smbus_write_byte_data(ec_key->i2c_client, wr_cmd, wr_data) != 0)
			{
				printk("EC_KEY Driver: Fail to Write I2C\n");
				//return;
			}

			key_code = rd_data;

#ifdef DEBUG
			printk("ec_key release read: %x\n",rd_data);
#endif

			// check which key been released
			for (i=0; i<NUM_EC_KEY; i++) {
				if(key_code == ec_key_flag[i]) {
#ifdef DEBUG
					printk("ec_key released: %s\n", ec_key_string[i]);
#endif
					input_report_key(ec_key->input_dev, ec_key_code[i], 0);
					input_sync(ec_key->input_dev);
					ec_key->key_pressing = EC_KEY_RELEASED;
					break;
				}
			}

		}
	} else {
		// unknow status, clear interrupt
		if (rd_data != 0) {
			wr_data = 0;
			if(i2c_smbus_write_byte_data(ec_key->i2c_client, wr_cmd, wr_data) != 0)
			{
				printk("EC_KEY Driver: Fail to Write I2C\n");
				//return;
			}
		}

	}

	enable_irq(ec_key->irq);
}

static int mv_ec_key_init_gpio(struct ec_key *ec_key)
{
//	int error;
//	int irq_gpio = ec_key->irq - IRQ_DOVE_GPIO_START;
      
//	error = gpio_request(irq_gpio,"TOUCH_KEY_IRQ");       
//	if(error!=0)
//		return -1;
 
//	gpio_direction_input(irq_gpio);
	set_irq_chip(ec_key->irq, &orion_gpio_irq_chip);
	set_irq_handler(ec_key->irq, handle_level_irq);
	set_irq_type(ec_key->irq, IRQ_TYPE_LEVEL_LOW);

	return 0;
}

static int __devinit mv_ec_key_probe(struct i2c_client *client,
			       const struct i2c_device_id *id)
{
	struct input_dev *input;
	struct ec_key *ec_key;
	int i;
	int error;
	struct i2c_adapter      *i2c_adapter = to_i2c_adapter(client->dev.parent);

#ifdef DEBUG
	printk("%s\n",__FUNCTION__);
#endif

	if (!i2c_check_functionality(i2c_adapter,
				  I2C_FUNC_I2C))
		return -EIO;


//	if(touch_key_hw_init(client) != 0)
//	{
//		return -ENODEV;
//	}

	printk("MV EC_KEY I2C driver loaded successfully\n");

	ec_key = kzalloc(sizeof(struct ec_key), GFP_KERNEL);

	ec_key->i2c_adapter = i2c_adapter;
	ec_key->i2c_client = client;

	input = input_allocate_device();
	if (!ec_key || !input)
	{
		error = -ENOMEM;
		goto err_free_mem;
	}

	dev_set_drvdata(&client->dev, ec_key);
	input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);

	for(i=0; i<NUM_EC_KEY; i++)
	{
		input->keybit[BIT_WORD(ec_key_code[i])] |= BIT_MASK(ec_key_code[i]);
	}


	input->name = client->name;
	input->dev.parent = &client->dev;

	input->id.bustype = BUS_I2C;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;

	ec_key->input_dev = input;

	ec_key->irq  = client->irq; 

	mv_ec_key_init_gpio(ec_key);

	error = input_register_device(input);
	if (error) {
		printk(KERN_ERR "Unable to register touch key input device\n");
		goto err_free_mem;
	}


	init_timer(&ec_key->polling_timer);
	ec_key->polling_timer.data = (unsigned long)ec_key;
	ec_key->polling_timer.function = polling_timer_handler;

	ec_key->workqueue = create_singlethread_workqueue("eckey_workqueue");

	if (ec_key->workqueue == NULL) {
		goto err_free_mem;
	}

	INIT_WORK(&ec_key->work, mv_ec_key_work);

	error = request_irq(ec_key->irq, ec_key_isr, 0,
				    "eckey_irq",
				    ec_key);
	if (error) {
		printk(KERN_ERR "ec-keys: unable to claim irq %d; error %d\n",
		       ec_key->irq, error);
		goto err_free_irq;
	}

	return 0;


err_free_irq:
	destroy_workqueue(ec_key->workqueue);
	if(ec_key->ec_key_active)
		free_irq(ec_key->irq,NULL);
err_free_mem:

	input_free_device(input);
	kfree(ec_key);
	return error;
}

static int __devexit mv_ec_key_remove(struct i2c_client *client)
{

	struct ec_key *ec_key = dev_get_drvdata(&client->dev);
	printk("%s\n",__FUNCTION__);	

//	gpio_free(touch_key->irq - IRQ_DOVE_GPIO_START);       


	flush_workqueue(ec_key->workqueue);
	destroy_workqueue(ec_key->workqueue);

	input_unregister_device(ec_key->input_dev);
	input_free_device(ec_key->input_dev);

	if(ec_key->ec_key_active)
		free_irq(ec_key->irq,NULL);
	ec_key->ec_key_active = 0;

	kfree(ec_key);
	return 0;
}

static const struct i2c_device_id ec_key_i2c_id[] = {
     { "ec_key_i2c", 0 },
     { }
};

static struct i2c_driver mv_ec_key_driver = {
     .driver = {
	  .name   = EC_KEY_DRV_NAME,
	  .owner  = THIS_MODULE,
     },
     .probe          = mv_ec_key_probe,
     .remove         = __devexit_p(mv_ec_key_remove),
     .id_table       = ec_key_i2c_id,
};

static int __init mv_ec_key_init(void)
{
     return i2c_add_driver(&mv_ec_key_driver);
}

static void __exit mv_ec_key_exit(void)
{
     i2c_del_driver(&mv_ec_key_driver);
}



//late_initcall(mv_ec_key_init);
module_init(mv_ec_key_init);
module_exit(mv_ec_key_exit);

MODULE_LICENSE("GPL");
