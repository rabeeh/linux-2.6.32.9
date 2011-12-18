/*
 * arch/arm/mach-dove/dove-videoplug-setup.c
 *
 * Marvell 88AP510 Video Plug Board Setup
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

#define DOVE_VIDEOPLUG_WAKEUP_GPIO	(3)
#define DOVE_VIDEOPLUG_POWER_OFF_GPIO	(8)

static unsigned int use_hal_giga = 0;
#ifdef CONFIG_MV643XX_ETH
module_param(use_hal_giga, uint, 0);
MODULE_PARM_DESC(use_hal_giga, "Use the HAL giga driver");
#endif

extern unsigned int useHalDrivers;
extern char *useNandHal;

static struct dovefb_mach_info dove_videoplug_lcd0_dmi = {
	.id_gfx			= "GFX Layer 0",
	.id_ovly		= "Video Layer 0",

	.clk_src		= MRVL_EXT_CLK1, /* enable using external clock */
	.clk_name		= "IDT_CLK1", /* use clock 1 */

//	.num_modes		= ARRAY_SIZE(video_modes),
//	.modes			= video_modes,
	.pix_fmt		= PIX_FMT_RGB888PACK,
	.io_pin_allocation	= IOPAD_DUMB24,
	.panel_rgb_type		= DUMB24_RGB888_0,
	.panel_rgb_reverse_lanes= 0,
	.gpio_output_data	= 0,// AVG 3,
	.gpio_output_mask	= 0,// AVG 3,
#ifndef CONFIG_FB_DOVE_CLCD0_I2C_DEFAULT_SETTING
	.ddc_i2c_adapter	= CONFIG_FB_DOVE_CLCD0_I2C_CHANNEL,
	.ddc_i2c_address	= CONFIG_FB_DOVE_CLCD0_I2C_ADDRESS,
#else
	.ddc_i2c_adapter	= 0,// AVG3,
	.ddc_i2c_address	= 0x3f,
#endif
	.invert_composite_blank	= 0,
	.invert_pix_val_ena	= 0,
	.invert_pixclock	= 0,
	.invert_vsync		= 0,
	.invert_hsync		= 0,
	.panel_rbswap		= 1,
	.active			= 1,
};

static struct dovefb_mach_info dove_videoplug_lcd0_vid_dmi = {
	.id_ovly		= "Video Layer 0",
//	.num_modes		= ARRAY_SIZE(video_modes),
//	.modes			= video_modes,
	.pix_fmt		= PIX_FMT_RGB888PACK,
	.io_pin_allocation	= IOPAD_DUMB24, //IOPAD_DUMB18GPIO,
	.panel_rgb_type		= DUMB24_RGB888_0, //DUMB18_RGB666_0,
	.panel_rgb_reverse_lanes= 0,
	.gpio_output_data	= 0, // AVG 3,
	.gpio_output_mask	= 0, // AVG 3,
	.ddc_i2c_adapter	= -1,
	.invert_composite_blank	= 0,
	.invert_pix_val_ena	= 0,
	.invert_pixclock	= 0,
	.invert_vsync		= 0,
	.invert_hsync		= 0,
	.panel_rbswap		= 1,
	.active			= 1,
	.enable_lcd0		= 1,
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

void __init dove_videoplug_clcd_init(void) {
#ifdef CONFIG_FB_DOVE
	u32 dev, rev;
	
	dove_pcie_id(&dev, &rev);

	/* LCD external clock supported only starting from rev A0 */
	if (rev < DOVE_REV_A0)
		dove_videoplug_lcd0_dmi.clk_src = MRVL_PLL_CLK;

	clcd_platform_init(&dove_videoplug_lcd0_dmi, &dove_videoplug_lcd0_vid_dmi,
			   NULL, NULL, NULL);
#endif /* CONFIG_FB_DOVE */
}

extern int __init pxa_init_dma_wins(struct mbus_dram_target_info * dram);

static struct orion_i2s_platform_data i2s1_data = {
	.i2s_play	= 0,
	.i2s_rec	= 0,
	.spdif_play	= 1,
};

#ifndef CONFIG_SND_DOVE_AC97
static struct orion_i2s_platform_data i2s0_data = {
	.i2s_play	= 0,
	.i2s_rec	= 0,
};
#else

#endif

#if 1
static struct mv643xx_eth_platform_data dove_videoplug_ge00_data = {
	.phy_addr	= MV643XX_ETH_PHY_ADDR_DEFAULT,
};

#endif

static struct mv_sata_platform_data dove_videoplug_sata_data = {
        .n_ports        = 1,
};

struct adi9889_i2c_platform_data videoplug_adi9889_i2c_conf ={
	.audio_format = SPDIF_FORMAT,

};



/*****************************************************************************
 * SPI Devices:
 * 	SPI0: 4M Flash ST-M25P32-VMF6P
 ****************************************************************************/
static const struct flash_platform_data dove_videoplug_spi_flash_data = {
	.type		= "m25p64",
};

static struct spi_board_info __initdata dove_videoplug_spi_flash_info[] = {
	{
		.modalias       = "m25p80",
		.platform_data  = &dove_videoplug_spi_flash_data,
		.irq            = -1,
		.max_speed_hz   = 20000000,
		.bus_num        = 0,
		.chip_select    = 0,
	},
};

/*****************************************************************************
 * PCI
 ****************************************************************************/
static int __init dove_videoplug_pci_init(void)
{
	if (machine_is_videoplug())
		dove_pcie_init(1, 1);

	return 0;
}

subsys_initcall(dove_videoplug_pci_init);


/*****************************************************************************
 * I2C buses - adc, hdmi
 ****************************************************************************/
static struct i2c_board_info __initdata i2c_plug[] = {
	{
		I2C_BOARD_INFO("adi9889_i2c", 0x39),
		.platform_data = &videoplug_adi9889_i2c_conf,					
	},
	{
		I2C_BOARD_INFO("adi9889_edid_i2c", 0x3F),
		.platform_data = &videoplug_adi9889_i2c_conf,					
	},
};

/*****************************************************************************
 * IDT clock 
 ****************************************************************************/
static struct idt_data dove_videoplug_idt_data = {
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
	.platform_data = &dove_videoplug_idt_data,
	
};

/*****************************************************************************
 * A2D on I2C bus
 ****************************************************************************/
static struct i2c_board_info __initdata i2c_a2d = {
	I2C_BOARD_INFO("cs42l51", 0x4A),
};

static struct platform_device dit_data = {
	.name = "spdif-dit",
	.id = -1
};


/*****************************************************************************
 * NAND
 ****************************************************************************/
static struct mtd_partition partition_dove[] = {
	{ .name		= "UBoot",
	  .offset	= 0,
	  .size		= 1 * SZ_1M },
	{ .name		= "uImage",
	  .offset	= MTDPART_OFS_APPEND,
	  .size		= 4 * SZ_1M },
	{ .name		= "initrd",
	  .offset	= MTDPART_OFS_APPEND,
	  .size		= 4 * SZ_1M },
	{ .name		= "root",
	  .offset	= MTDPART_OFS_APPEND,
	  .size         = MTDPART_SIZ_FULL},
};
static u64 nfc_dmamask = DMA_BIT_MASK(32);

static struct nfc_platform_data dove_videoplug_nfc_hal_data = {
	.nfc_width      = 8,			/* Assume non-ganged mode by default */
	.num_devs       = 1,			/* Assume non-ganged mode by default */
	.num_cs         = 1,
	.use_dma	= 1,
	.ecc_type	= MV_NFC_ECC_BCH_1K,	/* Assume 8bit ECC by default - K9LBG08U0D */
	.parts = partition_dove,
	.nr_parts = ARRAY_SIZE(partition_dove)
};

static struct nfc_platform_data dove_videoplug_nfc_data = {
	.nfc_width      = 8,
	.num_devs       = 1,
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
		.platform_data		= &dove_videoplug_nfc_data,
	},
	.resource	= dove_nfc_resources,
	.num_resources	= ARRAY_SIZE(dove_nfc_resources),
};

static void __init dove_videoplug_nfc_init(void)
{
	/* Check if HAL driver is intended */
	if(useHalDrivers || useNandHal) {
		dove_nfc.name = "orion-nfc-hal";

		/* Check for ganaged mode */
		if (strncmp(useNandHal, "ganged", 6) == 0) {
			dove_videoplug_nfc_hal_data.nfc_width = 16;
			dove_videoplug_nfc_hal_data.num_devs = 2;
			useNandHal += 7;
		}

		/* Check for ECC type directive */
		if (strcmp(useNandHal, "4bitecc") == 0) {
			dove_videoplug_nfc_hal_data.ecc_type = MV_NFC_ECC_BCH_2K;
		} else if (strcmp(useNandHal, "8bitecc") == 0) {
			dove_videoplug_nfc_hal_data.ecc_type = MV_NFC_ECC_BCH_1K;
		} else if (strcmp(useNandHal, "12bitecc") == 0) {
			dove_videoplug_nfc_hal_data.ecc_type = MV_NFC_ECC_BCH_704B;
		} else if (strcmp(useNandHal, "16bitecc") == 0) {
			dove_videoplug_nfc_hal_data.ecc_type = MV_NFC_ECC_BCH_512B;
		}

		dove_videoplug_nfc_hal_data.tclk = dove_tclk_get();
		dove_nfc.dev.platform_data = &dove_videoplug_nfc_hal_data;

	} else {  /* NON HAL driver is used */
		dove_videoplug_nfc_data.tclk = dove_tclk_get();
	}

	platform_device_register(&dove_nfc);
}

static struct dove_mpp_mode dove_videoplug_mpp_modes[] __initdata = {
	{ 0, MPP_GPIO },           	/* WOL_INT */
	{ 1, MPP_GPIO },           	/* Green Led Status */

	{ 2, MPP_PMU },			/* Standby power control / PM Standby enable*/
	{ 3, MPP_PMU },			/* Power button - standby wakeup */
	{ 4, MPP_PMU },			/* Core power good indication */
	{ 5, MPP_PMU },			/* DeepIdle power control/CPU Enable */
	{ 6, MPP_GPIO},			/* DDR termination */
	{ 7, MPP_PMU },			/* CPU Power good indication */

	{ 8, MPP_GPIO },		/* power off */

	{ 10, MPP_PMU },		/* DVS SDI control */

	{ 12, MPP_SDIO1 },           	/* SDIO1 */
	{ 13, MPP_GPIO },           	/* Blue Led Status */

        { 15, MPP_GPIO },               /* Amber Les status ( could be MPP14 )*/

	{ 24, MPP_CAM }, /* will configure MPPs 24-39*/

	{ 40, MPP_SDIO0 }, /* will configure MPPs 40-45 */

	{ 46, MPP_SDIO1 }, /*  will configure MPPs 46-51 */

        { 52, MPP_AUDIO1 }, /* AU1 Group - ill configure MPPs 52-57  */
 
	{ 58, MPP_SPI0 }, /* will configure MPPs 58-61 */

        { 62, MPP_UART1 }, /* UA1 */
        { 64, MPP_GPIO }, /* will configure MPPs 64-72 -  NF_IO [8:15] */
        { -1 },
};

static void dove_videoplug_power_off(void)
{
	if (gpio_request(DOVE_VIDEOPLUG_POWER_OFF_GPIO, "DOVE_VIDEOPLUG_POWER_OFF") != 0) {
		pr_err("Dove: failed to setup power off GPIO\n");
		return;
	}

	if (gpio_direction_output(DOVE_VIDEOPLUG_POWER_OFF_GPIO, 0) != 0) {
 		printk(KERN_ERR "%s failed to set power off output pin %d\n",
		       __func__, DOVE_VIDEOPLUG_POWER_OFF_GPIO);
		return;
	}
}

#ifdef CONFIG_PM
/*****************************************************************************
 * POWER MANAGEMENT
 ****************************************************************************/
static int __init dove_videoplug_pm_init(void)
{
	MV_PMU_INFO pmuInitInfo;
	u32 dev, rev;

	if (!machine_is_videoplug())
		return 0;

	dove_pcie_id(&dev, &rev);

	pmuInitInfo.batFltMngDis = MV_FALSE;			/* Keep battery fault enabled */
	pmuInitInfo.exitOnBatFltDis = MV_FALSE;			/* Keep exit from STANDBY on battery fail enabled */
	pmuInitInfo.sigSelctor[0] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[1] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[2] = PMU_SIGNAL_SLP_PWRDWN;	/* STANDBY => 0: I/O off, 1: I/O on */
	pmuInitInfo.sigSelctor[3] = PMU_SIGNAL_EXT0_WKUP;	/* power on push button */	
	if (rev >= DOVE_REV_X0) /* For X0 and higher Power Good indication is not needed */
		pmuInitInfo.sigSelctor[4] = PMU_SIGNAL_NC;
	else
		pmuInitInfo.sigSelctor[4] = PMU_SIGNAL_CPU_PWRGOOD;	/* CORE power good used as Standby PG */
	pmuInitInfo.sigSelctor[5] = PMU_SIGNAL_CPU_PWRDWN;	/* DEEP-IdLE => 0: CPU off, 1: CPU on */
	pmuInitInfo.sigSelctor[6] = PMU_SIGNAL_NC;		/* Charger interrupt - not used */
	pmuInitInfo.sigSelctor[7] = PMU_SIGNAL_NC;		/* CPU power good  - not used */
	pmuInitInfo.sigSelctor[8] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[9] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[10] = PMU_SIGNAL_SDI;		/* Voltage regulator control */
	pmuInitInfo.sigSelctor[11] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[12] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[13] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[14] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[15] = PMU_SIGNAL_NC;
	pmuInitInfo.dvsDelay = 0x4200;				/* ~100us in 166MHz cc - delay for DVS change */
	pmuInitInfo.ddrTermGpioNum = 6;			/* GPIO 6 used to disable terminations */
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

__initcall(dove_videoplug_pm_init);
#endif

/*****************************************************************************
 * Board Init
 ****************************************************************************/
//extern struct mbus_dram_target_info dove_mbus_dram_info;
//extern int __init pxa_init_dma_wins(struct mbus_dram_target_info * dram);
static void __init dove_videoplug_init(void)
{
	/*
	 * Basic Dove setup. Needs to be called early.
	 */
	dove_init();

	dove_mpp_conf(dove_videoplug_mpp_modes);
	
	pm_power_off = dove_videoplug_power_off;

        /* the (SW1) button is for use as a "wakeup" button */
	dove_wakeup_button_setup(DOVE_VIDEOPLUG_WAKEUP_GPIO);

	/* sdio card interrupt workaround using GPIOs */
	dove_sd_card_int_wa_setup(0);


#if defined(CONFIG_SND_DOVE_AC97)
#if !defined(CONFIG_CPU_ENDIAN_BE8)
	/* FIXME:
	 * we need fix __AC97(x) definition in
	 * arch/arm/mach-dove/include/mach/pxa-regs.h
	 * to resolve endian access.
	 */
	/* Initialize AC'97 related regs.	*/
//	dove_ac97_setup();
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
	dove_ge00_init(&dove_videoplug_ge00_data);
	dove_ehci0_init();
	dove_ehci1_init();

	/* ehci init functions access the usb port, only now it's safe to disable
	 * all clocks
	 */
	ds_clks_disable_all(0, 0);
	dove_sata_init(&dove_videoplug_sata_data);
	dove_spi0_init(0);
	dove_spi1_init(1);
	dove_uart0_init();
	dove_uart1_init();
	dove_i2c_init();
	dove_i2c_exp_init(0);
	dove_i2c_exp_init(1);
	dove_sdhci_cam_mbus_init();
	dove_sdio0_init();
	dove_videoplug_nfc_init();
	dove_videoplug_clcd_init();
	dove_vmeta_init();
	dove_gpu_init();
	dove_ssp_init(&dove_ssp_platform_data);
	dove_cesa_init();
	dove_hwmon_init();

#if !defined(CONFIG_SND_DOVE_AC97)
	dove_i2s_init(0, &i2s0_data);
#endif
	dove_i2s_init(1, &i2s1_data);
	platform_device_register(&dit_data);

	i2c_register_board_info(0, i2c_plug, ARRAY_SIZE(i2c_plug));
	i2c_register_board_info(0, &i2c_a2d, 1);
	i2c_register_board_info(0, &idt, 1);
	//i2c_register_board_info(0, dove_videoplug_gpio_ext_info, 1);
	spi_register_board_info(dove_videoplug_spi_flash_info,
				ARRAY_SIZE(dove_videoplug_spi_flash_info));
}

MACHINE_START(DOVE_VIDEOPLUG, "Marvell 88FAP510 Video Plug Board")
	.phys_io	= DOVE_SB_REGS_PHYS_BASE,
	.io_pg_offst	= ((DOVE_SB_REGS_VIRT_BASE) >> 18) & 0xfffc,
	.boot_params	= 0x00000100,
	.init_machine	= dove_videoplug_init,
	.map_io		= dove_map_io,
	.init_irq	= dove_init_irq,
	.timer		= &dove_timer,
/* reserve memory for VMETA and GPU */
	.fixup		= dove_tag_fixup_mem32,
MACHINE_END
