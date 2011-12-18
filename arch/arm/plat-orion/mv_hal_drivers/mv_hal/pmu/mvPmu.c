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

#warning "This include should be removed."
#include "ctrlEnv/sys/mvCpuIfRegs.h"

#define ROUND_DOWN(value,boundary)	((value) & (~((boundary)-1)))

/* PMU uController code fixes <add,val> couples */
MV_U32	uCfix[][2] = {{0,0x01}, {38, 0x33}, {120,0xB0}, {121,0x00}, {122,0x80}, {123,0xC0}, {124,0x00}, \
		{125,0x00}, {126,0x00}, {127,0x00}, {214,0x00}, {239,0xB0}, {241,0x90}, {242,0x20}, \
		{243,0x80}, {244,0x04}, {245,0x07}, {246,0x00},	{247,0x39}, {248,0xA8}, {249,0x14}, \
		{250,0x80}, {251,0x80}, {252,0x28}, {253,0x80}, {254,0xFF}, {255,0x08},	{256,0x20}, \
		{257,0xF0}, {258,0x44}, {259,0x00}, {260,0xF3}, {261,0xF0}, {262,0x3C}, {263,0x00}, \
		{264,0xFF}, {265,0x90}, {266,0x40}, {267,0x00}, {268,0x04}, {269,0x00}, {270,0x00}, \
		{271,0xA0}, {272,0x24}, {273,0x80}, {274,0x3F}, {275,0x00}, {276,0x00}, {277,0x20}, \
		{278,0xA8}, {279,0x24}, {280,0x80}, {281,0x90}, {282,0x28}, {283,0x80}, {284,0x03}, \
		{285,0x01}, {286,0x02}, {287,0x20}, {288,0x60}, {289,0x5B}, {290,0x00}, {291,0x59}, \
		{292,0x90}, {293,0x00}, {294,0x80}, {295,0xC0}, {296,0x00}, {297,0x00}, {298,0x00}, \
		{299,0x00}, {300,0x90}, {301,0x20}, {302,0x80}, {303,0x04}, {304,0x00}, {305,0x00}, \
		{306,0x19}, {307,0x94}, {308,0x20}, {309,0x7C},	{310,0xFF}, {311,0x00}, {356,0x00}, \
		{388,0x71}, {389,0x30}, {390,0x80}, {391,0x28}, {392,0x80}, {393,0xFF}, {394,0x08}, \
		{395,0x49}, {396,0x39}, {397,0x20}, {398,0xA8}, {399,0x18}, {400,0x80}, {401,0x69}, \
		{402,0x59}, {403,0x08}, {404,0x94}, {405,0x41}, {406,0x00}, {407,0x7D}, {408,0x19}, \
		{409,0x94}, {410,0x40}, {411,0x94}, {412, 0x42}, {413, 0xFF}};

/* Save L2 ratio at system power up */
static MV_U32 l2TurboRatio;
static MV_U32 dvsDefaultValues;

/* PLL 2 DDR Ratio Divider Map Table */
#define MAX_RATIO_MAP_CNT	10
MV_U32 ratio2DivMapTable[MAX_RATIO_MAP_CNT][2] = {{1,0x1}, 	/* DDR:PLL 1:1 */
						{2,0x2}, 	/* DDR:PLL 1:2 */
						{3,0x3}, 	/* DDR:PLL 1:3 */
						{4,0x4}, 	/* DDR:PLL 1:4 */
						{5,0x5}, 	/* DDR:PLL 1:5 */
						{6,0x6}, 	/* DDR:PLL 1:6 */
						{7,0x7}, 	/* DDR:PLL 1:7 */
						{8,0x8}, 	/* DDR:PLL 1:8 */
						{10,0xA}, 	/* DDR:PLL 1:10 */
						{12,0xC}}; 	/* DDR:PLL 1:12 */

/* Timing delays for CPU and Core powers */
#define PMU_DEEPIDLE_CPU_PWR_DLY	0x208D			/* TCLK clock cycles - ~50 us */
#define PMU_STBY_CPU_PWR_DLY		0x1			/* RTC 32KHz clock cycles ~31.25us */
#define PMU_DVS_POLL_DLY		10000			/* Poll DVS done count */

/* Standby blinking led timing */
#define PMU_STBY_LED_PERIOD		0x10000			/* RTC 32KHz cc ~2.0s */
#define PMU_STBY_LED_DUTY		0xE000			/* RTC 32KHz cc ~1.75s - 12.5%*/

/* Static functions */
static MV_U32 mvPmuGetPllFreq(void);
static MV_U32 mvPmuRatio2Divider(MV_U32 ratio);
static MV_U32 mvPmuDivider2Ratio(MV_U32 div);


/*******************************************************************************
* mvPmuCodeFix - Fix teh PMU uCode
*
* DESCRIPTION:
*   Apply the PMU uCode and function pointers fixes
*
* INPUT:
	None
* OUTPUT:
*	None
* RETURN:
*	None
*******************************************************************************/
static MV_VOID mvPmuCodeFix (MV_VOID)
{
	MV_U32 reg;
	MV_U32 i;

	/* Initialize the uCode PCs of the different PMU operations */
	MV_REG_WRITE(PMU_DFS_PROC_PC_0_REG, ((MV_REG_READ(PMU_DFS_PROC_PC_0_REG) & ~PMU_DFS_PRE_PC_MASK) | PMU_DFS_PRE_PC_VAL));
	MV_REG_WRITE(PMU_DFS_PROC_PC_0_REG, ((MV_REG_READ(PMU_DFS_PROC_PC_0_REG) & ~PMU_DFS_CPU_PC_MASK) | PMU_DFS_CPU_PC_VAL));
	MV_REG_WRITE(PMU_DFS_PROC_PC_1_REG, ((MV_REG_READ(PMU_DFS_PROC_PC_1_REG) & ~PMU_DFS_DDR_PC_MASK) | PMU_DFS_DDR_PC_VAL));
	MV_REG_WRITE(PMU_DFS_PROC_PC_1_REG, ((MV_REG_READ(PMU_DFS_PROC_PC_1_REG) & ~PMU_DFS_L2_PC_MASK) | PMU_DFS_L2_PC_VAL));
	MV_REG_WRITE(PMU_DFS_PROC_PC_2_REG, ((MV_REG_READ(PMU_DFS_PROC_PC_2_REG) & ~PMU_DFS_PLL_PC_MASK) | PMU_DFS_PLL_PC_VAL));
	MV_REG_WRITE(PMU_DFS_PROC_PC_2_REG, ((MV_REG_READ(PMU_DFS_PROC_PC_2_REG) & ~PMU_DFS_POST_PC_MASK) | PMU_DFS_POST_PC_VAL));
	MV_REG_WRITE(PMU_DVS_PROC_PC_REG, ((MV_REG_READ(PMU_DVS_PROC_PC_REG) & ~PMU_DVS_PRE_PC_MASK) | PMU_DVS_PRE_PC_VAL));
	MV_REG_WRITE(PMU_DVS_PROC_PC_REG, ((MV_REG_READ(PMU_DVS_PROC_PC_REG) & ~PMU_DVS_POST_PC_MASK) | PMU_DVS_POST_PC_VAL));
	MV_REG_WRITE(PMU_DEEPIDLE_STBY_PROC_PC_REG, ((MV_REG_READ(PMU_DEEPIDLE_STBY_PROC_PC_REG) & ~PMU_DPIDL_PC_MASK) | PMU_DPIDL_PC_VAL));
	MV_REG_WRITE(PMU_DEEPIDLE_STBY_PROC_PC_REG, ((MV_REG_READ(PMU_DEEPIDLE_STBY_PROC_PC_REG) & ~PMU_STBY_PRE_PC_MASK) | PMU_STBY_PRE_PC_VAL));
	MV_REG_WRITE(PMU_STBY_PROC_PC_REG, ((MV_REG_READ(PMU_STBY_PROC_PC_REG) & ~PMU_STBY_DDR_PC_MASK) | PMU_STBY_DDR_PC_VAL));
	MV_REG_WRITE(PMU_STBY_PROC_PC_REG, ((MV_REG_READ(PMU_STBY_PROC_PC_REG) & ~PMU_STBY_POST_PC_MASK) | PMU_STBY_POST_PC_VAL));

	/* Update uCode */
	for (i=0; i< (sizeof(uCfix) / sizeof(uCfix[0])); i++)
	{
		reg = MV_REG_READ(PMU_PROGRAM_REG(uCfix[i][0]));
		reg &= ~PMU_PROGRAM_MASK(uCfix[i][0]);
		reg |= (uCfix[i][1] << PMU_PROGRAM_OFFS(uCfix[i][0]));
		MV_REG_WRITE(PMU_PROGRAM_REG(uCfix[i][0]), reg);
	}
}

/*******************************************************************************
* mvPmuInit - Initialize the Power Management Unit (PMU)
*
* DESCRIPTION:
*   1. Initializes the internal fields of the data stucture.
*   2. initialize the controller and loads default values.
*
* INPUT:
*       Pointer to PMU information Data structure, this sturcture must be allocated
*         by the caller and passed to the mvPmuInit()
* OUTPUT:
*		None
* RETURN:
*    MV_OK      : PMU initialized successfully
*******************************************************************************/
MV_STATUS mvPmuInit (MV_PMU_INFO * pmu)
{
	MV_U32 reg;
	MV_32 i;
	MV_32 cke_mpp_num = -1;

	/* Set the DeepIdle and Standby power delays */
      	MV_REG_WRITE(PMU_STANDBY_PWR_DELAY_REG, pmu->standbyPwrDelay);

	/* Configure the Battery Management Control register */
	reg = 0;
	if (pmu->batFltMngDis) reg |= PMU_BAT_FLT_DISABLE_MASK;
	if (pmu->exitOnBatFltDis) reg |= PMU_BAT_FLT_STBY_EXIT_DIS_MASK;
	MV_REG_WRITE(PMU_BAT_MNGMT_CTRL_REG, reg);
	
	/* Invert the CPU and whole chip Power Down Polarity */
	reg = MV_REG_READ(PMU_PWR_SUPLY_CTRL_REG);
	reg &= ~PMU_PWR_CPU_OFF_LEVEL_MASK;	/* 0 - CPU is powered down (DEEP IDLE) */
	reg |= PMU_PWR_CPU_ON_LEVEL_MASK;	/* 1 - CPU is powered up */
	reg &= ~PMU_PWR_STBY_ON_LEVEL_MASK;	/* 0 - Dove is powered down (STANDBY) */
	reg |= PMU_PWR_STBY_OFF_LEVEL_MASK;	/* 1 - Dove is powered up */
	MV_REG_WRITE(PMU_PWR_SUPLY_CTRL_REG, reg);

	/* Enable the PMU Control on the CPU reset) */
	reg = MV_REG_READ(CPU_CONTROL_REG);
	reg |= CPU_CTRL_PMU_CPU_RST_EN_MASK;
	MV_REG_WRITE(CPU_CONTROL_REG, reg);

	/* Configure the PMU Signal selection */
	reg = 0;
	for (i=7; i>=0; i--)
		reg |= ((pmu->sigSelctor[i] & 0xF) << (i*4));
	MV_REG_WRITE(PMU_SIG_SLCT_CTRL_0_REG, reg);

	reg = 0;
	for (i=7; i>=0; i--)
		reg |= ((pmu->sigSelctor[i+8] & 0xF) << (i*4));
	MV_REG_WRITE(PMU_SIG_SLCT_CTRL_1_REG, reg);

	/* Configure the CPU Power Good if used */
	for (i=0; i<16; i++)
	{
		if (pmu->sigSelctor[i] == PMU_SIGNAL_CPU_PWRGOOD)
		{
			reg = MV_REG_READ(PMU_PWR_SUPLY_CTRL_REG);
			reg |= PMU_PWR_GOOD_PIN_EN_MASK;
			MV_REG_WRITE(PMU_PWR_SUPLY_CTRL_REG, reg);
			break;
		}
	}

	/* Check if CKE workaround is applicable */
	for (i=0; i<8; i++)
	{
		if (pmu->sigSelctor[i] == PMU_SIGNAL_CKE_OVRID)
		{
			cke_mpp_num = i;
			break;
		}
	}

	/* Configure the blinking led timings */
	MV_REG_WRITE(PMU_BLINK_PERIOD_CTRL_REG, PMU_STBY_LED_PERIOD);
	MV_REG_WRITE(PMU_BLINK_DUTY_CYCLE_CTRL_REG, PMU_STBY_LED_DUTY);

	/* Configure the DVS delay */
	MV_REG_WRITE(PMU_DVS_DELAY_REG, pmu->dvsDelay);
	dvsDefaultValues = pmu->dvsValues;
	/* Apply uCode Fixes */
	mvPmuCodeFix();

	/* Save L2 ratio at reset - Needed in DFS scale up */
	reg = MV_REG_READ(PMU_CLK_DIVIDER_0_REG);
	l2TurboRatio = (((reg & PMU_CLK_DIV_XPRATIO_MASK) >> PMU_CLK_DIV_XPRATIO_OFFS) << PMU_DFS_CTRL_L2_RATIO_OFFS);
	

	return mvPmuSramInit(pmu->ddrTermGpioNum, cke_mpp_num);
}

/*******************************************************************************
* mvPmuDeepIdle - Power down the CPU and enter WFI. This api blocks untill
*                receiving an interrupt, which wakes up the CPU and this
*                api returns to the caller.
*
* DESCRIPTION:
* The caller of this API is responsible for the following before calling this call:
*   - Disable the I-Cache
*   - Configure the Wake up events.
*   - Save all CPU context
*   - Gate all clocks to unused units
*
* The caller of this API is also responsible for the following after exiting this call:
*   - Restore the CPU context.
*   - Restore clocks to gated units
*   - Re-Enable the I-Cache
*
* This API performs the following steps (^ Ò steps that can be skipped if LCD refresh is not needed):
*  ^1. Wait for the LCD EndofScreen interrupt (using the callback suppiled in init).
*   2. Mask both IRQ and FIQ interrupts at the PMU level.
*   3. Prepare in the PMU scratch pad (SRAM) with the needed code to execute from there.
*   4. Read the DDR configuration paramters and write them to the scratch pad.
*   5. Jump to the scratch pad and start executing from there.
*   6. Block all data requests to the DDR controller (from the SB).
*  ^7. Disable DRAM DLL via a load mode register command (since the DDR frequency will be stepped down to a very
*      low value providing banwidth sufficient for refreshing the LCD buffer only).
*   8. Enter DDR self refresh mode.
*  ^9. Configure new timing parameters for the MC to adapt the new low frequency.
* ^10. Configure the "Future Mode" and "Ext Mode"  registers (The actual register updating will occur
*      only after exiting the DDR self refresh mode).
* ^11. Configure LCD registers with new values to adapt to the new frequency.
* ^12. UNBlock data requests to the MC from the SB.
*  13. Configure the PMU to execute the DeepIdle Enter sequence.
*  14. Save CPU context on the stack
*  15. Enater the WFI, where the PMU starts executing the pre-loaded micro code.
*
* The PMU micro-code performs the following actions (^ Ò steps that can be skipped if LCD refresh is not 
  needed):
*   1. Wait the cpu to idle.
*  ^2. Switch to the 50MHz clock (assert lcd_ref_mode)
*   3. Exit DDR self refresh mode (This will trigger the holded configuration for the DIMM's mode and 
*      ext. mode registers).
*   4. Holds the CPU in reset.
*   5. Power down the CPU PLL.
*   6. Indicates to the external PM to power down the CPU power domain (assert cpu_pwrdwn)
*   7. Wait for external interrupt to start the resume.
* On receiving a non-masked interrupt, the PMU micro-code performs the following:
*   8. Wait for the power idicator.
*   9. Configure the NB clock divider and PLL with the new parameters (taken from registers).
*  10. Power up the CPU PLL.
* ^11. Wait for LCD EOS interrupt.
* ^12. Enter DDR self refresh mode.
* ^13. Switch back to the normal clock provided by the NB (deassert lcd_ref_mode).
*  14. Indicate to the external PM to power up the CPU power domain.
*  15. Unmask the FIQ and IRQ.
*  16. Release CPU from reset.
*
* After releasing the CPU from reset, the CPU starts executing from the BootROM which does the following steps 
* (^ Ò steps that can be skipped if LCD refresh is not needed):
*   1. Read the PMU status register and detect the need for resume.
*   2. perform the prepared register configurations in the scratch pad.
*   3. Jump back to the resume address (in the scratch pad).
*   4. Restore CPU context.
*  ^5. Configure LCD registers to work back with NB frequency (not 50MHz used for LCD refresh).
*  ^6. Configure MC registers to adapt to the regular NB frequency.
*  ^7. Configure Mode and Ext Mode reigister in the DIMM to work with the regular NB frequency.
*  ^8. Block all SB data requests.
*   9. Exit DDR self refresh mode (this will initiate the configuration for the Mode and Ext Mode registers in 
*      the DIMM).
* ^10. Confgiure the Mode register to enable the DRAM's DLL via a load mode register command.
* ^11. UNBlock the SB data access to the DDR.
*  12. Jump back to the resume address in the DDR.
*  13. API returns with MV_OK return code.
*
* INPUT:
*       lcdRefresh: Boolean whether to use the LCD refresh mode (EBook) or not.
* OUTPUT:
*	None
* RETURN:
*    MV_ERROR   : Failed to enter DeepIdle mode
*    MV_OK      : Resumed successfully after receiving an interrupt
*******************************************************************************/
MV_STATUS mvPmuDeepIdle (MV_BOOL lcdRefresh)
{
	MV_U32 reg, reg1;

	/* Prepare resume info in SRAM */
	if (mvPmuSramDeepIdleResumePrep() != MV_OK)
		return MV_FAIL;

	/* mask out IRQ and FIQ from being assereted to the cpu */
	/* Disable DDR SR by default */
	reg = MV_REG_READ(PMU_CTRL_REG);
	reg |= (PMU_CTRL_MASK_IRQ_MASK | PMU_CTRL_MASK_FIQ_MASK);
	reg &= ~PMU_CTRL_DDR_SLF_RFRSH_EN_MASK;
	MV_REG_WRITE(PMU_CTRL_REG, reg);

	/* Set CPU power delay in TCLK cycles */
	MV_REG_WRITE(PMU_CPU_PWR_DELAY_REG, PMU_DEEPIDLE_CPU_PWR_DLY);

	/* Assume that DDR and L2 clock ratios wil not be changed */
	reg = (MV_REG_READ(PMU_CPU_DFS_CTRL_REG) & ~(PMU_DFS_CTRL_DDR_RATIO_MASK | PMU_DFS_CTRL_L2_RATIO_MASK));
	reg1 = MV_REG_READ(PMU_CLK_DIVIDER_0_REG);
	reg |= (((reg1 & PMU_CLK_DIV_XPRATIO_MASK) >> PMU_CLK_DIV_XPRATIO_OFFS) << PMU_DFS_CTRL_L2_RATIO_OFFS); /* L2 */
	reg |= (((reg1 & PMU_CLK_DIV_DPRATIO_MASK) >> PMU_CLK_DIV_DPRATIO_OFFS) << PMU_DFS_CTRL_DDR_RATIO_OFFS); /* DDR */
	MV_REG_WRITE(PMU_CPU_DFS_CTRL_REG, reg);

	/* Jump to the SRAM and execute the DeepIdle routine */
	mvPmuSramDeepIdle((MV_U32)lcdRefresh);

	/* Reset the DEEP-IDLE bit in the PMU resume status register */
	reg = MV_REG_READ(PMU_STATUS_REG);
	reg &= ~PMU_STATUS_DEEPIDLE_MASK;
	MV_REG_WRITE(PMU_STATUS_REG, reg);

	/* Clear the DeepIdle & Standby Flags in the PMU Status register */
	/* Otherwise the BootROM will try to resume on the next boot */
	reg = MV_REG_READ(PMU_STATUS_REG);
	reg &= ~(PMU_STATUS_DEEPIDLE_MASK | PMU_STATUS_STANDBY_OFFS);
	MV_REG_WRITE(PMU_STATUS_REG, reg);
	
	/* Finally unmask IRQ and FIQ */
	reg = MV_REG_READ(PMU_CTRL_REG);
	reg &= ~(PMU_CTRL_MASK_IRQ_MASK | PMU_CTRL_MASK_FIQ_MASK);
	MV_REG_WRITE(PMU_CTRL_REG, reg);	

	return MV_OK;
}

/*******************************************************************************
* mvPmuStandby - Power down the whole SoC. This api blocks untill
*              receiving an interrupt, which wakes up the SoC and this
*              api returns to the caller.
*
* DESCRIPTION:
* The caller of this API is resonsible to do the following requirements before:
*   - Configure the Wake up events.
*   - Save the context off all Units including the address decoding windows.
*   - Disable I-Cache.
*   - Disable IRQ and FIQ at the CPSR level.
*
* The caller of this API is also resonsible to do the following requirements after:
*   - Restore the context off all Units including the address decoding windows.
*   - Re-enable the I-Cache if needed.
*   - Re-enable the IRQ and FIQ at the CPSR level.
*
* This API performs the following steps:
*   1. Mask both IRQ and FIQ interrupts at the PMU level.
*   2. Prepare in the PMU scratch pad (SRAM) with the needed code to execute from there.
*   3. Read the DDR configuration paramters and write them to the scratch pad.
*   4. Jump to the scratch pad and start executing from there.
*   5. Block all data requests to the DDR controller (from the SB).
*   6. Configure the PMU to execute the Standby Enter sequence.
*   7. Save CPU context on the stack
*   8. Enater the WFI, where the PMU starts executing the pre-loaded micro code.
*
* The PMU micro-code performs the following actions (^ Ò steps that can be skipped if LCD refresh is not 
  needed):
*   1. Wait the cpu to idle.
*   3. Enter DDR self refresh mode.
*   4. Hold the CPU in reset.
*   5. Power down the CPU PLL.
*   6. Request from the external PM to power down the CPU power domain (assert cpu_pwrdwn)
*   7. Switch the PMU clock to the RTC clock (32KHz).
*   8. Request from the external PM to power down the whole SoC (assert slp_pwddwn).
* On receiving a non-masked interrupt, the PMU micro-code performs the following:
*   9. Request from the external PM to power UP the whole SoC (assert slp_pwddwn).
*  10. Request from the external PM to power UP the CPU power domain (assert cpu_pwrdwn)
*  11. Wait for the PLL to lock.
*  12. Change the clock MUX to the regular clock (instead of the RTC 32KHx clock).
*  13. Release CPU from reset.
*
* After releasing the CPU from reset, the CPU starts executing from the BootROM which does the following steps:
*   1. Read the PMU status register and detect the need for resume.
*   2. perform the prepared register configurations in the scratch pad.
*   3. Jump back to the resume address (in the scratch pad).
*   4. Restore CPU context.
*   5. Exit DDR self refresh mode.
*   6. Release the FIQ and IRQ interrupts.
*  12. Jump back to the resume address in the DDR.
*  13. API returns with MV_OK return code.
*
* INPUT:
*	None
* OUTPUT:
*	None
* RETURN:
*    	MV_ERROR   : Failed to enter Standby mode
*   	MV_OK      : Resumed successfully after receiving an unmased Wake-up event
*******************************************************************************/
MV_STATUS mvPmuStandby (void)
{
	MV_U32 reg;
	MV_U32 pllFreq, n, m, k;
	MV_U32 ddrDiv = 1;
	MV_U32 i;

	/* Get current DDR frequency to be configured in resume */
	reg = MV_REG_READ(PMU_PLL_CTRL_0_REG);
	n = ((reg & PMU_PLL_N_DIVIDER_MASK) >> PMU_PLL_N_DIVIDER_OFFS) + 1;
	m = ((reg & PMU_PLL_M_DIVIDER_MASK) >> PMU_PLL_M_DIVIDER_OFFS) + 1;
	k = ((reg & PMU_PLL_K_DIVIDER_MASK) >> PMU_PLL_K_DIVIDER_OFFS);
	reg = MV_REG_READ(MPP_SAMPLE_AT_RESET_REG1);
	if (reg & (1 << 13)) /* if SaR45 is set, pll is overriden*/
	    pllFreq = 2000;
	else
		pllFreq = ((25 * n) / (m * k)); /* function to convert form 25Mhz to PLL frequency */
	reg = MV_REG_READ(PMU_CLK_DIVIDER_0_REG);
	reg = ((reg & PMU_CLK_DIV_DPRATIO_MASK) >> PMU_CLK_DIV_DPRATIO_OFFS);

	/* Lookup the DDR divider */
	for (i = 0; i < MAX_RATIO_MAP_CNT; i++)
	{
		if (ratio2DivMapTable[i][1] == reg)
		{
			ddrDiv = ratio2DivMapTable[i][0];
			break;
		}
	}

	if (reg == MAX_RATIO_MAP_CNT)
		return MV_FAIL;
	
	/* Prepare resume info in SRAM */
	if (mvPmuSramStandbyResumePrep(pllFreq/ddrDiv) != MV_OK)
		return MV_FAIL;

	/* Mast out the M_RESETn for DDR3 */

	/* mask out IRQ and FIQ from being assereted to the cpu */
	reg = MV_REG_READ(PMU_CTRL_REG);
	reg |= (PMU_CTRL_MASK_IRQ_MASK | PMU_CTRL_MASK_FIQ_MASK);
	MV_REG_WRITE(PMU_CTRL_REG, reg);

	/* Set CPU Power delay in RTC clock cyclec */
	MV_REG_WRITE(PMU_CPU_PWR_DELAY_REG, PMU_STBY_CPU_PWR_DLY);	

	/* Jump to thsd SRAM and execute the Standby routine */
	mvPmuSramStandby();

	/* Reset the STANDBY bit in the PMU resume status register */
	reg = MV_REG_READ(PMU_STATUS_REG);
	reg &= ~PMU_STATUS_STANDBY_MASK;
	MV_REG_WRITE(PMU_STATUS_REG, reg);

	/* Clear the DeepIdle & Standby Flags in the PMU Status register */
	/* Otherwise the BootROM will try to resume on the next boot */
	reg = MV_REG_READ(PMU_STATUS_REG);
	reg &= ~(PMU_STATUS_DEEPIDLE_MASK | PMU_STATUS_STANDBY_OFFS);
	MV_REG_WRITE(PMU_STATUS_REG, reg);

	/* Finally unmask IRQ and FIQ */
	reg = MV_REG_READ(PMU_CTRL_REG);
	reg &= ~(PMU_CTRL_MASK_IRQ_MASK | PMU_CTRL_MASK_FIQ_MASK);
	MV_REG_WRITE(PMU_CTRL_REG, reg);

	/* Unmask the M_RESETn for DDR3 */

	/* Restore all uCode Fixes */
	mvPmuCodeFix();

	/* Restore PMU Control on CPU Reset */
	reg = MV_REG_READ(CPU_CONTROL_REG);
	reg |= CPU_CTRL_PMU_CPU_RST_EN_MASK;
	MV_REG_WRITE(CPU_CONTROL_REG, reg);	

	return MV_OK;
}

/*******************************************************************************
* mvPmuWakeupEventSet - Set the Events that can wak up the system from SLEEP.
*
* DESCRIPTION:
*   Bitmap of events that can cause the system to resume after SLEEP
*
* INPUT:
*       Events that should be unmasked (one or more)
* OUTPUT:
*	None
* RETURN:
*    MV_OK      : Wakeup events mask set successfully
*******************************************************************************/
MV_STATUS mvPmuWakeupEventSet (MV_U32 wkupEvents)
{
	MV_U32 reg;

	/* Read and modify the Wakeup events */
	reg = MV_REG_READ(PMU_STBY_WKUP_EVENT_CTRL_REG);
	reg &= ~(PMU_STBY_WKUP_CTRL_ALL_EV_MASK);
	reg |= (wkupEvents & PMU_STBY_WKUP_CTRL_ALL_EV_MASK);
	MV_REG_WRITE(PMU_STBY_WKUP_EVENT_CTRL_REG, reg);
	return MV_OK;
}

/*******************************************************************************
* mvPmuWakeupEventGet - Get the Events that can wak up the system from SLEEP
*
* DESCRIPTION:
*   Bitmap of events that can cause the system to resume after SLEEP
*
* INPUT:
*	None
* OUTPUT:
*       Events that were unmasked (one or more)
* RETURN:
*    MV_OK      : Wakeup events got successfully
*******************************************************************************/
MV_STATUS mvPmuWakeupEventGet (MV_U32 * wkupEvents)
{
	*wkupEvents = (MV_REG_READ(PMU_STBY_WKUP_EVENT_CTRL_REG) & PMU_STBY_WKUP_CTRL_ALL_EV_MASK);
	return MV_OK;
}

/*******************************************************************************
* mvPmuWakeupEventStatusGet - Get the event/s caused the system to wakeup from SLEEP
*
* DESCRIPTION:
*   Bitmap of events that caused the system to resume after SLEEP.
*
* INPUT:
*	None
* OUTPUT:
*       Events that were unmasked (one or more)
* RETURN:
*    MV_OK      : Wakeup event status read successfully
*******************************************************************************/
MV_STATUS mvPmuWakeupEventStatusGet (MV_U32 * wkupEvents)
{
	*wkupEvents = (MV_REG_READ(PMU_STBY_WKUP_EVENT_STAT_REG) & PMU_STBY_WKUP_CTRL_ALL_EV_MASK);
	return MV_OK;
}

/*******************************************************************************
* mvPmuCpuFreqScale - Change the frequency of the CPU
*
* DESCRIPTION:
*   Change the CPU frequency dynamically at runtime. The following is the sequence used:
*   1. MASK FIQ and IRQ interrupts at the PMU level.
*   2. Configure the new CPU speed (Turbo/Slow).
*   3. Configure the PMU registers to trigger DFS
*   4. Halt CPU through callig WFI.
*   5. When the PMU finishes the frequency update, an interrupt is generated that wakes
*      up the cpu in the new frequency.
*   6. Check and clean the DFSDone interrupt cause.
*
* Requirements:
*   1. mvPmu Init should be called (to unmask the PMU interrupt in Main Cause register)
*   2. Before calling this routine, the following should be done:
*      A. MASK IRQ and FIQ interrupts at the CPSR level
*      B. UNMASK the PMU interrupt in the Main cause register
*   3. After exiting this routine, the caller should do the following:
*      A. UNMASK IRQ and FIQ interrupts at the CPSR level
*      B. Mask the PMU interrupt if it was originally masked.
*
* INPUT:
*       Cpu clock speed.
* OUTPUT:
*       None.
* RETURN:
*    MV_ERROR   : Failed to change the CPU frequency
*    MV_OK      : CPU frequency scalled successfully
*******************************************************************************/
MV_STATUS mvPmuCpuFreqScale (MV_PMU_CPU_SPEED cpuSpeed)
{
	MV_U32 reg, reg1;
	
	/* First verify that DFS is really needed */
	reg = MV_REG_READ(PMU_DFS_STATUS_REG);
	reg = ((reg & PMU_DFS_STAT_SLOW_MASK) >> PMU_DFS_STAT_SLOW_OFFS);
	if (reg == (MV_U32)cpuSpeed)
		return MV_OK;

	/* Unmask the DFS done interrupt to wakeup the cpu */
	reg = MV_REG_READ(PMU_INT_MASK_REG);
	reg |= PMU_INT_DFS_DONE_MASK;
	MV_REG_WRITE(PMU_INT_MASK_REG, reg);	

	/* mask out IRQ and FIQ from being assereted to the cpu */
	reg = MV_REG_READ(PMU_CTRL_REG);
	reg |= (PMU_CTRL_MASK_IRQ_MASK | PMU_CTRL_MASK_FIQ_MASK);
	MV_REG_WRITE(PMU_CTRL_REG, reg); 

	/* Read the target ratio which should be the DDR ratio */
	reg1 = MV_REG_READ(PMU_CLK_DIVIDER_0_REG);

	/* Configure new CPU speed and DFS request */
	reg = MV_REG_READ(PMU_CPU_DFS_CTRL_REG);
	reg &= ~PMU_DFS_CTRL_L2_RATIO_MASK;
	if (cpuSpeed == CPU_CLOCK_TURBO)
	{
		reg &= ~PMU_DFS_CTRL_SLOW_SPD_MASK;
		reg |= l2TurboRatio;
	}
	else
	{
		reg |= PMU_DFS_CTRL_SLOW_SPD_MASK;	
		reg |= (((reg1 & PMU_CLK_DIV_DPRATIO_MASK) >> PMU_CLK_DIV_DPRATIO_OFFS) << PMU_DFS_CTRL_L2_RATIO_OFFS);
	}
	reg |= PMU_DFS_CTRL_DFS_EN_MASK;	
	MV_REG_WRITE(PMU_CPU_DFS_CTRL_REG, reg);

	/* Start DFS by entering WFI */
	// __asm__ ("mcr  p15, 0, r0, c7, c0, 4");

	mvPmuSramCpuDfs();

	/* 
	 * NOTE:
	 * At this level the PMU have unmasked the IRQ and FIQ bits in the 
	 * PMU_CONTROL_REGISTER.
	 */

	/* Check and clear the DFSDone interrupt */
	reg = MV_REG_READ(PMU_INT_CAUSE_REG);
	if ((reg & PMU_INT_DFS_DONE_MASK) == 0)
		return MV_FAIL;
	reg &= ~PMU_INT_DFS_DONE_MASK;
	MV_REG_WRITE(PMU_INT_CAUSE_REG, reg);
	
	/* Mask the DFS done interrupt */
	reg = MV_REG_READ(PMU_INT_MASK_REG);
	reg &= ~PMU_INT_DFS_DONE_MASK;
	MV_REG_WRITE(PMU_INT_MASK_REG, reg);

	/* Verify the new CPU speed */
	reg = MV_REG_READ(PMU_DFS_STATUS_REG);
	reg = ((reg & PMU_DFS_STAT_SLOW_MASK) >> PMU_DFS_STAT_SLOW_OFFS);
	if (reg != (MV_U32)cpuSpeed)
		return MV_FAIL;

	return MV_OK;
}

/*******************************************************************************
* mvPmuSysFreqScale - Change the frequency of the DDR only
*
* DESCRIPTION:
*   Change the frequency of DDR without changing the PLL frequency. The following 
*   is the sequence used:
*   1. MASK FIQ and IRQ interrupts at the PMU level.
*   2. Based on the target frequency, calculate the values for the MC and clock dividers
*      and copy them in the PMU scratch pad.
*   3. Copy the necessary code to the scratch pad to execut from there while changing 
*      the DDR frequency.
*   4. Jump and start executing from the scratch pad.
*   5. Block all data accesses to the MC from the SB.
*   6. Issue a load Mode Register command to disable/enable the DRAM DLL if needed.
*   7. Enter DDR self refresh mode.
*   8. Reconfigure the MC registers with the new timing appropriate to the new frequency, and
*      issue a load Mode and Extended Mode register configurations to update the DIMM with the
*      values suitable for the new frequency. Since the DDR in self refresh mode, these cycles
*      are not executed, and will be held untill execiting the self refresh mode.
*   9. Configure the PMU to initiate a complete DFS change.
*  10. Halt CPU through issuing a WFI instruction.
* At this stage the PMU takes controle and performs the following:
*  11. Wait for the cpu to idle.
*  12. Update the clock dividers (CPU, L2, DRAM) with the new values from the registers.
*  13. Unmask the FIQ and IRQ at the PMU level.
*  14. generate and interrupt that wakes up the CPU.
* At this stage the CPU takes back control and continues the flow as follows:
*  15. Exit DDR self refresh mode. Automaticall the Mode and Extended Mode registers will be
*      updated (from step 8).
*  16. UNBLOCK the SB data access to the MC.
*  17. Jump back to execute from DDR.
*
*  The caller is resonsible to do the following before calling this API:
*  1. Diable FIQ and IRQ interrupts at the CPSR level.
*  2. Flush and disable all caches.
*
* INPUT:
*       DDR target frequency
* OUTPUT:
*       None.
* RETURN:
*    MV_ERROR   : Failed to scale to the new frequency
*    MV_OK      : System frequency scalled successfully
*******************************************************************************/
MV_STATUS mvPmuSysFreqScale (MV_U32 ddrFreq, MV_U32 l2Freq, MV_U32 cpuFreq)
{
	MV_U32 reg, pllFreq, ddrDiv, l2Div, ddrParamCnt;
	MV_STATUS ret = MV_OK;

	/* Get current PLL frequency */
	pllFreq = mvPmuGetPllFreq();

	/* Check Sanity that all frequencies can be derived from current PLL */
	if (((pllFreq % ddrFreq) != 0) || ((pllFreq % l2Freq) != 0) || ((pllFreq % cpuFreq) != 0))
		return MV_FAIL;
	
	/* Calculate new DDR timing parameters */
	if (mvPmuSramDdrTimingPrep(ddrFreq, cpuFreq, &ddrParamCnt) != MV_OK)
		return MV_FAIL;

	/* Get divider value based on the PLL frequency */
	if((ddrDiv = mvPmuDivider2Ratio(pllFreq/ddrFreq)) == 0)
		return MV_FAIL;
	if((l2Div = mvPmuDivider2Ratio(pllFreq/l2Freq)) == 0)
		return MV_FAIL;

	/* Configure the new DDR and L2 ratios and CPU speed */
	reg = MV_REG_READ(PMU_CPU_DFS_CTRL_REG);
	reg &= ~(PMU_DFS_CTRL_DDR_RATIO_MASK | PMU_DFS_CTRL_L2_RATIO_MASK);
	reg |= (ddrDiv << PMU_DFS_CTRL_DDR_RATIO_OFFS);
	reg |= (l2Div << PMU_DFS_CTRL_L2_RATIO_OFFS);
	if(cpuFreq == (pllFreq/2)) /* CPU TURBO speed is always 1/2 of the PLL */
		reg &= ~PMU_DFS_CTRL_SLOW_SPD_MASK;
	else /* CPU speed is DDR speed */
		reg |= PMU_DFS_CTRL_SLOW_SPD_MASK;
	reg |= PMU_DFS_CTRL_DFS_EN_MASK; 	
	MV_REG_WRITE(PMU_CPU_DFS_CTRL_REG, reg);

	/* mask out IRQ and FIQ from being assereted to the cpu */
	reg = MV_REG_READ(PMU_CTRL_REG);
	reg |= (PMU_CTRL_MASK_IRQ_MASK | PMU_CTRL_MASK_FIQ_MASK);
	MV_REG_WRITE(PMU_CTRL_REG, reg);

	/* Unmask the DFS done interrupt to wakeup the cpu */
	reg = MV_REG_READ(PMU_INT_MASK_REG);
	reg |= PMU_INT_DFS_DONE_MASK;
	MV_REG_WRITE(PMU_INT_MASK_REG, reg);

	/* Jump to the SRAM and execute the DDR dfs routine */
	mvPmuSramDdrReconfig(ddrParamCnt);

	/* Check and clear the DFSDone interrupt */
	reg = MV_REG_READ(PMU_INT_CAUSE_REG);
	if ((reg & PMU_INT_DFS_DONE_MASK) == 0)
		ret = MV_FAIL;
	reg &= ~PMU_INT_DFS_DONE_MASK;
	MV_REG_WRITE(PMU_INT_CAUSE_REG, reg);
	
	/* Mask the DFS done interrupt */
	reg = MV_REG_READ(PMU_INT_MASK_REG);
	reg &= ~PMU_INT_DFS_DONE_MASK;
	MV_REG_WRITE(PMU_INT_MASK_REG, reg);

	/* Verify the new CPU speed */
	reg = MV_REG_READ(PMU_DFS_STATUS_REG);
	reg = ((reg & PMU_DFS_STAT_DDR_RATIO_MASK) >> PMU_DFS_STAT_DDR_RATIO_OFFS);
	if (reg != ddrDiv)
		ret = MV_FAIL;

	/* IRQ and FIQ are unmasked by the PMU */
#if 0
	reg = MV_REG_READ(PMU_CTRL_REG);
	reg &= ~(PMU_CTRL_MASK_IRQ_MASK | PMU_CTRL_MASK_FIQ_MASK);
	MV_REG_WRITE(PMU_CTRL_REG, reg);
#endif

	return ret;
}

/*******************************************************************************
* mvPmuDvs - Dynamic Voltage Scale
*
* DESCRIPTION:
*   Change the voltage supplied to the CPU power domain using the external PMIC. 
*   This call is blocking (busy wait) waiting for completion.
*   The following is the sequence used:
*   1. Set the new PMIC parameters in the "CPU DVS Control" register.
*   2. Initiate the voltage change by setting the "DVSEn" bit.
*   3. Wait for the change to occur by monitoring the 
*   4. Halt CPU through callig WFI.
*   5. When the PMU finishes the frequency update, an interrupt is generated that wakes
*      up the cpu in the new frequency.
*   6. UNMASK FIQ and IRQ interrupts.
*
* INPUT:
*       pSet: PMIC Vout percentage
*       vSet: PMIC Vout level
*       rAddr: PMIC register offset
*       sAddr: PMIC Slave address
* OUTPUT:
*       None.
* RETURN:
*    MV_TIMEOUT : Timed out waiting for DVS completion
*    MV_OK      : Voltage scalled successfully
*******************************************************************************/	
MV_STATUS mvPmuDvs (MV_U32 pSet, MV_U32 vSet, MV_U32 rAddr, MV_U32 sAddr)
{
	MV_U32 reg;
	MV_U32 i;

	/* Clean the PMU cause register */
	MV_REG_WRITE(PMU_INT_CAUSE_REG, 0x0);

	/* Trigger the DVS */
	reg = ((sAddr << PMU_DVS_CTRL_PMIC_SADDR_OFFS) & PMU_DVS_CTRL_PMIC_SADDR_MASK);
	reg |= ((rAddr << PMU_DVS_CTRL_PMIC_RADDR_OFFS) & PMU_DVS_CTRL_PMIC_RADDR_MASK);
	reg |= ((vSet << PMU_DVS_CTRL_PMIC_VSET_OFFS) & PMU_DVS_CTRL_PMIC_VSET_MASK);
	reg |= ((pSet << PMU_DVS_CTRL_PMIC_PSET_OFFS) & PMU_DVS_CTRL_PMIC_PSET_MASK);
	MV_REG_WRITE(PMU_CPU_DVS_CTRL_REG, reg);
	reg |= PMU_DVS_CTRL_DVS_EN_MASK;
	MV_REG_WRITE(PMU_CPU_DVS_CTRL_REG, reg);

	/* Disable all PMU interrupts and enable DVS done */
	reg = MV_REG_READ(PMU_INT_MASK_REG);
	MV_REG_WRITE(PMU_INT_MASK_REG, PMU_INT_DVS_DONE_MASK);
	for (i=0; i<PMU_DVS_POLL_DLY; i++)
		if ((MV_REG_READ(0x20210) & 0x2) == 0x0)
			break;
	
	/* return the original PMU MASK */
	MV_REG_WRITE(PMU_INT_MASK_REG, reg);

	if (i == PMU_DVS_POLL_DLY)	
		return MV_TIMEOUT;

	return MV_OK;
}

/*******************************************************************************
* mvPmuClockGateSet - Set the clock gating status
*
* DESCRIPTION:
*   Set the clock gating status of one or more units in the SoC
*
* INPUT:
*       unitMask: Maks of unit/s to be gated/ungated
*       gateStatus: gate/ungate (TRUE/FALSE) clock on specified units.
* OUTPUT:
*       None.
* RETURN:
*    MV_OK      : Request performed successfully
*******************************************************************************/	
MV_STATUS mvPmuClockGateSet (MV_U32 unitMask, MV_BOOL gateStatus)
{
	MV_U32 reg;

	reg = MV_REG_READ(PMU_CLK_GATING_CTRL_REG);
	reg &= unitMask;
	reg |= gateStatus;
	MV_REG_WRITE(PMU_CLK_GATING_CTRL_REG, reg);

	return MV_OK;
}

/*******************************************************************************
* mvPmuClockGateGet - Get the clock gating status
*
* DESCRIPTION:
*   Get the clock gating status of one or more units in the SoC
*
* INPUT:
*       None
* OUTPUT:
*       gateStatus: gate/ungate (TRUE/FALSE) clock on specified units
* RETURN:
*    MV_OK      : Request performed successfully
*******************************************************************************/	
MV_STATUS mvPmuClockGateGet (MV_BOOL * gateStatus)
{
	*gateStatus = MV_REG_READ(PMU_CLK_GATING_CTRL_REG);

	return MV_OK;
}

/*******************************************************************************
* mvPmuTempThresholdSet - Set the upper and lower temperature thresholds, and enable/disable
*                         the thermal Manager.
*
* DESCRIPTION:
*   Set the upper (overheat) and lower (cool) temperature thresholds.
*
* INPUT:
*       coolThr: lower threshold to declare normal state (0-511)
*       overheatThr: higher threshold to declare and overheat state (0-511)
*       thrMngrEn: enable/disable (TRUE/FALSE) thermal manager.
* OUTPUT:
*       None.
* RETURN:
*    MV_OK      : Request performed successfully
*******************************************************************************/	
MV_STATUS mvPmuTempThresholdSet (MV_U32 coolThr, MV_U32 overheatThr, MV_BOOL thrMngrEn)
{
	MV_U32 reg;

	reg = MV_REG_READ(PMU_THERMAL_MNGR_REG);
	reg &= ~(PMU_TM_DISABLE_MASK | PMU_TM_OVRHEAT_THRSH_MASK | PMU_TM_COOL_THRSH_MASK);
	reg |= ((overheatThr << PMU_TM_OVRHEAT_THRSH_OFFS) & PMU_TM_OVRHEAT_THRSH_MASK);
	reg |= ((coolThr << PMU_TM_COOL_THRSH_OFFS) & PMU_TM_COOL_THRSH_MASK);
	if (thrMngrEn == MV_FALSE)
		reg |=  PMU_TM_DISABLE_MASK;
	MV_REG_WRITE(PMU_THERMAL_MNGR_REG, reg);

	return MV_OK;
}

/*******************************************************************************
* mvPmuTempThresholdGet - Get the upper and lower temperature thresholds, and status 
*                         (enable/disable) of the thermal Manager.
*
* DESCRIPTION:
*   Get the upper (overheat) and lower (cool) temperature thresholds and status
*
* INPUT:
*	None
* OUTPUT:
*       coolThr: lower threshold to declare normal state.
*       overheatThr: higher threshold to declare and overheat state.
*       thrMngrEn: enable/disable (TRUE/FALSE) thermal manager.
* RETURN:
*    MV_OK      : Request performed successfully
*******************************************************************************/	
MV_STATUS mvPmuTempThresholdGet (MV_U32 *coolThr, MV_U32 *overheatThr, MV_BOOL *thrMngrEn)
{
	MV_U32 reg;

	reg = MV_REG_READ(PMU_THERMAL_MNGR_REG);
	*coolThr = ((reg & PMU_TM_COOL_THRSH_MASK) >> PMU_TM_COOL_THRSH_OFFS);
	*overheatThr = ((reg & PMU_TM_OVRHEAT_THRSH_MASK) >> PMU_TM_OVRHEAT_THRSH_OFFS);
	if (reg & PMU_TM_DISABLE_MASK)
		*thrMngrEn = MV_FALSE;
	else
		*thrMngrEn = MV_TRUE;

	return MV_OK;
}

/*******************************************************************************
* mvPmuTempGet - Read the current temperature
*
* DESCRIPTION:
*       Read the current temperature measured by the thermal diode.
*
* INPUT:
*	None
* OUTPUT:
*       temp: Temperature value read (0-511).
* RETURN:
*    MV_NOT_READY   : Temperature reading is not yet valid
*    MV_OK          : Temperature read OK.
*******************************************************************************/
MV_STATUS mvPmuTempGet (MV_U32 *temp)
{
	if (MV_REG_READ(PMU_TEMP_DIOD_CTRL1_REG) & PMU_TDC1_TEMP_VLID_MASK)
	{
		*temp = ((MV_REG_READ(PMU_THERMAL_MNGR_REG) & PMU_TM_CURR_TEMP_MASK) >> PMU_TM_CURR_TEMP_OFFS);
		return MV_OK;
	}

	return MV_NOT_READY;
}

/*******************************************************************************
* mvPmuTempSensorCfgSet - Set the sensor configuration
*
* DESCRIPTION:
*     	Configure the sensor according to the input
*
* INPUT:
*	snsrInfo : sensor configurations
* OUTPUT:
*       None
* RETURN:
*    MV_OK          : Configuration done successfully
*******************************************************************************/
MV_STATUS mvPmuTempSensorCfgSet (MV_TEMP_SNSR_INFO snsrInfo)
{
	MV_U32 reg;

	/* Configure the overheat and cooling delays */
	MV_REG_WRITE(PMU_TM_OVRHEAT_DLY_REG, snsrInfo.overheatDelay);
	MV_REG_WRITE(PMU_TM_COOLING_DLY_REG, snsrInfo.coolingDelay);

	/* Configure the Thermal Diode Control 0 register */
	reg = 0;
	if (snsrInfo.powerDown) reg |= 	PMU_TDC0_PWR_DWN_MASK;
	reg |= ((snsrInfo.tcTrip << PMU_TDC0_TC_TRIP_OFFS) & PMU_TDC0_TC_TRIP_MASK);
	reg |= ((snsrInfo.selVcal << PMU_TDC0_SEL_VCAL_OFFS) & PMU_TDC0_SEL_VCAL_MASK);
	if (snsrInfo.vbeBypass) reg |= PMU_TDC0_VBE_BYPS_MASK;
	if (snsrInfo.selRefIcc) reg |= PMU_TDC0_SELF_RFRNC_MASK; 
	reg |= ((snsrInfo.aTest << PMU_TDC0_A_TEST_OFFS) & PMU_TDC0_A_TEST_MASK);
	reg |= ((snsrInfo.refCalCount << PMU_TDC0_REF_CAL_CNT_OFFS) & PMU_TDC0_REF_CAL_CNT_MASK);
	reg |= ((snsrInfo.selCalCapInt << PMU_TDC0_CAL_CAP_SRC_OFFS) & PMU_TDC0_CAL_CAP_SRC_MASK);
	reg |= ((snsrInfo.averageNumber	<< PMU_TDC0_AVG_NUM_OFFS) & PMU_TDC0_AVG_NUM_MASK);
	if (snsrInfo.selDblSlope) reg |= PMU_TDC0_DBL_SLOP_MASK;
	if (snsrInfo.otfCal) reg |= PMU_TDC0_OTF_CAL_MASK;
	if (snsrInfo.sleepEnable) reg |= PMU_TDC0_SLEEP_EN_MASK;
	MV_REG_WRITE(PMU_TEMP_DIOD_CTRL0_REG, reg);

	/* Configure the Thermale Diode Control 1 register */
	reg = MV_REG_READ(PMU_TEMP_DIOD_CTRL1_REG);
	reg &= ~(PMU_TDC1_CAL_STRT_VAL_MASK);
	reg |= ((snsrInfo.initCalibVal <<  PMU_TDC1_CAL_STRT_VAL_OFFS) & PMU_TDC1_CAL_STRT_VAL_MASK);
	MV_REG_WRITE(PMU_TEMP_DIOD_CTRL1_REG, reg);

	return MV_OK;
}

/*******************************************************************************
* mvPmuTempSensorCalib - Trigger temperature calibration process
*
* DESCRIPTION:
*       Start the calibration process and wait for it to finish
*
* INPUT:
*	None
* OUTPUT:
*       None
* RETURN:
*    	MV_TIMEOUT : Timeouted waiting for calibration to finish
*    	MV_OK      : Calibration started and ended successfully
*******************************************************************************/
MV_STATUS mvPmuTempSensorCalib (void)
{
	MV_U32 reg;
	MV_U32 timeout;

	/* Flip the state of the calibration start bit */
	reg = MV_REG_READ(PMU_TEMP_DIOD_CTRL1_REG);
	MV_REG_WRITE (PMU_TEMP_DIOD_CTRL1_REG, (reg ^ PMU_TDC1_STRT_CAL_MASK));
	
	/* Wait for the end of calibration indication */
	for (timeout=0; timeout < 100000; timeout++)
	{
		if (MV_REG_READ(PMU_TEMP_DIOD_CTRL1_REG) & PMU_TDC1_CAL_STAT_MASK)
			return MV_OK;
	}
	return MV_TIMEOUT;
}

/*******************************************************************************
* mvPmuGpuPowerDown - Power Down/Up the Graphical Unit
*
* DESCRIPTION:
*       Power down/up the graphical unit
*
* INPUT:
*	pwrStat: power down status
* OUTPUT:
*       None
* RETURN:
*    	MV_OK      : Calibration started and ended successfully
*******************************************************************************/
MV_STATUS mvPmuGpuPowerDown(MV_BOOL pwrStat)
{
	MV_U32 reg;

	if (pwrStat)
	{
		/* Isolate unit */
		reg = MV_REG_READ(PMU_ISO_CTRL_REG);
		reg &= ~PMU_ISO_GPU_MASK;
		MV_REG_WRITE(PMU_ISO_CTRL_REG, reg);

		/* Reset unit */
		reg = MV_REG_READ(PMU_SW_RST_CTRL_REG);
		reg &= ~PMU_SW_RST_GPU_MASK;
		MV_REG_WRITE(PMU_SW_RST_CTRL_REG, reg);

		/* Power down the unit */
		reg = MV_REG_READ(PMU_PWR_SUPLY_CTRL_REG);
		reg |= PMU_PWR_GPU_PWR_DWN_MASK;
		MV_REG_WRITE(PMU_PWR_SUPLY_CTRL_REG, reg);
	}
	else
	{
		/* Power up the unit */
		reg = MV_REG_READ(PMU_PWR_SUPLY_CTRL_REG);
		reg &= ~PMU_PWR_GPU_PWR_DWN_MASK;
		MV_REG_WRITE(PMU_PWR_SUPLY_CTRL_REG, reg);
	
		/* Add delay to wait for Power Up */

		/* Deassert unit Reset */
		reg = MV_REG_READ(PMU_SW_RST_CTRL_REG);
		reg |= PMU_SW_RST_GPU_MASK;
		MV_REG_WRITE(PMU_SW_RST_CTRL_REG, reg);

		/* Deassert unit Isolation*/
		reg = MV_REG_READ(PMU_ISO_CTRL_REG);
		reg |= PMU_ISO_GPU_MASK;
		MV_REG_WRITE(PMU_ISO_CTRL_REG, reg);
	}
	
	return MV_OK;
}

/*******************************************************************************
* mvPmuVpuPowerDown - Power Down/Up the Video Unit
*
* DESCRIPTION:
*       Power down/up the Video Processing unit
*
* INPUT:
*	pwrStat: power down status
* OUTPUT:
*       None
* RETURN:
*    	MV_OK      : Calibration started and ended successfully
*******************************************************************************/
MV_STATUS mvPmuVpuPowerDown(MV_BOOL pwrStat)
{
	MV_U32 reg;

	if (pwrStat)
	{
		/* Isolate unit */
		reg = MV_REG_READ(PMU_ISO_CTRL_REG);
		reg &= ~PMU_ISO_VIDEO_MASK;
		MV_REG_WRITE(PMU_ISO_CTRL_REG, reg);

		/* Reset unit */
		reg = MV_REG_READ(PMU_SW_RST_CTRL_REG);
		reg &= ~PMU_SW_RST_VIDEO_MASK;
		MV_REG_WRITE(PMU_SW_RST_CTRL_REG, reg);

		/* Power down the unit */
		reg = MV_REG_READ(PMU_PWR_SUPLY_CTRL_REG);
		reg |= PMU_PWR_VPU_PWR_DWN_MASK;
		MV_REG_WRITE(PMU_PWR_SUPLY_CTRL_REG, reg);
	}
	else
	{
		/* Power up the unit */
		reg = MV_REG_READ(PMU_PWR_SUPLY_CTRL_REG);
		reg &= ~PMU_PWR_VPU_PWR_DWN_MASK;
		MV_REG_WRITE(PMU_PWR_SUPLY_CTRL_REG, reg);
	
		/* Add delay to wait for Power Up */

		/* Deassert unit Reset */
		reg = MV_REG_READ(PMU_SW_RST_CTRL_REG);
		reg |= PMU_SW_RST_VIDEO_MASK;
		MV_REG_WRITE(PMU_SW_RST_CTRL_REG, reg);

		/* Deassert unit Isolation*/
		reg = MV_REG_READ(PMU_ISO_CTRL_REG);
		reg |= PMU_ISO_VIDEO_MASK;
		MV_REG_WRITE(PMU_ISO_CTRL_REG, reg);
	}

	return MV_OK;
}

/*******************************************************************************
* mvPmuGetPllFreq - Detect the current PLL frequency
*
* DESCRIPTION:
*       Detect the current PLL frequency
*
* INPUT:
*	None
* OUTPUT:
*       None
* RETURN:
*    	PLL frequency
*******************************************************************************/	
static MV_U32 mvPmuGetPllFreq(void)
{
	MV_U32 n, m, k, reg;
	
	reg = MV_REG_READ(MPP_SAMPLE_AT_RESET_REG1);
	if (reg & (1 << 13)) /* if SaR45 is set, pll is overriden*/
	    return 2000;
	/* Get current DDR frequency to be configured in resume */
	reg = MV_REG_READ(PMU_PLL_CTRL_0_REG);
	n = ((reg & PMU_PLL_N_DIVIDER_MASK) >> PMU_PLL_N_DIVIDER_OFFS) + 1;
	m = ((reg & PMU_PLL_M_DIVIDER_MASK) >> PMU_PLL_M_DIVIDER_OFFS) + 1;
	k = ((reg & PMU_PLL_K_DIVIDER_MASK) >> PMU_PLL_K_DIVIDER_OFFS);
	
	return ((25 * n) / (m * k)); /* function to convert form 25Mhz to PLL frequency */
}

/*******************************************************************************
* mvPmuRatio2Divider - Convert frequency ratio to divider value
*
* DESCRIPTION:
*       Convert frequency ratio to divider value
*
* INPUT:
*	Ratio
* OUTPUT:
*       None
* RETURN:
*    	Divider value
*******************************************************************************/
static MV_U32 mvPmuRatio2Divider(MV_U32 ratio)
{
	MV_U32 i;

	for (i = 0; i < MAX_RATIO_MAP_CNT; i++)
	{
		if (ratio2DivMapTable[i][1] == ratio)
			return ratio2DivMapTable[i][0];
	}

	return 0;
}

/*******************************************************************************
* mvPmuRatio2Divider - Convert divider value to ratio
*
* DESCRIPTION:
*       Convert divider value to ratio
*
* INPUT:
*	divider
* OUTPUT:
*       None
* RETURN:
*    	Ratio value
*******************************************************************************/
static MV_U32 mvPmuDivider2Ratio(MV_U32 div)
{
	MV_U32 i;

	for (i = 0; i < MAX_RATIO_MAP_CNT; i++)
	{
		if (ratio2DivMapTable[i][0] == div)
			return ratio2DivMapTable[i][1];
	}

	return 0;
}

/*******************************************************************************
* mvPmuVpuPowerDown - Power Down/Up the Video Unit
*
* DESCRIPTION:
*       Power down/up the Video Processing unit
*
* INPUT:
*	pwrStat: power down status
* OUTPUT:
*       None
* RETURN:
*    	MV_OK      : Calibration started and ended successfully
*******************************************************************************/
MV_STATUS mvPmuGetCurrentFreq(MV_PMU_FREQ_INFO * freqs)
{
	MV_U32 reg;
	MV_U32 pllFreq;
	MV_U32 div, ratio;
	
	/* Get current DDR frequency to be configured in resume */
	pllFreq = mvPmuGetPllFreq();
	freqs->pllFreq = pllFreq;
	reg = MV_REG_READ(PMU_CLK_DIVIDER_0_REG);
	
	/* CPU frequency */
	ratio = ((reg & PMU_CLK_DIV_PPRATIO_MASK) >> PMU_CLK_DIV_PPRATIO_OFFS);
	if ((div = mvPmuRatio2Divider(ratio)) == 0)
		return MV_FAIL;
	freqs->cpuFreq = (pllFreq/div);

	/* AXI frequency */
	ratio = ((reg & PMU_CLK_DIV_BPRATIO_MASK) >> PMU_CLK_DIV_BPRATIO_OFFS);
	if ((div = mvPmuRatio2Divider(ratio)) == 0)
		return MV_FAIL;
	freqs->axiFreq = (pllFreq/div);

	/* L2 frequency */
	ratio = ((reg & PMU_CLK_DIV_XPRATIO_MASK) >> PMU_CLK_DIV_XPRATIO_OFFS);
	if ((div = mvPmuRatio2Divider(ratio)) == 0)
		return MV_FAIL;
	freqs->l2Freq = (pllFreq/div);

	/* DDR frequency */
	ratio = ((reg & PMU_CLK_DIV_DPRATIO_MASK) >> PMU_CLK_DIV_DPRATIO_OFFS);
	if ((div = mvPmuRatio2Divider(ratio)) == 0)
		return MV_FAIL;
	freqs->ddrFreq = (pllFreq/div);
	
	return MV_OK;
}

/*******************************************************************************
* mvPmuCpuSetOP - Set the CPU operating point
*
* DESCRIPTION:
*       CPU frequency TURBO->DDR: step down frequency, then step down voltage
*	CPU frequency DDR->TURBO: step up voltage, then step up frequency
*
* INPUT:
*	cpuSpeed: desired spu speed
*	dvsEnable:  enable voltage scaling with frequency
* OUTPUT:
*       None
* RETURN:
*    	MV_OK      : operating point changed successfully
*******************************************************************************/
MV_STATUS mvPmuCpuSetOP (MV_PMU_CPU_SPEED cpuSpeed, MV_BOOL dvsEnable)
{
	MV_STATUS ret = MV_OK;
	
	/* based on requested Operating Point set volatge and frequency */
	switch (cpuSpeed) {
		case CPU_CLOCK_TURBO:
			if ((dvsEnable) && (mvPmuDvs (((dvsDefaultValues >> DOVE_PSET_HI_OFFSET ) & 0xFF), ((dvsDefaultValues >> DOVE_VSET_HI_OFFSET) & 0xFF), DOVE_RADDR, DOVE_SADDR) != MV_OK))
				ret = MV_FAIL;

			if (mvPmuCpuFreqScale (CPU_CLOCK_TURBO) != MV_OK)
				ret = MV_FAIL;
			break;

		case CPU_CLOCK_SLOW:
			if (mvPmuCpuFreqScale (CPU_CLOCK_SLOW) != MV_OK)
				ret = MV_FAIL;
			
			if ((dvsEnable) && (mvPmuDvs (((dvsDefaultValues >> DOVE_PSET_LO_OFFSET) & 0xFF), ((dvsDefaultValues >> DOVE_VSET_LO_OFFSET) & 0xFF), DOVE_RADDR, DOVE_SADDR) != MV_OK))
				ret = MV_FAIL;
			break;

		default:
			return MV_FAIL;
	};

	return ret;
}

/*******************************************************************************
* mvPmuCpuIdleThresholdsSet - Set Hi and Low Thresholds	
*
* DESCRIPTION:
*       Set thresholds value in core clock cycles. If Idle time exceeds the
*       hi threshold, a HIGH interrupt is issued. If Idle time drops below
*       the low threshold, a LOW interrupt is issued.
*
* INPUT:
*	hiThreshold: Core clock cycles fixing the HIGH threshold
*       lowThreshold: Core clock cycles fixing the LOW threshold
* OUTPUT:
*       None
* RETURN:
*    	None
*******************************************************************************/
MV_VOID mvPmuCpuIdleThresholdsSet(MV_U32 hiThreshold, MV_U32 lowThreshold)
{
	MV_REG_WRITE(PMU_CPU_IDLE_HI_THRSHLD_REG, hiThreshold);
	MV_REG_WRITE(PMU_CPU_IDLE_LOW_THRSHLD_REG, lowThreshold);
}

/*******************************************************************************
* mvPmuCpuIdleTimeBaseValueSet - Set the IDLE base value
*
* DESCRIPTION:
*       Set the Time base for calculating the IDLE time.
*
* INPUT:
*       timeBase: Core clock cycles for each iteration
* OUTPUT:
*       None
* RETURN:
*    	None
*******************************************************************************/
MV_VOID mvPmuCpuIdleTimeBaseValueSet(MV_U32 timeBase)
{
	MV_REG_WRITE(PMU_CPU_IDLE_TMR_VAL_REG, timeBase);
}

/*******************************************************************************
* mvPmuCpuIdleIntMaskSet - Set the interrupt Mask
*
* DESCRIPTION:
*       Enable/Disable the Hi/low interrupts
*
* INPUT:
*       hiIntEnable: enable/disable (MV_TRUE/MV_FALSE) hi interrupt
*       lowIntEnable: enable/disable (MV_TRUE/MV_FALSE) low interrupt
* OUTPUT:
*       None
* RETURN:
*    	None
*******************************************************************************/
MV_VOID mvPmuCpuIdleIntMaskSet(MV_BOOL hiIntEnable, MV_BOOL lowIntEnable)
{
	MV_U32 reg = MV_REG_READ(PMU_IDLE_CTRL_STAT_REG);
	
	if (hiIntEnable)
		reg |= PMU_CPU_IDLE_HI_MASK_MASK;
	else
		reg &= ~PMU_CPU_IDLE_HI_MASK_MASK;

	if (lowIntEnable)
		reg |= PMU_CPU_IDLE_LOW_MASK_MASK;
	else
		reg &= ~PMU_CPU_IDLE_LOW_MASK_MASK;

	MV_REG_WRITE(PMU_IDLE_CTRL_STAT_REG, reg);
}

/*******************************************************************************
* mvPmuCpuIdleTimeGet - Get the latest Idle
*
* DESCRIPTION:
*       Get the value of the IDLE counter in the last iteration
*
* INPUT:
*       None
* OUTPUT:
*       None
* RETURN:
*    	Core clock cycles the CPU was IDLE in last iteratio
*******************************************************************************/
MV_U32 mvPmuCpuIdleTimeGet(void)
{
	return MV_REG_READ(PMU_CPU_IDLE_RESULT_REG);
}

/*******************************************************************************
* mvPmuCpuIdleIntStatGet - Get the interrupt cause status
*
* DESCRIPTION:
*       Get the interrupt status if asserted/deasserted (MV_TRUE/MV_FALSE)
*
* INPUT:
*       None
* OUTPUT:
*       hiIntStat: return the hi interrupt status
*	lowintStat: return the low interrupt status
* RETURN:
*    	None
*******************************************************************************/
MV_VOID mvPmuCpuIdleIntStatGet(MV_BOOL *hiIntStat, MV_BOOL *lowIntStat)
{
	MV_U32 reg = MV_REG_READ(PMU_IDLE_CTRL_STAT_REG);

	if (reg & PMU_CPU_IDLE_HI_STAT_MASK)
		*hiIntStat = MV_TRUE;
	else
		*hiIntStat = MV_FALSE;

	if (reg & PMU_CPU_IDLE_LOW_STAT_MASK)
		*hiIntStat = MV_TRUE;
	else
		*hiIntStat = MV_FALSE;
}

/*******************************************************************************
* mvPmuMcIdleThresholdsSet - Set Hi and Low Thresholds	
*
* DESCRIPTION:
*       Set thresholds value in core clock cycles. If Idle time exceeds the
*       hi threshold, a HIGH interrupt is issued. If Idle time drops below
*       the low threshold, a LOW interrupt is issued.
*
* INPUT:
*	hiThreshold: Core clock cycles fixing the HIGH threshold
*       lowThreshold: Core clock cycles fixing the LOW threshold
* OUTPUT:
*       None
* RETURN:
*    	None
*******************************************************************************/
MV_VOID mvPmuMcIdleThresholdsSet(MV_U32 hiThreshold, MV_U32 lowThreshold)
{
	MV_REG_WRITE(PMU_MC_IDLE_HI_THRSHLD_REG, hiThreshold);
	MV_REG_WRITE(PMU_MC_IDLE_LOW_THRSHLD_REG, lowThreshold);
}

/*******************************************************************************
* mvPmuMcIdleTimeBaseValueSet - Set the IDLE base value
*
* DESCRIPTION:
*       Set the Time base for calculating the IDLE time.
*
* INPUT:
*       timeBase: Core clock cycles for each iteration
* OUTPUT:
*       None
* RETURN:
*    	None
*******************************************************************************/
MV_VOID mvPmuMcIdleTimeBaseValueSet(MV_U32 timeBase)
{
	MV_REG_WRITE(PMU_MC_IDLE_TMR_VAL_REG, timeBase);
}

/*******************************************************************************
* mvPmuMcIdleIntMaskSet - Set the interrupt Mask
*
* DESCRIPTION:
*       Enable/Disable the Hi/low interrupts
*
* INPUT:
*       hiIntEnable: enable/disable (MV_TRUE/MV_FALSE) hi interrupt
*       lowIntEnable: enable/disable (MV_TRUE/MV_FALSE) low interrupt
* OUTPUT:
*       None
* RETURN:
*    	None	
*******************************************************************************/
MV_VOID mvPmuMcIdleIntMaskSet(MV_BOOL hiIntEnable, MV_BOOL lowIntEnable)
{
	MV_U32 reg = MV_REG_READ(PMU_IDLE_CTRL_STAT_REG);
	
	if (hiIntEnable)
		reg |= PMU_MC_IDLE_HI_MASK_MASK;
	else
		reg &= ~PMU_MC_IDLE_HI_MASK_MASK;

	if (lowIntEnable)
		reg |= PMU_MC_IDLE_LOW_MASK_MASK;
	else
		reg &= ~PMU_MC_IDLE_LOW_MASK_MASK;

	MV_REG_WRITE(PMU_IDLE_CTRL_STAT_REG, reg);
}

/*******************************************************************************
* mvPmuMcIdleTimeGet - Get the latest Idle
*
* DESCRIPTION:
*       Get the value of the IDLE counter in the last iteration
*
* INPUT:
*       None
* OUTPUT:
*       None
* RETURN:
*    	Core clock cycles the CPU was IDLE in last iteratio
*******************************************************************************/
MV_U32 mvPmuMcIdleTimeGet(void)
{
	return MV_REG_READ(PMU_MC_IDLE_RESULT_REG);
}

/*******************************************************************************
* mvPmuMcIdleIntStatGet - Get the interrupt cause status
*
* DESCRIPTION:
*       Get the interrupt status if asserted/deasserted (MV_TRUE/MV_FALSE)
*
* INPUT:
*       None
* OUTPUT:
*       hiIntStat: return the hi interrupt status
*	lowintStat: return the low interrupt status
* RETURN:
*    	None
*******************************************************************************/
MV_VOID mvPmuMcIdleIntStatGet(MV_BOOL *hiIntStat, MV_BOOL *lowIntStat)
{
	MV_U32 reg = MV_REG_READ(PMU_IDLE_CTRL_STAT_REG);

	if (reg & PMU_MC_IDLE_HI_STAT_MASK)
		*hiIntStat = MV_TRUE;
	else
		*hiIntStat = MV_FALSE;

	if (reg & PMU_MC_IDLE_LOW_STAT_MASK)
		*hiIntStat = MV_TRUE;
	else
		*hiIntStat = MV_FALSE;
}
