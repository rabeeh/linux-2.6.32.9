/*
 * arch/arm/mach-dove/addr-map.c
 *
 * Address map functions for Marvell Dove MV88F6781 SoC
 *
 * Author: Tzachi Perelstein <tzachi@marvell.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mbus.h>
#include <asm/mach/arch.h>
//#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/setup.h>
#include "common.h"

/*
 * Generic Address Decode Windows bit settings
 */
#define TARGET_DDR		0x0
#define TARGET_BOOTROM		0x1
#define TARGET_CESA		0x3
#define TARGET_PCIE0		0x4
#define TARGET_PCIE1		0x8
#define TARGET_SCRATCHPAD	0xd

#define ATTR_CESA		0x01
#define ATTR_BOOTROM		0xfd
#define ATTR_DEV_SPI0_ROM	0xfe
#define ATTR_DEV_SPI1_ROM	0xfb
#define ATTR_PCIE_IO		0xe0
#define ATTR_PCIE_MEM		0xe8
#define ATTR_SCRATCHPAD		0x0

/*
 * CPU Address Decode Windows registers
 */
#define WIN_CTRL(n)	(BRIDGE_VIRT_BASE + ((n) << 4) + 0x0)
#define WIN_BASE(n)	(BRIDGE_VIRT_BASE + ((n) << 4) + 0x4)
#define WIN_REMAP_LO(n)	(BRIDGE_VIRT_BASE + ((n) << 4) + 0x8)
#define WIN_REMAP_HI(n)	(BRIDGE_VIRT_BASE + ((n) << 4) + 0xc)

struct mbus_dram_target_info dove_mbus_dram_info;

static inline void __iomem *ddr_map_sc(int i)
{
	return (void __iomem *)(DOVE_MC_VIRT_BASE + 0x100 + ((i) << 4));
}

static int cpu_win_can_remap(int win)
{
	if (win < 4)
		return 1;

	return 0;
}

static void __init setup_cpu_win(int win, u32 base, u32 size,
				 u8 target, u8 attr, int remap)
{
	u32 ctrl;

	base &= 0xffff0000;
	ctrl = ((size - 1) & 0xffff0000) | (attr << 8) | (target << 4) | 1;

	writel(base, WIN_BASE(win));
	writel(ctrl, WIN_CTRL(win));
	if (cpu_win_can_remap(win)) {
		if (remap < 0)
			remap = base;
		writel(remap & 0xffff0000, WIN_REMAP_LO(win));
		writel(0, WIN_REMAP_HI(win));
	}
}

void __init dove_setup_cpu_mbus(void)
{
	int i;
	int cs;
	u32 dev, rev;

	/*
	 * First, disable and clear windows.
	 */
	for (i = 0; i < 8; i++) {
		writel(0, WIN_BASE(i));
		writel(0, WIN_CTRL(i));
		if (cpu_win_can_remap(i)) {
			writel(0, WIN_REMAP_LO(i));
			writel(0, WIN_REMAP_HI(i));
		}
	}

	/*
	 * Setup windows for PCIe IO+MEM space.
	 */
	setup_cpu_win(0, DOVE_PCIE0_IO_PHYS_BASE, DOVE_PCIE0_IO_SIZE,
		      TARGET_PCIE0, ATTR_PCIE_IO, DOVE_PCIE0_IO_BUS_BASE);
	setup_cpu_win(1, DOVE_PCIE1_IO_PHYS_BASE, DOVE_PCIE1_IO_SIZE,
		      TARGET_PCIE1, ATTR_PCIE_IO, DOVE_PCIE1_IO_BUS_BASE);
	setup_cpu_win(2, DOVE_PCIE0_MEM_PHYS_BASE, DOVE_PCIE0_MEM_SIZE,
		      TARGET_PCIE0, ATTR_PCIE_MEM, -1);
	setup_cpu_win(3, DOVE_PCIE1_MEM_PHYS_BASE, DOVE_PCIE1_MEM_SIZE,
		      TARGET_PCIE1, ATTR_PCIE_MEM, -1);

	/*
	 * Setup window for CESA engine.
	 */
	setup_cpu_win(4, DOVE_CESA_PHYS_BASE, DOVE_CESA_SIZE,
		      TARGET_CESA, ATTR_CESA, -1);

	/*
	 * Setup the Window to the BootROM for Stanby and Sleep Resume
	 */
	dove_pcie_id(&dev, &rev);
	if (rev >= DOVE_REV_X0) /* For X0 and higher use the internal BootROM */
		setup_cpu_win(5, DOVE_BOOTROM_PHYS_BASE, DOVE_BOOTROM_SIZE,
			      TARGET_BOOTROM, ATTR_BOOTROM, -1);
	else			/* Use the BootROM on the SPI */
		setup_cpu_win(5, DOVE_BOOTROM_PHYS_BASE, DOVE_BOOTROM_SIZE,
			      TARGET_BOOTROM, ATTR_DEV_SPI0_ROM, -1);

	/*
	 * Setup the Window to the PMU Scratch Pad space
	 */
	setup_cpu_win(6, DOVE_SCRATCHPAD_PHYS_BASE, DOVE_SCRATCHPAD_SIZE,
		      TARGET_SCRATCHPAD, ATTR_SCRATCHPAD, -1);

	/*
	 * Setup MBUS dram target info.
	 */
	dove_mbus_dram_info.mbus_dram_target_id = TARGET_DDR;

	for (i = 0, cs = 0; i < 2; i++) {
		u32 map = readl(ddr_map_sc(i));

		/*
		 * Chip select enabled?
		 */
		if (map & 1) {
			struct mbus_dram_window *w;

			w = &dove_mbus_dram_info.cs[cs++];
			w->cs_index = i;
			w->mbus_attr = 0; /* CS address decoding done inside */
					  /* the DDR controller, no need to  */
					  /* provide attributes */
			w->base = map & 0xff800000;
			w->size = 0x100000 << (((map & 0x000f0000) >> 16) - 4);
		}
	}
	dove_mbus_dram_info.num_cs = cs;
}

#ifdef CONFIG_PM
static u32 wins_pm_regs[8*4];	/* max 4 registers per window */

void dove_save_cpu_wins(void)
{
	int i;

	/* Save all all address decode windows */
	for (i = 0; i < 8; i++) {
		wins_pm_regs[i*4] = readl(WIN_CTRL(i));
		wins_pm_regs[i*4+1] = readl(WIN_BASE(i));
		if (cpu_win_can_remap(i)) {
			wins_pm_regs[i*4+2] = readl(WIN_REMAP_LO(i));
			wins_pm_regs[i*4+3] = readl(WIN_REMAP_HI(i));
		}
	}
}

void dove_restore_cpu_wins(void)
{
	int i = 0;

	/* Verify that all windows are closed before overloading them */
	for (i = 0; i < 8; i++) 
		writel(0, WIN_CTRL(i));

	/* Restore original value */
	for (i = 0; i < 8; i++) {
		writel(wins_pm_regs[i*4+1], WIN_BASE(i));
		if (cpu_win_can_remap(i))	{
			writel(wins_pm_regs[i*4+2], WIN_REMAP_LO(i));
			writel(wins_pm_regs[i*4+3], WIN_REMAP_HI(i));
		}
		writel(wins_pm_regs[i*4], WIN_CTRL(i));
	}
}
#if 0
static u32 orion_dev_wins_regs[ORION_DEV_WINS_REG_SAVE_SIZE];

void orion_save_device_wins(void)
{
	/* Save BAR0 used for internal registers */
	ORION_DEV_WINS_REGL_SAVE(PCIE_BAR_LO, 0);
	ORION_DEV_WINS_REGL_SAVE(PCIE_BAR_HI, 0);
}

void orion_restore_device_wins(void)
{
	u32 dev, rev;

	/* Reinitialize the Orion Devices windows to the DDR */
	orion_pcie_id(&dev, &rev);
	orion_setup_device_wins(dev);

	/* Restore BAR0 used for the internal registers */
	ORION_DEV_WINS_REGL_RESTORE(PCIE_BAR_LO, 0);
	ORION_DEV_WINS_REGL_RESTORE(PCIE_BAR_HI, 0);
}
#endif
#endif /* CONFIG_PM */
