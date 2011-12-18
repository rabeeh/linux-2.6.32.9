/*
 * arch/arm/mach-dove/dove-front-panel-common.c
 *
 * Marvell Dove MV88F6781-RD/DB Board Setup for device on front panel
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/gpio_mouse.h>
#include <linux/spi/spi.h>
#include <linux/spi/ads7846.h>
#include <linux/gpio.h>
#include <video/dovefb.h>
#include <video/dovefbreg.h>
#include <mach/dove_bl.h>
#include <plat/cafe-orion.h>
#include "dove-front-panel-common.h"

static struct dovefb_mach_info dove_lcd0_dmi = {
	.id_gfx			= "GFX Layer 0",
	.id_ovly		= "Video Layer 0",
	.clk_src		= MRVL_PLL_CLK,
	.clk_name		= "accurate_LCDCLK",
//	.num_modes		= ARRAY_SIZE(video_modes),
//	.modes			= video_modes,
	.pix_fmt		= PIX_FMT_RGB888PACK,
#if defined(CONFIG_FB_DOVE_CLCD_DCONB_BYPASS0)
	.io_pin_allocation	= IOPAD_DUMB24,
	.panel_rgb_type		= DUMB24_RGB888_0,
#else
	.io_pin_allocation	= IOPAD_DUMB18SPI,
	.panel_rgb_type		= DUMB18_RGB666_0,
#endif
	.panel_rgb_reverse_lanes= 0,
	.gpio_output_data	= 0,
	.gpio_output_mask	= 0,
	.ddc_i2c_adapter	= 0,
	.invert_composite_blank	= 0,
	.invert_pix_val_ena	= 0,
	.invert_pixclock	= 0,
	.invert_vsync		= 0,
	.invert_hsync		= 0,
	.panel_rbswap		= 1,
	.active			= 1,
};

static struct dovefb_mach_info dove_lcd0_vid_dmi = {
	.id_ovly		= "Video Layer 0",
//	.num_modes		= ARRAY_SIZE(video_modes),
//	.modes			= video_modes,
	.pix_fmt		= PIX_FMT_RGB888PACK,
	.io_pin_allocation	= IOPAD_DUMB18SPI,
	.panel_rgb_type		= DUMB18_RGB666_0,
	.panel_rgb_reverse_lanes= 0,
	.gpio_output_data	= 0,
	.gpio_output_mask	= 0,
	.ddc_i2c_adapter	= -1,
	.invert_composite_blank	= 0,
	.invert_pix_val_ena	= 0,
	.invert_pixclock	= 0,
	.invert_vsync		= 0,
	.invert_hsync		= 0,
	.panel_rbswap		= 1,
	.active			= 0,
	.enable_lcd0		= 0,
};

static struct dovefb_mach_info dove_lcd1_dmi = {
	.id_gfx			= "GFX Layer 1",
	.id_ovly		= "Video Layer 1",
	.clk_src		= MRVL_EXT_CLK1,
	.clk_name		= "IDT_CLK1",
//	.num_modes		= ARRAY_SIZE(video_modes),
//	.modes			= video_modes,
	.pix_fmt		= PIX_FMT_RGB565,
	.io_pin_allocation	= IOPAD_DUMB24,
	.panel_rgb_type		= DUMB24_RGB888_0,
	.panel_rgb_reverse_lanes= 0,
	.gpio_output_data	= 0,
	.gpio_output_mask	= 0,
	.ddc_i2c_adapter	= 0,
	.invert_composite_blank	= 0,
	.invert_pix_val_ena	= 0,
	.invert_pixclock	= 0,
	.invert_vsync		= 0,
	.invert_hsync		= 0,
	.panel_rbswap		= 1,
	.active			= 1,
#ifndef CONFIG_FB_DOVE_CLCD
	.enable_lcd0		= 1,
#else
	.enable_lcd0		= 0,
#endif
};

static struct dovefb_mach_info dove_lcd1_vid_dmi = {
	.id_ovly		= "Video Layer 1",
//	.num_modes		= ARRAY_SIZE(video_modes),
//	.modes			= video_modes,
	.pix_fmt		= PIX_FMT_RGB888PACK,
	.io_pin_allocation	= IOPAD_DUMB24,
	.panel_rgb_type		= DUMB24_RGB888_0,
	.panel_rgb_reverse_lanes= 0,
	.gpio_output_data	= 0,
	.gpio_output_mask	= 0,
	.ddc_i2c_adapter	= -1,
	.invert_composite_blank	= 0,
	.invert_pix_val_ena	= 0,
	.invert_pixclock	= 0,
	.invert_vsync		= 0,
	.invert_hsync		= 0,
	.panel_rbswap		= 1,
	.active			= 0,
};

static struct dovebl_platform_data fp_backlight_data = {
	.default_intensity = 0xa,
	.gpio_pm_control = 1,

	.lcd_start = DOVE_LCD1_PHYS_BASE,	/* lcd power control reg base. */
	.lcd_end = DOVE_LCD1_PHYS_BASE+0x1C8,	/* end of reg map. */
	.lcd_offset = LCD_SPU_DUMB_CTRL,	/* register offset */
	.lcd_mapped = 0,		/* va = 0, pa = 1 */
	.lcd_mask = 0x200000,		/* mask, bit[21] */
	.lcd_on = 0x200000,			/* value to enable lcd power */
	.lcd_off = 0x0,			/* value to disable lcd power */

	.blpwr_start = DOVE_LCD1_PHYS_BASE, /* bl pwr ctrl reg base. */
	.blpwr_end = DOVE_LCD1_PHYS_BASE+0x1C8,	/* end of reg map. */
	.blpwr_offset = LCD_SPU_DUMB_CTRL,	/* register offset */
	.blpwr_mapped = 0,		/* pa = 0, va = 1 */
	.blpwr_mask = 0x100000,		/* mask */
	.blpwr_on = 0x100000,		/* value to enable bl power */
	.blpwr_off = 0x0,		/* value to disable bl power */

	.btn_start = DOVE_LCD1_PHYS_BASE, /* brightness control reg base. */
	.btn_end = DOVE_LCD1_PHYS_BASE+0x1C8,	/* end of reg map. */
	.btn_offset = LCD_CFG_GRA_PITCH,	/* register offset */
	.btn_mapped = 0,		/* pa = 0, va = 1 */
	.btn_mask = 0xF0000000,	/* mask */
	.btn_level = 15,	/* how many level can be configured. */
	.btn_min = 0x1,	/* min value */
	.btn_max = 0xF,	/* max value */
	.btn_inc = 0x1,	/* increment */

};

struct cafe_cam_platform_data dove_cafe_cam_data = {
	.power_down 	= 1, //CTL0 connected to the sensor power down
	.reset		= 2, //CTL1 connected to the sensor reset
};

struct gpio_mouse_platform_data tact_dove_fp_data = {
	.scan_ms = 10,
	.polarity = 1,
	{
		{
			.up = 15,
			.down = 12,
			.left = 13,
			.right = 14,
			.bleft = 18,
			.bright = -1,
			.bmiddle = -1
		}
	}
};

#define DOVE_FP_TS_PEN_GPIO	(53)
#define DOVE_FP_TS_PEN_IRQ	(DOVE_FP_TS_PEN_GPIO + 64)

static int dove_fp_get_pendown_state(void)
{
	return !gpio_get_value(irq_to_gpio(DOVE_FP_TS_PEN_IRQ));
}

static const struct ads7846_platform_data dove_fp_ts_data = {
	.model			= 7846,
	.x_min			= 80,
	.x_max			= 3940,
	.y_min			= 330,
	.y_max			= 3849,
	.get_pendown_state	= dove_fp_get_pendown_state,
};

struct spi_board_info __initdata dove_fp_spi_devs[] = {
	{
		.modalias	= "ads7846",
		.platform_data	= &dove_fp_ts_data,
		.irq		= DOVE_FP_TS_PEN_IRQ,
		.max_speed_hz	= 125000 * 4,
		.bus_num	= 2,
		.chip_select	= 0,
	},
};

int __init dove_fp_spi_devs_num(void)
{
	return ARRAY_SIZE(dove_fp_spi_devs);
}

int __init dove_fp_ts_gpio_setup(void)
{
	set_irq_chip(DOVE_FP_TS_PEN_IRQ, &orion_gpio_irq_chip);
	set_irq_handler(DOVE_FP_TS_PEN_IRQ, handle_edge_irq);
	set_irq_type(DOVE_FP_TS_PEN_IRQ, IRQ_TYPE_EDGE_FALLING);
	orion_gpio_set_valid(DOVE_FP_TS_PEN_GPIO, 1);
	if (gpio_request(DOVE_FP_TS_PEN_GPIO, "DOVE_TS_PEN_IRQ") != 0)
		pr_err("Dove: failed to setup TS IRQ GPIO\n");
	if (gpio_direction_input(DOVE_FP_TS_PEN_GPIO) != 0) {
		printk(KERN_ERR "%s failed "
		       "to set output pin %d\n", __func__,
		       DOVE_FP_TS_PEN_GPIO);
		gpio_free(DOVE_FP_TS_PEN_GPIO);
		return -1;
	}
	return 0;
}

void __init dove_fp_clcd_init(void) {
#ifdef CONFIG_FB_DOVE
	clcd_platform_init(&dove_lcd0_dmi, &dove_lcd0_vid_dmi,
			   &dove_lcd1_dmi, &dove_lcd1_vid_dmi,
			   &fp_backlight_data);
#endif /* CONFIG_FB_DOVE */
}
