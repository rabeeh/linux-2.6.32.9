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

#include <linux/i2c.h>
#include "mv_touch_key_slider.h"
#include <asm/gpio.h>

//#define DEBUG


#define TOUCH_SLIDER_SLAVE_ADDR 0x0a


#define SLIDER_DATA_COMMAND 0x01
#define SLIDER_VELOCITY_DATA_COMMAND 0x02
#define FW_VERSION_COMMAND 0x16
#define TOUCH_SLIDER_ID 0x62
#define DUMMY_DATA 0x00

#define POLLING_TIMER 30 //msec

#define JITTER_RANGE 2

#define NUM_KEYS 8
#define MIN_VAL 0x08
#define MAX_VAL 0x30
#define STEP (MAX_VAL - MIN_VAL ) / (NUM_KEYS-1)


static const unsigned char mapping_table[] = 
{
	0,			//0
	MIN_VAL,		//1
	MIN_VAL + STEP,		//2
	MIN_VAL + STEP*2 ,	//3
	MIN_VAL + STEP*3 ,	//4
	MIN_VAL + STEP*4 ,	//5
	MIN_VAL + STEP*5 ,	//6
	MIN_VAL + STEP*6 ,	//7
	MAX_VAL,		//8
	0xff,
};


//#define USE_WHEEL
#define DECREASE 0
#define INCREASE 1



static const int touch_slider_code[2]=
{
	KEY_UP,		//DECREASE
	KEY_DOWN,	//INCREASE
};

#ifdef DEBUG
static const char * touch_slider_string[2]=
{
	"KEY_UP",		//DECREASE
	"KEY_DOWN",	//INCREASE
};
#endif

struct touch_slider
{
	struct input_dev *input_dev;
	int irq;

	int old_value;
	int touch_pressed;
	int current_touch_index;
	int old_touch_index;
	struct timer_list polling_timer;
	struct workqueue_struct *workqueue;
	struct work_struct work;

	struct i2c_adapter      *i2c_adapter;
	struct i2c_client *i2c_client;
};




static irqreturn_t touch_slider_isr(int irq, void *dev_id)
{
	struct touch_slider *touch_slider = (struct touch_slider *)dev_id;

#ifdef DEBUG
	printk("%s\n",__FUNCTION__);
#endif

	
	if(touch_slider->touch_pressed == 0)
	{
		mod_timer(&touch_slider->polling_timer, jiffies);
		disable_irq_nosync(irq);
	}
	
	return IRQ_HANDLED;
}


static void polling_timer_handler(unsigned long handle)
{

	struct touch_slider *touch_slider = (void *)handle;
#ifdef DEBUG
	printk("%s\n",__FUNCTION__);
#endif
	queue_work(touch_slider->workqueue, &touch_slider->work);


}


static void mv_touch_slider_work(struct work_struct *work)
{
	struct touch_slider *touch_slider = container_of(work, struct touch_slider, work);
	struct input_dev *ip = touch_slider->input_dev;
	int movement;

	int last_valid;
	unsigned char rd_data[3];

	int i;

#ifdef DEBUG
	printk("%s\n",__FUNCTION__);
#endif

	if(i2c_master_recv(touch_slider->i2c_client,rd_data,3) !=3)
	{
		printk("Fail to Read I2C\n");
		return;
	}
#ifdef DEBUG
	printk("%s: %x \n",__FUNCTION__,rd_data[1]);
#endif


	if(rd_data[1] == 0)
	{ // touch released
		#ifdef DEBUG
		printk("untouched\n");
		#endif
		touch_slider->old_value = 0;
		touch_slider->current_touch_index = 0;
		touch_slider->touch_pressed = 0;
		enable_irq(touch_slider->irq);
	}
	else
	{ // touch pressed


		if(abs(rd_data[1] - touch_slider->old_value) <= JITTER_RANGE)
		{
			rd_data[1] = touch_slider->old_value;
#ifdef DEBUG
			printk("after dejitter = %x \n",rd_data[1]);
#endif

		}else{
			touch_slider->old_value = rd_data[1];

		}

#ifdef DEBUG
		for(i = 0;mapping_table[i]!= 0xff ;i++)
		{
			printk("0x%x ",mapping_table[i]);
		}

		printk("\n");
#endif
		last_valid = 0;
		for(i = 0; mapping_table[i]!=0xff ;i++)
		{
			if(rd_data[1] > mapping_table[i])
			{
				last_valid = i;
			}
		}
		touch_slider->current_touch_index = last_valid;
#ifdef DEBUG
		printk("index = %d \n",touch_slider->current_touch_index);
#endif

//		touch_slider->current_touch_index = mapping_table[touch_slider->rd_data[1]];


		if(touch_slider->touch_pressed)
		{
			movement = touch_slider->current_touch_index - touch_slider->old_touch_index;
		}
		else
		{
			movement = 0;
			touch_slider->touch_pressed = 1;
		}
		
		if(movement !=0 )
		{
			int index;
			int i;


			index = movement > 0 ? INCREASE : DECREASE;
#ifdef DEBUG
			printk("move =%d\n",movement);
#endif

			for(i=0; i< abs(movement); i++)
			{
#ifdef USE_WHEEL
				input_report_key(touch_slider->input_dev, BTN_MIDDLE, 1);
				input_report_rel(touch_slider->input_dev, REL_WHEEL, movement > 0?1:-1);
				input_sync(touch_slider->input_dev);				
#else

				#ifdef DEBUG
				printk("%s press\n",touch_slider_string[index]);
				#endif
				input_report_key(touch_slider->input_dev, touch_slider_code[index], 1);
				input_sync(ip);

				#ifdef DEBUG
				printk("%s release\n",touch_slider_string[index]);
				#endif
				input_report_key(touch_slider->input_dev, touch_slider_code[index], 0);
				input_sync(ip);
#endif
			
			}
		}
		touch_slider->old_touch_index = touch_slider->current_touch_index;
		mod_timer(&touch_slider->polling_timer, jiffies + msecs_to_jiffies(POLLING_TIMER));
	}


}






static int touch_slider_hw_init(struct i2c_client *client)
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
	if(rd_data[1] != TOUCH_SLIDER_ID)
	{
		printk("Fail to Verify ID\n");
		return -1;
	}

	return 0;

}



static int mv_touch_slider_init_gpio(struct touch_slider *touch_slider)
{
	int ret = 0;
//	int irq_gpio = touch_slider->irq - IRQ_DOVE_GPIO_START;
//	printk("[%s] =%d\n",__FUNCTION__,irq_gpio);
//	ret = gpio_request(irq_gpio,"TOUCH_SLIDER_IRQ");       
//	if(ret!=0)
//	{
//		printk("[%s] request gpio err",__FUNCTION__);
//		return -1;
//	}
//	gpio_direction_input(irq_gpio);
	set_irq_type(touch_slider->irq,IRQ_TYPE_LEVEL_HIGH);

	return 0;
}



static int __devinit mv_touch_slider_probe(struct i2c_client *client,
			       const struct i2c_device_id *id)
{

	struct input_dev *input;
	int error;
	struct touch_slider *touch_slider;

	struct i2c_adapter      *i2c_adapter = to_i2c_adapter(client->dev.parent);

#ifdef DEBUG
	printk("%s\n",__FUNCTION__);
#endif

	if (!i2c_check_functionality(i2c_adapter,
				  I2C_FUNC_SMBUS_EMUL))
		return -EIO;


	if(touch_slider_hw_init(client) != 0)
	{
		return -ENODEV;
	}

	printk("MV TOUCH_SLIDER I2C driver loaded successfully\n");


	touch_slider = kzalloc(sizeof(struct touch_slider), GFP_KERNEL);
	input = input_allocate_device();
	if (!touch_slider || !input)
	{
		error = -ENOMEM;
		goto err_free_mem;
	}
	dev_set_drvdata(&client->dev, touch_slider);

	#ifdef USE_WHEEL
	input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
	input->relbit[0] = BIT_MASK(REL_WHEEL);
	input->keybit[BIT_WORD(BTN_MIDDLE)] = BIT_MASK(BTN_MIDDLE);

	#else
	input->evbit[0] = BIT_MASK(EV_KEY);
	input->keybit[BIT_WORD(KEY_UP)] = BIT_MASK(KEY_UP);
	input->keybit[BIT_WORD(KEY_DOWN)] |= BIT_MASK(KEY_DOWN);
	#endif

	input->name = client->name;
	input->dev.parent = &client->dev;

	input->id.bustype = BUS_I2C;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;

	touch_slider->input_dev = input;

	touch_slider->irq  = client->irq;


	mv_touch_slider_init_gpio(touch_slider);

	error = input_register_device(input);
	if (error) {
		printk(KERN_ERR "Unable to register touch key input device\n");
		goto err_free_mem;
	}

	init_timer(&touch_slider->polling_timer);
	touch_slider->polling_timer.data = (unsigned long)touch_slider;
	touch_slider->polling_timer.function = polling_timer_handler;


	touch_slider->workqueue = create_singlethread_workqueue("touchsilder_workqueue");

	if (touch_slider->workqueue == NULL) {
		goto err_free_mem;
	}

	INIT_WORK(&touch_slider->work, mv_touch_slider_work);
     
	error = request_irq(touch_slider->irq, touch_slider_isr, 0,
			    "touchslider_irq",
			    touch_slider);
	if (error) {
		printk(KERN_ERR "touch slider: unable to claim irq %d; error %d\n",
		       touch_slider->irq, error);
			goto err_free_irq;
		}

	return 0;



err_free_irq:
	destroy_workqueue(touch_slider->workqueue);
	free_irq(touch_slider->irq,NULL);
err_free_mem:

	input_free_device(input);
	kfree(touch_slider);
	return error;
}

static int __devexit mv_touch_slider_remove(struct i2c_client *client)
{
	struct touch_slider *touch_slider = dev_get_drvdata(&client->dev);

	gpio_free(touch_slider->irq);       

	flush_workqueue(touch_slider->workqueue);
	destroy_workqueue(touch_slider->workqueue);

	input_unregister_device(touch_slider->input_dev);
	input_free_device(touch_slider->input_dev);
	free_irq(touch_slider->irq,NULL);
	kfree(touch_slider);

	return 0;
}



static const struct i2c_device_id touch_slider_i2c_id[] = {
     { "touch_slider_i2c", 0 },
     { }
};

static struct i2c_driver mv_touch_slider_driver = {
     .driver = {
	  .name   = TOUCH_SLIDER_DRV_NAME,
	  .owner  = THIS_MODULE,
     },
     .probe          = mv_touch_slider_probe,
     .remove         = __devexit_p(mv_touch_slider_remove),
     .id_table       = touch_slider_i2c_id,
};

static int __init mv_touch_slider_init(void)
{
     return i2c_add_driver(&mv_touch_slider_driver);
}

static void __exit mv_touch_slider_exit(void)
{
     i2c_del_driver(&mv_touch_slider_driver);
}



late_initcall(mv_touch_slider_init);
module_exit(mv_touch_slider_exit);


MODULE_LICENSE("GPL");
