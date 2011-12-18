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

#include "mvCommon.h"
#include "mvOs.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "mvSysPmuConfig.h"
#include "mvPmuRegs.h"
#include "mvPmu.h"
#include "mvSram.h"
#include "ddr/mvDramIf.h"

/* Constants */
#define PMU_MAX_DESCR_CNT		7		
#define PMU_STANDBY_DESCR_CNT		5
#define PMU_DEEPIDLE_DESCR_CNT		0
#define PMU_SP_TARGET_ID		0xD
#define PMU_SP_ATTRIBUTE		0x0
#define MV_PMU_POST_CFG_CNT		4			/* 3 window cfgs and DDR3 WL ODT fix */

/* Defines */
//#define MV_PMU_FULL_DDR3_WL					/* Enable FULL DDR3 write leveling */

/* Type Definitions */
typedef struct
{
	MV_U32	addr;						/* Addres fo read/modify/write */
	MV_U32	mask;						/* mask of bits to overide */
	MV_U32	off_val;					/* value to powerr off terminations */
} DDR_TERM_CTRL;

typedef struct
{
	MV_U32	nib_mask;					/* PMU Signal selector niblle mask used to force CKE */
	MV_U32	nib_frc;					/* Niblle value used to force CKE '0' @ Standby entry */
	MV_U32	nib_rel;					/* Niblle value used to release CKE @ Standby exit */
	MV_U32	bit_mask;					/* Bit mask of MPP used to foce CKE */
} MV_PMU_CKE_MASK;

typedef struct
{
	MV_U32	addr;
	MV_U32	val;
} MV_PMU_AV_CFGS;

typedef struct
{
	DDR_TERM_CTRL		term_ctrl;			/* Space Holder for DDR termination ctrl */
	MV_PMU_CKE_MASK		cke_mask;			/* Space Holder for CKE WA Mask */
	MV_DDR_INIT_POLL_AMV	init_poll;			/* DDR initialization polling info */
	MV_DDR_BIT_CLR_AM	mask_clr;			/* CKE and M_RESET cleaing info */
#ifdef MV_PMU_FULL_DDR3_WL
	MV_DDR3_WL_TYP		ddr3_wl;			/* DDR3 full write leveling info */
#else
	MV_DDR3_WL_SET_VAL	ddr3_wl;			/* DDR3 write leveling set info */ 
#endif
	MV_PMU_AV_CFGS		post_cfg[MV_PMU_POST_CFG_CNT];	/* Post DDR init configurations */	 
} MV_PMU_SRAM_DATA;

/* Statics */
static MV_PMU_AV_CFGS post_init_cfgs[MV_PMU_POST_CFG_CNT] = {
				{0xD0800770, 0x0100000A},	/* Set DDR ODT settings */
				{0xD0800010, 0xF1800000},	/* Set DDR register space */
                        	{0xD00D025C, 0x000F1890},	/* Set NB register space */
				{0xD0020080, 0xF1000000},	/* Set SB register space */
				};

/* SRAM allocated data pointers */
static MV_VOID * _mvPmuSramDataPtr;				/* Pointer to start of data section */
static MV_VOID * _mvPmuSramDdrTermCtrlPtr;			/* External Read-only access */
static MV_VOID * _mvPmuSramDdrTermCtrlPtrInt;			/* Internal Read-write access */
static MV_VOID * _mvPmuSramCkeMaskCtrlPtr;			/* External Read-only access */
static MV_VOID * _mvPmuSramCkeMaskCtrlPtrInt;			/* Internal Read-write access */
static MV_VOID * _mvPmuSramDdrInitPollPtr;			/* External Read-only access */
static MV_VOID * _mvPmuSramDdrInitPollPtrInt;			/* Internal Read-write access */
static MV_VOID * _mvPmuSramDdrMresetPtr;			/* External Read-only access */
static MV_VOID * _mvPmuSramDdrMresetPtrInt;			/* Internal Read-write access */
static MV_VOID * _mvPmuSramDdr3wlPtr;				/* External Read-only access */
static MV_VOID * _mvPmuSramDdr3wlPtrInt;			/* Internal Read-write access */
static MV_VOID * _mvPmuSramPostInitPtr;				/* External Read-only access */
static MV_VOID * _mvPmuSramPostInitPtrInt;			/* Internal Read-write access */

/* DDR parameters pointer */
static MV_VOID * _mvPmuSramDdrParamPtr;				/* External Read-only access */
static MV_VOID * _mvPmuSramDdrParamPtrInt;			/* Internal Read-write access */

/* SRAM functions pointer */
static MV_VOID * _mvPmuSramPtr;					/* Pointer to start of code section */
static MV_VOID (*_mvPmuSramDdrReconfigPtr)(MV_U32 cplPtr, MV_U32 cplCnt, MV_U32 dryRun);
static MV_VOID (*_mvPmuSramDeepIdleEnterPtr)(MV_U32 ddrSelfRefresh);
static MV_VOID (*_mvPmuSramDeepIdleExitPtr)(MV_VOID);
static MV_VOID (*_mvPmuSramStandbyEnterPtr)(MV_VOID);
static MV_VOID (*_mvPmuSramStandbyExitPtr)(MV_VOID);
static MV_VOID (*_mvPmuSramCpuDfsPtr)(MV_VOID);

/* Sram Markers */
static unsigned long mvPmuSramOffs = 0x0;
static unsigned long mvPmuSramSize = PMU_SCRATCHPAD_SIZE;

/* Macros */
#define PmuSpVirt2Phys(addr)	(((MV_U32)addr - DOVE_SCRATCHPAD_VIRT_BASE) + DOVE_SCRATCHPAD_PHYS_BASE)
#define dbg_print(args...)
//#define dbg_print(args...)	printk(KERN_WARNING args)
#define PmuExt2Int(addr) 	((MV_VOID*)(((MV_U32)addr & (PMU_SCRATCHPAD_SIZE-1)) | (PMU_SCRATCHPAD_INT_BASE)))

/*******************************************************************************
* mvPmuSramRelocate - Relocate a function into the PMU SRAM
*
* DESCRIPTION:
*   	Relocate a function into the SRAM to be executed from there.
*
* INPUT:
*       start: starting address of the function to relocated
*	size: size of the function to be relocated
* OUTPUT:
*	None
* RETURN:
*	None
*******************************************************************************/
static MV_VOID * mvPmuSramRelocate(MV_VOID * start, MV_U32 size)
{
	MV_VOID * fncptr;
	MV_U32 * src;
	MV_U32 * dst;
	MV_U32 i;
	MV_U32 orig_size = size;

	/* Round up the size to a complete cache line */
	size += 31;
	size &= ~31;

	if (size > mvPmuSramSize) {
		dbg_print("No more space in SRAM for function relocation\n");
		return NULL;
	}

	mvPmuSramSize -= size;

	src = (MV_U32*)start;
	dst = (MV_U32*)(PMU_SCRATCHPAD_INT_BASE + mvPmuSramOffs);

	if (start)
	{
		for (i=0; i<orig_size; i+=4)
		{
			*dst = *src;
			dst++;
			src++;
		}
	}	

	fncptr = (MV_VOID *)(PMU_SCRATCHPAD_EXT_BASE + mvPmuSramOffs);
	mvPmuSramOffs += size;	
	
	dbg_print("\nmvPmuSramRelocate: From %08x to %08x (exec %08x), Size = %x\n", (MV_U32)start, (MV_U32)dst, (MV_U32)fncptr, (MV_U32)size);
	return fncptr;
}

/*******************************************************************************
* mvPmuSramDdrReconfig - Reconfigure the DDR parameters to new frequency
*
* DESCRIPTION:
*   	This call executes from the SRAM and performs all configurations needed
*	while having the DDR in self refresh
*
* INPUT:
*	None
* OUTPUT:
*	None
* RETURN:
*	None
*******************************************************************************/
MV_VOID mvPmuSramDdrReconfig(MV_U32 paramcnt)
{
	if (!_mvPmuSramDdrReconfigPtr)
		panic("Function not yet relocated in SRAM\n");

	/* First DRY run to avoid speculative prefetches*/
	_mvPmuSramDdrReconfigPtr((MV_U32)_mvPmuSramDdrParamPtrInt, paramcnt, 0);

	/* Real run to perform the scalinf */
	_mvPmuSramDdrReconfigPtr((MV_U32)_mvPmuSramDdrParamPtrInt, paramcnt, 1);
	
	return;
}	

/*******************************************************************************
* mvPmuSramCpuDfs - Reconfigure the CPU speed
*
* DESCRIPTION:
*   	This call executes from the SRAM and performs all configurations needed
*	to change the CPU clock withoiut accessing the DDR
*
* INPUT:
*	None
* OUTPUT:
*	None
* RETURN:
*	None
*******************************************************************************/
MV_VOID mvPmuSramCpuDfs(MV_VOID)
{
	if (!_mvPmuSramCpuDfsPtr)
		panic("Function not yet relocated in SRAM\n");

	return _mvPmuSramCpuDfsPtr();
}

/*******************************************************************************
* mvPmuSramDeepIdle - Enter Deep Idle mode
*
* DESCRIPTION:
*   	This call executes from the SRAM and performs all configurations needed
*	to enter deep Idle mode (power down the CPU, VFP and caches)
*
* INPUT:
*	ddrSelfRefresh: Enable/Disable (0x1/0x0) DDR selfrefresh while in Deep
*                       Idle.
* OUTPUT:
*	None
* RETURN:
*	None
*******************************************************************************/
MV_VOID mvPmuSramDeepIdle(MV_U32 ddrSelfRefresh)
{
	if (!_mvPmuSramDeepIdleEnterPtr)
		panic("Function not yet relocated in SRAM\n");

	return _mvPmuSramDeepIdleEnterPtr(ddrSelfRefresh);
}

/*******************************************************************************
* mvPmuSramStandby - Enter Standby mode
*
* DESCRIPTION:
*   	This call executes from the SRAM and performs all configurations needed
*	to enter standby mode (power down the whole SoC). On exiting the Standby
*	mode the CPU returns from this call normally
*
* INPUT:
*	lcdRefresh: LCD refresh mode, enabled/disabled
* OUTPUT:
*	None
* RETURN:
*	None
*******************************************************************************/
MV_VOID mvPmuSramStandby(MV_VOID)
{
	if (!_mvPmuSramStandbyEnterPtr)
		panic("Function not yet relocated in SRAM\n");

	return _mvPmuSramStandbyEnterPtr();
}

/*******************************************************************************
* mvPmuSramInit - Load the PMU Sram with all calls functions needed for PMU
*
* DESCRIPTION:
*   	Initialize the scratch pad SRAM region in the PMU so that all routines
*	needed for deepIdle, standby and DVFS are relocated from the DDR into the
*	SRAM
*
* INPUT:
*       None
* OUTPUT:
*	None
* RETURN:
*    	MV_OK	: All Functions relocated to PMU SRAM successfully
*	MV_FAIL	: At least on function failed relocation
*******************************************************************************/
MV_STATUS mvPmuSramInit (MV_32 ddrTermGpioCtrl, MV_U32 ckeMppNum)
{
	/* Allocate all data needed in the SRAM */
	if ((_mvPmuSramDataPtr = mvPmuSramRelocate(NULL, sizeof(MV_PMU_SRAM_DATA))) == NULL)
	{
		dbg_print("Failed to allocate space for DATA on SRAM!\n");
		return MV_FAIL;
	}

	/* Point out the extenal location of each field */
	_mvPmuSramDdrTermCtrlPtr = (MV_VOID*) (_mvPmuSramDataPtr + offsetof(MV_PMU_SRAM_DATA, term_ctrl));
	_mvPmuSramCkeMaskCtrlPtr = (MV_VOID*) (_mvPmuSramDataPtr + offsetof(MV_PMU_SRAM_DATA, cke_mask));
	_mvPmuSramDdrInitPollPtr = (MV_VOID*) (_mvPmuSramDataPtr + offsetof(MV_PMU_SRAM_DATA, init_poll));
	_mvPmuSramDdrMresetPtr = (MV_VOID*) (_mvPmuSramDataPtr + offsetof(MV_PMU_SRAM_DATA, mask_clr));
	_mvPmuSramDdr3wlPtr = (MV_VOID*) (_mvPmuSramDataPtr + offsetof(MV_PMU_SRAM_DATA, ddr3_wl));
	_mvPmuSramPostInitPtr = (MV_VOID*) (_mvPmuSramDataPtr + offsetof(MV_PMU_SRAM_DATA, post_cfg));

	/* Convert DDR init poll address from External to Internal Space */
	_mvPmuSramDdrTermCtrlPtrInt = PmuExt2Int(_mvPmuSramDdrTermCtrlPtr);
	_mvPmuSramCkeMaskCtrlPtrInt = PmuExt2Int(_mvPmuSramCkeMaskCtrlPtr);
	_mvPmuSramDdrInitPollPtrInt = PmuExt2Int(_mvPmuSramDdrInitPollPtr);
	_mvPmuSramDdrMresetPtrInt = PmuExt2Int(_mvPmuSramDdrMresetPtr);
	_mvPmuSramDdr3wlPtrInt = PmuExt2Int(_mvPmuSramDdr3wlPtr);
	_mvPmuSramPostInitPtrInt = PmuExt2Int(_mvPmuSramPostInitPtr);

	dbg_print("_mvPmuSramDataPtr          = %08x\n", (MV_U32)_mvPmuSramDataPtr);
	dbg_print("_mvPmuSramDdrTermCtrlPtr   = %08x (Internal = %08x)\n", (MV_U32)_mvPmuSramDdrTermCtrlPtr, (MV_U32)_mvPmuSramDdrTermCtrlPtrInt);
	dbg_print("_mvPmuSramCkeMaskCtrlPtr   = %08x (Internal = %08x)\n", (MV_U32)_mvPmuSramCkeMaskCtrlPtr, (MV_U32)_mvPmuSramCkeMaskCtrlPtrInt);
	dbg_print("_mvPmuSramDdrInitPollPtr   = %08x (Internal = %08x)\n", (MV_U32)_mvPmuSramDdrInitPollPtr, (MV_U32)_mvPmuSramDdrInitPollPtrInt);
	dbg_print("_mvPmuSramDdrMresetPtr     = %08x (Internal = %08x)\n", (MV_U32)_mvPmuSramDdrMresetPtr, (MV_U32)_mvPmuSramDdrMresetPtrInt);
	dbg_print("_mvPmuSramDdr3wlPtr        = %08x (Internal = %08x)\n", (MV_U32)_mvPmuSramDdr3wlPtr, (MV_U32)_mvPmuSramDdr3wlPtrInt);
	dbg_print("_mvPmuSramPostInitPtr      = %08x (Internal = %08x)\n", (MV_U32)_mvPmuSramPostInitPtr, (MV_U32)_mvPmuSramPostInitPtrInt);

	/* Allocate enough space for the DDR paramters */
	if ((_mvPmuSramDdrParamPtr = mvPmuSramRelocate(NULL, (mvDramIfParamCountGet() * sizeof(MV_DDR_MC_PARAMS)))) == NULL)
	{
		dbg_print("Failed to allocate space for DDR CONFIGURATIONS on SRAM!\n");
		return MV_FAIL;
	}

	/* Convert DDR base address from External to Internal Space */
	_mvPmuSramDdrParamPtrInt =  PmuExt2Int(_mvPmuSramDdrParamPtr);

	dbg_print("_mvPmuSramDdrParamPtr      = %08x (Internal = %08x)\n", (MV_U32)_mvPmuSramDdrParamPtr, (MV_U32)_mvPmuSramDdrParamPtrInt);

	/* Relocate all PMU routines fom the DDR to the SRAM */
	if ((_mvPmuSramPtr = mvPmuSramRelocate((MV_VOID*)mvPmuSramFuncSTART, mvPmuSramFuncSZ)) == NULL)
	{
		dbg_print("Failed to allocate space for CODE on SRAM!\n");
		return MV_FAIL;
	}

	/* Relocate the DDR DFS routine */
	_mvPmuSramDdrReconfigPtr = (MV_VOID(*)) ((MV_U32)_mvPmuSramPtr + 
					((MV_U32)mvPmuSramDdrReconfigFunc - (MV_U32)mvPmuSramFuncSTART));
	/* Relocate the DeepIdle functions */
	_mvPmuSramDeepIdleEnterPtr = (MV_VOID(*)) ((MV_U32)_mvPmuSramPtr + 
					((MV_U32)mvPmuSramDeepIdleEnterFunc - (MV_U32)mvPmuSramFuncSTART));
	_mvPmuSramDeepIdleExitPtr = (MV_VOID(*)) ((MV_U32)_mvPmuSramPtr + 
					((MV_U32)mvPmuSramDeepIdleExitFunc - (MV_U32)mvPmuSramFuncSTART));

	/* Relocate the Standby functions */
	_mvPmuSramStandbyEnterPtr = (MV_VOID(*)) ((MV_U32)_mvPmuSramPtr + 
					((MV_U32)mvPmuSramStandbyEnterFunc - (MV_U32)mvPmuSramFuncSTART));
	_mvPmuSramStandbyExitPtr = (MV_VOID(*)) ((MV_U32)_mvPmuSramPtr + 
					((MV_U32)mvPmuSramStandbyExitFunc - (MV_U32)mvPmuSramFuncSTART));

	/* Relocate the CPU DFS function */
	_mvPmuSramCpuDfsPtr = (MV_VOID(*)) ((MV_U32)_mvPmuSramPtr + 
					((MV_U32)mvPmuSramCpuDfsFunc - (MV_U32)mvPmuSramFuncSTART));

	dbg_print("_mvPmuSramPtr              = %08x\n", (MV_U32)_mvPmuSramPtr);
	dbg_print("_mvPmuSramDdrReconfigPtr   = %08x\n", (MV_U32)_mvPmuSramDdrReconfigPtr);
	dbg_print("_mvPmuSramDeepIdleEnterPtr = %08x\n", (MV_U32)_mvPmuSramDeepIdleEnterPtr);
	dbg_print("_mvPmuSramDeepIdleExitPtr  = %08x\n", (MV_U32)_mvPmuSramDeepIdleExitPtr);
	dbg_print("_mvPmuSramStandbyEnterPtr  = %08x\n", (MV_U32)_mvPmuSramStandbyEnterPtr);
	dbg_print("_mvPmuSramStandbyExitPtr   = %08x\n", (MV_U32)_mvPmuSramStandbyExitPtr);
	dbg_print("_mvPmuSramCpuDfsPtr        = %08x\n", (MV_U32)_mvPmuSramCpuDfsPtr);

	/* Fill in the Post configurations in the SRAM */
	memcpy(_mvPmuSramPostInitPtr, (MV_VOID*)post_init_cfgs, sizeof(post_init_cfgs));

	/* Sanity check that the DDR temination control A/M/V info was located at the SRAM start */
	if (((MV_U32)&((DDR_TERM_CTRL*)_mvPmuSramDdrTermCtrlPtrInt)->addr != PMU_SP_TERM_OFF_CTRL_ADDR) || 
	    ((MV_U32)&((DDR_TERM_CTRL*)_mvPmuSramDdrTermCtrlPtrInt)->mask != PMU_SP_TERM_OFF_MASK_ADDR) ||
	    ((MV_U32)&((DDR_TERM_CTRL*)_mvPmuSramDdrTermCtrlPtrInt)->off_val != PMU_SP_TERM_OFF_VAL_ADDR))
	{
		dbg_print("DDR Termination control information mislocated in SRAM!\n");
		return MV_FAIL;
	}

	/* Fill in the DDR termination control information */	
	if ((ddrTermGpioCtrl >= 0) | (ddrTermGpioCtrl <= 31))
	{	
		((DDR_TERM_CTRL*)_mvPmuSramDdrTermCtrlPtrInt)->addr = 0x1;
		((DDR_TERM_CTRL*)_mvPmuSramDdrTermCtrlPtrInt)->mask = (0x1 << ddrTermGpioCtrl);
		((DDR_TERM_CTRL*)_mvPmuSramDdrTermCtrlPtrInt)->off_val = 0x0;
	}
	else
	{
		((DDR_TERM_CTRL*)_mvPmuSramDdrTermCtrlPtrInt)->addr = 0x0;
		((DDR_TERM_CTRL*)_mvPmuSramDdrTermCtrlPtrInt)->mask = 0x0;
		((DDR_TERM_CTRL*)_mvPmuSramDdrTermCtrlPtrInt)->off_val = 0x0;
	}

	/* Fill in the CKE mask control */
	if ((ckeMppNum >= 0) | (ddrTermGpioCtrl <= 7))
	{
		((MV_PMU_CKE_MASK*)_mvPmuSramCkeMaskCtrlPtr)->nib_mask = (0xF << (ckeMppNum*4));
		((MV_PMU_CKE_MASK*)_mvPmuSramCkeMaskCtrlPtr)->nib_frc = (PMU_SIGNAL_1 << (ckeMppNum*4));
		((MV_PMU_CKE_MASK*)_mvPmuSramCkeMaskCtrlPtr)->nib_rel = (PMU_SIGNAL_0 << (ckeMppNum*4));
		((MV_PMU_CKE_MASK*)_mvPmuSramCkeMaskCtrlPtr)->bit_mask = (0x1 << ckeMppNum);
	}
	else
	{
		((MV_PMU_CKE_MASK*)_mvPmuSramCkeMaskCtrlPtr)->nib_mask = 0x0;
		((MV_PMU_CKE_MASK*)_mvPmuSramCkeMaskCtrlPtr)->nib_frc = 0x0;
		((MV_PMU_CKE_MASK*)_mvPmuSramCkeMaskCtrlPtr)->nib_rel = 0x0;
		((MV_PMU_CKE_MASK*)_mvPmuSramCkeMaskCtrlPtr)->bit_mask = 0x0;
	}

	return MV_OK;
}

/*******************************************************************************
* mvPmuSramDdrTimingPrep - Prepare new DDR timing params
*
* DESCRIPTION:
*   	Request the new timing parameters for the DDR MC according to the new
*	CPU:DDR ratio requested. These parameters are saved on the SRAM to be
*	set later in the DDR DFS sequence.
*
* INPUT:
*       ddrFreq: new target DDR frequency
*	cpuFreq: CPU frequency to calculate the values againt
* OUTPUT:
*	None
* RETURN:
*	status
*******************************************************************************/
MV_STATUS mvPmuSramDdrTimingPrep(MV_U32 ddrFreq, MV_U32 cpuFreq, MV_U32 * cnt)
{
	MV_U32 clear_size = (mvDramIfParamCountGet() * sizeof(MV_DDR_MC_PARAMS));
	MV_U32 i;
	MV_U32 * ptr = (MV_U32*) _mvPmuSramDdrParamPtrInt;

	/* Clear the whole region to zeros first */
	for (i=0; i<(clear_size/4); i++)
		ptr[i] = 0x0;
		
	/* Get the new timing parameters from the DDR HAL */
	return mvDramReconfigParamFill(ddrFreq, cpuFreq, (MV_DDR_MC_PARAMS*)_mvPmuSramDdrParamPtrInt, cnt);
}

/*******************************************************************************
* mvPmuSramDeepIdleResumePrep - Prepare information needed by the BootROM to resume
*       from Deep Idle Mode.
*
* DESCRIPTION:
*	Prepare the necessary register configuration to be executed by the 
*	BootROM before jumping back to the resume code in the SRAM.
*
* INPUT:
*       None
* OUTPUT:
*	None
* RETURN:
*	status
*******************************************************************************/
MV_STATUS mvPmuSramDeepIdleResumePrep(MV_VOID)
{
	MV_U32 reg, i;

	/* set the resume address */
	MV_REG_WRITE(PMU_RESUME_ADDR_REG, PmuSpVirt2Phys(_mvPmuSramDeepIdleExitPtr)); 

	/* Prepare the resume descriptors */
	reg = ((PMU_DEEPIDLE_DESCR_CNT << PMU_RC_DISC_CNT_OFFS) & PMU_RC_DISC_CNT_MASK);
	MV_REG_WRITE(PMU_RESUME_CTRL_REG, reg); 

	/* Fill in the used descriptors */
	for (i=0; i<PMU_DEEPIDLE_DESCR_CNT; i++)
	{
		// TBD
	}

	/* Clear out all non used descriptors */
	for (i=PMU_DEEPIDLE_DESCR_CNT; i<PMU_MAX_DESCR_CNT; i++)
	{
		MV_REG_WRITE(PMU_RESUME_DESC_CTRL_REG(i), 0x0);
		MV_REG_WRITE(PMU_RESUME_DESC_ADDR_REG(i), 0x0);
	}

	return MV_OK;
}

/*******************************************************************************
* mvPmuSramStandbyResumePrep - Prepare information needed by the BootROM to resume
*       from Standby Mode.
*
* DESCRIPTION:
*	Prepare the necessary register configuration to be executed by the 
*	BootROM before jumping back to the resume code in the SRAM.
*
* INPUT:
*       ddrFreq: DDR frequency to be configured upon resume from Standby
* OUTPUT:
*	None
* RETURN:
*	status
*******************************************************************************/
MV_STATUS mvPmuSramStandbyResumePrep(MV_U32 ddrFreq)
{
	MV_U32 reg, i, cnt;
	MV_U32 clear_size = (mvDramIfParamCountGet() * sizeof(MV_DDR_MC_PARAMS));
	MV_U32 * ptr = (MV_U32*) _mvPmuSramDdrParamPtrInt;

	/* Set the resume address */
	MV_REG_WRITE(PMU_RESUME_ADDR_REG, PmuSpVirt2Phys(_mvPmuSramStandbyExitPtr));

	/* Prepare the resume control parameters */
	reg = ((PMU_STANDBY_DESCR_CNT << PMU_RC_DISC_CNT_OFFS) & PMU_RC_DISC_CNT_MASK);	 
	reg |= ((PMU_SP_TARGET_ID << PMU_RC_WIN6_TARGET_OFFS) & PMU_RC_WIN6_TARGET_MASK);
	reg |= ((PMU_SP_ATTRIBUTE << PMU_RC_WIN6_ATTR_OFFS) & PMU_RC_WIN6_ATTR_MASK);
	reg |= (DOVE_SCRATCHPAD_PHYS_BASE & PMU_RC_WIN6_BASE_MASK);
	MV_REG_WRITE(PMU_RESUME_CTRL_REG, reg);

	/**************************************/
	/* Discriptor 0: DDR timing parametrs */
	/**************************************/

	/* Prepare DDR paramters in the scratch pad for BootROM */
	for (i=0; i<(clear_size/4); i++)	
		ptr[i] = 0x0;
	if (mvDramIfParamFill(ddrFreq, (MV_DDR_MC_PARAMS*)_mvPmuSramDdrParamPtrInt, &cnt) != MV_OK)
		return MV_FAIL;
	
	reg = PMU_RD_CTRL_DISC_TYPE_32AV;
	reg |= ((cnt << PMU_RD_CTRL_CFG_CNT_OFFS) & PMU_RD_CTRL_CFG_CNT_MASK);
	MV_REG_WRITE(PMU_RESUME_DESC_CTRL_REG(0), reg);
	MV_REG_WRITE(PMU_RESUME_DESC_ADDR_REG(0), PmuSpVirt2Phys(_mvPmuSramDdrParamPtr));

	/*****************************************/
	/* Descriptor 1: DDR Initiaization delay */
	/*****************************************/

	/* Prepare the DDR init done polling descriptor */
	if (mvDramInitPollAmvFill((MV_DDR_INIT_POLL_AMV*)_mvPmuSramDdrInitPollPtrInt) != MV_OK)
		return MV_FAIL;

	reg = PMU_RD_CTRL_DISC_TYPE_POLL;
	reg |= ((1 << PMU_RD_CTRL_CFG_CNT_OFFS) & PMU_RD_CTRL_CFG_CNT_MASK);
	MV_REG_WRITE(PMU_RESUME_DESC_CTRL_REG(1), reg);
	MV_REG_WRITE(PMU_RESUME_DESC_ADDR_REG(1), PmuSpVirt2Phys(_mvPmuSramDdrInitPollPtr));

	/***************************************/
	/* Descriptor 2: M_RESET mask clearing */
	/***************************************/

	/* Prepare DDR Phy Mask clearing */
	if (mvDramInitMresetMvFill((MV_DDR_BIT_CLR_AM*)_mvPmuSramDdrMresetPtrInt) != MV_OK)
		return MV_FAIL;
	
	reg = PMU_RD_CTRL_DISC_TYPE_32BC;
	reg |= ((1 << PMU_RD_CTRL_CFG_CNT_OFFS) & PMU_RD_CTRL_CFG_CNT_MASK);
	MV_REG_WRITE(PMU_RESUME_DESC_CTRL_REG(2), reg);
	MV_REG_WRITE(PMU_RESUME_DESC_ADDR_REG(2), PmuSpVirt2Phys(_mvPmuSramDdrMresetPtr));

	/*************************************/
	/* Descriptor 3: DDR3 write leveling */
	/*************************************/

	/* Prepare DDR Phy Mask clearing */
#ifdef MV_PMU_FULL_DDR3_WL
	if (mvDramInitDdr3wlTypeFill((MV_DDR3_WL_TYP*)_mvPmuSramDdr3wlPtrInt) != MV_OK)
		return MV_FAIL;

	reg = PMU_RD_CTRL_DISC_TYPE_DDR3WL;
	reg |= ((1 << PMU_RD_CTRL_CFG_CNT_OFFS) & PMU_RD_CTRL_CFG_CNT_MASK);
	MV_REG_WRITE(PMU_RESUME_DESC_CTRL_REG(3), reg);
	MV_REG_WRITE(PMU_RESUME_DESC_ADDR_REG(3), PmuSpVirt2Phys(_mvPmuSramDdr3wlPtr));	
#else
	if (mvDramInitDdr3wlSetFill((MV_DDR3_WL_SET_VAL*)_mvPmuSramDdr3wlPtrInt) != MV_OK)
		return MV_FAIL;

	reg = PMU_RD_CTRL_DISC_TYPE_DDR3WL_SET;
	reg |= ((1 << PMU_RD_CTRL_CFG_CNT_OFFS) & PMU_RD_CTRL_CFG_CNT_MASK);
	MV_REG_WRITE(PMU_RESUME_DESC_CTRL_REG(3), reg);
	MV_REG_WRITE(PMU_RESUME_DESC_ADDR_REG(3), PmuSpVirt2Phys(_mvPmuSramDdr3wlPtr));	
#endif
	/****************************************/
	/* Descriptor 4: Window initialization  */
	/****************************************/
	reg = PMU_RD_CTRL_DISC_TYPE_32AV;
	reg |= ((MV_PMU_POST_CFG_CNT << PMU_RD_CTRL_CFG_CNT_OFFS) & PMU_RD_CTRL_CFG_CNT_MASK);
	MV_REG_WRITE(PMU_RESUME_DESC_CTRL_REG(4), reg);
	MV_REG_WRITE(PMU_RESUME_DESC_ADDR_REG(4), PmuSpVirt2Phys(_mvPmuSramPostInitPtr));
	
	/* Clear out all non used descriptors */
	for (i=PMU_STANDBY_DESCR_CNT; i<PMU_MAX_DESCR_CNT; i++)
	{
		MV_REG_WRITE(PMU_RESUME_DESC_CTRL_REG(i), 0x0);
		MV_REG_WRITE(PMU_RESUME_DESC_ADDR_REG(i), 0x0);
	}

	return MV_OK;
}

/*******************************************************************************
* mvPmuSramVirt2Phys - Convert virtual address to physical
*
* DESCRIPTION:
*	Convert virtual address to physical
*
* INPUT:
*       addr: virtual address
* OUTPUT:
*	None
* RETURN:
*	physical address
*******************************************************************************/
unsigned long mvPmuSramVirt2Phys(void *addr)
{
	return mvOsIoVirtToPhy(NULL, addr);
}
