/**
 * @file op_model_mrvl_pj4.c
 * Marvell PJ4 Performance Monitor Driver
 *
 * Based on op_model_xscale.c
 *
 * @remark Copyright 2000-2004 Deepak Saxena <dsaxena@mvista.com>
 * @remark Copyright 2000-2004 MontaVista Software Inc
 * @remark Copyright 2004 Dave Jiang <dave.jiang@intel.com>
 * @remark Copyright 2004 Intel Corporation
 * @remark Copyright 2004 Zwane Mwaikambo <zwane@arm.linux.org.uk>
 * @remark Copyright 2004 OProfile Authors
 *
 * @remark Read the file COPYING
 *
 * @author 
 */

/* #define DEBUG */
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/oprofile.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#include <asm/system.h>

#include "op_counter.h"
#include "op_arm_model.h"

#define	PMU_ENABLE	0x001	/* Enable counters */
#define PMN_RESET	0x002	/* Reset event counters */
#define	CCNT_RESET	0x004	/* Reset clock counter */
#define	PMU_RESET	(CCNT_RESET | PMN_RESET)
#define PMU_CNT64	0x008	/* Make CCNT count every 64th cycle */

/* TODO do runtime detection */
#if (CONFIG_ARCH_DOVE && CONFIG_CPU_PJ4)
#define PJ4_PERFORM_MNTR_IRQ  IRQ_DOVE_PERFORM_MNTR
#endif

/*
 * Different types of events that can be counted by the Marvell PJ4 Performermance Monitor
 * as used by Oprofile userspace. Here primarily for documentation purposes.
 */
#define SOFTWARE_INCR			0x00	/* software increment */
#define IFU_IFETCH_REFILL		0x01	/* instruction fetch that cause a refill at the lowest level of instruction or unified cache */
#define IF_TLB_REFILL			0x02	/* instruction fetch that cause a TLB refill at the lowest level of TLB */
#define DATA_RW_CACHE_REFILL		0x03	/* data read or write operation that causes a refill of at the lowest level of data or unified cache */
#define DATA_RW_CACHE_ACCESS		0x04	/* data read or write operation that causes a cache access at the lowest level of data or unified cache */
#define DATA_RW_TLB_REFILL		0x05	/* data read or write operation that causes a TLB refill at the lowest level of TLB */
#define DATA_READ_INST_EXEC		0x06	/* data read architecturally executed */
#define DATA_WRIT_INST_EXEC		0x07	/* data write architecturally executed */
#define INSN_EXECUTED			0x08	/* instruction architecturally executed */
#define EXCEPTION_TAKEN			0x09	/* exception taken */
#define EXCEPTION_RETURN		0x0a	/* exception return architecturally executed */
#define INSN_WR_CONTEXTIDR		0x0b	/* instruction that writes to the Context ID Register architecturally executed */
#define SW_CHANGE_PC			0x0c	/* software change of PC, except by an exception, architecturally executed */
#define BR_EXECUTED			0x0d	/* immediate branch architecturally executed, taken or not taken */
#define PROCEDURE_RETURN		0x0e	/* procedure return architecturally executed */
#define UNALIGNED_ACCESS		0x0f	/* unaligned access architecturally executed */
#define BR_INST_MISS_PRED		0x10	/* branch mispredicted or not predicted */
#define CYCLE_COUNT			0x11	/* cycle count */
#define BR_PRED_TAKEN			0x12	/* branches or other change in the program flow that could have been predicted by the branch prediction resources of the processor */
#define DCACHE_READ_HIT 		0x40	/* counts the number of Data Cache read hits */
#define DCACHE_READ_MISS		0x41	/* connts the number of Data Cache read misses */
#define DCACHE_WRITE_HIT 		0x42	/* counts the number of Data Cache write hits */
#define DCACHE_WRITE_MISS		0x43	/* counts the number of Data Cache write misses */
#define MMU_BUS_REQUEST			0x44	/* counts the number of cycles of request to the MMU Bus */
#define ICACHE_BUS_REQUEST 		0x45	/* counts the number of cycles the Instruction Cache requests the bus until the data return */
#define WB_WRITE_LATENCY 		0x46	/* counts the number of cycles the Write Buffer requests the bus */
#define HOLD_LDM_STM 			0x47	/* counts the number of cycles the pipeline is held because of a load/store multiple instruction */
#define NO_DUAL_CFLAG 			0x48	/* counts the number of cycles the processor cannot dual issue because of a Carry flag dependency */
#define NO_DUAL_REGISTER_PLUS 		0x49	/* counts the number of cycles the processor cannot dual issue because the register file does not have enough read ports and at least one other reason */
#define LDST_ROB0_ON_HOLD 		0x4a	/* counts the number of cycles a load or store instruction waits to retire from ROB0 */
#define LDST_ROB1_ON_HOLD 		0x4b	/* counts the number of cycles a load or store instruction waits to retire from ROB0=1 */
#define DATA_WRITE_ACCESS_COUNT 	0x4c 	/* counts the number of any Data write access */
#define DATA_READ_ACCESS_COUNT 		0x4d 	/* counts the number of any Data read access */
#define A2_STALL 			0x4e 	/* counts the number of cycles ALU A2 is stalled */
#define L2C_WRITE_HIT 			0x4f 	/* counts the number of write accesses to addresses already in the L2C */
#define L2C_WRITE_MISS 			0x50	/* counts the number of write accesses to addresses not in the L2C */
#define L2C_READ_COUNT 			0x51	/* counts the number of L2C cache-to-bus external read request */
#define ICACHE_READ_MISS 		0x60 	/* counts the number of Instruction Cache read misses */
#define ITLB_MISS 			0x61 	/* counts the number of instruction TLB miss */
#define SINGLE_ISSUE 			0x62 	/* counts the number of cycles the processor single issues */
#define BR_RETIRED 			0x63 	/* counts the number of times one branch retires */
#define ROB_FULL 			0x64 	/* counts the number of cycles the Re-order Buffer (ROB) is full */
#define MMU_READ_BEAT 			0x65 	/* counts the number of times the bus returns RDY to the MMU */
#define WB_WRITE_BEAT 			0x66 	/* counts the number times the bus returns ready to the Write Buffer */
#define DUAL_ISSUE 			0x67 	/* counts the number of cycles the processor dual issues */
#define NO_DUAL_RAW 			0x68 	/* counts the number of cycles the processor cannot dual issue because of a Read after Write hazard */
#define HOLD_IS 			0x69 	/* counts the number of cycles the issue is held */
#define L2C_LATENCY 			0x6a 	/* counts the latency for the most recent L2C read from the external bus Counts cycles */
#define DCACHE_ACCESS 			0x70 	/* counts the number of times the Data cache is accessed */
#define DTLB_MISS 			0x71 	/* counts the number of data TLB misses */
#define BR_PRED_MISS 			0x72 	/* counts the number of mispredicted branches */
#define A1_STALL 			0x74 	/* counts the number of cycles ALU A1 is stalled */
#define DCACHE_READ_LATENCY 		0x75 	/* counts the number of cycles the Data cache requests the bus for a read */
#define DCACHE_WRITE_LATENCY 		0x76 	/* counts the number of cycles the Data cache requests the bus for a write */
#define NO_DUAL_REGISTER_FILE 		0x77 	/* counts the number of cycles the processor cannot dual issue because the register file doesn't have enough read ports */
#define BIU_SIMULTANEOUS_ACCESS 	0x78 	/* BIU Simultaneous Access */
#define L2C_READ_HIT 			0x79 	/* counts the number of L2C cache-to-bus external read requests */
#define L2C_READ_MISS 			0x7a 	/* counts the number of L2C read accesses that resulted in an external read request */
#define L2C_WB_FULL 			0x7b 	/* Not Applicable (L2C no longer has a Write Buffer(WB)) */
#define TLB_MISS 			0x80 	/* counts the number of instruction and data TLB misses */
#define BR_TAKEN 			0x81 	/* counts the number of taken branches */
#define WB_FULL 			0x82 	/* counts the number of cycles WB is full */
#define DCACHE_READ_BEAT 		0x83 	/* counts the number of times the bus returns Data to the Data cache during read request */
#define DCACHE_WRITE_BEAT 		0x84 	/* counts the number of times the bus returns ready to the Data cache during write request */
#define NO_DUAL_HW 			0x85 	/* counts the number of cycles the processor cannot dual issue because of hardware conflict */
#define NO_DUAL_MULTIPLE 		0x86 	/* counts the number of cycles the processor cannot dual issue because of multiple reasons */
#define BIU_ANY_ACCESS 			0x87 	/* counts the number of cycles the BIU is accessed by any unit */
#define MAIN_TLB_REFILL_BY_ICACHE 	0x88 	/* counts the number of instruction fetch operations that causes a Main TLB walk */
#define MAIN_TLB_REFILL_BY_DCACHE 	0x89 	/* counts the number of Data read or write operations that causes a Main TLB walk */
#define ICACHE_READ_BEAT 		0x8a 	/* counts the number of times the bus returns RDY to the instruction cache */
#define WMMX2_STORE_FIFO_FULL 		0xc0 	/* counts the number of cycles when the WMMX2 store FIFO is full */
#define WMMX2_FINISH_FIFO_FULL 		0xc1 	/* counts the number of cycles when the WMMX2 finish FIFO is full */
#define WMMX2_INST_FIFO_FULL 		0xc2 	/* counts the number of cycles when the WMMX2 instruction FIFO is full */
#define WMMX2_INST_RETIRED 		0xc3 	/* counts the number of retired WMMX2 instructions */
#define WMMX2_BUSY 			0xc4 	/* counts the number of cycles when the WMMX2 is busy */
#define WMMX2_HOLD_MI 			0xc5 	/* counts the number of cycles when WMMX2 holds the issue stage */
#define WMMX2_HOLD_MW 			0xc6 	/* counts the number of cycles when WMMX2 holds the write back stage */
/* EVT_CCNT is not hardware defined */
#define EVT_CCNT			0xFE		/* CPU_CYCLE */
#define EVT_UNUSED			0xFF

struct pmu_counter {
	volatile unsigned long ovf;
	unsigned long reset_counter;
};

enum { CCNT, PMN0, PMN1, PMN2, PMN3, MAX_COUNTERS };

static struct pmu_counter results[MAX_COUNTERS];

enum { PMU_PJ4 };

struct pmu_type {
	int id;
	char *name;
	int num_counters;
	unsigned int int_enable;
	unsigned int cnt_ovf[MAX_COUNTERS];
	unsigned int cnt_sel[MAX_COUNTERS];
	unsigned int int_mask[MAX_COUNTERS];
};

static struct pmu_type pmu_parms[] = {
	{
		.id		= PMU_PJ4,
		.name		= "arm/mrvl_pj4",
		.num_counters	= 5,
		.int_mask	= { [CCNT] = 0x80000000, [PMN0] = 0x01,
				    [PMN1] = 0x02, [PMN2] = 0x04,
				    [PMN3] = 0x08 },
		.cnt_ovf	= { [CCNT] = 0x80000000, [PMN0] = 0x01,
				    [PMN1] = 0x02, [PMN2] = 0x04,
				    [PMN3] = 0x08 },
		.cnt_sel	= { [CCNT] = 0xFFFFFFFF, [PMN0] = 0x0, 
				    [PMN1] = 0x01, [PMN2] = 0x02, 
				    [PMN3] = 0x03},
	},
};

static struct pmu_type *pmu;

static void write_pmnc(u32 val)
{
	asm volatile("mcr p15, 0, %0, c9, c12, 0": : "r"(val));
} 

static u32 read_pmnc(void)
{
	u32 val = 0;

	asm volatile("mrc p15, 0, %0, c9, c12, 0" : "=r"(val));

	return val;
}

static u32 __pj4_read_counter(int counter)
{
	u32 val = 0;
	int reg_shift = 0;

	switch (counter) {
	case CCNT:
		asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r" (val));
		break;
	case PMN0:
	case PMN1:
	case PMN2:
	case PMN3:
		reg_shift = 1 << pmu->cnt_sel[counter];
		asm volatile("mcr p15, 0, %0, c9, c12, 2": : "r"(reg_shift));
		asm volatile("mcr p15, 0, %0, c9, c12, 5": : "r"(pmu->cnt_sel[counter]));
		asm volatile("mrc p15, 0, %0, c9, c13, 2" : "=r"(val));
		break;
 	}

	return val;
} 

static u32 read_counter(int counter)
{
	u32 val = 0;

	val = __pj4_read_counter(counter);

	return val;
}

static void __pj4_write_counter(int counter, u32 val)
{
	int reg_shift = 0;

	switch (counter) {
	case CCNT:
		asm volatile("mcr p15, 0, %0, c9, c13, 0" : : "r" (val));
		break;
	case PMN0:
	case PMN1:
	case PMN2:
	case PMN3:
		reg_shift = 1 << pmu->cnt_sel[counter];
		asm volatile("mcr p15, 0, %0, c9, c12, 2": : "r"(reg_shift));
		asm volatile("mcr p15, 0, %0, c9, c12, 5": : "r"(pmu->cnt_sel[counter]));
		asm volatile("mcr p15, 0, %0, c9, c13, 2": : "r"(val));
		break;
	}
}

static void write_counter(int counter, u32 val)
{
	__pj4_write_counter(counter, val);
}

static void __pj4_counter_enable(int int_enable)
{
	asm volatile("mcr p15, 0, %0, c9, c12, 1": : "r"(int_enable));
} 

static void __pj4_counter_disable(int int_enable)
{ 
	asm volatile("mcr p15, 0, %0, c9, c12, 2": : "r"(int_enable));
}

static void __pj4_interrupt_set(int int_enable)
{
	asm volatile("mcr p15, 0, %0, c9, c14, 1": : "r"(int_enable));
} 

static void __pj4_interrupt_clear(int int_enable)
{ 
	asm volatile("mcr p15, 0, %0, c9, c14, 2": : "r"(int_enable));
}

static int mrvl_pj4_setup_ctrs(void)
{
	int i = 0;

	for (i = CCNT; i < MAX_COUNTERS; i++) {
		if (counter_config[i].enabled) {
			if(i != CCNT) {
				/* set type of perf counter */
				int reg_shift = 1 << pmu->cnt_sel[i];
				asm volatile("mcr p15, 0, %0, c9, c12, 2": : "r"(reg_shift));
				asm volatile("mcr p15, 0, %0, c9, c12, 5": : "r"(pmu->cnt_sel[i]));
				asm volatile("mcr p15, 0, %0, c9, c13, 1": : "r"(counter_config[i].event));
			}
		} else
			counter_config[i].event = EVT_UNUSED;
	}

	for (i = CCNT; i < MAX_COUNTERS; i++) {
		if (counter_config[i].event == EVT_UNUSED) {
			counter_config[i].event = 0;
			pmu->int_enable &= ~pmu->int_mask[i];
			continue;
		}

		results[i].reset_counter = counter_config[i].count;
		write_counter(i, -(u32)counter_config[i].count);
		pmu->int_enable |= pmu->int_mask[i];
		printk(KERN_DEBUG "mrvl_pj4_setup_ctrs: counter%d %#08x from %#08lx\n", i,
			    read_counter(i), counter_config[i].count);
	}

	return 0;
}

static void __pj4_check_ctrs(void)
{
	int i = 0;
	u32 flag = 0;
	u32 pmnc = read_pmnc();
	
	pmnc &= ~PMU_ENABLE;
	write_pmnc(pmnc);
	/* read overflow flag register */
	asm volatile("mrc p15, 0, %0, c9, c12, 3" : "=r"(flag));
	for (i = CCNT; i < MAX_COUNTERS; i++) {
		if (!(pmu->int_mask[i] & pmu->int_enable))
			continue;

		if (flag & pmu->cnt_ovf[i])
			results[i].ovf++;
		
 	}

	/* writeback clears overflow bits */
	asm volatile("mcr p15, 0, %0, c9, c12, 3": : "r"(flag));
} 

static irqreturn_t mrvl_pj4_pmu_interrupt(int irq, void *arg)
{
	int i = 0;
	u32 pmnc = read_pmnc();

	/* check counter */
	__pj4_check_ctrs();
	
	for (i = CCNT; i < MAX_COUNTERS; i++) {
		if (!results[i].ovf)
			continue;
		write_counter(i, -(u32)results[i].reset_counter);
		oprofile_add_sample(get_irq_regs(), i);
		results[i].ovf--;
 	}
	__pj4_counter_enable(pmu->int_enable);
	
	pmnc |= PMU_ENABLE;
	write_pmnc(pmnc);
	
	return IRQ_HANDLED;
} 

static void mrvl_pj4_pmu_stop(void)
{
	u32 pmnc = read_pmnc();
	
	/* disable each performonce counter & interrupt */
	__pj4_interrupt_clear(pmu->int_enable);
	__pj4_counter_disable(pmu->int_enable);

	pmnc &= ~PMU_ENABLE;
	write_pmnc(pmnc);

	free_irq(PJ4_PERFORM_MNTR_IRQ, results);
} 

static int mrvl_pj4_pmu_start(void)
{ 
	int ret = 0;
	u32 pmnc = read_pmnc();

	ret = request_irq(PJ4_PERFORM_MNTR_IRQ, mrvl_pj4_pmu_interrupt, IRQ_NOREQUEST,
			"Marvell PJ4 Performance Monitor Unit", (void *)results);
	if (ret < 0) {
		printk(KERN_ERR "oprofile: unable to request IRQ%d for PJ4 Perfromance Monitor Unit\n",
			PJ4_PERFORM_MNTR_IRQ);
		return ret;
 	} 

	/* enable each performance counter & interrupt */
	__pj4_counter_enable(pmu->int_enable);

	pmnc |= PMU_ENABLE;
	write_pmnc(pmnc);
	printk(KERN_DEBUG "marvell_mrvl_pj4_pmu_start: pmnc: %#08x mask: %08x\n", pmnc, pmu->int_enable);

	__pj4_interrupt_set(pmu->int_enable);

	return 0;
}

static int mrvl_pj4_detect_pmu(void)
{
	int ret = 0;

	pmu = &pmu_parms[PMU_PJ4];

	if (!ret) {
		op_mrvl_pj4_spec.name = pmu->name;
		op_mrvl_pj4_spec.num_counters = pmu->num_counters;
		printk(KERN_DEBUG "marvell_mrvl_pj4_detect_pmu: detected %s PMU\n", pmu->name);
	}

	return ret;
}

struct op_arm_model_spec op_mrvl_pj4_spec = {
	.init		= mrvl_pj4_detect_pmu,
	.setup_ctrs	= mrvl_pj4_setup_ctrs,
	.start		= mrvl_pj4_pmu_start,
	.stop		= mrvl_pj4_pmu_stop,
}; 

