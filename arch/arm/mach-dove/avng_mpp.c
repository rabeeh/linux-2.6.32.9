/*
 * arch/arm/plat-orion/mv_hal_drivers/mv_drivers_lsp/mv_mpp/mv_mpp.c
 *
 * Marvell Orion SoC MPP handling
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/bitops.h>
#include <linux/sysdev.h>
#include <linux/platform_device.h>
#include <asm/gpio.h>
#include <linux/delay.h>


#ifdef CONFIG_MACH_DOVE_RD_AVNG
#define MPP_Amp_PwrDn		2
#define MPP_GePhy_PwrDn		2
#define MPP_STBY_DETECTED	3
#define MPP_SHDN_DETECTED	4
#endif


/*
 * /sys/devices/platform/mv_mpp for MPP related hardware specific control
 *
 */

#ifdef CONFIG_MACH_DOVE_RD_AVNG

/*
 * For Avengers
 *
 *   /audio_amp_sw
 *      * used to control the power supplied to audio amplifier
 *      * is read/write as "on" or "off"
 *
 *   /eth_phy_sw
 *      * used to control the ethernet PHY entering COMA state
 *      * is read/write as "on" or "off"
 *
 *   /standby_detect
 *      * used to determine if the system is required to stand by immediately
 *      * is read only as "1" or "0"
 *
 *   /shutdown_detect
 *      * used to determine if the system is required to shut down immediately
 *      * is read only as "1" or "0"
 *
 */

static const char *status_str[2] = { "off", "on" };

static ssize_t store_mpp_powerdown(int pin, const char *buf, size_t count)
{
	char state[8];
	int value;

	if (sscanf(buf, "%3s", state) != 1)
		return -EINVAL;

	if (strnicmp(state, status_str[0], 3) == 0)
		value = 1;
	else if (strnicmp(state, status_str[1], 2) == 0)
		value = 0;
	else
		return -EINVAL;

	gpio_direction_output(pin, value);

	return count;
}

static ssize_t show_mpp_powerdown(int pin, char *buf)
{
	int ret;

	ret = gpio_get_value(pin);

	if (ret < 0)
		return ret;

	return sprintf(buf, "%s\n", ret ? status_str[0] : status_str[1]);
}

#define CREATE_DEVICE_ATTR(_name, _pin)					\
	static ssize_t show_##_name(struct device *dev,			\
				    struct device_attribute *attr,	\
				    char *buf)				\
	{								\
		return show_mpp_powerdown(_pin, buf);			\
	}								\
	static ssize_t store_##_name(struct device *dev,		\
				     struct device_attribute *attr,	\
				     const char *buf, size_t count)	\
	{								\
		return store_mpp_powerdown(_pin, buf, count);		\
	}								\
	static struct device_attribute dev_attr_##_name = {		\
		.attr = {						\
			.name = __stringify(_name),			\
			.mode = 0644,					\
			.owner = THIS_MODULE },				\
		.show   = show_##_name,					\
		.store  = store_##_name,				\
	}

CREATE_DEVICE_ATTR(eth_phy_sw, MPP_GePhy_PwrDn);
CREATE_DEVICE_ATTR(audio_amp_sw, MPP_Amp_PwrDn);

static ssize_t show_standby_detect(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n",
			gpio_get_value(MPP_STBY_DETECTED));
}

static DEVICE_ATTR(standby_detect, S_IRUSR, show_standby_detect, NULL);

static ssize_t show_shutdown_detect(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n",
			gpio_get_value(MPP_SHDN_DETECTED));
}

static DEVICE_ATTR(shutdown_detect, S_IRUSR, show_shutdown_detect, NULL);

#endif

#ifdef CONFIG_MV_MPP_TEST

/*
 * write_mpp_test - write method for "test" file in sysfs
 */
static ssize_t write_mpp_test(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	char command[8];
	int pin, value = 0, ret;

	ret = sscanf(buf, "%7s:%d:%x", command, &pin, &value);
	if (ret == 2 || ret == 3)
		goto process_command;

	ret = sscanf(buf, "%6s:%d:%x", command, &pin, &value);
	if (ret == 2 || ret == 3)
		goto process_command;

	ret = sscanf(buf, "%5s:%d:%x", command, &pin, &value);
	if (ret == 2 || ret == 3)
		goto process_command;

	ret = sscanf(buf, "%4s:%d:%x", command, &pin, &value);
	if (ret == 2 || ret == 3)
		goto process_command;

	return -EINVAL;

process_command:

	if (strnicmp(command, "getval", 6) == 0) {
		ret = gpio_get_value(pin);
		if (ret >=0)
			printk("Pin-%d: value=%x\n", pin, ret);
	} else if (strnicmp(command, "free", 4) == 0) {
		gpio_free(pin);
		ret = 0;
	} else if (strnicmp(command, "request", 7) == 0) {
		ret = gpio_request(pin, "test");
	} else if (strnicmp(command, "setin", 5) == 0) {
		ret = gpio_direction_input(pin);
	} else if (strnicmp(command, "setout", 6) == 0) {
		ret = gpio_direction_output(pin, value);
	} else if (strnicmp(command, "setval", 6) == 0) {
		gpio_set_value(pin, value);
		ret = 0;
	} else {
		return -EINVAL;
	}

	if (ret < 0) {
		printk("%s failed (ret=%d)\n", command, ret);
		return ret;
	}

	return count;
}

static DEVICE_ATTR(test, S_IWUSR, NULL, write_mpp_test);

#endif

static struct attribute *mpp_attributes[] = {

#ifdef CONFIG_MACH_DOVE_RD_AVNG
	&dev_attr_audio_amp_sw.attr,
	&dev_attr_eth_phy_sw.attr,
	&dev_attr_standby_detect.attr,
	&dev_attr_shutdown_detect.attr,
#endif

#ifdef CONFIG_MV_MPP_TEST
	&dev_attr_test.attr,
#endif

	NULL
};

static struct attribute_group mpp_attribute_group = {
	.attrs = mpp_attributes
};

static int gpio_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int gpio_resume(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver mpp_driver = {
	.suspend	= gpio_suspend,
	.resume		= gpio_resume,
	.driver = {
		   .name = "mv_mpp",
		   .owner = THIS_MODULE,
		   },
};

static struct platform_device *mpp_device;

static void __init gpio_hw_device_reset(void)
{

#ifdef CONFIG_MACH_DOVE_RD_AVNG
	/* Reset audio codec */
	gpio_direction_output(55, 0);	/* IO_RST# for Audio_RST# and Hub_RST# */
	mdelay(5);
	gpio_direction_output(55, 1);
#endif

}

int __init mvmpp_sys_init(void)
{
	int ret;

	gpio_hw_device_reset();

	/* Register platform stuff */
	ret = platform_driver_register(&mpp_driver);
	if (ret)
		goto fail_platform_driver;

	mpp_device = platform_device_alloc("mv_mpp", -1);
	if (! mpp_device) {
		ret = -ENOMEM;
		goto fail_platform_device1;
	}

	ret = platform_device_add(mpp_device);
	if (ret)
		goto fail_platform_device2;

	ret = sysfs_create_group(&mpp_device->dev.kobj,
				&mpp_attribute_group);
	if (ret)
		goto fail_sysfs;

	printk(KERN_NOTICE "Marvell MPP Driver Load\n");

	return 0;

fail_sysfs:
	platform_device_del(mpp_device);

fail_platform_device2:
	platform_device_put(mpp_device);

fail_platform_device1:
	platform_driver_unregister(&mpp_driver);

fail_platform_driver:
	return ret;
}
