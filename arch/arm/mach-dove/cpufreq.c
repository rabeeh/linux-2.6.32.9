/*
 * arch/arm/mach-dove/cpufreq.c
 *
 * Clock scaling for Dove SoC
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <asm/io.h>
#include <linux/mbus.h>
#include <asm/mach/pci.h>
#include <plat/pcie.h>
#include "pmu/mvPmu.h"
#include "pmu/mvPmuRegs.h"
#include "ctrlEnv/mvCtrlEnvRegs.h"

#define dprintk(msg...) cpufreq_debug_printk(CPUFREQ_DEBUG_DRIVER, "dove_cpufreq", msg)

/*
 * Following table maps 4 bit reset sample to CPU clock.
 */
u32 dove_sar2cpu_clk[] = { 0, 0, 0, 0, 0, /* 0->4 are reserved */
			   1000000, 933000, 933000,
			   800000, 800000, 800000,
			   1067000, 667000, 533000,
			   400000, 333000};

/*
 * Following table maps 3 bit reset sample to CPU:L2 ratio.
 */
					/* CPU:L2 */
u32 dove_sar_cpu2l2_ratio[] = { 10, 	/* 1:1 */
				0, 	/* Reserved */
				20, 	/* 1:2 */
				0, 	/* Reserved */
				30, 	/* 1:3 */
				0, 	/* Reserved */
				40, 	/* 1:4 */
				0};	/* Reserved */

/*
 * Following table maps 4 bit reset sample to CPU:DDR ratio.
 */
					/* CPU:DDR */
u32 dove_sar_cpu2ddr_ratio[] = {10, 	/* 1:1 */
				0, 	/* Reserved */
				20, 	/* 1:2 */
				0, 	/* Reserved */
				30, 	/* 1:3 */
				0, 	/* Reserved */
				40, 	/* 1:4 */
				0,	/* Reserved */
				50, 	/* 1:5 */
				0, 	/* Reserved */
				60, 	/* 1:6 */
				0, 	/* Reserved */
				70, 	/* 1:7 */
				0, 	/* Reserved */
				80, 	/* 1:8 */
				100};	/* 1:10 */

enum dove_cpufreq_range {
	DOVE_CPUFREQ_DDR 	= 0,
	DOVE_CPUFREQ_HIGH 	= 1,
	DOVE_CPUFREQ_STEPS_NR
};

static struct cpufreq_frequency_table dove_freqs[] = {
        { 0, 0                  },	/* DDR */
        { 0, 0                  },	/* HIGH */
        { 0, CPUFREQ_TABLE_END  }
}; 


/*
 * Power management function: set or unset powersave mode
 */
int global_dvs_enable = 0;
static inline int dove_set_frequency(enum dove_cpufreq_range freq)
{
	unsigned long flags;
	int ret = 0;

        local_irq_save(flags);

        if (freq == DOVE_CPUFREQ_DDR) {
		dprintk("Going to DDR Frequency\n");
		if (mvPmuCpuSetOP (CPU_CLOCK_SLOW, (global_dvs_enable ? MV_TRUE : MV_FALSE)) != MV_OK)
			ret = -EIO;			
	}
        else if (freq == DOVE_CPUFREQ_HIGH) {
		dprintk("Going to HIGH Frequency\n");
		if (mvPmuCpuSetOP (CPU_CLOCK_TURBO, (global_dvs_enable ? MV_TRUE : MV_FALSE)) != MV_OK)
			ret = -EIO;
	}
	else {
		ret = -EINVAL;
	}

        local_irq_restore(flags);
	return ret;
}

static int dove_cpufreq_verify(struct cpufreq_policy *policy)
{
        if (unlikely(!cpu_online(policy->cpu)))
                return -ENODEV;

        return cpufreq_frequency_table_verify(policy, dove_freqs);
}

/* 
 * Get the current frequency for a given cpu.
 */
static unsigned int dove_cpufreq_get(unsigned int cpu)
{
        unsigned int freq;
        u32 reg;

        if (unlikely(!cpu_online(cpu)))
                return -ENODEV;

	/* Read the PMU DFS status register - CpuSlowModeStts */
        reg = readl(INTER_REGS_BASE | PMU_DFS_STATUS_REG);

        if (reg & PMU_DFS_STAT_SLOW_MASK)
                freq = dove_freqs[DOVE_CPUFREQ_DDR].frequency;
        else
                freq = dove_freqs[DOVE_CPUFREQ_HIGH].frequency;

	dprintk("Current frequency: %dMHz\n", (freq/1000));

        return freq;
}

/* 
 * Set the frequency for a given cpu.
 */
static int dove_cpufreq_target(struct cpufreq_policy *policy,
                unsigned int target_freq, unsigned int relation)
{
        unsigned int index;
	struct cpufreq_freqs freqs;
	int ret;

        if (unlikely(!cpu_online(policy->cpu)))
                return -ENODEV;

        /* Lookup the next frequency */
        if (unlikely(cpufreq_frequency_table_target(policy, 
                        dove_freqs, target_freq, relation, &index))) 
                return -EINVAL;

	freqs.old = policy->cur;
	freqs.new = dove_freqs[index].frequency;
	freqs.cpu = policy->cpu;

	/* Check if no change is needed */
	if (freqs.new == freqs.old)
		return 0;

        cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);

	ret = dove_set_frequency(index);

        cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);

	dprintk("Set frequency: %dMHz\n", (dove_freqs[index].frequency / 1000));

        return ret;
}

static int dove_cpufreq_cpu_init(struct cpufreq_policy *policy)
{
        u32 reg;
	u32 cpuClk, ddrRatio;
	int ret;

        if (unlikely(!cpu_online(policy->cpu)))
                return -ENODEV;

	/* Read SAR register */
        reg = readl(INTER_REGS_BASE | MPP_SAMPLE_AT_RESET_REG0);

	/* Calculate CPU clock */
	cpuClk = ((reg & MSAR_CPUCLCK_MASK) >> MSAR_CPUCLCK_OFFS);
	cpuClk = dove_sar2cpu_clk[cpuClk];

	/* Calculate DDR clock */
	ddrRatio = ((reg & MSAR_DDRCLCK_RTIO_MASK) >> MSAR_DDRCLCK_RTIO_OFFS);
	ddrRatio = dove_sar_cpu2ddr_ratio[ddrRatio]; /* x10 */

	if ((cpuClk == 0) || (ddrRatio == 0)) {
		printk(KERN_WARNING "Dove cpuFreq: Unknown Reset Configuration\n");
		ret = -EINVAL;
		goto error;
	}

        /* Set both HIGH and DDR frequencies */
        dove_freqs[DOVE_CPUFREQ_HIGH].frequency = cpuClk;
	dove_freqs[DOVE_CPUFREQ_DDR].frequency = ((cpuClk * 10) / ddrRatio);
        
	dprintk("Hight frequency: %dHz - Low frequency: %dHz\n",
                        dove_freqs[DOVE_CPUFREQ_HIGH].frequency,
                        dove_freqs[DOVE_CPUFREQ_DDR].frequency);

	/* 
	 * The voltage regulator needs ~60us to step up voltage depending on the
	 * current consumption. So as to be on the safe side, 100us is used.
	 */
	policy->cpuinfo.transition_latency = 100000; /* in nano seconds */
	policy->cur = cpuClk;

	cpufreq_frequency_table_get_attr(dove_freqs, policy->cpu);

	ret = cpufreq_frequency_table_cpuinfo(policy, dove_freqs);

	printk(KERN_INFO "Dove cpuFreq Driver Initialized (%d/%d)\n", 
		(dove_freqs[DOVE_CPUFREQ_HIGH].frequency / 1000000), 
		(dove_freqs[DOVE_CPUFREQ_DDR].frequency / 1000000));

error:	
	return ret;
}

static int dove_cpufreq_cpu_exit(struct cpufreq_policy *policy)
{
        cpufreq_frequency_table_put_attr(policy->cpu);

        return 0;
}

static struct freq_attr* dove_freq_attr[] = {
        &cpufreq_freq_attr_scaling_available_freqs,
        NULL,
};

static struct cpufreq_driver dove_freq_driver = {
        .owner          = THIS_MODULE,
        .name           = "dove_cpufreq",
        .init           = dove_cpufreq_cpu_init,
        .exit		= dove_cpufreq_cpu_exit,
        .verify         = dove_cpufreq_verify,
        .target         = dove_cpufreq_target,
        .get            = dove_cpufreq_get,
        .attr           = dove_freq_attr,
};

extern int cpufreq_disable;

static int __init dove_cpufreq_init(void)
{
	if (cpufreq_disable)
		return 0;

        return cpufreq_register_driver(&dove_freq_driver);
}

static void __exit dove_cpufreq_exit(void)
{
	if (cpufreq_disable)
		return;

        cpufreq_unregister_driver(&dove_freq_driver);
}

MODULE_AUTHOR("Marvell Semiconductors ltd.");
MODULE_DESCRIPTION("CPU frequency scaling for Dove SoC");
MODULE_LICENSE("GPL");
module_init(dove_cpufreq_init);
module_exit(dove_cpufreq_exit);

