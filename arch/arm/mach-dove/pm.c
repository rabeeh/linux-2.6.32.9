/*
 * pm.c
 *
 * Power Management functions for Marvell Dove System On Chip
 *
 * Maintainer: Tawfik Bayouk <tawfik@marvell.com>
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#undef DEBUG

#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <linux/suspend.h>
#include <linux/interrupt.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <asm/hardware/cache-tauros2.h>
#include <asm/mach/arch.h>
#include <asm-arm/vfp.h>
#include "mvOs.h"
#include "pmu/mvPmu.h"
#include "pmu/mvPmuRegs.h"
#include "gpp/mvGppRegs.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "ctrlEnv/sys/mvCpuIfRegs.h"
#include "common.h"
#include "mpp.h"
#include "twsi.h"
#define CONFIG_PMU_PROC

/*
 * Global holding the target PM state
 * This should be replaced by the PMU flag register
 */
static suspend_state_t dove_target_pm_state = PM_SUSPEND_ON;

/* manage generic interface for list of registers to save & restore */
static LIST_HEAD(registers_list);
static DEFINE_MUTEX(registers_mutex);

struct regs_entry {
	struct list_head	node;
	union {
		u32		*regs_addresses;
		u32		reg_address;
	};
	union {      
		u32		*regs_values;
		u32		reg_value;
	};
	int		count;
	int		single;
};

enum pm_action_type {
	SAVE,
	RESTORE
};
static int __pm_registers_add(u32 *registers, int count, int single)
{
	struct regs_entry *entry;
	
	if (!count)
		return 0;

	entry = kzalloc(sizeof(struct regs_entry), GFP_KERNEL);
	if (!entry)
		return -ENOMEM;
	
	if (single)
		entry->reg_address = *registers;
	else
		entry->regs_addresses = registers;

	entry->count = count;
	entry->single = single;
	
	if (!single) {
		entry->regs_values = kmalloc(sizeof(u32) * count, GFP_KERNEL);
		if (!entry->regs_values) {
			kfree(entry);
			return -ENOMEM;
		}
	}

	mutex_lock(&registers_mutex);
	list_add_tail(&entry->node, &registers_list);
	mutex_unlock(&registers_mutex);

	return 0;
}

int pm_registers_add(u32 *registers, int count)
{
	return __pm_registers_add(registers, count, 0);
}

int pm_registers_add_single(u32 register_address)
{
	return __pm_registers_add(&register_address, 1, 1);
}

static void pm_registers_action(enum pm_action_type type)
{
	struct regs_entry *p;
	
	list_for_each_entry(p, &registers_list, node) {
		int i;
		if (p->single) {
			if (type == SAVE)
                                p->reg_value = readl(p->reg_address);
                        else
                                writel(p->reg_value, p->reg_address);
		} 
		else 
			for (i = 0; i < p->count; i++)
				if (type == SAVE)
					p->regs_values[i] = readl(p->regs_addresses[i]);
				else
					writel(p->regs_values[i], p->regs_addresses[i]);
	}
}

#ifdef CONFIG_PMU_PROC
#define CPU_VSET 0x9
#define CORE_VSET 0x9
#define DDR_VSET 0xb
#define SDI_CPU 10
#define SDI_CORE 9
extern MV_STATUS mvPmuDvs (MV_U32 pSet, MV_U32 vSet, MV_U32 rAddr, MV_U32 sAddr);
extern MV_STATUS mvPmuCpuFreqScale (MV_PMU_CPU_SPEED cpuSpeed);
extern MV_VOID mvPmuSramDeepIdle(MV_U32 ddrSelfRefresh);

void dove_pm_cpuidle_deepidle (void);
int pmu_proc_write(struct file *file, const char *buffer,unsigned long count,
		     void *data)
{
	int len = 0;
	char *str;
	unsigned long ints;
	unsigned int mc, mc2;
	int dummy;
	MV_U32 reg;
	MV_STATUS stat;
	str = "poweroff";
	if(!strncmp(buffer+len, str,strlen(str))) {
		unsigned int sb,nb;
		printk ("Powering off system\n");
		gpio_direction_output(1, 0); /* Disable external USB current limiter */
		sb = ioremap(0xf1000000,0x00100000);
		if (!sb) {
			printk ("Can't remap sb registers\n");
			return 0;
		}
		printk ("Regsiters is at 0x%x\n",sb);
		nb = ioremap(0xf1800000,0x00100000);
		if (!nb) {
			printk ("Can't remap nb registers\n");
			return 0;
		}
		local_irq_save(ints);
		// Start 407mA
		writel (0x009B1215, sb+0x0a2050); // eSata - 40mA
		writel (0x0003007f, sb+0x0d0058); // PEX clk - 10mA
		writel (0x00010800, sb+0x072004); // SMI power down phy --> ~20mA when previously 100mA
		msleep(100);
		writel (0x00000002, sb+0x0720b0); // gig digital unit down Dangerous - will hang chip
		writel (0x0003007b, sb+0x0d0058); // PEX clk - 10mA + gigE ios
		{ // GPU off - 30mA
			/* enable isolators */
			reg = MV_REG_READ(PMU_ISO_CTRL_REG);
			reg &= ~PMU_ISO_GPU_MASK;
			MV_REG_WRITE(PMU_ISO_CTRL_REG, reg);
			/* reset unit */
			reg = MV_REG_READ(PMU_SW_RST_CTRL_REG);
			reg &= ~PMU_SW_RST_GPU_MASK;
			MV_REG_WRITE(PMU_SW_RST_CTRL_REG, reg);
			/* power off */
			reg = MV_REG_READ(PMU_PWR_SUPLY_CTRL_REG);
			reg |= PMU_PWR_GPU_PWR_DWN_MASK;
			MV_REG_WRITE(PMU_PWR_SUPLY_CTRL_REG, reg);
		}
		reg = readl(sb + 0x0d0208);
		reg &= ~0xf00;
		reg |= 0x100; // Internall no select, keeps it almost floating
		writel(reg, sb + 0x0d0208);
		reg = readl(sb + 0x0d0400);
		reg |= (1<< 18);
		writel(reg, sb + 0x0d0400);
	
	
		// Add CPU DFS to 400
		// Add CPU voltage to -2.5
		// Add CORE voltage to -10
		// Add DDR voltage to -10
		
		// Here reached ~300mA
		writel(0xf0002000, nb+0x0200fc); // LCD PWM
		writel(0xff000160, sb+0x050400); // USB 0 phy shutdown
		writel(0xff000160, sb+0x050400); // USB 1 phy shutdown
		writel(0xff3800c4, sb+0x0d0038); // Clock gating of unused south bridge units

		// Disable all interrupts besides uart0
		mc = MV_REG_READ(CPU_MAIN_IRQ_MASK_REG);
		mc2 = MV_REG_READ(CPU_MAIN_IRQ_MASK_HIGH_REG);
		MV_REG_WRITE(CPU_MAIN_IRQ_MASK_REG, 0x80); /* disable all interrupts except UART0 */
		MV_REG_WRITE(CPU_MAIN_IRQ_MASK_HIGH_REG, 0x0);
	
		mvPmuSramDeepIdle(0);
//		__asm__ __volatile__("wfi\n");
		MV_REG_WRITE(CPU_MAIN_IRQ_MASK_REG, mc);
		MV_REG_WRITE(CPU_MAIN_IRQ_MASK_HIGH_REG, mc2);
		local_irq_restore(ints);			
	}
		
	str = "sdi ";
	if(!strncmp(buffer+len, str,strlen(str))) {
		len += strlen(str);
		str = "close";
		if(!strncmp(buffer+len, str,strlen(str))) {
			printk ("Closing interface\n");
			mvPmuSelSDI(0xff);
		}
		str = "cpu";
		if(!strncmp(buffer+len, str,strlen(str))) {
			mvPmuSelSDI(SDI_CPU);
		}
		str = "core";
		if(!strncmp(buffer+len, str,strlen(str))) {
			mvPmuSelSDI(SDI_CORE);
		}
	}
	str = "coredvs ";
	if(!strncmp(buffer+len, str,strlen(str))) {
		mvPmuSelSDI(SDI_CORE);
		len += strlen(str);
		str = "+10";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(15, CORE_VSET, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "+7.5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(14, CORE_VSET, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "+5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(13, CORE_VSET, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "+2.5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(12, CORE_VSET, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "0";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(0, 0, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "-2.5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(11, CORE_VSET, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "-5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(10, CORE_VSET, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "-7.5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(9, CORE_VSET, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "-10";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(8, CORE_VSET, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		goto done;
	}
	str = "ddrdvs ";
	if(!strncmp(buffer+len, str,strlen(str))) {
		mvPmuSelSDI(SDI_CORE);
		len += strlen(str);
		str = "+10";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(15, DDR_VSET, 0x0, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "+7.5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(14, DDR_VSET, 0x0, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "+5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(13, DDR_VSET, 0x0, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "+2.5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(12, DDR_VSET, 0x0, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "0";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(0, 0, 0x0, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "-2.5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(11, DDR_VSET, 0x0, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "-5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(10, DDR_VSET, 0x0, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "-7.5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(9, DDR_VSET, 0x0, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "-10";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(8, DDR_VSET, 0x0, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		goto done;
	}
	str = "dvs ";
	if(!strncmp(buffer+len, str,strlen(str))) {
		printk ("Selecting SDI_CPU\n");
		mvPmuSelSDI(SDI_CPU);
		len += strlen(str);
		str = "+10";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(15, CPU_VSET, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "+7.5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(14, CPU_VSET, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "+5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(13, CPU_VSET, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "+2.5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(12, CPU_VSET, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "0";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(0, 0, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "-2.5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			printk ("dvs -2.5\n");
			if (mvPmuDvs(11, CPU_VSET, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "-5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(10, CPU_VSET, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "-7.5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(9, CPU_VSET, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "-10";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(8, CPU_VSET, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		goto done;
	}

	str = "cpudfs ";
	if(!strncmp(buffer+len, str,strlen(str))) {
		len += strlen(str);
		str = "turbo";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Set CPU Frequency to TURBO");			

			local_irq_save(ints);
			mc = MV_REG_READ(CPU_MAIN_IRQ_MASK_HIGH_REG);
			MV_REG_WRITE(CPU_MAIN_IRQ_MASK_HIGH_REG, (mc | 0x2));	/* enable PMU interrupts if not already enabled */
			stat = mvPmuCpuFreqScale (CPU_CLOCK_TURBO);
			MV_REG_WRITE(CPU_MAIN_IRQ_MASK_HIGH_REG, mc);
			local_irq_restore(ints);			

			if (stat != MV_OK)
				printk(">E\n");
			else
				printk("\n");
			
			
			goto done;
		}
		str = "ddr";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Set CPU Frequency to DDR");
			
			local_irq_save(ints);
			mc = MV_REG_READ(CPU_MAIN_IRQ_MASK_HIGH_REG);
			MV_REG_WRITE(CPU_MAIN_IRQ_MASK_HIGH_REG, (mc | 0x2));
			stat = mvPmuCpuFreqScale (CPU_CLOCK_SLOW);
			MV_REG_WRITE(CPU_MAIN_IRQ_MASK_HIGH_REG, mc);
			local_irq_restore(ints);

			if (stat != MV_OK)
				printk(">E\n");
			else
				printk("\n");
						
			goto done;
		}
		goto done;
	}

	str = "sysdfs ";
	if(!strncmp(buffer+len, str,strlen(str))) {
		MV_U32 cpuFreq, l2Freq, ddrFreq;

		len += strlen(str);
		sscanf (buffer+len, "%d %d %d", &cpuFreq, &l2Freq, &ddrFreq);

		printk("Set New System Frequencies to CPU %dMhz, L2 %dMhz, DDR %dMhz", cpuFreq, l2Freq, ddrFreq);
		local_irq_save(ints);
		mc = MV_REG_READ(CPU_MAIN_IRQ_MASK_HIGH_REG);
		MV_REG_WRITE(CPU_MAIN_IRQ_MASK_HIGH_REG, (mc | 0x2));	/* PMU Interrupt Enable */
		if (mvPmuSysFreqScale (ddrFreq, l2Freq, cpuFreq) != MV_OK)
			printk(">>>>>> FAILED\n");
		else
			printk("\n");
		MV_REG_WRITE(CPU_MAIN_IRQ_MASK_HIGH_REG, mc);
		local_irq_restore(ints);
		goto done;
	}

	str = "deepidle_block";
	if(!strncmp(buffer+len, str,strlen(str))) {
		printk("Enter DeepIdle mode ");
		mc = MV_REG_READ(CPU_MAIN_IRQ_MASK_REG);
		mc2 = MV_REG_READ(CPU_MAIN_IRQ_MASK_HIGH_REG);
		MV_REG_WRITE(CPU_MAIN_IRQ_MASK_REG, 0x80); /* disable all interrupts except UART0 */
		MV_REG_WRITE(CPU_MAIN_IRQ_MASK_HIGH_REG, 0x0);
		dove_pm_cpuidle_deepidle();
		MV_REG_WRITE(CPU_MAIN_IRQ_MASK_REG, mc);
		MV_REG_WRITE(CPU_MAIN_IRQ_MASK_HIGH_REG, mc2);
		goto done;
	}        	

	str = "deepidle";
	if(!strncmp(buffer+len, str,strlen(str))) {
		local_irq_save(ints);
		dove_pm_cpuidle_deepidle();
		local_irq_restore(ints);
		goto done;
	}

	str = "standby";
	if(!strncmp(buffer+len, str,strlen(str))) {
		printk("Enter Standby mode ");

		if (mvPmuStandby() != MV_OK)
			printk(">>>>>> FAILED\n");
		else
			printk("\n");
		goto done;
	}

	str = "wfi_block";
	if(!strncmp(buffer+len, str,strlen(str))) {
		printk("Enter WFI mode ");
		mc = MV_REG_READ(CPU_MAIN_IRQ_MASK_REG);
		mc2 = MV_REG_READ(CPU_MAIN_IRQ_MASK_HIGH_REG);
		MV_REG_WRITE(CPU_MAIN_IRQ_MASK_REG, 0x80); /* disable all interrupts except UART0 */
		MV_REG_WRITE(CPU_MAIN_IRQ_MASK_HIGH_REG, 0x0);		

#ifdef CONFIG_DOVE_DEBUGGER_MODE_V6
		__asm__ __volatile__("mcr p15, 0, %0, c7, c0, 4\n" : "=r" (dummy));
#else
		__asm__ __volatile__("wfi\n");
#endif
		MV_REG_WRITE(CPU_MAIN_IRQ_MASK_REG, mc);
		MV_REG_WRITE(CPU_MAIN_IRQ_MASK_HIGH_REG, mc2);
		goto done;
	}

	str = "block";
	if(!strncmp(buffer+len, str,strlen(str))) {
		mc = MV_REG_READ(CPU_MAIN_IRQ_MASK_REG);
		mc2 = MV_REG_READ(CPU_MAIN_IRQ_MASK_HIGH_REG);
		MV_REG_WRITE(CPU_MAIN_IRQ_MASK_REG, 0x0);
		MV_REG_WRITE(CPU_MAIN_IRQ_MASK_HIGH_REG, 0x0);
		while(1);
		goto done;
	}

	str = "dvfs ";
	if(!strncmp(buffer+len, str,strlen(str))) {
		len += strlen(str);
		str = "hi";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Going to hi gear (CPU=TURBO, V=1.10) ");
			/* Upscale Voltage +10% */
			if (mvPmuDvs(15, 0x8, 0x2, 0x5) != MV_OK)
				printk("Volatge up-scaling failed\n");

			/* Upscale frequency */
			local_irq_save(ints);
			mc = MV_REG_READ(CPU_MAIN_INT_CAUSE_HIGH_REG);
			MV_REG_WRITE(CPU_MAIN_IRQ_MASK_HIGH_REG, (mc | 0x2));
			if (mvPmuCpuFreqScale (CPU_CLOCK_TURBO) != MV_OK)
				printk(">>>>>> FAILED\n");
			else
				printk("\n");
			MV_REG_WRITE(CPU_MAIN_IRQ_MASK_HIGH_REG, mc);
			local_irq_restore(ints);
			goto done;
		}
		str = "lo";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Going to low gear (CPU=DDR, V=0.975) ");
			/* Downscale frequency */
			local_irq_save(ints);
			mc = MV_REG_READ(CPU_MAIN_INT_CAUSE_HIGH_REG);
			MV_REG_WRITE(CPU_MAIN_IRQ_MASK_HIGH_REG, (mc | 0x2));
			if (mvPmuCpuFreqScale (CPU_CLOCK_SLOW) != MV_OK)
				printk(">>>>>> FAILED\n");
			else
				printk("\n");
			MV_REG_WRITE(CPU_MAIN_IRQ_MASK_HIGH_REG, mc);
			local_irq_restore(ints);

			/* Downscale Voltage -2.5% */
			if (mvPmuDvs(11, 0x8, 0x2, 0x5) != MV_OK)
				printk("Volatge down-scaling failed\n");
			goto done;
		}
		goto done;
	}

	str = "units ";
	if(!strncmp(buffer+len, str,strlen(str))) {
		len += strlen(str);
		str = "off";
		if(!strncmp(buffer+len, str,strlen(str))) {
			MV_U32 units_reg;			
			len += strlen(str);

			printk("Units Disabled: SATA, NAND, CAM, AUD0, AUD1, CESA, PDMA, XOR0, XOR1\n");
			/* Unit clock gating */
			units_reg = MV_REG_READ(0xD0038);
			units_reg &= ~0x01C0BC08;
			MV_REG_WRITE(0xD0038, units_reg);

			goto done;
		}
		str = "on";
		if (!strncmp(buffer+len, str, strlen(str))) {
			MV_U32 units_reg;
			len += strlen(str);

			printk(KERN_INFO "Units Enabled: SATA, NAND, CAM, AUD0, AUD1, CESA, PDMA, XOR0, XOR1\n");
			/* Unit clock gating */
			units_reg = MV_REG_READ(0xD0038);
			units_reg |= 0x01C0BC08;
			MV_REG_WRITE(0xD0038, units_reg);
			goto done;
		}
		goto done;
	}

	str = "vpu ";
	if(!strncmp(buffer+len, str,strlen(str))) {
		len += strlen(str);
		str = "off";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Setting VPU power OFF.\n");
			/* enable isolators */
			reg = MV_REG_READ(PMU_ISO_CTRL_REG);
			reg &= ~PMU_ISO_VIDEO_MASK;
			MV_REG_WRITE(PMU_ISO_CTRL_REG, reg);
			/* reset unit */
			reg = MV_REG_READ(PMU_SW_RST_CTRL_REG);
			reg &= ~PMU_SW_RST_VIDEO_MASK;
			MV_REG_WRITE(PMU_SW_RST_CTRL_REG, reg);
			/* power off */
			reg = MV_REG_READ(PMU_PWR_SUPLY_CTRL_REG);
			reg |= PMU_PWR_VPU_PWR_DWN_MASK;
			MV_REG_WRITE(PMU_PWR_SUPLY_CTRL_REG, reg);
			goto done;
		}
		str = "on";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Setting VPU power ON.\n");
			/* power on */
			reg = MV_REG_READ(PMU_PWR_SUPLY_CTRL_REG);
			reg &= ~PMU_PWR_VPU_PWR_DWN_MASK;
			MV_REG_WRITE(PMU_PWR_SUPLY_CTRL_REG, reg);
			/* un-reset unit */
			reg = MV_REG_READ(PMU_SW_RST_CTRL_REG);
			reg |= PMU_SW_RST_VIDEO_MASK;
			MV_REG_WRITE(PMU_SW_RST_CTRL_REG, reg);
			/* disable isolators */
			reg = MV_REG_READ(PMU_ISO_CTRL_REG);
			reg |= PMU_ISO_VIDEO_MASK;
			MV_REG_WRITE(PMU_ISO_CTRL_REG, reg);
			goto done;
		}
		goto done;
	}

	str = "gpu ";
	if(!strncmp(buffer+len, str,strlen(str))) {
		len += strlen(str);
		str = "off";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Setting GPU power OFF.\n");
			/* enable isolators */
			reg = MV_REG_READ(PMU_ISO_CTRL_REG);
			reg &= ~PMU_ISO_GPU_MASK;
			MV_REG_WRITE(PMU_ISO_CTRL_REG, reg);
			/* reset unit */
			reg = MV_REG_READ(PMU_SW_RST_CTRL_REG);
			reg &= ~PMU_SW_RST_GPU_MASK;
			MV_REG_WRITE(PMU_SW_RST_CTRL_REG, reg);
			/* power off */
			reg = MV_REG_READ(PMU_PWR_SUPLY_CTRL_REG);
			reg |= PMU_PWR_GPU_PWR_DWN_MASK;
			MV_REG_WRITE(PMU_PWR_SUPLY_CTRL_REG, reg);
			goto done;
		}
		str = "on";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Setting GPU power ON.\n");
			/* power on */
			reg = MV_REG_READ(PMU_PWR_SUPLY_CTRL_REG);
			reg &= ~PMU_PWR_GPU_PWR_DWN_MASK;
			MV_REG_WRITE(PMU_PWR_SUPLY_CTRL_REG, reg);
			/* un-reset unit */
			reg = MV_REG_READ(PMU_SW_RST_CTRL_REG);
			reg |= PMU_SW_RST_GPU_MASK;
			MV_REG_WRITE(PMU_SW_RST_CTRL_REG, reg);
			/* disable isolators */
			reg = MV_REG_READ(PMU_ISO_CTRL_REG);
			reg |= PMU_ISO_GPU_MASK;
			MV_REG_WRITE(PMU_ISO_CTRL_REG, reg);
			goto done;
		}
		goto done;
	}

	str = "lcd ";
	if(!strncmp(buffer+len, str,strlen(str))) {
		len += strlen(str);
		str = "off";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("HW doesn't support LCD power OFF.\n");
			goto done;
		}
		str = "on";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("HW doesn't support LCD power ON.\n");
			goto done;
		}
		goto done;
	}

	str = "wlan ";
	if(!strncmp(buffer+len, str,strlen(str))) {
		len += strlen(str);
		str = "off";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Setting WLAN SC in low power.\n");
			/* Set level */
			reg = MV_REG_READ(GPP_DATA_OUT_REG(0));
			reg &= ~0x10;
			MV_REG_WRITE(GPP_DATA_OUT_REG(0), reg);
			/* verify pin is output */
			reg = MV_REG_READ(GPP_DATA_OUT_EN_REG(0));
			reg &= ~0x10;
			MV_REG_WRITE(GPP_DATA_OUT_EN_REG(0), reg);
			goto done;
		}
		str = "on";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Setting WLAN SC out of low power.\n");
			/* Set level */
			reg = MV_REG_READ(GPP_DATA_OUT_REG(0));
			reg |= 0x10;
			MV_REG_WRITE(GPP_DATA_OUT_REG(0), reg);
			/* verify pin is output */
			reg = MV_REG_READ(GPP_DATA_OUT_EN_REG(0));
			reg &= ~0x10;
			MV_REG_WRITE(GPP_DATA_OUT_EN_REG(0), reg);
			goto done;
		}
		goto done;
	}

	str = "freqs";
	if(!strncmp(buffer+len, str,strlen(str)))
	{
		MV_PMU_FREQ_INFO freqs;

		printk("Frequencies: ");
		if(mvPmuGetCurrentFreq(&freqs) != MV_OK)
			printk(">>>>>> FAILED!\n");
		else
			printk(KERN_INFO "PLL %dMhz CPU %dMhz, AXI %dMHz,"
			       " L2 %dMhz, DDR %dMhz\n", freqs.pllFreq, 
			       freqs.cpuFreq, freqs.axiFreq, freqs.l2Freq,
			       freqs.ddrFreq);
	}

done:
	return count;
}

int pmu_proc_read(char* page, char** start, off_t off, int count,int* eof,
		    void* data)
{
	int len = 0;

	len += sprintf(page+len,"PM Proc debug shell:\n");
	len += sprintf(page+len,"   dvs <+10|+7.5|+5|+2.5|0|-2.5|-5|-7.5|-10>\n");
	len += sprintf(page+len,"   coredvs <+10|+7.5|+5|+2.5|0|-2.5|-5|-7.5|-10>\n");
	len += sprintf(page+len,"   ddrdvs <+10|+7.5|+5|+2.5|0|-2.5|-5|-7.5|-10>\n");
	len += sprintf(page+len,"   cpudfs <turbo|ddr>\n");
	len += sprintf(page+len,"   sysdfs <cpu> <l2> <ddr>\n");
	len += sprintf(page+len,"   freqs\n");
	len += sprintf(page+len,"   deepidle\n");
	len += sprintf(page+len,"   standby\n");
	len += sprintf(page+len,"   deepidle_block\n");
	len += sprintf(page+len,"   wfi_block\n");
	len += sprintf(page+len,"   dvfs <hi|lo>\n");
	len += sprintf(page+len,"   gpu <on|off>\n");
	len += sprintf(page+len,"   vpu <on|off>\n");
	len += sprintf(page+len,"   wlan <on|off>\n");
	len += sprintf(page+len,"   units <on|off>\n");
	len += sprintf(page+len,"   deepcnt\n");
	return len;
}

static struct proc_dir_entry *pmu_proc_entry;
extern struct proc_dir_entry proc_root;
static int pm_proc_init(void)
{
	pmu_proc_entry = create_proc_entry("pm", 0644, &proc_root);
	pmu_proc_entry->read_proc = pmu_proc_read;
	pmu_proc_entry->write_proc = pmu_proc_write;
	return 0;
}
#endif /* CONFIG_PMU_PROC */

#if defined(CONFIG_FB)
static MV_BOOL enable_ebook = MV_TRUE;
#else
/* Use Deep-Idle by default if no FB in the system */
static MV_BOOL enable_ebook = MV_FALSE;
#endif

static ssize_t ebook_show(struct kobject *kobj, struct kobj_attribute *attr,
			 char *buf)
{
	return sprintf(buf, "%lu\n", (unsigned long)enable_ebook);
}

static ssize_t ebook_store(struct kobject *kobj, struct kobj_attribute *attr,
			  const char * buf, size_t n)
{
	unsigned short value;
	if (sscanf(buf, "%hu", &value) != 1 ||
	    (value != 0 && value != 1)) {
		printk(KERN_ERR "idle_sleep_store: Invalid value\n");
		return -EINVAL;
	}
	if (value)
		enable_ebook = MV_TRUE;
	else
		enable_ebook = MV_FALSE;
		
	return n;
}

static struct kobj_attribute ebook_attr =
	__ATTR(ebook, 0644, ebook_show, ebook_store);

#if defined(CONFIG_FB)

/* The framebuffer notifier block */
static struct notifier_block fb_notif;

/* This callback gets called when something important happens inside a
 * framebuffer driver. If the LCD is blancking then go into Deep-Idle
 * instead of eBook.
 */
static int pm_fb_notifier_callback(struct notifier_block *self,
				unsigned long event, void *data)
{
	struct fb_event *evdata = data;

	/* If we aren't interested in this event, skip it immediately ... */
	if (event != FB_EVENT_BLANK && event != FB_EVENT_CONBLANK)
		return 0;

	if (*(int *)evdata->data == FB_BLANK_UNBLANK) {
		pr_debug("FB notifier: Using eBook mode...\n");
		enable_ebook = MV_TRUE;		/* Work in eBook mode */
	}
	else {
		pr_debug("FB notifier: Using Deep-Idle mode...\n");
		enable_ebook = MV_FALSE;	/* Work in Deep-Idle mode */
	}

	return 0;
}

static int pm_register_fb(void)
{
	memset(&fb_notif, 0, sizeof(fb_notif));
	fb_notif.notifier_call = pm_fb_notifier_callback;
	return fb_register_client(&fb_notif);
}
#endif /* CONFIG_FB */


/*
 * Enter the Dove DEEP IDLE mode (power off CPU only)
 */
void dove_deepidle(void)
{
	MV_U32 reg;

	pr_debug("dove_deepidle: Entering Dove DEEP IDLE mode.\n");
	
	/* Put on the Led on MPP7 */
	reg = MV_REG_READ(PMU_SIG_SLCT_CTRL_0_REG);
	reg &= ~PMU_SIG_7_SLCT_CTRL_MASK;
	reg |= (PMU_SIGNAL_0 << PMU_SIG_7_SLCT_CTRL_OFFS);
	MV_REG_WRITE(PMU_SIG_SLCT_CTRL_0_REG, reg);

#ifdef CONFIG_IWMMXT
	/* force any iWMMXt context to ram **/
	if (elf_hwcap & HWCAP_IWMMXT)
		iwmmxt_task_disable(NULL);
#endif

#if defined(CONFIG_VFP)
	vfp_save();
#endif

	/* Suspend the CPU only */
	mvPmuDeepIdle(enable_ebook);
	cpu_init();

#if defined(CONFIG_VFP)
	vfp_restore();
#endif

	/* Put off the Led on MPP7 */
	reg = MV_REG_READ(PMU_SIG_SLCT_CTRL_0_REG);
	reg &= ~PMU_SIG_7_SLCT_CTRL_MASK;
	reg |= (PMU_SIGNAL_1 << PMU_SIG_7_SLCT_CTRL_OFFS);
	MV_REG_WRITE(PMU_SIG_SLCT_CTRL_0_REG, reg);

	pr_debug("dove_deepidle: Exiting Dove DEEP IDLE mode.\n");
}

/*
 * Enter the Dove STANDBY mode (Power off all SoC)
 */
void dove_standby(void)
{
	MV_U32 reg;

	printk(KERN_NOTICE "Dove: Entering Standby Mode...\n");

	/* Put on the Led on MPP7 */
	reg = MV_REG_READ(PMU_SIG_SLCT_CTRL_0_REG);
	reg &= ~PMU_SIG_7_SLCT_CTRL_MASK;
	reg |= (PMU_SIGNAL_BLINK/*PMU_SIGNAL_0*/ << PMU_SIG_7_SLCT_CTRL_OFFS);
	MV_REG_WRITE(PMU_SIG_SLCT_CTRL_0_REG, reg);

#ifdef CONFIG_IWMMXT
	/* force any iWMMXt context to ram **/
	if (elf_hwcap & HWCAP_IWMMXT)
		iwmmxt_task_disable(NULL);
#endif

	/* Save generic list of registes */
	pm_registers_action(SAVE);

	/* Save CPU Peripherals state */
	dove_save_cpu_wins();
	dove_save_cpu_conf_regs();
	dove_save_upstream_regs();	
	dove_save_timer_regs();	
	dove_save_int_regs();

	/* Suspend the CPU only */	
	mvPmuStandby();	
	cpu_init();

	/* Restore CPU Peripherals state */
	dove_restore_int_regs();
	dove_restore_timer_regs();	
	dove_restore_upstream_regs();	
	dove_restore_cpu_conf_regs();
	dove_restore_cpu_wins();

	/* Save generic list of registes */
	pm_registers_action(RESTORE);

	dove_restore_pcie_regs(); /* Should be done after restoring cpu configuration registers */
	/* reset the current i2c expander port id to sync with hw settings */
#ifdef CONFIG_I2C_MV64XXX_PORT_EXPANDER
	dove_reset_exp_port();
#endif
	/* Put off the Led on MPP7 */
	reg = MV_REG_READ(PMU_SIG_SLCT_CTRL_0_REG);
	reg &= ~PMU_SIG_7_SLCT_CTRL_MASK;
	reg |= (PMU_SIGNAL_1 << PMU_SIG_7_SLCT_CTRL_OFFS);
	MV_REG_WRITE(PMU_SIG_SLCT_CTRL_0_REG, reg);

	printk(KERN_NOTICE "Dove: Exiting Standby Mode...\n");
}

/*
 * Logical check for Dove valid PM states
 */
static int dove_pm_valid(suspend_state_t state)
{
	return ((state == PM_SUSPEND_MEM) ||
		(state == PM_SUSPEND_STANDBY));
}

/*
 * Initialise a transition to given system sleep state.
 * This is needed by all devices to decide whether to save device context
 * (for "ram") or not (for "standby")
 */
static int dove_pm_begin(suspend_state_t state)
{
	/* TODO: write target mode to the PMU flag register */
	dove_target_pm_state = state;
	return 0;
}

/*
 * Check if the CORE devices will loose (or have lost) power during suspend
 */
int dove_io_core_lost_power(void)
{
	if (dove_target_pm_state >= PM_SUSPEND_MEM)
		return 1;

	return 0;
}
EXPORT_SYMBOL(dove_io_core_lost_power);

/*
 * Preparation to enter a new PM state. This includes disabling the call
 * for the WFI instruction from the context of the do_idle() routine
 */
static int dove_pm_prepare(void)
{
	pr_debug("PM DEBUG: Preparing to enter PM state.\n");
	return 0;
}

/*
 * Enter the requested PM state
 */
static int dove_pm_enter(suspend_state_t state)
{
	pr_debug("PM DEBUG: Entering PM state (%d).\n", state);
	switch (state)	{
	case PM_SUSPEND_STANDBY:
		dove_deepidle();
		break;
	case PM_SUSPEND_MEM:
		dove_standby();
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*
 * This is called when the system has just left a sleep state, right after
 * the nonboot CPUs have been enabled.
 */
static void dove_pm_finish(void)
{
	local_irq_enable();
}

/*
 * This is called by the PM core right after resuming devices, to indicate to
 * the platform that the system has returned to the working state or
 * the transition to the sleep state has been aborted.
 */
static void dove_pm_end(void)
{
	dove_target_pm_state = PM_SUSPEND_ON;
}

static struct platform_suspend_ops dove_pm_ops = {
	.valid		= dove_pm_valid,
	.begin     	= dove_pm_begin,
	.prepare	= dove_pm_prepare,
	.enter		= dove_pm_enter,
	.finish		= dove_pm_finish,
	.end		= dove_pm_end,
};

int dove_timekeeping_resume(void);
int dove_timekeeping_suspend(void);

void dove_pm_cpuidle_deepidle (void)
{
	dove_pm_enter(PM_SUSPEND_STANDBY);
}

void dove_pm_register (void)
{
	MV_PMU_FREQ_INFO freqs;

	printk(KERN_NOTICE "Power Management for Marvell Dove.\n");

	suspend_set_ops(&dove_pm_ops);

	/* Create EBook control file in sysfs */
	if (sysfs_create_file(power_kobj, &ebook_attr.attr))
		printk(KERN_ERR "dove_pm_register: ebook sysfs_create_file failed!");

#ifdef CONFIG_PMU_PROC
	pm_proc_init();
#endif

#if defined(CONFIG_FB)
	if (pm_register_fb())
		printk(KERN_ERR "dove_pm_register: FB notifier register failed!");
#endif

	if (mvPmuGetCurrentFreq(&freqs) == MV_OK)
		printk(KERN_NOTICE "PMU Detected Frequencies CPU %dMhz, AXI %dMhz, L2 %dMhz, DDR %dMhz\n", 
				freqs.cpuFreq, freqs.axiFreq, freqs.l2Freq, freqs.ddrFreq);
	else
		printk(KERN_ERR "PMU Failed to detect current system frequencies!\n");
}
