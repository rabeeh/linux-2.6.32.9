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
*******************************************************************************/
/*******************************************************************************
* mvKW_HAL_glue - 
*
* DESCRIPTION:
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/

#include "mvOs.h"
#include "mvCommon.h"
#include "ctrlEnv/mvCtrlEnvRegs.h"
#include "mvSysHwConfig.h"
#ifdef CONFIG_MV_INCLUDE_GIG_ETH
#include "eth/mvEth.h"
#endif

/*******************************************************************************
* mvCtrlEthMaxPortGet - Get Marvell controller number of etherent ports.
*
* DESCRIPTION:
*       This function returns Marvell controller number of etherent port.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       Marvell controller number of etherent port.
*
*******************************************************************************/
#if 1
MV_U32 mvCtrlEthMaxPortGet(MV_VOID)
{
     return 1;
}

/*******************************************************************************
* mvCtrlModelGet - Get Marvell controller device model (Id)
*
* DESCRIPTION:
*       This function returns 16bit describing the device model (ID) as defined
*       in PCI Device and Vendor ID configuration register offset 0x0.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       16bit desscribing Marvell controller ID 
*
*******************************************************************************/
MV_U16 mvCtrlModelGet(MV_VOID)
{
     return	MV_6781_DEV_ID;
}
MV_U8 mvCtrlRevGet(MV_VOID)
{
#ifdef CONFIG_DOVE_REV_Y0
	return 2;
#endif
}
//EXPORT_SYMBOL(mvCtrlEthMaxPortGet);
#endif


/*******************************************************************************
* mvBoardTclkGet - Get the board Tclk (Controller clock)
*
* DESCRIPTION:
*       This routine extract the controller core clock.
*       This function uses the controller counters to make identification.
*		Note: In order to avoid interference, make sure task context switch
*		and interrupts will not occure during this function operation
*
* INPUT:
*       countNum - Counter number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit clock cycles in Hertz.
*
*******************************************************************************/
MV_U32 mvBoardTclkGet(MV_VOID)
{
#if defined(TCLK_AUTO_DETECT)
	MV_U32 tmpTClkRate = 166666667;

        tmpTClkRate = MV_REG_READ(MPP_SAMPLE_AT_RESET_REG0);
        tmpTClkRate &= MSAR_TCLCK_MASK;

        switch (tmpTClkRate)
        {
        case MSAR_TCLCK_166:
                tmpTClkRate = MV_BOARD_TCLK_166MHZ;
                break;
        }
	return tmpTClkRate;
#else
	return MV_BOARD_DEFAULT_TCLK;
#endif

}
#include <asm/mach-types.h>
#ifdef CONFIG_MV_INCLUDE_GIG_ETH
/*******************************************************************************
* mvBoardPhyAddrGet - Get the phy address
*
* DESCRIPTION:
*       This routine returns the Phy address of a given ethernet port.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit describing Phy address, -1 if the port number is wrong.
*
*******************************************************************************/
MV_32 mvBoardPhyAddrGet(MV_U32 ethPortNum)
{
#if 0
	MV_U32 boardId= mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardPhyAddrGet: Board unknown.\n");
		return MV_ERROR;
	}

	return BOARD_INFO(boardId)->pBoardMacInfo[ethPortNum].boardEthSmiAddr;
#else
#if 0
	MV_U32 ret = 0;
 	if(machine_is_rd88f6281())
	     ret = 0;
	else if (machine_is_db88f6281_bp())
	     ret = 8;
	else if (machine_is_rd88f6192_nas())
	     ret = 8;
	else
	     printk("error in %s: unknown board\n", __func__);
#endif
	MV_U32 ret = 8;
//	if (machine_is_dove_db_b())
		ret = 1;
//	printk("%s: PhyAddr: %x\n", __func__, ret);
	return ret;
#endif
}

/*******************************************************************************
* mvBoardMacSpeedGet - Get the Mac speed
*
* DESCRIPTION:
*       This routine returns the Mac speed if pre define of a given ethernet port.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_BOARD_MAC_SPEED, -1 if the port number is wrong.
*
*******************************************************************************/
MV_ETH_MAC_SPEED      mvBoardMacSpeedGet(MV_U32 ethPortNum)
{
#if 0
	MV_U32 boardId= mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardMacSpeedGet: Board unknown.\n");
		return MV_ERROR;
	}

	return BOARD_INFO(boardId)->pBoardMacInfo[ethPortNum].boardMacSpeed;
#else
#if 0
 	if(machine_is_rd88f6281())
	     return BOARD_MAC_SPEED_1000M;
	else if (machine_is_db88f6281_bp())
	     return BOARD_MAC_SPEED_AUTO;
	else if (machine_is_rd88f6192_nas())
	     return BOARD_MAC_SPEED_AUTO;
	else
	     printk("error in %s: unknown board", __func__);
#endif
	return ETH_MAC_SPEED_AUTO;
#endif
}

MV_BOOL mvBoardIsPortInSgmii(MV_U32 ethPortNum)
{
     return MV_TRUE;
}

u32 overEthAddr = 0;
//u8     mvMacAddr[6] = {0, };
u8 mvMacAddr[CONFIG_MV_ETH_PORTS_NUM][6];
u16 mvMtu[CONFIG_MV_ETH_PORTS_NUM] = {0};

#endif
/*******************************************************************************
* mvBoardSpecInitGet -
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*       None.
*
* RETURN: Return MV_TRUE and parameters in case board need spesific phy init, 
* 	  otherwise return MV_FALSE. 
*
*
*******************************************************************************/

MV_BOOL mvBoardSpecInitGet(MV_U32* regOff, MV_U32* data)
{
	return MV_FALSE;
}

MV_U32 mvCpuPclkGet(MV_VOID)
{
#if defined(PCLCK_AUTO_DETECT)
     MV_U32  tmpPClkRate=0;
     MV_U32 cpuCLK[] = MV_CPU_CLCK_TBL;
     
     tmpPClkRate = MV_REG_READ(MPP_SAMPLE_AT_RESET_REG0);
     tmpPClkRate = tmpPClkRate & MSAR_CPUCLCK_MASK;
     tmpPClkRate = tmpPClkRate >> MSAR_CPUCLCK_OFFS;
     
     tmpPClkRate = cpuCLK[tmpPClkRate];
	  
     return tmpPClkRate;
#else
     return MV_DEFAULT_PCLK
#endif
}
/*******************************************************************************
 * mvCpuL2ClkGet - Get the CPU L2 (CPU bus clock)
 *
 * DESCRIPTION:
 *       This routine extract the CPU L2 clock.
 *
 * RETURN:
 *       32bit clock cycles in Hertz.
 *
 *******************************************************************************/
MV_U32  mvCpuL2ClkGet(MV_VOID)
{
#ifdef L2CLK_AUTO_DETECT
     MV_U32 L2ClkRate, tmp, pClkRate, indexL2Rtio;
     MV_U32 cpuCLK[] = MV_CPU_CLCK_TBL;
     MV_U32 L2Rtio[][2] = MV_L2_CLCK_RTIO_TBL;

     pClkRate = mvCpuPclkGet();

     tmp = MV_REG_READ(MPP_SAMPLE_AT_RESET_REG0);
     indexL2Rtio = tmp & MSAR_L2CLCK_RTIO_MASK;
     indexL2Rtio = indexL2Rtio >> MSAR_L2CLCK_RTIO_OFFS;

     L2ClkRate = ((pClkRate * L2Rtio[indexL2Rtio][1]) / L2Rtio[indexL2Rtio][0]);

     return L2ClkRate;
#else
     return MV_BOARD_DEFAULT_L2CLK;
#endif
}
/*******************************************************************************
* mvCtrlPwrClckGet - Get Power State of specific Unit
*
* DESCRIPTION:		
*
* INPUT:
*
* OUTPUT:
*
* RETURN:
******************************************************************************/
MV_BOOL	mvCtrlPwrClckGet(MV_UNIT_ID unitId, MV_U32 index)
{
	MV_U32 reg = MV_REG_READ(CLOCK_GATING_CTRL_REG);
	MV_BOOL state = MV_TRUE;

	switch (unitId)
	{
#if defined(MV_INCLUDE_USB)
	case USB_UNIT_ID:
		if ((reg & CGC_USBENCLOCK_MASK(index)) == CGC_USBENCLOCK_DIS(index))
			state = MV_FALSE;
		else 
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_GIG_ETH)
	case ETH_GIG_UNIT_ID:
		if ((reg & CGC_GEENCLOCK_MASK) == CGC_GEENCLOCK_DIS)
			state = MV_FALSE;
		else 
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_SATA)
	case SATA_UNIT_ID:
		if ((reg & CGC_SATAENCLOCK_MASK) == CGC_SATAENCLOCK_DIS)
			state = MV_FALSE;
		else 
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_PEX)
	case PEX_UNIT_ID:
		if ((reg & CGC_PEXENCLOCK_MASK(index)) == CGC_PEXENCLOCK_DIS(index))
			state = MV_FALSE;
		else 
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_SDIO)
	case SDIO_UNIT_ID:
		if ((reg & CGC_SDIOENCLOCK_MASK(index))== CGC_SDIOENCLOCK_DIS(index))
			state = MV_FALSE;
		else 
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_NAND)
	case NAND_UNIT_ID:
		if ((reg & CGC_NANDENCLOCK_MASK) == CGC_NANDENCLOCK_DIS)
			state = MV_FALSE;
		else 
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_CAMERA)
	case CAMERA_UNIT_ID:
		if ((reg & CGC_CAMENCLOCK_MASK) == CGC_CAMENCLOCK_DIS)
			state = MV_FALSE;
		else 
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_AUDIO)
	case AUDIO_UNIT_ID:
		if ((reg & CGC_ADENCLOCK_MASK(index)) == CGC_ADENCLOCK_DIS(index))
			state = MV_FALSE;
		else 
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_GPU)
	case GPU_UNIT_ID:
		if ((reg & CGC_GPUENCLOCK_MASK) == CGC_GPUENCLOCK_DIS)
			state = MV_FALSE;
		else 
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_AC97)
	case AC97_UNIT_ID:
		if ((reg & CGC_AC97ENCLOCK_MASK) == CGC_AC97ENCLOCK_DIS)
			state = MV_FALSE;
		else 
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_PDAM)
	case PDMA_UNIT_ID:
		if ((reg & CGC_PDMAENCLOCK_MASK) == CGC_PDMAENCLOCK_DIS)
			state = MV_FALSE;
		else 
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_XOR)
	case XOR_UNIT_ID:
		if ((reg & CGC_XORENCLOCK_MASK(index))== CGC_XORENCLOCK_DIS(index))
			state = MV_FALSE;
		else 
			state = MV_TRUE;
		break;
#endif
	default:
		state = MV_TRUE;
		break;
	}

	return state;	
}
#ifdef CONFIG_MV_INCLUDE_AUDIO
MV_VOID mvAudioHalInit(MV_U8 unit);
MV_STATUS mvAudioInit(MV_U8 unit)
{
    mvAudioHalInit(unit);
    
    return MV_OK;
}
#endif
#ifdef CONFIG_MV_INCLUDE_XOR
MV_VOID mvXorHalInit (MV_U32 xorChanNum);

MV_STATUS mvXorInit (MV_VOID)
{
     MV_U32         i;
#if 0
     /* Initiate XOR address decode */
     for(i = 0; i < MV_XOR_MAX_UNIT; i++)
	  mvXorInitWinsUnit(i);
#endif
     mvXorHalInit(MV_XOR_MAX_CHAN);

     return MV_OK;
}
#endif
#if 0
#include "twsi/mvTwsi.h"
MV_U8 mvBoardA2DTwsiChanNumGet(MV_U8 unit)
{
     return 0;
}

MV_U8 mvBoardA2DTwsiAddrTypeGet(MV_U8 unit)
{
	return ADDR7_BIT;
}


MV_U8 mvBoardA2DTwsiAddrGet(MV_U8 port)
{
     return 0x4A;
}
#endif
MV_U32 mvCtrlUsbMaxGet(void)
{
	return 2;
}

MV_U32 mv_crypto_base_get(void)
{
	return (DOVE_CESA_VIRT_BASE); 
}

MV_U32 mv_crypto_phys_base_get(void)
{
	return (DOVE_CESA_PHYS_BASE + 0x10000); 
}

MV_U32 mv_crypto_irq_get(void)
{
	return IRQ_DOVE_CRYPTO; 
}
/*******************************************************************************
* mvBoardUSBVbusGpioPinGet - return Vbus input GPP
*
* DESCRIPTION:
*
* INPUT:
*		int  devNo.
*
* OUTPUT:
*		None.
*
* RETURN:
*       GPIO pin number. The function return -1 for bad parameters.
*
*******************************************************************************/
MV_32 mvBoardUSBVbusGpioPinGet(int devId)
{
	if (machine_is_dove_db() || machine_is_dove_db_b())
		return 9;
	else if (machine_is_dove_rd_avng())
		return 14;
	else if(machine_is_dove_avng_v3())
		return 32;
	return N_A;
}

/*******************************************************************************
* mvBoardUSBVbusEnGpioPinGet - return Vbus Enable output GPP
*
* DESCRIPTION:
*
* INPUT:
*		int  devNo.
*
* OUTPUT:
*		None.
*
* RETURN:
*       GPIO pin number. The function return -1 for bad parameters.
*
*******************************************************************************/
MV_32 mvBoardUSBVbusEnGpioPinGet(int devId)
{
	return  N_A;
}

