/*
 * arch/arm/mach-dove/dove-db-setup.c
 *
 * Marvell DB-MV88F6781-BP Development Board Setup
 *
 * Author: Tzachi Perelstein <tzachi@marvell.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/mtd/physmap.h>
#include <linux/mtd/nand.h>
#include <linux/timer.h>
#include <linux/ata_platform.h>
#include <linux/mv643xx_eth.h>
#include <linux/i2c.h>
#include <linux/i2c/pca953x.h>
#include <linux/pci.h>
#include <linux/gpio_mouse.h>
#include <linux/gpio_keys.h>
#include <linux/spi/spi.h>
#include <linux/spi/orion_spi.h>
#include <linux/spi/flash.h>
#include <linux/spi/tsc200x.h>
#include <video/dovefb.h>
#include <video/dovefbreg.h>
#include <mach/dove_bl.h>
#include <plat/i2s-orion.h>
#include <asm/mach-types.h>
#include <asm/gpio.h>
#include <asm/mach/arch.h>
#include <mach/dove.h>
#include <asm/hardware/pxa-dma.h>
#include <plat/orion_nfc.h>
#include <mach/dove_ssp.h>
#include <mach/pm.h>
#include <plat/cafe-orion.h>
#include "common.h"
#include "clock.h"
#include "idt5v49ee503.h"
#include "mpp.h"
#include "pmu/mvPmu.h"
#include "pmu/mvPmuRegs.h"
#include "pdma/mvPdma.h"
#include <linux/adi9889_i2c.h>

#define DOVE_DB_WAKEUP_GPIO	(3)
#define DOVE_DB_POWER_OFF_GPIO	(8)

static unsigned int front_panel = 0;
module_param(front_panel, uint, 0);
MODULE_PARM_DESC(front_panel, "set to 1 if the dove DB front panel connected");

static unsigned int lcd2dvi = 0;
module_param(lcd2dvi, uint, 0);
MODULE_PARM_DESC(lcd2dvi, "set to 1 if the LCD2DVI connected");

static unsigned int left_tact = 0;
module_param(left_tact, uint, 0);
MODULE_PARM_DESC(left_tact, "Use left tact as mouse");

static unsigned int right_tact = 0;
module_param(right_tact, uint, 0);
MODULE_PARM_DESC(right_tact, "Use Right tact as keys");

static unsigned int use_hal_giga = 0;
#ifdef CONFIG_MV643XX_ETH
module_param(use_hal_giga, uint, 0);
MODULE_PARM_DESC(use_hal_giga, "Use the HAL giga driver");
#endif

static unsigned int standby_fix = 1;
module_param(standby_fix, uint, 0);
MODULE_PARM_DESC(standby_fix, "if 1 then CKE and MRESET are connected to MPP4 and MPP6");

static int dvs_enable = 1;
module_param(dvs_enable, int, 1);
MODULE_PARM_DESC(dvs_enable, "if 1 then enable DVS");

extern unsigned int useHalDrivers;
extern char *useNandHal;
/*
 * set lcd clock source in dovefb_mach_info, notice the sequence is not
 * same to bootargs.
 * = MRVL_AXI_CLK, choose AXI, 333Mhz.
         name is "AXICLK"
 * = MRVL_PLL_CLK, choose PLL, auto use accurate mode if only one LCD refer to it.
         name is "LCDCLK" or "accurate_LCDCLK"
 * = MRVL_EXT_CLK0, choose external clk#0, (available REV A0)
         name is "IDT_CLK0"
 * = MRVL_EXT_CLK1, choose external clk#1, (available REV A0)
         name is "IDT_CLK1"
 */
/*
 * LCD HW output Red[0] to LDD[0] when set bit [19:16] of reg 0x190
 * to 0x0. Which means HW outputs BGR format default. All platforms
 * uses this controller should enable .panel_rbswap. Unless layout
 * design connects Blue[0] to LDD[0] instead.
 */
static struct dovefb_mach_info dove_db_lcd0_dmi = {
	.id_gfx			= "GFX Layer 0",
	.id_ovly		= "Video Layer 0",
	.clk_src		= MRVL_EXT_CLK1,
	.clk_name		= "IDT_CLK1",
//	.num_modes		= ARRAY_SIZE(video_modes),
//	.modes			= video_modes,
	.pix_fmt		= PIX_FMT_RGB888PACK,
	.io_pin_allocation	= IOPAD_DUMB24,
	.panel_rgb_type		= DUMB24_RGB888_0,
	.panel_rgb_reverse_lanes= 0,
	.gpio_output_data	= 0,
	.gpio_output_mask	= 0,
#ifndef CONFIG_FB_DOVE_CLCD0_I2C_DEFAULT_SETTING
	.ddc_i2c_adapter	= CONFIG_FB_DOVE_CLCD0_I2C_CHANNEL,
	.ddc_i2c_address	= CONFIG_FB_DOVE_CLCD0_I2C_ADDRESS,
#else
	.ddc_i2c_adapter	= 0,
	.ddc_i2c_address        = 0x3f,
#endif
	.invert_composite_blank	= 0,
	.invert_pix_val_ena	= 0,
	.invert_pixclock	= 0,
	.invert_vsync		= 0,
	.invert_hsync		= 0,
	.panel_rbswap		= 1,
	.active			= 1,
};

static struct dovefb_mach_info dove_db_lcd0_vid_dmi = {
	.id_ovly		= "Video Layer 0",
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
	.enable_lcd0		= 0,
};

static struct dovefb_mach_info dove_db_fp_lcd0_dmi = {
	.id_gfx			= "GFX Layer 0",
	.id_ovly		= "Video Layer 0",
	.clk_src		= MRVL_EXT_CLK1,
	.clk_name		= "IDT_CLK1",
//	.num_modes		= ARRAY_SIZE(video_modes),
//	.modes			= video_modes,
	.pix_fmt		= PIX_FMT_RGB888PACK,
#if defined(CONFIG_FB_DOVE_CLCD_DCONB_BYPASS0)
	.io_pin_allocation	= IOPAD_DUMB24,
	.panel_rgb_type		= DUMB24_RGB888_0,
#else
	.io_pin_allocation	= IOPAD_DUMB18GPIO,
	.panel_rgb_type		= DUMB18_RGB666_0,
#endif
	.panel_rgb_reverse_lanes= 0,
	.gpio_output_data	= 3,
	.gpio_output_mask	= 3,
	.ddc_polling_disable	= 1,
#ifndef CONFIG_FB_DOVE_CLCD0_I2C_DEFAULT_SETTING
        .ddc_i2c_adapter        = CONFIG_FB_DOVE_CLCD0_I2C_CHANNEL,
        .ddc_i2c_address        = CONFIG_FB_DOVE_CLCD0_I2C_ADDRESS,
#else
	.ddc_i2c_adapter	= 3,
	.ddc_i2c_address	= 0x50,
#endif
	.invert_composite_blank	= 0,
	.invert_pix_val_ena	= 0,
	.invert_pixclock	= 0,
	.invert_vsync		= 0,
	.invert_hsync		= 0,
	.panel_rbswap		= 1,
	.active			= 1,
};

static struct dovefb_mach_info dove_db_fp_lcd0_vid_dmi = {
	.id_ovly		= "Video Layer 0",
//	.num_modes		= ARRAY_SIZE(video_modes),
//	.modes			= video_modes,
	.pix_fmt		= PIX_FMT_RGB888PACK,
	.io_pin_allocation	= IOPAD_DUMB18GPIO,
	.panel_rgb_type		= DUMB18_RGB666_0,
	.panel_rgb_reverse_lanes= 0,
	.gpio_output_data	= 3,
	.gpio_output_mask	= 3,
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

static struct dovefb_mach_info dove_db_lcd1_dmi = {
	.id_gfx			= "GFX Layer 1",
	.id_ovly		= "Video Layer 1",
	.clk_src		= MRVL_PLL_CLK,
	.clk_name		= "accurate_LCDCLK",
//	.num_modes		= ARRAY_SIZE(video_modes),
//	.modes			= video_modes,
	.pix_fmt		= PIX_FMT_RGB565,
	.io_pin_allocation	= IOPAD_DUMB24,
	.panel_rgb_type		= DUMB24_RGB888_0,
	.panel_rgb_reverse_lanes= 0,
	.gpio_output_data	= 0,
	.gpio_output_mask	= 0,
#ifndef CONFIG_FB_DOVE_CLCD1_I2C_DEFAULT_SETTING
        .ddc_i2c_adapter        = CONFIG_FB_DOVE_CLCD1_I2C_CHANNEL,
        .ddc_i2c_address        = CONFIG_FB_DOVE_CLCD1_I2C_ADDRESS,
#else
        .ddc_i2c_adapter        = 1,
        .ddc_i2c_address        = 0x50,
#endif
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

static struct dovefb_mach_info dove_db_lcd1_vid_dmi = {
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



static struct dove_ssp_platform_data dove_ssp_platform_data = {
	.use_dma = 0,
	.use_loopback = 1,
	.dss = 32,
	.scr = 2,
	.frf = 1,
	.rft = 8,
	.tft = 8
};


/*****************************************************************************
 * BACKLIGHT
 ****************************************************************************/
static struct dovebl_platform_data dove_db_backlight_data = {
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

#ifdef CONFIG_ADI9889
extern int ad9889_int_status;
#endif

static int db_lcd0_mon_sense(int *connect_status)
{
	/* suppose AD9889 chip is enabled.
	 * We rely on it to do the detect
	 */
#ifdef CONFIG_ADI9889
	if (lcd2dvi) {
		if (ad9889_int_status & 0x2000) {
			*connect_status = 1;
			return 0;
		} else if (0 == ad9889_int_status) {
			*connect_status = 0;
			return 0;
		}
	}
#endif
	/* read HW detection info. */
	*connect_status = 0;
	return 0;
}

#if 0 /* VGA Port have no hw detection for now. */
static int db_lcd1_mon_sense(int *connect_status)
{
	/* read HW detection info. */
	*connect_status = 0;
	return 0;
}
#endif

void __init dove_db_clcd_init(void) {
#ifdef CONFIG_FB_DOVE
	struct dovefb_mach_info *lcd0_dmi, *lcd0_vid_dmi;
	u32 dev, rev;
	
	dove_pcie_id(&dev, &rev);

	if (front_panel) {
		lcd0_dmi = &dove_db_fp_lcd0_dmi;
		lcd0_vid_dmi = &dove_db_fp_lcd0_vid_dmi;
	} else {
		if (lcd2dvi) {
			dove_db_lcd0_dmi.mon_sense
			    = db_lcd0_mon_sense;
		}

		lcd0_dmi = &dove_db_lcd0_dmi;
		lcd0_vid_dmi = &dove_db_lcd0_vid_dmi;
	}

	clcd_platform_init(lcd0_dmi, lcd0_vid_dmi,
			   &dove_db_lcd1_dmi, &dove_db_lcd1_vid_dmi,
			   &dove_db_backlight_data);

#endif /* CONFIG_FB_DOVE */
}

#define DOVE_DB_PHY_RST_GPIO	(1)
#define DOVE_DB_TS_PEN_GPIO	(63)
#define DOVE_DB_TS_PEN_IRQ	(64 + DOVE_DB_TS_PEN_GPIO)

static struct tsc2005_platform_data ts_info = {
	.model			= 2005,
	.x_plate_ohms		= 450,
	.y_plate_ohms		= 250,
};

struct spi_board_info __initdata dove_db_spi_ts_dev[] = {
	{
		.modalias  		= "tsc2005",
		.irq			= DOVE_DB_TS_PEN_IRQ,
		.max_speed_hz		= 10000000, //10MHz
		.bus_num		= 1,
		.chip_select		= 0,
		.mode		        = SPI_MODE_0,      
		.platform_data          = &ts_info,
	},
};

int __init dove_db_ts_gpio_setup(void)
{
	orion_gpio_set_valid(DOVE_DB_TS_PEN_GPIO, 1);
	if (gpio_request(DOVE_DB_TS_PEN_GPIO, "DOVE_TS_PEN_IRQ") != 0)
		pr_err("Dove: failed to setup TS IRQ GPIO\n");
	if (gpio_direction_input(DOVE_DB_TS_PEN_GPIO) != 0) {
		printk(KERN_ERR "%s failed "
		       "to set output pin %d\n", __func__,
		       DOVE_DB_TS_PEN_GPIO);
		gpio_free(DOVE_DB_TS_PEN_GPIO);
		return -1;
	}
	//IRQ
	set_irq_chip(DOVE_DB_TS_PEN_IRQ, &orion_gpio_irq_chip);
	set_irq_handler(DOVE_DB_TS_PEN_IRQ, handle_level_irq);
	set_irq_type(DOVE_DB_TS_PEN_IRQ, IRQ_TYPE_LEVEL_LOW);

	return 0;
}

int __init dove_db_b_giga_phy_gpio_setup(void)
{
	orion_gpio_set_valid(DOVE_DB_PHY_RST_GPIO, 1);
	if (gpio_request(DOVE_DB_PHY_RST_GPIO, "Giga Phy reset gpio") != 0)
		pr_err("Dove: failed to setup Giga phy reset GPIO\n");
	if (gpio_direction_output(DOVE_DB_PHY_RST_GPIO, 1) != 0) {
		printk(KERN_ERR "%s failed to set output pin %d\n", __func__,
		       DOVE_DB_PHY_RST_GPIO);
		gpio_free(DOVE_DB_PHY_RST_GPIO);
		return -1;
	}

	return 0;
}

static struct cafe_cam_platform_data dove_cafe_cam_data = {
	.power_down 	= 1, //CTL0 connected to the sensor power down
	.reset		= 2, //CTL1 connected to the sensor reset
	.numbered_i2c_bus = 1,
	.i2c_bus_id	= 3,
};

extern int __init pxa_init_dma_wins(struct mbus_dram_target_info * dram);

static struct orion_i2s_platform_data i2s1_data = {
	.i2s_play	= 1,
	.i2s_rec	= 1,
	.spdif_play	= 1,
};

#if !defined(CONFIG_SND_DOVE_AC97)
static struct orion_i2s_platform_data i2s0_data = {
	.i2s_play	= 1,
	.i2s_rec	= 1,
};
#endif

static struct mv643xx_eth_platform_data dove_db_ge00_data = {
	.phy_addr	= MV643XX_ETH_PHY_ADDR_DEFAULT,
};

static struct mv643xx_eth_platform_data dove_db_b_ge00_data = {
	.phy_addr	= 1,
};

static struct mv_sata_platform_data dove_db_sata_data = {
        .n_ports        = 1,
};

static struct gpio_mouse_platform_data ltact_dove_data = {
        .scan_ms = 10,
        .polarity = 1,
        {
                {
			.up = 67,
			.down = 64,
			.left = 65,
			.right = 66,
			.bleft = 68,
                        .bright = -1,
                        .bmiddle = -1
                } 
        },
	.use_activity_irq = 1,
	.activity_gpio = 57,
};

static struct gpio_keys_button gpio_buttons[] = {
	{
		.code			= KEY_KP2,
		.gpio			= 72,
		.active_low		= 1,
		.debounce_interval	= 10,
		.desc			= "down",
	},
	{
		.code			= KEY_KP4,
		.gpio			= 73,
		.active_low		= 1,
		.debounce_interval	= 10,
		.desc			= "left",
	},
	{
		.code			= KEY_KP6,
		.gpio			= 74,
		.active_low		= 1,
		.debounce_interval	= 10,
		.desc			= "right",
	},
	{
		.code			= KEY_KP8,
		.gpio			= 75,
		.active_low		= 1,
		.debounce_interval	= 10,
		.desc			= "up",
	},
	{
		.code			= KEY_ENTER,
		.gpio			= 76,
		.active_low		= 1,
		.debounce_interval	= 10,
		.desc			= "enter",
	},
};

static struct gpio_keys_platform_data gpio_key_info = {
	.buttons	= gpio_buttons,
	.nbuttons	= ARRAY_SIZE(gpio_buttons),
	.use_shared_irq = 1,
	.shared_irq = 57 + 64,
};

static struct platform_device keys_gpio = {
	.name	= "gpio-keys",
	.id	= 1,
	.dev	= {
		.platform_data	= &gpio_key_info,
	},
};

/*****************************************************************************
 * SPI Devices:
 * 	SPI0: 8M Flash ST-M25P64-VMF6P
 ****************************************************************************/
static const struct flash_platform_data dove_db_spi_flash_data = {
	.type		= "m25p64",
};

static struct spi_board_info __initdata dove_db_spi_flash_info[] = {
	{
		.modalias       = "m25p80",
		.platform_data  = &dove_db_spi_flash_data,
		.irq            = -1,
		.max_speed_hz   = 20000000,
		.bus_num        = 0,
		.chip_select    = 0,
	},
};

/*****************************************************************************
 * 7-Segment on GPIO 14,15,18,19
 * Dummy counter up to 7 every 1 sec
 ****************************************************************************/

static struct timer_list dove_db_timer;
static int dove_db_7seg_gpios[] = {14, 15, 62};

static void dove_db_7seg_event(unsigned long data)
{
     static int count = 0;

     gpio_set_value(dove_db_7seg_gpios[0], count & 1);
     gpio_set_value(dove_db_7seg_gpios[1], count & (1 << 1));
     gpio_set_value(dove_db_7seg_gpios[2], count & (1 << 2));

     count = (count + 1) & 7;
     mod_timer(&dove_db_timer, jiffies + 1 * HZ);
}

static int __init dove_db_7seg_init(void)
{
	if (machine_is_dove_db() || machine_is_dove_db_b()) {
		int i;
		
		for(i = 0; i < 3; i++){
			int pin = dove_db_7seg_gpios[i];

			if (gpio_request(pin, "7seg LED") == 0) {
				if (gpio_direction_output(pin, 0) != 0) {
					printk(KERN_ERR "%s failed "
					       "to set output pin %d\n", __func__,
					       pin);
					gpio_free(pin);
					return 0;
				}
			} else {
				printk(KERN_ERR "%s failed "
				       "to request gpio %d\n", __func__, pin);
				return 0;
			}
		}
		setup_timer(&dove_db_timer, dove_db_7seg_event, 0);
		mod_timer(&dove_db_timer, jiffies + 1 * HZ);
	}
	return 0;
}

__initcall(dove_db_7seg_init);

/*****************************************************************************
 * PCI
 ****************************************************************************/
static int __init dove_db_pci_init(void)
{
	if (machine_is_dove_db() || machine_is_dove_db_b())
		dove_pcie_init(1, 1);

	return 0;
}

subsys_initcall(dove_db_pci_init);

static int __init dove_db_cam_init(void)
{
	if ((machine_is_dove_db() || machine_is_dove_db_b())  &&  front_panel)
		dove_cam_init(&dove_cafe_cam_data);
	
	return 0;
}

late_initcall(dove_db_cam_init);

static int pca9555_setup(struct i2c_client *client,
			 unsigned gpio, unsigned ngpio,
			 void *context)
{
	if(front_panel) {
		if(left_tact)
			dove_tact_init(&ltact_dove_data);
		if(right_tact)
			platform_device_register(&keys_gpio);
	}
	return 0;
}

/* PCA9555 */
static struct pca953x_platform_data dove_db_gpio_ext_pdata = {
	.gpio_base = 64,
	.setup = pca9555_setup,
};

static struct i2c_board_info dove_db_gpio_ext_info[] = {
	[0] = {
		I2C_BOARD_INFO("pca9555", 0x27),
		.platform_data = &dove_db_gpio_ext_pdata,
	},
};

struct adi9889_i2c_platform_data db_adi9889_i2c_conf ={
	.audio_format = SPDIF_FORMAT,

};

/*****************************************************************************
 * A2D on I2C bus
 ****************************************************************************/
static struct i2c_board_info __initdata i2c_a2d = {
	I2C_BOARD_INFO("cs42l51", 0x4A),
};

/*****************************************************************************
 * IDT clock 
 ****************************************************************************/
static struct idt_data dove_db_idt_data = {
	/* clock 0 connected to pin LCD_EXT_REF_CLK[0]*/
	.clock0_enable = 1,
	.clock0_out_id = IDT_OUT_ID_2,
	.clock0_pll_id = IDT_PLL_1,
	/* clock 1 connected to pin LCD_EXT_REF_CLK[1]*/
	.clock1_enable = 1,
	.clock1_out_id = IDT_OUT_ID_3,
	.clock1_pll_id = IDT_PLL_2,
};

static struct i2c_board_info __initdata idt = {
	I2C_BOARD_INFO("idt5v49ee503", 0x6A),
	.platform_data = &dove_db_idt_data,
};

/*****************************************************************************
 * I2C buses - ADI9889 LCD to HDMI/DVI/VGA converter
 ****************************************************************************/
static struct i2c_board_info __initdata adi9889[] = {
	{
		I2C_BOARD_INFO("adi9889_i2c", 0x3d),
		.platform_data = &db_adi9889_i2c_conf,			
	},
	{
		I2C_BOARD_INFO("adi9889_edid_i2c", 0x3F),
		.platform_data = &db_adi9889_i2c_conf,					
	},
	{
		I2C_BOARD_INFO("ths8200_i2c", 0x21),
	},
};

/*****************************************************************************
 * NAND
 ****************************************************************************/
static struct mtd_partition partition_dove[] = {
	{ .name		= "UBoot",
	  .offset	= 0,
	  .size		= 1 * SZ_1M },
	{ .name		= "UImage",
	  .offset	= MTDPART_OFS_APPEND,
	  .size		= 4 * SZ_1M },
	{ .name		= "Root",
	  .offset	= MTDPART_OFS_APPEND,
	  .size         = MTDPART_SIZ_FULL},
};
static u64 nfc_dmamask = DMA_BIT_MASK(32);
static struct nfc_platform_data dove_db_nfc_hal_data = {
	.nfc_width      = 8,			/* Assume non-ganged by default */
	.num_devs       = 1,			/* Assume non-ganged by default */
	.num_cs         = 2,			/* Samsung K9HBG08U1A */
//	.num_cs		= 1,			/* MT 29F64G908CBAAA */
	.use_dma	= 1,
	.ecc_type	= MV_NFC_ECC_BCH_2K,	/* 4bit ECC required by K9HBG08U1A */
	.parts = partition_dove,
	.nr_parts = ARRAY_SIZE(partition_dove)
};

static struct nfc_platform_data dove_db_nfc_data = {
	.nfc_width      = 8,
	.num_devs       = 1,
	.num_cs         = 2,
	.use_dma        = 1,
	.ecc_type	= MV_NFC_ECC_HAMMING,
	.parts = partition_dove,
	.nr_parts = ARRAY_SIZE(partition_dove)
 };

static struct resource dove_nfc_resources[]  = {
	[0] = {
		.start	= (DOVE_NFC_PHYS_BASE),
		.end	= (DOVE_NFC_PHYS_BASE + 0xFF),
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_NAND,
		.end	= IRQ_NAND,
		.flags	= IORESOURCE_IRQ,
	},
	[2] = {
		/* DATA DMA */
		.start	= 97,
		.end	= 97,
		.flags	= IORESOURCE_DMA,
	},
	[3] = {
		/* COMMAND DMA */
		.start	= 99,
		.end	= 99,
		.flags	= IORESOURCE_DMA,
	},
};

static struct platform_device dove_nfc = {
	.name		= "orion-nfc",
	.id		= -1,
	.dev		= {
		.dma_mask		= &nfc_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &dove_db_nfc_data,
	},
	.resource	= dove_nfc_resources,
	.num_resources	= ARRAY_SIZE(dove_nfc_resources),
};

static void __init dove_db_nfc_init(void)
{
	/* Check if HAL driver is intended */
	if(useHalDrivers || useNandHal) {
		dove_nfc.name = "orion-nfc-hal";

		/* Check for ganaged mode */
		if (strncmp(useNandHal, "ganged", 6) == 0) {
			dove_db_nfc_hal_data.nfc_width = 16;
			dove_db_nfc_hal_data.num_devs = 2;
			useNandHal += 7;
		}

		/* Check for ECC type directive */
		if (strcmp(useNandHal, "4bitecc") == 0) {
			dove_db_nfc_hal_data.ecc_type = MV_NFC_ECC_BCH_2K;
		} else if (strcmp(useNandHal, "8bitecc") == 0) {
			dove_db_nfc_hal_data.ecc_type = MV_NFC_ECC_BCH_1K;
		} else if (strcmp(useNandHal, "12bitecc") == 0) {
			dove_db_nfc_hal_data.ecc_type = MV_NFC_ECC_BCH_704B;
		} else if (strcmp(useNandHal, "16bitecc") == 0) {
			dove_db_nfc_hal_data.ecc_type = MV_NFC_ECC_BCH_512B;
		}

		dove_db_nfc_hal_data.tclk = dove_tclk_get();
		dove_nfc.dev.platform_data = &dove_db_nfc_hal_data;

	} else {  /* NON HAL driver is used */
		dove_db_nfc_data.tclk = dove_tclk_get();
	}

	platform_device_register(&dove_nfc);
}

static struct dove_mpp_mode dove_db_mpp_modes[] __initdata = {
	{ 0, MPP_SDIO0 },           	/* SDIO0 */
	{ 1, MPP_SDIO0 },           	/* SDIO0 */

	{ 2, MPP_PMU },			/* Standby power control */
	{ 3, MPP_PMU },			/* Power button - standby wakeup */
	{ 4, MPP_PMU },			/* Core power good indication */
	{ 5, MPP_PMU },			/* DeepIdle power control */
	{ 6, MPP_PMU },			/* Wakup - Charger */
	{ 7, MPP_PMU },			/* Standby led */

	{ 8, MPP_GPIO },		/* power off */

	{ 9, MPP_PMU },			/* Cpu power good indication */
	{ 10, MPP_PMU },		/* DVS SDI control */

	{ 11, MPP_SATA_ACT },           /* SATA active */
	{ 12, MPP_SDIO1 },           	/* SDIO1 */
	{ 13, MPP_SDIO1 },           	/* SDIO1 */

        { 14, MPP_GPIO },               /* 7segDebug Led */
        { 15, MPP_GPIO },               /* 7segDebug Led */
	{ 16, MPP_GPIO },		/* PMU - DDR termination control */
	{ 17, MPP_TWSI },
	{ 18, MPP_GPIO },
	{ 19, MPP_TWSI },

	{ 20, MPP_SPI1 },
	{ 21, MPP_SPI1 },
	{ 22, MPP_SPI1 },
	{ 23, MPP_SPI1 },

	{ 24, MPP_CAM }, /* will configure MPPs 24-39*/

	{ 40, MPP_SDIO0 }, /* will configure MPPs 40-45 */

	{ 46, MPP_SDIO1 }, /* SD0 Group */
	{ 47, MPP_SDIO1 }, /* SD0 Group */
	{ 48, MPP_SDIO1 }, /* SD0 Group */
	{ 49, MPP_SDIO1 }, /* SD0 Group */
	{ 50, MPP_SDIO1 }, /* SD0 Group */
	{ 51, MPP_SDIO1 }, /* SD0 Group */

        { 52, MPP_AUDIO1 }, /* AU1 Group */
        { 53, MPP_AUDIO1 }, /* AU1 Group */
        { 54, MPP_AUDIO1 }, /* AU1 Group */
        { 55, MPP_AUDIO1 }, /* AU1 Group */
        { 56, MPP_AUDIO1 }, /* AU1 Group */
        { 57, MPP_AUDIO1 }, /* AU1 Group */

	{ 58, MPP_SPI0 }, /* will configure MPPs 58-61 */

        { 62, MPP_GPIO }, /* 7segDebug Led */
        { 63, MPP_GPIO }, /* Touch screen irq */
        { -1 },
};

static struct dove_mpp_mode dove_db_b_mpp_modes[] __initdata = {
	{ 0, MPP_PMU },           	/*  */
	{ 1, MPP_PMU },           	/* SDIO0 */

	{ 2, MPP_GPIO },		/* PMU - DDR termination control */	
	{ 3, MPP_PMU },			/* standby wakeup/wake on lan */
	{ 4, MPP_PMU },			/* CKE mask */
	{ 5, MPP_PMU },			/* Standby power control */
	{ 6, MPP_PMU },			/* DDR-M-RESET */
	{ 7, MPP_PMU },			/* Standby led */

	{ 8, MPP_GPIO },		/* power off */

	{ 9, MPP_GPIO },		/* USB DEV VBUS */
	{ 10, MPP_PMU },		/* DVS SDI control */

	{ 11, MPP_SATA_ACT },           /* SATA active */
	{ 12, MPP_SDIO1 },           	/* SDIO1 */
	{ 13, MPP_SDIO1 },           	/* SDIO1 */

        { 14, MPP_GPIO },               /* 7segDebug Led */
        { 15, MPP_GPIO },               /* 7segDebug Led */
	{ 16, MPP_SDIO0 },		/* SDIO0 */
#ifdef CONFIG_DOVE_DB_USE_GPIO_I2C
	{ 17, MPP_GPIO },
	{ 19, MPP_GPIO },
#else
	{ 17, MPP_TWSI },
	{ 19, MPP_TWSI },
#endif
	{ 18, MPP_GPIO },
	{ 20, MPP_SPI1 },
	{ 21, MPP_SPI1 },
	{ 22, MPP_SPI1 },
	{ 23, MPP_SPI1 },

	{ 24, MPP_CAM }, /* will configure MPPs 24-39*/

	{ 40, MPP_SDIO0 }, /* will configure MPPs 40-45 */

	{ 46, MPP_SDIO1 }, /* SD1 Group */
	{ 47, MPP_SDIO1 }, /* SD1 Group */
	{ 48, MPP_SDIO1 }, /* SD1 Group */
	{ 49, MPP_SDIO1 }, /* SD1 Group */
	{ 50, MPP_SDIO1 }, /* SD1 Group */
	{ 51, MPP_SDIO1 }, /* SD1 Group */

        { 52, MPP_AUDIO1 }, /* AU1 Group */
        { 53, MPP_AUDIO1 }, /* AU1 Group */
        { 54, MPP_AUDIO1 }, /* AU1 Group */
        { 55, MPP_AUDIO1 }, /* AU1 Group */
        { 56, MPP_AUDIO1 }, /* AU1 Group */
        { 57, MPP_AUDIO1 }, /* AU1 Group */

	{ 58, MPP_SPI0 }, /* will configure MPPs 58-61 */

        { 62, MPP_GPIO }, /* 7segDebug Led */
        { 63, MPP_GPIO }, /* Touch screen irq */
        { -1 },
};

static struct dove_mpp_mode dove_db_tact_int_mpp_modes[] __initdata = {
	{ 57, MPP_GPIO_AUDIO1 }, /* use this mpp for the tact irq line */
	{ -1 },
};

static void dove_db_power_off(void)
{
	if (gpio_request(DOVE_DB_POWER_OFF_GPIO, "DOVE_DB_POWER_OFF") != 0) {
		pr_err("Dove: failed to setup power off GPIO\n");
		return;
	}

	if (gpio_direction_output(DOVE_DB_POWER_OFF_GPIO, 0) != 0) {
 		printk(KERN_ERR "%s failed to set power off output pin %d\n",
		       __func__, DOVE_DB_POWER_OFF_GPIO);
		return;
	}
}

#ifdef CONFIG_PM
/*****************************************************************************
 * POWER MANAGEMENT
 ****************************************************************************/
extern int global_dvs_enable;
extern u32 dvs_values_param;
static int __init dove_db_pm_init(void)
{
	MV_PMU_INFO pmuInitInfo;
	u32 dev, rev;

	if (!machine_is_dove_db() && !machine_is_dove_db_b())
		return 0;

	global_dvs_enable = dvs_enable;

	dove_pcie_id(&dev, &rev);

	pmuInitInfo.batFltMngDis = MV_FALSE;				/* Keep battery fault enabled */
	pmuInitInfo.exitOnBatFltDis = MV_FALSE;				/* Keep exit from STANDBY on battery fail enabled */
	if (machine_is_dove_db() && rev < DOVE_REV_A0) {

		pmuInitInfo.sigSelctor[0] = PMU_SIGNAL_NC;
		pmuInitInfo.sigSelctor[1] = PMU_SIGNAL_NC;
		pmuInitInfo.sigSelctor[2] = PMU_SIGNAL_SLP_PWRDWN;		/* STANDBY => 0: I/O off, 1: I/O on */
		pmuInitInfo.sigSelctor[3] = PMU_SIGNAL_EXT0_WKUP;		/* power on push button */	
		if (rev >= DOVE_REV_X0) { /* For X0 and higher Power Good indication is not needed */
			if (standby_fix)
				pmuInitInfo.sigSelctor[4] = PMU_SIGNAL_CKE_OVRID;	/* CKE controlled by Dove */
			else
				pmuInitInfo.sigSelctor[4] = PMU_SIGNAL_NC;
		}
		else
			pmuInitInfo.sigSelctor[4] = PMU_SIGNAL_CPU_PWRGOOD;	/* CORE power good used as Standby PG */
		
		pmuInitInfo.sigSelctor[5] = PMU_SIGNAL_CPU_PWRDWN;		/* DEEP-IdLE => 0: CPU off, 1: CPU on */
		
		if ((rev >= DOVE_REV_X0) && (standby_fix)) /* For boards with X0 we use MPP6 as MRESET */
			pmuInitInfo.sigSelctor[6] = PMU_SIGNAL_MRESET_OVRID;		/* M_RESET is pulled up - always HI */
		else
			pmuInitInfo.sigSelctor[6] = PMU_SIGNAL_NC;
	} else {
		
		pmuInitInfo.sigSelctor[0] = PMU_SIGNAL_CPU_PWRDWN; /* DEEP-IdLE => 0: CPU off, 1: CPU on */
		pmuInitInfo.sigSelctor[1] = PMU_SIGNAL_1;
		
		pmuInitInfo.sigSelctor[2] = PMU_SIGNAL_NC;
		
		pmuInitInfo.sigSelctor[3] = PMU_SIGNAL_EXT0_WKUP;		/* power on push button */	
		pmuInitInfo.sigSelctor[4] = PMU_SIGNAL_CKE_OVRID;	/* CKE controlled by Dove */
		pmuInitInfo.sigSelctor[5] = PMU_SIGNAL_SLP_PWRDWN;		/* STANDBY => 0: I/O off, 1: I/O on */
		
		/* For boards with X0 we use MPP6 as MRESET */
		pmuInitInfo.sigSelctor[6] = PMU_SIGNAL_MRESET_OVRID;
	}
	pmuInitInfo.sigSelctor[7] = PMU_SIGNAL_1;			/* Standby Led - inverted */
	pmuInitInfo.sigSelctor[8] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[9] = PMU_SIGNAL_NC;			/* CPU power good  - not used */
	pmuInitInfo.sigSelctor[10] = PMU_SIGNAL_SDI;			/* Voltage regulator control */
	pmuInitInfo.sigSelctor[11] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[12] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[13] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[14] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[15] = PMU_SIGNAL_NC;
	pmuInitInfo.dvsDelay = 0x4200;				/* ~100us in 166MHz cc - delay for DVS change */

	if (dvs_values_param != 0)
		pmuInitInfo.dvsValues = dvs_values_param;	/* calibrated values from u-boot */
	else
		pmuInitInfo.dvsValues = (DOVE_PSET_HI << DOVE_PSET_HI_OFFSET) | (DOVE_VSET_HI << DOVE_VSET_HI_OFFSET) | (DOVE_PSET_LO << DOVE_PSET_LO_OFFSET) | (DOVE_VSET_LO << DOVE_VSET_LO_OFFSET);	/* default values */

	if (machine_is_dove_db_b() || rev >= DOVE_REV_A0)
		pmuInitInfo.ddrTermGpioNum = 2;			/* GPIO 2 used to disable terminations */
	else
		pmuInitInfo.ddrTermGpioNum = 16;			/* GPIO 16 used to disable terminations */

	if (rev >= DOVE_REV_X0) /* For X0 and higher wait at least 150ms + spare */
		pmuInitInfo.standbyPwrDelay = 0x2000;		/* 250ms delay to wait for complete powerup */
	else
		pmuInitInfo.standbyPwrDelay = 0x140;		/* 10ms delay after getting the power good indication */
	
	/* Initialize the PMU HAL */
	if (mvPmuInit(&pmuInitInfo) != MV_OK)
	{
		printk(KERN_ERR "ERROR: Failed to initialise the PMU!\n");
		return 0;
	}

	/* Configure wakeup events */
	mvPmuWakeupEventSet(PMU_STBY_WKUP_CTRL_EXT0_FALL | PMU_STBY_WKUP_CTRL_RTC_MASK);

	/* Register the PM operation in the Linux stack */
	dove_pm_register();

	return 0;
}

__initcall(dove_db_pm_init);
#endif

/*****************************************************************************
 * Board Init
 ****************************************************************************/
//extern struct mbus_dram_target_info dove_mbus_dram_info;
//extern int __init pxa_init_dma_wins(struct mbus_dram_target_info * dram);
static void __init dove_db_init(void)
{
	u32 dev, rev;

	/*
	 * Basic Dove setup. Needs to be called early.
	 */
	dove_init();

	dove_pcie_id(&dev, &rev);

	if (machine_is_dove_db_b() || rev >= DOVE_REV_A0) {
		dove_mpp_conf(dove_db_b_mpp_modes);
		dove_db_b_giga_phy_gpio_setup();
	} else
		dove_mpp_conf(dove_db_mpp_modes);
	
	if ((front_panel) && (left_tact || right_tact)) {
		dove_mpp_conf(dove_db_tact_int_mpp_modes);
		i2s1_data.spdif_play = 0;
	}

	pm_power_off = dove_db_power_off;

        /* the (SW1) button is for use as a "wakeup" button */
	dove_wakeup_button_setup(DOVE_DB_WAKEUP_GPIO);

	/* sdio card interrupt workaround using GPIOs */
	dove_sd_card_int_wa_setup(0);
	dove_sd_card_int_wa_setup(1);

	if(front_panel) {
		/* JPR6 shoud be on 1-2 for touchscreen irq line */

		if (dove_db_ts_gpio_setup() != 0)
			return;
	}

#if defined(CONFIG_SND_DOVE_AC97)
#if !defined(CONFIG_CPU_ENDIAN_BE8)
	/* FIXME:
	 * we need fix __AC97(x) definition in
	 * arch/arm/mach-dove/include/mach/pxa-regs.h
	 * to resolve endian access.
	 */
	/* Initialize AC'97 related regs.	*/
	dove_ac97_setup();
#endif
#endif

	dove_rtc_init();

	pxa_init_dma_wins(&dove_mbus_dram_info);
	pxa_init_dma(16);
#ifdef CONFIG_MV_HAL_DRIVERS_SUPPORT
	if(useHalDrivers || useNandHal) {
		if (mvPdmaHalInit(MV_PDMA_MAX_CHANNELS_NUM) != MV_OK) {
			printk(KERN_ERR "mvPdmaHalInit() failed.\n");
			BUG();
		}
		/* reserve channels for NAND Data and command PDMA */
		pxa_reserve_dma_channel(MV_PDMA_NAND_DATA);
		pxa_reserve_dma_channel(MV_PDMA_NAND_COMMAND);
	}
#endif
	dove_xor0_init();
	dove_xor1_init();
#ifdef CONFIG_MV_ETHERNET
	if(use_hal_giga || useHalDrivers)
		dove_mv_eth_init();
	else
#endif
	if (rev >= DOVE_REV_A0)
		dove_ge00_init(&dove_db_b_ge00_data);
	else
		dove_ge00_init(&dove_db_ge00_data);
	dove_ehci0_init();
	dove_ehci1_init();

	/* ehci init functions access the usb port, only now it's safe to disable
	 * all clocks
	 */
	ds_clks_disable_all(0, 0);
	dove_sata_init(&dove_db_sata_data);
	dove_spi0_init(0);
	dove_spi1_init(1);
	dove_uart0_init();
	dove_uart1_init();
	dove_i2c_init();
	dove_i2c_exp_init(0);
#ifdef CONFIG_DOVE_DB_USE_GPIO_I2C
	dove_add_gpio_i2c();
#else
	dove_i2c_exp_init(1);
#endif
	dove_sdhci_cam_mbus_init();
	dove_sdio0_init();
	dove_sdio1_init();
	dove_db_nfc_init();
	dove_db_clcd_init();
	dove_vmeta_init();
	dove_gpu_init();
	dove_ssp_init(&dove_ssp_platform_data);
	dove_cesa_init();
	dove_hwmon_init();

#if !defined(CONFIG_SND_DOVE_AC97)
	dove_i2s_init(0, &i2s0_data);
#endif
	dove_i2s_init(1, &i2s1_data);

	i2c_register_board_info(0, &i2c_a2d, 1);
	i2c_register_board_info(0, dove_db_gpio_ext_info, 1);
	if (machine_is_dove_db_b() || rev >= DOVE_REV_A0)
		i2c_register_board_info(0, &idt, 1);
	if (lcd2dvi)
		i2c_register_board_info(0, adi9889, ARRAY_SIZE(adi9889));
	spi_register_board_info(dove_db_spi_flash_info,
				ARRAY_SIZE(dove_db_spi_flash_info));
	if (front_panel)
		spi_register_board_info(dove_db_spi_ts_dev, 
					ARRAY_SIZE(dove_db_spi_ts_dev));
#ifdef CONFIG_ANDROID_PMEM
        android_add_pmem("pmem", 0x02000000UL, 1, 0);
        android_add_pmem("pmem_adsp", 0x00400000UL, 0, 0);
#endif

}

MACHINE_START(DOVE_DB, "Marvell DB-MV88F6781-BP Development Board")
	.phys_io	= DOVE_SB_REGS_PHYS_BASE,
	.io_pg_offst	= ((DOVE_SB_REGS_VIRT_BASE) >> 18) & 0xfffc,
	.boot_params	= 0x00000100,
	.init_machine	= dove_db_init,
	.map_io		= dove_map_io,
	.init_irq	= dove_init_irq,
	.timer		= &dove_timer,
/* reserve memory for VMETA and GPU */
	.fixup		= dove_tag_fixup_mem32,
MACHINE_END

MACHINE_START(DOVE_DB_B, "Marvell DB-MV88F6781-BP-B Development Board")
	.phys_io	= DOVE_SB_REGS_PHYS_BASE,
	.io_pg_offst	= ((DOVE_SB_REGS_VIRT_BASE) >> 18) & 0xfffc,
	.boot_params	= 0x00000100,
	.init_machine	= dove_db_init,
	.map_io		= dove_map_io,
	.init_irq	= dove_init_irq,
	.timer		= &dove_timer,
/* reserve memory for VMETA and GPU */
	.fixup		= dove_tag_fixup_mem32,
MACHINE_END
