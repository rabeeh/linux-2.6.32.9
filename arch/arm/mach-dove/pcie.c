/*
 * arch/arm/mach-dove/pcie.c
 *
 * PCIe functions for Marvell Dove 88F6781 SoC
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/mbus.h>
#include <linux/delay.h>
#include <asm/mach/pci.h>
#include <asm/mach/arch.h>
#include <asm/setup.h>
#include <plat/pcie.h>
#include <mach/irqs.h>
#include <mach/bridge-regs.h>
#include "common.h"

struct pcie_port {
	u8			index;
	u8			root_bus_nr;
	void __iomem		*base;
	spinlock_t		conf_lock;
	char			io_space_name[16];
	char			mem_space_name[16];
	struct resource		res[2];
};

static struct pcie_port pcie_port[2];
static int num_pcie_ports;

static unsigned int noscan = 0;
module_param(noscan, uint, 0);
MODULE_PARM_DESC(noscan, "set to 1 to disable pci scan.");

#define PCIE_BASE	((void __iomem *)DOVE_PCIE0_VIRT_BASE)

#define PCIE_PHY_INDIRECT_ACCESS_REG	0x1B00
#define  PCIE_PHY_INDIRECT_ACCESS_RD_FLAG	(1 << 31)
#define  PCIE_PHY_INDIRECT_ACCESS_OFFSET	16

void __init dove_pcie_id(u32 *dev, u32 *rev)
{
	*dev = orion_pcie_dev_id(PCIE_BASE);
	*rev = orion_pcie_rev(PCIE_BASE);
}

/*
 * Optimal setting for PCIe clocks current drive - 13.5mA
 */
static void dove_pcie_clk_out_config(int nr)
{
	u32 reg = readl(DOVE_PCIE_PORT_CONTROL);
	reg |= 3 << (nr * 2);
	writel(reg, DOVE_PCIE_PORT_CONTROL);
}

static void dove_pcie_tune_phy(void __iomem *base)
{
	u32 reg;
	/* request read access of register 0x92 */
	writel(PCIE_PHY_INDIRECT_ACCESS_RD_FLAG|
	       0x92 << PCIE_PHY_INDIRECT_ACCESS_OFFSET
	       , base + PCIE_PHY_INDIRECT_ACCESS_REG);
	reg = readl(base + PCIE_PHY_INDIRECT_ACCESS_REG);
	/* set bit 7:0 to 0x8B */
	reg &= ~0xFF;
	reg |= 0x8B;
	/* clear the read flag */
	reg &= ~PCIE_PHY_INDIRECT_ACCESS_RD_FLAG;
	writel(reg, base + PCIE_PHY_INDIRECT_ACCESS_REG);
}
static int __init dove_pcie_setup(int nr, struct pci_sys_data *sys)
{
	struct pcie_port *pp;

	if (nr >= num_pcie_ports)
		return 0;

	pp = &pcie_port[nr];
	pp->root_bus_nr = sys->busnr;

	/* dove specific clock settings */
	dove_pcie_clk_out_config(nr);

	/* dove specific phy settings */
	dove_pcie_tune_phy(pp->base);

	/*
	 * Generic PCIe unit setup.
	 */
	orion_pcie_set_local_bus_nr(pp->base, sys->busnr);
#ifdef CONFIG_MV_HAL_DRIVERS_SUPPORT
	orion_pcie_setup(pp->base, NULL);
#else
	orion_pcie_setup(pp->base, &dove_mbus_dram_info);
#endif

	/*
	 * IORESOURCE_IO
	 */
	snprintf(pp->io_space_name, sizeof(pp->io_space_name),
		 "PCIe %d I/O", pp->index);
	pp->io_space_name[sizeof(pp->io_space_name) - 1] = 0;
	pp->res[0].name = pp->io_space_name;
	if (pp->index == 0) {
		pp->res[0].start = DOVE_PCIE0_IO_BUS_BASE;
		pp->res[0].end = pp->res[0].start + DOVE_PCIE0_IO_SIZE - 1;
	} else {
		pp->res[0].start = DOVE_PCIE1_IO_BUS_BASE;
		pp->res[0].end = pp->res[0].start + DOVE_PCIE1_IO_SIZE - 1;
	}
	pp->res[0].flags = IORESOURCE_IO;
	if (request_resource(&ioport_resource, &pp->res[0]))
		panic("Request PCIe IO resource failed\n");
	sys->resource[0] = &pp->res[0];

	/*
	 * IORESOURCE_MEM
	 */
	snprintf(pp->mem_space_name, sizeof(pp->mem_space_name),
		 "PCIe %d MEM", pp->index);
	pp->mem_space_name[sizeof(pp->mem_space_name) - 1] = 0;
	pp->res[1].name = pp->mem_space_name;
	if (pp->index == 0) {
		pp->res[1].start = DOVE_PCIE0_MEM_PHYS_BASE;
		pp->res[1].end = pp->res[1].start + DOVE_PCIE0_MEM_SIZE - 1;
	} else {
		pp->res[1].start = DOVE_PCIE1_MEM_PHYS_BASE;
		pp->res[1].end = pp->res[1].start + DOVE_PCIE1_MEM_SIZE - 1;
	}
	pp->res[1].flags = IORESOURCE_MEM;
	if (request_resource(&iomem_resource, &pp->res[1]))
		panic("Request PCIe Memory resource failed\n");
	sys->resource[1] = &pp->res[1];

	sys->resource[2] = NULL;

	return 1;
}

static struct pcie_port *bus_to_port(int bus)
{
	int i;

	for (i = num_pcie_ports - 1; i >= 0; i--) {
		int rbus = pcie_port[i].root_bus_nr;
		if (rbus != -1 && rbus <= bus)
			break;
	}

	return i >= 0 ? pcie_port + i : NULL;
}

static int pcie_valid_config(struct pcie_port *pp, int bus, int dev)
{
	/*
	 * Don't go out when trying to access nonexisting devices
	 * on the local bus.
	 */
	if (bus == pp->root_bus_nr && dev > 1)
		return 0;

	return 1;
}

static int pcie_rd_conf(struct pci_bus *bus, u32 devfn, int where,
			int size, u32 *val)
{
	struct pcie_port *pp = bus_to_port(bus->number);
	unsigned long flags;
	int ret;

	if (noscan)
		return PCIBIOS_DEVICE_NOT_FOUND;

	if (pcie_valid_config(pp, bus->number, PCI_SLOT(devfn)) == 0) {
		*val = 0xffffffff;
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	spin_lock_irqsave(&pp->conf_lock, flags);
	ret = orion_pcie_rd_conf(pp->base, bus, devfn, where, size, val);
	spin_unlock_irqrestore(&pp->conf_lock, flags);

	return ret;
}

static int pcie_wr_conf(struct pci_bus *bus, u32 devfn,
			int where, int size, u32 val)
{
	struct pcie_port *pp = bus_to_port(bus->number);
	unsigned long flags;
	int ret;

	if (pcie_valid_config(pp, bus->number, PCI_SLOT(devfn)) == 0)
		return PCIBIOS_DEVICE_NOT_FOUND;

	spin_lock_irqsave(&pp->conf_lock, flags);
	ret = orion_pcie_wr_conf(pp->base, bus, devfn, where, size, val);
	spin_unlock_irqrestore(&pp->conf_lock, flags);

	return ret;
}

static struct pci_ops pcie_ops = {
	.read = pcie_rd_conf,
	.write = pcie_wr_conf,
};

static void __devinit rc_pci_fixup(struct pci_dev *dev)
{
	/*
	 * Prevent enumeration of root complex.
	 */
	if (dev->bus->parent == NULL && dev->devfn == 0) {
		int i;

		for (i = 0; i < DEVICE_COUNT_RESOURCE; i++) {
			dev->resource[i].start = 0;
			dev->resource[i].end   = 0;
			dev->resource[i].flags = 0;
		}
	}
}
DECLARE_PCI_FIXUP_HEADER(PCI_VENDOR_ID_MARVELL, PCI_ANY_ID, rc_pci_fixup);

static struct pci_bus __init *
dove_pcie_scan_bus(int nr, struct pci_sys_data *sys)
{
	struct pci_bus *bus;

	if (nr < num_pcie_ports) {
		bus = pci_scan_bus(sys->busnr, &pcie_ops, sys);
	} else {
		bus = NULL;
		BUG();
	}

	return bus;
}

static int __init dove_pcie_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	struct pcie_port *pp = bus_to_port(dev->bus->number);
     
	return pp->index ? IRQ_DOVE_PCIE1 : IRQ_DOVE_PCIE0;
}

static struct hw_pci dove_pci __initdata = {
	.nr_controllers	= 2,
	.swizzle	= pci_std_swizzle,
	.setup		= dove_pcie_setup,
	.scan		= dove_pcie_scan_bus,
	.map_irq	= dove_pcie_map_irq,
};

static void __init add_pcie_port(int index, unsigned long base)
{
	printk(KERN_INFO "Dove PCIe port %d: ", index);

	if (orion_pcie_link_up((void __iomem *)base)) {
		struct pcie_port *pp = &pcie_port[num_pcie_ports++];

		printk("link up\n");

		pp->index = index;
		pp->root_bus_nr = -1;
		pp->base = (void __iomem *)base;
		spin_lock_init(&pp->conf_lock);
		memset(pp->res, 0, sizeof(pp->res));
	} else {
		printk("link down, ignoring\n");
	}
}

void __init dove_pcie_init(int init_port0, int init_port1)
{
	if (init_port0)
		add_pcie_port(0, DOVE_PCIE0_VIRT_BASE);

	if (init_port1)
		add_pcie_port(1, DOVE_PCIE1_VIRT_BASE);

	pci_common_init(&dove_pci);
}

#ifdef CONFIG_PM
void dove_save_pcie_regs(void)
{

}

void dove_restore_pcie_regs(void)
{
	u32 i;
	u32 timeout = 50;
	u32 reg;

	/* Configure PCIE ports */
	for (i = 0; i<num_pcie_ports; i++)
	{
		orion_pcie_set_local_bus_nr(pcie_port[i].base, pcie_port[i].root_bus_nr);
		orion_pcie_setup(pcie_port[i].base, &dove_mbus_dram_info);
		dove_pcie_clk_out_config(i);
		dove_pcie_tune_phy(pcie_port[i].base);
	}

	/* Enable Link on both ports */
	reg = readl(CPU_CONTROL);
	reg &= ~(CPU_CTRL_PCIE0_LINK | CPU_CTRL_PCIE1_LINK);
	writel(reg, CPU_CONTROL);

	/*
	 * Loop waiting for link up on the phy of the ports.
	 */

	do {
		int i;
		int links_ready = 1;

		for (i = 0; i < num_pcie_ports; i++)
			if (!orion_pcie_link_up(pcie_port[i].base))
				links_ready = 0;

		if (links_ready)
			break;

		mdelay(1);
	} while (timeout--);
}
#endif
