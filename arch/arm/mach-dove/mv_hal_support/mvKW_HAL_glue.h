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
#include "ctrlEnv/mvCtrlEnvSpec.h"

#if 1
MV_U32 mvCtrlEthMaxPortGet(MV_VOID);
MV_U16 mvCtrlModelGet(MV_VOID);
MV_U8 mvCtrlRevGet(MV_VOID);
#endif


MV_U32 mvBoardTclkGet(MV_VOID);

#ifdef CONFIG_MV_INCLUDE_GIG_ETH
MV_32 mvBoardPhyAddrGet(MV_U32 ethPortNum);
MV_ETH_MAC_SPEED      mvBoardMacSpeedGet(MV_U32 ethPortNum);

MV_BOOL mvBoardIsPortInSgmii(MV_U32 ethPortNum);
void    mvEthInit(void);
#endif

MV_BOOL mvBoardSpecInitGet(MV_U32* regOff, MV_U32* data);

MV_U32 mvCpuPclkGet(MV_VOID);

MV_U32  mvCpuL2ClkGet(MV_VOID);
MV_BOOL	mvCtrlPwrClckGet(MV_UNIT_ID unitId, MV_U32 index);
#ifdef CONFIG_MV_INCLUDE_AUDIO
MV_STATUS mvAudioInit(MV_U8 unit);
#endif
#ifdef CONFIG_MV_INCLUDE_XOR
MV_STATUS mvXorInit (MV_VOID);
#endif
MV_U8 mvBoardA2DTwsiChanNumGet(MV_U8 unit);
MV_U8 mvBoardA2DTwsiAddrTypeGet(MV_U8 unit);
MV_U8 mvBoardA2DTwsiAddrGet(MV_U8 port);

#ifdef CONFIG_MV_INCLUDE_CESA

MV_STATUS mvCesaInit (int numOfSession, int queueDepth, char* pSramBase, void *osHandle);
#endif

MV_U32 mvCtrlUsbMaxGet(void);
MV_32 mvBoardUSBVbusGpioPinGet(int devId);
MV_32 mvBoardUSBVbusEnGpioPinGet(int devId);
MV_U32 mv_crypto_base_get(void);
MV_U32 mv_crypto_phys_base_get(void);
MV_U32 mv_crypto_irq_get(void);
