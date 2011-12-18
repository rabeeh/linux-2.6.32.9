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
#include <linux/irq.h>
#include "mv_touch_key_slider.h"
#include <linux/i2c.h>
#include <asm/gpio.h>

//#define DEBUG

/////////////////////////////////////////////////////////////////////////////////////
//touch key
/////////////////////////////////////////////////////////////////////////////////////
#define NUM_TOUCH_KEY 2
#define TOUCH_KEY_SLAVE_ADDR 0x0b
#define KEY_DATA_COMMAND 0x01
#define FW_VERSION_COMMAND 0x16
#define TOUCH_KEY_ID 0x61
#define DUMMY_DATA 0x00


#define POLLING_TIMER 50 //msec


static const int touch_key_flag[NUM_TOUCH_KEY]=
{
	0x04,
	0x08,
};

static const int touch_key_code[NUM_TOUCH_KEY]=
{
	KEY_F11,
	KEY_WWW,
};

#ifdef DEBUG
static const char * touch_key_string[NUM_TOUCH_KEY]=
{
	"KEY_F11",
	"KEY_WWW",
};
#endif

struct touch_key
{
	struct input_dev *input_dev;

	int touch_key_active;
	int irq;

	struct workqueue_struct *workqueue;
	struct work_struct work;
	struct timer_list polling_timer;

	u8 touch_pressed;

	struct i2c_adapter      *i2c_adapter;
	struct i2c_client *i2c_client;
};




/////////////////////////////////////////////////////////////////////////////////////
//touch key
/////////////////////////////////////////////////////////////////////////////////////
static irqreturn_t touch_key_isr(int irq, void *dev_id)
{
	struct touch_key *touch_key = (struct touch_key *)dev_id;

#ifdef DEBUG
	printk("%s\n",__FUNCTION__);
#endif

	if(touch_key->touch_pressed == 0)
	{
		mod_timer(&touch_key->polling_timer, jiffies);
		disable_irq_nosync(irq);
	}

	
	return IRQ_HANDLED;
}

static void polling_timer_handler(unsigned long handle)
{

	struct touch_key *touch_key = (void *)handle;
#ifdef DEBUG
	printk("%s\n",__FUNCTION__);
#endif
	queue_work(touch_key->workqueue, &touch_key->work);


}



static void mv_touch_key_work(struct work_struct *work)
{

	struct touch_key *touch_key = container_of(work, struct touch_key, work);
	unsigned char wr_cmd[3];
	unsigned char rd_data[3];


	wr_cmd[0] = KEY_DATA_COMMAND;	
	wr_cmd[1] = DUMMY_DATA;	
	wr_cmd[2] = DUMMY_DATA;

	if(i2c_master_send(touch_key->i2c_client,&wr_cmd[0],3) != 3)
	{
		printk("Fail to Write I2C\n");
		return;
   	}

	if(i2c_master_recv(touch_key->i2c_client,&rd_data[0],3) !=3)
	{
		printk("Fail to Read I2C\n");
	        return;
   	}
#ifdef DEBUG
	printk("%x : %x : %x\n",rd_data[0],rd_data[1],rd_data[2]);
#endif

	if((touch_key->touch_pressed == 0) && (rd_data[1] == 0))
	{
		enable_irq(touch_key->irq);
		return;
	}
	if(rd_data[1] & touch_key_flag[0])
	{
		if((touch_key->touch_pressed & touch_key_flag[0]) == 0)
		{
#ifdef DEBUG
			printk("%s pressed\n",touch_key_string[0]);
#endif
			touch_key->touch_pressed |= touch_key_flag[0];
			input_report_key(touch_key->input_dev, touch_key_code[0], 1);
			input_sync(touch_key->input_dev);
		}
		mod_timer(&touch_key->polling_timer, jiffies + msecs_to_jiffies(POLLING_TIMER));

	}else if(touch_key->touch_pressed & touch_key_flag[0]){
#ifdef DEBUG
		printk("%s released\n",touch_key_string[0]);
#endif
		touch_key->touch_pressed &= ~touch_key_flag[0];
		input_report_key(touch_key->input_dev, touch_key_code[0], 0);
		input_sync(touch_key->input_dev);
		enable_irq(touch_key->irq);
	}

	if(rd_data[1] & touch_key_flag[1])
	{
		if((touch_key->touch_pressed & touch_key_flag[1])==0)
		{
#ifdef DEBUG
			printk("%s pressed\n",touch_key_string[1]);
#endif
			touch_key->touch_pressed |= touch_key_flag[1];
			input_report_key(touch_key->input_dev, touch_key_code[1], 1);
			input_sync(touch_key->input_dev);
		}
		mod_timer(&touch_key->polling_timer, jiffies + msecs_to_jiffies(POLLING_TIMER));

	}else if(touch_key->touch_pressed & touch_key_flag[1]){
#ifdef DEBUG
		printk("%s released\n",touch_key_string[1]);
#endif
		touch_key->touch_pressed &= ~touch_key_flag[1];
		input_report_key(touch_key->input_dev, touch_key_code[1], 0);
		input_sync(touch_key->input_dev);
		enable_irq(touch_key->irq);
	}


}


static int touch_key_hw_init(struct i2c_client *client)
{

	unsigned char wr_cmd[3];
	unsigned char rd_data[3];

	wr_cmd[0] = FW_VERSION_COMMAND;
	wr_cmd[1] = DUMMY_DATA;
	wr_cmd[2] = DUMMY_DATA;

	if(i2c_master_send(client,wr_cmd,3) != 3)
	{
#ifdef DEBUG
		printk("Fail to Write I2C\n");
#endif
		return -1;
	}
	if(i2c_master_recv(client,rd_data,3) !=3)
	{
#ifdef DEBUG
		printk("Fail to Read I2C\n");
#endif
		return -1;
	}
	printk("[%s] %x : %x : %x\n",__FUNCTION__,rd_data[0],rd_data[1],rd_data[2]);

	printk("[%s]id = %x\n", __FUNCTION__,rd_data[1]);
	printk("[%s]version = %x\n", __FUNCTION__,rd_data[2]);
	if(rd_data[0] != FW_VERSION_COMMAND)
	{
		printk("Fail to Verify command ack\n");
		return -1;
	}
	if(rd_data[1] != TOUCH_KEY_ID)
	{
		printk("Fail to Verify ID\n");
		return -1;
	}

	return 0;
}


static int mv_touch_key_init_gpio(struct touch_key *touch_key)
{
	int error;
//	int irq_gpio = touch_key->irq - IRQ_DOVE_GPIO_START;
      
//	error = gpio_request(irq_gpio,"TOUCH_KEY_IRQ");       
//	if(error!=0)
//		return -1;

//	gpio_direction_input(irq_gpio);
	set_irq_type(touch_key->irq,IRQ_TYPE_LEVEL_HIGH);

	return 0;

}


static int __devinit mv_touch_key_probe(struct i2c_client *client,
			       const struct i2c_device_id *id)
{
	struct input_dev *input;
	struct touch_key *touch_key;
	int i;
	int error;
	struct i2c_adapter      *i2c_adapter = to_i2c_adapter(client->dev.parent);

#ifdef DEBUG
	printk("%s\n",__FUNCTION__);
#endif

	if (!i2c_check_functionality(i2c_adapter,
				  I2C_FUNC_SMBUS_EMUL))
		return -EIO;


	if(touch_key_hw_init(client) != 0)
	{
		return -ENODEV;
	}

	printk("MV TOUCH_KEY I2C driver loaded successfully\n");

	touch_key = kzalloc(sizeof(struct touch_key), GFP_KERNEL);

	touch_key->i2c_adapter = i2c_adapter;
	touch_key->i2c_client = client;

	input = input_allocate_device();
	if (!touch_key || !input)
	{
		error = -ENOMEM;
		goto err_free_mem;
	}


	dev_set_drvdata(&client->dev,touch_key);
	input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);

	for(i=0;i<NUM_TOUCH_KEY;i++)
	{
		input->keybit[BIT_WORD(touch_key_code[i])] |= BIT_MASK(touch_key_code[i]);
	}


	input->name = client->name;
	input->dev.parent = &client->dev;

	input->id.bustype = BUS_I2C;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;

	touch_key->input_dev = input;

	touch_key->irq  = client->irq;

	mv_touch_key_init_gpio(touch_key);

	error = input_register_device(input);
	if (error) {
		printk(KERN_ERR "Unable to register touch key input device\n");
		goto err_free_mem;
	}


	init_timer(&touch_key->polling_timer);
	touch_key->polling_timer.data = (unsigned long)touch_key;
	touch_key->polling_timer.function = polling_timer_handler;

	touch_key->workqueue = create_singlethread_workqueue("touchkey_workqueue");

	if (touch_key->workqueue == NULL) {
		goto err_free_mem;
	}

	INIT_WORK(&touch_key->work, mv_touch_key_work);

	error = request_irq(touch_key->irq, touch_key_isr, 0,
				    "touchkey_irq",
				    touch_key);
	if (error) {
		printk(KERN_ERR "touch-keys: unable to claim irq %d; error %d\n",
		       touch_key->irq, error);
		goto err_free_irq;
	}

	return 0;


err_free_irq:
	destroy_workqueue(touch_key->workqueue);
	if(touch_key->touch_key_active)
		free_irq(touch_key->irq,NULL);
err_free_mem:

	input_free_device(input);
	kfree(touch_key);
	return error;
}

static int __devexit mv_touch_key_remove(struct i2c_client *client)
{

	struct touch_key *touch_key = dev_get_drvdata(&client->dev);
	printk("%s\n",__FUNCTION__);	

	gpio_free(touch_key->irq - IRQ_DOVE_GPIO_START);       


	flush_workqueue(touch_key->workqueue);
	destroy_workqueue(touch_key->workqueue);

	input_unregister_device(touch_key->input_dev);
	input_free_device(touch_key->input_dev);

	if(touch_key->touch_key_active)
		free_irq(touch_key->irq,NULL);


	kfree(touch_key);
	return 0;
}




static const struct i2c_device_id touch_key_i2c_id[] = {
     { "touch_key_i2c", 0 },
     { }
};

static struct i2c_driver mv_touch_key_driver = {
     .driver = {
	  .name   = TOUCH_KEY_DRV_NAME,
	  .owner  = THIS_MODULE,
     },
     .probe          = mv_touch_key_probe,
     .remove         = __devexit_p(mv_touch_key_remove),
     .id_table       = touch_key_i2c_id,
};

static int __init mv_touch_key_init(void)
{
     return i2c_add_driver(&mv_touch_key_driver);
}

static void __exit mv_touch_key_exit(void)
{
     i2c_del_driver(&mv_touch_key_driver);
}



late_initcall(mv_touch_key_init);
module_exit(mv_touch_key_exit);

MODULE_LICENSE("GPL");
