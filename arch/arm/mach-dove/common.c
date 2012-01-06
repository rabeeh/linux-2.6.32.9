/*
 * arch/arm/mach-dove/common.c
 *
 * Core functions for Marvell Dove MV88F6781 System On Chip
 *
 * Author: Tzachi Perelstein <tzachi@marvell.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/pci.h>
#include <linux/serial_8250.h>
#include <linux/clk.h>
#include <linux/mbus.h>
#include <linux/mv643xx_eth.h>
#include <linux/mv643xx_i2c.h>
#include <linux/ata_platform.h>
#include <linux/spi/orion_spi.h>
#include <linux/spi/mv_spi.h>
#include <linux/gpio_mouse.h>
#include <linux/dove_sdhci.h>
#include <linux/gpio_keys.h>
#include <linux/i2c-gpio.h>
#include <linux/input.h>
#include <asm/page.h>
#include <asm/setup.h>
#include <asm/timex.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#include <asm/mach/pci.h>
#include <mach/dove.h>
#include <mach/bridge-regs.h>
#include <asm/mach/arch.h>
#include <mach/gpio.h>
#include <linux/irq.h>
#include <asm/hardware/cache-tauros2.h>
#include <plat/ehci-orion.h>
#include <plat/i2s-orion.h>
#include <plat/orion_nand.h>
#include <plat/mv_eth.h>
#include <plat/mv_cesa.h>
#include <plat/time.h>
#include <plat/mv_xor.h>
#include <mvSysHwConfig.h>
#include <ctrlEnv/mvCtrlEnvRegs.h>
#include "ctrlEnv/sys/mvCpuIfRegs.h"
#ifdef CONFIG_MV_HAL_DRIVERS_SUPPORT
#include "mvTypes.h"
#endif
#ifdef CONFIG_MV_ETHERNET
#include "../plat-orion/mv_hal_drivers/mv_drivers_lsp/mv_network/mv_ethernet/mv_netdev.h"
#endif
#include <mach/pm.h>
#include "clock.h"
#include "twsi.h"
#ifdef CONFIG_USB_ANDROID
#include <linux/usb/android_composite.h>
#endif

static unsigned int dove_vmeta_memory_start;
static unsigned int dove_gpu_memory_start;


/* used for memory allocation for the VMETA video engine */
#ifdef CONFIG_UIO_VMETA
#define UIO_DOVE_VMETA_MEM_SIZE (CONFIG_UIO_DOVE_VMETA_MEM_SIZE << 20)
#else
#define UIO_DOVE_VMETA_MEM_SIZE 0
#endif

unsigned int vmeta_size = UIO_DOVE_VMETA_MEM_SIZE;

static int __init vmeta_size_setup(char *str)
{
	get_option(&str, &vmeta_size);

	if (!vmeta_size)
		return 1;

	vmeta_size <<= 20;

	return 1;
}
__setup("vmeta_size=", vmeta_size_setup);


/* used for memory allocation for the GPU graphics engine */
#ifdef CONFIG_DOVE_GPU
#define DOVE_GPU_MEM_SIZE (CONFIG_DOVE_GPU_MEM_SIZE << 20)
#else
#define DOVE_GPU_MEM_SIZE 0
#endif

unsigned int __initdata gpu_size = DOVE_GPU_MEM_SIZE;

static int __init gpu_size_setup(char *str)
{
	get_option(&str, &gpu_size);

	if (!gpu_size)
		return 1;

	gpu_size <<= 20;

	return 1;
}
__setup("gpu_size=", gpu_size_setup);

unsigned int __initdata pvt_size = 0;

static int __init pvt_size_setup(char *str)
{
	get_option(&str, &pvt_size);

	if (!pvt_size)
		return 1;

	pvt_size <<= 20;

	return 1;
}
__setup("pvt_size=", pvt_size_setup);

char *useNandHal = NULL;
static int __init useNandHal_setup(char *s)
{
	useNandHal = s;
	return 1;
}
__setup("useNandHal=", useNandHal_setup);

int useHalDrivers = 0;
#ifdef CONFIG_MV_HAL_DRIVERS_SUPPORT
static int __init useHalDrivers_setup(char *__unused)
{
     useHalDrivers = 1;
     return 1;
}
__setup("useHalDrivers", useHalDrivers_setup);
#endif

#ifdef CONFIG_MV_INCLUDE_USB
#include "mvSysUsbApi.h"
/* Required to get the configuration string from the Kernel Command Line */
static char *usb0Mode = "host";
static char *usb1Mode = "host";
static char *usb_dev_name  = "mv_udc";
int mv_usb0_cmdline_config(char *s);
int mv_usb1_cmdline_config(char *s);
__setup("usb0Mode=", mv_usb0_cmdline_config);
__setup("usb1Mode=", mv_usb1_cmdline_config);

int mv_usb0_cmdline_config(char *s)
{
    usb0Mode = s;
    return 1;
}

int mv_usb1_cmdline_config(char *s)
{
    usb1Mode = s;
    return 1;
}

#endif 

static int noL2 = 0;
static int __init noL2_setup(char *__unused)
{
     noL2 = 1;
     return 1;
}

__setup("noL2", noL2_setup);

int pm_disable = 1;
static int __init pm_disable_setup(char *__unused)
{
     pm_disable = 1;
     return 1;
}

__setup("pm_disable", pm_disable_setup);

static int __init pm_enable_setup(char *__unused)
{
     pm_disable = 0;
     return 1;
}

__setup("pm_enable", pm_enable_setup);


int cpufreq_disable = 0;
static int __init cpufreq_disable_setup(char *__unused)
{
     cpufreq_disable = 1;
     return 1;
}

__setup("cpufreq_disable", cpufreq_disable_setup);


#include "common.h"

#ifdef CONFIG_PM
enum orion_dwnstrm_conf_save_state {
	/* CPU Configuration Registers */
	DOVE_DWNSTRM_BRDG_CPU_CONFIG = 0,
	DOVE_DWNSTRM_BRDG_CPU_CONTROL,
	DOVE_DWNSTRM_BRDG_RSTOUTn_MASK,
	DOVE_DWNSTRM_BRDG_BRIDGE_MASK,
	DOVE_DWNSTRM_BRDG_POWER_MANAGEMENT,
	/* CPU Timers Registers */
	DOVE_DWNSTRM_BRDG_TIMER_CTRL,
	DOVE_DWNSTRM_BRDG_TIMER0_RELOAD,
	DOVE_DWNSTRM_BRDG_TIMER1_RELOAD,
	DOVE_DWNSTRM_BRDG_TIMER_WD_RELOAD,
	/* Main Interrupt Controller Registers */
	DOVE_DWNSTRM_BRDG_IRQ_MASK_LOW,
	DOVE_DWNSTRM_BRDG_FIQ_MASK_LOW,
	DOVE_DWNSTRM_BRDG_ENDPOINT_MASK_LOW,
	DOVE_DWNSTRM_BRDG_IRQ_MASK_HIGH,
	DOVE_DWNSTRM_BRDG_FIQ_MASK_HIGH,
	DOVE_DWNSTRM_BRDG_ENDPOINT_MASK_HIGH,
	DOVE_DWNSTRM_BRDG_PCIE_INTERRUPT_MASK,	

	DOVE_DWNSTRM_BRDG_SIZE
};

#define DOVE_DWNSTRM_BRDG_SAVE(x) \
	dove_downstream_regs[DOVE_DWNSTRM_BRDG_##x] = readl(x)
#define DOVE_DWNSTRM_BRDG_RESTORE(x) \
	writel(dove_downstream_regs[DOVE_DWNSTRM_BRDG_##x], x)

static u32 dove_downstream_regs[DOVE_DWNSTRM_BRDG_SIZE];

enum orion_uptrm_conf_save_state {
	/* Upstream Bridge Configuration Registers */
	DOVE_UPSTRM_AXI_P_D_CTRL_REG = 0,
	DOVE_UPSTRM_D2X_ARB_LO_REG,
	DOVE_UPSTRM_D2X_ARB_HI_REG,

	DOVE_UPSTRM_BRDG_SIZE
};

#define DOVE_UPSTRM_BRDG_SAVE(x) \
	dove_upstream_regs[DOVE_##x] = readl(DOVE_SB_REGS_VIRT_BASE | x)
#define DOVE_UPSTRM_BRDG_RESTORE(x) \
	writel(dove_upstream_regs[DOVE_##x], (DOVE_SB_REGS_VIRT_BASE | x))

static u32 dove_upstream_regs[DOVE_UPSTRM_BRDG_SIZE];
#endif

/*****************************************************************************
 * General
 ****************************************************************************/

/*
 * Identify device ID and revision.
 */
u32 chip_dev, chip_rev;
EXPORT_SYMBOL(chip_dev);
EXPORT_SYMBOL(chip_rev);

static char * __init dove_id(void)
{
	u32 dev, rev;

	dove_pcie_id(&dev, &rev);
	chip_dev = dev;
	chip_rev = rev;

	if (rev == DOVE_REV_Z0)
		return "MV88AP510-Z0";
	if (rev == DOVE_REV_Z1)
		return "MV88AP510-Z1";
	if (rev == DOVE_REV_Y0)
		return "MV88AP510-Y0";
	if (rev == DOVE_REV_Y1)
		return "MV88AP510-Y1";
	if (rev == DOVE_REV_X0)
		return "MV88AP510-X0";
	if (rev == DOVE_REV_A0)
		return "MV88AP510-A0";
	else
		return "MV88AP510-Rev-Unsupported";
}

/*****************************************************************************
 * I/O Address Mapping
 ****************************************************************************/
static struct map_desc dove_io_desc[] __initdata = {
	{
		.virtual	= DOVE_SCRATCHPAD_VIRT_BASE,
		.pfn		= __phys_to_pfn(DOVE_SCRATCHPAD_PHYS_BASE),
		.length		= DOVE_SCRATCHPAD_SIZE,
		.type		= MT_EXEC_REGS,
	}, {
		.virtual	= DOVE_SB_REGS_VIRT_BASE,
		.pfn		= __phys_to_pfn(DOVE_SB_REGS_PHYS_BASE),
		.length		= DOVE_SB_REGS_SIZE,
		.type		= MT_DEVICE,
	}, {
		.virtual	= DOVE_NB_REGS_VIRT_BASE,
		.pfn		= __phys_to_pfn(DOVE_NB_REGS_PHYS_BASE),
		.length		= DOVE_NB_REGS_SIZE,
		.type		= MT_DEVICE,
	}, {
		.virtual	= DOVE_PCIE0_IO_VIRT_BASE,
		.pfn		= __phys_to_pfn(DOVE_PCIE0_IO_PHYS_BASE),
		.length		= DOVE_PCIE0_IO_SIZE,
		.type		= MT_DEVICE,
	}, {
		.virtual	= DOVE_PCIE1_IO_VIRT_BASE,
		.pfn		= __phys_to_pfn(DOVE_PCIE1_IO_PHYS_BASE),
		.length		= DOVE_PCIE1_IO_SIZE,
		.type		= MT_DEVICE,
	},{
		.virtual	= DOVE_CESA_VIRT_BASE,
		.pfn		= __phys_to_pfn(DOVE_CESA_PHYS_BASE),
		.length		= DOVE_CESA_SIZE,
		.type		= MT_DEVICE,
	},


};

void __init dove_map_io(void)
{
	iotable_init(dove_io_desc, ARRAY_SIZE(dove_io_desc));
}
/*****************************************************************************
 * SDIO
 ****************************************************************************/
#define DOVE_SD0_START_GPIO	40
#define DOVE_SD1_START_GPIO	46


struct sdhci_dove_int_wa sdio0_data = {
	.gpio = DOVE_SD0_START_GPIO + 3,
	.func_select_bit = 0
};

static u64 sdio_dmamask = DMA_BIT_MASK(32);
/*****************************************************************************
 * SDIO0
 ****************************************************************************/

static struct resource dove_sdio0_resources[] = {
	{
		.start	= DOVE_SDIO0_PHYS_BASE,
		.end	= DOVE_SDIO0_PHYS_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_DOVE_SDIO0,
		.end	= IRQ_DOVE_SDIO0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_sdio0 = {
	.name		= "sdhci-mv",
	.id		= 0,
	.dev		= {
		.dma_mask		= &sdio_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
	.resource	= dove_sdio0_resources,
	.num_resources	= ARRAY_SIZE(dove_sdio0_resources),
};

void __init dove_sdio0_init(void)
{
	platform_device_register(&dove_sdio0);
}

/*****************************************************************************
 * SDIO1
 ****************************************************************************/
struct sdhci_dove_int_wa sdio1_data = {
	.gpio = DOVE_SD1_START_GPIO + 3,
	.func_select_bit = 1
};

static struct resource dove_sdio1_resources[] = {
	{
		.start	= DOVE_SDIO1_PHYS_BASE,
		.end	= DOVE_SDIO1_PHYS_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_DOVE_SDIO1,
		.end	= IRQ_DOVE_SDIO1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_sdio1 = {
	.name		= "sdhci-mv",
	.id		= 1,
	.dev		= {
		.dma_mask		= &sdio_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
	.resource	= dove_sdio1_resources,
	.num_resources	= ARRAY_SIZE(dove_sdio1_resources),
};

void __init dove_sdio1_init(void)
{
	platform_device_register(&dove_sdio1);
}

/* sdio card interrupt workaround, when no command is running, switch the
 * DATA[1] pin to be gpio-in with level interrupt.
 */

void __init dove_sd_card_int_wa_setup(int port)
{
	int	gpio = 0, i, irq = 0;
	char	*name;

	switch(port) {
	case 0:
		gpio = DOVE_SD0_START_GPIO;
		name = "sd0";
		dove_sdio0.dev.platform_data = &sdio0_data;
		irq = sdio0_data.irq = gpio_to_irq(sdio0_data.gpio);
		break;
	case 1:
		gpio = DOVE_SD1_START_GPIO;
		name = "sd1";
		dove_sdio1.dev.platform_data = &sdio1_data;
		irq = sdio1_data.irq = gpio_to_irq(sdio1_data.gpio);
		break;
	default:
		printk(KERN_ERR "dove_sd_card_int_wa_setup: bad port (%d)\n", 
		       port);
		return;
	}
	
	for (i = gpio; i < (gpio + 6); i++) {
		orion_gpio_set_valid(i, 1);
		
		if (gpio_request(i, name) != 0)
			printk(KERN_ERR "dove: failed to config gpio (%d) for %s\n",
			       i, name);

		gpio_direction_input(i);
	}
	set_irq_type(irq, IRQ_TYPE_LEVEL_LOW);
}

/*****************************************************************************
 * EHCI
 ****************************************************************************/
static struct orion_ehci_data dove_ehci_data = {
	.dram		= &dove_mbus_dram_info,
	.phy_version	= EHCI_PHY_DOVE,
};

static u64 ehci_dmamask = DMA_BIT_MASK(32);

/*****************************************************************************
 * EHCI0
 ****************************************************************************/
static struct resource dove_ehci0_resources[] = {
	{
		.start	= DOVE_USB0_PHYS_BASE,
		.end	= DOVE_USB0_PHYS_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_DOVE_USB0,
		.end	= IRQ_DOVE_USB0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_ehci0 = {
	.name		= "orion-ehci",
	.id		= 0,
	.dev		= {
		.dma_mask		= &ehci_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &dove_ehci_data,
	},
	.resource	= dove_ehci0_resources,
	.num_resources	= ARRAY_SIZE(dove_ehci0_resources),
};

void __init dove_ehci0_init(void)
{
#ifdef CONFIG_MV_INCLUDE_USB
	if (	(strcmp(usb0Mode, "device") == 0) && 
		(strcmp(usb1Mode, "device") == 0)) {
		printk("Warning: trying to set both USB0 and USB1 to device mode!\n");
	}

	if (strcmp(usb0Mode, "host") == 0) {
		printk("Initializing USB0 Host\n");
		if (useHalDrivers) {
			dove_ehci_data.dram = NULL;
			dove_ehci_data.phy_version = EHCI_PHY_NA;
			mvSysUsbInit(0, 1);
		}
	}
	else {
		printk("Initializing USB0 Device\n");
		dove_ehci0.name = usb_dev_name;
		mvSysUsbInit(0, 0);
	}
#endif
	platform_device_register(&dove_ehci0);
}

/*****************************************************************************
 * EHCI1
 ****************************************************************************/
static struct resource dove_ehci1_resources[] = {
	{
		.start	= DOVE_USB1_PHYS_BASE,
		.end	= DOVE_USB1_PHYS_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_DOVE_USB1,
		.end	= IRQ_DOVE_USB1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_ehci1 = {
	.name		= "orion-ehci",
	.id		= 1,
	.dev		= {
		.dma_mask		= &ehci_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &dove_ehci_data,
	},
	.resource	= dove_ehci1_resources,
	.num_resources	= ARRAY_SIZE(dove_ehci1_resources),
};

void __init dove_ehci1_init(void)
{
#ifdef CONFIG_MV_INCLUDE_USB
	if (strcmp(usb1Mode, "host") == 0) {
		printk("Initializing USB1 Host\n");
		if (useHalDrivers) {
			dove_ehci_data.dram = NULL;
			dove_ehci_data.phy_version = EHCI_PHY_NA;
			mvSysUsbInit(1, 1);
		}
	}
	else {
		printk("Initializing USB1 Device\n");
		dove_ehci1.name = usb_dev_name;
		mvSysUsbInit(1, 0);
	}
#endif
	platform_device_register(&dove_ehci1);
}

#ifdef CONFIG_MV_ETHERNET
/*****************************************************************************
 * Ethernet
 ****************************************************************************/
struct mv_eth_addr_dec_platform_data dove_eth_addr_dec_data = {
	.dram		= &dove_mbus_dram_info,
};

static struct platform_device dove_eth_addr_dec = {
	.name		= MV_ETH_ADDR_DEC_NAME,
	.id		= 0,
	.dev		= {
		.platform_data	= &dove_eth_addr_dec_data,
	},
	.num_resources	= 0,
};

static struct mv_netdev_platform_data dove_eth_data = {
	.port_number = 0
};

static struct resource dove_eth_resources[] = {
	{
		.name	= "eth irq",
		.start	= IRQ_DOVE_GE00_SUM,
		.end	= IRQ_DOVE_GE00_SUM,
		.flags	= IORESOURCE_IRQ,
	}
};

static struct platform_device dove_eth = {
	.name		= MV_NETDEV_ETH_NAME,
	.id		= 0,
	.num_resources	= 1,
	.resource	= dove_eth_resources,
};

void __init dove_mv_eth_init(void)
{
	dove_eth.dev.platform_data = &dove_eth_data;
	platform_device_register(&dove_eth);
	if(!useHalDrivers)
		platform_device_register(&dove_eth_addr_dec);
}
#endif
#if 1
/*****************************************************************************
 * GE00
 ****************************************************************************/
struct mv643xx_eth_shared_platform_data dove_ge00_shared_data = {
	.t_clk		= 0,
	.dram		= &dove_mbus_dram_info,
};

static struct resource dove_ge00_shared_resources[] = {
	{
		.name	= "ge00 base",
		.start	= DOVE_GE00_PHYS_BASE + 0x2000,
		.end	= DOVE_GE00_PHYS_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device dove_ge00_shared = {
	.name		= MV643XX_ETH_SHARED_NAME,
	.id		= 0,
	.dev		= {
		.platform_data	= &dove_ge00_shared_data,
	},
	.num_resources	= 1,
	.resource	= dove_ge00_shared_resources,
};

static struct resource dove_ge00_resources[] = {
	{
		.name	= "ge00 irq",
		.start	= IRQ_DOVE_GE00_SUM,
		.end	= IRQ_DOVE_GE00_SUM,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_ge00 = {
	.name		= MV643XX_ETH_NAME,
	.id		= 0,
	.num_resources	= 1,
	.resource	= dove_ge00_resources,
	.dev		= {
		.coherent_dma_mask	= 0xffffffff,
	},
};

void __init dove_ge00_init(struct mv643xx_eth_platform_data *eth_data)
{
	eth_data->shared = &dove_ge00_shared;
	dove_ge00.dev.platform_data = eth_data;

	platform_device_register(&dove_ge00_shared);
	platform_device_register(&dove_ge00);
}
#endif

/*****************************************************************************
 * SoC RTC
 ****************************************************************************/
static struct resource dove_rtc_resource[] = {
	{
		.start	= DOVE_RTC_PHYS_BASE,
		.end	= DOVE_RTC_PHYS_BASE + 32 - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_DOVE_RTC,
		.flags	= IORESOURCE_IRQ,
	}
};

void __init dove_rtc_init(void)
{
	platform_device_register_simple("rtc-mv", -1, dove_rtc_resource, 2);
}

/*****************************************************************************
 * SoC hwmon Thermal Sensor
 ****************************************************************************/
void __init dove_hwmon_init(void)
{
	platform_device_register_simple("dove-temp", 0, NULL, 0);
}

/*****************************************************************************
 * SATA
 ****************************************************************************/
static struct resource dove_sata_resources[] = {
	{
		.name	= "sata base",
		.start	= DOVE_SATA_PHYS_BASE,
		.end	= DOVE_SATA_PHYS_BASE + 0x5000 - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.name	= "sata irq",
		.start	= IRQ_DOVE_SATA,
		.end	= IRQ_DOVE_SATA,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_sata = {
	.name		= "sata_mv",
	.id		= 0,
	.dev		= {
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
	.num_resources	= ARRAY_SIZE(dove_sata_resources),
	.resource	= dove_sata_resources,
};

void __init dove_sata_init(struct mv_sata_platform_data *sata_data)
{
	if (useHalDrivers) {
		sata_data->dram = NULL;
		dove_sata.name = "sata_mv_hal";
	} else
		sata_data->dram = &dove_mbus_dram_info;

	dove_sata.dev.platform_data = sata_data;

	platform_device_register(&dove_sata);
}

/*****************************************************************************
 * UART0
 ****************************************************************************/
static struct plat_serial8250_port dove_uart0_data[] = {
	{
		.mapbase	= DOVE_UART0_PHYS_BASE,
		.membase	= (char *)DOVE_UART0_VIRT_BASE,
		.irq		= IRQ_DOVE_UART_0,
		.flags		= UPF_SKIP_TEST | UPF_BOOT_AUTOCONF,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= 0,
	}, {
	},
};

static struct resource dove_uart0_resources[] = {
	{
		.start		= DOVE_UART0_PHYS_BASE,
		.end		= DOVE_UART0_PHYS_BASE + SZ_256 - 1,
		.flags		= IORESOURCE_MEM,
	}, {
		.start		= IRQ_DOVE_UART_0,
		.end		= IRQ_DOVE_UART_0,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_uart0 = {
	.name			= "serial8250",
	.id			= 0,
	.dev			= {
		.platform_data	= dove_uart0_data,
	},
	.resource		= dove_uart0_resources,
	.num_resources		= ARRAY_SIZE(dove_uart0_resources),
};

void __init dove_uart0_init(void)
{
	platform_device_register(&dove_uart0);
}

/*****************************************************************************
 * UART1
 ****************************************************************************/
static struct plat_serial8250_port dove_uart1_data[] = {
	{
		.mapbase	= DOVE_UART1_PHYS_BASE,
		.membase	= (char *)DOVE_UART1_VIRT_BASE,
		.irq		= IRQ_DOVE_UART_1,
		.flags		= UPF_SKIP_TEST | UPF_BOOT_AUTOCONF,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= 0,
	}, {
	},
};

static struct resource dove_uart1_resources[] = {
	{
		.start		= DOVE_UART1_PHYS_BASE,
		.end		= DOVE_UART1_PHYS_BASE + SZ_256 - 1,
		.flags		= IORESOURCE_MEM,
	}, {
		.start		= IRQ_DOVE_UART_1,
		.end		= IRQ_DOVE_UART_1,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_uart1 = {
	.name			= "serial8250",
	.id			= 1,
	.dev			= {
		.platform_data	= dove_uart1_data,
	},
	.resource		= dove_uart1_resources,
	.num_resources		= ARRAY_SIZE(dove_uart1_resources),
};

void __init dove_uart1_init(void)
{
	platform_device_register(&dove_uart1);
}

/*****************************************************************************
 * UART2
 ****************************************************************************/
static struct plat_serial8250_port dove_uart2_data[] = {
	{
		.mapbase	= DOVE_UART2_PHYS_BASE,
		.membase	= (char *)DOVE_UART2_VIRT_BASE,
		.irq		= IRQ_DOVE_UART_2,
		.flags		= UPF_SKIP_TEST | UPF_BOOT_AUTOCONF,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= 0,
	}, {
	},
};

static struct resource dove_uart2_resources[] = {
	{
		.start		= DOVE_UART2_PHYS_BASE,
		.end		= DOVE_UART2_PHYS_BASE + SZ_256 - 1,
		.flags		= IORESOURCE_MEM,
	}, {
		.start		= IRQ_DOVE_UART_2,
		.end		= IRQ_DOVE_UART_2,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_uart2 = {
	.name			= "serial8250",
	.id			= 2,
	.dev			= {
		.platform_data	= dove_uart2_data,
	},
	.resource		= dove_uart2_resources,
	.num_resources		= ARRAY_SIZE(dove_uart2_resources),
};

void __init dove_uart2_init(void)
{
	platform_device_register(&dove_uart2);
}

/*****************************************************************************
 * UART3
 ****************************************************************************/
static struct plat_serial8250_port dove_uart3_data[] = {
	{
		.mapbase	= DOVE_UART3_PHYS_BASE,
		.membase	= (char *)DOVE_UART3_VIRT_BASE,
		.irq		= IRQ_DOVE_UART_3,
		.flags		= UPF_SKIP_TEST | UPF_BOOT_AUTOCONF,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= 0,
	}, {
	},
};

static struct resource dove_uart3_resources[] = {
	{
		.start		= DOVE_UART3_PHYS_BASE,
		.end		= DOVE_UART3_PHYS_BASE + SZ_256 - 1,
		.flags		= IORESOURCE_MEM,
	}, {
		.start		= IRQ_DOVE_UART_3,
		.end		= IRQ_DOVE_UART_3,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_uart3 = {
	.name			= "serial8250",
	.id			= 3,
	.dev			= {
		.platform_data	= dove_uart3_data,
	},
	.resource		= dove_uart3_resources,
	.num_resources		= ARRAY_SIZE(dove_uart3_resources),
};

void __init dove_uart3_init(void)
{
	platform_device_register(&dove_uart3);
}

/*****************************************************************************
 * SPI0
 ****************************************************************************/
static struct orion_spi_info dove_spi0_data = {
	.tclk		= 0,
	.optional_div	= 1,
};

static struct resource dove_spi0_resources[] = {
	{
		.start	= DOVE_SPI0_PHYS_BASE,
		.end	= DOVE_SPI0_PHYS_BASE + SZ_512 - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_DOVE_SPI0,
		.end	= IRQ_DOVE_SPI0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_spi0 = {
	.name		= "orion_spi",
	.id		= 0,
	.resource	= dove_spi0_resources,
	.dev		= {
		.platform_data	= &dove_spi0_data,
	},
	.num_resources	= ARRAY_SIZE(dove_spi0_resources),
};

void __init dove_spi0_init(int use_interrupt)
{
	dove_spi0_data.use_interrupt = use_interrupt;
	platform_device_register(&dove_spi0);
}

/*****************************************************************************
 * SPI1
 ****************************************************************************/
static struct orion_spi_info dove_spi1_data = {
	.tclk		= 0,
	.optional_div	= 1,
};

static struct resource dove_spi1_resources[] = {
	{
		.start	= DOVE_SPI1_PHYS_BASE,
		.end	= DOVE_SPI1_PHYS_BASE + SZ_512 - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_DOVE_SPI1,
		.end	= IRQ_DOVE_SPI1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_spi1 = {
	.name		= "orion_spi",
	.id		= 1,
	.resource	= dove_spi1_resources,
	.dev		= {
		.platform_data	= &dove_spi1_data,
	},
	.num_resources	= ARRAY_SIZE(dove_spi1_resources),
};

void __init dove_spi1_init(int use_interrupt)
{
	dove_spi1_data.use_interrupt = use_interrupt;
	platform_device_register(&dove_spi1);
}

/*****************************************************************************
 * LCD SPI
 ****************************************************************************/
static struct mv_spi_info dove_lcd_spi_data = {
	.clk		= 400000000,
};

static struct resource dove_lcd_spi_resources[] = {
	{
		.start	= DOVE_LCD_PHYS_BASE + 0x10000,
		.end	= DOVE_LCD_PHYS_BASE + 0x10000 + SZ_512 - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device dove_lcd_spi = {
	.name		= "mv_spi",
	.id		= 2,
	.dev		= {
		.platform_data	= &dove_lcd_spi_data,
	},
};

void __init dove_lcd_spi_init(void)
{
	platform_device_register(&dove_lcd_spi);
}

/*****************************************************************************
 * I2C
 ****************************************************************************/
static struct mv64xxx_i2c_pdata dove_i2c_data = {
	.freq_m		= 10, /* assumes 166 MHz TCLK gets 94.3kHz */
	.freq_n		= 3,
	.delay_after_stop = 3, /* 3 ms delay needed when freq is 94.3kHz */
	.timeout	= 1000, /* Default timeout of 1 second */
#ifdef CONFIG_I2C_MV64XXX_PORT_EXPANDER
	.select_exp_port = dove_select_exp_port,
#endif
};

static struct resource dove_i2c_resources[] = {
	{
		.name	= "i2c base",
		.start	= DOVE_I2C_PHYS_BASE,
		.end	= DOVE_I2C_PHYS_BASE + 0x20 -1,
		.flags	= IORESOURCE_MEM,
	}, {
		.name	= "i2c irq",
		.start	= IRQ_DOVE_I2C,
		.end	= IRQ_DOVE_I2C,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_i2c = {
	.name		= MV64XXX_I2C_CTLR_NAME,
	.id		= 0,
	.num_resources	= ARRAY_SIZE(dove_i2c_resources),
	.resource	= dove_i2c_resources,
	.dev		= {
		.platform_data = &dove_i2c_data,
	},
};
#ifdef CONFIG_I2C_MV64XXX_PORT_EXPANDER
static struct mv64xxx_i2c_exp_pdata dove_i2c_exp_port0_data = {
	.hw_adapter	= &dove_i2c,
	.timeout	= 1000, /* Default timeout of 1 second */
};

static struct platform_device dove_i2c_exp_port0 = {
	.name		= MV64XXX_I2C_EXPANDER_NAME,
	.id		= 0,
	.dev		= {
		.platform_data = &dove_i2c_exp_port0_data,
	},
};

static struct mv64xxx_i2c_exp_pdata dove_i2c_exp_port1_data = {
	.hw_adapter	= &dove_i2c,
	.timeout	= 1000, /* Default timeout of 1 second */
};

static struct platform_device dove_i2c_exp_port1 = {
	.name		= MV64XXX_I2C_EXPANDER_NAME,
	.id		= 1,
	.dev		= {
		.platform_data = &dove_i2c_exp_port1_data,
	},
};

static struct mv64xxx_i2c_exp_pdata dove_i2c_exp_port2_data = {
	.hw_adapter	= &dove_i2c,
	.timeout	= 1000, /* Default timeout of 1 second */
};

static struct platform_device dove_i2c_exp_port2 = {
	.name		= MV64XXX_I2C_EXPANDER_NAME,
	.id		= 2,
	.dev		= {
		.platform_data = &dove_i2c_exp_port2_data,
	},
};
#endif
void __init dove_i2c_init(void)
{
	platform_device_register(&dove_i2c);
}

void __init dove_i2c_exp_init(int nr)
{
#ifdef CONFIG_I2C_MV64XXX_PORT_EXPANDER
	if (nr == 0) {
		dove_i2c_exp_port0_data.hw_adapter = &dove_i2c;
		platform_device_register(&dove_i2c_exp_port0);
	}

	if (nr == 1) {
		dove_i2c_exp_port1_data.hw_adapter = &dove_i2c;
		platform_device_register(&dove_i2c_exp_port1);
	}
	if (nr == 2) {
		dove_i2c_exp_port2_data.hw_adapter = &dove_i2c;
		platform_device_register(&dove_i2c_exp_port2);
	}
#endif
}

#ifdef CONFIG_DOVE_DB_USE_GPIO_I2C
static struct i2c_gpio_platform_data dove_gpio_i2c_pdata = {
	.sda_pin	= 17,
	.scl_pin	= 19,
	.udelay		= 2, /* ~100 kHz */
};

static struct platform_device dove_gpio_i2c_device = {
	.name                   = "i2c-gpio",
	.id                     = 1,
	.dev.platform_data      = &dove_gpio_i2c_pdata,
};

void __init dove_add_gpio_i2c(void)
{
	platform_device_register(&dove_gpio_i2c_device);
}
#endif

/*****************************************************************************
 * Camera
 ****************************************************************************/
static struct resource dove_cam_resources[] = {
	{
		.start	= DOVE_CAM_PHYS_BASE,
		.end	= DOVE_CAM_PHYS_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_DOVE_CAM,
		.end	= IRQ_DOVE_CAM,
		.flags	= IORESOURCE_IRQ,
	},
};

static u64 dove_cam_dmamask = DMA_BIT_MASK(32);

static struct platform_device dove_cam = {
	.name			= "cafe1000-ccic",
	.id			= 0,
	.dev			= {
		.dma_mask		= &dove_cam_dmamask,
		.coherent_dma_mask = 0xFFFFFFFF,
	},
	.num_resources		= ARRAY_SIZE(dove_cam_resources),
	.resource		= dove_cam_resources,
};

void __init dove_cam_init(struct cafe_cam_platform_data *cafe_cam_data)
{
	dove_cam.dev.platform_data = cafe_cam_data;
	platform_device_register(&dove_cam);
}

/*****************************************************************************
 * SDIO and Camera mbus driver
 ****************************************************************************/
#define DOVE_SDHCI_CAM_MBUS_PHYS_BASE DOVE_CAFE_WIN_PHYS_BASE
static struct resource dove_sdhci_cam_mbus_resources[] = {
	{
		.start	= DOVE_SDHCI_CAM_MBUS_PHYS_BASE,
		.end	= DOVE_SDHCI_CAM_MBUS_PHYS_BASE + 64 - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device dove_sdhci_cam_mbus = {
	.name			= "sdhci_cam_mbus",
	.id			= 0,
	.dev			= {
		.platform_data	= &dove_mbus_dram_info,
	},
	.num_resources		= ARRAY_SIZE(dove_sdhci_cam_mbus_resources),
	.resource		= dove_sdhci_cam_mbus_resources,
};

void __init dove_sdhci_cam_mbus_init(void)
{
	platform_device_register(&dove_sdhci_cam_mbus);
}

/*****************************************************************************
 * I2S/SPDIF
 ****************************************************************************/
static struct resource dove_i2s0_resources[] = {
	[0] = {
		.start  = DOVE_AUD0_PHYS_BASE,
		.end    = DOVE_AUD0_PHYS_BASE + SZ_16K -1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_DOVE_I2S0,
		.end    = IRQ_DOVE_I2S0,
		.flags  = IORESOURCE_IRQ,
	}
};

static u64 dove_i2s0_dmamask = 0xFFFFFFFFUL;

 
static struct platform_device dove_i2s0 = {
	.name           = "mv88fx_snd",
	.id             = 0,
	.dev            = {
		.dma_mask = &dove_i2s0_dmamask,
		.coherent_dma_mask = 0xFFFFFFFF,
	},
	.num_resources  = ARRAY_SIZE(dove_i2s0_resources),
	.resource       = dove_i2s0_resources,
};

static struct resource dove_i2s1_resources[] = {
	[0] = {
		.start  = DOVE_AUD1_PHYS_BASE,
		.end    = DOVE_AUD1_PHYS_BASE + SZ_16K -1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_DOVE_I2S1,
		.end    = IRQ_DOVE_I2S1,
		.flags  = IORESOURCE_IRQ,
	}
};

static u64 dove_i2s1_dmamask = 0xFFFFFFFFUL;

static struct platform_device dove_i2s1 = {
	.name           = "mv88fx_snd",
	.id             = 1,
	.dev            = {
		.dma_mask = &dove_i2s1_dmamask,
		.coherent_dma_mask = 0xFFFFFFFF,
	},
	.num_resources  = ARRAY_SIZE(dove_i2s1_resources),
	.resource       = dove_i2s1_resources,
};

static struct platform_device dove_mv88fx_i2s0 = {
	.name           = "mv88fx-i2s",
	.id             = 0,
};

static struct platform_device dove_mv88fx_i2s1 = {
	.name           = "mv88fx-i2s",
	.id             = 1,
};


void __init dove_i2s_init(int port, struct orion_i2s_platform_data *i2s_data)
{
	switch(port){
	case 0:
		platform_device_register(&dove_mv88fx_i2s0);
		i2s_data->dram = &dove_mbus_dram_info;
		dove_i2s0.dev.platform_data = i2s_data;
		platform_device_register(&dove_i2s0);
		return;
	case 1:
		platform_device_register(&dove_mv88fx_i2s1);
		i2s_data->dram = &dove_mbus_dram_info;
		dove_i2s1.dev.platform_data = i2s_data;
		platform_device_register(&dove_i2s1);
		return;
	default:
		BUG();
	}

		
}

/*****************************************************************************
 * CESA
 ****************************************************************************/
struct mv_cesa_addr_dec_platform_data dove_cesa_addr_dec_data = {
	.dram		= &dove_mbus_dram_info,
};

static struct platform_device dove_cesa_ocf = {
	.name           = "dove_cesa_ocf",
	.id             = -1,
	.dev		= {
		.platform_data	= &dove_cesa_addr_dec_data,
	},
};

static struct platform_device dove_cesadev = {
	.name           = "dove_cesadev",
	.id             = -1,
};


void __init dove_cesa_init(void)
{
	platform_device_register(&dove_cesa_ocf);
	platform_device_register(&dove_cesadev);
}

/*****************************************************************************
 * VPU
 ****************************************************************************/
static struct resource dove_vmeta_resources[] = {
	[0] = {
		.start	= DOVE_VPU_PHYS_BASE,
		.end	= DOVE_VPU_PHYS_BASE + 0x280000 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {		/* Place holder for reserved memory */
		.start	= 0,
		.end	= 0,
		.flags	= IORESOURCE_MEM,
	},
	[2] = {
		.start  = IRQ_DOVE_VMETA_DMA1,
		.end    = IRQ_DOVE_VMETA_DMA1,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device dove_vmeta = {
	.name		= "ap510-vmeta",
	.id		= 0,
	.dev		= {
		.dma_mask		= DMA_BIT_MASK(32),
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
	.resource	= dove_vmeta_resources,
	.num_resources	= ARRAY_SIZE(dove_vmeta_resources),
};

void __init dove_vmeta_init(void)
{
#ifdef CONFIG_UIO_VMETA
	if (vmeta_size == 0) {
		printk("memory allocation for VMETA failed\n");
		return;
	}

	dove_vmeta_resources[1].start = dove_vmeta_memory_start;
	dove_vmeta_resources[1].end = dove_vmeta_memory_start + vmeta_size - 1;

	platform_device_register(&dove_vmeta);
#endif
}

#ifdef CONFIG_DOVE_VPU_USE_BMM
unsigned int dove_vmeta_get_memory_start(void)
{
	return dove_vmeta_memory_start;
}
EXPORT_SYMBOL(dove_vmeta_get_memory_start);

int dove_vmeta_get_memory_size(void)
{
	return vmeta_size;
}
EXPORT_SYMBOL(dove_vmeta_get_memory_size);
#endif

/*****************************************************************************
 * GPU
 ****************************************************************************/
static struct resource dove_gpu_resources[] = {
	{
		.name   = "gpu_irq",
		.start	= IRQ_DOVE_GPU,
		.end	= IRQ_DOVE_GPU,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "gpu_base",
		.start	= DOVE_GPU_PHYS_BASE,
		.end	= DOVE_GPU_PHYS_BASE + 0x40000 - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name   = "gpu_mem",
		.start	= 0,
		.end	= 0,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device dove_gpu = {
	.name		= "galcore",
	.id		= 0,
	.num_resources  = ARRAY_SIZE(dove_gpu_resources),
	.resource       = dove_gpu_resources,
};

void __init dove_gpu_init(void)
{
#ifdef CONFIG_DOVE_GPU
	if (gpu_size == 0) {
		printk("memory allocation for GPU failed\n");
		return;
	}

	dove_gpu_resources[2].start = dove_gpu_memory_start;
	dove_gpu_resources[2].end = dove_gpu_memory_start + gpu_size - 1;
	
	platform_device_register(&dove_gpu);
#endif
}

#ifdef CONFIG_DOVE_GPU_USE_BMM
unsigned int dove_gpu_get_memory_start(void)
{
	return dove_gpu_memory_start;
}
EXPORT_SYMBOL(dove_gpu_get_memory_start);

int dove_gpu_get_memory_size(void)
{
	return gpu_size;
}
EXPORT_SYMBOL(dove_gpu_get_memory_size);
#endif

/*****************************************************************************
 * SSP
 ****************************************************************************/
static struct resource dove_ssp_resources[] = {
	{
		.name	= "ssp base",
		.start	=  DOVE_SSP_PHYS_BASE,
		.end	=  DOVE_SSP_PHYS_BASE +  64 - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.name	= "ssp csr",
		.start	=  DOVE_SSP_CTRL_STATUS_1,
		.end	=  DOVE_SSP_CTRL_STATUS_1 +  16 - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.name	= "ssp irq",
		.start	= IRQ_DOVE_SSP,
		.end	= IRQ_DOVE_SSP,
		.flags	= IORESOURCE_IRQ,
	},
	{
		/* Receive */
		.start	= 13,
		.end	= 13,
		.flags	= IORESOURCE_DMA,
	},
	{
		/* Transmit */
		.start	= 14,
		.end	= 14,
		.flags	= IORESOURCE_DMA,
	},

};




static struct platform_device dove_ssp_pdev = {
	.name		= "dove-ssp",
	.id		= 0,
	.dev = {
		.platform_data = NULL,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
	.num_resources	= ARRAY_SIZE(dove_ssp_resources),
	.resource	= dove_ssp_resources,
};



void __init dove_ssp_init(struct dove_ssp_platform_data *pdata)
{
	dove_ssp_pdev.dev.platform_data = pdata;
	platform_device_register(&dove_ssp_pdev);
}


/*****************************************************************************
 * TACT switch
 ****************************************************************************/
static struct platform_device dove_tact = {
	.name			= "gpio_mouse",
	.id			= 0,
};

void __init dove_tact_init(struct gpio_mouse_platform_data *tact_data)
{
	dove_tact.dev.platform_data = tact_data;
	platform_device_register(&dove_tact);
}

/*****************************************************************************
 * Time handling
 ****************************************************************************/

static void dove_timer_init(void)
{
	orion_time_init(IRQ_DOVE_BRIDGE, dove_tclk_get());
}

struct sys_timer dove_timer = {
	.init = dove_timer_init,
};

/*****************************************************************************
 * XOR 
 ****************************************************************************/
static struct mv_xor_platform_shared_data dove_xor_shared_data = {
	.dram		= &dove_mbus_dram_info,
};

/*****************************************************************************
 * XOR 0
 ****************************************************************************/
static u64 dove_xor0_dmamask = DMA_BIT_MASK(32);

static struct resource dove_xor0_shared_resources[] = {
	{
		.name	= "xor 0 low",
		.start	= DOVE_XOR0_PHYS_BASE,
		.end	= DOVE_XOR0_PHYS_BASE + 0xff,
		.flags	= IORESOURCE_MEM,
	}, {
		.name	= "xor 0 high",
		.start	= DOVE_XOR0_HIGH_PHYS_BASE,
		.end	= DOVE_XOR0_HIGH_PHYS_BASE + 0xff,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device dove_xor0_shared = {
	.name		= MV_XOR_SHARED_NAME,
	.id		= 0,
	.dev		= {
		.platform_data = &dove_xor_shared_data,
	},
	.num_resources	= ARRAY_SIZE(dove_xor0_shared_resources),
	.resource	= dove_xor0_shared_resources,
};

static struct resource dove_xor00_resources[] = {
	[0] = {
		.start	= IRQ_DOVE_XOR_00,
		.end	= IRQ_DOVE_XOR_00,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct mv_xor_platform_data dove_xor00_data = {
	.shared		= &dove_xor0_shared,
	.hw_id		= 0,
	.pool_size	= PAGE_SIZE,
};

static struct platform_device dove_xor00_channel = {
	.name		= MV_XOR_NAME,
	.id		= 0,
	.num_resources	= ARRAY_SIZE(dove_xor00_resources),
	.resource	= dove_xor00_resources,
	.dev		= {
		.dma_mask		= &dove_xor0_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(64),
		.platform_data		= (void *)&dove_xor00_data,
	},
};

static struct resource dove_xor01_resources[] = {
	[0] = {
		.start	= IRQ_DOVE_XOR_01,
		.end	= IRQ_DOVE_XOR_01,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct mv_xor_platform_data dove_xor01_data = {
	.shared		= &dove_xor0_shared,
	.hw_id		= 1,
	.pool_size	= PAGE_SIZE,
};

static struct platform_device dove_xor01_channel = {
	.name		= MV_XOR_NAME,
	.id		= 1,
	.num_resources	= ARRAY_SIZE(dove_xor01_resources),
	.resource	= dove_xor01_resources,
	.dev		= {
		.dma_mask		= &dove_xor0_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(64),
		.platform_data		= (void *)&dove_xor01_data,
	},
};

void __init dove_xor0_init(void)
{
	if(useHalDrivers) {
		platform_device_register_simple("mv_xor_hal", -1, NULL, 0);
	} else {
		platform_device_register(&dove_xor0_shared);

		/*
		 * two engines can't do memset simultaneously, this limitation
		 * satisfied by removing memset support from one of the engines.
		 */
		dma_cap_set(DMA_MEMCPY, dove_xor00_data.cap_mask);
		dma_cap_set(DMA_XOR, dove_xor00_data.cap_mask);
		platform_device_register(&dove_xor00_channel);

		dma_cap_set(DMA_MEMCPY, dove_xor01_data.cap_mask);
		dma_cap_set(DMA_MEMSET, dove_xor01_data.cap_mask);
		dma_cap_set(DMA_XOR, dove_xor01_data.cap_mask);
		platform_device_register(&dove_xor01_channel);
	}
}

/*****************************************************************************
 * XOR 1
 ****************************************************************************/
static u64 dove_xor1_dmamask = DMA_BIT_MASK(32);

static struct resource dove_xor1_shared_resources[] = {
	{
		.name	= "xor 0 low",
		.start	= DOVE_XOR1_PHYS_BASE,
		.end	= DOVE_XOR1_PHYS_BASE + 0xff,
		.flags	= IORESOURCE_MEM,
	}, {
		.name	= "xor 0 high",
		.start	= DOVE_XOR1_HIGH_PHYS_BASE,
		.end	= DOVE_XOR1_HIGH_PHYS_BASE + 0xff,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device dove_xor1_shared = {
	.name		= MV_XOR_SHARED_NAME,
	.id		= 1,
	.dev		= {
		.platform_data = &dove_xor_shared_data,
	},
	.num_resources	= ARRAY_SIZE(dove_xor1_shared_resources),
	.resource	= dove_xor1_shared_resources,
};

static struct resource dove_xor10_resources[] = {
	[0] = {
		.start	= IRQ_DOVE_XOR_10,
		.end	= IRQ_DOVE_XOR_10,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct mv_xor_platform_data dove_xor10_data = {
	.shared		= &dove_xor1_shared,
	.hw_id		= 0,
	.pool_size	= PAGE_SIZE,
};

static struct platform_device dove_xor10_channel = {
	.name		= MV_XOR_NAME,
	.id		= 2,
	.num_resources	= ARRAY_SIZE(dove_xor10_resources),
	.resource	= dove_xor10_resources,
	.dev		= {
		.dma_mask		= &dove_xor1_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(64),
		.platform_data		= (void *)&dove_xor10_data,
	},
};

static struct resource dove_xor11_resources[] = {
	[0] = {
		.start	= IRQ_DOVE_XOR_11,
		.end	= IRQ_DOVE_XOR_11,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct mv_xor_platform_data dove_xor11_data = {
	.shared		= &dove_xor1_shared,
	.hw_id		= 1,
	.pool_size	= PAGE_SIZE,
};

static struct platform_device dove_xor11_channel = {
	.name		= MV_XOR_NAME,
	.id		= 3,
	.num_resources	= ARRAY_SIZE(dove_xor11_resources),
	.resource	= dove_xor11_resources,
	.dev		= {
		.dma_mask		= &dove_xor1_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(64),
		.platform_data		= (void *)&dove_xor11_data,
	},
};

void __init dove_xor1_init(void)
{
	if(useHalDrivers)
		dove_xor_shared_data.dram = NULL;
	platform_device_register(&dove_xor1_shared);

	/*
	 * two engines can't do memset simultaneously, this limitation
	 * satisfied by removing memset support from one of the engines.
	 */
	dma_cap_set(DMA_MEMCPY, dove_xor10_data.cap_mask);
	dma_cap_set(DMA_XOR, dove_xor10_data.cap_mask);
	platform_device_register(&dove_xor10_channel);

	dma_cap_set(DMA_MEMCPY, dove_xor11_data.cap_mask);
	dma_cap_set(DMA_MEMSET, dove_xor11_data.cap_mask);
	dma_cap_set(DMA_XOR, dove_xor11_data.cap_mask);
	platform_device_register(&dove_xor11_channel);
}

/*****************************************************************************
 * General
 ****************************************************************************/
static struct gpio_keys_button gpio_standby_button = {
	.code			= KEY_POWER,
	.desc			= "standby",
	.type			= EV_KEY,
	.active_low		= 1,
	.wakeup			= 1,
};

static struct gpio_keys_platform_data gpio_standby_data = {
	.buttons  = &gpio_standby_button,
	.nbuttons = 1,
};

static struct platform_device gpio_standby_keys = {
	.name = "gpio-keys",
	.dev  = {
		.platform_data = &gpio_standby_data,
	},
	.id   = 0,
};

void __init dove_wakeup_button_setup(int gpio)
{
	orion_gpio_set_valid(gpio, 1);
	gpio_standby_button.gpio = gpio;
	platform_device_register(&gpio_standby_keys);
}

/*
 * This fixup function is used to reserve memory for the GPU and VPU engines
 * as these drivers require large chunks of consecutive memory.
 */
void __init dove_tag_fixup_mem32(struct machine_desc *mdesc, struct tag *t,
		char **from, struct meminfo *meminfo)
{
//	struct tag *last_tag = NULL;
	int total_size = pvt_size + vmeta_size + gpu_size;
	struct membank *bank = &meminfo->bank[meminfo->nr_banks];
	int i;

	for (i = 0; i < meminfo->nr_banks; i++) {
		bank--;
		if (bank->size >= total_size)
			break;
	}
	if (i >= meminfo->nr_banks) {
		early_printk(KERN_WARNING "No suitable memory bank was found, "
				"required memory %d MB.\n", total_size);
		vmeta_size = 0;
		gpu_size = 0;
		return;
	}

	/* Resereve memory from last bank for PVT tests	*/
	bank->size -= pvt_size;

	/* Resereve memory from last bank for VPU usage.	*/
	bank->size -= vmeta_size;
	dove_vmeta_memory_start = bank->start + bank->size;

	/* Reserve memory for gpu usage */
	bank->size -= gpu_size;
	dove_gpu_memory_start = bank->start + bank->size;
}

static struct resource dove_ac97_resources[] = {
        [0] = {
                .start  = DOVE_AC97_PHYS_BASE,
                .end    = DOVE_AC97_PHYS_BASE + 0xfff,
                .flags  = IORESOURCE_MEM,
        },
        [1] = {
                .start  = IRQ_AC97,
                .end    = IRQ_AC97,
                .flags  = IORESOURCE_IRQ,
        },
};

static u64 dove_ac97_dmamask = DMA_BIT_MASK(32);


struct platform_device dove_device_ac97 = {
	.name           = "pxa2xx-ac97",
	.id             = -1,
	.dev            = {
		.dma_mask = &dove_ac97_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
	.num_resources  = ARRAY_SIZE(dove_ac97_resources),
	.resource       = dove_ac97_resources,
};

void __init dove_ac97_setup(void)
{
	uint32_t reg;
	/* Enable AC97 Control 			*/
	reg = readl(DOVE_SB_REGS_VIRT_BASE + MPP_GENERAL_CONTROL_REG);
	if ((reg & 0x10000) == 0) {
		printk("\nError: AC97 is disabled according to sample at reset "
			"option (Reg 0x%x).\n", MPP_GENERAL_CONTROL_REG);
		return;
	}

	platform_device_register(&dove_device_ac97);
}

#ifdef CONFIG_PM
static u32 dove_arbitration_regs[] = {
	DOVE_MC_VIRT_BASE + 0x280,
	DOVE_MC_VIRT_BASE + 0x510,
};
#endif
void __init dove_config_arbitration(void)
{
	u32 sc_dec;

	sc_dec = readl(DOVE_MC_VIRT_BASE + 0x280);
	printk("DOVE_MC @ 0x280 is %08X\n", sc_dec);
	#ifdef CONFIG_DOVE_REV_Y0
	sc_dec &= 0xfff0ffff;
	sc_dec |= 0x000e0000;
	#endif
	writel(sc_dec, DOVE_MC_VIRT_BASE + 0x280);
	
        /* Dove Z0 and Z1
        * Master 0 - Vmeta
        * Master 1 - GC500
        * Master 2 - LCD
        * Master 3 - Upstream (SB)
        */

	/* Dove Y0
 	 * MC Master 0 - CPU
 	 * MC Master 1 - vmeta-GC-UP
 	 * MC Master 2 - LCD
 	 */
        /*
  	 * MC Master 1
         * Master 0 - Vmeta
	 * Master 1 - GC500
	 * Master 2 - LCD
	 * Master 3 - Upstream (SB)
	 */

        sc_dec = readl(DOVE_MC_VIRT_BASE + 0x510);
        printk("PLiao: DOVE_MC @ 0x510 is %08X\n", sc_dec);
	
	sc_dec &= 0xf0f0f0f0;
#ifdef CONFIG_DOVE_REV_Y0
	sc_dec |= 0x01010101;
#endif
        writel(sc_dec, DOVE_MC_VIRT_BASE + 0x510);
        /* End of supersection testing */
#ifdef CONFIG_PM
	pm_registers_add(dove_arbitration_regs,
			 ARRAY_SIZE(dove_arbitration_regs));
#endif
}

#ifdef CONFIG_DEBUG_LL
extern void printascii(const char *);
static void check_cpu_mode(void)
{
		u32 cpu_id_code_ext;
		int cpu_mode = 0;
		asm volatile("mrc p15, 1, %0, c15, c12, 0": "=r"(cpu_id_code_ext));

		if (((cpu_id_code_ext >> 16) & 0xF) == 0x2)
			cpu_mode = 6;
		else if (((cpu_id_code_ext >> 16) & 0xF) == 0x3)
			cpu_mode = 7;
		else 
			pr_err("unknow cpu mode!!!\n");

#ifdef CONFIG_DOVE_DEBUGGER_MODE_V6
		if (cpu_mode != 6) {
			printascii("cpu mode (ARMv7) doesn't mach kernel configuration\n");
			panic("cpu mode mismatch");
		}
#else
#ifdef CONFIG_CPU_V7
		if (cpu_mode != 7) {
			printascii("cpu mode (ARMv6) doesn't mach kernel configuration\n");
			panic("cpu mode mismatch");
		}
#endif
#endif
}
#endif
void __init dove_init(void)
{
	int tclk;
	struct clk *clk;

	dove_devclks_init();
	clk = clk_get(NULL, "tclk");
	if (IS_ERR(clk)) {
	     printk(KERN_ERR "failed to get tclk \n");
	     return;
	}
	tclk = clk_get_rate(clk);

	printk(KERN_INFO "Dove %s SoC, ", dove_id());
	printk("TCLK = %dMHz\n", (tclk + 499999) / 1000000);
#ifdef CONFIG_DEBUG_LL
	check_cpu_mode();
#endif
	dove_setup_cpu_mbus();
	dove_config_arbitration();
#if defined(CONFIG_CACHE_TAUROS2)
	if (!noL2)
		tauros2_init();
#endif
	dove_clk_config(NULL, "GCCLK", 500*1000000);
	dove_clk_config(NULL, "AXICLK", 333*1000000);

#if 1
	dove_ge00_shared_data.t_clk = tclk;
#endif
	dove_uart0_data[0].uartclk = tclk;
	dove_uart1_data[0].uartclk = tclk;
	dove_uart2_data[0].uartclk = tclk;
	dove_uart3_data[0].uartclk = tclk;
	dove_spi0_data.tclk = tclk;
	dove_spi1_data.tclk = tclk;
}

#ifdef CONFIG_PM
void dove_save_cpu_conf_regs(void)
{
	DOVE_DWNSTRM_BRDG_SAVE(CPU_CONFIG);
	DOVE_DWNSTRM_BRDG_SAVE(CPU_CONTROL);
	DOVE_DWNSTRM_BRDG_SAVE(RSTOUTn_MASK);
	DOVE_DWNSTRM_BRDG_SAVE(BRIDGE_MASK);
	DOVE_DWNSTRM_BRDG_SAVE(POWER_MANAGEMENT);
}

void dove_restore_cpu_conf_regs(void)
{
	DOVE_DWNSTRM_BRDG_RESTORE(CPU_CONFIG);
	DOVE_DWNSTRM_BRDG_RESTORE(CPU_CONTROL);
	DOVE_DWNSTRM_BRDG_RESTORE(RSTOUTn_MASK);
	DOVE_DWNSTRM_BRDG_RESTORE(BRIDGE_MASK);
	DOVE_DWNSTRM_BRDG_RESTORE(POWER_MANAGEMENT);
}

void dove_save_upstream_regs(void)
{
	DOVE_UPSTRM_BRDG_SAVE(UPSTRM_AXI_P_D_CTRL_REG);
	DOVE_UPSTRM_BRDG_SAVE(UPSTRM_D2X_ARB_LO_REG);
	DOVE_UPSTRM_BRDG_SAVE(UPSTRM_D2X_ARB_HI_REG);
}

void dove_restore_upstream_regs(void)
{
	DOVE_UPSTRM_BRDG_RESTORE(UPSTRM_AXI_P_D_CTRL_REG);
	DOVE_UPSTRM_BRDG_RESTORE(UPSTRM_D2X_ARB_LO_REG);
	DOVE_UPSTRM_BRDG_RESTORE(UPSTRM_D2X_ARB_HI_REG);
}

void dove_save_timer_regs(void)
{
	DOVE_DWNSTRM_BRDG_SAVE(TIMER_CTRL);
	DOVE_DWNSTRM_BRDG_SAVE(TIMER0_RELOAD);
	DOVE_DWNSTRM_BRDG_SAVE(TIMER1_RELOAD);
	DOVE_DWNSTRM_BRDG_SAVE(TIMER_WD_RELOAD);
}

void dove_restore_timer_regs(void)
{
	DOVE_DWNSTRM_BRDG_RESTORE(TIMER_CTRL);
	DOVE_DWNSTRM_BRDG_RESTORE(TIMER0_RELOAD);
	DOVE_DWNSTRM_BRDG_RESTORE(TIMER1_RELOAD);
	DOVE_DWNSTRM_BRDG_RESTORE(TIMER_WD_RELOAD);
}

void dove_save_int_regs(void)
{
	DOVE_DWNSTRM_BRDG_SAVE(IRQ_MASK_LOW);
	DOVE_DWNSTRM_BRDG_SAVE(FIQ_MASK_LOW);
        DOVE_DWNSTRM_BRDG_SAVE(ENDPOINT_MASK_LOW);
	DOVE_DWNSTRM_BRDG_SAVE(IRQ_MASK_HIGH);
	DOVE_DWNSTRM_BRDG_SAVE(FIQ_MASK_HIGH);
	DOVE_DWNSTRM_BRDG_SAVE(ENDPOINT_MASK_HIGH);
	DOVE_DWNSTRM_BRDG_SAVE(PCIE_INTERRUPT_MASK);
}

void dove_restore_int_regs(void)
{
	DOVE_DWNSTRM_BRDG_RESTORE(IRQ_MASK_LOW);
	DOVE_DWNSTRM_BRDG_RESTORE(FIQ_MASK_LOW);
	DOVE_DWNSTRM_BRDG_RESTORE(ENDPOINT_MASK_LOW);
	DOVE_DWNSTRM_BRDG_RESTORE(IRQ_MASK_HIGH);
	DOVE_DWNSTRM_BRDG_RESTORE(FIQ_MASK_HIGH);
	DOVE_DWNSTRM_BRDG_RESTORE(ENDPOINT_MASK_HIGH);
	DOVE_DWNSTRM_BRDG_RESTORE(PCIE_INTERRUPT_MASK);
}
#endif



#ifdef CONFIG_USB_ANDROID
static char *usb_functions[] = {
#ifdef CONFIG_USB_ANDROID_ADB
        "adb",
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
        "rndis",
#endif
#ifdef CONFIG_USB_ANDROID_ACM
        "acm",
#endif
#if defined(CONFIG_USB_ANDROID_MASS_STORAGE) || defined(CONFIG_USB_FILE_STORAG)
        "usb_mass_storage",
#endif
};

#if defined(CONFIG_USB_ANDROID_MASS_STORAGE) || defined(CONFIG_USB_FILE_STORAG)
static struct usb_mass_storage_platform_data mass_storage_pdata = {
        .nluns = 2,
        .vendor = "Marvell     ",
        .product = "Android   ",
        .release = 0x0100,
};


static struct platform_device usb_mass_storage_device = {
        .name = "usb_mass_storage",
        .id = -1,
        .dev = {
                .platform_data = &mass_storage_pdata,
        },
};
#endif

static struct android_usb_platform_data android_usb_pdata = {
        .vendor_id      = 0x0bb4, //temporary use HTC vendor name
        .product_id     = 0x0c01,
        .version        = 0x0100,
        .product_name   = "Android",
        .manufacturer_name = "Marvell",
        .num_functions  = ARRAY_SIZE(usb_functions),
        .functions      = &usb_functions,
};

static struct platform_device android_device_usb = {
        .name   = "android_usb",
        .id             = -1,
        .dev            = {
                .platform_data = &android_usb_pdata,
        },
};


void android_add_usb_devices(void)
{
#if defined(CONFIG_USB_ANDROID_MASS_STORAGE) || defined(CONFIG_USB_FILE_STORAG)
        platform_device_register(&usb_mass_storage_device);
#endif
        platform_device_register(&android_device_usb);
}


struct platform_device dove_udc = {
        .name           = "mv_udc",
        .id             = -1,
        .resource       = dove_ehci1_resources,
        .num_resources  = ARRAY_SIZE(dove_ehci1_resources),

};

void __init dove_udc_init(void)
{
        platform_device_register(&dove_udc);
}

#endif
