/*
 * linux/drivers/backlight/dove_bl.c -- Marvell DOVE LCD Backlight driver.
 *
 * Copyright (C) Marvell Semiconductor Company.  All rights reserved.
 *
 * Written by Shadi Ammouri <shadi@marvell.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive for
 * more details.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/lcd.h>
#include <linux/clk.h>
#include <video/dovefbreg.h>
#include <mach/dove_bl.h>

#define DOVEBL_BL_DIV	     0x146	/* 100Hz */

struct dove_backlight {
	int powermode;          /* blacklight */
	int lcd_powermode;      /* lcd panel */
	int current_intensity;

	struct clk *clk;
	void *lcdpwr_reg;
	void *blpwr_reg;
	void *btn_reg;
	struct device *dev;
	struct dovebl_platform_data platform_data;

	unsigned long lcd_mask;		/* mask */
	unsigned long lcd_on;		/* value to enable lcd power */
	unsigned long lcd_off;		/* value to disable lcd power */

	unsigned long blpwr_mask;	/* mask */
	unsigned long blpwr_on;		/* value to enable bl power */
	unsigned long blpwr_off;	/* value to disable bl power */

	unsigned long btn_mask;	/* mask */
	unsigned long btn_level;	/* how many level can be configured. */
	unsigned long btn_min;	/* min value */
	unsigned long btn_max;	/* max value */
	unsigned long btn_inc;	/* increment */

};

void dovebl_config_reg(unsigned long *reg, unsigned long mask, unsigned long value)
{
	unsigned int x;
	volatile unsigned long* addr;

	addr = reg;
	x = *addr;
	x &= ~(mask);	/* clear */
	x |= value;
	*addr = x;
}

static int dovebl_update_status(struct backlight_device *dev)
{
	struct dove_backlight *bl = dev_get_drvdata(&dev->dev);
	u32 brightness = dev->props.brightness;

	if ((dev->props.power != FB_BLANK_UNBLANK) ||
		(dev->props.fb_blank != FB_BLANK_UNBLANK) ||
		(bl->powermode == FB_BLANK_POWERDOWN)) {
		brightness = 0;
	} else {
		/*
		 * brightness = 0 is 1 in the duty cycle in order not to
		 * fully shutdown the backlight. framework check max
		 * automatically.
		 */
		if (bl->btn_min > brightness) {
			brightness = bl->btn_min;
			printk(KERN_WARNING "brightness level is too small, "
				"use min value\n");
		}
	}
	
	if (bl->current_intensity != brightness) {
		dovebl_config_reg(bl->btn_reg,
			bl->btn_mask,
			(bl->btn_inc*brightness*0x10000000)); 
		bl->current_intensity = brightness;

		if (brightness != 0) {
			/* Set backlight power on. */
			dovebl_config_reg(bl->blpwr_reg,
			    bl->blpwr_mask,
			    bl->blpwr_on);
			printk( KERN_INFO "backlight power on\n");
		} else {
			/* Set backlight power down. */
			dovebl_config_reg(bl->blpwr_reg,
			    bl->blpwr_mask,
			    bl->blpwr_off);
			printk( KERN_INFO "backlight power down\n");
		}
	}

	return 0;
}

static int dovebl_get_brightness(struct backlight_device *dev)
{
	struct dove_backlight *bl = dev_get_drvdata(&dev->dev);

	return bl->current_intensity;
}

/*
 * Check if given framebuffer device is the one LCD is bound to;
 * return 0 if not, !=0 if it is.
 */
static int dove_check_fb(struct fb_info *fi)
{
	/* LCD Panel should be bound to LCD0 gfx fb driver. */
	if (strstr(fi->fix.id, "GFX Layer 0"))
		return 1;

	return 0;
}

static int dovebl_check_fb(struct fb_info *fi)
{
	return dove_check_fb(fi);
}

static struct backlight_ops dovebl_ops = {
	.get_brightness = dovebl_get_brightness,
	.update_status  = dovebl_update_status,
	.check_fb	= dovebl_check_fb,
};

#ifdef CONFIG_PM
static int dovebl_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct backlight_device *bd = platform_get_drvdata(pdev);
	struct dove_backlight *bl = bl_get_data(bd);

	bl->powermode = FB_BLANK_POWERDOWN;
	backlight_update_status(bd);
	clk_disable(bl->clk);

	return 0;
}

static int dovebl_resume(struct platform_device *pdev)
{
	struct backlight_device *bd = platform_get_drvdata(pdev);
	struct dove_backlight *bl = bl_get_data(bd);


	clk_enable(bl->clk);

	bl->powermode = FB_BLANK_UNBLANK;
	backlight_update_status(bd);

	return 0;
}
#endif

static int dove_lcd_get_power(struct lcd_device *ld)
{
	struct dove_backlight *bl = lcd_get_data(ld);

	return bl->lcd_powermode;
}

/* Enable or disable power to the LCD (0: on; 4: off, see FB_BLANK_XXX) */
static int dove_lcd_set_power(struct lcd_device *ld, int power)
{
	struct dove_backlight *bl = lcd_get_data(ld);

	switch (power) {
	case FB_BLANK_UNBLANK:          /* 0 */
		/* Set LCD panel power on. */
		dovebl_config_reg(bl->lcdpwr_reg, bl->lcd_mask, bl->lcd_on);
		pr_debug("panel power on(FB_BLANK_UNBLANK)\n");
		break;
	case FB_BLANK_POWERDOWN:        /* 4 */
		/* Set LCD panel power down. */
		dovebl_config_reg(bl->lcdpwr_reg, bl->lcd_mask, bl->lcd_off);
		pr_debug("panel power down(FB_BLANK_POWERDOWN)\n");
		break;
	case FB_BLANK_NORMAL:
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
		/* Set LCD panel power down. */
		dovebl_config_reg(bl->lcdpwr_reg,
			bl->lcd_mask,
			bl->lcd_off);

		pr_debug("panel power down(FB_BLANK_NORMAL)\n");
		break;

	default:
		return -EIO;
	}

	bl->lcd_powermode = power;

	return 0;
}

static int dove_lcd_check_fb(struct lcd_device *ld, struct fb_info *fi)
{
	return dove_check_fb(fi);
}

static struct lcd_ops dove_lcd_ops = {
	.get_power      = dove_lcd_get_power,
	.set_power      = dove_lcd_set_power,
	.check_fb	= dove_lcd_check_fb,
};

static int dovebl_probe(struct platform_device *pdev)
{
	struct backlight_device *dev;
	struct dove_backlight *bl;
	struct lcd_device *ldp;
	struct dovebl_platform_data *dbm = pdev->dev.platform_data;
	struct resource *res;

	if (!dbm)
		return -ENXIO;

	bl = kzalloc(sizeof(struct dove_backlight), GFP_KERNEL);
	if (bl == NULL)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		kfree(bl);
		return -EINVAL;
	}

	/* Get lcd power control reg. */
	if (0 == dbm->lcd_mapped) 
		bl->lcdpwr_reg = ioremap_nocache(dbm->lcd_start,
			dbm->lcd_end - dbm->lcd_start);
	else
		bl->lcdpwr_reg = (void *)dbm->lcd_start;

	if (bl->lcdpwr_reg == NULL) {
		printk(KERN_ERR "Can't map lcd power control reg\n");
		kfree(bl);
		return -ENOMEM;
	}
	bl->lcdpwr_reg += dbm->lcd_offset;

	/* Get backlight power control reg. */
	if (0 == dbm->blpwr_mapped) 
		bl->blpwr_reg = ioremap_nocache(dbm->blpwr_start,
			dbm->blpwr_end - dbm->blpwr_start);
	else
		bl->blpwr_reg = (void*)dbm->blpwr_start;
		
	if (bl->blpwr_reg == NULL) {
		kfree(bl);
		return -ENOMEM;
	}
	bl->blpwr_reg += dbm->blpwr_offset;

	/* Get brightness control reg. */
	if (0 == dbm->btn_mapped) 
		bl->btn_reg = ioremap_nocache(dbm->btn_start,
			dbm->btn_end - dbm->btn_start);
	else
		bl->btn_reg = (void*)dbm->btn_start;
	
	if (bl->btn_reg == NULL) {
		kfree(bl);
		return -ENOMEM;
	}
	bl->btn_reg += dbm->btn_offset;

	dev = backlight_device_register("dove-bl", &pdev->dev, bl, &dovebl_ops);
	if (IS_ERR(dev)) {
		iounmap(bl->btn_reg);
		iounmap(bl->blpwr_reg);
		iounmap(bl->lcdpwr_reg);
		kfree(bl);
		return PTR_ERR(dev);
	}

	if (dbm->gpio_pm_control) {
		ldp = lcd_device_register("dove-lcd", &pdev->dev, bl,
					  &dove_lcd_ops);
		if (IS_ERR(ldp))
			return PTR_ERR(ldp);
	}

	bl->powermode = FB_BLANK_UNBLANK;
	bl->lcd_powermode = FB_BLANK_UNBLANK;
	bl->platform_data = *dbm;
	bl->dev = &pdev->dev;

        bl->lcd_mask	= dbm->lcd_mask;
	bl->lcd_on	= dbm->lcd_on;
        bl->lcd_off	= dbm->lcd_off;

	bl->blpwr_mask	= dbm->blpwr_mask;
	bl->blpwr_on	= dbm->blpwr_on;
	bl->blpwr_off	= dbm->blpwr_off;

	bl->btn_mask	= dbm->btn_mask;
	bl->btn_level	= dbm->btn_level;
	bl->btn_min	= dbm->btn_min;
	bl->btn_max	= dbm->btn_max;
	bl->btn_inc	= dbm->btn_inc;

	platform_set_drvdata(pdev, dev);

	/* Get LCD clock information. */
	bl->clk = clk_get(&pdev->dev, "LCDCLK");

	dev->props.fb_blank = FB_BLANK_UNBLANK;
	dev->props.max_brightness = dbm->btn_max;
	dev->props.brightness = dbm->default_intensity;

	/* set default PWN to 100Hz. */
	dovebl_config_reg(bl->btn_reg,
			0x0FFF0000,
			(DOVEBL_BL_DIV << 16)); 

	pr_debug("Dove Backlight Driver start power on sequence.\n");
	/* power on backlight and set default brightness */
	dovebl_update_status(dev);

	/* power on LCD panel. */
	pr_debug("first time to"
		"panel power on(FB_BLANK_UNBLANK)\n");
	dovebl_config_reg(bl->lcdpwr_reg, bl->lcd_mask, bl->lcd_on);

	printk(KERN_INFO "Dove Backlight Driver Initialized.\n");
	return 0;
}

static int dovebl_remove(struct platform_device *pdev)
{
	struct backlight_device *dev = platform_get_drvdata(pdev);
	struct dove_backlight *bl = dev_get_drvdata(&dev->dev);

	backlight_device_unregister(dev);
	kfree(bl);

	printk(KERN_INFO "Dove Backlight Driver Unloaded\n");
	return 0;
}

static struct platform_driver dovebl_driver = {
	.probe		= dovebl_probe,
	.remove		= dovebl_remove,
#ifdef CONFIG_PM
	.suspend	= dovebl_suspend,
	.resume		= dovebl_resume,
#endif
	.driver		= {
		.name	= "dove-bl",
	},
};

static int __init dovebl_init(void)
{
	return platform_driver_register(&dovebl_driver);
}

static void __exit dovebl_exit(void)
{
	platform_driver_unregister(&dovebl_driver);
}

module_init(dovebl_init);
module_exit(dovebl_exit);

MODULE_AUTHOR("Shadi Ammouri <shadi@marvell.com>");
MODULE_DESCRIPTION("Dove Backlight Driver");
MODULE_LICENSE("GPL");
