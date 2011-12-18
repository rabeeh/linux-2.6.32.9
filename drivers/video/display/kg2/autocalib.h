/*******************************************************************************
*                Copyright 2008, MARVELL SEMICONDUCTOR, LTD.                   *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL.                      *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
*                                                                              *
* MARVELL COMPRISES MARVELL TECHNOLOGY GROUP LTD. (MTGL) AND ITS SUBSIDIARIES, *
* MARVELL INTERNATIONAL LTD. (MIL), MARVELL TECHNOLOGY, INC. (MTI), MARVELL    *
* SEMICONDUCTOR, INC. (MSI), MARVELL ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K.  *
* (MJKK), MARVELL ISRAEL LTD. (MSIL). MARVELL INDIA PVT LTD (MIPL)             *
*******************************************************************************/

//Pll types
typedef enum AVC275X_PLL_TYPE
{
    AVC_DAPLL = 0,
    AVC_SAPLL1,
    AVC_SAPLL2

}AVC275X_PLL_TYPE;

typedef unsigned char   UINT8;
typedef unsigned int    UINT16;
typedef signed int      INT;
typedef unsigned long   UINT32;

//Function Prototypes
UINT16 StartPllAutoCalibration(AVC275X_PLL_TYPE PllType);
UINT16 StartSdramAutoCalibration(void);


/******************************************************************************/

#if 0
//README
// The order in which these functions to be called.

// Before setting any PLL calibration, set these registers
1. ConfigureAnalogMacroSettings()
// First set all the SDRAM registers and its calibration.
2. StartSdramAutoCalibration()
// Calibrate the PLL registers.
3. StartPllAutoCalibration(PllType)

#endif
