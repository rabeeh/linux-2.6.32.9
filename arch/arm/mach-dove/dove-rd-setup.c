/*
 * arch/arm/mach-dove/dove-rd-setup.c
 *
 * Marvell Dove MV88F6781-RD Board Setup
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
#include <linux/spi/ads7846.h>
#include <video/dovefb.h>
#include <plat/i2s-orion.h>
#include <asm/mach-types.h>
#include <asm/gpio.h>
#include <asm/mach/arch.h>
#include <mach/dove.h>
#include <asm/hardware/pxa-dma.h>
#include <plat/orion_nfc.h>
#include "common.h"
#include "mpp.h"
#include "clock.h"
#include "dove-front-panel-common.h"
#include <mach/pm.h>
#include "pmu/mvPmu.h"
#include "pmu/mvPmuRegs.h"

#define DOVE_RD_WAKEUP_GPIO	(3)

extern int __init pxa_init_dma_wins(struct mbus_dram_target_info *dram);
#ifdef CONFIG_MV_ETHERNET
#else
static struct mv643xx_eth_platform_data dove_rd_ge00_data = {
	.phy_addr	= MV643XX_ETH_PHY_ADDR_DEFAULT,
};
#endif
static struct orion_i2s_platform_data i2s0_data = {
	.i2s_play	= 1,
	.i2s_rec	= 1,
	.spdif_play	= 0, /* RD doesn't have SPDIF ports */
	.spdif_rec	= 0,
};

/*****************************************************************************
 * SATA
 ****************************************************************************/
static struct mv_sata_platform_data dove_rd_sata_data = {
	.n_ports	= 1,
};

/*****************************************************************************
 * SPI Devices:
 * 	SPI0: 4M Flash ST-M25P32-VMF6P
 ****************************************************************************/
static const struct flash_platform_data dove_rd_spi_flash_data = {
	.type		= "m25p32",
};

static struct spi_board_info __initdata dove_rd_spi_flash_info[] = {
	{
		.modalias       = "m25p80",
		.platform_data  = &dove_rd_spi_flash_data,
		.irq            = -1,
		.max_speed_hz   = 20000000,
		.bus_num        = 0,
		.chip_select    = 0,
	},
};

/*****************************************************************************
 * I2C devices:
 * 	Audio codec CS42L51-CNZ, address 0x94
 * 	eeprom CAT24C64LI-G, address 0xa0
 * 	VGA connector, address ??
 * 	LCD board, address ??
 * 	Battery, address ??
 * 	Charger, address ??
 ****************************************************************************/
static struct i2c_board_info __initdata dove_rd_i2c_devs[] = {
	{ I2C_BOARD_INFO("cs42l51", 0x4A) },
	{ I2C_BOARD_INFO("bq2084_i2c", 0x0B) },		/* Battery Gauge */
	{ I2C_BOARD_INFO("max1535_i2c", 0x09) },	/* SMBus Charger */
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
	  .size		= 2043 * SZ_1M },
};
static u64 nfc_dmamask = DMA_BIT_MASK(32);
static struct nfc_platform_data dove_rd_nfc_data = {
	.nfc_width	= 8,
	.use_dma	= 1,
	.use_ecc	= 1,
	.use_bch	= 1,
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
		.platform_data		= &dove_rd_nfc_data,
	},
	.resource	= dove_nfc_resources,
	.num_resources	= ARRAY_SIZE(dove_nfc_resources),
};

static void __init dove_rd_nfc_init(void)
{
	dove_rd_nfc_data.tclk = dove_tclk_get();
	platform_device_register(&dove_nfc);
}


/*****************************************************************************
 * MPP
 ****************************************************************************/
static struct dove_mpp_mode dove_rd_mpp_modes[] __initdata = {
	{  1, MPP_PMU },		/* CPU power control */
	{  3, MPP_PMU },		/* Wakeup - power button */
	{  4, MPP_PMU },		/* CPU power good inication */
	{  7, MPP_PMU }, 		/* Standby Led */
	{ 10, MPP_PMU }, 		/* DVS SDI control */
	{ 12, MPP_GPIO },
	{ 13, MPP_GPIO },
	{ 14, MPP_GPIO },
	{ 15, MPP_GPIO },
	{ 18, MPP_GPIO },
//	{ 20, MPP_LCD0_SPI},
//	{ 21, MPP_LCD0_SPI},
//	{ 22, MPP_LCD0_SPI},
//	{ 23, MPP_LCD0_SPI},
	{ 24, MPP_CAM },		/* CAM Group */
	{ 40, MPP_SDIO0 },		/* SDIO0 Group */
	{ 46, MPP_SDIO1 },		/* SDIO1 Group */
	{ 52, MPP_GPIO },		/* AU1 Group to GPIO */
	{ -1 },
};

void __init dove_battery_init(void)
{
	platform_device_register_simple("bq2084-battery", 0, NULL, 0);
}

#ifdef CONFIG_PM
/*****************************************************************************
 * POWER MANAGEMENT
 ****************************************************************************/
static int __init dove_rd_pm_init(void)
{
	MV_PMU_INFO pmuInitInfo;	

	if (!machine_is_dove_rd())
		return 0;

	pmuInitInfo.batFltMngDis = MV_FALSE;			/* Keep battery fault enabled */
	pmuInitInfo.exitOnBatFltDis = MV_FALSE;			/* Keep exit from STANDBY on battery fail enabled */
	pmuInitInfo.sigSelctor[0] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[1] = PMU_SIGNAL_CPU_PWRDWN;	/* DEEP-IdLE => 0: CPU off, 1: CPU on */
	pmuInitInfo.sigSelctor[2] = PMU_SIGNAL_NC;		/* STANDBY => Not used in Z0 */
	pmuInitInfo.sigSelctor[3] = PMU_SIGNAL_EXT0_WKUP;	/* power on push button */
	pmuInitInfo.sigSelctor[4] = PMU_SIGNAL_CPU_PWRGOOD;	/* CORE power good used as Standby PG */
	pmuInitInfo.sigSelctor[5] = PMU_SIGNAL_NC;		/* NON PMU in Z0 Board */
	pmuInitInfo.sigSelctor[6] = PMU_SIGNAL_NC;		/* Charger interrupt - not used */
	pmuInitInfo.sigSelctor[7] = PMU_SIGNAL_1;		/* Standby Led - inverted */
	pmuInitInfo.sigSelctor[8] = PMU_SIGNAL_NC;		/* PM OFF Control - used as GPIO */
	pmuInitInfo.sigSelctor[9] = PMU_SIGNAL_NC;		/* CPU power good  - not used */
	pmuInitInfo.sigSelctor[10] = PMU_SIGNAL_SDI;		/* Voltage regulator control */
	pmuInitInfo.sigSelctor[11] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[12] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[13] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[14] = PMU_SIGNAL_NC;
	pmuInitInfo.sigSelctor[15] = PMU_SIGNAL_NC;
	pmuInitInfo.dvsDelay = 0x4200;				/* ~100us in 166MHz cc - delay for DVS change */
	pmuInitInfo.ddrTermGpioNum = -1;			/* No GPIO is used to disable terminations */

	/* Initialize the PMU HAL */
	if (mvPmuInit(&pmuInitInfo) != MV_OK)
	{
		printk(KERN_NOTICE "Failed to initialise the PMU!\n");
		return 0;
	}

	/* Configure wakeup events */
	mvPmuWakeupEventSet(PMU_STBY_WKUP_CTRL_EXT0_FALL | PMU_STBY_WKUP_CTRL_RTC_MASK);

	/* Register the PM operation in the Linux stack */
	dove_pm_register();

	return 0;
}

__initcall(dove_rd_pm_init);
#endif

/*****************************************************************************
 * Board Init
 ****************************************************************************/
static void __init dove_rd_init(void)
{
	/*
	 * Basic Dove setup (needs to be called early).
	 */
	dove_init();

	if (dove_fp_ts_gpio_setup() != 0)
		return;

	/*
	 * Mux pins and GPIO pins setup
	 */
	dove_mpp_conf(dove_rd_mpp_modes);

        /* the (SW1) button is for use as a "wakeup" button */
//	dove_wakeup_button_setup(DOVE_RD_WAKEUP_GPIO);

	/* card interrupt workaround using GPIOs */
//	dove_sd_card_int_wa_setup(0);
//	dove_sd_card_int_wa_setup(1);

	/*
	 * On-chip device registration
	 */
	dove_rtc_init();
	pxa_init_dma_wins(&dove_mbus_dram_info);
	pxa_init_dma(16);
	dove_xor0_init();
	dove_xor1_init();
	dove_ehci0_init();
	dove_ehci1_init();

	/* ehci init functions access the usb port, only now it's safe to disable
	 * all clocks
	 */
	ds_clks_disable_all(1, 1);

#ifdef CONFIG_MV_ETHERNET
	dove_mv_eth_init();
#else
	dove_ge00_init(&dove_rd_ge00_data);
#endif
	dove_sata_init(&dove_rd_sata_data);
	dove_spi0_init(0);
	dove_spi1_init(0);
	/* uart1 is the debug port, register it first so it will be */
	/* represented by device ttyS0, root filesystems usually expect the */
	/* console to be on that device */
	dove_uart1_init();
	dove_uart0_init();
	/* dove_uart2_init(); not in use (?) */
	/* dove_uart3_init(); not in use (?) */
	dove_i2c_init();
	dove_i2c_exp_init(0);
	dove_sdhci_cam_mbus_init();
	dove_sdio0_init();
	dove_sdio1_init();
	dove_rd_nfc_init();
	dove_i2s_init(0, &i2s0_data);

	dove_cam_init(&dove_cafe_cam_data);
	dove_lcd_spi_init();
	dove_fp_clcd_init();
	dove_vmeta_init();
	dove_gpu_init();
	dove_tact_init(&tact_dove_fp_data);
	dove_cesa_init();
	dove_hwmon_init();

	/*
	 * On-board device registration
	 */

	spi_register_board_info(dove_rd_spi_flash_info,
				ARRAY_SIZE(dove_rd_spi_flash_info));
	spi_register_board_info(dove_fp_spi_devs, dove_fp_spi_devs_num());
	i2c_register_board_info(0, dove_rd_i2c_devs, ARRAY_SIZE(dove_rd_i2c_devs));
	dove_battery_init();
}

MACHINE_START(DOVE_RD, "Marvell MV88F6781-RD Board")
	.phys_io	= DOVE_SB_REGS_PHYS_BASE,
	.io_pg_offst	= ((DOVE_SB_REGS_VIRT_BASE) >> 18) & 0xfffc,
	.boot_params	= 0x00000100,
	.init_machine	= dove_rd_init,
	.map_io		= dove_map_io,
	.init_irq	= dove_init_irq,
	.timer		= &dove_timer,
/* reserve memory for VMETA and GPU */
	.fixup		= dove_tag_fixup_mem32,
MACHINE_END
