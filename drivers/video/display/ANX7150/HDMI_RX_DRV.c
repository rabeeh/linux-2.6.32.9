

//******************************************************************************
//  I N C L U D E    F I L E S
//******************************************************************************

#include "HDMI_RX_DRV.h"   
//#include "i2c_intf.h"
//#include "HDMI_TX_i2c_intf.h"
//#include "ANX7150_i2C_intf.h"

//#include "Timer.h"
//#include "mcu.h"
#include "ANX7150.h"


//******************************************************************************
//  G L O B A L    V A R I A B L E S
//******************************************************************************
BYTE g_HDMI_DVI_Status;
BYTE g_Sys_State;
BYTE g_Cur_Pix_Clk;
BYTE g_Video_Stable_Cntr;
BYTE g_Audio_Stable_Cntr;
BYTE g_Sync_Expire_Cntr;
BYTE g_Timer_Slot ;  
BYTE g_HDCP_Err_Cnt;
BYTE g_Sync_Change; //record sync interrupt

WORD g_Cur_H_Res;
WORD g_Cur_V_Res;
BIT g_Video_Muted;
BIT g_Audio_Muted;
BIT g_CTS_Got;
BIT g_Audio_Got;
BIT g_Restart_System; 

BYTE set_color_mode_counter, deep_color_set_done,video_format_supported;
BIT hdmi_rx_ycmux_setting,hdmi_rx_auth_done;
BYTE hdmi_rx_avi_repeat_time;
BYTE hdmi_rx_chip_id;


HDMI_Audio_Mode HDMI_AUD_Mode;
HDMI_I2S_Format HDMI_I2S_Fmt;
HDMI_Infoframe_Type HDMI_InfoFrame;
HDMI_Video_Detailed_Info_t HDMI_Vid_Info;

//xdata unsigned char sysState,sysState_bkp;

/*
void HDMI_RX_Get_Infoframe(BYTE infoframe_type)
{
    BYTE i;
    switch(infoframe_type)
    {
        case InfoFrame_AVI:
            HDMI_RX_ReadI2C_RX1(HDMI_RX_AVI_TYPE_REG, &HDMI_InfoFrame.Type);
            HDMI_RX_ReadI2C_RX1(HDMI_RX_AVI_VER_REG, &HDMI_InfoFrame.Version);
            HDMI_RX_ReadI2C_RX1(HDMI_RX_AVI_LEN_REG, &HDMI_InfoFrame.Length);
            HDMI_RX_ReadI2C_RX1(HDMI_RX_AVI_CHKSUM_REG, &HDMI_InfoFrame.CRC);
            for(i = 0; i < 13; i ++)
            {
                HDMI_RX_ReadI2C_RX1(HDMI_RX_AVI_DATA00_REG + i, &HDMI_InfoFrame.Data[i]);
            }
            break;
        case InfoFrame_AUD:
            HDMI_RX_ReadI2C_RX1(HDMI_RX_AUDIO_TYPE_REG, &HDMI_InfoFrame.Type);
            HDMI_RX_ReadI2C_RX1(HDMI_RX_AUDIO_VER_REG, &HDMI_InfoFrame.Version);
            HDMI_RX_ReadI2C_RX1(HDMI_RX_AUDIO_LEN_REG, &HDMI_InfoFrame.Length);
            HDMI_RX_ReadI2C_RX1(HDMI_RX_AUDIO_CHKSUM_REG, &HDMI_InfoFrame.CRC);
            for(i = 0; i < 10; i ++)
            {
                HDMI_RX_ReadI2C_RX1(HDMI_RX_AUDIO_DATA00_REG + i, &HDMI_InfoFrame.Data[i]);
            }
            break;
        case InfoFrame_SPD:
            HDMI_RX_ReadI2C_RX1(HDMI_RX_SPD_TYPE_REG, &HDMI_InfoFrame.Type);
            HDMI_RX_ReadI2C_RX1(HDMI_RX_SPD_VER_REG, &HDMI_InfoFrame.Version);
            HDMI_RX_ReadI2C_RX1(HDMI_RX_SPD_LEN_REG, &HDMI_InfoFrame.Length);
            HDMI_RX_ReadI2C_RX1(HDMI_RX_SPD_CHKSUM_REG, &HDMI_InfoFrame.CRC);
            for(i = 0; i < 25; i ++)
            {
                HDMI_RX_ReadI2C_RX1(HDMI_RX_SPD_DATA00_REG + i, &HDMI_InfoFrame.Data[i]);
            }
            break;
        case InfoFrame_ACP:
            HDMI_RX_ReadI2C_RX1(HDMI_RX_ACP_HB0_REG, &HDMI_InfoFrame.Type);
            for(i = 0; i < 30; i ++)
            {
                HDMI_RX_ReadI2C_RX1(HDMI_RX_ACP_HB1_REG + i, &HDMI_InfoFrame.Data[i]);
            }
            break;
        case InfoFrame_MPEG:
            HDMI_RX_ReadI2C_RX1(HDMI_RX_MPEG_TYPE_REG, &HDMI_InfoFrame.Type);
            HDMI_RX_ReadI2C_RX1(HDMI_RX_MPEG_VER_REG, &HDMI_InfoFrame.Version);
            HDMI_RX_ReadI2C_RX1(HDMI_RX_MPEG_LEN_REG, &HDMI_InfoFrame.Length);
            HDMI_RX_ReadI2C_RX1(HDMI_RX_MPEG_CHKSUM_REG, &HDMI_InfoFrame.CRC);
            for(i = 0; i < 10; i ++)
            {
                HDMI_RX_ReadI2C_RX1(HDMI_RX_MPEG_DATA00_REG + i, &HDMI_InfoFrame.Data[i]);
            }
            break;
        default:
            break;
    }
//    return(HDMI_InfoFrame);
}

BYTE HDMI_RX_Get_SysState(void)
{
    if(g_Sys_State == MONITOR_CKDT)
        return Monitor_CKDT;
    else if(g_Sys_State == WAIT_SCDT)
        return Wait_SCDT;
    else if(g_Sys_State == WAIT_VIDEO)
        return Wait_VIDEO;
    else if(g_Sys_State == WAIT_AUDIO)
        return Wait_AUDIO;
    else if(g_Sys_State == PLAYBACK)
        return Playback;
}

bit HDMI_RX_Get_HDMI_DVI_Mode(void)
{
    BYTE hdmi_dvi,c;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_HDMI_STATUS_REG, &c);
    if (c & HDMI_RX_HDMI_MODE)
        hdmi_dvi = HDMI_MODE;
    else
        hdmi_dvi = DVI_MODE;
    return hdmi_dvi;
}
*/
int HDMI_RX_Get_Vid_H_Tol(void)
{
    BYTE c,c1;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_H_RESH_REG, &c);
    HDMI_RX_ReadI2C_RX0(HDMI_RX_H_RESL_REG, &c1);
    HDMI_Vid_Info.H_Tot = c;
    HDMI_Vid_Info.H_Tot = (HDMI_Vid_Info.H_Tot << 8) + c1;
    return HDMI_Vid_Info.H_Tot;
}

int HDMI_RX_Get_Vid_V_Tol(void)
{
    BYTE c,c1;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_V_RESH_REG, &c);
    HDMI_RX_ReadI2C_RX0(HDMI_RX_V_RESL_REG, &c1);
    HDMI_Vid_Info.V_Tol = c;
     HDMI_Vid_Info.V_Tol = ( HDMI_Vid_Info.V_Tol << 8) + c1;
    return  HDMI_Vid_Info.V_Tol;
}

int HDMI_RX_Get_Vid_H_Act(void)
{
    BYTE c,c1;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_DE_PIXH_REG, &c);
    HDMI_RX_ReadI2C_RX0(HDMI_RX_DE_PIXL_REG, &c1);
     HDMI_Vid_Info.H_Act = c;
    HDMI_Vid_Info.H_Act = (HDMI_Vid_Info.H_Act << 8) + c1;
    return HDMI_Vid_Info.H_Act;
}

int HDMI_RX_Get_Vid_V_Act(void)
{
    BYTE c,c1;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_DE_LINH_REG, &c);
    HDMI_RX_ReadI2C_RX0(HDMI_RX_DE_LINL_REG, &c1);
    HDMI_Vid_Info.V_Act = c;
    HDMI_Vid_Info.V_Act = (HDMI_Vid_Info.V_Act << 8) + c1;
    return HDMI_Vid_Info.V_Act;
}
/*
int HDMI_RX_Get_Vid_V_Freq(void)
{
    BYTE c;
    long int c1,c2,c3,c4;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_VID_PCLK_CNTR_REG, &c);
    c1 = HDMI_RX_Get_Vid_H_Tol();
    c2 = HDMI_RX_Get_Vid_V_Tol();
    c3 = c;
    c4 = c1 * c2 * c3;
    HDMI_Vid_Info.V_Freq = 13.5*1000000*128 / c4; 
    return HDMI_Vid_Info.V_Freq;
}

BYTE HDMI_RX_Get_Vid_V_FrontPorch(void)
{
    HDMI_RX_ReadI2C_RX0(HDMI_RX_ACTIVE_LIN_TO_VSYNC_REG, &HDMI_Vid_Info.V_Front_Porch);
    return HDMI_Vid_Info.V_Front_Porch;
}

BYTE HDMI_RX_Get_Vid_V_BackPorch(void)
{
    HDMI_RX_ReadI2C_RX0(HDMI_RX_VSYNC_TO_ACTIVE_LIN_REG, &HDMI_Vid_Info.V_Back_Porth);
    return HDMI_Vid_Info.V_Back_Porth;
}

BYTE HDMI_RX_Get_Vid_H_FrontPorch(void)
{
    HDMI_RX_ReadI2C_RX0(HDMI_RX_H_FRONT_PORCH_REG, &HDMI_Vid_Info.H_Front_Porch);
    return HDMI_Vid_Info.H_Front_Porch;
}

int HDMI_RX_Get_Vid_Hsync_Width(void)
{
    BYTE c,c1;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_HSYNC_ACTIVE_WIDTHH_REG, &c);
    HDMI_RX_ReadI2C_RX0(HDMI_RX_HSYNC_ACTIVE_WIDTHL_REG, &c1);
    HDMI_Vid_Info.Hsync_width = c;
    HDMI_Vid_Info.Hsync_width = (HDMI_Vid_Info.Hsync_width << 8) + c1;
    return HDMI_Vid_Info.Hsync_width;
}

bit HDMI_RX_Get_Vid_Vsync_Pol(void)
{
    BYTE c;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_VIDEO_STATUS_REG, &c);
    if(c & HDMI_RX_VSYNC_POL)
        HDMI_Vid_Info.VSYNC_POL = POL_positive;
    else
        HDMI_Vid_Info.VSYNC_POL = POL_negative;
    return HDMI_Vid_Info.VSYNC_POL;
}

bit HDMI_RX_Get_Vid_Hsync_Pol(void)
{
    BYTE c;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_VIDEO_STATUS_REG, &c);
    if(c & HDMI_RX_HSYNC_POL)
        HDMI_Vid_Info.HSYNC_POL = POL_positive;
    else
        HDMI_Vid_Info.HSYNC_POL = POL_negative;
    return HDMI_Vid_Info.HSYNC_POL;
}

BYTE HDMI_RX_Get_Vid_I_P(void)
{
    BYTE c;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_VIDEO_STATUS_REG, &c);
    if(c & HDMI_RX_VIDEO_TYPE)
        return HDMI_SIGNAL_INTERLACED;
    else
        return HDMI_SIGNAL_PROGRESSIVE;
}

BYTE HDMI_RX_Get_Vid_CS(void)
{
    BYTE c;
    HDMI_RX_ReadI2C_RX1(HDMI_RX_AVI_DATA00_REG, &c);
    if(c & 0x60 == 0x00)
        return HDMI_RGB;
    else if(c & 0x60 == 0x20)
        return HDMI_YCBCR422;
    else if(c & 0x60 == 0x40)
        return HDMI_YCBCR444;
}

BYTE HDMI_RX_Get_Device_ID_Low(void)
{
    BYTE id_low;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_DEV_IDL_REG, &id_low);
    return id_low;
}

BYTE HDMI_RX_Get_Device_ID_High(void)
{
    BYTE id_high;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_DEV_IDH_REG, &id_high);
    return id_high;
}

BYTE HDMI_RX_Get_Device_Revision(void)
{
    BYTE revision;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_DEV_REV_REG, &revision);
    return revision;
}

BYTE HDMI_RX_Get_Vid_Repeat_Times(void)
{
    BYTE c;
    HDMI_RX_ReadI2C_RX1(HDMI_RX_AVI_DATA04_REG, &c);
    HDMI_Vid_Info.pixel_repeat_times = c & 0x0f;
    return HDMI_Vid_Info.pixel_repeat_times;
}

void HDMI_RX_Power_On(void)
{
    BYTE c;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_PWDN1_REG, &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_SYS_PWDN1_REG, c & (~HDMI_RX_PWDN_CTRL)); 
}

void HDMI_RX_Power_Off(void)
{
    BYTE c;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_PWDN1_REG, &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_SYS_PWDN1_REG, c |HDMI_RX_PWDN_CTRL); 
}

BYTE HDMI_RX_Get_Power_Status(void)
{
    BYTE PWR_Status;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_PWDN1_REG, &PWR_Status);
    if(PWR_Status & 0x01)
        return 0;//power down
    else
        return 1;//power on
}

BYTE HDMI_RX_Get_VID_Output_Fmt(void)
{
    BYTE Vid_HW_Interface_fmt;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_VID_AOF_REG, &Vid_HW_Interface_fmt);
    return Vid_HW_Interface_fmt;
}
*/
BYTE HDMI_RX_Get_Input_Color_Depth(void)
{
    BYTE input_color_depth;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_VIDEO_STATUS_REG, &input_color_depth);
    input_color_depth = input_color_depth & 0xf0;
    if (input_color_depth == Deep_Color_legacy)
        return Hdmi_legacy;
    else if (input_color_depth == Deep_Color_24bit)
        return Hdmi_24bit;
    else if (input_color_depth == Deep_Color_30bit)
        return Hdmi_30bit;
    else if (input_color_depth == Deep_Color_36bit)
        return Hdmi_36bit;
}
/*
BYTE HDMI_RX_Get_Output_Color_Depth(void)
{
    BYTE output_color_depth;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_DEEP_COLOR_CTRL_REG, &output_color_depth);
    if(output_color_depth & HDMI_RX_EN_MAN_DEEP_COLOR)
        output_color_depth = output_color_depth & 0xf0;
    else 
    {
        HDMI_RX_ReadI2C_RX0(HDMI_RX_VIDEO_STATUS_REG, &output_color_depth);
        output_color_depth = output_color_depth & 0xf0;
    }
    if (output_color_depth == Deep_Color_legacy)
        return Hdmi_legacy;
    else if (output_color_depth == Deep_Color_24bit)
        return Hdmi_24bit;
    else if (output_color_depth == Deep_Color_30bit)
        return Hdmi_30bit;
    else if (output_color_depth == Deep_Color_36bit)
        return Hdmi_36bit;
}

void HDMI_RX_Get_AUD_HW_Interface_Fmt(void)
{
    BYTE aud_hw_interface = 0,c;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_AUD_OUT_I2S_CTRL2_REG, &c);
    c = c & 0xf0;
    if(c)
        aud_hw_interface = I2S_Output_Selected;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_AUDIO_OUT_CTRL_REG, &c);
    c = c & HDMI_RX_SP_EN;
    if(c)
        aud_hw_interface = aud_hw_interface | SPDIF_Output_Selected;
    switch(aud_hw_interface)
    {
        case I2S_Output_Selected:
            HDMI_AUD_Mode.I2S_enable = 1;
            break;
        case SPDIF_Output_Selected:
            HDMI_AUD_Mode.SPDIF_enable = 1;
            break;
        case I2SandSPDIF_Output_Selected:
            HDMI_AUD_Mode.I2S_enable = 1;
            HDMI_AUD_Mode.SPDIF_enable = 1;
            break;
        default:
            break;
    }
//    return HDMI_AUD_Mode;
}

BYTE HDMI_RX_Get_AUD_Fs(void)
{
    BYTE Fs;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_AUD_IN_SPDIF_CH_STATUS4_REG, &Fs);
    if(Fs == 0x00)
        return _44KHz;
    else if(Fs == 0x02)
        return _48KHz;
    else if(Fs == 0x03)
        return _32KHz;
    else if(Fs == 0x08)
        return _82KHz;
    else if(Fs == 0x0a)
        return _96KHz;
    else if(Fs == 0x0e)
        return _192KHz;
}

BYTE HDMI_RX_Get_AUD_MCLK_Freq(void)
{
    BYTE MCLK;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_ACR_AUD_FS_REG, &MCLK);
    MCLK = MCLK & 0x30;
    if(MCLK == 0x00)
        return _128Fs;
    else if(MCLK == 0x10)
        return _256Fs;
    else if(MCLK == 0x20)
        return _384Fs;
    else if(MCLK == 0x30)
        return _512Fs;
}

void HDMI_RX_Get_I2S_Info(void)
{
    BYTE I2S_Info;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_AUD_OUT_I2S_CTRL1_REG, &I2S_Info);
    if(I2S_Info & HDMI_RX_SCK_EDGE)
        HDMI_I2S_Fmt.SCK_edge = 1;
    else
        HDMI_I2S_Fmt.SCK_edge = 0;
    if(I2S_Info & HDMI_RX_SIZE_SEL)
        HDMI_I2S_Fmt.Size_Select = 1;
    else
        HDMI_I2S_Fmt.Size_Select = 0;
    if(I2S_Info & HDMI_RX_MSB_SIGN_EXT)
        HDMI_I2S_Fmt.MSB_SIGN_EXT = 1;
    else
        HDMI_I2S_Fmt.MSB_SIGN_EXT = 0;
    if(I2S_Info & HDMI_RX_WS_POL)
        HDMI_I2S_Fmt.WS_POL = 1;
    else
        HDMI_I2S_Fmt.WS_POL = 0;
    if(I2S_Info & HDMI_RX_JUST_CTRL)
        HDMI_I2S_Fmt.JUST_CTRL = 1;
    else
        HDMI_I2S_Fmt.JUST_CTRL = 0;
    if(I2S_Info & HDMI_RX_DIR_CTRL)
        HDMI_I2S_Fmt.DIR_CTRL = 1;
    else
        HDMI_I2S_Fmt.DIR_CTRL = 0;
    if(I2S_Info & HDMI_RX_SHIFT1)
        HDMI_I2S_Fmt.SHIFT = 1;
    else
        HDMI_I2S_Fmt.SHIFT = 0;
//    return (HDMI_I2S_Format);
}

BYTE HDMI_RX_Get_IntMask(BYTE Int_ID)
{
    BYTE MaskBits;
    HDMI_RX_ReadI2C_RX0(Int_ID, &MaskBits);
    return MaskBits;
}

BYTE HDMI_RX_Get_IntStatus(BYTE Int_ID)
{
    BYTE IntStatus;
    HDMI_RX_ReadI2C_RX0(Int_ID, &IntStatus);
    return IntStatus;
}

BYTE HDMI_RX_Get_Port_Status(void)
{
    BYTE Port_Status;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_PORT_SEL_REG, &Port_Status);
    if(Port_Status == 0x11)
        return 1;
    else if(Port_Status == 0x22)
        return 2;
    else
        return 0;
}
*/


void HDMI_RX_Int_Process(void)
{
    BYTE s1, s2, s3, s4, s5, s6,s7;
    if(g_Sys_State != MONITOR_CKDT)
    {
            HDMI_RX_ReadI2C_RX0(HDMI_RX_INT_STATUS1_REG , &s1);
            HDMI_RX_WriteI2C_RX0(HDMI_RX_INT_STATUS1_REG , s1);

            HDMI_RX_ReadI2C_RX0(HDMI_RX_INT_STATUS2_REG, &s2);
            HDMI_RX_WriteI2C_RX0(HDMI_RX_INT_STATUS2_REG, s2);

            HDMI_RX_ReadI2C_RX0(HDMI_RX_INT_STATUS3_REG, &s3);
            HDMI_RX_WriteI2C_RX0(HDMI_RX_INT_STATUS3_REG, s3);


            HDMI_RX_ReadI2C_RX0(HDMI_RX_INT_STATUS4_REG, &s4);
            HDMI_RX_WriteI2C_RX0(HDMI_RX_INT_STATUS4_REG, s4);

            HDMI_RX_ReadI2C_RX0(HDMI_RX_INT_STATUS5_REG, &s5);
            HDMI_RX_WriteI2C_RX0(HDMI_RX_INT_STATUS5_REG, s5);

            HDMI_RX_ReadI2C_RX0(HDMI_RX_INT_STATUS6_REG, &s6);
            HDMI_RX_WriteI2C_RX0(HDMI_RX_INT_STATUS6_REG, s6);
            
            HDMI_RX_ReadI2C_RX0(HDMI_RX_INT_STATUS7_REG, &s7);
            HDMI_RX_WriteI2C_RX0(HDMI_RX_INT_STATUS7_REG, s7);

            if (s1 & HDMI_RX_CKDT_CHANGE)//clk detect change interrupt
            {
                HDMI_RX_Clk_Detected_Int();
            }
            
            if (s1 & HDMI_RX_SCDT_CHANGE)  // SYNC detect interrupt
            {
                HDMI_RX_Sync_Det_Int();
            }
            
            if (s1 & HDMI_RX_HDMI_DVI) // HDMI_DVI detect interrupt
            {                
                HDMI_RX_HDMI_DVI_Int();
            }

            if (s6 & HDMI_RX_NEW_AVI) 
            {
                HDMI_RX_New_AVI_Int();
            }

            if ((s5 & HDMI_RX_CTS_ACR_CHANGE) || (s3 & HDMI_RX_ACR_PLL_UNLOCK))  
            { 
                HDMI_RX_Restart_Audio_Chk();
            }

            if (s6 & HDMI_RX_CTS_RCV)  
            {
                HDMI_RX_Cts_Rcv_Int();
            }

            if (s5 & HDMI_RX_AUDIO_RCV)  
            {
                HDMI_RX_Audio_Rcv_Int();
            }
      
            if ( s2 & HDMI_RX_HDCP_ERR)   // HDCP error
            {
                HDMI_RX_HDCP_Error_Int();
            }
            else
            {
                g_HDCP_Err_Cnt = 0;
            }
            
            if (s2 & HDMI_RX_MUTE_DONE)  // AAC done
            {
                HDMI_RX_Aac_Done_Int();
            }

			//debug
			//i2c_read_p0_reg(0xca, &ch4);
		    //debug_printf("$$$$$$ch4= %.2x\n", (unsigned int) ch4 );
        } 
    }

void HDMI_RX_Video_Timer_Slot(void)
{
    BYTE c, c1,avi_rpt;
    BYTE AVMUTE_STATUS;

    HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_STATUS_REG, &c);

    if(!(c & HDMI_RX_PWR5V))
    {
        HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_PWDN1_REG , &c1);
        HDMI_RX_WriteI2C_RX0(HDMI_RX_SYS_PWDN1_REG , c1 | HDMI_RX_PWDN_CTRL); // power down all	
        HDMI_RX_Set_Sys_State(MONITOR_CKDT);
    }
    else
    {
         if ((g_Sys_State == MONITOR_CKDT) )
        {
            HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_PWDN1_REG , &c1);
            HDMI_RX_WriteI2C_RX0(HDMI_RX_SYS_PWDN1_REG , c1 & (~HDMI_RX_PWDN_CTRL)); // power up all	
        }
    }

    c1 = c & HDMI_RX_CLK_DET;
    if( c1 ) 
    {					// KW 07/02/25
       if ((g_Sys_State == MONITOR_CKDT) )
       {
        	HDMI_RX_Set_Sys_State(WAIT_SCDT);
    	}
    }
    else
    {
        HDMI_RX_Set_Sys_State(MONITOR_CKDT);
    }

    if (g_Sys_State == MONITOR_CKDT) { return; }

    HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_STATUS_REG, &c);
    if (!(c & HDMI_RX_HDMI_DET)) // No sync in WAIT_VIDEO, WAIT_AUDIO, PLAYBACK
    {
            if (g_Sync_Expire_Cntr >= SCDT_EXPIRE_TH) 
            {
                printk("No sync for long time."); 
                HDMI_RX_WriteI2C_RX0(0x12, 0x02);
                HDMI_RX_Set_Sys_State(MONITOR_CKDT);
                g_Sync_Expire_Cntr = 0;
            }
            else
            {
                g_Sync_Expire_Cntr++;
            }
        return;
    }
    else// clear sync expire counter
    {
        g_Sync_Expire_Cntr = 0;
    }

    if ( g_Sys_State < WAIT_VIDEO )  
    { 
        g_HDMI_DVI_Status = DVI_MODE;
        return; 
    }

    HDMI_RX_Set_Color_Mode();

    //General Control Packet AV mute handler
    HDMI_RX_ReadI2C_RX0(HDMI_RX_HDMI_STATUS_REG, &c);
    AVMUTE_STATUS = (c & HDMI_RX_MUTE_STAT);
    if(AVMUTE_STATUS == HDMI_RX_MUTE_STAT) 
    {
        printk("HDMI_RX AV mute packet received."); 

        if (!g_Video_Muted) 
        {
            HDMI_RX_Mute_Video();
        }
        if (!g_Audio_Muted)
        {
            HDMI_RX_Mute_Audio();
        }
        HDMI_RX_Set_Sys_State(WAIT_VIDEO);
        //In HDMI_RX, AV mute status bit is write clear.
        HDMI_RX_WriteI2C_RX0(HDMI_RX_HDMI_STATUS_REG, c & (~HDMI_RX_MUTE_STAT));
    }

    if (HDMI_RX_Is_Video_Change()) 
    {
        printk("Video Changed , mute video and mute audio");
        HDMI_RX_Set_Sys_State(WAIT_VIDEO);
        g_Video_Stable_Cntr = 0;

        if (!g_Video_Muted)
        {  
            HDMI_RX_Mute_Video();
        }
        if (!g_Audio_Muted)
        {
            HDMI_RX_Mute_Audio();
        }
    } 
    else if (g_Video_Stable_Cntr < VIDEO_STABLE_TH) 
    {
        g_Video_Stable_Cntr++;
        printk("WAIT_VIDEO: Wait for video stable cntr.");
    } 
    else if ((g_Sys_State == WAIT_VIDEO) && (AVMUTE_STATUS==0x00))
    {
        if (g_HDMI_DVI_Status) 
        {
            if (g_Sys_State == WAIT_VIDEO) 
            {
                HDMI_RX_Get_Video_Info();
                printk("HDMI mode: Video is stable."); 
                HDMI_RX_Unmute_Video();	
                HDMI_RX_Set_Sys_State(WAIT_AUDIO);
            }
        } 
        else 
        {
            HDMI_RX_Get_Video_Info();
            printk("DVI mode: Video is stable.");
            HDMI_RX_Unmute_Video();
            HDMI_RX_Unmute_Audio();
            HDMI_RX_Set_Sys_State(PLAYBACK);
            HDMI_RX_Show_Video_Info();
        }
    }

    if((g_Sys_State >= WAIT_VIDEO) && (hdmi_rx_ycmux_setting == 1))
    {
        HDMI_RX_ReadI2C_RX1(0xa8,&avi_rpt);//get pixel repeat times in avi.
        avi_rpt = avi_rpt & 0x0f;
        if( hdmi_rx_avi_repeat_time != avi_rpt)
        {
            if(avi_rpt)
            {
                HDMI_RX_ReadI2C_RX0(HDMI_RX_AVC_EXCEPTION_MASK2_REG, &c);
                HDMI_RX_WriteI2C_RX0(HDMI_RX_AVC_EXCEPTION_MASK2_REG, c & ~HDMI_RX_OUT_CLK_DIV_MASK);

				 //for 480i DDR yc mux 601 demo, divide video clock.
				 if (P2_5)
			 	{
					 HDMI_RX_ReadI2C_RX0(HDMI_RX_AVC_EXCEPTION_MASK2_REG, &c);
	                HDMI_RX_WriteI2C_RX0(HDMI_RX_AVC_EXCEPTION_MASK2_REG, c | HDMI_RX_OUT_CLK_DIV_MASK);
	                HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_CTRL1_REG, &c);
	                HDMI_RX_WriteI2C_RX0(HDMI_RX_SYS_CTRL1_REG, (c & 0x3f ) | 0x40);
	                printk("HDMI_RX DDR YC-MUX 601 output re-setting!");
			 	}
            }
            else
            {
                HDMI_RX_ReadI2C_RX0(HDMI_RX_AVC_EXCEPTION_MASK2_REG, &c);
                HDMI_RX_WriteI2C_RX0(HDMI_RX_AVC_EXCEPTION_MASK2_REG, c | HDMI_RX_OUT_CLK_DIV_MASK);
                HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_CTRL1_REG, &c);
                HDMI_RX_WriteI2C_RX0(HDMI_RX_SYS_CTRL1_REG, c | 0x80);
                printk("HDMI_RX YC-MUX output re-setting!");
            }
        }
        hdmi_rx_avi_repeat_time = avi_rpt;
    }
    else
    {
        HDMI_RX_ReadI2C_RX0(HDMI_RX_AVC_EXCEPTION_MASK2_REG, &c);
        HDMI_RX_WriteI2C_RX0(HDMI_RX_AVC_EXCEPTION_MASK2_REG, c & ~HDMI_RX_OUT_CLK_DIV_MASK);
    }

    HDMI_RX_Get_Video_Info();
}

void HDMI_RX_Audio_Timer_Slot(void)
{
    BYTE c;//,c1,c2,c3;

    if (g_Sys_State == MONITOR_CKDT)
    {
        return; 
    }

    HDMI_RX_ReadI2C_RX0(HDMI_RX_HDMI_STATUS_REG, &c);
    if (!(c & HDMI_RX_HDMI_MODE)) // if not HDMI mode, do nothing
    {
        return; 
    }

    if (g_Sys_State == PLAYBACK) 
    {
        HDMI_RX_ReadI2C_RX0(HDMI_RX_HDMI_MUTE_CTRL_REG, &c);
        if (c & HDMI_RX_AUD_MUTE) 
        {
            HDMI_RX_Set_Sys_State(WAIT_AUDIO);
        }
    }

    if ((g_Sys_State != WAIT_AUDIO)) 
    {
        return;
    }

    HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_STATUS_REG, &c);
    if (!(c & HDMI_RX_HDMI_DET  )) 
    {
        return ; 
    }


    if (g_CTS_Got && g_Audio_Got) 
    {
        if (g_Audio_Stable_Cntr >= AUDIO_STABLE_TH) 
        {
            HDMI_RX_Unmute_Audio();
            HDMI_RX_Unmute_Video();
            g_Audio_Stable_Cntr = 0;
            HDMI_RX_Set_Sys_State(PLAYBACK);
            HDMI_RX_Show_Video_Info();
        } 
        else 
        {
            g_Audio_Stable_Cntr++;
        }
    } 
    else 
    {
        g_Audio_Stable_Cntr = 0;
    }
}

void HDMI_RX_Timer_Slot2(void) 
{
    BYTE c;

    HDMI_RX_ReadI2C_RX0(HDMI_RX_INT_MASK1_REG, &c);
    if (c == 0) { g_Restart_System = 1; }
    
    if(g_Restart_System)
    {
        g_Restart_System = 0;
        HDMI_RX_Chip_Located();
	 HDMI_RX_Initialize();  

    }
}

void HDMI_RX_Timer_Process(void)
{
    if(g_Timer_Slot == 3) 
        g_Timer_Slot = 0;
    else 
        g_Timer_Slot ++;

    if(g_Timer_Slot == 0) 
        HDMI_RX_Video_Timer_Slot();
    else if(g_Timer_Slot == 1) 
        HDMI_RX_Audio_Timer_Slot();
    else if(g_Timer_Slot == 2) 
        HDMI_RX_Timer_Slot2(); 
}

BYTE HDMI_RX_Chip_Located(void)
{
    BYTE c,c1;
    HDMI_RX_HardReset();
    HDMI_RX_WriteI2C_RX0(HDMI_RX_HDMI_MUTE_CTRL_REG , 0x3);
    HDMI_RX_ReadI2C_RX0(HDMI_RX_DEV_IDL_REG, &c);
    HDMI_RX_ReadI2C_RX0(HDMI_RX_DEV_IDH_REG, &c1);
    if((c == 0x35) && (c1 == 0x91))
    {
        printk("Repeater ANX8775 found");
        hdmi_rx_chip_id = ANX8775;
        return 1;
    }
    else if((c == 0x25) && (c1 == 0x91))
    {
        printk("Receiver ANX8770 found");
        hdmi_rx_chip_id = ANX8770;
        return 1;
    }
    else
    {
        printk("\n Can not read registers from chip, something wrong with I2C or chip.");
        //enable_debug_output = 0;//
        return 0;
    }
}

void HDMI_RX_HardReset(void)
{
    HDMIRX_reset_pin = 0;
    delay_ms(10);
    HDMIRX_reset_pin = 1;
    delay_ms(10);
}

void HDMI_RX_Init_var(void)
{
    HDMI_RX_Set_Sys_State(MONITOR_CKDT);
    g_Cur_H_Res = 0;
    g_Cur_V_Res = 0;
    g_Cur_Pix_Clk = 0;
    g_Video_Muted = 1;
    g_Audio_Muted = 1;
    g_Audio_Stable_Cntr = 0;
    g_Sync_Expire_Cntr = 0;
    g_Restart_System = 0;
    g_HDCP_Err_Cnt = 0;
    g_Sync_Change = 0;
    set_color_mode_counter = 0;
    video_format_supported = 1;
    hdmi_rx_avi_repeat_time = 0xff;
}

void HDMI_RX_Set_Sys_State(BYTE ss)
{
    if(g_Sys_State != ss)
    {
        printk("");
        g_Sys_State = ss;
        switch (ss)
        {
            case MONITOR_CKDT:
                printk("HDMI_RX:  MONITOR_CKDT");
                break;
            case WAIT_SCDT:
                printk("HDMI_RX:  WAIT_SCDT");
                break;
            case WAIT_VIDEO:
                printk("HDMI_RX:  WAIT_VIDEO");
                break;
            case WAIT_AUDIO:
                printk("HDMI_RX:  WAIT_AUDIO");
                HDMI_RX_Restart_Audio_Chk();
                break;
            case PLAYBACK:
                printk("HDMI_RX:  PLAYBACK");
                break;
            default:
                break;
        }
    }
}

void  HDMI_RX_Restart_Audio_Chk(void)
{
    if (g_Sys_State == WAIT_AUDIO)  
    {
        printk("WAIT_AUDIO: HDMI_RX_Restart_Audio_Chk.");
        g_CTS_Got = 0;
        g_Audio_Got = 0;
    }
}

void HDMI_RX_Mute_Video(void)
{
    BYTE c;

    printk("Mute Video.");
    HDMI_RX_ReadI2C_RX0(HDMI_RX_HDMI_MUTE_CTRL_REG, &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_HDMI_MUTE_CTRL_REG, c | HDMI_RX_VID_MUTE);
    g_Video_Muted = 1;
}
void HDMI_RX_Unmute_Video(void)
{
    BYTE c;

    printk("Unmute Video.");
    HDMI_RX_ReadI2C_RX0(HDMI_RX_HDMI_MUTE_CTRL_REG, &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_HDMI_MUTE_CTRL_REG, c & (~HDMI_RX_VID_MUTE));	
    g_Video_Muted =0;
}

void HDMI_RX_Mute_Audio(void)
{
    BYTE c;

    printk("Mute Audio.");
    HDMI_RX_ReadI2C_RX0(HDMI_RX_HDMI_MUTE_CTRL_REG, &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_HDMI_MUTE_CTRL_REG, c | HDMI_RX_AUD_MUTE);

    g_Audio_Muted = 1;
}

void HDMI_RX_Unmute_Audio(void)
{
    BYTE c;

    printk("Unmute Audio.");
    HDMI_RX_ReadI2C_RX0(HDMI_RX_HDMI_MUTE_CTRL_REG, &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_HDMI_MUTE_CTRL_REG, c & (~HDMI_RX_AUD_MUTE));
    g_Audio_Muted = 0;
}


char HDMI_RX_Is_Video_Change(void)
{
    BYTE ch, cl;
    xdata unsigned long n; //WORD n;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_H_RESL_REG, &cl);
    HDMI_RX_ReadI2C_RX0(HDMI_RX_H_RESH_REG, &ch);
    n = ch;
    n = (n << 8) + cl;
    if ((g_Cur_H_Res < (n-10)) || (g_Cur_H_Res > (n+10))) 
    {
        deep_color_set_done = 0;
        return 1;
    }
    HDMI_RX_ReadI2C_RX0(HDMI_RX_V_RESL_REG, &cl);  
    HDMI_RX_ReadI2C_RX0(HDMI_RX_V_RESH_REG, &ch);
    n = ch;
    n = (n << 8) + cl;
    if ((g_Cur_V_Res < (n-10)) || (g_Cur_V_Res > (n+10)))
    {
        deep_color_set_done = 0;
        return 1;
    }
    HDMI_RX_ReadI2C_RX0(HDMI_RX_HDMI_STATUS_REG, &cl);

    cl &= HDMI_RX_HDMI_MODE;
    if(g_HDMI_DVI_Status != cl)
    {
        printk("DVI to HDMI or HDMI to DVI Change.");
        deep_color_set_done = 0;
        return 1;
    }

    return 0;
}

void HDMI_RX_Get_Video_Info(void)
{
    BYTE ch, cl;
    unsigned int n;

    HDMI_RX_ReadI2C_RX0(HDMI_RX_H_RESL_REG, &cl);
    HDMI_RX_ReadI2C_RX0(HDMI_RX_H_RESH_REG, &ch);
    n = ch;
    n = (n << 8) + cl;
    g_Cur_H_Res = n;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_V_RESL_REG, &cl);
    HDMI_RX_ReadI2C_RX0(HDMI_RX_V_RESH_REG, &ch);
    n = ch;
    n = (n << 8) + cl;
    g_Cur_V_Res = n;

    HDMI_RX_ReadI2C_RX0(HDMI_RX_VID_PCLK_CNTR_REG, &cl);
    g_Cur_Pix_Clk = cl;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_HDMI_STATUS_REG, &cl);
    g_HDMI_DVI_Status = ((cl & HDMI_RX_HDMI_MODE) == HDMI_RX_HDMI_MODE);
}

void HDMI_RX_Sync_Det_Int(void)
{
    BYTE c;
    
    printk("*HDMI_RX Interrupt: Sync Detect."); 
    g_Sync_Change = 1;    // record sync change
    deep_color_set_done = 0;
    set_color_mode_counter = 0;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_STATUS_REG, &c);
    
    if (c & HDMI_RX_HDMI_DET) // sync existed
    {
            printk("Sync found.");
            HDMI_RX_Mute_Audio();
            HDMI_RX_Mute_Video();
            HDMI_RX_Set_Sys_State(WAIT_VIDEO);
            g_Video_Stable_Cntr = 0;
            HDMI_RX_Get_Video_Info();// save first video info  
    } 
    else    // sync lost
    {
        printk("Sync lost."); 
        HDMI_RX_Mute_Audio();
        HDMI_RX_Mute_Video();
        if(c & 0x02) //check if the clock lost
        {
            HDMI_RX_Set_Sys_State(WAIT_SCDT); 
        }
        else 
        { 
            HDMI_RX_Set_Sys_State(MONITOR_CKDT); 
        }
    }// end of sync lost
}

void HDMI_RX_Aac_Done_Int(void)
{
    BYTE c;

    printk("*HDMI_RX Interrupt: AEC AAC Done.");
    HDMI_RX_ReadI2C_RX0(HDMI_RX_HDMI_MUTE_CTRL_REG, &c);
    if (c & HDMI_RX_AUD_MUTE) 
    {
        g_Audio_Muted = 1;
        if (g_Sys_State == PLAYBACK) 
        {
            HDMI_RX_ReadI2C_RX0(HDMI_RX_HDMI_STATUS_REG, &c);
            if (c & HDMI_RX_HDMI_MODE)
            {
                HDMI_RX_Set_Sys_State(WAIT_AUDIO); 
            }
        }
    } 
}


void HDMI_RX_HDMI_DVI_Int(void)
{
    BYTE c;

    printk("*HDMI_RX Interrupt: HDMI-DVI Mode Change."); 
    HDMI_RX_ReadI2C_RX0(HDMI_RX_HDMI_STATUS_REG, &c);
    HDMI_RX_Get_Video_Info();
    if ((c & HDMI_RX_HDMI_MODE) == HDMI_RX_HDMI_MODE) 
    {
        printk("HDMI_RX_HDMI_DVI_Int: HDMI MODE."); 
        if ( g_Sys_State==PLAYBACK )
        {
            HDMI_RX_Set_Sys_State(WAIT_AUDIO); 
        }
    } 
    else 
    {
        HDMI_RX_Unmute_Audio();
    }
}

void HDMI_RX_Cts_Rcv_Int(void)
{
    g_CTS_Got = 1;
}

void HDMI_RX_Audio_Rcv_Int(void)
{
    g_Audio_Got = 1;
}

void HDMI_RX_HDCP_Error_Int(void)
{
    BYTE c;

    g_Audio_Got = 0;
    g_CTS_Got = 0;
    
    if(g_HDCP_Err_Cnt >= 40 )
    {
        g_HDCP_Err_Cnt = 0;
        printk("Lots of hdcp error occured ..."); 
        HDMI_RX_Mute_Audio();
        HDMI_RX_Mute_Video();

        //issue hotplug
        HDMI_RX_ReadI2C_RX0(HDMI_RX_PORT_SEL_REG, &c);
        if( c&0x01 )    //port 0
        {
            HDMI_RX_Set_HPD(0,300);
            printk("++++++++++Debug information:   HDCP error interrupt: port 0 HPD issued."); 
        }
        else
        {
            HDMI_RX_Set_HPD(1,300);
            printk("++++++++++Debug information:   HDCP error interrupt: port 1 HPD issued."); 
        }
    }
    else if((g_Sys_State==MONITOR_CKDT) || (g_Sys_State == WAIT_SCDT))
    {
        g_HDCP_Err_Cnt = 0;
    }
    else
    {
        g_HDCP_Err_Cnt ++;
    }

}

void HDMI_RX_New_AVI_Int(void)
{
    BYTE c;
    
    printk("*HDMI_RX Interrupt: New AVI Packet."); 

    HDMI_RX_ReadI2C_RX1(HDMI_RX_AVI_TYPE_REG, &c);
    debug_printf("AVI Infoframe:\n");
    debug_printf("0x%.2x ",(WORD)c);
    HDMI_RX_ReadI2C_RX1(HDMI_RX_AVI_VER_REG, &c);
    debug_printf("0x%.2x ",(WORD)c);
    HDMI_RX_ReadI2C_RX1(HDMI_RX_AVI_LEN_REG, &c);
    debug_printf("0x%.2x ",(WORD)c);
    HDMI_RX_ReadI2C_RX1(HDMI_RX_AVI_CHKSUM_REG, &c);
    debug_printf("    Check Sum = 0x%.2x \n",(WORD)c);
    HDMI_RX_ReadI2C_RX1(HDMI_RX_AVI_DATA00_REG, &c);
    debug_printf("0x%.2x ",(WORD)c);
    HDMI_RX_ReadI2C_RX1(HDMI_RX_AVI_DATA01_REG, &c);
    debug_printf("0x%.2x ",(WORD)c);
    HDMI_RX_ReadI2C_RX1(HDMI_RX_AVI_DATA02_REG, &c);
    debug_printf("0x%.2x ",(WORD)c);
    HDMI_RX_ReadI2C_RX1(HDMI_RX_AVI_DATA03_REG, &c);
    debug_printf("0x%.2x ",(WORD)c);
    HDMI_RX_ReadI2C_RX1(HDMI_RX_AVI_DATA04_REG, &c);
    debug_printf("0x%.2x ",(WORD)c);
    HDMI_RX_ReadI2C_RX1(HDMI_RX_AVI_DATA05_REG, &c);
    debug_printf("0x%.2x ",(WORD)c);
    HDMI_RX_ReadI2C_RX1(HDMI_RX_AVI_DATA06_REG, &c);
    debug_printf("0x%.2x ",(WORD)c);
    HDMI_RX_ReadI2C_RX1(HDMI_RX_AVI_DATA07_REG, &c);
    debug_printf("0x%.2x ",(WORD)c);
    HDMI_RX_ReadI2C_RX1(HDMI_RX_AVI_DATA08_REG, &c);
    debug_printf("0x%.2x ",(WORD)c);
    HDMI_RX_ReadI2C_RX1(HDMI_RX_AVI_DATA09_REG, &c);
    debug_printf("0x%.2x \n",(WORD)c);
}

void HDMI_RX_VID_Output_Fmt(BYTE Vid_HW_Interface_fmt)
{
    HDMI_RX_WriteI2C_RX0(HDMI_RX_VID_AOF_REG, Vid_HW_Interface_fmt);
    if((Vid_HW_Interface_fmt == YCbCr422_YCMUX) || (Vid_HW_Interface_fmt == YCbCr422_656_YCMUX))
        hdmi_rx_ycmux_setting = 1;
    else 
        hdmi_rx_ycmux_setting = 0;
}

void HDMI_RX_MCLK_Source(BYTE MCLK_Source)
{
    BYTE c;
    // MCLK select 
    HDMI_RX_ReadI2C_RX0(HDMI_RX_ACR_CTRL1_REG, &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_ACR_CTRL1_REG, 
        (c & 0xfe) |MCLK_Source);
}

void HDMI_RX_Set_MCLK_Freq(BYTE MCLK_Fs_Relationship)
{
    BYTE c;
    // MCLK frequence select
    HDMI_RX_ReadI2C_RX0(HDMI_RX_ACR_AUD_FS_REG, &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_ACR_AUD_FS_REG, 
        (c & 0xcf) | MCLK_Fs_Relationship);
}

void HDMI_RX_I2S_Mode(bit SCK_EDGE, bit size_sel, bit msb_sign_ext,
        bit ws_pol, bit JUST_CTRL, bit DIR_CTRL, bit SHIFT1)
{
    BYTE c,c0,c1,c2,c3,c4,c5,c6;
    c6 = SCK_EDGE;
    c6 = c6 << 6;
    c5 = size_sel;
    c5 = c5 << 5;
    c4 = msb_sign_ext;
    c4 = c4 << 4;
    c3 = ws_pol;
    c3 = c3 << 3;
    c2 = JUST_CTRL;
    c2 = c2 << 2;
    c1 = DIR_CTRL;
    c1 = c1 << 1;
    c0 = SHIFT1;
    c = c0 | c1 | c2 | c3 | c4 | c5 | c6;
    HDMI_RX_WriteI2C_RX0(HDMI_RX_AUD_OUT_I2S_CTRL1_REG, c);
}

void HDMI_RX_AUD_Output_Fmt(BYTE Aud_HW_Interface_fmt)
{
    BYTE c;
    // Enable I2S output 
    if((Aud_HW_Interface_fmt == I2S_Output_Selected)
        ||(Aud_HW_Interface_fmt == I2SandSPDIF_Output_Selected))
    {
        printk("Enable I2S audio output.");
        HDMI_RX_ReadI2C_RX0(HDMI_RX_AUD_OUT_I2S_CTRL2_REG, &c);
        HDMI_RX_WriteI2C_RX0(HDMI_RX_AUD_OUT_I2S_CTRL2_REG, c | 0xf9);
		// Set I2S_MODE on  KW 07/02/24
        HDMI_RX_ReadI2C_RX0(HDMI_RX_AUDIO_OUT_CTRL_REG, &c);
        HDMI_RX_WriteI2C_RX0(HDMI_RX_AUDIO_OUT_CTRL_REG, c | HDMI_RX_I2S_MODE);
    }
    else  // Disable I2S 
    {
        printk("Disable I2S audio output.");
        HDMI_RX_ReadI2C_RX0(HDMI_RX_AUD_OUT_I2S_CTRL2_REG, &c);
        HDMI_RX_WriteI2C_RX0(HDMI_RX_AUD_OUT_I2S_CTRL2_REG, c & 0x0f);
// KW
        HDMI_RX_ReadI2C_RX0(HDMI_RX_AUDIO_OUT_CTRL_REG, &c);
        HDMI_RX_WriteI2C_RX0(HDMI_RX_AUDIO_OUT_CTRL_REG, c & (~HDMI_RX_I2S_MODE));
    }

     if((Aud_HW_Interface_fmt == SPDIF_Output_Selected)
        ||(Aud_HW_Interface_fmt == I2SandSPDIF_Output_Selected))    // Enable SPDIF
    {
        printk("Enable SPDIF audio output.");
        HDMI_RX_ReadI2C_RX0(HDMI_RX_AUDIO_OUT_CTRL_REG, &c);
        HDMI_RX_WriteI2C_RX0(HDMI_RX_AUDIO_OUT_CTRL_REG, c | HDMI_RX_SP_EN);
		 HDMI_RX_ReadI2C_RX0(HDMI_RX_AUD_OUT_I2S_CTRL2_REG, &c); //
        HDMI_RX_WriteI2C_RX0(HDMI_RX_AUD_OUT_I2S_CTRL2_REG, c | 0x08);
    }
    else        //Disable SPDIF
    {
        printk("Disable SPDIF audio output.");
        HDMI_RX_ReadI2C_RX0(HDMI_RX_AUDIO_OUT_CTRL_REG, &c);
        HDMI_RX_WriteI2C_RX0(HDMI_RX_AUDIO_OUT_CTRL_REG, c & (~HDMI_RX_SP_EN));
    }

}

void HDMI_RX_Set_IntPin(BYTE Pin_Output_Type, 
    BYTE Pin_Output_Polarity)
{
    BYTE c;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_INT_CTRL_REG, &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_INT_CTRL_REG, (c & 0xfc) 
        |Pin_Output_Type | Pin_Output_Polarity);
}

void HDMI_RX_Set_IntMask(BYTE Int_ID, BYTE MaskBits)
{
    HDMI_RX_WriteI2C_RX0(Int_ID, MaskBits);
}
/*
void HDMI_RX_Clear_Int(BYTE Int_ID, BYTE ClrBits)
{
    HDMI_RX_WriteI2C_RX0(Int_ID, ClrBits);
}
*/
void HDMI_RX_Set_AEC_CtrlBits(BYTE AEC_ID, BYTE CtrlBits)
{
    HDMI_RX_WriteI2C_RX0(AEC_ID, CtrlBits);
}
/*
BYTE HDMI_RX_Get_AEC_CtrlBits(BYTE AEC_id)
{
    BYTE CtrlBits;
    HDMI_RX_ReadI2C_RX0(AEC_id, &CtrlBits);
    return CtrlBits;
}
*/
void HDMI_RX_AVC_On(void)
{
    BYTE c;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_AEC_CTRL_REG , &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_AEC_CTRL_REG , c | HDMI_RX_AVC_EN );
}

/*
void HDMI_RX_AVC_Off(void)
{
    BYTE c;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_AEC_CTRL_REG , &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_AEC_CTRL_REG , c & ~HDMI_RX_AVC_EN );
}
*/

void HDMI_RX_Set_Video_CH_Mapping(BYTE MappingValue)
{
    HDMI_RX_WriteI2C_RX0(HDMI_RX_VID_CH_MAP_REG, MappingValue);
}
/*
void HDMI_RX_Set_Video_Bit_Swap(bit swap_value)
{
    BYTE c,c1;
    c1 = swap_value;
    c1 = c1 << 3;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_VID_OUTPUT_CTRL2_REG, &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_VID_OUTPUT_CTRL2_REG , (c & 0xf7) | c1);
}

void HDMI_RX_Set_Video_Bus_Mode(bit bus_mode)
{
    BYTE c,c1;
    c1 = bus_mode;
    c1 = c1 << 2;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_VID_OUTPUT_CTRL2_REG, &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_VID_OUTPUT_CTRL2_REG , (c & 0xfb) | c1);
}
*/
void HDMI_RX_Set_Color_Depth(BYTE ColorDepth)
{
    BYTE c;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_DEEP_COLOR_CTRL_REG , &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_DEEP_COLOR_CTRL_REG , 
        c | HDMI_RX_EN_MAN_DEEP_COLOR | ColorDepth);
}

void HDMI_RX_Set_HPD(BYTE Port, int delaytime)
{
    if(Port)
    {
        ;
/*
        printk("port 1 HPD issued... ");
        HDMI_RX_Port1_HPD = 0;
        delay_ms(delaytime);
        HDMI_RX_Port1_HPD = 1;    
*/        
    }
    else
    {
        printk("port 0 HPD issued... ");
        HDMIRX_Hotplug_pin = 0;
        delay_ms(delaytime);
        HDMIRX_Hotplug_pin = 1;    
    }    
}

void HDMI_RX_Initialize()
{
    BYTE c;

    HDMI_RX_Init_var();

    // Mute audio and video output
    HDMI_RX_WriteI2C_RX0(HDMI_RX_HDMI_MUTE_CTRL_REG, 0x03);
    HDMI_RX_WriteI2C_RX1(HDMI_RX_HDCP_BCAPS_SHADOW_REG, 0x81);
    
    // Set clock detect source
    // Enable pll_lock, digital clock detect, disable analog clock detect
    HDMI_RX_WriteI2C_RX0(HDMI_RX_CHIP_CTRL_REG, 0xe5);

    // Init Interrupt
    HDMI_RX_Set_IntPin(IntPin_Out_Type_open_drain,IntPin_Out_Polarity_Low);
    HDMI_RX_Set_IntMask(HDMI_RX_INT_MASK1_REG, 0xff);    
    HDMI_RX_Set_IntMask(HDMI_RX_INT_MASK2_REG, 0xf3);    
    HDMI_RX_Set_IntMask(HDMI_RX_INT_MASK3_REG, 0x3f);    
    HDMI_RX_Set_IntMask(HDMI_RX_INT_MASK4_REG, 0x17);    
    HDMI_RX_Set_IntMask(HDMI_RX_INT_MASK5_REG, 0xff);    
    HDMI_RX_Set_IntMask(HDMI_RX_INT_MASK6_REG, 0xff);
    HDMI_RX_Set_IntMask(HDMI_RX_INT_MASK7_REG, 0x07);

    HDMI_RX_Set_AEC_CtrlBits(HDMI_RX_AEC_EN0_REG, 0xe7);   
    HDMI_RX_Set_AEC_CtrlBits(HDMI_RX_AEC_EN1_REG, 0x17);    // KW
    HDMI_RX_Set_AEC_CtrlBits(HDMI_RX_AEC_EN2_REG, 0xf4);    // KW

    // Init CTS threshold
    HDMI_RX_WriteI2C_RX0(HDMI_RX_ACR_CTRL1_REG, 0xc0);  // for 1080P Audio at Deep Color By KW
    HDMI_RX_WriteI2C_RX0(HDMI_RX_ACR_CTRL3_REG, 0x07);

	//# Enable AVC and AAC
    HDMI_RX_AVC_On();
    HDMI_RX_ReadI2C_RX0(HDMI_RX_AEC_CTRL_REG, &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_AEC_CTRL_REG, c | HDMI_RX_AAC_EN);

	if (!BIST_EN ) //no bist
	{
	    if(QFN48_EN)
    	{
			c = P2_6;
			//c = (c<<1) | P2_6;
			debug_printf("video interface***c= %.2x\n", (unsigned int)c);
			switch (c)
			{
				case 0:
					HDMI_RX_VID_Output_Fmt(YCbCr422_YCMUX); //yc mux 601
					break;
				case 1:
					HDMI_RX_VID_Output_Fmt(YCbCr422_656_YCMUX); //yc mux 656
					break;
#if 0
				case 2:
					HDMI_RX_VID_Output_Fmt(YCbCr422_YCMUX); //DDR yc422 601
					hdmi_rx_ycmux_setting = 0;
					//HDMI_RX_AVC_Off();
					/*
					HDMI_RX_ReadI2C_RX0(HDMI_RX_AVC_EXCEPTION_MASK2_REG, &c); //for 8770 internal divide.
				    HDMI_RX_WriteI2C_RX0(HDMI_RX_AVC_EXCEPTION_MASK2_REG, ((c & 0xbf) | 0x40));
					HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_CTRL1_REG, &c);
				    HDMI_RX_WriteI2C_RX0(HDMI_RX_SYS_CTRL1_REG, ((c & 0x3f) | 0x40));
				    */
					break;

				case 3:
					HDMI_RX_VID_Output_Fmt(YCbCr422_656_YCMUX); //DDR yc422 656
					hdmi_rx_ycmux_setting = 0;

					//HDMI_RX_AVC_Off();
					/*
					HDMI_RX_ReadI2C_RX0(HDMI_RX_AVC_EXCEPTION_MASK2_REG, &c);
				    HDMI_RX_WriteI2C_RX0(HDMI_RX_AVC_EXCEPTION_MASK2_REG, ((c & 0xbf) | 0x40));
					HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_CTRL1_REG, &c);
	    			HDMI_RX_WriteI2C_RX0(HDMI_RX_SYS_CTRL1_REG, ((c & 0x3f) | 0x40));
	    			*/
					break;
#endif
	      default: 
	            break;
								
				}
	    	}
			else
			{
				HDMI_RX_VID_Output_Fmt(RGB444);
			   HDMI_RX_ReadI2C_RX0(HDMI_RX_VID_OUTPUT_CTRL2_REG, &c);
              HDMI_RX_WriteI2C_RX0(HDMI_RX_VID_OUTPUT_CTRL2_REG, c | 0x04);
			}
	}
	else
		HDMI_RX_VID_Output_Fmt(RGB444);

    //Swap color Red and Blue to simplify video DAC routing 0x8a
    HDMI_RX_Set_Video_CH_Mapping(VID_MAP_Red_Green_Blue);
//* 
    //For debug purpose disable this function temporary //zjn 070212
    //Set output video bus width to 30bits by using the rounding feature
    HDMI_RX_ReadI2C_RX0(HDMI_RX_VID_DDR_OUTPUT_CTRL_REG, &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_VID_DDR_OUTPUT_CTRL_REG, ((c & 0xf3) | 0x04));
//*/
    

//    HDMI_RX_AUD_Output_Fmt(I2S_Output_Selected);
    HDMI_RX_AUD_Output_Fmt(SPDIF_Output_Selected);
    HDMI_RX_MCLK_Source(HDMI_MCLK_Selected);
    HDMI_RX_Set_MCLK_Freq(MCLK_Fs_128);
    HDMI_RX_I2S_Mode(I2S_SCK_EDGE_Rising, I2S_Size_32bit, I2S_MSB_SIGN_EXT_Enable,
        I2S_WS_POL_Low,I2S_JUST_CTRL_Left,I2S_DIR_CTRL_MSB,I2S_SHIFT);

    //swap the i2s output form sd0~3 to sd3~0 to match the 8560 bu board design
    HDMI_RX_WriteI2C_RX0(HDMI_RX_AUD_OUT_I2S_MAP_REG, 0xe4);//0x1b);

    //  reset HDCP
    HDMI_RX_ReadI2C_RX0(HDMI_RX_SRST_REG, &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_SRST_REG , c | HDMI_RX_HDCP_MAN_RST);
	delay_ms(10);
// KW 07/02/22
    HDMI_RX_ReadI2C_RX0(HDMI_RX_SRST_REG, &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_SRST_REG , c & ~HDMI_RX_HDCP_MAN_RST);

    HDMI_RX_SoftReset();

    HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_PWDN1_REG, &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_SYS_PWDN1_REG, c &(~HDMI_RX_PWDN_CTRL)); // power up all

    // Set EQ Value
    HDMI_RX_WriteI2C_RX0(HDMI_RX_TMDS_P0_EQ_REG, 0xec);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_TMDS_P1_EQ_REG, 0xec);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_TMDS_EQ2_REG, 0x28);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_TMDS_EQ_VCOM_REG, 0x20);
    //Set PLL LOOP GAIN
    HDMI_RX_WriteI2C_RX0(HDMI_RX_TMDS_PLL_CTRL1_REG, 0xe0); 
    // set cdr bandwidth
    HDMI_RX_WriteI2C_RX0(HDMI_RX_TIMING_LOOP_CNFG2_REG, 0x5a); 

	HDMI_RX_WriteI2C_RX0(HDMI_RX_AUD_OUT_I2S_CTRL1_REG, 0x00);  	//wen updated

    //Range limitation for RGB input
    HDMI_RX_ReadI2C_RX0(HDMI_RX_VID_DATA_RNG_CTRL_REG, &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_VID_DATA_RNG_CTRL_REG, (c | HDMI_RX_R2Y_INPUT_LIMIT)); 

	HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_CTRL1_REG, &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_SYS_CTRL1_REG, (c | HDMI_RX_OUT_CLK_INV)); 

		HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_PWDN2_REG, &c);	//dis output of 8770, 090630
	    HDMI_RX_WriteI2C_RX0(HDMI_RX_SYS_PWDN2_REG, c | 0x07);
			
    printk("Chip is initialized..."); 
}

void HDMI_RX_SoftReset(void)
{
    BYTE c;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_SRST_REG, &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_SRST_REG , c | HDMI_RX_SW_MAN_RST);
    delay_ms(10);
    HDMI_RX_ReadI2C_RX0(HDMI_RX_SRST_REG, &c);
    HDMI_RX_WriteI2C_RX0(HDMI_RX_SRST_REG , c & ~HDMI_RX_SW_MAN_RST);
}


void HDMI_RX_Port_Select(INPUT_PORT Selected_Port)
{
    BYTE CurrentInputPort, c ;
    HDMI_RX_ReadI2C_RX0(HDMI_RX_PORT_SEL_REG, &CurrentInputPort);

    if( (Selected_Port == PORT1) && (CurrentInputPort != HDMI_PORT1) )  
    {
        printk("**** switch to port1"); 
        HDMI_RX_Initialize();

// KW 07/02/25
// Power down port0 termination and on port1
        HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_PWDN3_REG, &c);
        HDMI_RX_WriteI2C_RX0(HDMI_RX_SYS_PWDN3_REG , (c | HDMI_RX_PD_TERM0) & ~HDMI_RX_PD_TERM1); 

        HDMI_RX_WriteI2C_RX0(HDMI_RX_PORT_SEL_REG, 0x02); 
        HDMI_RX_Set_HPD(PORT1, 300);
        HDMI_RX_WriteI2C_RX0(HDMI_RX_PORT_SEL_REG, 0x22); // Enable DDC after HPD
    }
    else if((Selected_Port == PORT0) && (CurrentInputPort != HDMI_PORT0) ) 
    {
        printk("**** switch to port0"); 
        HDMI_RX_Initialize();

// KW 07/02/25
// Power down port1 termination and on port0
        HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_PWDN3_REG, &c);
        HDMI_RX_WriteI2C_RX0(HDMI_RX_SYS_PWDN3_REG , ( c | HDMI_RX_PD_TERM1) & ~HDMI_RX_PD_TERM0);
        
        HDMI_RX_WriteI2C_RX0(HDMI_RX_PORT_SEL_REG, 0x01);
        HDMI_RX_Set_HPD(PORT0, 300);
        HDMI_RX_WriteI2C_RX0(HDMI_RX_PORT_SEL_REG, 0x11); 

    }
}

void HDMI_RX_Clk_Detected_Int(void)
{
    BYTE c;

    debug_printf("*HDMI_RX Interrupt: Pixel Clock Change.\n");
    HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_STATUS_REG, &c);
    if (c & HDMI_RX_CLK_DET)
    {
        debug_printf("   Pixel clock existed. \n");
        return;
    } 
    else
    {
        debug_printf("   Pixel clock lost. \n");
        HDMI_RX_Set_Sys_State(MONITOR_CKDT);
        g_Sync_Expire_Cntr = 0;
    }	
}

void HDMI_RX_Show_Video_Info(void)
{
    BYTE c;
    BYTE cl,ch;
    WORD n;
    WORD h_res,v_res;
    
    HDMI_RX_ReadI2C_RX0(HDMI_RX_DE_PIXL_REG, &cl);
    HDMI_RX_ReadI2C_RX0(HDMI_RX_DE_PIXH_REG, &ch);
    n = ch;
    n = (n << 8) + cl;
    h_res = n;
    
    HDMI_RX_ReadI2C_RX0(HDMI_RX_DE_LINL_REG, &cl);
    HDMI_RX_ReadI2C_RX0(HDMI_RX_DE_LINH_REG, &ch);	// KW 07/02/25
    n = ch;
    n = (n << 8) + cl;
    v_res = n;
    
    printk("");
    printk("******************************HDMI_RX Info*******************************");
    debug_printf("HDMI_RX Is Normally Play Back.\n");
    HDMI_RX_ReadI2C_RX0(HDMI_RX_HDMI_STATUS_REG, &c); 
    if(c & HDMI_RX_HDMI_MODE)
        debug_printf("HDMI_RX Mode = HDMI Mode.\n");
    else
        debug_printf("HDMI_RX Mode = DVI Mode.\n");

    HDMI_RX_ReadI2C_RX0(HDMI_RX_VIDEO_STATUS_REG, &c); 
    if(c & HDMI_RX_VIDEO_TYPE)
    {
        v_res += v_res;
    }       
    debug_printf("HDMI_RX Video Resolution = %d * %d ",h_res,v_res);
    HDMI_RX_ReadI2C_RX0(HDMI_RX_VIDEO_STATUS_REG, &c); 
    if(c & HDMI_RX_VIDEO_TYPE)
        printk("    Interlace Video.");
    else
        printk("    Progressive Video.");


    debug_printf("The Video Interface = Digital Video");
    if(ANALOG_VID_EN)
        debug_printf("  +  Analog Video.\n");
    else
        debug_printf(" .\n");
    
    HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_CTRL1_REG, &c);  
    if((c & 0x30) == 0x00)
        debug_printf("Input Pixel Clock = Not Repeated.\n");
    else if((c & 0x30) == 0x10)
        debug_printf("Input Pixel Clock = 2x Video Clock. Repeated.\n");
    else if((c & 0x30) == 0x30)
        debug_printf("Input Pixel Clock = 4x Video Clock. Repeated.\n");

    if((c & 0xc0) == 0x00)
        debug_printf("Output Video Clock = Not Divided.\n");
    else if((c & 0xc0) == 0x40)
        debug_printf("Output Video Clock = Divided By 2.\n");
    else if((c & 0xc0) == 0xc0)
        debug_printf("Output Video Clock = Divided By 4.\n");

    if(c & 0x02)
        debug_printf("Output Video Using Rising Edge To Latch Data.\n");
    else
        debug_printf("Output Video Using Falling Edge To Latch Data.\n");

	if ( c & 0x80)
  		debug_printf("Output Video Clock = 2x Video Clock. For YC MUX.\n");

    debug_printf("Input Video Color Depth = "); // KW 07/02/25
    HDMI_RX_ReadI2C_RX0(HDMI_RX_VIDEO_STATUS_REG, &c);  
    c &= 0xF0;
    if(c == 0x00)        
        debug_printf("Legacy Mode.\n");
    else if(c == 0x40)   
        debug_printf("24 Bit Mode.\n");
    else if(c == 0x50)   
        debug_printf("30 Bit Mode.\n");
    else if(c == 0x60)   
        debug_printf("36 Bit Mode.\n");
    else if(c == 0x70)   
        debug_printf("48 Bit Mode.\n");
    else                 
        debug_printf("Unknow 0x44 = 0x%.2x\n",(int)c);

    debug_printf("Input Video Color Space = ");
    HDMI_RX_ReadI2C_RX1(HDMI_RX_AVI_DATA00_REG, &c);  
    c &= 0x60;
    if(c == 0x20)        
        debug_printf("YCbCr4:2:2 .\n");
    else if(c == 0x40)   
        debug_printf("YCbCr4:4:4 .\n");
    else if(c == 0x00)   
        debug_printf("RGB.\n");
    else                 
        debug_printf("Unknow 0x44 = 0x%.2x\n",(int)c);
    
    HDMI_RX_ReadI2C_RX0(HDMI_RX_VID_AOF_REG, &c);
    if((c == 0x00) || (c == 0x80))
        debug_printf("Output Video Format = RGB.\n");
    else if((c == 0x01) || (c == 0x81))
        debug_printf("Output Video Format = YCbCr4:4:4.\n");
    else if (c == 0x07)
        debug_printf("Output Video Format = YCbCr4:2:2 YCMUX.\n");
	else if (c == 0x0f)
        debug_printf("Output Video Format = YCbCr4:2:2 YCMUX+656.\n");
	HDMI_RX_ReadI2C_RX0(HDMI_RX_AUDIO_OUT_CTRL_REG, &c);
    if( c & 0x04)
    {
        debug_printf("Output Audio Interface = I2S.\n");
	}
	else if ( c & 0x02)
        debug_printf("Output Audio Interface = SPDIF.\n");

    HDMI_RX_ReadI2C_RX0(0xa0,&c);
    c = c & 0x01;
    if(!c)
    {
        printk("Input MCLK is selected.");
    }
    else
        printk("External MCLK is selected.");
    
    HDMI_RX_ReadI2C_RX0(0xa2,&c);
    c = c & 0x30;
    debug_printf("Output MCLK Frequence = ");
    
    switch(c)
    {
        case 0x00: 
            printk("128 * Fs.");
            break;
        case 0x10: 
            printk("256 * Fs.");
            break;
        case 0x20:
            printk("384 * Fs.");
            break;
        case 0x30: 
            printk("512 * Fs.");
            break;
        default :         
            printk("Wrong MCLK output.");
            break;
    }
    

    debug_printf("Audio Fs = ");
    HDMI_RX_ReadI2C_RX0(HDMI_RX_AUD_IN_SPDIF_CH_STATUS4_REG, &c); // KW 07/02/25
    c &= 0x0f;
    switch(c)
    {
        case 0x00: 
            printk("44.1 KHz.");
            break;
        case 0x02: 
            printk("48 KHz.");
            break;
        case 0x03: 
            printk("32 KHz.");
            break;
        case 0x08: 
            printk("88.2 KHz.");
            break;        
        case 0x0a: 
            printk("96 KHz.");
            break;
		case 0x0c: 
		      printk("176.4 KHz.");
            break;
        case 0x0e: 
            printk("192 KHz.");
            break;
        default :         
            printk("Wrong MCLK output.");
            break;
    }

    HDMI_RX_ReadI2C_RX1(HDMI_RX_HDCP_STATUS_REG, &c); 
    if(c & HDMI_RX_AUTHEN)
        printk("Authentication is attempted.");
    else
        printk("Authentication is not attempted.");

    for(cl=0;cl<20;cl++)
    {
        HDMI_RX_ReadI2C_RX1(HDMI_RX_HDCP_STATUS_REG, &c); 
        if(c & HDMI_RX_DECRYPT)
            break;
        else
            delay_ms(10);
    }
    if(cl < 20)
        printk("Decryption is active.");
    else
        printk("Decryption is not active.");
             
    printk("***********************************************************************************");
    printk("");
}

/*
void HDMI_RX_AUDIO_BIST()
{
	unsigned char c1;
	
		HDMI_RX_WriteI2C_RX0(HDMI_RX_PORT_SEL_REG, 0x00);
		HDMI_RX_WriteI2C_RX0(HDMI_RX_SYS_PWDN1_REG, 0x00);
	   HDMI_RX_WriteI2C_RX0(HDMI_RX_VID_AOF_REG, 0x03);
	   	HDMI_RX_WriteI2C_RX0(HDMI_RX_ACR_CTRL1_REG, 0x38);				//wen updated
	   	HDMI_RX_WriteI2C_RX0(HDMI_RX_AUD_OUT_I2S_CTRL2_REG, 0x09);
	    HDMI_RX_WriteI2C_RX0(HDMI_RX_ACR_PLL_DEBUG2_REG, 0x93);	 
	    HDMI_RX_WriteI2C_RX0(HDMI_RX_ACR_PLL_DEBUG2_REG, 0x92);
	    HDMI_RX_ReadI2C_RX0(HDMI_RX_ACR_DEBUG_REG, &c1);
	    c1 = c1 & 0x1f;

	
	//#if 1
	if(c1 >=20) 
	{
		   HDMI_RX_WriteI2C_RX0(HDMI_RX_ACR_CTS_SOFTWARE_VALUE1_REG, 0x50);
			HDMI_RX_WriteI2C_RX0(HDMI_RX_ACR_CTS_SOFTWARE_VALUE2_REG, 0x46);
			HDMI_RX_WriteI2C_RX0(HDMI_RX_ACR_CTS_SOFTWARE_VALUE3_REG, 0x00);
    	} 
	else 
	{
       	HDMI_RX_WriteI2C_RX0(HDMI_RX_ACR_CTS_SOFTWARE_VALUE1_REG, 0x9a);
       	HDMI_RX_WriteI2C_RX0(HDMI_RX_ACR_CTS_SOFTWARE_VALUE2_REG, 0x9e);
			HDMI_RX_WriteI2C_RX0(HDMI_RX_ACR_CTS_SOFTWARE_VALUE3_REG, 0x00);
	}
	//#endif
}
*/
void HDMI_RX_Set_Color_Mode(void)
{
    if(!video_format_supported)
    { 
        printk("video format not supported!");
         if (!g_Video_Muted)
        {  
            HDMI_RX_Mute_Video();
        }
        if (!g_Audio_Muted)
        {
            HDMI_RX_Mute_Audio();
        }
        deep_color_set_done = 0;
        if(set_color_mode_counter > 3)
            set_color_mode_counter = 0;
    }
    else 
        set_color_mode_counter = 0;

    if(!deep_color_set_done && (g_Sys_State >= WAIT_VIDEO))
    {
        if((HDMI_RX_Get_Input_Color_Depth() == Hdmi_24bit) ||
            (HDMI_RX_Get_Input_Color_Depth() == Hdmi_30bit) || 
            (HDMI_RX_Get_Input_Color_Depth() == Hdmi_36bit))
        {
            HDMI_RX_WriteI2C_RX0(HDMI_RX_DEEP_COLOR_CTRL_REG , 0x00);
            delay_ms(32);
            printk("RX gets 3 GCP and works in deep color mode!");
        }
        else
        {
            printk("RX doesn't get 3 GCP and trys to judge which color mode it should work in !");
            debug_printf("set_color_mode_counter = %.2x\n", (WORD)set_color_mode_counter);
	     debug_printf("video_format_supported = %.2x\n", (WORD)video_format_supported);
	     switch(set_color_mode_counter)
            {
                case 0:
                    HDMI_RX_Set_Color_Depth(Deep_Color_legacy);
                    delay_ms(32);
                    break;
                case 1:
                    HDMI_RX_Set_Color_Depth(Deep_Color_36bit);
                    delay_ms(32);
                    break;
                case 2:
                    HDMI_RX_Set_Color_Depth(Deep_Color_30bit);
                    delay_ms(32);
                    break;
                case 3:
                    HDMI_RX_Set_Color_Depth(Deep_Color_24bit);
                    delay_ms(32);
                    break;
                default:
                    break;
            }
            set_color_mode_counter++;
        }
        deep_color_set_done = 1;
        video_format_supported = 1;
    }
}


BYTE HDMI_RX_ReadI2C_RX0(BYTE sub_addr, BYTE *rxdata)
{
    BYTE rc;
    rc = i2c_read_p0_reg(sub_addr,rxdata);
    if(rc) { printk("**** HDMI_RX RX0 Read Error"); }
    return	rc;
}

BYTE HDMI_RX_WriteI2C_RX0(BYTE sub_addr, BYTE txdata)
{
    BYTE rc;
    rc = i2c_write_p0_reg(sub_addr,txdata);
    if(rc) { printk("**** HDMI_RX RX0 Write Error"); }
    return	rc;	
}

BYTE HDMI_RX_ReadI2C_RX1(BYTE sub_addr,  BYTE *rxdata)
{
    BYTE rc;
    rc = i2c_read_p1_reg(sub_addr,rxdata);
    if(rc) { printk("**** HDMI_RX RX1 Read Error"); }
    return	rc;
}

BYTE HDMI_RX_WriteI2C_RX1(BYTE sub_addr,  BYTE txdata)
{
    BYTE	rc;
    rc = i2c_write_p1_reg(sub_addr,txdata);
    if(rc) { printk("**** HDMI_RX RX1 Write Error"); }
    return	rc;
}



