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

#include "kg2_i2c.h"

//Use 0x20 as device address for 88DE   2 Page addressing
#define AVC275X_BASE_ADDR               (0x20)
//Example 0x20, 0x00, 0x0F (0x20 - 88DE275X Device ID, 0x00 - default, 0x0F - Analog PLL Page ID AVC275X_PLL_PAGE_ID)

//Use 0x22 as the Sub address for addressing an Offset register within a Page
#define AVC275X_SUB_ADDR                (0x22)
//Example 0x22, 0x06, 0x01 (0x22 - 88DE275X Base Address, 0x06 - register Offset for AVC275X_SA_DAPLL_KVCO_CAL_CTRL, 0x01 - value)

//Page ID for addressing the Analog Pll Page
#define AVC275X_PLL_PAGE_ID             (0x0F)

//--------------------  ANA_CTRL MACROS ---------------------------

//SubAdress(SA) for DAPLL registers
#define AVC275X_SA_DAPLL_KVCO_CAL_CTRL  (0x06)     //ANA_CTRL6
#define AVC275X_SA_DAPLL_CTRL           (0x07)     //ANA_CTRL7
#define AVC275X_SA_DAPLL_VCO_CTRL       (0x08)     //ANA_CTRL8

//SubAdress(SA) for SAPLL1 registers
#define AVC275X_SA_SAPLL1_KVCO_CAL_CTRL  (0x0E)    //ANA_CTRL14
#define AVC275X_SA_SAPLL1_CTRL           (0x10)    //ANA_CTRL16
#define AVC275X_SA_SAPLL1_VCO_CTRL       (0x11)    //ANA_CTRL17

//SubAdress(SA) for SAPLL2 registers
#define AVC275X_SA_SAPLL2_KVCO_CAL_CTRL  (0x17)   //ANA_CTRL23
#define AVC275X_SA_SAPLL2_CTRL           (0x19)   //ANA_CTRL25
#define AVC275X_SA_SAPLL2_VCO_CTRL       (0x1A)   //ANA_CTRL26

#define AVC275X_SA_PLL_STATUS            (0x41)    //ANA_STAT0

#define AVC275X_SA_PLL_STAT14		     (0x4F)	   //ANA_STAT14

//SubAddress(SA) for XTAL setting
#define AVC275X_XTAL_AMP_SEL			 (0x1E)	   //ANA_CTRL30

//SubAddress(SA) for DAC current selection.
#define AVC275X_DAC_CURRENT_SEL			 (0x1F)    //ANA_CTRL31

// SubAddredd(SA) for XTAL control.
#define AVC275X_XTAL_CTRL				 (0x20)	   //ANA_CTRL32


//--------------------  SDRAM MACROS ---------------------------

//Page ID for addressing the Sdram Control Page
#define AVC275X_DDR_PAGE_ID              (0x04)

//SubAddress(SA) for SDRAM Request monitor
#define AVC275X_DDR_REQ_MON				 (0x48)   //DDR_REQ_MON0_0

//Page ID for addressing the Sdram Phy registers Page
#define AVC275X_DDR_PHY_PAGE_ID          (0x05)

//SubAddress(SA) for Calibration registers
#define AVC275X_DDR_CAL_TRIG			 (0xC7)   //DDR_CAL_TRIG
#define AVC275X_DDR_CALP_L				 (0xC8)   //DDR_CALP_L
#define AVC275X_DDR_CALP_H				 (0xC9)   //DDR_CALP_H
#define AVC275X_DDR_CALN_L				 (0xCA)   //DDR_CALN_L
#define AVC275X_DDR_CALN_H				 (0xCB)   //DDR_CALN_H

//SubAddress(SA) for ZPR/ZNR registers
#define AVC275X_DDR_ZPR_0				 (0x5C)   //DS_PHY_ZPR_0
#define AVC275X_DDR_ZNR_0				 (0x5D)   //DS_PHY_ZNR_0
#define AVC275X_DDR_ZPR_1				 (0x5E)   //DS_PHY_ZPR_1
#define AVC275X_DDR_ZNR_1				 (0x5F)   //DS_PHY_ZNR_1

#define AVC_SDRAM_CLNT_NUM1   			 (1)
#define AVC_SDRAM_CLNT_NUM14			 (14)

// According to windows application
#define AVC_DELAY_WEIGHTING_FACTOR		 (0.00000265)               //Delay weighting factor based on Window platform

//Pll types
typedef enum AVC275X_PLL_TYPE
{
    AVC_DAPLL = 0,
    AVC_SAPLL1,
    AVC_SAPLL2

}AVC275X_PLL_TYPE;

typedef enum
{
    FALSE = 0,
    TRUE  = 1

}BOOLOP;

typedef unsigned char   UINT8;
typedef unsigned int    UINT16;
typedef signed int      INT;
typedef unsigned long   UINT32;

//Function Prototypes
UINT16 StartPllAutoCalibration(AVC275X_PLL_TYPE PllType);
UINT16 StartSdramAutoCalibration(void);
static UINT16 ConfigureAnalogMacroSettings(void);

#if 0
// i2c functions for read and write The function should be implemented by <customer> based
// on the i2c implementation
/**************************************************************************************/
BOOLOP AVC275X_i2csend(UINT8 BaseAddr,
                       UINT8 subAddr,
                       UINT8 *data,
                       UINT32 length)
/**************************************************************************************/
{
    //To be filled by customer with appropriate I2C Send function
    return(TRUE);
}

/**************************************************************************************/
BOOLOP AVC275X_i2crecv(UINT8 BaseAddr,
                       UINT8 subAddr,
                       UINT8 *data,
                       UINT32 length)
/**************************************************************************************/
{
    //To be filled by customer with appropriate I2C Receive function
    return(TRUE);
}
#endif

#define AVC275X_i2csend(baseaddr, subaddr, data, length) kg2_i2c_write(baseaddr, subaddr, data, length)
#define AVC275X_i2crecv(baseaddr, subaddr, data, length) kg2_i2c_read(baseaddr, subaddr, data, length)

/**************************************************************************************/
void AvcDelay(UINT32 DelayTics)
/**************************************************************************************/
{
    //The delay function is windows specific to be modified as per platform.
    UINT32 Counter = 0;
    Counter = (DelayTics/(AVC_DELAY_WEIGHTING_FACTOR));
    while(Counter > 0)
    {
        Counter--;
    }
}

/*****************************************************************************
- Function Name         : StartAutoCalibration
- Function Parameters   : ClkPllType - decides the fpll type
- Function Return type  : static AVC_RETURNS
- Function Description  :

This function sets the VCO range based on input FPLL type.
******************************************************************************/
UINT16 StartPllAutoCalibration(AVC275X_PLL_TYPE PllType)
{
    UINT8 Value;
    UINT8 Cnt;

    Value = AVC275X_PLL_PAGE_ID;
    //Address the 88DE275X page, as all the registers are in the same page addressing of the page will be done only once
    AVC275X_i2csend(AVC275X_BASE_ADDR, 0x0, &Value, 1);

    if(AVC_DAPLL == PllType)
    {
        //Step 1. Power Down the Pll (DAPLL, SAPLL1, SAPLL2)
        //refer ANA_CTRL7 - [bit 0] for DAPLL,
        AVC275X_i2crecv(AVC275X_SUB_ADDR, AVC275X_SA_DAPLL_CTRL, &Value, 1);
        Value |= 0x01;
        AVC275X_i2csend(AVC275X_SUB_ADDR, AVC275X_SA_DAPLL_CTRL, &Value, 1);

        //Step 2. Power Up the Pll
        //refer ANA_CTRL7 - [bit 0] for DAPLL,
        Value = (Value & 0xFE);
        AVC275X_i2csend(AVC275X_SUB_ADDR, AVC275X_SA_DAPLL_CTRL, &Value, 1);

        //Step 3. Enable VCO Calibration control bit
        //refer ANA_CTRL8 - [bit 2] for DAPLL
        AVC275X_i2crecv(AVC275X_SUB_ADDR, AVC275X_SA_DAPLL_VCO_CTRL, &Value, 1);
        Value = (Value | 0x04);
        AVC275X_i2csend(AVC275X_SUB_ADDR, AVC275X_SA_DAPLL_VCO_CTRL, &Value, 1);

        //Step 4. Set the KVCO with value starting with 7 and check the VCO Overthreshold status
        //Test the VCO Overthreshold status as 0 (bit set as Low)
        //In case the above condition is not satified decrement the KVCO and repeat the above step untill the
        //the condition is satisfied. KVCO value can be decremented until it becomes 0.

        for(Cnt = 8;Cnt > 0;Cnt--)
        {
            //refer ANA_CTRL6 - [bit 2:0] for DAPLL_KVCO
            Value = Cnt;
            if(Value >= 1)
                Value = Value - 1;
            else
                Value = 0;

            AVC275X_i2csend(AVC275X_SUB_ADDR, AVC275X_SA_DAPLL_KVCO_CAL_CTRL, &Value, 1);

            //refer ANA_STAT0 - [bit 1] for the VCO Overthreshold indicator status
            AVC275X_i2crecv(AVC275X_SUB_ADDR, AVC275X_SA_PLL_STATUS, &Value, 1);
            if(0 == (Value & 0x02))
                break;
        }

        //Step 5. After the KVCO calibration is completed, disable the VCO Calibration control bit
        //refer ANA_CTRL8 - [bit 2] for DAPLL
        AVC275X_i2crecv(AVC275X_SUB_ADDR, AVC275X_SA_DAPLL_VCO_CTRL, &Value, 1);
        Value = (Value & 0xFB);
        AVC275X_i2csend(AVC275X_SUB_ADDR, AVC275X_SA_DAPLL_VCO_CTRL, &Value, 1);
    }
    else if(AVC_SAPLL1 == PllType)
    {
        //Step 1. Power Down the Pll (DAPLL, SAPLL1, SAPLL2)
        //refer ANA_CTRL16 - [bit 1] for SAPLL1,
        AVC275X_i2crecv(AVC275X_SUB_ADDR, AVC275X_SA_SAPLL1_CTRL, &Value, 1);
        Value |= 0x02;
        AVC275X_i2csend(AVC275X_SUB_ADDR, AVC275X_SA_SAPLL1_CTRL, &Value, 1);


        //Step 2. Power Up the Pll
        //refer ANA_CTRL16 - [bit 1] for SAPLL1,
        Value = (Value & 0xFD);
        AVC275X_i2csend(AVC275X_SUB_ADDR, AVC275X_SA_SAPLL1_CTRL, &Value, 1);

        //Step 3. Enable VCO Calibration control bit
        //refer ANA_CTRL17 - [bit 2] for DAPLL
        AVC275X_i2crecv(AVC275X_SUB_ADDR, AVC275X_SA_SAPLL1_VCO_CTRL, &Value, 1);
        Value = (Value | 0x04);
        AVC275X_i2csend(AVC275X_SUB_ADDR, AVC275X_SA_SAPLL1_VCO_CTRL, &Value, 1);

        //Step 4. Set the KVCO with value starting with 7 and check the VCO Overthreshold status
        //Test the VCO Overthreshold status as 0 (bit set as Low)
        //In case the above condition is not satified decrement the KVCO and repeat the above step untill the
        //the condition is satisfied. KVCO value can be decremented until it becomes 0.

        for(Cnt = 8;Cnt > 0;Cnt--)
        {
            //refer ANA_CTRL14 - [bit 2:0] for DAPLL_KVCO
            Value = Cnt;
            if(Value >= 1)
                Value = Value - 1;
            else
                Value = 0;

            AVC275X_i2csend(AVC275X_SUB_ADDR, AVC275X_SA_SAPLL1_KVCO_CAL_CTRL, &Value, 1);

            //refer ANA_STAT0 - [bit 3] for the VCO Overthreshold indicator status
            AVC275X_i2crecv(AVC275X_SUB_ADDR, AVC275X_SA_PLL_STATUS, &Value, 1);
            if(0 == (Value & 0x08))
                    break;
        }

        //Step 5. After the KVCO calibration is completed, disable the VCO Calibration control bit
        //refer ANA_CTRL17 - [bit 2] for DAPLL
        AVC275X_i2crecv(AVC275X_SUB_ADDR, AVC275X_SA_SAPLL1_VCO_CTRL, &Value, 1);
        Value = (Value & 0xFB);
        AVC275X_i2csend(AVC275X_SUB_ADDR, AVC275X_SA_SAPLL1_VCO_CTRL, &Value, 1);
    }
    else if(AVC_SAPLL2 == PllType)
    {
        //Step 1. Power Down the Pll (DAPLL, SAPLL1, SAPLL2)
        //refer ANA_CTRL25 - [bit 1] for SAPLL1,
        AVC275X_i2crecv(AVC275X_SUB_ADDR, AVC275X_SA_SAPLL2_CTRL, &Value, 1);
        Value |= 0x02;
        AVC275X_i2csend(AVC275X_SUB_ADDR, AVC275X_SA_SAPLL2_CTRL, &Value, 1);

        //Step 2. Power Up the Pll
        //refer ANA_CTRL25 - [bit 1] for SAPLL1,
        Value = (Value & 0xFD);
        AVC275X_i2csend(AVC275X_SUB_ADDR, AVC275X_SA_SAPLL2_CTRL, &Value, 1);

        //Step 3. Enable VCO Calibration control bit
        //refer ANA_CTRL26 - [bit 2] for DAPLL
        AVC275X_i2crecv(AVC275X_SUB_ADDR, AVC275X_SA_SAPLL2_VCO_CTRL, &Value, 1);
        Value = (Value | 0x04);
        AVC275X_i2csend(AVC275X_SUB_ADDR, AVC275X_SA_SAPLL2_VCO_CTRL, &Value, 1);

        //Step 4. Set the KVCO with value starting with 7 and check the VCO Overthreshold status
        //Test the VCO Overthreshold status as 0 (bit set as Low)
        //In case the above condition is not satified decrement the KVCO and repeat the above step untill the
        //the condition is satisfied. KVCO value can be decremented until it becomes 0.

        for(Cnt = 8;Cnt > 0;Cnt--)
        {
            //refer ANA_CTRL23 - [bit 2:0] for DAPLL_KVCO
            Value = Cnt;
            if(Value >= 1)
                Value = Value - 1;
            else
                Value = 0;

            AVC275X_i2csend(AVC275X_SUB_ADDR, AVC275X_SA_SAPLL2_KVCO_CAL_CTRL, &Value, 1);

            //refer ANA_STAT0 - [bit 5] for the VCO Overthreshold indicator status
            AVC275X_i2crecv(AVC275X_SUB_ADDR, AVC275X_SA_PLL_STATUS, &Value, 1);
            if(0 == (Value & 0x20))
                    break;
        }

        //Step 5. After the KVCO calibration is completed, disable the VCO Calibration control bit
        //refer ANA_CTRL26 - [bit 2] for DAPLL
        AVC275X_i2crecv(AVC275X_SUB_ADDR, AVC275X_SA_SAPLL2_VCO_CTRL, &Value, 1);
        Value = (Value & 0xFB);
        AVC275X_i2csend(AVC275X_SUB_ADDR, AVC275X_SA_SAPLL2_VCO_CTRL, &Value, 1);
    }

    return 1;
}


/*****************************************************************************
- Function Name         : StartSdramAutoCalibration
- Function Parameters   : None
- Function Return type  : static AVC_RETURNS
- Function Description  :

This function does SDRAM auto calibration.
******************************************************************************/
UINT16 StartSdramAutoCalibration(void)
{
    UINT8 index = 0,Value = 0,Cnt = 0;
    UINT8 Value_L = 0,Value_H = 0;
    UINT16 CalibVal;

    // These settings are done before SDRAM Auto calibration and before PLL calibration.
    ConfigureAnalogMacroSettings();

    Value = AVC275X_DDR_PAGE_ID;
    // Address the 88DE275X page to access DDR Control registers
    AVC275X_i2csend(AVC275X_BASE_ADDR, 0x0, &Value, 1);

    // Step:1
    // Make all request monitors to 0 so that there is no activity on DDR pads
    // refer DDR_REQ_MON0_0 to DDR_REQ_MON6_1
    for(index = AVC_SDRAM_CLNT_NUM1; index <= AVC_SDRAM_CLNT_NUM14; index++ )
    {
        Value = 0x0;
        AVC275X_i2csend(AVC275X_SUB_ADDR, AVC275X_DDR_REQ_MON + (index - 1), &Value, 1);
    }

    // DDR phy registers are addressed in different page.
    Value = AVC275X_DDR_PHY_PAGE_ID;
    // Change the page address to access DDR Phy registers
    AVC275X_i2csend(AVC275X_BASE_ADDR, 0x0, &Value, 1);

    // Step:2
    // Trigger DDR_CAL_TRIG with any value.
    // refer DDR_CAL_TRIG
    Value = 0x0;
    AVC275X_i2csend(AVC275X_SUB_ADDR, AVC275X_DDR_CAL_TRIG, &Value, 1);

    // Wait for 100ms
    AvcDelay(100);

    // Step:3
    // Calculate ZPR low and high value.
    // refer DDR_CALP_L, DDR_CALP_H
    AVC275X_i2crecv(AVC275X_SUB_ADDR, AVC275X_DDR_CALP_L, &Value_L, 1);
    AVC275X_i2crecv(AVC275X_SUB_ADDR, AVC275X_DDR_CALP_H, &Value_H, 1);

    // Calibration value is of 16-bits.
    CalibVal = ((Value_L) | (Value_H << 8));

    // Continue the loop for 16 times and count the number of trailing zeroe's
    for(index = 0; index < 16; index++)
    {
        Value = (CalibVal & 0x1);
        if(!Value)
            Cnt++;
        else
            break;

        CalibVal >>= 1;
    }

    // Total trailing zeroes + 1 gives the Zpr value.
    Value = (Cnt + 1);

    // refer DS_PHY_ZPR_0, DS_PHY_ZPR_1
    AVC275X_i2csend(AVC275X_SUB_ADDR, AVC275X_DDR_ZPR_0, &Value, 1);
    AVC275X_i2csend(AVC275X_SUB_ADDR, AVC275X_DDR_ZPR_1, &Value, 1);

    // Set the Same value as ZPR.
    // refer DS_PHY_ZNR_0, DS_PHY_ZNR_1
    AVC275X_i2csend(AVC275X_SUB_ADDR, AVC275X_DDR_ZNR_0, &Value, 1);
    AVC275X_i2csend(AVC275X_SUB_ADDR, AVC275X_DDR_ZNR_1, &Value, 1);

    Value = AVC275X_DDR_PAGE_ID;
    // Address the 88DE275X page to access DDR Control registers
    AVC275X_i2csend(AVC275X_BASE_ADDR, 0x0, &Value, 1);

    // Step:5
    // Enable all the request monitors.
    // refer DDR_REQ_MON0_0 to DDR_REQ_MON6_1
    for(index = AVC_SDRAM_CLNT_NUM1; index <= AVC_SDRAM_CLNT_NUM14; index++ )
    {
        Value = 0xFF;
        AVC275X_i2csend(AVC275X_SUB_ADDR, AVC275X_DDR_REQ_MON + (index - 1), &Value, 1);
    }

    return 1;
}


/*****************************************************************************
- Function Name         : ConfigureAnalogMacroSettings
- Function Parameters   : None
- Function Return type  : static AVC_RETURNS
- Function Description  :

This function sets the Analog Control registers.
******************************************************************************/
static UINT16 ConfigureAnalogMacroSettings(void)
{
    UINT8 Value;

    Value = AVC275X_PLL_PAGE_ID;
    // Address the 88DE275X page, as all the registers are in the same page addressing
    // of the page will be done only once
    AVC275X_i2csend(AVC275X_BASE_ADDR, 0x0, &Value, 1);

    AVC275X_i2crecv(AVC275X_SUB_ADDR, AVC275X_SA_PLL_STAT14, &Value, 1);

    // Check the value.
    if(0x02 == Value)
    {
        // If the value is 0x02, set the following Ana_Ctrl settings
        Value = 0x3C;
        AVC275X_i2csend(AVC275X_SUB_ADDR,AVC275X_SA_DAPLL_CTRL, &Value, 1);

        Value = 0x05;
        AVC275X_i2csend(AVC275X_SUB_ADDR,AVC275X_XTAL_AMP_SEL, &Value, 1);

        Value = 0x03;
        AVC275X_i2csend(AVC275X_SUB_ADDR,AVC275X_DAC_CURRENT_SEL, &Value, 1);

        Value = 0x10;
        AVC275X_i2csend(AVC275X_SUB_ADDR,AVC275X_XTAL_CTRL, &Value, 1);
    }
    else if(0x03 == Value)
    {
        // If the value is 0x03, set the following Ana_Ctrl settings
        Value = 0x1C;
        AVC275X_i2csend(AVC275X_SUB_ADDR,AVC275X_SA_DAPLL_CTRL, &Value, 1);

        Value = 0x07;
        AVC275X_i2csend(AVC275X_SUB_ADDR,AVC275X_XTAL_AMP_SEL, &Value, 1);

        Value = 0x03;
        AVC275X_i2csend(AVC275X_SUB_ADDR,AVC275X_DAC_CURRENT_SEL, &Value, 1);

        Value = 0x9;
        AVC275X_i2csend(AVC275X_SUB_ADDR,AVC275X_XTAL_CTRL, &Value, 1);
    }
    return 1;
}


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
