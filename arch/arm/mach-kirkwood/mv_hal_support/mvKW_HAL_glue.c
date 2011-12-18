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
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "cpu/mvCpu.h"
#ifdef CONFIG_MV_ETHERNET
#include "eth/mvEth.h"
#endif
//#include "ctrlEnv/sys/mvCpuIf.h"

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
#if 0	
	MV_U32 devId;

	devId = mvCtrlModelGet();

	switch(devId){
		case MV_6281_DEV_ID:
			return MV_6281_ETH_MAX_PORTS;
			break;
		case MV_6192_DEV_ID:
			return MV_6192_ETH_MAX_PORTS;
			break;
		case MV_6180_DEV_ID:
			return MV_6180_ETH_MAX_PORTS;
			break;		
	}
	return 0;
#else
	return 1;
#endif
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
	MV_U32 devId;
	
	devId = MV_REG_READ(CHIP_BOND_REG);
	devId &= PCKG_OPT_MASK;

	switch(devId){
		case 2:
			return	MV_6281_DEV_ID;
			break;
		case 1:
			return	MV_6192_DEV_ID;
			break;
		case 0:
			return	MV_6180_DEV_ID;
			break;
	}

	return 0;
}
MV_U8 mvCtrlRevGet(MV_VOID)
{
     return 0;
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

        tmpTClkRate = MV_REG_READ(MPP_SAMPLE_AT_RESET);
        tmpTClkRate &= MSAR_TCLCK_MASK;

        switch (tmpTClkRate)
        {
        case MSAR_TCLCK_166:
                tmpTClkRate = MV_BOARD_TCLK_166MHZ;
                break;
        case MSAR_TCLCK_200:
                tmpTClkRate = MV_BOARD_TCLK_200MHZ;
                break;
        }
	return tmpTClkRate;
#else
	if(mvCtrlModelGet()==MV_6281_DEV_ID)
	     return MV_BOARD_TCLK_200MHZ;
	else
	     return MV_BOARD_TCLK_166MHZ;
#endif

}

#include <asm/mach-types.h>
#ifdef CONFIG_MV_ETHERNET
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
	MV_U32 ret = 0;
 	if(machine_is_rd88f6281())
	     ret = 0;
	else if (machine_is_db88f6281_bp())
	     ret = 8;
	else if (machine_is_rd88f6192_nas())
	     ret = 8;
	else
	     printk("error in %s: unknown board\n", __func__);

	printk("%s: PhyAddr: %x\n", __func__, ret);
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
MV_BOARD_MAC_SPEED      mvBoardMacSpeedGet(MV_U32 ethPortNum)
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
 	if(machine_is_rd88f6281())
	     return BOARD_MAC_SPEED_1000M;
	else if (machine_is_db88f6281_bp())
	     return BOARD_MAC_SPEED_AUTO;
	else if (machine_is_rd88f6192_nas())
	     return BOARD_MAC_SPEED_AUTO;
	else
	     printk("error in %s: unknown board", __func__);

	return BOARD_MAC_SPEED_AUTO;
#endif
}

MV_U8 mvBoardIsSwitchConnected(MV_U32 port)
{
     if(machine_is_rd88f6281())
	  return 1;
     return 0;
}

MV_BOOL mvBoardIsPortInSgmii(MV_U32 ethPortNum)
{
     return MV_TRUE;
}
#if 0
	/* MV_32 linkStatusIrq, {MV_32 qdPort0, MV_32 qdPort1, MV_32 qdPort2, MV_32 qdPort3, MV_32 qdPort4}, 
		MV_32 qdCpuPort, MV_32 smiScanMode} */
	{{35, {4, 0, 1, 2, 3}, 5, 1}};
#endif
MV_32	mvBoardLinkStatusIrqGet(MV_U32 ethPortNum)
{
     return 35;
}
MV_32	mvBoardSwitchPortGet(MV_U32 ethPortNum, MV_U8 boardPortNum)
{
     int map[] = {4, 0, 1, 2, 3};

     return map[boardPortNum];
}
MV_32	mvBoardSwitchCpuPortGet(MV_U32 ethPortNum)
{
     return 5;
}

MV_32	mvBoardSmiScanModeGet(MV_U32 ethPortNum)
{
     return 1;
}

u32 overEthAddr = 0;
//u8     mvMacAddr[6] = {0, };
u8 mvMacAddr[CONFIG_MV_ETH_PORTS_NUM][6]; 
u16 mvMtu[CONFIG_MV_ETH_PORTS_NUM] = {0};



void    mvEthInit(void)
{
     MV_U32 port;

     /* Power down all existing ports */
     for(port=0; port<mvCtrlEthMaxPortGet(); port++)
     {

//	  mvEthWinInit(port);
#warning "giga unit address decoding not implemented"

     }
     mvEthHalInit();
}

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

MV_U32 mvCpu6180PclkGet(MV_VOID)
{
	MV_U32 	tmpPClkRate=0;
	MV_CPU_ARM_CLK cpu6180_ddr_l2_CLK[] = MV_CPU6180_DDR_L2_CLCK_TBL;

	tmpPClkRate = MV_REG_READ(MPP_SAMPLE_AT_RESET);
	tmpPClkRate = tmpPClkRate & MSAR_CPUCLCK_MASK;
	tmpPClkRate = tmpPClkRate >> MSAR_CPUCLCK_OFFS;
			
	tmpPClkRate = cpu6180_ddr_l2_CLK[tmpPClkRate].cpuClk;

	return tmpPClkRate;
}

MV_U32 mvCpuPclkGet(MV_VOID)
{
#if defined(PCLCK_AUTO_DETECT)
	MV_U32 	tmpPClkRate=0;
	MV_U32 cpuCLK[] = MV_CPU_CLCK_TBL;

	if(mvCtrlModelGet() == MV_6180_DEV_ID)
		return mvCpu6180PclkGet();

	tmpPClkRate = MV_REG_READ(MPP_SAMPLE_AT_RESET);
	tmpPClkRate = tmpPClkRate & MSAR_CPUCLCK_MASK;
	tmpPClkRate = tmpPClkRate >> MSAR_CPUCLCK_OFFS;
		
	tmpPClkRate = cpuCLK[tmpPClkRate];

	return tmpPClkRate;
#else
	return MV_DEFAULT_PCLK
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
MV_BOOL		mvCtrlPwrClckGet(MV_UNIT_ID unitId, MV_U32 index)
{
	MV_U32 reg = MV_REG_READ(POWER_MNG_CTRL_REG);
	MV_BOOL state = MV_TRUE;

	switch (unitId)
    {
#if defined(MV_INCLUDE_PEX)
	case PEX_UNIT_ID:
		if ((reg & PMC_PEXSTOPCLOCK_MASK) == PMC_PEXSTOPCLOCK_STOP)
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;

		break;
#endif
#if defined(MV_INCLUDE_GIG_ETH)
	case ETH_GIG_UNIT_ID:
		if ((reg & PMC_GESTOPCLOCK_MASK(index)) == PMC_GESTOPCLOCK_STOP(index))
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_SATA)
	case SATA_UNIT_ID:
		if ((reg & PMC_SATASTOPCLOCK_MASK(index)) == PMC_SATASTOPCLOCK_STOP(index))
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_CESA)
	case CESA_UNIT_ID:
		if ((reg & PMC_SESTOPCLOCK_MASK) == PMC_SESTOPCLOCK_STOP)
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_USB)
	case USB_UNIT_ID:
		if ((reg & PMC_USBSTOPCLOCK_MASK) == PMC_USBSTOPCLOCK_STOP)
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_AUDIO)
	case AUDIO_UNIT_ID:
		if ((reg & PMC_AUDIOSTOPCLOCK_MASK) == PMC_AUDIOSTOPCLOCK_STOP)
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_TS)
	case TS_UNIT_ID:
		if ((reg & PMC_TSSTOPCLOCK_MASK) == PMC_TSSTOPCLOCK_STOP)
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_SDIO)
	case SDIO_UNIT_ID:
		if ((reg & PMC_SDIOSTOPCLOCK_MASK)== PMC_SDIOSTOPCLOCK_STOP)
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_TDM)
	case TDM_UNIT_ID:
		if ((reg & PMC_TDMSTOPCLOCK_MASK) == PMC_TDMSTOPCLOCK_STOP)
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
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
#include "twsi/mvTwsi.h"
MV_U8 mvBoardA2DTwsiChanNumGet(MV_U8 unit)
{
     return 0;
}

MV_U8 mvBoardA2DTwsiAddrTypeGet(MV_U8 unit)
{
	return ADDR7_BIT;
}


MV_U8 mvBoardA2DTwsiAddrGet(MV_U8 unit)
{
     return 0x4A;
}

#ifdef CONFIG_MV_INCLUDE_CESA
#include "cesa/mvCesa.h"

MV_STATUS mvCesaHalInit (int numOfSession, int queueDepth, char* pSramBase,
			 MV_U32 cryptEngBase, void *osHandle);

MV_STATUS mvCesaInit (int numOfSession, int queueDepth, char* pSramBase, void *osHandle)
{
    MV_U32 cesaCryptEngBase;
#if 0
    MV_CPU_DEC_WIN addrDecWin;
#endif
    if(sizeof(MV_CESA_SRAM_MAP) > MV_CESA_SRAM_SIZE)
    {
        mvOsPrintf("mvCesaInit: Wrong SRAM map - %d > %d\n",
                sizeof(MV_CESA_SRAM_MAP), MV_CESA_SRAM_SIZE);
        return MV_FAIL;
    }
#if 0
    if (mvCpuIfTargetWinGet(CRYPT_ENG, &addrDecWin) == MV_OK)
        cesaCryptEngBase = addrDecWin.addrWin.baseLow;
    else
    {
        mvOsPrintf("mvCesaInit: ERR. mvCpuIfTargetWinGet failed\n");
        return MV_ERROR;
    }

#if (MV_CESA_VERSION >= 2)
    mvCesaTdmaAddrDecInit();
#endif /* MV_CESA_VERSION >= 2 */
#endif
	return mvCesaHalInit(numOfSession, queueDepth, pSramBase, cesaCryptEngBase, 
			     osHandle);

}
#endif
MV_U32 mvCtrlUsbMaxGet(void)
{
	return 1;
}
u32 mvIsUsbHost = 1;
