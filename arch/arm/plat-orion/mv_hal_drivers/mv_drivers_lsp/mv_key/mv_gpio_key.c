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
#include <asm/gpio.h>

#define NUM_GPIO_KEY 5


static const int gpio_key_mpp[NUM_GPIO_KEY] =
{
	7,
        8,
	9,
	10,
	11,
};


static const int gpio_key_code[NUM_GPIO_KEY] = 
{
	KEY_WWW,
	KEY_F11,
	KEY_ESC,
	KEY_UP,
	KEY_DOWN,
};


static const char *gpio_key_string[NUM_GPIO_KEY] = 
{
	"KEY_WWW",
	"KEY_F11",
	"KEY_ESC",
	"KEY_UP",
	"KEY_DOWN",
};


struct gpio_key
{
	struct input_dev *input_dev;

	int gpio_key_active;
	int irq;


	int gpio_key_pressed[NUM_GPIO_KEY];
	int gpio_key_irq[NUM_GPIO_KEY];
};

//#define DEBUG


/////////////////////////////////////////////////////////////////////////////////////
//gpio key
/////////////////////////////////////////////////////////////////////////////////////


static irqreturn_t gpio_key_isr(int irq, void *dev_id)
{
	struct gpio_key *gpio_key = (struct gpio_key *)dev_id;
	int i,j;

#ifdef DEBUG
	printk("%s = %d\n",__FUNCTION__,irq - IRQ_DOVE_GPIO_START);
#endif
	for(i=0;i<NUM_GPIO_KEY;i++)
	{
		if(irq == gpio_key->gpio_key_irq[i])
			break;
	}

	if(i == NUM_GPIO_KEY)
	{
		return IRQ_HANDLED;
	}


	for(j=0;j<NUM_GPIO_KEY;j++)
	{
		if(j!=i && gpio_key->gpio_key_pressed[j])
		{
			return IRQ_HANDLED;
		}
	}


	if(gpio_key->gpio_key_pressed[i] == 0)
	{ //pressed
#ifdef DEBUG
		printk("%s pressed\n",gpio_key_string[i]);
#endif

		//we'll issue interrupt when release
		set_irq_type(irq,IRQ_TYPE_LEVEL_LOW);
		gpio_key->gpio_key_pressed[i] = 1;

		input_report_key(gpio_key->input_dev, gpio_key_code[i], 1);
		input_sync(gpio_key->input_dev);


	}else{
#ifdef DEBUG
		printk("%s released\n",gpio_key_string[i]);
#endif
		//release
		set_irq_type(irq,IRQ_TYPE_LEVEL_HIGH);		
		gpio_key->gpio_key_pressed[i] = 0;
		input_report_key(gpio_key->input_dev, gpio_key_code[i], 0);
		input_sync(gpio_key->input_dev);

	}

	return IRQ_HANDLED;
}


static int gpio_key_hw_init(struct gpio_key *gpio_key)
{

	int i;
	int ret;
	int gpio;

	for(i=0;i<NUM_GPIO_KEY;i++)
	{
		gpio = gpio_key_mpp[i];
//		ret = gpio_request(gpio,gpio_key_string[i]);
//		if(ret!=0)
//			return -1;
//		gpio_direction_input(gpio);
		set_irq_type(gpio + IRQ_DOVE_GPIO_START ,IRQ_TYPE_LEVEL_HIGH);
		gpio_key->gpio_key_irq[i] = gpio + IRQ_DOVE_GPIO_START;
	}


	return 0;

}




static int __devinit mv_gpio_key_probe(struct platform_device *pdev)
{
//	struct mv_gpio_key_platform_data *pdata = pdev->dev.platform_data;
	struct input_dev *input;
	struct gpio_key *gpio_key;
	int i;
	int error;
#ifdef DEBUG
	printk("%s\n",__FUNCTION__);
#endif

	gpio_key = kzalloc(sizeof(struct gpio_key), GFP_KERNEL);
	input = input_allocate_device();
	if (!gpio_key || !input)
	{
		error = -ENOMEM;
		goto err_free_mem;
	}
	dev_set_drvdata(&pdev->dev, gpio_key);

	platform_set_drvdata(pdev, input);

	input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);

	for(i=0;i<NUM_GPIO_KEY;i++)
	{
		input->keybit[BIT_WORD(gpio_key_code[i])] |= BIT_MASK(gpio_key_code[i]);
	}

	input->name = pdev->name;
	input->dev.parent = &pdev->dev;

	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;

	gpio_key->input_dev = input;


	error = input_register_device(input);
	if (error) {
		printk(KERN_ERR "Unable to register gpio key input device\n");
		goto err_free_mem;
	}



	if(gpio_key_hw_init(gpio_key) == -1)
	{
		error = -ENODEV;
		printk(KERN_ERR " gpio key device didn't exist\n");
		goto err_free_irq;
	}


	for(i=0;i<NUM_GPIO_KEY;i++)
	{
		
		error = request_irq(gpio_key->gpio_key_irq[i], gpio_key_isr, 0,
				    pdev->dev.driver->name,
				    gpio_key);
	
		if (error) {
			printk(KERN_ERR "gpio-keys: unable to claim irq %d; error %d\n",
			       gpio_key->irq, error);
			goto err_free_irq;
		}
	
	}


	return 0;


err_free_irq:
	if(gpio_key->gpio_key_active)
		free_irq(gpio_key->irq,pdev);
err_free_mem:

	input_free_device(input);
	kfree(gpio_key);
	return error;
}

static int __devexit mv_gpio_key_remove(struct platform_device *pdev)
{
	int i;
	struct gpio_key *gpio_key = dev_get_drvdata(&pdev->dev);
#ifdef DEBUG
	printk("%s\n",__FUNCTION__);	
#endif
	input_unregister_device(gpio_key->input_dev);
	input_free_device(gpio_key->input_dev);

	if(gpio_key->gpio_key_active)
		free_irq(gpio_key->irq,pdev);


	for(i=0;i<NUM_GPIO_KEY;i++)
	{
		gpio_free(gpio_key->gpio_key_irq[i] - IRQ_DOVE_GPIO_START );
		free_irq(gpio_key->gpio_key_irq[i],pdev);
	}

	kfree(gpio_key);
	return 0;
}



/*-------------------------------------------------------------------------*/
struct platform_driver mv_gpio_key_device_driver = {
	.probe		= mv_gpio_key_probe,
	.remove		= __devexit_p(mv_gpio_key_remove),
	.driver		= {
		.name	= "mv_gpio_key",
	}
};

static int __init mv_gpio_key_init(void)
{
//	printk("%s\n",__FUNCTION__);
	return platform_driver_register(&mv_gpio_key_device_driver);
}

static void __exit mv_gpio_key_exit(void)
{
	platform_driver_unregister(&mv_gpio_key_device_driver);
}

late_initcall(mv_gpio_key_init);
module_exit(mv_gpio_key_exit);

MODULE_LICENSE("GPL");
