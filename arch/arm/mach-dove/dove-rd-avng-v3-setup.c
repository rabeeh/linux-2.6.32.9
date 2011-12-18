/* * arch/arm/mach-dove/dove-rd-avng-setup.c
 *
 * Marvell Dove MV88F6781-RD Avengers Mobile Internet Device Board Setup
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
#include "idt5v49ee503.h"
#include <linux/adi9889_i2c.h>

#define DOVE_AVNG_POWER_OFF_GPIO	(8)
#define DOVE_AVNG_V3_BMA_INT	(35)

extern int __init pxa_init_dma_wins(struct mbus_dram_target_info *dram);

static unsigned int use_hal_giga = 0;
#ifdef CONFIG_MV643XX_ETH
module_param(use_hal_giga, uint, 0);
MODULE_PARM_DESC(use_hal_giga, "Use the HAL giga driver");
#endif

static unsigned int standby_fix = 1;
module_param(standby_fix, uint, 0);
MODULE_PARM_DESC(standby_fix, "if 1 then CKE and MRESET are connected to MPP4 and MPP6");

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
static struct dovefb_mach_info dove_rd_avng_v3_lcd0_dmi = {
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

static struct dovefb_mach_info dove_rd_avng_v3_lcd0_vid_dmi = {
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

static struct dovefb_mach_info dove_rd_avng_v3_lcd1_dmi = {
	.id_gfx			= "GFX Layer 1",
	.id_ovly		= "Video Layer 1",
	.clk_src		= MRVL_PLL_CLK,
	.clk_name		= "accurate_LCDCLK",
//	.num_modes		= ARRAY_SIZE(video_modes),
//	.modes			= video_modes,
	.pix_fmt		= PIX_FMT_RGB888PACK,
	.io_pin_allocation	= IOPAD_DUMB24,
	.panel_rgb_type		= DUMB24_RGB888_0,
	.panel_rgb_reverse_lanes= 0,
	.gpio_output_data	= 0,
	.gpio_output_mask	= 0,
#ifndef CONFIG_FB_DOVE_CLCD1_I2C_DEFAULT_SETTING
	.ddc_i2c_adapter	= CONFIG_FB_DOVE_CLCD1_I2C_CHANNEL,
	.ddc_i2c_address	= CONFIG_FB_DOVE_CLCD1_I2C_ADDRESS,
#else
	.ddc_i2c_adapter	= 1	,
	.ddc_i2c_address	= 0x50,
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

static struct dovefb_mach_info dove_rd_avng_v3_lcd1_vid_dmi = {
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

void __init dove_rd_avng_v3_clcd_init(void) {
#ifdef CONFIG_FB_DOVE
	clcd_platform_init(&dove_rd_avng_v3_lcd0_dmi, &dove_rd_avng_v3_lcd0_vid_dmi,
			   &dove_rd_avng_v3_lcd1_dmi, &dove_rd_avng_v3_lcd1_vid_dmi,
			   &dove_rd_avng_v3_backlight_data);

#endif /* CONFIG_FB_DOVE */
}


//Touch
#define IRQ0_TSC2007 24
static int ts_get_pendown_state(void)
{
	int val = 0;

    	val = gpio_get_value(IRQ0_TSC2007);
	return val ? 0 : 1;
}

static int ts_init(void)
{

	return 0;
}

struct tsc2007_platform_data tsc2007_info = {
	.model			= 2007,
	.x_plate_ohms		= 180,
	.get_pendown_state	= ts_get_pendown_state,
	.init_platform_hw	= ts_init,
};

static struct i2c_board_info ts_i2c_clients = {
	I2C_BOARD_INFO("tsc2007", 0x48),
	.type		= "tsc2007",
	.platform_data	= &tsc2007_info,
	.irq		= (IRQ_DOVE_GPIO_START+IRQ0_TSC2007),
};


struct adi9889_i2c_platform_data avng_v3_adi9889_i2c_conf ={
	.audio_format = I2S_FORMAT,

};

/*****************************************************************************
 * I2C devices:
 * 	ALC5630 codec, address 0x
 * 	Battery charger, address 0x??
 * 	G-Sensor, address 0x??
 * 	MCU PIC-16F887, address 0x??
 ****************************************************************************/
static struct i2c_board_info __initdata dove_rd_avng_v3_i2c_bus0_devs[] = {
	{
		I2C_BOARD_INFO("rt5630", 0x1f),
	},
	{
		I2C_BOARD_INFO("rt5623", 0x1a),
	},
#ifdef CONFIG_MV_EC_KEY
	{
		I2C_BOARD_INFO("ec_key_i2c", 0x2c),
		.irq = IRQ_DOVE_GPIO_START,
	},
#endif	
#ifdef CONFIG_BMA020
	{
		I2C_BOARD_INFO("bma020", 0x38),
		.irq = IRQ_DOVE_GPIO_START + DOVE_AVNG_V3_BMA_INT,
	},
#endif
#ifdef CONFIG_CH7025_COMPOSITE
	{
		I2C_BOARD_INFO("ch7025_i2c",0x76),
	},
#endif
#if 0
	{
		I2C_BOARD_INFO("pic-16f887", 0x??),
	},
#endif
#ifdef CONFIG_BATTERY_MCU
	{
		I2C_BOARD_INFO("power_mcu", 0x2C),
	},
#endif
};

static struct i2c_board_info __initdata dove_rd_avng_v3_i2c_bus1_devs[] = {
#ifdef CONFIG_ADI9889
	{
		I2C_BOARD_INFO("adi9889_i2c", 0x39),
		.platform_data = &avng_v3_adi9889_i2c_conf,			
	},
	{
		I2C_BOARD_INFO("adi9889_edid_i2c", 0x3F),
		.platform_data = &avng_v3_adi9889_i2c_conf,			
	},
#endif
#ifdef CONFIG_TOUCHSCREEN_TSC2007
	{
		I2C_BOARD_INFO("tsc2007",0x48),
		.irq = (IRQ_DOVE_GPIO_START+IRQ0_TSC2007) ,
		.type		= "tsc2007",
		.platform_data = &tsc2007_info,
	},
#endif

};

/*****************************************************************************
 * IDT clock
 ****************************************************************************/
static struct idt_data dove_uc2_idt_data = {
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
	.platform_data = &dove_uc2_idt_data,
};

/*****************************************************************************
 * PCI
 ****************************************************************************/
static int __init dove_rd_avng_v3_pci_init(void)
{

	if (machine_is_dove_avng_v3())
			dove_pcie_init(1, 0);

	return 0;
}

subsys_initcall(dove_rd_avng_v3_pci_init);


/*****************************************************************************
 * Camera
 ****************************************************************************/
 /*
static struct cafe_cam_platform_data dove_cafe_cam_data = {
	.power_down 	= 2, //CTL0 connected to the sensor power down
	.reset		= 1, //CTL1 connected to the sensor reset
	.numbered_i2c_bus = 1,
	.i2c_bus_id	= 3,
};

static int __init dove_rd_avng_cam_init(void)
{
	if (machine_is_dove_rd_avng())
		dove_cam_init(&dove_cafe_cam_data);

	return 0;
}

late_initcall(dove_rd_avng_cam_init);
*/

/*****************************************************************************
 * Audio I2S
 ****************************************************************************/
static struct orion_i2s_platform_data i2s0_data = {
	.i2s_play	= 1,
	.i2s_rec	= 1,
};


/*****************************************************************************
 * Ethernet
 ****************************************************************************/
static struct mv643xx_eth_platform_data dove_rd_avng_v3_ge00_data = {
	.phy_addr	= MV643XX_ETH_PHY_ADDR_DEFAULT,
};


/*****************************************************************************
 * SATA
 ****************************************************************************/
static struct mv_sata_platform_data dove_rd_avng_v3_sata_data = {
        .n_ports        = 1,
};


/*****************************************************************************
 * SPI Devices:
 *     SPI0: 4M Flash MX25L3205D
 ****************************************************************************/
static const struct flash_platform_data dove_rd_avng_v3_spi_flash_data = {
	.type           = "mx25l3205d",
};

static struct spi_board_info __initdata dove_rd_avng_v3_spi_flash_info[] = {
	{
		.modalias       = "m25p80",
		.platform_data  = &dove_rd_avng_v3_spi_flash_data,
		.irq            = -1,
		.max_speed_hz   = 20000000,
		.bus_num        = 0,
		.chip_select    = 0,
	},
};

/*****************************************************************************
 * NAND
 ****************************************************************************/
static struct mtd_partition partition_dove[] = {
	{ .name		= "Root",
	  .offset	= 0,
	  .size		= MTDPART_SIZ_FULL },
};
static u64 nfc_dmamask = DMA_BIT_MASK(32);
static struct nfc_platform_data dove_rd_avng_v3_nfc_hal_data = {
	.nfc_width      = 8,			/* Assume non ganaged mode by default */
	.num_devs       = 1,			/* Assume non ganaged mode by defautl */
	.num_cs         = 1,			/* K9LBG08U0D */
	.use_dma	= 1,
	.ecc_type	= MV_NFC_ECC_BCH_1K,	/* 8bit ECC required by K9LBG08U0D */
	.parts = partition_dove,
	.nr_parts = ARRAY_SIZE(partition_dove)
};

static struct nfc_platform_data dove_rd_avng_v3_nfc_data = {
	.nfc_width      = 8,
	.num_devs       = 1,
	.num_cs         = 1,
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
		.platform_data		= &dove_rd_avng_v3_nfc_data,
	},
	.resource	= dove_nfc_resources,
	.num_resources	= ARRAY_SIZE(dove_nfc_resources),
};

static void __init dove_rd_avng_v3_nfc_init(void)
{
	/* Check if HAL driver is intended */
	if(useHalDrivers || useNandHal) {
		dove_nfc.name = "orion-nfc-hal";

		/* Check for ganaged mode */
		if (strncmp(useNandHal, "ganged", 6) == 0) {
			dove_rd_avng_v3_nfc_hal_data.nfc_width = 16;
			dove_rd_avng_v3_nfc_hal_data.num_devs = 2;
			useNandHal += 7;
		}

		/* Check for ECC type directive */
		if (strcmp(useNandHal, "4bitecc") == 0) {
			dove_rd_avng_v3_nfc_hal_data.ecc_type = MV_NFC_ECC_BCH_2K;
		} else if (strcmp(useNandHal, "8bitecc") == 0) {
			dove_rd_avng_v3_nfc_hal_data.ecc_type = MV_NFC_ECC_BCH_1K;
		} else if (strcmp(useNandHal, "12bitecc") == 0) {
			dove_rd_avng_v3_nfc_hal_data.ecc_type = MV_NFC_ECC_BCH_704B;
		} else if (strcmp(useNandHal, "16bitecc") == 0) {
			dove_rd_avng_v3_nfc_hal_data.ecc_type = MV_NFC_ECC_BCH_512B;
		}

		dove_rd_avng_v3_nfc_hal_data.tclk = dove_tclk_get();
		dove_nfc.dev.platform_data = &dove_rd_avng_v3_nfc_hal_data;

	} else {  /* NON HAL driver is used */
		dove_rd_avng_v3_nfc_data.tclk = dove_tclk_get();
	}

	platform_device_register(&dove_nfc);
}


/*****************************************************************************
 * MPP
 ****************************************************************************/
static struct dove_mpp_mode dove_rd_avng_v3_a0_mpp_modes[] __initdata = {
		{ 0, MPP_GPIO },	/* MCU_INTRn */
		{ 1, MPP_PMU }, /*PM_CORE_PG	   */
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
		{ 12, MPP_SDIO1 },	/* SD1_CDn */
		{ 13, MPP_GPIO },	/* WLAN_WAKEUP_HOST */
		{ 14, MPP_UART2 },	/* UA2_TXD			  */
		{ 15, MPP_UART2 },	/* UA2_RXD		 */
		{ 16, MPP_GPIO },	/* MPP_DDR_TERM 		   */
		{ 17, MPP_TWSI },	/* I2C_1_SDA	  */
	
		{ 18, MPP_LCD },	/* LCD0_PWM   */
		{ 19, MPP_TWSI },	/* I2C_1_SCK  */
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

		
	{ 52, MPP_GPIO },	/* HDMI_PD  */
	{ 53, MPP_GPIO },	/* EXT_CLK_PD   */
	{ 54, MPP_GPIO },	/* MPP54    */
	{ 55, MPP_GPIO },	/* USB_PDn     */
	{ 56, MPP_GPIO },	/* PCIE_PDn   */
	{ 57, MPP_GPIO },	/* AU1 Group */

//	{ 52, MPP_GPIO_AUDIO1 },	/* HDMI_PD  */
//	{ 53, MPP_GPIO_AUDIO1 },	/* EXT_CLK_PD   */
	//{ 54, MPP_GPIO_AUDIO1 },	/* MPP54    */
//	{ 55, MPP_GPIO_AUDIO1 },	/* USB_PDn     */
//	{ 56, MPP_GPIO_AUDIO1 },	/* PCIE_PDn   */
//	{ 57, MPP_GPIO_AUDIO1 },	/* AU1 Group */p */
	
		{ 58, MPP_SPI0 },	/* will configure MPPs 58-61 */
		{ 62, MPP_UART1 },	/* UA1_RXD	   */
		{ 63, MPP_UART1 },	/* UA1_TXD		*/
		{ -1 },
};
	 

/*****************************************************************************
 * GPIO
 *
 ***************************************************************************/


 static void dove_rd_avng_v3_power_off(void)
{
	if (gpio_direction_output(DOVE_AVNG_POWER_OFF_GPIO, 0) != 0) {
 		printk(KERN_ERR "%s failed to set power off output pin %d\n",
		       __func__, DOVE_AVNG_POWER_OFF_GPIO);
	}
}

static void dove_rd_avng_v3_gpio_init(u32 rev)
{
	int pin;

	printk(KERN_ERR"dove_rd_avng_v3_gpio_init ENTER!\n");
	orion_gpio_set_valid(0, 1);
	if (gpio_request(0, "MCU_INTRn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for MCU_INTRn\n");
	gpio_direction_input(0);	/* MCU interrupt */
	
	if (rev >= DOVE_REV_X0) {
		pin = 16;
	} else {
		pin = 6;
	}
	
	 orion_gpio_set_valid(39, 1);
	if (gpio_request(39, "MPP39n" ) != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for GPIO for Expansionn\n");
	gpio_direction_output(39,0);	/*PCI-E reset */
	
	orion_gpio_set_valid(pin, 1);
	if (gpio_request(pin, "MPP_DDR_TERM") != 0)
	printk(KERN_ERR "Dove: failed to setup GPIO for MPP_DDR_TERM\n");
	gpio_direction_output(pin, 1);	/* Enable DDR 1.8v */

	orion_gpio_set_valid(DOVE_AVNG_POWER_OFF_GPIO, 1);
	if (gpio_request(DOVE_AVNG_POWER_OFF_GPIO, "OFF_CTRL") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for OFF_CTRL\n");
	gpio_direction_output(DOVE_AVNG_POWER_OFF_GPIO, 1);	/* Power off */
#if 1
	 orion_gpio_set_valid(24, 1);
	if (gpio_request(24, "TSC_TI_INTRn ") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for TSC2007 interruptn\n");
	gpio_direction_input(24);	/* TSC2007 interruptnn */
 #endif 
   orion_gpio_set_valid(25, 1);
	if (gpio_request(25, "VGA_DETn ") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for VGA plug-in detectionn\n");
	gpio_direction_input(25);	/* VGA plug-in detectionn */
	
	  orion_gpio_set_valid(26, 1);
	if (gpio_request(26, "LCM_DCMn ") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for LCD panel VDD enablen\n");
	gpio_direction_output(26,0);	/* LCD panel VDD enablen */
  
  	  orion_gpio_set_valid(27, 1);
	if (gpio_request(27, "INT_HDMI_ADIn ") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for ADI 9389B interruptn\n");
	gpio_direction_input(27);		/* ADI 9389B interrupt  */
 
 	  orion_gpio_set_valid(28, 1);
	if (gpio_request(28, "LCM_BL_CTRLn ") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for LCD backlight voltage enablen\n");
	gpio_direction_output(28,0);	/* LCD backlight voltage enablen */
  
   	  orion_gpio_set_valid(29, 1);
	if (gpio_request(29, "WLAN_BT_CLK_REQn ") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for Module Sleep indicationn\n");
	gpio_direction_input(29);		/* Module Sleep indication   */
	
	  	  orion_gpio_set_valid(30, 1);
	if (gpio_request(30, "MIC2_DETn ") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for Microphone plug-in detectionn\n");
	gpio_direction_input(30);		/* Microphone plug-in detection   */
	
	  	  orion_gpio_set_valid(31, 1);
	if (gpio_request(31, "I2S_CODEC_IRQn ") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for ALC6530 interruptn\n");
	gpio_direction_input(31);		/* ALC6530 interrupt    */
	
	  	  orion_gpio_set_valid(32, 1);
	if (gpio_request(32, "USB_DEV_DETn" ) != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for USB client cable plug-in detectionn\n");
	gpio_direction_input(32);		/* USB client cable plug-in detection   */
	
	   orion_gpio_set_valid(33, 1);
	if (gpio_request(33, "HOST_WAKEUP_WLANn ") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for Host wakeup AW-NH387 modulen\n");
	gpio_direction_output(33,0);	/* Host wakeup AW-NH387 module  */
	
		  	  orion_gpio_set_valid(34, 1);
	if (gpio_request(34, "MPP34n" ) != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for GPIO for Expansionn\n");
	gpio_direction_input(34);		/* GPIO for Expansion   */
  
    	  orion_gpio_set_valid(35, 1);
	if (gpio_request(35, "MPP35n" ) != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for GPIO for Expansionn\n");
	gpio_direction_input(35);		/* GPIO for Expansion   */
  
  orion_gpio_set_valid(38, 1);
	if (gpio_request(38, "HUB_RESETn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for HUB_RESETn\n");
	gpio_direction_output(38, 0);	/* HUB_RESETn */
	


  orion_gpio_set_valid(52, 1);
	if (gpio_request(52, "HDMI_PD") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for HDMI Power Downn\n");
	gpio_direction_output(52,1);	/* HDMI Power Downn */
	
	orion_gpio_set_valid(53, 1);
	if (gpio_request(53, "EXT_CLK_PDn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for Power Down for LCD Clock Generatorn\n");
	gpio_direction_output(53, 0);	/* Power Down for LCD Clock Generatorn */

   	  orion_gpio_set_valid(54, 1);
	if (gpio_request(54, "MPP54n" ) != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for GPIO for Expansionn\n");
	gpio_direction_input(54);		/* GPIO for Expansion   */

  orion_gpio_set_valid(55, 1);
	if (gpio_request(55, "USB_PDn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for USB HUB Power Downn\n");
	gpio_direction_output(55, 1);	/* USB HUB Power Downn */
	
	orion_gpio_set_valid(56, 1);
	if (gpio_request(56, "PCIE_PDn") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for PCI-E Power Down\n");
	gpio_direction_output(56, 1);	/* PCI-E Power Down */

	orion_gpio_set_valid(57, 1);
	if (gpio_request(57, "SPDIF_NOUSE") != 0)
		printk(KERN_ERR "Dove: failed to setup GPIO for SPDIF_NOUSEn");
	gpio_direction_output(57, 1);	/* SPDIF_NOUSE*/
	
	orion_gpio_set_valid(13, 1);
	if (gpio_request(13, "WLAN_WAKEUP_HOST") != 0)
			printk(KERN_ERR "Dove: failed to setup GPIO for WLAN_WAKEUP_HOST\n");
		gpio_direction_input(13);

	gpio_direction_output(55, 1);	/* USB HUB Power Downn */


	msleep(200);
	gpio_direction_output(39,1);	/*PCI-E reset */		
	gpio_direction_output(38,1);	/* HUB_RESETn */	
	gpio_direction_output(52,0);	/* HDMI Power Downn */	
	msleep(200);	
 	printk(KERN_ERR"dove_rd_avng_v3_gpio_init exit!\n");

}



#ifdef CONFIG_PM
extern int global_dvs_enable;
/*****************************************************************************
 * POWER MANAGEMENT
 ****************************************************************************/
static int __init dove_rd_avng_v3_pm_init(void)
{
	MV_PMU_INFO pmuInitInfo;
	u32 dev, rev;

	if (!machine_is_dove_avng_v3())
		return 0;

	global_dvs_enable = dvs_enable;

	dove_pcie_id(&dev, &rev);

	pmuInitInfo.batFltMngDis = MV_FALSE;			/* Keep battery fault enabled */
	pmuInitInfo.exitOnBatFltDis = MV_FALSE;			/* Keep exit from STANDBY on battery fail enabled */
	pmuInitInfo.sigSelctor[0] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[1] = PMU_SIGNAL_CPU_PWRGOOD;
	pmuInitInfo.sigSelctor[2] = PMU_SIGNAL_SLP_PWRDWN;	/* STANDBY => 0: I/O off, 1: I/O on */
	pmuInitInfo.sigSelctor[3] = PMU_SIGNAL_EXT0_WKUP;	/* power on push button */
	if (rev >= DOVE_REV_X0) { /* For X0 and higher Power Good indication is not needed */
		if (standby_fix)
			pmuInitInfo.sigSelctor[4] = PMU_SIGNAL_CKE_OVRID;	/* CKE controlled by Dove */
		else
			pmuInitInfo.sigSelctor[4] = PMU_SIGNAL_NC;
	}
	else
		pmuInitInfo.sigSelctor[4] = PMU_SIGNAL_CPU_PWRGOOD;	/* CORE power good used as Standby PG */

	pmuInitInfo.sigSelctor[5] = PMU_SIGNAL_CPU_PWRDWN;	/* DEEP-IdLE => 0: CPU off, 1: CPU on */

	if ((rev >= DOVE_REV_X0) && (standby_fix)) /* For boards with X0 we use MPP6 as MRESET */
		pmuInitInfo.sigSelctor[6] = PMU_SIGNAL_MRESET_OVRID;		/* M_RESET is pulled up - always HI */
	else
		pmuInitInfo.sigSelctor[6] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[7] = PMU_SIGNAL_1;		/* Standby Led - inverted */
	pmuInitInfo.sigSelctor[8] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[9] = PMU_SIGNAL_NC;		/* CPU power good  - not used */
	pmuInitInfo.sigSelctor[10] = PMU_SIGNAL_SDI;		/* Voltage regulator control */
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

__initcall(dove_rd_avng_v3_pm_init);
#endif



#ifdef CONFIG_BATTERY_MCU
void __init dove_battery_init_v3(void)
{
	platform_device_register_simple("battery", 0, NULL, 0);
}
#endif

/*
#ifdef CONFIG_ANDROID_PMEM
#include <linux/dma-mapping.h>
#include <linux/android_pmem.h>

void android_add_pmem(char *name, size_t size, int no_allocator, int cached)
{
        struct platform_device *android_pmem_device;
        struct android_pmem_platform_data *android_pmem_pdata;
        struct page *page;
        unsigned long addr, tmp;
        static int id;
        unsigned long paddr = 0;

        android_pmem_device = kzalloc(sizeof(struct platform_device), GFP_KERNEL);
        if(android_pmem_device == NULL)
                return ;

        android_pmem_pdata = kzalloc(sizeof(struct android_pmem_platform_data), GFP_KERNEL);
        if(android_pmem_pdata == NULL) {
                kfree(android_pmem_device);
                return ;
        }

        page = alloc_pages(GFP_KERNEL, get_order(size));
        if (page == NULL)
                return ;

        addr = (unsigned long)page_address(page);
        paddr = virt_to_phys((void *)addr);
        tmp = size;
        dma_cache_maint(addr, size, DMA_FROM_DEVICE);
        while(tmp > 0) {
                SetPageReserved(virt_to_page(addr));
                addr += PAGE_SIZE;
                tmp -= PAGE_SIZE;
        }
        android_pmem_pdata->name = name;
        android_pmem_pdata->start = paddr;
        android_pmem_pdata->size = size;
        android_pmem_pdata->no_allocator = no_allocator ;
        android_pmem_pdata->cached = cached;

        android_pmem_device->name = "android_pmem";
        android_pmem_device->id = id++;
        android_pmem_device->dev.platform_data = android_pmem_pdata;

        platform_device_register(android_pmem_device);
}
#endif
*/

/*****************************************************************************
 * Board Init
 ****************************************************************************/
static void __init dove_rd_avng_v3_init(void)
{
		u32 dev, rev;
	
		/*
		 * Basic Dove setup. Needs to be called early.
		 */
		dove_init();
		dove_pcie_id(&dev, &rev);
	
	  dove_mpp_conf(dove_rd_avng_v3_a0_mpp_modes);
	
	  dove_rd_avng_v3_gpio_init(rev);
	
	  pm_power_off = dove_rd_avng_v3_power_off;

		/* sdio card interrupt workaround using GPIOs */
		dove_sd_card_int_wa_setup(0);
		dove_sd_card_int_wa_setup(1);
	
	
		/* Initialize AC'97 related regs. */
		//dove_avng_DCO_CLK_init();
	
		dove_rtc_init();
		pxa_init_dma_wins(&dove_mbus_dram_info);
		pxa_init_dma(16);
#ifdef CONFIG_MV_HAL_DRIVERS_SUPPORT
		if (useHalDrivers || useNandHal) {
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
		if (use_hal_giga || useHalDrivers)
			dove_mv_eth_init();
		else
#endif
		dove_ge00_init(&dove_rd_avng_v3_ge00_data);
		dove_ehci0_init();
		dove_ehci1_init();
		/* ehci init functions access the usb port, only now it's safe to disable
		 * all clocks
		 */
		ds_clks_disable_all(0, 0);
		dove_sata_init(&dove_rd_avng_v3_sata_data);
		dove_spi0_init(0);
		/* dove_spi1_init(1); */
	
		/* uart0 is the debug port, register it first so it will be */
		/* represented by device ttyS0, root filesystems usually expect the */
		/* console to be on that device */
		dove_uart0_init();
		dove_uart1_init();
		dove_uart2_init();
	
		dove_i2c_init();
		dove_i2c_exp_init(0);
		if (rev >= DOVE_REV_X0) {
			dove_i2c_exp_init(1);
		}
		dove_sdhci_cam_mbus_init();
		dove_sdio0_init();
		dove_sdio1_init();
		//dove_rd_avng_v3_nfc_init();
		dove_rd_avng_v3_clcd_init();
		dove_vmeta_init();
		dove_gpu_init();
		dove_cesa_init();
		dove_hwmon_init();
	
		dove_i2s_init(0, &i2s0_data);
	
		
		i2c_register_board_info(0, dove_rd_avng_v3_i2c_bus0_devs,
					ARRAY_SIZE(dove_rd_avng_v3_i2c_bus0_devs));
		
		i2c_register_board_info(1, dove_rd_avng_v3_i2c_bus1_devs,
					ARRAY_SIZE(dove_rd_avng_v3_i2c_bus1_devs));
			

	if (rev >= DOVE_REV_A0)
		i2c_register_board_info(0, &idt, 1);
	
		spi_register_board_info(dove_rd_avng_v3_spi_flash_info,
					ARRAY_SIZE(dove_rd_avng_v3_spi_flash_info));
#ifdef CONFIG_BATTERY_MCU
		dove_battery_init_v3();
#endif
#ifdef CONFIG_ANDROID_PMEM
        android_add_pmem("pmem", 0x02000000UL, 1, 0);
        android_add_pmem("pmem_adsp", 0x00400000UL, 0, 0);
#endif

#ifdef CONFIG_USB_ANDROID
		dove_udc_init();
		android_add_usb_devices();
#endif
	
	
}


MACHINE_START(DOVE_RD_AVNG_V3, "Marvell MV88AP510 Avengers v3.0 MID Board")
	.phys_io	= DOVE_SB_REGS_PHYS_BASE,
	.io_pg_offst	= ((DOVE_SB_REGS_VIRT_BASE) >> 18) & 0xfffc,
	.boot_params	= 0x00000100,
	.init_machine	= dove_rd_avng_v3_init,
	.map_io		= dove_map_io,
	.init_irq	= dove_init_irq,
	.timer		= &dove_timer,
/* reserve memory for VMETA and GPU */
	.fixup		= dove_tag_fixup_mem32,
MACHINE_END
