/* * arch/arm/mach-dove/dove-cubox.c
 *
 * SolidRun CuBox platform
 *
 * Author: Rabeeh Khoury <rabeeh@solid-run.com>
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
#include <linux/timer.h>
#include <linux/ata_platform.h>
#include <linux/mv643xx_eth.h>
#include <linux/i2c.h>
#include <linux/pci.h>
#include <linux/gpio_mouse.h>
#include <linux/spi/spi.h>
#include <linux/spi/orion_spi.h>
#include <linux/spi/flash.h>
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
#include <mach/pm.h>
#include <plat/cafe-orion.h>
#include "common.h"
#include "clock.h"
#include "idt5v49ee503.h"
#include "mpp.h"
#include "mach/pm.h"
#include "pmu/mvPmu.h"
#include "pmu/mvPmuRegs.h"
#include "pdma/mvPdma.h"
#include <ctrlEnv/mvCtrlEnvRegs.h>
#include <audio/mvAudioRegs.h>
#include "gpp/mvGppRegs.h"
#include <linux/i2c/tsc2007.h>
#include <linux/adi9889_i2c.h>

extern int __init pxa_init_dma_wins(struct mbus_dram_target_info *dram);

static unsigned int use_hal_giga = 0;
#ifdef CONFIG_MV643XX_ETH
module_param(use_hal_giga, uint, 0);
MODULE_PARM_DESC(use_hal_giga, "Use the HAL giga driver");
#endif

static int dvs_enable = 0;
module_param(dvs_enable, int, 1);
MODULE_PARM_DESC(dvs_enable, "if 1 then enable DVS");

extern unsigned int useHalDrivers;
extern char *useNandHal;


/*****************************************************************************
 * LCD
 ****************************************************************************/
/*
 * LCD HW output Red[0] to LDD[0] when set bit [19:16] of reg 0x190
 * to 0x0. Which means HW outputs BGR format default. All platforms
 * uses this controller should enable .panel_rbswap. Unless layout
 * design connects Blue[0] to LDD[0] instead.
 */
static struct dovefb_mach_info dove_cubox_lcd0_dmi = {
	.id_gfx			= "GFX Layer 0",
	.id_ovly		= "Video Layer 0",
//	.clk_src		= MRVL_PLL_CLK,
//	.clk_name		= "accurate_LCDCLK",
	.clk_src		= MRVL_EXT_CLK1,
	.clk_name		= "SILAB_CLK0",
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
	.ddc_i2c_adapter	= 1,
	.ddc_i2c_address	= 0x3f,
#endif
	.secondary_ddc_mode	= 1,
	.ddc_i2c_adapter_2nd	= 0,
	.ddc_i2c_address_2nd    = 0x50,
	.invert_composite_blank	= 0,
	.invert_pix_val_ena	= 0,
	.invert_pixclock	= 0,
	.invert_vsync		= 0,
	.invert_hsync		= 0,
	.panel_rbswap		= 1,
	.active			= 1,
};

static struct dovefb_mach_info dove_cubox_lcd0_vid_dmi = {
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

/*****************************************************************************
 * BACKLIGHT
 ****************************************************************************/
static struct dovebl_platform_data dove_rd_avng_v3_backlight_data = {
	.default_intensity = 0xa,
	.gpio_pm_control = 1,

	.lcd_start = DOVE_SB_REGS_VIRT_BASE,	/* lcd power control reg base. */
	.lcd_end = DOVE_SB_REGS_VIRT_BASE+GPP_DATA_OUT_REG(0),	/* end of reg map. */
	.lcd_offset = GPP_DATA_OUT_REG(0),	/* register offset */
	.lcd_mapped = 1,		/* va = 0, pa = 1 */
	.lcd_mask = (1<<26),		/* mask, bit[11] */
	.lcd_on = (1<<26),		/* value to enable lcd power */
	.lcd_off = 0x0,			/* value to disable lcd power */

	.blpwr_start = DOVE_SB_REGS_VIRT_BASE, /* bl pwr ctrl reg base. */
	.blpwr_end = DOVE_SB_REGS_VIRT_BASE+GPP_DATA_OUT_REG(0),	/* end of reg map. */
	.blpwr_offset = GPP_DATA_OUT_REG(0),	/* register offset */
	.blpwr_mapped = 1,		/* pa = 0, va = 1 */
	.blpwr_mask = (1<<28),		/* mask, bit[13] */
	.blpwr_on = (1<<28),		/* value to enable bl power */
	.blpwr_off = 0x0,		/* value to disable bl power */

	.btn_start = DOVE_LCD1_PHYS_BASE, /* brightness control reg base. */
	.btn_end = DOVE_LCD1_PHYS_BASE+0x1C8,	/* end of reg map. */
	.btn_offset = LCD_CFG_GRA_PITCH,	/* register offset */
	.btn_mapped = 0,		/* pa = 0, va = 1 */
	.btn_mask = 0xF0000000,		/* mask */
	.btn_level = 15,		/* how many level can be configured. */
	.btn_min = 0x1,			/* min value */
	.btn_max = 0xF,			/* max value */
	.btn_inc = 0x1,			/* increment */
};

void __init dove_cubox_clcd_init(void) {
#ifdef CONFIG_FB_DOVE
	clcd_platform_init(&dove_cubox_lcd0_dmi, &dove_cubox_lcd0_vid_dmi,
			   NULL,NULL,
			   &dove_rd_avng_v3_backlight_data);

#endif /* CONFIG_FB_DOVE */
}

/*****************************************************************************
 * I2C devices:
 * 	ALC5630 codec, address 0x
 * 	Battery charger, address 0x??
 * 	G-Sensor, address 0x??
 * 	MCU PIC-16F887, address 0x??
 ****************************************************************************/
static struct i2c_board_info __initdata dove_cubox_i2c_bus0_devs[] = {
	{
		I2C_BOARD_INFO("silab5351a", 0x60),
	},
#ifdef CONFIG_TDA19988
	{ /* First CEC that enables 0x70 for HDMI */
		I2C_BOARD_INFO("tda99Xcec", 0x34),.irq=91,
	},
	{
		I2C_BOARD_INFO("tda998X", 0x70),.irq=91,
	},
#endif
	{
		I2C_BOARD_INFO("cs42l51", 0x4A), /* Fake device for spdif only */
	},
};

static struct platform_device dit_data = {
        .name = "spdif-dit",
        .id = -1
};

/*****************************************************************************
 * Audio I2S
 ****************************************************************************/
static struct orion_i2s_platform_data i2s1_data = {
	.i2s_play       = 0,
	.i2s_rec        = 0,
	.spdif_play     = 1,
	.spdif_rec	= 0,
};


/*****************************************************************************
 * Ethernet
 ****************************************************************************/
static struct mv643xx_eth_platform_data dove_cubox_ge00_data = {
	.phy_addr	= MV643XX_ETH_PHY_ADDR_DEFAULT,
};


/*****************************************************************************
 * SATA
 ****************************************************************************/
static struct mv_sata_platform_data dove_cubox = {
        .n_ports        = 1,
};


/*****************************************************************************
 * SPI Devices:
 *     SPI0: 4M Flash MX25L3205D
 ****************************************************************************/
static const struct flash_platform_data dove_cubox_spi_flash_data = {
//	.type           = "mx25l3205d",
};

static struct spi_board_info __initdata dove_cubox_spi_flash_info[] = {
	{
		.modalias       = "m25p80",
		.platform_data  = &dove_cubox_spi_flash_data,
		.irq            = -1,
		.max_speed_hz   = 20000000,
		.bus_num        = 0,
		.chip_select    = 0,
	},
};

/*****************************************************************************
 * MPP
 ****************************************************************************/
static struct dove_mpp_mode dove_cubox_mpp_modes[] __initdata = {
		{ 0, MPP_GPIO },	/* MCU_INTRn */
		{ 1, MPP_PMU }, /*PM_CORE_PG	   */
//		{ 2, MPP_GPIO},// Rabeeh - cleanup MPP_PMU }, 	/* PM_STBY_ONn	 */
		{ 2, MPP_PMU }, 	/* PM_STBY_ONn	 */
		{ 3, MPP_PMU }, 	/* PM_Switch_Pressed				   */
		{ 4, MPP_PMU }, 	/* M_CKE_MASK	  */
	  { 5, MPP_PMU },		/* PM_CPU_EN	*/
	  { 6, MPP_PMU },		/*M_RST_MASK		   */
	
		{ 7, MPP_PMU  },	/* LED_STBY_ONn 		 */
	
		{ 8, MPP_GPIO },	/* OFF_CTRL */
	
		{ 9,  MPP_PMU },	/* CORE_DVS_CTR   */
		{ 10, MPP_PMU },	/* CPU_DVS_CTRL 	*/
	
		{ 11, MPP_SATA_ACT },	/* LED_SATA_ONn 	*/
		{ 12, MPP_GPIO },//SDIO0 },	/* SD0_CDn */
		{ 13, MPP_GPIO },	/* WLAN_WAKEUP_HOST */
		{ 14, MPP_UART2 },	/* UA2_TXD			  */
		{ 15, MPP_UART2 },	/* UA2_RXD		 */
		{ 16, MPP_GPIO },	/* MPP_DDR_TERM 		   */
		{ 17, MPP_TWSI },	/* I2C_1_SDA	  */
		{ 18, MPP_LCD },	/* LCD0_PWM   */
		{ 19, MPP_GPIO },	/* IR sensor  */
		{ 20, MPP_SPI1 },	/* SPI_1_MISO	   */
		{ 21, MPP_SPI1 },	/* SPI_1_CSn		 */
		{ 22, MPP_SPI1 },	/* SPI_1_MOSI		*/
		{ 23, MPP_SPI1 },	/* SPI_1_SCK			 */
	
		{ 24, MPP_GPIO },	/* TSC_TI_INTRn 		 */
	  { 25, MPP_GPIO }, /* VGA_DET					  */
		{ 26, MPP_GPIO },	/* LCM_DCM			 */
		{ 27, MPP_GPIO },	/* INT_HDMI_ADI 			  */
		{ 28, MPP_GPIO },	/* LCM_BL_CTRL							   */
	  { 29, MPP_GPIO }, /* WLAN_BT_CLK_REQ							  */
	  { 30, MPP_GPIO }, /* MIC2_DET 							*/
	  { 31, MPP_GPIO }, /* I2S_CODEC_IRQ				*/
		{ 32, MPP_GPIO },	/* USB_DEV_DET						  */
		{ 33, MPP_GPIO },	/* HOST_WAKEUP_WLAN 							*/
		{ 34, MPP_GPIO },	/* MPP34						  */
	  { 35, MPP_GPIO }, /* MPP35						*/
	 
		{ 36, MPP_CAM },	/* Cam_SCLK 								  */
	  { 37, MPP_CAM },	/* Cam_SDA								 */
	 
		{ 38, MPP_GPIO },	/* HUB_RESETn						  */
	  { 39, MPP_GPIO }, /* MPP39						   */ 
	  
	  { 40, MPP_SDIO0 },	/* will configure MPPs 40-45 */
	
		{ 46, MPP_SDIO1 },	/* SD1 Group */
		{ 47, MPP_SDIO1 },	/* SD1 Group */
		{ 48, MPP_SDIO1 },	/* SD1 Group */
		{ 49, MPP_SDIO1 },	/* SD1 Group */
		{ 50, MPP_SDIO1 },	/* SD1 Group */
		{ 51, MPP_SDIO1 },	/* SD1 Group */

#if 1		
	{ 52, MPP_GPIO },	/* HDMI_PD  */
	{ 53, MPP_GPIO },	/* EXT_CLK_PD   */
	{ 54, MPP_GPIO },	/* MPP54    */
	{ 55, MPP_GPIO },	/* USB_PDn     */
	{ 56, MPP_GPIO },	/* PCIE_PDn   */
	{ 57, MPP_GPIO },	/* AU1 Group */
#else
	{ 52, MPP_AUDIO1 },	/* HDMI_PD  */
	{ 53, MPP_AUDIO1 },	/* EXT_CLK_PD   */
	{ 54, MPP_AUDIO1 },	/* MPP54    */
	{ 55, MPP_AUDIO1 },	/* USB_PDn     */
	{ 56, MPP_AUDIO1 },	/* PCIE_PDn   */
	{ 57, MPP_AUDIO1 },	/* AU1 Group */
#endif
	
		{ 58, MPP_SPI0 },	/* will configure MPPs 58-61 */
		{ 62, MPP_GPIO},//UART1 },	/* UA1_RXD	   */
		{ 63, MPP_UART1 },	/* UA1_TXD		*/
		{ -1 },
};
	 

/*****************************************************************************
 * GPIO
 *
 ***************************************************************************/


 static void dove_cubox_power_off(void)
{
	printk (KERN_INFO "System is always on\n");
	printk (KERN_INFO "Put system in lowest power state possible\n");
}

static void dove_cubox_gpio_init(u32 rev)
{
#if 1	
	orion_gpio_set_valid(19, 1);
	if (gpio_request(19, "IR_Receive" ) != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for GPIO for Expansionn\n");
	gpio_direction_input(19);
#endif	
  	  orion_gpio_set_valid(27, 1);
	orion_gpio_set_valid(34, 1);
	if (gpio_request(34, "MPP34n" ) != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for GPIO for Expansionn\n");
	gpio_direction_input(34);		/* GPIO for Expansion   */
  
    	  orion_gpio_set_valid(35, 1);
	if (gpio_request(35, "MPP35n" ) != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for GPIO for Expansionn\n");
	gpio_direction_input(35);		/* GPIO for Expansion   */
  
	orion_gpio_set_valid(1,1);
	if (gpio_request(1, "USB Enable") != 0)
		printk(KERN_ERR "Dove: failed to setup USB Enable bit");
	gpio_direction_output(1, 1);

	orion_gpio_set_valid(2,1);
	if (gpio_request(1, "USB Over Current") != 0)
		printk(KERN_ERR "Dove: failed to setup USB Over current detection");
	gpio_direction_input(2);

}



#ifdef CONFIG_PM
extern int global_dvs_enable;
/*****************************************************************************
 * POWER MANAGEMENT
 ****************************************************************************/
static int __init dove_cubox_pm_init(void)
{
	MV_PMU_INFO pmuInitInfo;
	u32 dev, rev;

	if (!machine_is_cubox())
		return 0;

	global_dvs_enable = dvs_enable;

	dove_pcie_id(&dev, &rev);

	pmuInitInfo.batFltMngDis = MV_FALSE;			/* Keep battery fault enabled */
	pmuInitInfo.exitOnBatFltDis = MV_FALSE;			/* Keep exit from STANDBY on battery fail enabled */
	pmuInitInfo.sigSelctor[0] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[1] = PMU_SIGNAL_NC;//PMU_SIGNAL_CPU_PWRGOOD;
	pmuInitInfo.sigSelctor[2] = PMU_SIGNAL_NC;//PMU_SIGNAL_SLP_PWRDWN;	/* STANDBY => 0: I/O off, 1: I/O on */
	pmuInitInfo.sigSelctor[3] = PMU_SIGNAL_NC;//PMU_SIGNAL_EXT0_WKUP;	/* power on push button */
	pmuInitInfo.sigSelctor[4] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[5] = PMU_SIGNAL_NC;//PMU_SIGNAL_CPU_PWRDWN;	/* DEEP-IdLE => 0: CPU off, 1: CPU on */

	pmuInitInfo.sigSelctor[6] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[7] = PMU_SIGNAL_NC;		/* Standby Led - inverted */
	pmuInitInfo.sigSelctor[8] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[9] = PMU_SIGNAL_SDI;//PMU_SIGNAL_SDI;//PMU_SIGNAL_NC;		/* CPU power good  - not used */
	pmuInitInfo.sigSelctor[10] = PMU_SIGNAL_SDI; //PMU_SIGNAL_NC; // Original PMU_SIGNAL_SDI;		/* Voltage regulator control */
	pmuInitInfo.sigSelctor[11] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[12] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[13] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[14] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[15] = PMU_SIGNAL_NC;
	pmuInitInfo.dvsDelay = 0x4200;				/* ~100us in 166MHz cc - delay for DVS change */
	if (rev >= DOVE_REV_X0) { /* For X0 and higher wait at least 150ms + spare */
		pmuInitInfo.standbyPwrDelay = 0x2000;		/* 250ms delay to wait for complete powerup */
		pmuInitInfo.ddrTermGpioNum = 16;		/* GPIO 16 used to disable terminations */
	} else {
		pmuInitInfo.standbyPwrDelay = 0x140;		/* 10ms delay after getting the power good indication */
		pmuInitInfo.ddrTermGpioNum = 6;			/* GPIO 6 used to disable terminations */
	}

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

__initcall(dove_cubox_pm_init);
#endif



/*****************************************************************************
 * Board Init
 ****************************************************************************/
static void __init dove_cubox_init(void)
{
	u32 dev, rev;

	/*
	 * Basic Dove setup. Needs to be called early.
	 */
	dove_init();
	dove_pcie_id(&dev, &rev);
	dove_mpp_conf(dove_cubox_mpp_modes);
	dove_cubox_gpio_init(rev);
	pm_power_off = dove_cubox_power_off;

	/* sdio card interrupt workaround using GPIOs */
	dove_sd_card_int_wa_setup(0);

	dove_rtc_init();
	pxa_init_dma_wins(&dove_mbus_dram_info);
	pxa_init_dma(16);
	dove_xor0_init();
	dove_xor1_init();
#ifdef CONFIG_MV_ETHERNET
	if (use_hal_giga || useHalDrivers)
		dove_mv_eth_init();
	else
#endif
	dove_ge00_init(&dove_cubox_ge00_data);
	dove_ehci0_init();
	dove_ehci1_init();
	/* ehci init functions access the usb port, only now it's safe to disable
	 * all clocks
	 */
#if 1
	ds_clks_disable_all(0, 0);
#else
	ds_clks_disable_all(1, 1); // Disable PCI-E 1 and 2 too
#endif
	// Rabeeh - limit to SATA gen 1
	dove_sata_init(&dove_cubox);
	dove_spi0_init(0);
	/* dove_spi1_init(1); */

	/* uart0 is the debug port, register it first so it will be */
	/* represented by device ttyS0, root filesystems usually expect the */
	/* console to be on that device */
	dove_uart0_init();

	dove_i2c_init();
	dove_i2c_exp_init(0);
	if (rev >= DOVE_REV_X0) {
		dove_i2c_exp_init(1);
	}
	dove_sdhci_cam_mbus_init();
	dove_sdio0_init();
	dove_cubox_clcd_init();
	dove_vmeta_init();
	dove_gpu_init();
	dove_cesa_init();
	dove_hwmon_init();
        dove_i2s_init(1, &i2s1_data);
	platform_device_register(&dit_data);
	i2c_register_board_info(0, dove_cubox_i2c_bus0_devs,
				ARRAY_SIZE(dove_cubox_i2c_bus0_devs));
	spi_register_board_info(dove_cubox_spi_flash_info,
				ARRAY_SIZE(dove_cubox_spi_flash_info));
#ifdef CONFIG_ANDROID_PMEM
        android_add_pmem("pmem", 0x02000000UL, 1, 0);
        android_add_pmem("pmem_adsp", 0x00400000UL, 0, 0);
#endif

#ifdef CONFIG_USB_ANDROID
	dove_udc_init();
	android_add_usb_devices();
#endif
	
	
}


MACHINE_START(CUBOX, "SolidRun CuBox Platform")
	.phys_io	= DOVE_SB_REGS_PHYS_BASE,
	.io_pg_offst	= ((DOVE_SB_REGS_VIRT_BASE) >> 18) & 0xfffc,
	.boot_params	= 0x00000100,
	.init_machine	= dove_cubox_init,
	.map_io		= dove_map_io,
	.init_irq	= dove_init_irq,
	.timer		= &dove_timer,
/* reserve memory for VMETA and GPU */
	.fixup		= dove_tag_fixup_mem32,
MACHINE_END
