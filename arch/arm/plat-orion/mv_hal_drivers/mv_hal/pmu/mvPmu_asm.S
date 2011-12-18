/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell 
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or 
modify this File in accordance with the terms and conditions of the General 
Public License Version 2, June 1991 (the "GPL License"), a copy of which is 
available along with the File in the license.txt file or by writing to the Free 
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or 
on the worldwide web at http://www.gnu.org/licenses/gpl.txt. 

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED 
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY 
DISCLAIMED.  The GPL License provides additional details about this warranty 
disclaimer.
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or 
modify this File under the following licensing terms. 
Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	    this list of conditions and the following disclaimer. 

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution. 

    *   Neither the name of Marvell nor the names of its contributors may be 
        used to endorse or promote products derived from this software without 
        specific prior written permission. 
    
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#define MV_ASMLANGUAGE

#include "mvCommon.h"
#include "mvOs_asm.h"
#include "mvSysPmuConfig.h"
#include "pmu/mvPmuRegs.h"



#ifndef ENTRY
#define ENTRY(name) \
	.globl name; \
	.align 4,0x90; \
	name:
#endif

	.text

ENTRY(mvPmuSramFuncSTART)

/***************************************************************
 * mvPmuSramDdrSrEnter: Enter DDR self refresh mode
 * Uses R1-R2 internally
 * Inputs:
 *    R0: PMU control register address
 * Outputs:
 *    None
 **************************************************************/
ENTRY(mvPmuSramDdrSrEnter)
	/*
	 * Set DDR in Self refresh mode and poll status bit
	 * Needed for a clean transition of the CKE 
	 */	
	ldr	r2, [r0]
	orr	r2, r2, #PMU_CTRL_MC_SR_REQ_MASK
	str	r2, [r0]
sr_exit_poll:
	ldr	r2, [r0]
	tst	r2, #PMU_CTRL_MC_SR_ACK_MASK
	beq	sr_exit_poll
	mov	pc, lr

/***************************************************************
 * mvPmuSramDdrSrExit: Exit DDR self refresh mode
 * Uses R1-R2 internally
 * Inputs:
 *    R0: PMU control register address
 * Outputs:
 *    None
 **************************************************************/
ENTRY(mvPmuSramDdrSrExit)
	/*
	 * Exit DDR self refresh mode if active
	 * CKE and M_RESET already unmasked at BootROM level
	 */	
	ldr	r2, [r0]
	bic	r2, r2, #PMU_CTRL_MC_SR_REQ_MASK
	str	r2, [r0]
sr_enter_poll:
	ldr	r2, [r0]
	tst	r2, #PMU_CTRL_MC_SR_ACK_MASK
	bne	sr_enter_poll
	mov	pc, lr

/***************************************************************
 * mvPmuSramFlushCaches: Flush all dirty lines to DRAM
 * Uses R0-R1
 * Inputs:
 *    None
 * Outputs:
 *    None
 **************************************************************/
ENTRY(mvPmuSramFlushCaches)
	/*
	 * Flush all L1 Cache
	 */
	mov	r0, #0
	mcr	p15, 0, r0, c7, c14, 0		@ D cache clean+invalidate
	mcr     p15, 0, r0, c7, c5, 6           @ flush BTAC/BTB
#ifdef CONFIG_CPU_PJ4_ERRATA_4574_4731
	mcr	p15, 0, r0, c7, c5, 0		@ I+BTB cache invalidate
#endif
	mcr	p15, 0, r0, c7, c5, 0		@ I+BTB cache invalidate

	mrc 	p15, 1, r1, c15, c12, 0
	mov	r1, r1, lsr #16
	and	r1, r1, #0xF
	cmp	r1, #0x3			@ 0x2 for V6 and 0x3 for V7
	beq	check_l2_v7
	mrc 	p15, 0, r1, c1, c0, 0		@ Read CP15 - CR
	tst	r1, #0x04000000			@ check if L2 is enabled in V6
	beq	skip_l2_clean
	b	do_l2_clean
check_l2_v7:
	mrc     p15, 0, r1, c1, c0, 1           @ read Aux CR
	tst     r1, #0x2			@ check if L2 is enabled in V7
	beq	skip_l2_clean
do_l2_clean:
	mcr 	p15, 1, r0, c7, c11, 0		@ clean all L2 cache
skip_l2_clean:
	mcr     p15, 0, r0, c7, c10, 4          @ drain write buffer
	mcr	p15, 0, r0, c7, c10, 5		@ data memory barrier
	mcr	p15, 0, r0, c7, c5, 4		@ ISB
	mov	pc, lr


/***************************************************************
 * mvPmuSramInvalidateCaches: Invalidate all caches
 * Uses R1
 * Inputs:
 *    None
 * Outputs:
 *    None
 **************************************************************/
ENTRY(mvPmuSramInvalidateCaches)
	mov	r1, #0
#ifdef CONFIG_CPU_PJ4_ERRATA_4574_4731
	mcr	p15, 0, r1, c7, c5, 0		@ Invalidate both caches, BTC, BTAC
#endif
	mcr	p15, 0, r1, c7, c5, 0		@ Invalidate both caches, BTC, BTAC
	mcr	p15, 0, r1, c8, c7, 0		@ TLB invalidate	
	mcr	p15, 1, r1, c7, c7, 0		@ Invalidate entire L2
	mcr	p15, 0, r1, c7, c10, 4		@ DSB
	mcr	p15, 0, r1, c7, c5, 4		@ Prefetch Flush - ISB
	mov	pc, lr

/***************************************************************
 * mvPmuSramWfi: Trigger PMU operation through entering WFI mode
 * Uses R0
 * Inputs:
 *    None
 * Outputs:
 *    None
 **************************************************************/
ENTRY(mvPmuSramWfi)
	mrc 	p15, 1, r0, c15, c12, 0
	mov	r0, r0, lsr #16
	and	r0, r0, #0xF
	cmp	r0, #0x3			@ 0x2 for V6 and 0x3 for V7
	beq	wfi_v7
	mov	r0, #0
	mcr	p15, 0, r0, c7, c0, 4
	mov	pc, lr
wfi_v7:
	dsb
	wfi
	mov	pc, lr

/***************************************************************
 * mvPmuSramSaveCP15: Save all CP15 context on stack
 * Uses R2-r9, SP
 * Inputs:
 *    None
 * Outputs:
 *    None
 **************************************************************/
ENTRY(mvPmuSramSaveCP15)
	/*
	 * Save the CP15 context
	 */
	mrc 	p15, 0, r2, c1, c0, 0		@ save CP15 - CR
	mrc 	p15, 0, r3, c3, c0, 0		@ save CP15 - DACR
	mrc 	p15, 0, r4, c13, c0, 0		@ save CP15 - FCSE
	mrc 	p15, 0, r5, c2, c0, 0		@ save CP15 - TTBR
	mrc     p15, 0, r6, c13, c0, 1          @ save CP15 - context ID
	mrc	p15, 1, r7, c15, c1, 0		@ save CP15 - extra features
	mrc	p15, 0, r8, c1, c0, 1		@ save CP15 - Aux CR
	mov	r9, r13				@ save resume SP
	stmfd	sp!, {r2-r9}
	mrc	p15, 1, r5, c15, c1, 1		@ save CP15 - Aux Debug Modes Ctrl 1
	mrc     p15, 0, r6, c9, c14, 0		@ save CP15 - PMC
	mrc	p15, 0, r7, c10, c2, 0		@ save CP15 - PRRR
	mrc	p15, 0, r8, c10, c2, 1		@ save CP15 - NMRR
	stmfd	sp!, {r5-r8}
	mov	pc, lr

/***************************************************************
 * mvPmuSramResumeCP15: Restore CP15 registers and start MMU
 * Uses R0, R2-R9
 * Inputs:
 *    None
 * Outputs:
 *    R2 - CP15 Control Register (CR)
 *    R5 - CP15 Translation Base Addrress (TTBR)
 *    R9 - Virtual SP
 **************************************************************/
ENTRY(mvPmuSramResumeCP15)
	ldr	r0, saved_sp			@ stack phys addr
	ldmfd	r0!, {r5-r8}
	mcr	p15, 1, r5, c15, c1, 1		@ restore CP15 - Aux Debug Modes Ctrl 1
	mcr	p15, 0, r8, c10, c2, 1		@ restore CP15 - NMRR
	mcr	p15, 0, r7, c10, c2, 0		@ restore CP15 - PRRR
	mcr     p15, 0, r6, c9, c14, 0		@ restore CP15 - PMC
	ldmfd	r0, {r2-r9}
	mcr	p15, 0, r8, c1, c0, 1		@ restore CP15 - Aux CR
#ifdef CONFIG_CPU_ENABLE_WRITECO
	mrc	p15, 0, r10, c0, c0, 0		@ Main ID Y0 or not
	and	r10, #0xF
        cmp     r10, #1

	mrcne   p15, 1, r0, c15, c2, 0          @ Auxiliary Function Reg
	bicne	r0, #0x3 << 4
	orrne	r0, r0, #CONFIG_CPU_WRITECO_CYCLE << 4  @ X0
	mcrne	p15, 1, r0, c15, c2, 0		@ Write Marvell Auxiliary Function Reg
#endif	
	mcr	p15, 1, r7, c15, c1, 0		@ restore CP15 - extra features		
	mcr     p15, 0, r6, c13, c0, 1          @ restore CP15 - context ID
	mcr 	p15, 0, r5, c2, c0, 0		@ restore CP15 - TTBR
	bic	r5, r5, #0x1f
	mcr 	p15, 0, r4, c13, c0, 0		@ restore CP15 - FCSE
	mcr 	p15, 0, r3, c3, c0, 0		@ restore CP15 - DACR
	mov	pc, lr

/***************************************************************
 * mvPmuSramMmuEnable: Enable MMU in a secure manner
 * Uses R0-R1, R2-R9
 * Inputs:
 *    R2: CR
 *    R5 - CP15 Translation Base Addrress (TTBR)
 * Outputs:
 *    None
 **************************************************************/
ENTRY(mvPmuSramMmuEnable)
	/*
	 * Translate LR to it virtual value and set the entry point
	 */
	adr	r3, resume_after_mmu		@ physical resume address
	ldr	r4, pte_virt_mask
	and	r3, r3, r4
	and	lr, lr, r4
	ldr	r1, sp_virt_offs		@ scratch pad virtual offset
	orr	r3, r3, r1
	orr	lr, lr, r1

	/*
	 * temporarily map PMU SRAM into the page table so that PA=VA
	 * otherwise prefetch abort occurs after MMU is turned on
	 */
	mov	r1, r1, lsr #20
	ldr	r0, [r5, r1, lsl #2]		@ use PMU mapping mode
	bic	r0, r0, #0xFF000000		@ Clear bits 24-31
	bic	r0, r0, #0x00F00000		@ Clear bits 20-23
	ldr	r1, sp_phys_offs
	mov	r1, r1, lsr #20
	orr	r7, r0, r1, lsl #20		@ temporay mapping
	ldr	r4, [r5, r1, lsl #2]		@ save original mapping
	str     r7, [r5, r1, lsl #2]		@ set temporary mapping

	/*
	 * Map the page table to its physical equivalent (PA=VA)
	 * Needed to restore the original PTA in virtual mode
	 */
	mov	r6, r5, lsr #20
	orr	r7, r0, r6, lsl #20		@ temporay mapping
	ldr	r10, [r5, r6, lsl #2]		@ save original mapping
	str     r7, [r5, r6, lsl #2]		@ set temporary mapping	
	mcr     p15, 0, r0, c7, c10, 4          @ drain write buffer
	mcr	p15, 0, r0, c7, c10, 5		@ data memory barrier

	.align 5
	mcr 	p15, 0, r2, c1, c0, 0		@ restore CP15 - CR
	mov 	r0, #0
	mcr	p15, 0, r0, c7, c5, 4		@ Prefetch Flush - ISB
	mov	pc, r3				@ jump to Virtual Space

resume_after_mmu:
	str	r4, [r5, r1, lsl #2]		@ restore original PMU mapping
	str	r10, [r5, r6, lsl #2]		@ restore original PT mapping
	/*
	 * No need for cach flushes since the 2 PT entries are strongly ordered
	 */
	mcr     p15, 0, r0, c7, c10, 4          @ drain write buffer
	mcr     p15, 0, r0, c7, c10, 5          @ data memory barrier

	/* invalidate the L2 entry of the temporary mapping */
	orr	r4, r5, r1, lsl #2
	bic	r4, r4, #0x1F
	mcr	p15, 1, r4, c7, c7, 3

	mcr	p15, 0, r0, c8, c7, 0		@ TLB invalidate
	mcr     p15, 0, r0, c7, c5, 4		@ ISB
	mov	r13, r9
	mov	pc, lr

/***************************************************************
 * mvPmuSramDdrReconfigFunc: Change the DDR frequency
 * Inputs:
 *    r0: address of first Addr/Value couple
 *    r1: count of Addr/Value couples
 *    r2: Dry run flag (0 - dry run, 1 - real run)
 * Outputs:
 *    None
 **************************************************************/
ENTRY(mvPmuSramDdrReconfigFunc)
	stmfd	sp!, {r4 - r10, lr}	@ save registers on stack

	mov	r9, r0
	mov	r10, r1
	mov     r11, r2

        /*
         * Verify that the SRAM PTE is found in dTLB
         * iTLB is guranteed.... Already in the SRAM
         */
	ldr     r2, pmu_sp_addr

	/* 
	 * Disable All SB access to DDR
	 */
	ldr	r3, ddfs_arbit_val
	ldr	r2, ddfs_arbit_low_reg	
	ldr	r7, [r2]			@ r7 saves Arbitor HI
	cmp	r11, #0
	strne	r3, [r2]
	ldr	r2, ddfs_arbit_hi_reg
	ldr	r8, [r2]			@ r8 saves Arbitor LOW
	cmp	r11, #0
	strne	r3, [r2]

	/*
	 * Disable and flush BPU
	 */
	mrc	p15, 0, r12, c1, c0, 0		@ read and save CP15 CR
	bic	r2, r12, #0x800			@ clear BPU bit11
	cmp	r11, #0
	mcrne	p15, 0, r2, c1, c0, 0		@ disable BPU
	mov	r2, #0
	cmp	r11, #0
	mcrne	p15, 1, r2, c15, c12, 4		@ BPU flush

	/*
	 * Drain Write buffer
	 */
	mcr     p15, 0, r0, c7, c10, 4          @ drain write buffer
	mcr	p15, 0, r0, c7, c10, 5		@ data memory barrier
	mcr     p15, 0, r0, c7, c5, 4		@ ISB

	/*
	 * Block all data requests to the MC from the SB
	 */
	ldr	r2, ddfs_mc_ctrl_reg
	mov	r3, #0x1
	cmp	r11, #0
	strne	r3, [r2]
	ldr	r3, [r2]

	/*
	 * Set DDR in Self refresh mode and poll status bit
	 */
	ldr	r2, pmu_ctrl_reg_virt
	ldr	r4, [r2]
	orr	r4, r4, #PMU_CTRL_MC_SR_REQ_MASK
	cmp	r11, #0
	strne	r4, [r2]
ddr_fs_sr_loop:
	ldr	r4, [r2]
	cmp	r11, #0
	moveq	r4, #PMU_CTRL_MC_SR_ACK_MASK
	tst	r4, #PMU_CTRL_MC_SR_ACK_MASK
	beq	ddr_fs_sr_loop

	/*
	 * Enter WFI
	 */	
	mrc 	p15, 1, r0, c15, c12, 0
	mov	r0, r0, lsr #16
	and	r0, r0, #0xF
	cmp	r0, #0x3			@ 0x2 for V6 and 0x3 for V7
	beq	ddr_fs_wfi_v7
	cmp     r11, #0
	movne	r0, #0
	mcrne	p15, 0, r0, c7, c0, 4
	b	ddr_fs_wfi_done
ddr_fs_wfi_v7:
	cmp     r11, #0
	itt	ne
	dsb
	wfi
ddr_fs_wfi_done:

	/*
	 * Exit DDR self refresh mode if active
	 */
	ldr	r0, pmu_ctrl_reg_virt
	ldr	r2, [r0]
	tst 	r2, #PMU_CTRL_MC_SR_REQ_MASK
	beq	skip_ddrfs_sr_exit
	bic	r2, r2, #PMU_CTRL_MC_SR_REQ_MASK
	cmp	r11, #0
	strne	r2, [r0]
ddrfs_sr_exit_loop:
	ldr	r2, [r0]
	cmp	r11, #0
	moveq	r2, #PMU_CTRL_MC_SR_ACK_MASK
	tst	r2, #PMU_CTRL_MC_SR_ACK_MASK
	bne	ddrfs_sr_exit_loop
skip_ddrfs_sr_exit:	

	/*
	 * Loop and execute MC init addr/value couples
	 */
	mov	r0, r9
	mov	r1, r10
	ldr	r4, ddfs_nb_offset
	ldr	r5, ddfs_nb_mask
mc_init_loop:
	cmp	r1, #0
	beq	mc_init_done
	ldr 	r2, [r0]			@ Address
	add	r0, #4
	ldr	r3, [r0]			@ Value
	add	r0, #4
	bic	r2, r2, r5			
	orr	r2, r2, r4			@ NB Offset
	cmp	r11, #0
	strne	r3, [r2]			@ [Address]=Value
	sub	r1, r1, #1
	b	mc_init_loop
mc_init_done:
	ldr	r3, [r2]

	/*
	 * UN-Block data requests to the MC from the SB
	 */
	ldr	r2, ddfs_mc_ctrl_reg
	mov	r3, #0x0
	cmp	r11, #0
	strne	r3, [r2]
	ldr	r3, [r2]

	/*
	 * Enable and flush BPU
	 */
	cmp	r11, #0
	mcrne	p15, 0, r12, c1, c0, 0		@ restore saved CP15 CR

	/* 
	 * Restore SB access ro DDR
	 */
	ldr	r2, ddfs_arbit_low_reg
	cmp	r11, #0
	strne	r7, [r2]			@ restore Arbitor HI
	ldr	r2, ddfs_arbit_hi_reg
	cmp	r11, #0
	strne   r8, [r2]                        @ restore Arbitor LOW

	ldmfd	sp!, {r4 - r10, pc}	@ restore regs and return

ddfs_nb_offset:
	.word (DOVE_NB_REGS_VIRT_BASE)
ddfs_nb_mask:
	.word (0xFFF00000)
ddfs_mc_ctrl_reg:
	.word (DOVE_NB_REGS_VIRT_BASE + 0x7E0)

ddfs_arbit_low_reg:
	.word(DOVE_SB_REGS_VIRT_BASE + 0xD02A8)
ddfs_arbit_hi_reg:
	.word(DOVE_SB_REGS_VIRT_BASE + 0xD02AC)
ddfs_arbit_val:
	.word(0xFFFFFFFF)


/***************************************************************
 * mvPmuSramDeepIdleEnterFunc: Enter Deep Idle mode
 * Inputs:
 *    r0: 0x0: enter ddr SR, else enter
 * Outputs:
 *    None
 **************************************************************/
ENTRY(mvPmuSramDeepIdleEnterFunc)
	stmfd	sp!, {r4-r12, lr}		@ save general regs

	/*
	 * Save the CP15 context
	 */
	bl	mvPmuSramSaveCP15

	/*
	 * Save the physical address of the resume SP
	 */
	mov	r4, r0				@ suspend mode needed later
	mov	r0, sp
	ldr	r1, die_vtp
	adr	lr, sb_pc_v2p_res
	mov	pc, r1
sb_pc_v2p_res:
	adr	r1, saved_sp

	/*
	 * Write SP to scratch NVRAM
	 * First Convert external to internal addrress space
	 */
	ldr	r3, nvram_sp_mask
	and	r1, r1, r3
	ldr	r3, pmu_sp_addr
	orr	r1, r1, r3
	str	r0, [r1]

	bl	mvPmuSramFlushCaches		@ Flush all caches back to DRAM
	cmp	r4, #0				@ eBook / Deep-Idle selector
	bne	die_leave_ddr_active		@ Keep DDR alive in eBook mode
	ldr	r0, pmu_ctrl_reg_virt
	bl	mvPmuSramDdrSrEnter		@ Force DDR in SR mode

	/*
	 * Disable DDR Terminations
	 */
	ldr	r0, die_term_ctrl_addr
	ldr	r0, [r0]
	cmp	r0, #0x0
	beq	die_leave_ddr_active
	ldr	r2, die_term_mask_addr
	ldr	r2, [r2]
	ldr	r0, die_gpio_out_val_reg
	ldr	r1, [r0]
	bic	r1, r1, r2
	str	r1, [r0]

die_leave_ddr_active:
	/*
	 * Initiate Deep Idle flow
	 */
	ldr	r0, pmu_ctrl_reg_virt
	mov	r1, #PMU_CTRL_DEEPIDLE_MODE_EN_MASK
	ldr	r2, [r0]
	orr	r2, r2, r1
	str	r2, [r0]

	/*
	 * Relocate the Internal Registers Window to h/w default
	 */
	ldr	r1, die_irb_reg
	mov	r0, #DOVE_SB_REGS_HW_DEF_PHYS_BASE
	str	r0, [r1]	
	
	bl	mvPmuSramWfi			@ Enter WFI to trigger PMU

die_vtp:
	.word mvPmuSramVirt2Phys
die_irb_reg:
	.word (DOVE_SB_REGS_VIRT_BASE + 0x20080)
die_gpio_out_val_reg:
	.word (DOVE_SB_REGS_VIRT_BASE + 0xD0400)
die_term_ctrl_addr:
	.word (PMU_SP_TERM_OFF_CTRL_ADDR)
die_term_mask_addr:
	.word (PMU_SP_TERM_OFF_MASK_ADDR)

/***************************************************************
 * mvPmuSramDeepIdleExitFunc: Rollback from Deep Idle mode
 * Inputs:
 *    None
 * Outputs:
 *    None
 **************************************************************/
ENTRY(mvPmuSramDeepIdleExitFunc)
	/*
	 * Move back Internal regs to Linux base
	 */
	ldr	r1, dix_irbd_reg
	mov	r0, #DOVE_SB_REGS_PHYS_BASE
	str	r0, [r1]

	/*
	* DDR Sync workaround
	*/
	ldr     r1, dix_ddr_sync_reg
	mov	r0, #0x80000000
	str     r0, [r1]
	ldr	r0, [r1]

	bl	mvPmuSramInvalidateCaches	@ Invalidate all caches	

	/*
	 * Check if DDR in self refresh mode
	 */
	ldr	r0, pmu_ctrl_reg_phys
//	mov	r1, #PMU_CTRL_MC_SR_REQ_MASK
	ldr	r2, [r0]
	tst 	r2, #PMU_CTRL_MC_SR_REQ_MASK
	beq	dix_ddr_active

	/*
	 * Re-enable DDR terminations
	 */
	ldr	r0, dix_term_ctrl_addr
	ldr	r0, [r0]
	cmp	r0, #0x0
	beq	dix_sr_exit
	ldr	r2, dix_term_mask_addr
	ldr	r2, [r2]
	ldr	r0, dix_gpio_out_val_reg
	ldr	r1, [r0]
	orr	r1, r1, r2
	str	r1, [r0]

	mov	r1, #0x8000
dix_term_enable_delay:
	sub	r1, r1, #1
	cmp	r1, #0
	bne	dix_term_enable_delay

dix_sr_exit:

	ldr	r0, pmu_ctrl_reg_phys
	bl	mvPmuSramDdrSrExit
dix_ddr_active:	

	bl	mvPmuSramResumeCP15		@ restore CP15 registers
	bl	mvPmuSramMmuEnable		@ Enable MMU translation
	ldmfd	sp!, {r4-r12, pc}		@ restore SVC registers

dix_irbd_reg:
	.word (0xD0020080)
dix_gpio_out_val_reg:
	.word (DOVE_SB_REGS_PHYS_BASE + 0xD0400)
dix_term_ctrl_addr:
	.word (PMU_SP_TERM_OFF_CTRL_ADDR_PHYS)
dix_term_mask_addr:
	.word (PMU_SP_TERM_OFF_MASK_ADDR_PHYS)
dix_ddr_sync_reg:
	.word (0xF1800240)


/***************************************************************
 * mvPmuSramStandbyEnterFunc: Enter Standby mode
 * Inputs:
 *    None
 * Outputs:
 *    None
 **************************************************************/
ENTRY(mvPmuSramStandbyEnterFunc)

	stmfd	sp!, {r4-r12, lr}		@ save general regs

	bl	mvPmuSramSaveCP15		@ save all cp15 context on stack

	/*
	 * Save the physical address of the resume SP
	 */
	mov	r4, r0				@ suspend mode needed later
	mov	r0, sp
	ldr	r1, sbe_vtp
	adr	lr, sbe_pc_v2p_res
	mov	pc, r1
sbe_pc_v2p_res:
	adr	r1, saved_sp

	/*
	 * Write SP to scratch NVRAM
	 * Convert to internal addrress space
	 */
	ldr	r2, nvram_sp_mask
	and	r1, r1, r2
	ldr	r2, pmu_sp_addr
	orr	r1, r1, r2
	str	r0, [r1]

	bl	mvPmuSramFlushCaches		@ Flush all Caches back to DRAM
	ldr	r0, pmu_ctrl_reg_virt
	bl	mvPmuSramDdrSrEnter		@ Force DDR SR mode

	/*
	 * Force CKE to be LOW at board level
	 */
	ldr	r0, sbe_pmu_sig_slct_reg
	ldr	r1, [r0]
	ldr	r2, sbe_cke_nib_mask
	ldr	r2, [r2]
	cmp	r2, #0x0
	beq	sbe_skip_cke_wa
	bic	r1, r1, r2
	ldr	r2, sbe_cke_nib_frc
	ldr	r2, [r2]
	orr	r1, r1, r2
	str	r1, [r0]
sbe_skip_cke_wa:

	/*
	 * Initiate standby flow
	 */
	ldr	r0, pmu_ctrl_reg_virt
	mov	r1, #(PMU_CTRL_STANDBY_MODE_EN_MASK | PMU_CTRL_DDR_SLF_RFRSH_EN_MASK)
	ldr	r2, [r0]
	orr	r2, r2, r1
	str	r2, [r0]

	bl	mvPmuSramWfi			@ Enter WFI to trigger PMU

sbe_vtp:
	.word mvPmuSramVirt2Phys
sbe_sply_ctrl_reg:
	.word (DOVE_SB_REGS_VIRT_BASE + PMU_PWR_SUPLY_CTRL_REG)
sbe_pmu_sig_slct_reg:
	.word (DOVE_SB_REGS_VIRT_BASE + PMU_SIG_SLCT_CTRL_0_REG)
sbe_cke_nib_mask:
	.word (PMU_SP_CKE_NIB_MASK_ADDR)
sbe_cke_nib_frc:
	.word (PMU_SP_CKE_NIB_FRC_ADDR)



/***************************************************************
 * mvPmuSramStandbyExitFunc: Rollback from Standby mode
 * Inputs:
 *    None
 * Outputs:
 *    None
 **************************************************************/
ENTRY(mvPmuSramStandbyExitFunc)
	bl	mvPmuSramInvalidateCaches	@ Invalidate all caches

	/*
	 * Check if CKE WA is implemented on board
	 */
	ldr	r3, sbx_cke_bit_mask
	ldr	r3, [r3]
	cmp	r3, #0x0
	beq	sbx_skip_cke_wa

	/*
	 * Set DDR in Self refresh mode and poll status bit
	 */
	ldr	r0, pmu_ctrl_reg_phys
	bl	mvPmuSramDdrSrEnter

	/*
	 * UN-Mask M_RESET and CKE at board level
	 */
	ldr	r0, sbx_mpp_pmu_slct_reg
	ldr	r1, [r0]
	orr	r1, r1, r3			@ Select MPP used for CKE control as PMU MPP
	str	r1, [r0]

	ldr	r0, sbx_pmu_sig_slct_reg
	ldr	r1, [r0]
	ldr	r2, sbx_cke_nib_mask
	ldr	r2, [r2]
	bic	r1, r1, r2
	ldr	r2, sbx_cke_nib_rel		@ Set PMU MPP to '0' -> release CKE forcing
	ldr	r2, [r2]
	orr	r1, r1, r2
	str	r1, [r0]
sbx_skip_cke_wa:

	/*
	 * Exit DDR Self refresh mode and poll status bit
	 */
	ldr	r0, pmu_ctrl_reg_phys
	bl	mvPmuSramDdrSrExit

	bl	mvPmuSramResumeCP15		@ restore CP15 registers
	bl	mvPmuSramMmuEnable		@ Enable MMU translation
	ldmfd	sp!, {r4-r12, pc}		@ restore SVC registers

sbx_mpp_pmu_slct_reg:
	.word (DOVE_SB_REGS_PHYS_BASE + 0xD0210)
sbx_cke_bit_mask:
	.word (PMU_SP_CKE_BIT_MASK_ADDR_PHYS)
sbx_pmu_sig_slct_reg:
	.word (DOVE_SB_REGS_PHYS_BASE + PMU_SIG_SLCT_CTRL_0_REG)
sbx_cke_nib_mask:
	.word (PMU_SP_CKE_NIB_MASK_ADDR_PHYS)
sbx_cke_nib_rel:
	.word (PMU_SP_CKE_NIB_REL_ADDR_PHYS)


/***************************************************************
 * mvPmuSramCpuDfsFunc: Change the CPU frequency
 * Inputs:
 *    None
 * Outputs:
 *    None
 **************************************************************/
ENTRY(mvPmuSramCpuDfsFunc)
	stmfd	sp!, {r0 - r10, lr}	@ save registers on stack
	ldr	r0, pmu_ctrl_reg_virt
	bl	mvPmuSramDdrSrEnter	@ Force DRAM in SR mode
	bl	mvPmuSramWfi		@ Enter WFI to trigger PMU
	ldr	r0, pmu_ctrl_reg_virt
	bl	mvPmuSramDdrSrExit	@ Exit SR mode
	ldmfd	sp!, {r0 - r10, pc}	@ restore regs and return


/***************************************************************
 *    SHARED LITERAL POOL
 **************************************************************/
saved_sp:
	.word	0				@ physical SP saved here
sp_virt_offs:
	.word (DOVE_SCRATCHPAD_VIRT_BASE)
sp_phys_offs:
	.word (DOVE_SCRATCHPAD_PHYS_BASE)
pmu_ctrl_reg_virt:
	.word (DOVE_SB_REGS_VIRT_BASE + PMU_CTRL_REG)
pmu_ctrl_reg_phys:
	.word (DOVE_SB_REGS_PHYS_BASE + PMU_CTRL_REG)
pmu_sp_addr:
	.word (PMU_SCRATCHPAD_INT_BASE)
pte_virt_mask:
	.word (0xFFFFF)
nvram_sp_mask:
	.word (0x7FF)

ENTRY(mvPmuSramFuncSZ)
	.word	. - mvPmuSramFuncSTART + 0x4
