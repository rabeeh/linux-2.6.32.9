/*
 * arch/arm/mach-dove/mpp.c
 *
 * MPP functions for Marvell Dove SoCs
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mbus.h>
#include <asm/gpio.h>
//#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <asm/mach/arch.h>
#include <asm/mach-types.h>
#include "common.h"
#include <mach/pm.h>
#include "mpp.h"
#include "ctrlEnv/mvCtrlEnvRegs.h"

#ifdef CONFIG_PM
static u32 dove_mpp_regs[] = {
	DOVE_MPP_VIRT_BASE,
	DOVE_MPP_VIRT_BASE + 4,
	DOVE_MPP_VIRT_BASE + 8,
	DOVE_PMU_MPP_GENERAL_CTRL,
	DOVE_MPP_GENERAL_VIRT_BASE,
	DOVE_MPP_CTRL4_VIRT_BASE,
	DOVE_SSP_CTRL_STATUS_1
};

static u32 dove_pm_gpio_regs[] = DOVE_GPIO_REGISTERS;
#endif

#define DOVE_MPP_MAX_OPTIONS	7
#define DOVE_PMU_MAX_PINS	16

struct mpp_type {
	enum dove_mpp_type	type;
	int			val;
};

#define MPP_LAST	{MPP_END, 0}
#define MPP_LAST2	MPP_LAST, MPP_LAST
#define MPP_LAST3	MPP_LAST, MPP_LAST2
#define MPP_LAST4	MPP_LAST, MPP_LAST3
#define MPP_LAST5	MPP_LAST, MPP_LAST4
#define MPP_LAST6	MPP_LAST, MPP_LAST5

struct mpp_config {
	struct  mpp_type types[DOVE_MPP_MAX_OPTIONS];
	int	(*config)(int mpp, enum dove_mpp_type type, int val);
};

int __init dove_mpp_legacy_config(int mpp, enum dove_mpp_type type, int val);
int __init dove_mpp_high_config(int mpp, enum dove_mpp_type type, int val);
void __init dove_mpp_pmu_config(int mpp, enum dove_mpp_type type);

/* The index is the mpp number
 * The table still not complete
 */
struct mpp_config dove_mpp_table[] __initdata = 
{
	/* MPP 0 */
	{{{MPP_GPIO, 0}, {MPP_PMU, 0}, {MPP_SDIO0, 3}, MPP_LAST3},
	 dove_mpp_legacy_config},
	/* MPP 1 */
	{{{MPP_GPIO, 0}, {MPP_SDIO0, 3}, {MPP_PMU, 0}, {MPP_LCD, 0xf}, MPP_LAST3},
	 dove_mpp_legacy_config},
	/* MPP 2 */
	{{{MPP_GPIO, 0}, {MPP_SATA_PRESENCE, 1}, {MPP_PMU, 0}, MPP_LAST4},
	 dove_mpp_legacy_config},
	/* MPP 3 */
	{{{MPP_GPIO, 0}, {MPP_PMU, 0}, MPP_LAST5},
	 dove_mpp_legacy_config},
	/* MPP 4 */
	{{{MPP_GPIO, 0}, {MPP_PMU, 0}, MPP_LAST5},
	 dove_mpp_legacy_config},
	/* MPP 5 */
	{{{MPP_GPIO, 0}, {MPP_PMU, 0}, MPP_LAST5},
	 dove_mpp_legacy_config},
	/* MPP 6 */
	{{{MPP_GPIO, 0}, {MPP_PMU, 0}, MPP_LAST5},
	 dove_mpp_legacy_config},
	/* MPP 7 */
	{{{MPP_GPIO, 0}, {MPP_PMU, 0}, {MPP_MII, 5}, MPP_LAST4},
	 dove_mpp_legacy_config},
	/* MPP 8 */
	{{{MPP_GPIO, 0}, MPP_LAST6},
	 dove_mpp_legacy_config},
	/* MPP 9 */
	{{{MPP_GPIO, 0}, {MPP_PMU, 0}, {MPP_PCIE, 5}, MPP_LAST4},
	 dove_mpp_legacy_config},
	/* MPP 10 */
	{{{MPP_GPIO, 0}, {MPP_PMU, 0}, MPP_LAST5},
	 dove_mpp_legacy_config},
	/* MPP 11 */
	{{{MPP_GPIO, 0}, {MPP_SATA_PRESENCE, 1}, {MPP_SATA_ACT, 2}, {MPP_SDIO0, 3},
	  {MPP_SDIO1, 4}, MPP_LAST2},
	 dove_mpp_legacy_config},
	/* MPP 12 */
	{{{MPP_GPIO, 0}, {MPP_SDIO1, 4}, MPP_LAST5},
	 dove_mpp_legacy_config},
	/* MPP 13 */
	{{{MPP_GPIO, 0}, {MPP_SDIO1, 4}, MPP_LAST5},
	 dove_mpp_legacy_config},
	/* MPP 14 */
	{{{MPP_GPIO, 0}, {MPP_UART2, 2}, MPP_LAST5},
	 dove_mpp_legacy_config},
	/* MPP 15 */
	{{{MPP_GPIO, 0}, {MPP_UART2, 2}, MPP_LAST5},
	 dove_mpp_legacy_config},
	/* MPP 16 */
	{{{MPP_GPIO, 0}, {MPP_SDIO0, 3}, MPP_LAST5},
	 dove_mpp_legacy_config},
	/* MPP 17 */
	{{{MPP_GPIO, 0}, {MPP_TWSI, 4}, MPP_LAST5},
	 dove_mpp_legacy_config},
	/* MPP 18 */
	{{{MPP_GPIO, 0}, {MPP_UART3, 2}, {MPP_LCD, 4}, MPP_LAST4},
	 dove_mpp_legacy_config},
	/* MPP 19 */
	{{{MPP_GPIO, 0}, {MPP_UART3, 2}, {MPP_TWSI, 4}, {MPP_MII, 6}, MPP_LAST3},
	 dove_mpp_legacy_config},
	/* MPP 20 */
	{{{MPP_GPIO, 0}, {MPP_NB_CLOCK, 4}, {MPP_LCD0_SPI, 2}, {MPP_SPI1, 6}, MPP_LAST3},
	 dove_mpp_legacy_config},
	/* MPP 21 */
	{{{MPP_GPIO, 0}, {MPP_UART1, 1}, {MPP_LCD0_SPI, 2}, {MPP_SPI1, 6}, MPP_LAST3},
	 dove_mpp_legacy_config},
	/* MPP 22 */
	{{{MPP_GPIO, 0}, {MPP_UART1, 1}, {MPP_LCD0_SPI, 2}, {MPP_SPI1, 6}, MPP_LAST3},
	 dove_mpp_legacy_config},
	/* MPP 23 */
	{{{MPP_GPIO, 0}, {MPP_LCD0_SPI, 2}, {MPP_SPI1, 6}, MPP_LAST4},
	 dove_mpp_legacy_config},
	/* MPP 24 */
	{{{MPP_GPIO, 2}, {MPP_CAM, 2}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 25 */
	{{{MPP_GPIO, 2}, {MPP_CAM, 2}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 26 */
	{{{MPP_GPIO, 2}, {MPP_CAM, 2}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 27 */
	{{{MPP_GPIO, 2}, {MPP_CAM, 2}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 28 */
	{{{MPP_GPIO, 2}, {MPP_CAM, 2}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 29 */
	{{{MPP_GPIO, 2}, {MPP_CAM, 2}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 30 */
	{{{MPP_GPIO, 2}, {MPP_CAM, 2}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 31 */
	{{{MPP_GPIO, 2}, {MPP_CAM, 2}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 32 (HIGH 0) */
	{{{MPP_GPIO, 2}, {MPP_CAM, 2}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 33 (HIGH 1) */
	{{{MPP_GPIO, 2}, {MPP_CAM, 2}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 34 (HIGH 2) */
	{{{MPP_GPIO, 2}, {MPP_CAM, 2}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 35 (HIGH 3) */
	{{{MPP_GPIO, 2}, {MPP_CAM, 2}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 36 (HIGH 4) */
	{{{MPP_GPIO, 2}, {MPP_CAM, 2}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 37 (HIGH 5) */
	{{{MPP_GPIO, 2}, {MPP_CAM, 2}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 38 (HIGH 6) */
	{{{MPP_GPIO, 2}, {MPP_CAM, 2}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 39 (HIGH 7) */
	{{{MPP_GPIO, 2}, {MPP_CAM, 2}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 40 (HIGH 8) */
	{{{MPP_GPIO, 0}, {MPP_SDIO0, 0}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 41 (HIGH 9) */
	{{{MPP_GPIO, 0}, {MPP_SDIO0, 0}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 42 (HIGH 10) */
	{{{MPP_GPIO, 0}, {MPP_SDIO0, 0}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 43 (HIGH 11) */
	{{{MPP_GPIO, 0}, {MPP_SDIO0, 0}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 44 (HIGH 12) */
	{{{MPP_GPIO, 0}, {MPP_SDIO0, 0}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 45 (HIGH 13) */
	{{{MPP_GPIO, 0}, {MPP_SDIO0, 0}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 46 (HIGH 14) */
	{{{MPP_GPIO, 1}, {MPP_SDIO1, 1}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 47 (HIGH 15) */
	{{{MPP_GPIO, 1}, {MPP_SDIO1, 1}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 48 (HIGH 16) */
	{{{MPP_GPIO, 1}, {MPP_SDIO1, 1}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 49 (HIGH 17) */
	{{{MPP_GPIO, 1}, {MPP_SDIO1, 1}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 50 (HIGH 18) */
	{{{MPP_GPIO, 1}, {MPP_SDIO1, 1}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 51 (HIGH 19) */
	{{{MPP_GPIO, 1}, {MPP_SDIO1, 1}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 52 (HIGH 20) */
	{{{MPP_GPIO, 3}, {MPP_AUDIO1, 3}, {MPP_SSP, 3}, MPP_LAST4},
	 dove_mpp_high_config},
	/* MPP 53 (HIGH 21) */
	{{{MPP_GPIO, 3}, {MPP_AUDIO1, 3}, {MPP_SSP, 3}, MPP_LAST4},
	 dove_mpp_high_config},
	/* MPP 54 (HIGH 22) */
	{{{MPP_GPIO, 3}, {MPP_AUDIO1, 3}, {MPP_SSP, 3}, MPP_LAST4},
	 dove_mpp_high_config},
	/* MPP 55 (HIGH 23) */
	{{{MPP_GPIO, 3}, {MPP_AUDIO1, 3}, {MPP_SSP, 3}, MPP_LAST4},
	 dove_mpp_high_config},
	/* MPP 56 (HIGH 24) */
	{{{MPP_GPIO, 3}, {MPP_AUDIO1, 3}, {MPP_TWSI, 3}, MPP_LAST4},
	 dove_mpp_high_config},
	/* MPP 57 (HIGH 25) */
	{{{MPP_GPIO, 3}, {MPP_GPIO_AUDIO1, 3}, {MPP_AUDIO1, 3}, {MPP_TWSI, 3}, MPP_LAST3},
	 dove_mpp_high_config},
	/* MPP 58 (HIGH 26) */
	{{{MPP_GPIO, 5}, {MPP_SPI0, 5}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 59 (HIGH 27) */
	{{{MPP_GPIO, 5}, {MPP_SPI0, 5}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 60 (HIGH 28) */
	{{{MPP_GPIO, 5}, {MPP_SPI0, 5}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 61 (HIGH 29) */
	{{{MPP_GPIO, 5}, {MPP_SPI0, 5}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 62 (HIGH 30) */
	{{{MPP_GPIO, 4}, {MPP_UART1, 4}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 63 (HIGH 31) */
	{{{MPP_GPIO, 4}, {MPP_UART1, 4}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 64 (HIGH 32) */
	{{{MPP_GPIO, 0}, {MPP_NFC8_15, 0}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 65 (HIGH 33) */
	{{{MPP_GPIO, 0}, {MPP_NFC8_15, 0}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 66 (HIGH 34) */
	{{{MPP_GPIO, 0}, {MPP_NFC8_15, 0}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 67 (HIGH 35) */
	{{{MPP_GPIO, 0}, {MPP_NFC8_15, 0}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 68 (HIGH 36) */
	{{{MPP_GPIO, 0}, {MPP_NFC8_15, 0}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 69 (HIGH 37) */
	{{{MPP_GPIO, 0}, {MPP_NFC8_15, 0}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 70 (HIGH 38) */
	{{{MPP_GPIO, 0}, {MPP_NFC8_15, 0}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 71 (HIGH 39) */
	{{{MPP_GPIO, 0}, {MPP_NFC8_15, 0}, MPP_LAST5},
	 dove_mpp_high_config},
	/* MPP 72 (HIGH 40) */
	{{{MPP_GPIO, 0}, {MPP_NFC8_15, 0}, MPP_LAST5},
	 dove_mpp_high_config},

};

void __init dove_mpp_conf(struct dove_mpp_mode *mode)
{
	/* Initialize gpiolib. */
	orion_gpio_init();

	while (mode->mpp >= 0) {
		struct mpp_config *entry;
		int i;
		int found = 0;

		if(mode->mpp >= sizeof(dove_mpp_table)) {
			printk("dove_mpp_conf: invalid MPP number (%d)\n",
			       mode->mpp);
			return;
		}
		/* 
		 * First configure PMU/MPP pin selection 
		 * Valid for first 16 pins only
		 */
		if(mode->mpp < DOVE_PMU_MAX_PINS)
			dove_mpp_pmu_config(mode->mpp, mode->type);	

		entry = &dove_mpp_table[mode->mpp];

		for (i = 0; i < DOVE_MPP_MAX_OPTIONS; i++) {
			if (entry->types[i].type == mode->type) {
				found = 1;
				break;
			}
			if (entry->types[i].type == MPP_END)
				break;
		}

		if (found) {
			if (entry->config)
				entry->config(mode->mpp,
					      entry->types[i].type,
					      entry->types[i].val);

			if (mode->type == MPP_UNUSED)
				orion_gpio_set_unused(mode->mpp);
			if ((mode->type == MPP_GPIO) || (mode->type == MPP_GPIO_AUDIO1))
				orion_gpio_set_valid(mode->mpp, 1);
			else
				orion_gpio_set_valid(mode->mpp, 0);
		} else {
			printk("dove_mpp_conf: MPP[%d] type %d not supported\n",
			       mode->mpp ,mode->type);
			return;
		}
		mode++;
	}
#ifdef CONFIG_PM
	pm_registers_add(dove_mpp_regs, ARRAY_SIZE(dove_mpp_regs));
	pm_registers_add(dove_pm_gpio_regs, ARRAY_SIZE(dove_pm_gpio_regs));
#endif
}

/* This function configures mpp mode for mpps 0-23 */
int __init dove_mpp_legacy_config(int mpp, enum dove_mpp_type type, int val)
{
	u32 mpp_ctrl_reg;
	u32 mpp_ctrl;
	int shift;
	
	switch (mpp) {
	case 0 ... 7:
		mpp_ctrl_reg = DOVE_MPP_VIRT_BASE;
		break;
	case 8 ... 15:
		mpp_ctrl_reg = DOVE_MPP_VIRT_BASE + 4;
		break;
	case 16 ... 23:
		mpp_ctrl_reg = DOVE_MPP_VIRT_BASE + 8;
		break;
	default:
		printk(KERN_ERR "%s: invalid MPP (%d)\n", __func__, mpp);
		return -1;
	}
	
	mpp_ctrl = readl(mpp_ctrl_reg);

	shift = (mpp & 7) << 2;
	mpp_ctrl &= ~(0xf << shift);
	mpp_ctrl |= (val & 0xf) << shift;
		
	writel(mpp_ctrl, mpp_ctrl_reg);
	return 0;
}

/* This function configures mpp mode for mpps >= 24 */
int __init dove_mpp_high_config(int mpp, enum dove_mpp_type type, int mask)
{
	u32 mpp_ctrl = readl(DOVE_MPP_CTRL4_VIRT_BASE);

	if (type == MPP_GPIO)
	mpp_ctrl |= 0x1 << mask;
	else
	mpp_ctrl &= ~(0x1 << mask);

	writel(mpp_ctrl, DOVE_MPP_CTRL4_VIRT_BASE);

	if (mpp >= 52 && mpp <= 55) {
		u32 ssp_ctrl = readl(DOVE_SSP_CTRL_STATUS_1);
		if (machine_is_dove_avng_v3())
		{
			if (type != MPP_AUDIO1)
				ssp_ctrl &= ~DOVE_SSP_ON_AU1;
			else
				ssp_ctrl |= DOVE_SSP_ON_AU1;
		}
		else
		{
			if (type == MPP_AUDIO1)
				ssp_ctrl &= ~DOVE_SSP_ON_AU1;
			else
				ssp_ctrl |= DOVE_SSP_ON_AU1;
		}
		writel(ssp_ctrl, DOVE_SSP_CTRL_STATUS_1);
	}
	if (mpp >= 56 && mpp <= 57) {
		u32 global_config_2 = readl(DOVE_GLOBAL_CONFIG_2);

		if (type == MPP_TWSI)
			global_config_2 |= DOVE_TWSI_OPTION3_GPIO;
		else
			global_config_2 &= ~DOVE_TWSI_OPTION3_GPIO;
		writel(global_config_2, DOVE_GLOBAL_CONFIG_2);
	}
	if (mpp == 57) {
		u32 mpp_general_config = readl(DOVE_MPP_GENERAL_VIRT_BASE);
		if (type == MPP_GPIO_AUDIO1)
			mpp_general_config |= DOVE_AU1_SPDIFO_GPIO_EN;
		else
			mpp_general_config &= ~DOVE_AU1_SPDIFO_GPIO_EN;
		writel(mpp_general_config, DOVE_MPP_GENERAL_VIRT_BASE);
	}
	if (mpp >= 64 && mpp <= 72) {
		u32 mpp_general_config = readl(DOVE_MPP_GENERAL_VIRT_BASE);
		if (type == MPP_GPIO)
			mpp_general_config |= 0x1 << mask;
		else
			mpp_general_config &= ~(0x1 << mask);
		writel(mpp_general_config, DOVE_MPP_GENERAL_VIRT_BASE);
	}

	return 0;
}

/* This function routes MPP to PMU directly */
void __init dove_mpp_pmu_config(int mpp, enum dove_mpp_type type)
{
	u32 pmu_mpp_ctrl;
	
	pmu_mpp_ctrl = readl(DOVE_PMU_MPP_GENERAL_CTRL);
	if (type == MPP_PMU)
		pmu_mpp_ctrl |= (1 << mpp);
	else
		pmu_mpp_ctrl &= ~(1 << mpp);
	writel(pmu_mpp_ctrl, DOVE_PMU_MPP_GENERAL_CTRL);
}
