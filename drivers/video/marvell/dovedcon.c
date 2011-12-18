/*
 * linux/drivers/video/dovedcon.c -- Marvell DCON driver for DOVE
 *
 *
 * Copyright (C) Marvell Semiconductor Company.  All rights reserved.
 *
 * Written by:
 *	Green Wan <gwan@marvell.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive for
 * more details.
 *
 */

#include <video/dovedcon.h>

#define VGA_CHANNEL_DEFAULT	0x90C78
static int dovedcon_enable(struct dovedcon_info *ddi)
{
	unsigned int channel_ctrl;
	unsigned int channel_status;
	unsigned int ctrl0;
	void *addr;
	int i;

	/*
	 * Get current configuration of CTRL0
	 */
	ctrl0 = readl(ddi->reg_base+DCON_CTRL0);

#ifdef CONFIG_FB_DOVE_CLCD_DCONB_BYPASS0
	/* enable lcd0 pass to PortB */
	ctrl0 &= ~(0x3 << 8);
	ctrl0 |= (0x1 << 8);
	ddi->port_b = 1;
#endif
	
	/*
	 * Enable VGA clock, clear it to enable.
	 */
	if (ddi->port_b)
		ctrl0 &= ~(ddi->port_b << 25);
	else
		ctrl0 |= (0x1 << 25);
		
	/*
	 * Enable LCD clock, clear it to enable
	 * Enable LCD Parallel Interface, clear it to enable
	 */
	if (ddi->port_a)
		ctrl0 &= ~(ddi->port_a << 24);
	else
		ctrl0 |= (0x1 << 24);

	if (!ddi->port_a && !ddi->port_b)
		ctrl0 |= (0x1 << 17);
	else
		ctrl0 &= ~(0x1 << 17);

	writel(ctrl0, ddi->reg_base+DCON_CTRL0);

	addr = ddi->reg_base+DCON_VGA_DAC_CHANNEL_A_CTRL;
	for (i = 0; i < 3; i++) {
		channel_ctrl = readl(addr+(i*4));
		channel_status = readl(addr+(i*4)+0xc);
	}

	/*
	 * Configure VGA data channel and power on them.
	 */
	if (ddi->port_b) {
		int i;

		addr = ddi->reg_base+DCON_VGA_DAC_CHANNEL_A_CTRL;
		for (i = 0; i < 3; i++) {
			/* channel_ctrl = VGA_CHANNEL_DEFAULT; */
			channel_ctrl = ((ddi->bias_adj[i]	<< 19) |
					(ddi->vcal_ena[i]	<< 18) |
					(ddi->vcal_range[i]	<< 12) |
					(ddi->ovol_detect[i]	<< 16) |
					(ddi->impd_detect[i]	<< 11) |
					(ddi->use_37_5ohm[i]	<< 10) |
					(ddi->use_75ohm[i]	<<  9) |
					(ddi->gain[i]		<<  1)
					);
			writel(channel_ctrl, addr+(i*4));
		}
	}

	return 0;
}

static int dovedcon_disable(struct dovedcon_info *ddi)
{
	unsigned int channel_ctrl;
	unsigned int ctrl0;

	/*
	 * Get current configuration of CTRL0
	 */
	ctrl0 = readl(ddi->reg_base+DCON_CTRL0);

	if (!ddi->port_b) {
		/*
		 * Power down VGA data channel.
		 */
		channel_ctrl = (0x1 << 22);
		writel(channel_ctrl, ddi->reg_base+DCON_VGA_DAC_CHANNEL_A_CTRL);
		writel(channel_ctrl, ddi->reg_base+DCON_VGA_DAC_CHANNEL_B_CTRL);
		writel(channel_ctrl, ddi->reg_base+DCON_VGA_DAC_CHANNEL_C_CTRL);

		/*
		 * Disable VGA clock, set to 1 to disable it.
		 */
		ctrl0 |= (0x1 << 25);
	}

	if (!ddi->port_a)
		/*
		 * Disable LCD clock, set to 1 to disable it.
		 */
		ctrl0 |= (0x1 << 24);

	if (!ddi->port_a && !ddi->port_b)
		/*
		 * Disable LCD Parallel Interface, set to 1 to disable it.
		 */
		ctrl0 |= (0x1 << 17);

	writel(ctrl0, ddi->reg_base+DCON_CTRL0);
	
	return 0;
}


#ifdef CONFIG_PM

/* suspend and resume support */
static int dovedcon_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct dovedcon_info *ddi = platform_get_drvdata(pdev);

	printk(KERN_INFO "dovedcon_suspend().\n");

	dovedcon_disable(ddi);
	clk_disable(ddi->clk[0]);
	clk_disable(ddi->clk[1]);

	return 0;
}

static int dovedcon_resume(struct platform_device *pdev)
{
	struct dovedcon_info *ddi = platform_get_drvdata(pdev);

	printk(KERN_INFO "dovedcon_resume().\n");
	clk_enable(ddi->clk[0]);
	clk_enable(ddi->clk[1]);
	dovedcon_enable(ddi);

	return 0;
}

#endif

static int dovedcon_fb_event_callback(struct notifier_block *self,
				unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	struct dovedcon_info *ddi;
	struct fb_info *fi;
	int port_no = -1;

	/* If we aren't interested in this event, skip it immediately ... */
	if (event != FB_EVENT_BLANK && event != FB_EVENT_CONBLANK)
		return 0;

	ddi = container_of(self, struct dovedcon_info, fb_notif);

	fi = evdata->info;

	if (strstr(fi->fix.id, "GFX Layer 0"))
		port_no = 0;
	else if (strstr(fi->fix.id, "GFX Layer 1"))
		port_no = 1;

	if (-1 == port_no)
		return 0;

	if (*(int *)evdata->data == FB_BLANK_UNBLANK) {
		if (port_no)
			ddi->port_b = 1;
		else
			ddi->port_a = 1;
		dovedcon_enable(ddi);
	} else {
		if (port_no)
			ddi->port_b = 0;
		else
			ddi->port_a = 0;
		dovedcon_disable(ddi);
	}

	return 0;
}

static int dovedcon_hook_fb_event(struct dovedcon_info *ddi)
{
	memset(&ddi->fb_notif, 0, sizeof(ddi->fb_notif));
	ddi->fb_notif.notifier_call = dovedcon_fb_event_callback;
	return fb_register_client(&ddi->fb_notif);
}

/*
 * dcon sysfs interface implementation
 */
static ssize_t dcon_show_pa_clk(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dovedcon_info *ddi;

	ddi = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", ddi->port_a);
}

static ssize_t dcon_ena_pa_clk(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	struct dovedcon_info *ddi;
	unsigned long ena_clk;

	ddi = dev_get_drvdata(dev);
	rc = strict_strtoul(buf, 0, &ena_clk);
	if (rc)
		return rc;

	rc = -ENXIO;
	
	if (ddi->port_a != ena_clk) {
		unsigned int ctrl0;

		ddi->port_a = ena_clk;

		/*
		 * Get current configuration of CTRL0
		 */
		ctrl0 = readl(ddi->reg_base+DCON_CTRL0);

		/* enable or disable LCD clk. */
		if (0 == ddi->port_a)
			ctrl0 |= (0x1 << 24);
		else
			ctrl0 &= ~(0x1 << 24);

		/* Apply setting. */
		writel(ctrl0, ddi->reg_base+DCON_CTRL0);
	}

	rc = count;

	return rc;
}

static ssize_t dcon_show_pb_clk(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dovedcon_info *ddi;

	ddi = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", ddi->port_b);
}

static ssize_t dcon_ena_pb_clk(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	struct dovedcon_info *ddi;
	unsigned long ena_clk;

	ddi = dev_get_drvdata(dev);
	rc = strict_strtoul(buf, 0, &ena_clk);
	if (rc)
		return rc;

	if (ddi->port_b != ena_clk) {
		unsigned int ctrl0;

		ddi->port_b = ena_clk;

		/*
		 * Get current configuration of CTRL0
		 */
		ctrl0 = readl(ddi->reg_base+DCON_CTRL0);

		/* enable or disable LCD clk. */
		if (0 == ddi->port_b)
			ctrl0 |= (0x1 << 25);
		else
			ctrl0 &= ~(0x1 << 25);

		/* Apply setting. */
		writel(ctrl0, ddi->reg_base+DCON_CTRL0);
	}

	rc = count;

	return rc;
}

static ssize_t dcon_show_pa_mode(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dovedcon_info *ddi;
	unsigned int ctrl0;

	ddi = dev_get_drvdata(dev);

	/*
	 * Get current configuration of CTRL0
	 */
	ctrl0 = readl(ddi->reg_base+DCON_CTRL0);

	return sprintf(buf, "%d\n", (ctrl0 & (0x3 << 6)) >> 6 );
}

static ssize_t dcon_cfg_pa_mode(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	struct dovedcon_info *ddi;
	unsigned long value;
	unsigned int ctrl0;

	ddi = dev_get_drvdata(dev);
	rc = strict_strtoul(buf, 0, &value);
	if (rc)
		return rc;

	rc = -ENXIO;

	/*
	 * Get current configuration of CTRL0
	 */
	ctrl0 = readl(ddi->reg_base+DCON_CTRL0);

	if (value <= 3) {
		ctrl0 &= ~(0x3 << 6);
		ctrl0 |= (value << 6);

		/* Apply setting. */
		writel(ctrl0, ddi->reg_base+DCON_CTRL0);
		rc = count;
	}

	return rc;
}

static ssize_t dcon_show_pb_mode(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dovedcon_info *ddi;
	unsigned int ctrl0;

	ddi = dev_get_drvdata(dev);

	/*
	 * Get current configuration of CTRL0
	 */
	ctrl0 = readl(ddi->reg_base+DCON_CTRL0);

	return sprintf(buf, "%d\n", (ctrl0 & (0x3 << 8)) >> 8 );
}

static ssize_t dcon_cfg_pb_mode(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	struct dovedcon_info *ddi;
	unsigned long value;
	unsigned int ctrl0;

	ddi = dev_get_drvdata(dev);
	rc = strict_strtoul(buf, 0, &value);
	if (rc)
		return rc;

	rc = -ENXIO;

	/*
	 * Get current configuration of CTRL0
	 */
	ctrl0 = readl(ddi->reg_base+DCON_CTRL0);

	if (value <= 3) {
		ctrl0 &= ~(0x3 << 8);
		ctrl0 |= (value << 8);

		/* Apply setting. */
		writel(ctrl0, ddi->reg_base+DCON_CTRL0);
		rc = count;
	}

	return rc;
}

static ssize_t dcon_show_parallel_ena(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dovedcon_info *ddi;
	unsigned int ctrl0;

	ddi = dev_get_drvdata(dev);

	/*
	 * Get current configuration of CTRL0
	 */
	ctrl0 = readl(ddi->reg_base+DCON_CTRL0);

	return sprintf(buf, "%d\n", (ctrl0 & (0x1 << 17)) ? 0x0:0x1 );
}

static ssize_t dcon_ena_parallel(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	struct dovedcon_info *ddi;
	unsigned long value;
	unsigned int ctrl0;

	ddi = dev_get_drvdata(dev);
	rc = strict_strtoul(buf, 0, &value);
	if (rc)
		return rc;

	rc = -ENXIO;

	/*
	 * Get current configuration of CTRL0
	 */
	ctrl0 = readl(ddi->reg_base+DCON_CTRL0);

	if (value <= 1) {
		if (0 == value)
			ctrl0 |= (0x1 << 17);
		else
			ctrl0 &= ~(0x1 << 17);

		/* Apply setting. */
		writel(ctrl0, ddi->reg_base+DCON_CTRL0);
		rc = count;
	}

	return rc;
}

static ssize_t dcon_show_bit(struct device *dev,
		struct device_attribute *attr, char *buf,
		int offset, int len, unsigned int mask)
{
	struct dovedcon_info *ddi;
	unsigned int channel;
	unsigned int result;

	result = 0;
	ddi = dev_get_drvdata(dev);

	/*
	 * Get channel A over voltage detect
	 */
	channel = readl(ddi->reg_base+DCON_VGA_DAC_CHANNEL_A_CTRL);
	result |= (channel & (mask << offset)) >> (offset);
	/*
	 * Get channel B voltage calibration range
	 */
	channel = readl(ddi->reg_base+DCON_VGA_DAC_CHANNEL_B_CTRL);
	result |= ((channel & (mask << offset)) >> (offset)) << len;

	/*
	 * Get channel C voltage calibration range
	 */
	channel = readl(ddi->reg_base+DCON_VGA_DAC_CHANNEL_C_CTRL);
	result |= ((channel & (mask << offset)) >> (offset)) << (len*2);

	return sprintf(buf, "0x%08x\n", result);
}

static ssize_t dcon_cfg_bit(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count,
		int offset, int len, unsigned int mask, unsigned int *field)
{
	int rc;
	struct dovedcon_info *ddi;
	unsigned long value;
	unsigned int channel;
	unsigned int channel_ctrl;
	unsigned int channel_status;
	int i;
	void *addr;

	ddi = dev_get_drvdata(dev);
	rc = strict_strtoul(buf, 0, &value);
	if (rc)
		return rc;

	rc = -ENXIO;

	addr = ddi->reg_base+DCON_VGA_DAC_CHANNEL_A_CTRL;
	for (i = 0; i < 3; i++) {
		channel_ctrl = readl(addr+(i*4));
		channel_status = readl(addr+(i*4)+0xc);
	}

	/* Turn off VGA channel first. */
	channel = 0x1 << 22;
	addr = ddi->reg_base+DCON_VGA_DAC_CHANNEL_A_CTRL;
	for (i = 0; i < 3; i++)
		writel(channel, addr+(i*4));

	addr = ddi->reg_base+DCON_VGA_DAC_CHANNEL_A_CTRL;
	for (i = 0; i < 3; i++) {
		channel_ctrl = readl(addr+(i*4));
		channel_status = readl(addr+(i*4)+0xc);
	}


	/*
	 * bit[0] for channel A over voltage detect.
	 * bit[1] for channel B over voltage detect.
	 * bit[2] for channel C over voltage detect.
	 */
	channel = 0;
	addr = ddi->reg_base+DCON_VGA_DAC_CHANNEL_A_CTRL;
	for (i = 0; i < 3; i++) {
		/* config channel A, B, C. */
		field[i] = ((value & (mask << (i*len))) >> (i*len));
		channel =	((ddi->bias_adj[i]	<< 19) |
				(ddi->vcal_ena[i]	<< 18) |
				(ddi->vcal_range[i]	<< 12) |
				(ddi->ovol_detect[i]	<< 16) |
				(ddi->impd_detect[i]	<< 11) |
				(ddi->use_37_5ohm[i]	<< 10) |
				(ddi->use_75ohm[i]	<<  9) |
				(ddi->gain[i]		<<  1)
				);

		/* Apply setting. */
		writel(channel, addr+(i*4));
	}

	rc = count;

	addr = ddi->reg_base+DCON_VGA_DAC_CHANNEL_A_CTRL;
	for (i = 0; i < 3; i++) {
		channel_ctrl = readl(addr+(i*4));
		channel_status = readl(addr+(i*4)+0xc);
	}


	return rc;
}

static ssize_t dcon_show_gain(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return dcon_show_bit(dev, attr, buf, 1, 8, 0xff);
}

static ssize_t dcon_cfg_gain(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct dovedcon_info *ddi;
	ddi = dev_get_drvdata(dev);

	return dcon_cfg_bit(dev, attr, buf, count, 1, 8, 0xff,
		ddi->gain);
}

static ssize_t dcon_show_use_75ohm(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return dcon_show_bit(dev, attr, buf, 9, 1, 0x1);
}

static ssize_t dcon_ena_use_75ohm(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct dovedcon_info *ddi;
	ddi = dev_get_drvdata(dev);

	return dcon_cfg_bit(dev, attr, buf, count, 9, 1, 0x1,
		ddi->use_75ohm);
}


static ssize_t dcon_show_use_37_5ohm(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return dcon_show_bit(dev, attr, buf, 10, 1, 0x1);
}

static ssize_t dcon_ena_use_37_5ohm(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct dovedcon_info *ddi;
	ddi = dev_get_drvdata(dev);

	return dcon_cfg_bit(dev, attr, buf, count, 10, 1, 0x1,
		ddi->use_37_5ohm);
}

static ssize_t dcon_show_impd_detect(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return dcon_show_bit(dev, attr, buf, 11, 1, 0x1);
}

static ssize_t dcon_ena_impd_detect(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct dovedcon_info *ddi;
	ddi = dev_get_drvdata(dev);

	return dcon_cfg_bit(dev, attr, buf, count, 11, 1, 0x1,
		ddi->impd_detect);
}


static ssize_t dcon_show_ovol_detect(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return dcon_show_bit(dev, attr, buf, 16, 1, 0x1);
}

static ssize_t dcon_ena_ovol_detect(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct dovedcon_info *ddi;
	ddi = dev_get_drvdata(dev);

	return dcon_cfg_bit(dev, attr, buf, count, 16, 1, 0x1,
		ddi->ovol_detect);
}


static ssize_t dcon_show_vcal_range(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return dcon_show_bit(dev, attr, buf, 12, 4, 0xf);
}

static ssize_t dcon_cfg_vcal_range(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct dovedcon_info *ddi;
	ddi = dev_get_drvdata(dev);

	return dcon_cfg_bit(dev, attr, buf, count, 12, 4, 0xf,
		ddi->vcal_range);
}

static ssize_t dcon_show_vcal_ena(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return dcon_show_bit(dev, attr, buf, 18, 1, 0x1);
}

static ssize_t dcon_ena_vcal(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct dovedcon_info *ddi;
	ddi = dev_get_drvdata(dev);

	return dcon_cfg_bit(dev, attr, buf, count, 18, 1, 0x1,
		ddi->vcal_ena);
}

static ssize_t dcon_show_bias_adj(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return dcon_show_bit(dev, attr, buf, 19, 1, 0x1);
}

static ssize_t dcon_ena_bias_adj(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct dovedcon_info *ddi;
	ddi = dev_get_drvdata(dev);

	return dcon_cfg_bit(dev, attr, buf, count, 19, 1, 0x1,
		ddi->bias_adj);
}

static ssize_t dcon_show_vga_pwr(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dovedcon_info *ddi;
	unsigned int channel;
	unsigned int result;

	result = 0;
	ddi = dev_get_drvdata(dev);

	/*
	 * Get channel A power status
	 */
	channel = readl(ddi->reg_base+DCON_VGA_DAC_CHANNEL_A_CTRL);
	result |= (channel & (0x1 << 22)) ? 0x0:0x1;
	
	/*
	 * Get channel B power status
	 */
	channel = readl(ddi->reg_base+DCON_VGA_DAC_CHANNEL_B_CTRL);
	result |= (channel & (0x1 << 22)) ? 0x0:0x2;
	
	/*
	 * Get channel C power status
	 */
	channel = readl(ddi->reg_base+DCON_VGA_DAC_CHANNEL_C_CTRL);
	result |= (channel & (0x1 << 22)) ? 0x0:0x4;

	return sprintf(buf, "%d\n", result);
}

static ssize_t dcon_ena_vga_pwr(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	struct dovedcon_info *ddi;
	unsigned long value;
	unsigned int channel;

	ddi = dev_get_drvdata(dev);
	rc = strict_strtoul(buf, 0, &value);
	if (rc)
		return rc;

	rc = -ENXIO;
	channel = 0;

	/*
	 * bit[0] for channel A power status.
	 * bit[1] for channel B power status.
	 * bit[2] for channel C power status.
	 */
	if (value <= 0x7) {
		/* config channel A */
		if (value & 0x1) {
			/* channel = VGA_CHANNEL_DEFAULT; */
			channel =	((ddi->bias_adj[0]	<< 19) |
					(ddi->vcal_ena[0]	<< 18) |
					(ddi->vcal_range[0]	<< 12) |
					(ddi->ovol_detect[0]	<< 16) |
					(ddi->impd_detect[0]	<< 11) |
					(ddi->use_37_5ohm[0]	<< 10) |
					(ddi->use_75ohm[0]	<<  9) |
					(ddi->gain[0]		<<  1)
					);
		} else
			channel |= (0x1 << 22);

		/* Apply setting. */
		writel(channel, ddi->reg_base+DCON_VGA_DAC_CHANNEL_A_CTRL);

		/* config channel B */
		if (value & 0x2) {
			/* channel = VGA_CHANNEL_DEFAULT; */
			channel =	((ddi->bias_adj[1]	<< 19) |
					(ddi->vcal_ena[1]	<< 18) |
					(ddi->vcal_range[1]	<< 12) |
					(ddi->ovol_detect[1]	<< 16) |
					(ddi->impd_detect[1]	<< 11) |
					(ddi->use_37_5ohm[1]	<< 10) |
					(ddi->use_75ohm[1]	<<  9) |
					(ddi->gain[1]		<<  1)
					);
		} else
			channel |= (0x1 << 22);

		/* Apply setting. */
		writel(channel, ddi->reg_base+DCON_VGA_DAC_CHANNEL_B_CTRL);

		/* config channel C */
		if (value & 0x4) {
			/* channel = VGA_CHANNEL_DEFAULT; */
			channel =	((ddi->bias_adj[0]	<< 19) |
					(ddi->vcal_ena[0]	<< 18) |
					(ddi->vcal_range[0]	<< 12) |
					(ddi->ovol_detect[0]	<< 16) |
					(ddi->impd_detect[0]	<< 11) |
					(ddi->use_37_5ohm[0]	<< 10) |
					(ddi->use_75ohm[0]	<<  9) |
					(ddi->gain[0]		<<  1)
					);
		} else
			channel |= (0x1 << 22);

		/* Apply setting. */
		writel(channel, ddi->reg_base+DCON_VGA_DAC_CHANNEL_C_CTRL);

		rc = count;
	}

	return rc;
}

/* Using sysfs to configure DCON function. */
static struct class *dcon_class;
#define DCON_ATTR_NUM 14
static struct device_attribute dcon_device_attributes[DCON_ATTR_NUM+1] = {
	__ATTR(pa_clk_ena, 0644, dcon_show_pa_clk, dcon_ena_pa_clk),
	__ATTR(pb_clk_ena, 0644, dcon_show_pb_clk, dcon_ena_pb_clk),
	__ATTR(pa_mode, 0644, dcon_show_pa_mode, dcon_cfg_pa_mode),
	__ATTR(pb_mode, 0644, dcon_show_pb_mode, dcon_cfg_pb_mode),
	__ATTR(parallel_ena, 0644, dcon_show_parallel_ena, dcon_ena_parallel),
	__ATTR(vga_ch_pwr, 0644, dcon_show_vga_pwr, dcon_ena_vga_pwr),
	__ATTR(bias_adj, 0644, dcon_show_bias_adj, dcon_ena_bias_adj),
	__ATTR(vcal_ena, 0644, dcon_show_vcal_ena, dcon_ena_vcal),
	__ATTR(vcal_range, 0644, dcon_show_vcal_range, dcon_cfg_vcal_range),
	__ATTR(ovol_detect, 0644, dcon_show_ovol_detect, dcon_ena_ovol_detect),
	__ATTR(impd_detect, 0644, dcon_show_impd_detect, dcon_ena_impd_detect),
	__ATTR(use_37_5ohm, 0644, dcon_show_use_37_5ohm, dcon_ena_use_37_5ohm),
	__ATTR(use_75ohm, 0644, dcon_show_use_75ohm, dcon_ena_use_75ohm),
	__ATTR(gain, 0644, dcon_show_gain, dcon_cfg_gain),
	__ATTR_NULL,
};

static int __init dcon_class_init(void)
{
	dcon_class = class_create(THIS_MODULE, "dcon");
	if (IS_ERR(dcon_class)) {
		printk(KERN_WARNING "Unable to create dcon class; errno = %ld\n",
				PTR_ERR(dcon_class));
		return PTR_ERR(dcon_class);
	}

	dcon_class->dev_attrs = dcon_device_attributes;
	dcon_class->suspend = NULL;
	dcon_class->resume = NULL;
	return 0;
}

static void dcon_device_release(struct device *dev)
{
	struct dovedcon_info *ddi;

	ddi = dev_get_drvdata(dev);
	kfree(ddi);
}

/* Initialization */
static int __init dovedcon_probe(struct platform_device *pdev)
{
	struct dovedcon_mach_info *ddmi;
	struct dovedcon_info *ddi;
	struct resource *res;
	int i;

	ddmi = pdev->dev.platform_data;
	if (!ddmi)
		return -EINVAL;

	ddi = kzalloc(sizeof(struct dovedcon_info), GFP_KERNEL);
	if (ddi == NULL)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		kfree(ddi);
		return -EINVAL;
	}

	if (!request_mem_region(res->start, res->end - res->start,
	    "MRVL DCON Regs")) {
		printk(KERN_INFO "Cannot reserve DCON memory mapped"
			" area 0x%lx @ 0x%lx\n",
			(unsigned long)res->start,
			(unsigned long)res->end - res->start);
		kfree(ddi);
		return -ENXIO;
	}

	ddi->reg_base = ioremap_nocache(res->start, res->end - res->start);

	if (!ddi->reg_base) {
		kfree(ddi);
		return -EINVAL;
	}

	platform_set_drvdata(pdev, ddi);

	/* init 2 clk for port A and B. */
	for (i = 0; i < 3; i++) {
		if (ddmi->clk_name[0]) {
			ddi->clk[0] = clk_get(&pdev->dev, ddmi->clk_name[0]);
		} else {
			printk(KERN_WARNING "DCON: Can't get clk for port"
				" %s.\n", i ? "B" : "A");
			ddi->clk[0] = clk_get(&pdev->dev, "LCDCLK");
		}

		if (!IS_ERR(ddi->clk[0]))
			clk_enable(ddi->clk[0]);
	}

	ddi->port_a = ddmi->port_a;
	ddi->port_b = ddmi->port_b;

	for (i = 0; i < 3; i++) {
		ddi->bias_adj[i]
		    =	((VGA_CHANNEL_DEFAULT & (0x1 << 19)) >> 19);
		ddi->vcal_ena[i]
		    =	((VGA_CHANNEL_DEFAULT & (0x1 << 18)) >> 18);
		ddi->vcal_range[i]
		    =	((VGA_CHANNEL_DEFAULT & (0xf << 12)) >> 12);
		ddi->ovol_detect[i]
		    =	((VGA_CHANNEL_DEFAULT & (0x1 << 16)) >> 16);
		ddi->impd_detect[i]
		    =	((VGA_CHANNEL_DEFAULT & (0x1 << 11)) >> 11);
		ddi->use_37_5ohm[i]
		    =	((VGA_CHANNEL_DEFAULT & (0x1 << 10)) >> 10);
		ddi->use_75ohm[i]
		    =	((VGA_CHANNEL_DEFAULT & (0x1 <<  9)) >>  9);
		ddi->gain[i]
		    =	((VGA_CHANNEL_DEFAULT & (0xff << 1)) >>  1);
	}

	/* Initialize DCON hardware */
	dovedcon_enable(ddi);

	/*
	 * Register to receive fb blank event.
	 */
	dovedcon_hook_fb_event(ddi);

	dcon_class_init();
	pdev->dev.class = dcon_class;
	pdev->dev.release = dcon_device_release;

	{
		int i;

		for(i = 0; i < DCON_ATTR_NUM; i++)
			if (device_create_file(&pdev->dev,
			    &dcon_device_attributes[i]))
				printk(KERN_ERR
				    "dcon register <%d> sysfs failed\n", i);
	}

	printk(KERN_INFO "dovedcon has been initialized.\n");

	return 0;
}

/*
 *  Cleanup
 */
static int dovedcon_remove(struct platform_device *pdev)
{
	struct dovedcon_info *ddi = platform_get_drvdata(pdev);

	clk_disable(ddi->clk[0]);
	clk_disable(ddi->clk[1]);
	clk_put(ddi->clk[0]);
	clk_put(ddi->clk[1]);
	iounmap(ddi->reg_base);
	kfree(ddi);

	return 0;
}

static struct platform_driver dovedcon_pdriver = {
	.probe		= dovedcon_probe,
	.remove		= dovedcon_remove,
#ifdef CONFIG_PM
	.suspend	= dovedcon_suspend,
	.resume		= dovedcon_resume,
#endif /* CONFIG_PM */
	.driver		=	{
		.name	=	"dovedcon",
		.owner	=	THIS_MODULE,
	},
};
static int __init dovedcon_init(void)
{
	return platform_driver_register(&dovedcon_pdriver);
}

static void __exit dovedcon_exit(void)
{
	platform_driver_unregister(&dovedcon_pdriver);
}

module_init(dovedcon_init);
module_exit(dovedcon_exit);

MODULE_AUTHOR("Green Wan <gwan@marvell.com>");
MODULE_DESCRIPTION("DCON driver for Dove LCD unit");
MODULE_LICENSE("GPL");
