
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include "ANX7150_Sys7150.h"
#include "ANX7150_System_Config.h"
#include "ANX7150_i2c_intf.h"
#ifdef ANX8770_USED
#include "HDMI_RX_DRV.h"   
#endif
#include "ANX7150.h"
#include <video/kg2.h>

extern struct i2c_client *g_client;
bool EX0;
char timer_done=1;
BYTE timer_slot =-1;//BYTE timer_slot =0;


//code struct ANX7150_video_timingtype ANX7150_video_timingtype_table =
struct ANX7150_video_timingtype ANX7150_video_timingtype_table =  
{
     //640x480p-60hz
    {0x20/*H_RES_LOW*/, 0x03/*H_RES_HIGH*/,0x80 /*ACT_PIX_LOW*/,0x02 /*ACT_PIX_HIGH*/, 
    0x60/*HSYNC_WIDTH_LOW*/,0x00 /*HSYNC_WIDTH_HIGH*/,0x30 /*H_BP_LOW*/,0x00 /*H_BP_HIGH*/, 
    0xe0/*ACT_LINE_LOW*/, 0x01/*ACT_LINE_HIGH*/,0x02 /*VSYNC_WIDTH*/, 0x21/*V_BP_LINE*/, 
    0x0a/*V_FP_LINE*/,0x10 /*H_FP_LOW*/, 0x00/*H_FP_HIGH*/, 
    ANX7150_Progressive, ANX7150_Neg_Hsync_pol, ANX7150_Neg_Vsync_pol},
    //720x480p-60hz
    {0x5a/*H_RES_LOW*/,0x03 /*H_RES_HIGH*/,0xd0/*ACT_PIX_LOW*/, 0x02/*ACT_PIX_HIGH*/, 
    0x3e/*HSYNC_WIDTH_LOW*/, 0x00/*HSYNC_WIDTH_HIGH*/, 0x3c/*H_BP_LOW*/, 0x00/*H_BP_HIGH*/, 
    0xe0/*ACT_LINE_LOW*/, 0x01/*ACT_LINE_HIGH*/, 0x06/*VSYNC_WIDTH*/, 0x1e/*V_BP_LINE*/, 
    0x09/*V_FP_LINE*/, 0x10/*H_FP_LOW*/, 0x00/*H_FP_HIGH*/, 
    ANX7150_Progressive, ANX7150_Neg_Hsync_pol, ANX7150_Neg_Vsync_pol},
    //720p-60hz
    {0x72/*H_RES_LOW*/, 0x06/*H_RES_HIGH*/, 0x00/*ACT_PIX_LOW*/, 0x05/*ACT_PIX_HIGH*/, 
    0x28/*HSYNC_WIDTH_LOW*/, 0x00/*HSYNC_WIDTH_HIGH*/, 0xdc/*H_BP_LOW*/, 0x00/*H_BP_HIGH*/, 
    0xd0/*ACT_LINE_LOW*/, 0x02/*ACT_LINE_HIGH*/, 0x05/*VSYNC_WIDTH*/, 0x14/*V_BP_LINE*/, 
    0x05/*V_FP_LINE*/, 0x6e/*H_FP_LOW*/, 0x00/*H_FP_HIGH*/, 
    ANX7150_Progressive, ANX7150_Pos_Hsync_pol, ANX7150_Pos_Vsync_pol},
    //1080i-60hz
    {0x98/*H_RES_LOW*/, 0x08/*H_RES_HIGH*/, 0x80/*ACT_PIX_LOW*/, 0x07/*ACT_PIX_HIGH*/, 
    0x2c/*HSYNC_WIDTH_LOW*/, 0x00/*HSYNC_WIDTH_HIGH*/, 0x94/*H_BP_LOW*/, 0x00/*H_BP_HIGH*/, 
    0x38/*ACT_LINE_LOW*/, 0x04/*ACT_LINE_HIGH*/, 0x05/*VSYNC_WIDTH*/, 0x0f/*V_BP_LINE*/, 
    0x02/*V_FP_LINE*/, 0x58/*H_FP_LOW*/, 0x00/*H_FP_HIGH*/, 
    ANX7150_Interlace, ANX7150_Pos_Hsync_pol, ANX7150_Pos_Vsync_pol},
    //720x480i-60hz
    {0x5a/*H_RES_LOW*/,0x03 /*H_RES_HIGH*/,0xd0/*ACT_PIX_LOW*/, 0x02/*ACT_PIX_HIGH*/, 
    0x3e/*HSYNC_WIDTH_LOW*/, 0x00/*HSYNC_WIDTH_HIGH*/, 0x39/*H_BP_LOW*/, 0x00/*H_BP_HIGH*/, 
    0xe0/*ACT_LINE_LOW*/, 0x01/*ACT_LINE_HIGH*/, 0x03/*VSYNC_WIDTH*/, 0x0f/*V_BP_LINE*/, 
    0x04/*V_FP_LINE*/, 0x13/*H_FP_LOW*/, 0x00/*H_FP_HIGH*/, 
    ANX7150_Interlace, ANX7150_Neg_Hsync_pol, ANX7150_Neg_Vsync_pol},											//update
    //576p-50hz
    {0x60/*H_RES_LOW*/,0x03 /*H_RES_HIGH*/,0xd0 /*ACT_PIX_LOW*/, 0x02/*ACT_PIX_HIGH*/, 
    0x40/*HSYNC_WIDTH_LOW*/, 0x00/*HSYNC_WIDTH_HIGH*/, 0x44/*H_BP_LOW*/,0x00 /*H_BP_HIGH*/, 
    0x40/*ACT_LINE_LOW*/, 0x02/*ACT_LINE_HIGH*/, 0x05/*VSYNC_WIDTH*/, 0x27/*V_BP_LINE*/, 
    0x05/*V_FP_LINE*/, 0x0c/*H_FP_LOW*/, 0x00/*H_FP_HIGH*/, 
    ANX7150_Progressive, ANX7150_Neg_Hsync_pol, ANX7150_Neg_Vsync_pol},
    //720p-50hz
    {0xbc/*H_RES_LOW*/, 0x07/*H_RES_HIGH*/, 0x00/*ACT_PIX_LOW*/, 0x05/*ACT_PIX_HIGH*/, 
    0x28/*HSYNC_WIDTH_LOW*/, 0x00/*HSYNC_WIDTH_HIGH*/, 0xdc/*H_BP_LOW*/, 0x00/*H_BP_HIGH*/, 
    0xd0/*ACT_LINE_LOW*/, 0x02/*ACT_LINE_HIGH*/, 0x05/*VSYNC_WIDTH*/, 0x14/*V_BP_LINE*/, 
    0x05/*V_FP_LINE*/, 0xb8/*H_FP_LOW*/, 0x01/*H_FP_HIGH*/, 
    ANX7150_Progressive, ANX7150_Pos_Hsync_pol, ANX7150_Pos_Vsync_pol},
    //1080i-50hz
    {0x50/*H_RES_LOW*/, 0x0a/*H_RES_HIGH*/, 0x80/*ACT_PIX_LOW*/, 0x07/*ACT_PIX_HIGH*/, 
    0x2c/*HSYNC_WIDTH_LOW*/, 0x00/*HSYNC_WIDTH_HIGH*/, 0x94/*H_BP_LOW*/, 0x00/*H_BP_HIGH*/, 
    0x38/*ACT_LINE_LOW*/, 0x04/*ACT_LINE_HIGH*/, 0x05/*VSYNC_WIDTH*/, 0x0f/*V_BP_LINE*/, 
    0x02/*V_FP_LINE*/, 0x10/*H_FP_LOW*/, 0x02/*H_FP_HIGH*/, 
    ANX7150_Interlace, ANX7150_Pos_Hsync_pol, ANX7150_Pos_Vsync_pol},
     //576i-50hz
    {0x60/*H_RES_LOW*/,0x03 /*H_RES_HIGH*/,0xd0 /*ACT_PIX_LOW*/, 0x02/*ACT_PIX_HIGH*/, 
    0x3f/*HSYNC_WIDTH_LOW*/, 0x00/*HSYNC_WIDTH_HIGH*/, 0x45/*H_BP_LOW*/,0x00 /*H_BP_HIGH*/, 
    0x40/*ACT_LINE_LOW*/,0x02 /*ACT_LINE_HIGH*/, 0x03/*VSYNC_WIDTH*/, 0x13/*V_BP_LINE*/, 
    0x02/*V_FP_LINE*/, 0x0c/*H_FP_LOW*/, 0x00/*H_FP_HIGH*/, 
    ANX7150_Interlace, ANX7150_Neg_Hsync_pol, ANX7150_Neg_Vsync_pol},
};
//#endif
//code struct Bist_Video_Format bist_demo[] = {
struct Bist_Video_Format bist_demo[] = {  
//h_total  h_act   v_total v_act   h_fp  h_wid h_bp v_fp v_wid v_bp  h_pol v_pol    I_P  mode
    {800,    640,    525,    480,     16,     96,    48,    10,    2,    33,     1,        1,       0,      0},//640x480p@60
    {1650,   1280,  750,    720,    110,   40,    220,   5,      5,    20,     0,        0,       0,      1},//1280x720p@60
    {2200,   1920,  1125,  1080,    88,    44,    148,   2,     5,    15,     0,        0,        1,     2},//1920x1080i@60 
    {2200,   1920,  1125,  1080,    88,    44,     148,   4,    5,    36,     0,        0,        0,     2},//1920x1080p@60
    {1980,    1280,  750,    720,     440,  40,     220,   5,    5,    20,     0,        0,        0,     0},//1280x720p@50
    {2640,    1920,  1125,  1080,    528,  44,     148,   2,    5,   15,      0,        0,        1,     1},//1920x1080i@50
    {864,    720,     625,    576,    12,     64,    68,     5,     5,    39,     1,         1,       0,      0},//720x576p@50
    {864,    720,     625,    576,    12,     63,    69,     2,     3,    19,     1,         1,       1,      1},//720x576i@50
    {858,    720,     525,    480,    19,     63,    57,     4,     3,    15,     1,         1,       1,      2},//720x480i@60
    {2080,  1920,   1235,   1200,  88,     44,   28,      4,     5,    26,     0,         0,       0,      0},
};
BYTE ANX7150_EDID_Buf[256];
BYTE ANX7150_avi_data[19];//, ANX7150_avi_checksum;
BYTE ANX7150_system_state;
BYTE spdif_error_cnt = 0x00;
BYTE misc_reset_needed;
BYTE ANX7150_stdaddr,ANX7150_stdreg,ANX7150_ext_block_num;
BYTE ANX7150_svd_length,ANX7150_sau_length;
BYTE ANX7150_edid_dtd[18];
WORD ANX7150_edid_length;
ANX7150_edid_result_4_system ANX7150_edid_result;

BIT ANX7150_ddc_fifo_full;
BIT ANX7150_ddc_progress;
BIT ANX7150_hdcp_auth_en;
//BIT ANX7150_bksv_ready; //replace by srm_checked xy 01.09
BIT ANX7150_HDCP_enable;
BIT ANX7150_ksv_srm_pass;
BYTE ANX7150_hdcp_bcaps;
BYTE ANX7150_hdcp_bstatus[2];
BIT ANX7150_srm_checked;
BIT ANX7150_hdcp_auth_pass;
BIT ANX7150_avmute_enable;
BIT ANX7150_send_blue_screen;
BIT ANX7150_hdcp_encryption;
BIT ANX7150_hdcp_init_done;
BIT ANX7150_hdcp_wait_100ms_needed;
BIT ANX7150_auth_fully_pass;
BIT ANX7150_parse_edid_done;//060714 XY
//BIT testen;
//BYTE ANX7150_avi_data[19], ANX7150_avi_checksum;
BYTE ANX7150_hdcp_auth_fail_counter ;

BYTE ANX7150_video_format_config;
BIT ANX7150_emb_sync_mode,ANX7150_de_gen_en,ANX7150_demux_yc_en,ANX7150_ddr_bus_mode;
BIT ANX7150_ddr_edge,ANX7150_ycmux_BIT_sel;
BIT ANX7150_system_config_done;
BYTE ANX7150_RGBorYCbCr; //modified by zy 060814   
BYTE ANX7150_in_pix_rpt,ANX7150_tx_pix_rpt;
BYTE ANX7150_in_pix_rpt_bkp,ANX7150_tx_pix_rpt_bkp;
BYTE ANX7150_video_timing_id;
BIT ANX7150_pix_rpt_set_by_sys;
BYTE ANX7150_video_timing_parameter[18];
BYTE switch_value_sw_backup,switch_value_pc_backup;
BYTE switch_value,bist_switch_value_pc;
BIT ANX7150_new_csc,ANX7150_new_vid_id,ANX7150_new_HW_interface;
BIT ANX7150_INT_Done;

audio_config_struct s_ANX7150_audio_config;
config_packets s_ANX7150_packet_config;

BYTE FREQ_MCLK;         //0X72:0X50 bit2:0
                                //000b:Fm = 128*Fs 
                                //001b:Fm = 256*Fs 
                                //010b:Fm = 384*Fs 
                                //011b:Fm = 512*Fs 
BYTE ANX7150_audio_clock_edge;
int ANX7150_app_hold_system_config;
int ANX7150_shutdown;

                                
void ANX7150_Task()
{
	if(!EXT_INT_EN)
	{
		ANX7150_i2c_read_p0_reg(ANX7150_INTR1_STATUS_REG, &int_s1);
		ANX7150_i2c_write_p0_reg(ANX7150_INTR1_STATUS_REG, int_s1);

		ANX7150_i2c_read_p0_reg(ANX7150_INTR2_STATUS_REG, &int_s2);
		ANX7150_i2c_write_p0_reg(ANX7150_INTR2_STATUS_REG, int_s2);

		ANX7150_i2c_read_p0_reg(ANX7150_INTR3_STATUS_REG, &int_s3);
		ANX7150_i2c_write_p0_reg(ANX7150_INTR3_STATUS_REG, int_s3);

		ANX7150_INT_Done = 1;		
	}
	
    if(ANX7150_INT_Done)
	{
                ANX7150_DEBUG("### int_s1=%x\n",int_s1);
		ANX7150_Interrupt_Process();
		ANX7150_INT_Done = 0;
	}
    ANX7150_Timer_Process ();
	
}

void ANX7150_Timer_Slot1() 
{
    BYTE c;

    ANX7150_DEBUG("###ANX7150_Timer_Slot1\n");
    if(ANX7150_system_state == ANX7150_INITIAL)
    {
        ANX7150_DEBUG("###ANX7150_INITIAL at timer slot1\n");
        return;
    }
    		
    //while(ANX7150_system_state == ANX7150_WAIT_HOTPLUG)
    if(ANX7150_system_state == ANX7150_WAIT_HOTPLUG)
	{
                ANX7150_DEBUG("###ANX7150_WAIT_HOTPLUG at timer slot1\n");
		ANX7150_i2c_read_p0_reg(ANX7150_INTR_STATE_REG, &c);
                ANX7150_DEBUG("###0x40=%x\n",c);
        	if(c & 0x01)
        	{
				ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL3_REG, &c);
		        ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL3_REG, c | 0x01);//power up all, 090630
				//HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_PWDN2_REG, &c);	//en output of 8770,
			    //HDMI_RX_WriteI2C_RX0(HDMI_RX_SYS_PWDN2_REG, c & 0xf8);
						
	            ANX7150_Hotplug_Change_Interrupt();
				 ANX7150_DEBUG("###HPD Det at timer slot1\n");
			}
			//else
			//{
			//	msleep(1000);
			//}
	msleep(500);
	}
	
    
    if(ANX7150_system_state == ANX7150_READ_PARSE_EDID)
    {

        ANX7150_i2c_read_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, &c);
        ANX7150_DEBUG("ANX7150_WAIT_HOTPLUG--->111111111ANX7150_TMDS_CLKCH_CONFIG_REG=%x\n",c);	
        ANX7150_i2c_write_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, c |0x80);
        ANX7150_DEBUG("ANX7150_WAIT_HOTPLUG--->222222222ANX7150_TMDS_CLKCH_CONFIG_REG=%x\n",c);

//        ANX7150_DEBUG("###ANX7150_READ_PARSE_EDID\n");
        ANX7150_i2c_read_p0_reg(ANX7150_HDCP_CTRL0_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL0_REG, (c &(~ANX7150_HDCP_CTRL0_HW_AUTHEN)));
        ANX7150_hdcp_auth_en = 0;

        ANX7150_RST_DDCChannel();
    //enable video input
    ANX7150_i2c_read_p0_reg(ANX7150_VID_CTRL_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_VID_CTRL_REG, c | ANX7150_VID_CTRL_IN_EN);  
    ANX7150_DEBUG("Video pre-onfigure OK~~!\n");
        
        ANX7150_Parse_EDID();
	 ANX7150_parse_edid_done = 1;

        ANX7150_Set_System_State(ANX7150_WAIT_RX_SENSE);//060819
    }

    if(ANX7150_system_state == ANX7150_WAIT_RX_SENSE)
    {
        ANX7150_i2c_read_p0_reg(ANX7150_SYS_STATE_REG, &c);
        if(c & ANX7150_SYS_STATE_RSV_DET)
        {
            ANX7150_DEBUG("###Receiver sense active.\n");
            ANX7150_Set_System_State(ANX7150_CONFIG_VIDEO);//060819
        }
        else
        {
            ANX7150_Set_System_State(ANX7150_CONFIG_VIDEO);//060819 
        } 
    }
}

void ANX7150_Timer_Slot2(void)
{
    ANX7150_DEBUG("###ANX7150_Timer_Slot2\n");
    if(ANX7150_system_state == ANX7150_CONFIG_VIDEO)
    {
        ANX7150_Config_Video();
    }

    if(ANX7150_system_state == ANX7150_CONFIG_AUDIO)
    {
        ANX7150_Config_Audio();
    }

    if(ANX7150_system_state == ANX7150_CONFIG_PACKETS)
    {
        ANX7150_Config_Packet();
    }
}    
 
void ANX7150_Timer_Slot3(void)
{
     ANX7150_DEBUG("###ANX7150_Timer_Slot3\n");
    if(ANX7150_system_state == ANX7150_HDCP_AUTHENTICATION)
    {
        ANX7150_HDCP_Process();
    }
    
    if(ANX7150_system_state == ANX7150_PLAY_BACK)
    {            
        ANX7150_PLAYBACK_Process();

    }
}    

void ANX7150_Timer_Slot4() 
{
    BYTE c;
    ANX7150_i2c_read_p0_reg(0x10, &c);
    ANX7150_DEBUG("###timer_slot4,[0x10]=0x%x ----\n",c);
}

void ANX7150_Variable_Initial() 
{
    BYTE i;
    ANX7150_DEBUG("+++ANX7150_Variable_Initial+++\n");   
    ANX7150_Set_System_State(ANX7150_INITIAL);
    ANX7150_hdcp_auth_en = 0;
    ANX7150_ksv_srm_pass =0;
    ANX7150_srm_checked = 0;
    ANX7150_hdcp_auth_pass = 0;
    ANX7150_avmute_enable = 1;
    ANX7150_hdcp_auth_fail_counter =0;
    ANX7150_hdcp_encryption = 0;
    ANX7150_send_blue_screen = 0;
    ANX7150_hdcp_init_done = 0;
    ANX7150_hdcp_wait_100ms_needed = 1;
    ANX7150_auth_fully_pass = 0;
    //********************for video config**************
    ANX7150_video_timing_id = 0;
    ANX7150_in_pix_rpt = 0;
    ANX7150_tx_pix_rpt = 0;
    ANX7150_new_csc = 0;
    ANX7150_new_vid_id = 0;
    ANX7150_new_HW_interface = 0;
    //********************end of video config*********

    //********************for edid parse***********
    ANX7150_edid_result.is_HDMI = 0;
    ANX7150_edid_result.ycbcr422_supported = 0;
    ANX7150_edid_result.ycbcr444_supported = 0;
    ANX7150_edid_result.supported_720p_60Hz = 0;
    ANX7150_edid_result.supported_720p_50Hz = 0;
    ANX7150_edid_result.supported_576p_50Hz = 0;
    ANX7150_edid_result.supported_576i_50Hz = 0;
    ANX7150_edid_result.supported_1080i_60Hz = 0;
    ANX7150_edid_result.supported_1080i_50Hz = 0;
    ANX7150_edid_result.supported_640x480p_60Hz = 0;
    ANX7150_edid_result.supported_720x480p_60Hz = 0;
    ANX7150_edid_result.supported_720x480i_60Hz = 0;
    ANX7150_edid_result.edid_errcode = 0;
    ANX7150_edid_result.SpeakerFormat = 0;
    for(i = 0; i < 8; i ++)
    {
        ANX7150_edid_result.AudioChannel[i] = 0;
        ANX7150_edid_result.AudioFormat[i] = 0;
        ANX7150_edid_result.AudioFs[i] = 0;
        ANX7150_edid_result.AudioLength[i] = 0;
    }
    //********************end of edid**************

    s_ANX7150_packet_config.packets_need_config = 0x03;   //new avi infoframe
    s_ANX7150_packet_config.avi_info.type = 0x82;
    s_ANX7150_packet_config.avi_info.version = 0x02;
    s_ANX7150_packet_config.avi_info.length = 0x0d;
    s_ANX7150_packet_config.avi_info.pb_byte[1] = 0x21;//YCbCr422
    s_ANX7150_packet_config.avi_info.pb_byte[2] = 0x08; 
    s_ANX7150_packet_config.avi_info.pb_byte[3] = 0x00; 
    s_ANX7150_packet_config.avi_info.pb_byte[4] = 0x00; 
    s_ANX7150_packet_config.avi_info.pb_byte[5] = 0x00; 
    s_ANX7150_packet_config.avi_info.pb_byte[6] = 0x00; 
    s_ANX7150_packet_config.avi_info.pb_byte[7] = 0x00; 
    s_ANX7150_packet_config.avi_info.pb_byte[8] = 0x00; 
    s_ANX7150_packet_config.avi_info.pb_byte[9] = 0x00; 
    s_ANX7150_packet_config.avi_info.pb_byte[10] = 0x00; 
    s_ANX7150_packet_config.avi_info.pb_byte[11] = 0x00; 
    s_ANX7150_packet_config.avi_info.pb_byte[12] = 0x00; 
    s_ANX7150_packet_config.avi_info.pb_byte[13] = 0x00; 

    // audio infoframe
    s_ANX7150_packet_config.audio_info.type = 0x84;
    s_ANX7150_packet_config.audio_info.version = 0x01;
    s_ANX7150_packet_config.audio_info.length = 0x0a;
    s_ANX7150_packet_config.audio_info.pb_byte[1] = 0x00;  //zy 061123 for ATC
    s_ANX7150_packet_config.audio_info.pb_byte[2] = 0x00;
    s_ANX7150_packet_config.audio_info.pb_byte[3] = 0x00;
    s_ANX7150_packet_config.audio_info.pb_byte[4] = 0x00;
    s_ANX7150_packet_config.audio_info.pb_byte[5] = 0x00;
    s_ANX7150_packet_config.audio_info.pb_byte[6] = 0x00;
    s_ANX7150_packet_config.audio_info.pb_byte[7] = 0x00;
    s_ANX7150_packet_config.audio_info.pb_byte[8] = 0x00;
    s_ANX7150_packet_config.audio_info.pb_byte[9] = 0x00;
    s_ANX7150_packet_config.audio_info.pb_byte[10] = 0x00;

    ANX7150_Reset_AVI(); 
	ANX7150_INT_Done = 0;
    ANX7150_DEBUG("---ANX7150_Variable_Initial---\n");  
}

void ANX7150_HW_Interface_Variable_Initial()
{
    BYTE c;
    ANX7150_DEBUG("+++ANX7150_HW_Interface_Variable_Initial+++\n");

		if (!BIST_EN ) //no bist
		{
			if(QFN48_EN)
			{
/*
				c = P2_5;
				c = (c<<1) | P2_6;
				debug_printf("QFN48: video interface###c= %.2x\n", (unsigned int)c);

				switch (c)
				{
					case 0:
						ANX7150_video_format_config = 0x03;//yc mux, 601, 15-8,3-0
						//HDMI_RX_VID_Output_Fmt(YCbCr422_YCMUX); //yc mux 601
						break;
					case 1:
						ANX7150_video_format_config = 0x05;//yc mux, 656, 15-8,3-0
						//HDMI_RX_VID_Output_Fmt(YCbCr422_656_YCMUX); //yc mux 656
						break;
					case 2:
						ANX7150_video_format_config = 0x07;//DDR yc422, 601, 
						break;
					case 3:
						ANX7150_video_format_config = 0x08;//DDR yc422, 656, 
						break;
		      			default: 
				                break;	

				}
*/							
				    ANX7150_RGBorYCbCr = 0x01;
			}
			else
			{
			    ANX7150_DEBUG("TQFP80/GBA81: video interface\n");
			    ANX7150_video_format_config = 0x00;	
			    ANX7150_RGBorYCbCr = 0x00;
			}
		}

//		if (!BIST_EN ) //no bist
//		{
//			{
//				ANX7150_DEBUG("TQFP80/GBA81: video interface\n");
//			    ANX7150_video_format_config = 0x00;	
//			    ANX7150_RGBorYCbCr = 0x00;
//			}
//		}

//    ANX7150_RGBorYCbCr = ANX7150_INPUT_COLORSPACE;
    ANX7150_ddr_edge = ANX7150_IDCK_EDGE_DDR;

    c = 0;
    c = (ANX7150_I2S_CH0_ENABLE << 2) | (ANX7150_I2S_CH1_ENABLE << 3) |
        (ANX7150_I2S_CH2_ENABLE << 4) | (ANX7150_I2S_CH3_ENABLE << 5);
    s_ANX7150_audio_config.audio_type = ANX7150_AUD_HW_INTERFACE;     // input I2S
    s_ANX7150_audio_config.down_sample = 0x00;
    s_ANX7150_audio_config.i2s_config.audio_channel = c;//0x04;
    s_ANX7150_audio_config.i2s_config.Channel_status1 =0x00;
    s_ANX7150_audio_config.i2s_config.Channel_status1 = 0x00;
    s_ANX7150_audio_config.i2s_config.Channel_status2 = 0x00;
    s_ANX7150_audio_config.i2s_config.Channel_status3 = 0x00;
    s_ANX7150_audio_config.i2s_config.Channel_status4 = 0x00;//0x02;//48k
    s_ANX7150_audio_config.i2s_config.Channel_status5 = ANX7150_I2S_WORD_LENGTH;//0x0b;
	s_ANX7150_audio_config.audio_layout = 0x00;

    c = (ANX7150_I2S_SHIFT_CTRL << 3) | (ANX7150_I2S_DIR_CTRL << 2)  |
          (ANX7150_I2S_WS_POL << 1) | ANX7150_I2S_JUST_CTRL;  
    s_ANX7150_audio_config.i2s_config.i2s_format = c;//0x00;

    FREQ_MCLK = ANX7150_MCLK_Fs_RELATION;//set the relation of MCLK and WS 
    ANX7150_audio_clock_edge = ANX7150_AUD_CLK_EDGE;

    ANX7150_DEBUG("---ANX7150_HW_Interface_Variable_Initial---\n");	
}

void ANX7150_Hotplug_Change_Interrupt(void) 
{
    BYTE c;
    ANX7150_i2c_read_p0_reg(ANX7150_SYS_STATE_REG, &c);
    if (c & ANX7150_SYS_STATE_HP) 
    {
        ANX7150_DEBUG("##ANX7150 HotPlug detected.\n");
        //disable audio & video & hdcp & TMDS and init    begin
        ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, c & (~ANX7150_HDMI_AUDCTRL1_IN_EN));

        ANX7150_i2c_read_p0_reg(ANX7150_VID_CTRL_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_VID_CTRL_REG, c & (~ANX7150_VID_CTRL_IN_EN));



        ANX7150_DEBUG("ANX7150_Hotplug_Change_Interrupt--->&&&&&& 0xef\n");
        ANX7150_i2c_read_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, &c);
        ANX7150_DEBUG("ANX7150_Hotplug_Change_Interrupt--->ANX7150_TMDS_CLKCH_CONFIG_REG=%x\n",c);
        ANX7150_i2c_write_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, c & (~ANX7150_TMDS_CLKCH_MUTE));

        ANX7150_i2c_read_p0_reg(ANX7150_HDCP_CTRL0_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL0_REG, (c & (~ANX7150_HDCP_CTRL0_HW_AUTHEN)));

        ANX7150_Variable_Initial();
        //disable video & audio & hdcp & TMDS and init    end

        ANX7150_Set_System_State(ANX7150_READ_PARSE_EDID);//060819
        //Power on chip and select DVI mode
        ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL1_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL1_REG, c | 0x01); 
        ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL1_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL1_REG, c & 0xfd); 
        //ANX7150_DEBUG("ANX7150 is set to DVI mode\n");
        ANX7150_RST_DDCChannel();      
#if 1
		 //Initial Interrupt 
	    // disable video/audio CLK,Format change and before config video. 060713 xy
	    ANX7150_i2c_write_p0_reg(ANX7150_INTR1_MASK_REG, 0x04);//3
	    ANX7150_i2c_write_p0_reg(ANX7150_INTR2_MASK_REG, 0x00);
	    ANX7150_i2c_write_p0_reg(ANX7150_INTR3_MASK_REG, 0x00);

		ANX7150_i2c_read_p0_reg(ANX7150_INTR1_STATUS_REG, &c);
		ANX7150_i2c_write_p0_reg(ANX7150_INTR1_STATUS_REG, c);

		ANX7150_i2c_read_p0_reg(ANX7150_INTR2_STATUS_REG, &c);
		ANX7150_i2c_write_p0_reg(ANX7150_INTR2_STATUS_REG, c);

		ANX7150_i2c_read_p0_reg(ANX7150_INTR3_STATUS_REG, &c);
		ANX7150_i2c_write_p0_reg(ANX7150_INTR3_STATUS_REG, c);

		ANX7150_i2c_write_p0_reg(ANX7150_INTR_CTRL_REG, 0x09);

		if(EXT_INT_EN)
		{
	        ANX7150_DEBUG("enable ext int");
			 EX0 = 1;
		}
#endif		 
    }  
    else 
    { 
    	 //ANX7150_DEBUG("disable ext int");
        //EX0 = 0;
//HDCP CTS
    	 ANX7150_Variable_Initial();   
    	 ANX7150_HW_Interface_Variable_Initial();  
    	 ANX7150_Hardware_Initial();   
    	 ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL0_REG, 0x00);   
//HDCP CTS
        ANX7150_hdcp_wait_100ms_needed = 1;
        ANX7150_auth_fully_pass = 0;
		 #if 0//#if ANX8770_USED
		 HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_PWDN2_REG, &c);	//dis output of 8770,090630
	    HDMI_RX_WriteI2C_RX0(HDMI_RX_SYS_PWDN2_REG, c | 0x07);
		#endif
    }
    // clear ANX7150_parse_edid_done & ANX7150_system_config_done
    ANX7150_parse_edid_done = 0;
    ANX7150_system_config_done = 0;
    ANX7150_srm_checked = 0;
}


/*
void ANX7150_Video_Clock_Change_Interrupt(void) 
{
    BYTE c;
    if((ANX7150_system_state != ANX7150_INITIAL) && (ANX7150_system_state != ANX7150_WAIT_HOTPLUG)
     &&  (ANX7150_system_state != ANX7150_READ_PARSE_EDID)
     &&  (ANX7150_system_state != ANX7150_WAIT_RX_SENSE)) 
    {
         ANX7150_Set_AVMute(); 
         //stop HDCP and reset DDC
         ANX7150_i2c_read_p0_reg(ANX7150_HDCP_CTRL0_REG, &c);
         ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL0_REG, (c & (~ANX7150_HDCP_CTRL0_HW_AUTHEN)));
         ANX7150_RST_DDCChannel(); 
         ANX7150_DEBUG("after video clock change int ");
         ANX7150_Set_System_State(ANX7150_CONFIG_VIDEO);
    }
    //when clock change, clear this reg to avoid error in package config
    ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL1_REG, 0x00);
    ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL2_REG, 0x00);
    //xy 11.06 when clock change, need system config again
    ANX7150_system_config_done = 0;
}
*/

void ANX7150_Video_Format_Change_Interrupt(void) 
{
    BYTE c;
    if((ANX7150_system_state != ANX7150_INITIAL) 
     && (ANX7150_system_state != ANX7150_WAIT_HOTPLUG)
     &&  (ANX7150_system_state != ANX7150_READ_PARSE_EDID)
     && (ANX7150_system_state != ANX7150_WAIT_RX_SENSE)) 
    {
        ANX7150_DEBUG("after video format change int ");
        ANX7150_Set_AVMute();
         //stop HDCP and reset DDC
        ANX7150_i2c_read_p0_reg(ANX7150_HDCP_CTRL0_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL0_REG, (c & (~ANX7150_HDCP_CTRL0_HW_AUTHEN)));
        ANX7150_RST_DDCChannel(); 
        ANX7150_Set_System_State(ANX7150_CONFIG_VIDEO);
    } 
    //when format change, clear this reg to avoid error in package config
    ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL1_REG, 0x00);
    ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL2_REG, 0x00);
    //xy 11.06 when format change, need system config again
    ANX7150_system_config_done = 0;
}

/*
void ANX7150_Audio_CLK_Change_Interrupt(void) 
{

    BYTE c;
    if((ANX7150_system_state != ANX7150_INITIAL) 
        && (ANX7150_system_state != ANX7150_WAIT_HOTPLUG) 
        && (ANX7150_system_state != ANX7150_READ_PARSE_EDID)
        && (ANX7150_system_state != ANX7150_WAIT_RX_SENSE)
        && (ANX7150_system_state != ANX7150_CONFIG_VIDEO))
    {
        ANX7150_DEBUG("ANX7150: audio clock changed interrupt,disable audio.");
        // disable audio 
        ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG,&c);
        c &= ~ANX7150_HDMI_AUDCTRL1_IN_EN;
        ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG,c);
        ANX7150_Set_System_State(ANX7150_CONFIG_AUDIO);
    }
    
    //xy 11.06 when format change, need system config again
    ANX7150_system_config_done = 0;
}
*/

void ANX7150_Set_AVMute(void) 
{
    BYTE c;

    ANX7150_i2c_write_p1_reg(ANX7150_GNRL_CTRL_PKT_REG, 0x01);
    ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL1_REG, &c);
    ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL1_REG, (c | 0x0c));
    ANX7150_avmute_enable = 1;
    ANX7150_DEBUG("@@@@@@@@@@@@@@@@@@@@ANX7150_Set_AVMute\n");

}

void ANX7150_Clear_AVMute(void) 
{
    BYTE c;
    ANX7150_i2c_write_p1_reg(ANX7150_GNRL_CTRL_PKT_REG, 0x02);
    ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL1_REG, &c);
    ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL1_REG, (c | 0x0c));
    ANX7150_avmute_enable = 0;
    ANX7150_DEBUG("@@@@@@@@@@@@@@@@@@@@ANX7150_Clear_AVMute\n");

}

void ANX7150_Auth_Done_Interrupt() 
{
    BYTE c;
    ANX7150_i2c_read_p0_reg(ANX7150_HDCP_STATUS_REG, &c);
    if(c & ANX7150_HDCP_STATUS_AUTH_PASS) 
    {
        ANX7150_DEBUG("ANX7150_Authentication pass in Auth_Done\n");
        ANX7150_Blue_Screen_Disable();
        ANX7150_hdcp_auth_pass = 1;
        ANX7150_hdcp_auth_fail_counter = 0;
    } 
    else 
    {
        ANX7150_DEBUG("ANX7150_Authentication failed\n");
        ANX7150_hdcp_wait_100ms_needed = 1;
        ANX7150_auth_fully_pass = 0;
        ANX7150_hdcp_auth_pass = 0;
        ANX7150_hdcp_auth_fail_counter ++;
        if(ANX7150_hdcp_auth_fail_counter >= ANX7150_HDCP_FAIL_THRESHOLD) 
        {
            ANX7150_hdcp_auth_fail_counter = 0;
            //ANX7150_bksv_ready = 0;
            // TODO: Reset link; 
            ANX7150_Blue_Screen_Enable();
            ANX7150_HDCP_Encryption_Disable();
            //disable audio
            ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, c & (~ANX7150_HDMI_AUDCTRL1_IN_EN));
        }
    }
}

void ANX7150_Auth_Change_Interrupt() 
{
    BYTE c;
    ANX7150_i2c_read_p0_reg(ANX7150_HDCP_STATUS_REG, &c);
    if(c & ANX7150_HDCP_STATUS_AUTH_PASS) 
    {
        ANX7150_hdcp_auth_pass = 1;
        ANX7150_DEBUG("ANX7150_Authentication pass in Auth_Change************************\n");
    } 
    else 
    {
			ANX7150_Set_AVMute();
         ANX7150_DEBUG("ANX7150_Authentication failed_by_Auth_change\n");
         ANX7150_hdcp_auth_pass = 0;
         ANX7150_hdcp_wait_100ms_needed = 1;
         ANX7150_auth_fully_pass = 0;
		  ANX7150_hdcp_init_done=0;
         ANX7150_hdcp_auth_en=0;
         ANX7150_HDCP_Encryption_Disable();
         if(ANX7150_system_state == ANX7150_PLAY_BACK) 
         {
             	ANX7150_auth_fully_pass = 0;
             	ANX7150_Set_System_State(ANX7150_HDCP_AUTHENTICATION);//ANX7150_CONFIG_VIDEO);	//updated for Changhong TV;
             	//disable audio
            	ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, &c);
            	ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, c & (~ANX7150_HDMI_AUDCTRL1_IN_EN));
				ANX7150_Clean_HDCP();							//updated for Changhong TV
	    }
      }
}


void ANX7150_AFIFO_Overrun_Interrupt(void)
{
    BYTE c;
    
    if(ANX7150_system_state != ANX7150_INITIAL 
        && ANX7150_system_state != ANX7150_WAIT_HOTPLUG 
        && ANX7150_system_state != ANX7150_READ_PARSE_EDID
        && ANX7150_system_state != ANX7150_WAIT_RX_SENSE
        && ANX7150_system_state != ANX7150_CONFIG_VIDEO)
    {
        ANX7150_DEBUG("ANX7150: AFIFO overrun interrupt,disable audio.\n");
        // disable audio 
        ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG,&c);
        c &= ~ANX7150_HDMI_AUDCTRL1_IN_EN;
        ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG,c);
        ANX7150_Set_System_State(ANX7150_CONFIG_AUDIO);
    }
}
/*
void ANX7150_PllLock_Interrupt()
{
    BYTE c;

    if((ANX7150_system_state != ANX7150_INITIAL) 
        && (ANX7150_system_state != ANX7150_WAIT_HOTPLUG) 
        && (ANX7150_system_state != ANX7150_READ_PARSE_EDID)
        && (ANX7150_system_state != ANX7150_WAIT_RX_SENSE))
    {
        ANX7150_Set_AVMute();
        ANX7150_DEBUG("ANX7150: PLL unlock interrupt,disable audio.\n");
        // disable audio & video
        ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG,&c);
        c &= ~ANX7150_HDMI_AUDCTRL1_IN_EN;
        ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG,c);

        ANX7150_i2c_read_p0_reg(ANX7150_VID_CTRL_REG,&c);
        c &= ~ANX7150_VID_CTRL_IN_EN;
        ANX7150_i2c_write_p0_reg(ANX7150_VID_CTRL_REG,c);
        ANX7150_Set_System_State(ANX7150_CONFIG_VIDEO);
    }
    //when pll change, clear this reg to avoid error in package config
    ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL1_REG, 0x00);
    ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL2_REG, 0x00);

		    
		    ANX7150_system_config_done = 0;//

}
*/
void ANX7150_PllLock_Interrupt()
{
    BYTE c;

    if((ANX7150_system_state != ANX7150_INITIAL) 
        && (ANX7150_system_state != ANX7150_WAIT_HOTPLUG) 
        && (ANX7150_system_state != ANX7150_READ_PARSE_EDID)
        && (ANX7150_system_state != ANX7150_WAIT_RX_SENSE))
    {
        ANX7150_i2c_read_p0_reg(ANX7150_CHIP_STATUS_REG,&c);

    	if (!(c & ANX7150_CHIP_STATUS_PLL_LOCK))
		{
			ANX7150_Set_AVMute();
			ANX7150_DEBUG("ANX7150: PLL unlock interrupt,disable audio.");
			// disable audio & video
			ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG,&c);
			c &= ~ANX7150_HDMI_AUDCTRL1_IN_EN;
			ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG,c);

			ANX7150_i2c_read_p0_reg(ANX7150_VID_CTRL_REG,&c);
			c &= ~ANX7150_VID_CTRL_IN_EN;
			ANX7150_i2c_write_p0_reg(ANX7150_VID_CTRL_REG,c);
			ANX7150_Set_System_State(ANX7150_CONFIG_VIDEO);

	    //when pll change, clear this reg to avoid error in package config
			ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL1_REG, 0x00);
    		ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL2_REG, 0x00);
		    ANX7150_system_config_done = 0;

			}
    }
    

}

void ANX7150_SPDIF_Error_Interrupt(BYTE int1, BYTE int3)
{
    if((ANX7150_system_state == ANX7150_CONFIG_AUDIO
        || ANX7150_system_state == ANX7150_CONFIG_PACKETS
        || ANX7150_system_state == ANX7150_HDCP_AUTHENTICATION
        || ANX7150_system_state == ANX7150_PLAY_BACK ) 
        && (int3 & 0x81)) 
    {
        ANX7150_DEBUG("SPDIF BI Phase or Unstable error.\n");
        spdif_error_cnt += 0x03; 
    }
    if((ANX7150_system_state == ANX7150_CONFIG_AUDIO
        || ANX7150_system_state == ANX7150_CONFIG_PACKETS
        || ANX7150_system_state == ANX7150_HDCP_AUTHENTICATION
        || ANX7150_system_state == ANX7150_PLAY_BACK ) 
        && (int1 & ANX7150_INTR1_STATUS_SPDIF_ERR))
    {
        ANX7150_DEBUG("SPDIF Parity error.\n");
        spdif_error_cnt += 0x01; 
    }
    // adjust spdif phase
    if(spdif_error_cnt >= spdif_error_th)
    {
        BYTE freq_mclk,c1,c;
        spdif_error_cnt = 0x00;
        ANX7150_DEBUG("adjust mclk phase!\n");
        ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL0_REG, &c);
        ANX7150_i2c_read_p0_reg(ANX7150_I2S_CTRL_REG, &c1);

        freq_mclk = c & 0x07; 
        switch(freq_mclk)
        {
            case ANX7150_mclk_128_Fs:   //invert 0x50[3]
                ANX7150_DEBUG("adjust mclk phase when 128*Fs!\n");
                if( c & 0x08 )    c &= 0xf7;
                else   c |= 0x08;
                ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL0_REG, c);
                break;
                
            case ANX7150_mclk_256_Fs:
            case ANX7150_mclk_384_Fs:
                ANX7150_DEBUG("adjust mclk phase when 256*Fs or 384*Fs!\n");
                if( c1 & 0x60 )   c1 &= 0x9f;
                else     c1 |= 0x20;
                ANX7150_i2c_write_p0_reg(ANX7150_I2S_CTRL_REG, c1);
                break;

            case ANX7150_mclk_512_Fs:
                ANX7150_DEBUG("adjust mclk phase when 512*Fs!\n");
                if( c1 & 0x60 )   c1 &= 0x9f;
                else    c1 |= 0x40;
                ANX7150_i2c_write_p0_reg(ANX7150_I2S_CTRL_REG, c1);
                break;
            default:
                break;

        }     
    }
  }

void ANX7150_Rx_Sense_Interrupt(void)
{
    BYTE c;

    ANX7150_i2c_read_p0_reg(ANX7150_SYS_STATE_REG,&c);
    ANX7150_DEBUG("ANX7150_Rx_Sense_Interrupt, ANX7150_SYS_STATE_REG = %.2x\n", (unsigned int)c);

    if( c & ANX7150_SYS_STATE_RSV_DET)
    {
         //xy 11.06 Power on chip
        ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL1_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL1_REG, c | 0x01);
	s_ANX7150_packet_config.packets_need_config = 0x03;   //new avi infoframe
    }
    else
    {
        // Rx is not active
        if((ANX7150_system_state != ANX7150_INITIAL) 
            && (ANX7150_system_state != ANX7150_WAIT_HOTPLUG) 
            && (ANX7150_system_state != ANX7150_READ_PARSE_EDID))
        {
            ANX7150_Set_System_State(ANX7150_WAIT_RX_SENSE);
            //stop HDCP and reset DDC when lost Rx sense
            ANX7150_i2c_read_p0_reg(ANX7150_HDCP_CTRL0_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL0_REG, (c & (~ANX7150_HDCP_CTRL0_HW_AUTHEN)));
            ANX7150_RST_DDCChannel(); 
            ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL1_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL1_REG, c & 0xfd);
            // mute TMDS link
            ANX7150_DEBUG("ANX7150_Rx_Sense_Interrupt--->&&&&&&&&0xef\n");
            ANX7150_i2c_read_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, &c);
	    ANX7150_DEBUG("ANX7150_Rx_Sense_Interrupt=%x\n",c);
            ANX7150_i2c_write_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, c & (~ANX7150_TMDS_CLKCH_MUTE));
        }
        //Power down chip
        ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL1_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL1_REG, c & 0xfe);
    }
	 //xy 11.06 when format change, need system config again
    ANX7150_system_config_done = 0;
}

void ANX7150_HDCPLINK_CHK_Interrupt(void)
{
			BYTE c,c1, ri0,ri1;
		    //ANX7150_InitDDC_Read(0x74, segmentpointer, offset, 0x01, 0x00);
			//Write slave device address
			ANX7150_i2c_write_p0_reg(ANX7150_DDC_SLV_ADDR_REG, 0x74);
			// Write segment address
			//ANX7150_i2c_write_p0_reg(ANX7150_DDC_SLV_SEGADDR_REG, segmentpointer);
			//Write offset
			ANX7150_i2c_write_p0_reg(ANX7150_DDC_SLV_OFFADDR_REG, 0x08);
			//Write number for access
			ANX7150_i2c_write_p0_reg(ANX7150_DDC_ACCNUM0_REG, 0x02);
			ANX7150_i2c_write_p0_reg(ANX7150_DDC_ACCNUM1_REG, 0x00);
			//Clear FIFO
			ANX7150_i2c_write_p0_reg(ANX7150_DDC_ACC_CMD_REG, 0x05);
			//EDDC sequential Read
			ANX7150_i2c_write_p0_reg(ANX7150_DDC_ACC_CMD_REG, 0x01);

			msleep(1);

	        ANX7150_i2c_read_p0_reg(ANX7150_DDC_FIFO_ACC_REG, &ri0);      
	        ANX7150_i2c_read_p0_reg(ANX7150_DDC_FIFO_ACC_REG, &ri1);      

			
			ANX7150_i2c_read_p0_reg(ANX7150_HDCP_RI1_REG, &c);
			ANX7150_i2c_read_p0_reg(ANX7150_HDCP_RI2_REG, &c1);
			//ANX7150_DEBUG("DDC read Ri0= %.2x\n", (unsigned int)ri0);
			//ANX7150_DEBUG("DDC read Ri1= %.2x\n", (unsigned int)ri1);
			//ANX7150_DEBUG("###ANX7150 Ri0= %.2x\n", (unsigned int)c);
			//ANX7150_DEBUG("###ANX7150 Ri1= %.2x\n", (unsigned int)c1);
			if ((ri0 != c) || (ri1 != c1))
			{
					ANX7150_DEBUG("Ri check error!!!!!!!\n");

					ANX7150_hdcp_auth_fail_counter ++;
			        if(ANX7150_hdcp_auth_fail_counter >= ANX7150_HDCP_FAIL_THRESHOLD) 
			        {					
								ANX7150_Set_AVMute();
								ANX7150_DEBUG("ANX7150_Authentication failed_by_Ri check error!\n");
								ANX7150_hdcp_auth_pass = 0;
								ANX7150_hdcp_wait_100ms_needed = 1;
								ANX7150_auth_fully_pass = 0;
								ANX7150_hdcp_init_done=0;   //HDCP CTS
								ANX7150_hdcp_auth_en=0;   //HDCP CTS
								ANX7150_HDCP_Encryption_Disable();
								ANX7150_hdcp_auth_fail_counter = 0;
								if(ANX7150_system_state == ANX7150_PLAY_BACK) 
								{
								   	ANX7150_auth_fully_pass = 0;
								   	ANX7150_Set_System_State(ANX7150_HDCP_AUTHENTICATION);//ANX7150_CONFIG_VIDEO);	//updated for Changhong TV;
								   	//disable audio
								  	ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, &c);
								  	ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, c & (~ANX7150_HDMI_AUDCTRL1_IN_EN));
									ANX7150_Clean_HDCP();//updated for Changhong TV
								}
			        	}
			}

}


void ANX7150_Blue_Screen_Format_Config(void) 
{
    // TODO:Add ITU 601 format.(Now only ITU 709 format added)
    switch (ANX7150_RGBorYCbCr) 
    {
        case ANX7150_RGB: //select RGB mode
	    ANX7150_DEBUG("##ANX7150_Blue_Screen_Format_Config..ANX7150_RGB\n");
            ANX7150_i2c_write_p0_reg(ANX7150_HDCP_BLUESCREEN0_REG, 0x10);
            ANX7150_i2c_write_p0_reg(ANX7150_HDCP_BLUESCREEN1_REG, 0xeb);
            ANX7150_i2c_write_p0_reg(ANX7150_HDCP_BLUESCREEN2_REG, 0x10);
            break;
        case ANX7150_YCbCr422: //select YCbCr4:2:2 mode
	    ANX7150_DEBUG("##ANX7150_Blue_Screen_Format_Config..ANX7150_YCbCr422\n");
            ANX7150_i2c_write_p0_reg(ANX7150_HDCP_BLUESCREEN0_REG, 0x00);
            ANX7150_i2c_write_p0_reg(ANX7150_HDCP_BLUESCREEN1_REG, 0xad);
            ANX7150_i2c_write_p0_reg(ANX7150_HDCP_BLUESCREEN2_REG, 0x2a);
            break;
        case ANX7150_YCbCr444: //select YCbCr4:4:4 mode
	    ANX7150_DEBUG("##ANX7150_Blue_Screen_Format_Config..ANX7150_YCbCr444\n");
            ANX7150_i2c_write_p0_reg(ANX7150_HDCP_BLUESCREEN0_REG, 0x1a);
            ANX7150_i2c_write_p0_reg(ANX7150_HDCP_BLUESCREEN1_REG, 0xad);
            ANX7150_i2c_write_p0_reg(ANX7150_HDCP_BLUESCREEN2_REG, 0x2a);
            break;
        default: 
            break;
    }
}

void ANX7150_Blue_Screen_Enable(void) 
{
    BYTE c;
    ANX7150_i2c_read_p0_reg(ANX7150_HDCP_CTRL1_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL1_REG,  c | ANX7150_HDCP_CTRL1_BLUE_SCREEN_EN);
    ANX7150_send_blue_screen = 1;
}    

void ANX7150_Blue_Screen_Disable(void) 
{
    BYTE c;
    ANX7150_i2c_read_p0_reg(ANX7150_HDCP_CTRL1_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL1_REG,  (c & 0xfb));
    ANX7150_send_blue_screen = 0;
}

void ANX7150_HDCP_Encryption_Enable(void) 
{
    BYTE c;
    ANX7150_i2c_read_p0_reg(ANX7150_HDCP_CTRL0_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL0_REG, (c | ANX7150_HDCP_CTRL0_ENC_EN));
    ANX7150_hdcp_encryption = 1;
}    

void ANX7150_HDCP_Encryption_Disable(void) 
{
    BYTE c;
    ANX7150_i2c_read_p0_reg(ANX7150_HDCP_CTRL0_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL0_REG, (c & 0xfb));
    ANX7150_hdcp_encryption = 0;
}    


//******************************Video Config***************************************
void ANX7150_Config_Video()
{
    BYTE c,TX_is_HDMI;
    BIT cspace_y2r, y2r_sel, up_sample,range_y2r;

    cspace_y2r = 0;
    y2r_sel = 0;
    up_sample = 0;
    range_y2r = 0;

	if (QFN48_EN)
		ANX7150_RGBorYCbCr = 0x01;						//YCbCr 422
	else
		ANX7150_RGBorYCbCr = 0x00;						//RGB
	//ANX7150_RGBorYCbCr = ANX7150_INPUT_COLORSPACE;						//update
	ANX7150_i2c_write_p0_reg(ANX7150_VID_MODE_REG, 0x00);			//update

	ANX7150_i2c_read_p0_reg(ANX7150_VID_CTRL_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_VID_CTRL_REG, c & (~ANX7150_VID_CTRL_BITCTRL_EN));
	
    if(!ANX7150_system_config_done)
    {
        ANX7150_DEBUG("System has not finished config!\n");
        return;
    }

    ANX7150_i2c_read_p0_reg(ANX7150_SYS_STATE_REG, &c);
    if(!(c & 0x02))
    {
        ANX7150_DEBUG("No clock detected !\n");
        //ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL2_REG, 0x02);
        return;
    }

    ANX7150_Clean_HDCP();
    
    //color space issue
    switch(ANX7150_video_timing_id)
    {
        case ANX7150_V1280x720p_50Hz:
        case ANX7150_V1280x720p_60Hz:
        case ANX7150_V1920x1080i_60Hz:
        case ANX7150_V1920x1080i_50Hz: 
        case ANX7150_V1920x1080p_60Hz:
        case ANX7150_V1920x1080p_50Hz:
           y2r_sel = ANX7150_CSC_BT709; 
           break;
        default:
           y2r_sel = ANX7150_CSC_BT601; 
           break;
    }
    //rang[0~255]/[16~235] select
    if(ANX7150_video_timing_id == ANX7150_V640x480p_60Hz)
        range_y2r = 1;//rang[0~255]
    else
        range_y2r = 0;//rang[16~235]
    if((ANX7150_RGBorYCbCr == ANX7150_YCbCr422) && (!ANX7150_edid_result.ycbcr422_supported))
    {
        up_sample = 1;
	 if(ANX7150_edid_result.ycbcr444_supported)
	    cspace_y2r = 0;
	 else
	    cspace_y2r = 1;
    }
    if((ANX7150_RGBorYCbCr == ANX7150_YCbCr444) && (!ANX7150_edid_result.ycbcr444_supported))
    {
        cspace_y2r = 1;
    }
    //Config the embeded blue screen format according to output video format.
    ANX7150_Blue_Screen_Format_Config();
    
    ANX7150_Parse_Video_Format();

    if(ANX7150_de_gen_en)
    {
        ANX7150_DEBUG("ANX7150_de_gen_en!\n");
        ANX7150_DE_Generator();
    }
    else
    {
        ANX7150_i2c_read_p0_reg(ANX7150_VID_CAPCTRL0_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL0_REG, c & (~ANX7150_VID_CAPCTRL0_DEGEN_EN));
    }
    if(ANX7150_emb_sync_mode)
    {
        ANX7150_DEBUG("ANX7150_Embed_Sync_Decode!\n");
        ANX7150_Embed_Sync_Decode();
		if (ANX7150_ddr_bus_mode) //for DDR embeded sync
		{
			ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL4_REG, &c);
        	ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL4_REG, c | 0x04);
		}
		else
		{
			ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL4_REG, &c);
        	ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL4_REG, c & 0xfb);
		}
    }
    else
    {
        ANX7150_i2c_read_p0_reg(ANX7150_VID_CAPCTRL0_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL0_REG, c & (~ANX7150_VID_CAPCTRL0_EMSYNC_EN));
    }
    if(ANX7150_demux_yc_en)
    {
        ANX7150_DEBUG("ANX7150_demux_yc_en!\n");
        ANX7150_i2c_read_p0_reg(ANX7150_VID_CAPCTRL0_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL0_REG, c | ANX7150_VID_CAPCTRL0_DEMUX_EN);
        if(ANX7150_ycmux_BIT_sel)
        {
            ANX7150_i2c_read_p0_reg(ANX7150_VID_CTRL_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_VID_CTRL_REG, c | ANX7150_VID_CTRL_YCBIT_SEL);//bit mapping for yc mux, D13-8,1-0 -->D11-4
			  ANX7150_DEBUG("ANX7150_demux_yc_en!####D11-4\n");

            ANX7150_i2c_read_p0_reg(ANX7150_VID_CTRL_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_VID_CTRL_REG, c | ANX7150_VID_CTRL_BITCTRL_EN);

				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL11,  0x0d);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL10,  0x0c);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL9,  0x0b);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL8,  0x0a);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL7,  0x09);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL6,  0x08);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL5,  0x01);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL4,  0x00);		
								
						//
         }
         else
         {
	            ANX7150_i2c_read_p0_reg(ANX7150_VID_CTRL_REG, &c);
	            ANX7150_i2c_write_p0_reg(ANX7150_VID_CTRL_REG, c & (~ANX7150_VID_CTRL_YCBIT_SEL));//bit mapping for yc mux, D13-8,1-0 -->D15-8, 
				 ANX7150_DEBUG("ANX7150_demux_yc_en!####D15-8");

				ANX7150_i2c_read_p0_reg(ANX7150_VID_CTRL_REG, &c);
            	ANX7150_i2c_write_p0_reg(ANX7150_VID_CTRL_REG, c | ANX7150_VID_CTRL_BITCTRL_EN);					

				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL15, 0x0d);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL14, 0x0c);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL13,  0x0b);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL12, 0x0a);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL11, 0x09);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL10, 0x08);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL9, 0x01);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL8, 0x00);	
						//

			}
     }
    else
    {
        ANX7150_i2c_read_p0_reg(ANX7150_VID_CAPCTRL0_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL0_REG, c & (~ANX7150_VID_CAPCTRL0_DEMUX_EN));

		}
     if(ANX7150_ddr_bus_mode)
     {
         ANX7150_DEBUG("ANX7150_ddr_bus_mode!\n");
         ANX7150_i2c_read_p0_reg(ANX7150_VID_CAPCTRL0_REG, &c);
         ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL0_REG, c | ANX7150_VID_CAPCTRL0_DV_BUSMODE);


			ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL4_REG, &c); //qfn48
        	ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL4_REG, (c & 0xfc) | 0x02);
			ANX7150_i2c_read_p0_reg(ANX7150_VID_CTRL_REG, &c); //bit map for 
			ANX7150_i2c_write_p0_reg(ANX7150_VID_CTRL_REG, c |ANX7150_VID_CTRL_YCBIT_SEL);		

				 
         if(ANX7150_ddr_edge)
        {
            ANX7150_i2c_read_p0_reg(ANX7150_VID_CAPCTRL0_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL0_REG, c | ANX7150_VID_CAPCTRL0_DDR_EDGE);
        }  
		 else
		 {
            ANX7150_i2c_read_p0_reg(ANX7150_VID_CAPCTRL0_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL0_REG, c & (~ANX7150_VID_CAPCTRL0_DDR_EDGE));
        }

		//for DDR+seperate maping
		 if (ANX7150_video_format_config == 0x07)//DDR yc422, 601, 
		 {
				ANX7150_DEBUG("ANX7150_DDR_601_Maping!\n");

				ANX7150_i2c_read_p0_reg(ANX7150_VID_CTRL_REG, &c);
            	ANX7150_i2c_write_p0_reg(ANX7150_VID_CTRL_REG, c | ANX7150_VID_CTRL_BITCTRL_EN);					

				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL23,  0x0B);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL22,  0x0A);				
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL21,  0x09);				
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL20,  0x08);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL19,  0x07);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL18,  0x06);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL17,  0x05);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL16, 0x04);

				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL15,  0x17);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL14,  0x16);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL13,  0x15);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL12,  0x14);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL11,  0x13);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL10,  0x12);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL9,  0x11);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL8,  0x10);	

				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL7,  0x03);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL6,  0x02);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL5,  0x01);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL4,  0x00);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL3,  0x0F);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL2,  0x0E);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL1, 0x0D);
				ANX7150_i2c_write_p0_reg(VID_BIT_CTRL0,  0x0C);					

		 }
		 else if (ANX7150_video_format_config == 0x08)//DDR yc422, 656,
		 {
				ANX7150_DEBUG("ANX7150_DDR_656_Maping!\n");

				ANX7150_i2c_read_p0_reg(ANX7150_VID_CTRL_REG, &c);
            	ANX7150_i2c_write_p0_reg(ANX7150_VID_CTRL_REG, c & (~ANX7150_VID_CTRL_BITCTRL_EN));		

		 }
		 
     }
     else
    {
         ANX7150_i2c_read_p0_reg(ANX7150_VID_CAPCTRL0_REG, &c);
         ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL0_REG, c & (~ANX7150_VID_CAPCTRL0_DV_BUSMODE));
         ANX7150_i2c_read_p0_reg(ANX7150_VID_CAPCTRL0_REG, &c);
         ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL0_REG, c & (~ANX7150_VID_CAPCTRL0_DDR_EDGE));
		  ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL4_REG, &c); //qfn48
        	ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL4_REG, c & 0xfc);
     }

    if(cspace_y2r)
    {
        ANX7150_DEBUG("Color space Y2R enabled********\n");
        ANX7150_i2c_read_p0_reg(ANX7150_VID_MODE_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_VID_MODE_REG, c | ANX7150_VID_MODE_CSPACE_Y2R);
        if(y2r_sel)
        {
            ANX7150_DEBUG("Y2R_SEL!\n");
            ANX7150_i2c_read_p0_reg(ANX7150_VID_MODE_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_VID_MODE_REG, c | ANX7150_VID_MODE_Y2R_SEL);
        }
        else
        {
            ANX7150_i2c_read_p0_reg(ANX7150_VID_MODE_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_VID_MODE_REG, c & (~ANX7150_VID_MODE_Y2R_SEL));
        }
    }
    else
    {
        ANX7150_i2c_read_p0_reg(ANX7150_VID_MODE_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_VID_MODE_REG, c & (~ANX7150_VID_MODE_CSPACE_Y2R));
    }
        
    if(up_sample)
    {
        ANX7150_DEBUG("UP_SAMPLE!\n");
        ANX7150_i2c_read_p0_reg(ANX7150_VID_MODE_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_VID_MODE_REG, c | ANX7150_VID_MODE_UPSAMPLE);
    }
    else
    {
        ANX7150_i2c_read_p0_reg(ANX7150_VID_MODE_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_VID_MODE_REG, c & (~ANX7150_VID_MODE_UPSAMPLE));
    }
    
    if(range_y2r)
    {
        ANX7150_i2c_read_p0_reg(ANX7150_VID_MODE_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_VID_MODE_REG, c | ANX7150_VID_MODE_RANGE_Y2R);
    }
    else
    {
        ANX7150_i2c_read_p0_reg(ANX7150_VID_MODE_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_VID_MODE_REG, c & (~ANX7150_VID_MODE_RANGE_Y2R));
    }

    if(!ANX7150_pix_rpt_set_by_sys)
    {
        if((ANX7150_video_timing_id == ANX7150_V720x480i_60Hz_16x9) 
            || (ANX7150_video_timing_id == ANX7150_V720x576i_50Hz_16x9)
            || (ANX7150_video_timing_id == ANX7150_V720x480i_60Hz_4x3) 
            || (ANX7150_video_timing_id == ANX7150_V720x576i_50Hz_4x3))
            ANX7150_tx_pix_rpt = 1;
        else
            ANX7150_tx_pix_rpt = 0;
    }
    //set input pixel repeat times
    ANX7150_i2c_read_p0_reg(ANX7150_VID_MODE_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_VID_MODE_REG, ((c & 0xfc) |ANX7150_in_pix_rpt));
    //set link pixel repeat times
    ANX7150_i2c_read_p0_reg(ANX7150_VID_CTRL_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_VID_CTRL_REG, ((c & 0xfc) |ANX7150_tx_pix_rpt));

     if((ANX7150_in_pix_rpt != ANX7150_in_pix_rpt_bkp) 
        ||(ANX7150_tx_pix_rpt != ANX7150_tx_pix_rpt_bkp) )
    {
        ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL2_REG, 0x02);
        ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL2_REG, 0x00);
        ANX7150_DEBUG("MISC_Reset!\n");
        ANX7150_in_pix_rpt_bkp = ANX7150_in_pix_rpt;
        ANX7150_tx_pix_rpt_bkp = ANX7150_tx_pix_rpt;
    }
    //enable video input
    ANX7150_i2c_read_p0_reg(ANX7150_VID_CTRL_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_VID_CTRL_REG, c | ANX7150_VID_CTRL_IN_EN);
    
    ANX7150_DEBUG("Video configure OK!\n");

    ANX7150_i2c_read_p0_reg(ANX7150_VID_STATUS_REG, &c);
    ANX7150_DEBUG("ANX7150_VID_STATUS_REG=%x\n",c);
//    for (i=0;i<300;i++)
{
    if(!(c & ANX7150_VID_STATUS_VID_STABLE))
    {
        ANX7150_DEBUG("Video not stable!\n");
//	msleep(100);
//	if(i==298)
           return;
    }
}
    if(cspace_y2r)
        ANX7150_RGBorYCbCr = ANX7150_RGB;
    //Enable video CLK,Format change after config video.
   // ANX7150_i2c_read_p0_reg(ANX7150_INTR1_MASK_REG, &c);
   // ANX7150_i2c_write_p0_reg(ANX7150_INTR1_MASK_REG, c |0x01);//3
   ANX7150_i2c_read_p0_reg(ANX7150_INTR2_MASK_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_INTR2_MASK_REG, c | 0x48);
    ANX7150_i2c_read_p0_reg(ANX7150_INTR3_MASK_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_INTR3_MASK_REG, c | 0x40);


    if(ANX7150_edid_result.is_HDMI)
    {
        ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL1_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL1_REG, c | 0x02);
        ANX7150_DEBUG("ANX7150 is set to HDMI mode\n");
    }
    else
    {
	ANX7150_DEBUG("ANX7150 is none HDMI mode\n");	
    }
    ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL1_REG, &c);
    TX_is_HDMI = c & 0x02;

    if(TX_is_HDMI == 0x02) 
    {
        ANX7150_Set_AVMute();
        ANX7150_Set_System_State(ANX7150_CONFIG_AUDIO);
    }  
    else 
    {
    //To-Do: Config to DVI mode. 
        ANX7150_Set_System_State(ANX7150_HDCP_AUTHENTICATION);
    }

    //SW reset before TMDS clock enable  
	ANX7150_i2c_read_p0_reg(ANX7150_SRST_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_SRST_REG, c | ANX7150_SRST_SW_RST);
	ANX7150_i2c_write_p0_reg(ANX7150_SRST_REG, (c & (~ANX7150_SRST_SW_RST)));

    //reset TMDS link to align 4 channels  xy 061120
    ANX7150_i2c_read_p0_reg(ANX7150_SRST_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_SRST_REG, (c | ANX7150_TX_RST));
    ANX7150_i2c_write_p0_reg(ANX7150_SRST_REG, (c & (~ANX7150_TX_RST)));
    //Enable TMDS clock output // just enable bit7, and let the other bits along to avoid overwriting.
    ANX7150_DEBUG("ANX7150_Config_Video--->|||||||||0x80\n");
    ANX7150_i2c_read_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, &c);
    ANX7150_DEBUG("ANX7150_TMDS_CLKCH_CONFIG_REG=%x\n",c);
    ANX7150_i2c_write_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, (c | ANX7150_TMDS_CLKCH_MUTE));
    msleep(100);  //msleep(10); //HDCP CTS

    //ANX7150_i2c_read_p0_reg(ANX7150_VID_MODE_REG, &c);  //zy 061110
}

void ANX7150_Parse_Video_Format(void)
{
    switch(ANX7150_video_format_config)
    {
        case ANX7150_RGB_YCrCb444_SepSync:
            ANX7150_emb_sync_mode = 0;
            ANX7150_demux_yc_en = 0;
            ANX7150_ddr_bus_mode = 0;
            ANX7150_de_gen_en = 0;
            ANX7150_DEBUG("RGB_YCrCb444_SepSync mode!\n");
            break;
        case ANX7150_YCrCb422_SepSync:
            ANX7150_emb_sync_mode = 0;
            ANX7150_demux_yc_en = 0;
            ANX7150_ddr_bus_mode = 0;
            ANX7150_de_gen_en = 0;
            ANX7150_DEBUG("YCrCb422_SepSync mode!\n");
            break;
        case ANX7150_YCrCb422_EmbSync:
            ANX7150_DEBUG("YCrCb422_EmbSync mode!\n");
            ANX7150_demux_yc_en = 0;
            ANX7150_ddr_bus_mode = 0;
            ANX7150_de_gen_en = 0;
            ANX7150_emb_sync_mode = 1;
            ANX7150_Get_Video_Timing();
            break;
        case ANX7150_YCMux422_SepSync_Mode1:
            ANX7150_DEBUG("YCMux422_SepSync_Mode1 mode!\n");
            ANX7150_emb_sync_mode = 0;
            ANX7150_ddr_bus_mode = 0;
            ANX7150_de_gen_en = 0;
            ANX7150_ycmux_BIT_sel = 0;
            ANX7150_demux_yc_en = 1;
            break;
        case ANX7150_YCMux422_SepSync_Mode2:
            ANX7150_DEBUG("YCMux422_SepSync_Mode2 mode!\n");
            ANX7150_emb_sync_mode = 0;
            ANX7150_ddr_bus_mode = 0;
            ANX7150_de_gen_en = 0;
            ANX7150_ycmux_BIT_sel = 1;
            ANX7150_demux_yc_en = 1;
            break;
        case ANX7150_YCMux422_EmbSync_Mode1:
            ANX7150_DEBUG("YCMux422_EmbSync_Mode1 mode!\n");
            ANX7150_ddr_bus_mode = 0;
            ANX7150_de_gen_en = 0;
            ANX7150_emb_sync_mode = 1;
            ANX7150_ycmux_BIT_sel = 0;
            ANX7150_demux_yc_en = 1;
            ANX7150_Get_Video_Timing();
            break;
        case ANX7150_YCMux422_EmbSync_Mode2:
            ANX7150_DEBUG("YCMux422_EmbSync_Mode2 mode!\n");
            ANX7150_ddr_bus_mode = 0;
            ANX7150_de_gen_en = 0;
            ANX7150_emb_sync_mode = 1;
            ANX7150_ycmux_BIT_sel = 1;
            ANX7150_demux_yc_en = 1;
            ANX7150_Get_Video_Timing();
            break;
        case ANX7150_RGB_YCrCb444_DDR_SepSync:
            ANX7150_DEBUG("RGB_YCrCb444_DDR_SepSync mode!\n");
            ANX7150_emb_sync_mode = 0;
            ANX7150_demux_yc_en = 0;
            ANX7150_de_gen_en = 0;
            ANX7150_ddr_bus_mode = 1;
            break;
        case ANX7150_RGB_YCrCb444_DDR_EmbSync:
            ANX7150_DEBUG("RGB_YCrCb444_DDR_EmbSync mode!\n");
            ANX7150_demux_yc_en = 0;
            ANX7150_de_gen_en = 0;
            ANX7150_emb_sync_mode = 1;
            ANX7150_ddr_bus_mode = 1;
            ANX7150_Get_Video_Timing();
            break;
        case ANX7150_RGB_YCrCb444_SepSync_No_DE:
            ANX7150_DEBUG("RGB_YCrCb444_SepSync_No_DE mode!\n");
            ANX7150_emb_sync_mode = 0;
            ANX7150_demux_yc_en = 0;
            ANX7150_ddr_bus_mode = 0;
            ANX7150_de_gen_en = 1;
            ANX7150_Get_Video_Timing();
            break;
        case ANX7150_YCrCb422_SepSync_No_DE:
            ANX7150_DEBUG("YCrCb422_SepSync_No_DE mode!\n");
            ANX7150_emb_sync_mode = 0;
            ANX7150_demux_yc_en = 0;
            ANX7150_ddr_bus_mode = 0;
            ANX7150_de_gen_en = 1;
            ANX7150_Get_Video_Timing();
            break;
        default:
            break;
    }
}

void ANX7150_Get_Video_Timing()
{
    BYTE i;
//#ifdef ITU656
    for(i = 0; i < 18; i++)
    {
        switch(ANX7150_video_timing_id)
        {
            case ANX7150_V640x480p_60Hz:
                ANX7150_DEBUG("640x480p_60Hz!\n");
                ANX7150_video_timing_parameter[i] = ANX7150_video_timingtype_table.ANX7150_640x480p_60Hz[i];
                break;
            case ANX7150_V720x480p_60Hz_4x3:
            case ANX7150_V720x480p_60Hz_16x9:
                ANX7150_DEBUG("720x480p_60Hz!\n");
                ANX7150_video_timing_parameter[i] = ANX7150_video_timingtype_table.ANX7150_720x480p_60Hz[i];
                break;
            case ANX7150_V1280x720p_60Hz:
                ANX7150_DEBUG("1280x720p_60Hz!\n");
                ANX7150_video_timing_parameter[i] = ANX7150_video_timingtype_table.ANX7150_1280x720p_60Hz[i];
                break;
            case ANX7150_V1920x1080i_60Hz:
                ANX7150_DEBUG("1920x1080i_60Hz!\n");
                ANX7150_video_timing_parameter[i] = ANX7150_video_timingtype_table.ANX7150_1920x1080i_60Hz[i];
                break;
            case ANX7150_V720x480i_60Hz_4x3:
            case ANX7150_V720x480i_60Hz_16x9:
                ANX7150_DEBUG("720x480i_60Hz!\n");
                ANX7150_video_timing_parameter[i] = ANX7150_video_timingtype_table.ANX7150_720x480i_60Hz[i];
                break;
            case ANX7150_V720x576p_50Hz_4x3:
            case ANX7150_V720x576p_50Hz_16x9:
                ANX7150_DEBUG("720x576p_50Hz!\n");
                ANX7150_video_timing_parameter[i] = ANX7150_video_timingtype_table.ANX7150_720x576p_50Hz[i];
                break;
            case ANX7150_V1280x720p_50Hz:
                ANX7150_DEBUG("1280x720p_50Hz!\n");
                ANX7150_video_timing_parameter[i] = ANX7150_video_timingtype_table.ANX7150_1280x720p_50Hz[i];
                break;
            case ANX7150_V1920x1080i_50Hz:
                ANX7150_DEBUG("1920x1080i_50Hz!\n");
                ANX7150_video_timing_parameter[i] = ANX7150_video_timingtype_table.ANX7150_1920x1080i_50Hz[i];
                break;
            case ANX7150_V720x576i_50Hz_4x3:
            case ANX7150_V720x576i_50Hz_16x9:
                ANX7150_DEBUG("720x576i_50Hz!\n");
                ANX7150_video_timing_parameter[i] = ANX7150_video_timingtype_table.ANX7150_720x576i_50Hz[i];
                break;

            default:
                break;
        }
        ANX7150_DEBUG("Video_Timing_Parameter[%.2x]=%.2x\n", (WORD)i, (WORD) ANX7150_video_timing_parameter[i]);
    }
/*#else
    for(i = 0; i < 18; i++)
    {
        switch(ANX7150_video_timing_id)
        {
            case ANX7150_V640x480p_60Hz:
                //ANX7150_DEBUG("640x480p_60Hz!\n");
                ANX7150_video_timing_parameter[i] = Load_from_EEPROM(0, i);
                msleep(3);
                break;
            case ANX7150_V720x480p_60Hz_4x3:
            case ANX7150_V720x480p_60Hz_16x9:
                //ANX7150_DEBUG("720x480p_60Hz!\n");
                ANX7150_video_timing_parameter[i] = Load_from_EEPROM(0, 18 + i);
                msleep(3);
                break;
            case ANX7150_V1280x720p_60Hz:
                //ANX7150_DEBUG("1280x720p_60Hz!\n");
                ANX7150_video_timing_parameter[i] = Load_from_EEPROM(0, 36 + i);
                msleep(3);
                break;
            case ANX7150_V1920x1080i_60Hz:
                //ANX7150_DEBUG("1920x1080i_60Hz!\n");
                ANX7150_video_timing_parameter[i] = Load_from_EEPROM(0, 54 + i);
                msleep(3);
                break;
            case ANX7150_V720x480i_60Hz_4x3:
            case ANX7150_V720x480i_60Hz_16x9:
                //ANX7150_DEBUG("720x480i_60Hz!\n");
                ANX7150_video_timing_parameter[i] = Load_from_EEPROM(0, 72 + i);
                msleep(3);
                break;
            case ANX7150_V720x576p_50Hz_4x3:
            case ANX7150_V720x576p_50Hz_16x9:
                //ANX7150_DEBUG("720x576p_50Hz!\n");
                ANX7150_video_timing_parameter[i] = Load_from_EEPROM(0, 90 + i);
                msleep(3);
                break;
            case ANX7150_V1280x720p_50Hz:
                //ANX7150_DEBUG("1280x720p_50Hz!\n");
                ANX7150_video_timing_parameter[i] = Load_from_EEPROM(0, 108 + i);
                msleep(3);
                break;
            case ANX7150_V1920x1080i_50Hz:
                //ANX7150_DEBUG("1920x1080i_50Hz!\n");
                ANX7150_video_timing_parameter[i] = Load_from_EEPROM(0, 126 + i);
                msleep(3);
                break;
            case ANX7150_V720x576i_50Hz_4x3:
            case ANX7150_V720x576i_50Hz_16x9:
                //ANX7150_DEBUG("720x576i_50Hz!\n");
                ANX7150_video_timing_parameter[i] = Load_from_EEPROM(0, 144 + i);
                msleep(3);
                break;

            default:
                break;
        }
        //ANX7150_DEBUG("Video_Timing_Parameter[%.2x]=%.2x\n", (WORD)i, (WORD) ANX7150_video_timing_parameter[i]);
    }
#endif*/
}

void ANX7150_DE_Generator()
{
    BYTE c,video_type,hsync_pol,vsync_pol,v_fp,v_bp,vsync_width;	
    BYTE hsync_width_low,hsync_width_high,v_active_low,v_active_high;
    BYTE h_active_low,h_active_high,h_res_low,h_res_high,h_bp_low,h_bp_high;
    WORD hsync_width,h_active,h_res,h_bp;

    video_type = ANX7150_video_timing_parameter[15];
    hsync_pol = ANX7150_video_timing_parameter[16];
    vsync_pol = ANX7150_video_timing_parameter[17];
    v_fp = ANX7150_video_timing_parameter[12];
    v_bp = ANX7150_video_timing_parameter[11];
    vsync_width = ANX7150_video_timing_parameter[10];
    hsync_width = ANX7150_video_timing_parameter[5];
    hsync_width = (hsync_width << 8) + ANX7150_video_timing_parameter[4];
    v_active_high = ANX7150_video_timing_parameter[9];
    v_active_low = ANX7150_video_timing_parameter[8];
    h_active = ANX7150_video_timing_parameter[3];
    h_active = (h_active << 8) + ANX7150_video_timing_parameter[2];
    h_res = ANX7150_video_timing_parameter[1];
    h_res = (h_res << 8) + ANX7150_video_timing_parameter[0];
    h_bp = ANX7150_video_timing_parameter[7];
    h_bp = (h_bp << 8) + ANX7150_video_timing_parameter[6];
    if(ANX7150_demux_yc_en)
    {
        hsync_width = 2* hsync_width;
        h_active = 2 * h_active;
        h_res = 2 * h_res;
        h_bp = 2 * h_bp;
    }
    hsync_width_low = hsync_width & 0xff;
    hsync_width_high = (hsync_width >> 8) & 0xff;
    h_active_low = h_active & 0xff;
    h_active_high = (h_active >> 8) & 0xff;
    h_res_low = h_res & 0xff;
    h_res_high = (h_res >> 8) & 0xff;
    h_bp_low = h_bp & 0xff;
    h_bp_high = (h_bp >> 8) & 0xff;
    
    ANX7150_i2c_read_p0_reg(ANX7150_VID_CAPCTRL1_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL1_REG, c & 0xf7 |video_type);//set video type
    ANX7150_i2c_read_p0_reg(ANX7150_VID_CAPCTRL1_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL1_REG, c  & 0xdf |hsync_pol);//set HSYNC polarity
    ANX7150_i2c_read_p0_reg(ANX7150_VID_CAPCTRL1_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL1_REG, c & 0xbf |vsync_pol);//set VSYNC polarity
    ANX7150_i2c_write_p0_reg(ANX7150_ACT_LINEL_REG, v_active_low);
    ANX7150_i2c_write_p0_reg(ANX7150_ACT_LINEH_REG, v_active_high);
    ANX7150_i2c_write_p0_reg(ANX7150_VSYNC_WID_REG, vsync_width);
    ANX7150_i2c_write_p0_reg(ANX7150_VSYNC_TAIL2VIDLINE_REG, v_bp);
    ANX7150_i2c_write_p0_reg(ANX7150_VID_PIXL_REG, h_active_low);
    ANX7150_i2c_write_p0_reg(ANX7150_VID_PIXH_REG, h_active_high);
    ANX7150_i2c_write_p0_reg(ANX7150_H_RESL_REG, h_res_low);
    ANX7150_i2c_write_p0_reg(ANX7150_H_RESH_REG, h_res_high);
    ANX7150_i2c_write_p0_reg(ANX7150_HSYNC_ACT_WIDTHL_REG, hsync_width_low);
    ANX7150_i2c_write_p0_reg(ANX7150_HSYNC_ACT_WIDTHH_REG, hsync_width_high);
    ANX7150_i2c_write_p0_reg(ANX7150_H_BACKPORCHL_REG, h_bp_low);
    ANX7150_i2c_write_p0_reg(ANX7150_H_BACKPORCHH_REG, h_bp_high);
    ANX7150_i2c_read_p0_reg(ANX7150_VID_CAPCTRL0_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL0_REG, c | ANX7150_VID_CAPCTRL0_DEGEN_EN);
}

void ANX7150_Embed_Sync_Decode()
{
    BYTE c,video_type,hsync_pol,vsync_pol,v_fp,vsync_width;	
    BYTE h_fp_low,h_fp_high,hsync_width_low,hsync_width_high;
    WORD h_fp,hsync_width;

    video_type = ANX7150_video_timing_parameter[15];
    hsync_pol = ANX7150_video_timing_parameter[16];
    vsync_pol = ANX7150_video_timing_parameter[17];
    v_fp = ANX7150_video_timing_parameter[12];
    vsync_width = ANX7150_video_timing_parameter[10];
    h_fp = ANX7150_video_timing_parameter[14];
    h_fp = (h_fp << 8) + ANX7150_video_timing_parameter[13];
    hsync_width = ANX7150_video_timing_parameter[5];
    hsync_width = (hsync_width << 8) + ANX7150_video_timing_parameter[4];
    if(ANX7150_demux_yc_en)
    {
        h_fp = 2 * h_fp;
        hsync_width = 2* hsync_width;
    }
    h_fp_low = h_fp & 0xff;
    h_fp_high = (h_fp >> 8) & 0xff;
    hsync_width_low = hsync_width & 0xff;
    hsync_width_high = (hsync_width >> 8) & 0xff;
    
    ANX7150_i2c_read_p0_reg(ANX7150_VID_CAPCTRL1_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL1_REG, c & 0xf7 |video_type);//set video type
    ANX7150_i2c_read_p0_reg(ANX7150_VID_CAPCTRL1_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL1_REG, c & 0xdf |hsync_pol);//set HSYNC polarity
    ANX7150_i2c_read_p0_reg(ANX7150_VID_CAPCTRL1_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL1_REG, c & 0xbf |vsync_pol);//set VSYNC polarity
    ANX7150_i2c_read_p0_reg(ANX7150_VID_CAPCTRL0_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL0_REG, c | ANX7150_VID_CAPCTRL0_EMSYNC_EN);
    ANX7150_i2c_write_p0_reg(ANX7150_ACT_LINE2VSYNC_REG, v_fp);
    ANX7150_i2c_write_p0_reg(ANX7150_VSYNC_WID_REG, vsync_width);
    ANX7150_i2c_write_p0_reg(ANX7150_H_FRONTPORCHL_REG, h_fp_low);
    ANX7150_i2c_write_p0_reg(ANX7150_H_FRONTPORCHH_REG, h_fp_high);
    ANX7150_i2c_write_p0_reg(ANX7150_HSYNC_ACT_WIDTHL_REG, hsync_width_low);
    ANX7150_i2c_write_p0_reg(ANX7150_HSYNC_ACT_WIDTHH_REG, hsync_width_high);
}

void ANX7150_Show_Video_Parameter()
{
   // int h_res,h_act,v_res,v_act,h_fp,hsync_width,h_bp;
//Add for KG2+
#ifdef KG2_APP
    unsigned short HActive = 0, VActive = 0;
    unsigned char  IsProgressive = 0;		
    unsigned short RefRate = 0;			
#endif
//Add for  KG2-
    BYTE c, c1;
   
    ANX7150_DEBUG("***********************************ANX7150 Info************************************\n");
    if(BIST_EN)
        ANX7150_DEBUG("   ANX7150 mode = BIST mode");
    else
        ANX7150_DEBUG("   ANX7150 mode = Normal mode");
    if((ANX7150_demux_yc_en == 1) && (ANX7150_emb_sync_mode == 0))
        ANX7150_DEBUG("   Input video format = YC_MUX");
    if((ANX7150_demux_yc_en == 0) && (ANX7150_emb_sync_mode == 1))
        ANX7150_DEBUG("   Input video format = 656");
    if((ANX7150_demux_yc_en == 1) && (ANX7150_emb_sync_mode == 1))
        ANX7150_DEBUG("   Input video format = YC_MUX + 656");
    if((ANX7150_demux_yc_en == 0) && (ANX7150_emb_sync_mode == 0))
        ANX7150_DEBUG("   Input video format = Seperate Sync");
    if(ANX7150_de_gen_en)
        ANX7150_DEBUG("   DE generator = Enable");
    else
        ANX7150_DEBUG("   DE generator = Disable");
    if((ANX7150_ddr_bus_mode == 1)&& (ANX7150_emb_sync_mode == 0))
        ANX7150_DEBUG("   Input video format = DDR mode");
    else if ((ANX7150_ddr_bus_mode == 1)&& (ANX7150_emb_sync_mode == 1))
        ANX7150_DEBUG("   Input video format = DDR mode + 656");
    ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL1_REG, &c1);
    c1 = (c1 & 0x02);
    if(c1)
    {
        ANX7150_DEBUG("   Output video mode = HDMI\n");
        ANX7150_i2c_read_p1_reg(0x04, &c);
        c = (c & 0x60) >> 5; 
        switch(c)
        {
            case ANX7150_RGB:
                ANX7150_DEBUG("   Output video color format = RGB\n");
                break;
            case ANX7150_YCbCr422:
                ANX7150_DEBUG("   Output video color format = YCbCr422\n");
                break;
            case ANX7150_YCbCr444:
                ANX7150_DEBUG("   Output video color format = YCbCr444\n");
                break;
            default:
		ANX7150_DEBUG("??????Output video color format = %x\n",c);
                break;
        }
    }
    else
    {
        ANX7150_DEBUG("   Output video mode = DVI");
        ANX7150_DEBUG("   Output video color format = RGB");
    }
    
    /*for(i = 0x10; i < 0x25; i ++)
    {
        ANX7150_i2c_read_p0_reg(i, &c );
        ANX7150_DEBUG("0x%.2x = 0x%.2x\n",(unsigned int)i,(unsigned int)c);
    }*/
 /*   ANX7150_i2c_read_p0_reg(ANX7150_VID_STATUS_REG, &c);
    if((c & ANX7150_VID_STATUS_TYPE) == 0x04)
        ANX7150_DEBUG("Video Type = Interlace");
    else
        ANX7150_DEBUG("Video Type = Progressive");
    ANX7150_i2c_read_p0_reg(ANX7150_VIDF_HRESH_REG, &c);
    h_res = c;
    h_res = h_res << 8;
    ANX7150_i2c_read_p0_reg(ANX7150_VIDF_HRESL_REG, &c);
    h_res = h_res + c;
    ANX7150_DEBUG("H_resolution = %u\n",h_res);
    ANX7150_i2c_read_p0_reg(ANX7150_VIDF_PIXH_REG, &c);
    h_act = c;
    h_act = h_act << 8;
    ANX7150_i2c_read_p0_reg(ANX7150_VIDF_PIXL_REG, &c);
    h_act = h_act + c;
    ANX7150_DEBUG("H_active = %u\n",h_act);

    ANX7150_i2c_read_p0_reg(ANX7150_VIDF_VRESH_REG, &c);
    v_res = c;
    v_res = v_res << 8;
    ANX7150_i2c_read_p0_reg(ANX7150_VIDF_VRESL_REG, &c);
    v_res = v_res + c;
    ANX7150_DEBUG("V_resolution = %u\n",v_res);
    ANX7150_i2c_read_p0_reg(ANX7150_VIDF_ACTVIDLINEH_REG, &c);
    v_act = c;
    v_act = v_act << 8;
    ANX7150_i2c_read_p0_reg(ANX7150_VIDF_ACTVIDLINEL_REG, &c);
    v_act = v_act + c;
    ANX7150_DEBUG("V_active = %u\n",v_act);

    ANX7150_i2c_read_p0_reg(ANX7150_VIDF_HFORNTPORCHH_REG, &c);
    h_fp = c;
    h_fp = h_fp << 8;
    ANX7150_i2c_read_p0_reg(ANX7150_VIDF_HFORNTPORCHL_REG, &c);
    h_fp = h_fp + c;
    ANX7150_DEBUG("H_FP = %u\n",h_fp);

    ANX7150_i2c_read_p0_reg(ANX7150_VIDF_HBACKPORCHH_REG, &c);
    h_bp = c;
    h_bp = h_bp << 8;
    ANX7150_i2c_read_p0_reg(ANX7150_VIDF_HBACKPORCHL_REG, &c);
    h_bp = h_bp + c;
    ANX7150_DEBUG("H_BP = %u\n",h_bp);

    ANX7150_i2c_read_p0_reg(ANX7150_VIDF_HSYNCWIDH_REG, &c);
    hsync_width = c;
    hsync_width = hsync_width << 8;
    ANX7150_i2c_read_p0_reg(ANX7150_VIDF_HSYNCWIDL_REG, &c);
    hsync_width = hsync_width + c;
    ANX7150_DEBUG("Hsync_width = %u\n",hsync_width);

    ANX7150_i2c_read_p0_reg(ANX7150_VIDF_ACTLINE2VSYNC_REG, &c);
    ANX7150_DEBUG("Vsync_FP = %bu\n",c);

    ANX7150_i2c_read_p0_reg(ANX7150_VIDF_VSYNCTAIL2VIDLINE_REG, &c);
    ANX7150_DEBUG("Vsync_BP = %bu\n",c);

    ANX7150_i2c_read_p0_reg(ANX7150_VIDF_VSYNCWIDLINE_REG, &c);
    ANX7150_DEBUG("Vsync_width = %bu\n",c);*/
    ANX7150_DEBUG("");
    if(BIST_EN)
    {
        ANX7150_DEBUG("   BIST mode output video format is ");
        if(0)
        	;
        else
        {
            ANX7150_DEBUG("1280x720p@60, ");
            if(ANX7150_edid_result.supported_720p_60Hz)
                ANX7150_DEBUG("and sink supports this format.");
            else
                ANX7150_DEBUG("but sink does not support this format.");
        }
    }
    else
    {
        ANX7150_DEBUG("   Normal mode output video format is ");
        switch(ANX7150_video_timing_id)
        {
            case ANX7150_V720x480p_60Hz_4x3:
            case ANX7150_V720x480p_60Hz_16x9:
#ifdef KG2_APP
		HActive=720;
		VActive=480;
		IsProgressive=1;
		RefRate=60;
#endif 
                ANX7150_DEBUG("720x480p@60, ");
                if(ANX7150_edid_result.supported_720x480p_60Hz)
                    ANX7150_DEBUG("and sink supports this format.");
                else
                    ANX7150_DEBUG("but sink does not support this format.");
                break;
            case ANX7150_V1280x720p_60Hz:
#ifdef KG2_APP
		HActive=1280;
		VActive=720;
		IsProgressive=1;
		RefRate=60;
#endif 
                ANX7150_DEBUG("1280x720p@60, ");
                if(ANX7150_edid_result.supported_720p_60Hz)
                    ANX7150_DEBUG("and sink supports this format.");
                else
                    ANX7150_DEBUG("but sink does not support this format.");
                break;
            case ANX7150_V1920x1080i_60Hz:
#ifdef KG2_APP
		HActive=1920;
		VActive=540;
		IsProgressive=0;
		RefRate=60;
#endif
                ANX7150_DEBUG("1920x1080i@60, ");
                if(ANX7150_edid_result.supported_1080i_60Hz)
                    ANX7150_DEBUG("and sink supports this format.");
                else
                    ANX7150_DEBUG("but sink does not support this format.");
                break;
            case ANX7150_V1920x1080p_60Hz:
#ifdef KG2_APP
		HActive=1920;
		VActive=1080;
		IsProgressive=1;
		RefRate=60;
#endif
                ANX7150_DEBUG("1920x1080p@60, ");
                if(ANX7150_edid_result.supported_1080p_60Hz)
                    ANX7150_DEBUG("and sink supports this format.");
                else
                    ANX7150_DEBUG("but sink does not support this format.");
                break;
            case ANX7150_V1920x1080p_50Hz:
#ifdef KG2_APP
		HActive=1920;
		VActive=1080;
		IsProgressive=1;
		RefRate=50;
#endif
                ANX7150_DEBUG("1920x1080p@50, ");
                if(ANX7150_edid_result.supported_1080p_50Hz)
                    ANX7150_DEBUG("and sink supports this format.");
                else
                    ANX7150_DEBUG("but sink does not support this format.");
                break;
            case ANX7150_V1280x720p_50Hz:
#ifdef KG2_APP
		HActive=1280;
		VActive=720;
		IsProgressive=1;
		RefRate=50;
#endif
                ANX7150_DEBUG("1280x720p@50, ");
                if(ANX7150_edid_result.supported_720p_50Hz)
                    ANX7150_DEBUG("and sink supports this format.");
                else
                    ANX7150_DEBUG("but sink does not support this format.");
                break;
            case ANX7150_V1920x1080i_50Hz:
#ifdef KG2_APP
		HActive=1920;
		VActive=540;
		IsProgressive=0;
		RefRate=50;
#endif
                ANX7150_DEBUG("1920x1080i@50, ");
                if(ANX7150_edid_result.supported_1080i_50Hz)
                    ANX7150_DEBUG("and sink supports this format.");
                else
                    ANX7150_DEBUG("but sink does not support this format.");
                break;
            case ANX7150_V720x576p_50Hz_4x3:
            case ANX7150_V720x576p_50Hz_16x9:
#ifdef KG2_APP
		HActive=720;
		VActive=576;
		IsProgressive=1;
		RefRate=50;
#endif
                ANX7150_DEBUG("720x576p@50, ");
                if(ANX7150_edid_result.supported_576p_50Hz)
                    ANX7150_DEBUG("and sink supports this format.");
                else
                    ANX7150_DEBUG("but sink does not support this format.");
                break;
            case ANX7150_V720x576i_50Hz_4x3:
            case ANX7150_V720x576i_50Hz_16x9:
#ifdef KG2_APP
		HActive=720;
		VActive=288;
		IsProgressive=0;
		RefRate=50;
#endif
                ANX7150_DEBUG("720x576i@50, ");
                if(ANX7150_edid_result.supported_576i_50Hz)
                    ANX7150_DEBUG("and sink supports this format.");
                else
                    ANX7150_DEBUG("but sink does not support this format.");
                break;
            case ANX7150_V720x480i_60Hz_4x3:
            case ANX7150_V720x480i_60Hz_16x9:
#ifdef KG2_APP
		HActive=720;
		VActive=240;
		IsProgressive=0;
		RefRate=60;
#endif

                ANX7150_DEBUG("720x480i@60, ");
                if(ANX7150_edid_result.supported_720x480i_60Hz)
                    ANX7150_DEBUG("and sink supports this format.");
                else
                    ANX7150_DEBUG("but sink does not support this format.");
                break;
            default:
                ANX7150_DEBUG("unknown(video ID is: %.2x).\n",(WORD)ANX7150_video_timing_id);
                break;
        }
    }
#ifdef KG2_APP
//for testing sending msg to KG2 API+
                ANX7150_DEBUG("***********+add for testing sending msg to KG2 API+**********");
		AVC_CMD_TIMING_PARAM kg2_timing_param;
		
		{
			printk(KERN_INFO "anx7150: Change kg2 output timing\n");

			kg2_timing_param.HTotal = 0;				// Filled by kg2 daemon
			kg2_timing_param.HActive = HActive;
			kg2_timing_param.HFrontPorch = 0;			// Filled by kg2 daemon
			kg2_timing_param.HSyncWidth = 0;			// Filled by kg2 daemon
			kg2_timing_param.HPolarity = AVC_CMD_POLARITY_INVERT;	// Filled by kg2 daemon
			kg2_timing_param.VTotal = 0;				// Filled by kg2 daemon
			kg2_timing_param.VActive = VActive;
			kg2_timing_param.VFrontPorch = 0;			// Filled by kg2 daemon
			kg2_timing_param.VSyncWidth = 0;			// Filled by kg2 daemon
			kg2_timing_param.VPolarity = AVC_CMD_POLARITY_INVERT;	// Filled by kg2 daemon
			kg2_timing_param.AspRatio = 0;				// Filled by kg2 daemon
			kg2_timing_param.IsProgressive = IsProgressive;
			kg2_timing_param.RefRate = RefRate;

			kg2_set_output_timing(&kg2_timing_param);
		}
                ANX7150_DEBUG("***********-add for testing sending msg to KG2 API-**********");
//for testing sending msg to KG2 API-
#endif
	    if(c1)//HDMI output
		{
				ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL0_REG, &c);
				c = c & 0x03;    
				ANX7150_DEBUG("   MCLK Frequence = ");

				switch(c)
				{
				  case 0x00: 
				      ANX7150_DEBUG("128 * Fs.");
				      break;
				  case 0x10: 
				      ANX7150_DEBUG("256 * Fs.");
				      break;
				  case 0x20:
				      ANX7150_DEBUG("384 * Fs.");
				      break;
				  case 0x30: 
				      ANX7150_DEBUG("512 * Fs.");
				      break;
				  default :         
				      ANX7150_DEBUG("Wrong MCLK output.");
				      break;
				}
				
				if( ANX7150_AUD_HW_INTERFACE == 0x01)
				{
				  ANX7150_DEBUG("   Input Audio Interface = I2S.\n");
				  ANX7150_i2c_read_p0_reg(ANX7150_I2SCH_STATUS4_REG, &c);
				}
				else if (ANX7150_AUD_HW_INTERFACE == 0x02)
				{
				  ANX7150_DEBUG("   Input Audio Interface = SPDIF.\n");
				  ANX7150_i2c_read_p0_reg(ANX7150_SPDIFCH_STATUS_REG, &c);
				 c=c>>4;
				}
				
				ANX7150_DEBUG("   Audio Fs = ");
				c &= 0x0f;
				switch(c)
				{
				  case 0x00: 
				      ANX7150_DEBUG("44.1 KHz.");
				      break;
				  case 0x02: 
				      ANX7150_DEBUG("48 KHz.");
				      break;
				  case 0x03: 
				      ANX7150_DEBUG("32 KHz.");
				      break;
				  case 0x08: 
				      ANX7150_DEBUG("88.2 KHz.");
				      break;        
				  case 0x0a: 
				      ANX7150_DEBUG("96 KHz.");
				      break;
				case 0x0c: 
				      ANX7150_DEBUG("176.4 KHz.");
				      break;
				  case 0x0e: 
				      ANX7150_DEBUG("192 KHz.");
				      break;
				  default :         
				      ANX7150_DEBUG("Wrong Fs output.");
				      break;
				}

				if	(ANX7150_HDCP_enable == 1)
					  ANX7150_DEBUG("   ANX7150_HDCP_Enable.");
				else
					  ANX7150_DEBUG("   ANX7150_HDCP_Disable.");	
				
			}         
    ANX7150_DEBUG("***********************************************************************************");    
    ANX7150_DEBUG("**********************************ANX7150 Info*************************************");


    ANX7150_i2c_read_p0_reg(ANX7150_H_RESL_REG, &c);
    ANX7150_DEBUG("##ANX7150_H_RESL_REG= %x\n",c);
    ANX7150_i2c_read_p0_reg(ANX7150_H_RESH_REG, &c);
    ANX7150_DEBUG("##ANX7150_H_RESH_REG=%x\n",c);
    ANX7150_i2c_read_p0_reg(ANX7150_VID_PIXL_REG, &c);
    ANX7150_DEBUG("##ANX7150_VID_PIXL_REG=%x\n",c);
    ANX7150_i2c_read_p0_reg(ANX7150_VID_PIXH_REG, &c);
    ANX7150_DEBUG("##ANX7150_VID_PIXH_REG=%x\n",c);


}

//********************************************end video config*******************************

BYTE ANX7150_Config_Audio()
{
    BYTE exe_result = 0x00;
    BYTE c = 0x00;
    BYTE audio_layout = 0x00;
    BYTE fs = 0x00;
    WORD ACR_N = 0x0000;


    //set audio clock edge
    ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL0_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL0_REG,  (c & 0xf7) | ANX7150_audio_clock_edge);
    //cts get select 
    ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL0_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL0_REG,  (c & 0xef));
    ANX7150_DEBUG("audio_type = 0x%.2x\n",(WORD)s_ANX7150_audio_config.audio_type);
    if (s_ANX7150_audio_config.audio_type & ANX7150_i2s_input)
    {
        exe_result |= ANX7150_Config_I2s();
    }
    else
    {
        //disable I2S audio input
        //ANX7150_DEBUG("ANX7150: disable I2S audio input.");
        ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, &c);
        c &= 0xc3;
        ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, c);
    }
    
    if (s_ANX7150_audio_config.audio_type & ANX7150_spdif_input)
    {
        exe_result |= ANX7150_Config_Spdif();
    }
    else
    {
        //disable SPDIF audio input
        ANX7150_DEBUG("ANX7150: disable SPDIF audio input.");
        ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, &c);
        c &= ~ANX7150_HDMI_AUDCTRL1_SPDIFIN_EN;
        ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, c);
  
    }
    
    if (s_ANX7150_audio_config.audio_type & ANX7150_super_audio_input)
    {
        exe_result |= ANX7150_Config_Super_Audio();
    }
    else
    {
        //disable super audio output
        ANX7150_DEBUG("ANX7150: disable super audio output.");
        ANX7150_i2c_write_p0_reg(ANX7150_ONEBIT_AUD_CTRL_REG, 0x00);
    }
    
    if (s_ANX7150_audio_config.audio_type & 0x07 == 0x00)
    {
        ;ANX7150_DEBUG("ANX7150 input no audio type.");
    }

    //audio layout 
    if (s_ANX7150_audio_config.audio_type & ANX7150_i2s_input)
    {
        //ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, &c);
		audio_layout = s_ANX7150_audio_config.audio_layout;

        //HDMI_RX_ReadI2C_RX0(0x15, &c);
        #if 0
        if((c & 0x08) ==0x08 )    //BIT[5:3]
        {
            audio_layout = 0x80;
        }
		else 
			{
				audio_layout = 0x00;
			}
		#endif
    }
    if (s_ANX7150_audio_config.audio_type & ANX7150_super_audio_input)
    {
        ANX7150_i2c_read_p0_reg(ANX7150_ONEBIT_AUD_CTRL_REG, &c);
        if( c & 0xfc)       //BIT[5:3]
        {
            audio_layout = 0x80;
        }
    }

    ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL0_REG, &c);
	c &= ~0x80;
    c |= audio_layout;
    ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL0_REG, c);

    if(  (s_ANX7150_audio_config.audio_type & 0x07) == exe_result )
    {
        //Initial N value
        
        ANX7150_i2c_read_p0_reg(ANX7150_I2SCH_STATUS4_REG, &c);
        fs = c & 0x0f;
        // set default value to N
        ACR_N = ANX7150_N_48k;
        switch(fs)
        {
            case(0x00)://44.1k
                ACR_N = ANX7150_N_44k;
                break;
            case(0x02)://48k
                ACR_N = ANX7150_N_48k;
                break;
            case(0x03)://32k
                ACR_N = ANX7150_N_32k;
                break;
            case(0x08)://88k
                ACR_N = ANX7150_N_88k;
                break;
            case(0x0a)://96k
                ACR_N = ANX7150_N_96k;
                break;
            case(0x0c)://176k
                ACR_N = ANX7150_N_176k;
                break;
            case(0x0e)://192k
                ACR_N = ANX7150_N_192k;
                break;
            default:
                ANX7150_DEBUG("note wrong fs.");
                break;
        }
        // write N(ACR) to corresponding regs
        c = ACR_N;
        ANX7150_i2c_write_p1_reg(ANX7150_ACR_N1_SW_REG, c);
        c = ACR_N>>8;
        ANX7150_i2c_write_p1_reg(ANX7150_ACR_N2_SW_REG, c);
        
        ANX7150_i2c_write_p1_reg(ANX7150_ACR_N3_SW_REG, 0x00);

        // set the relation of MCLK and Fs  xy 070117
        ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL0_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL0_REG, (c & 0xf8) | FREQ_MCLK);
        ANX7150_DEBUG("Audio MCLK input mode is: %.2x\n",(WORD)FREQ_MCLK);

        //Enable control of ACR
        ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL1_REG, &c);
        ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL1_REG, (c | ANX7150_INFO_PKTCTRL1_ACR_EN));

        //audio enable: 
        ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, &c);
        c |= ANX7150_HDMI_AUDCTRL1_IN_EN;
        ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, c);
        
        ANX7150_Set_System_State(ANX7150_CONFIG_PACKETS); 
    }

    return exe_result;

}

BYTE ANX7150_Config_I2s()
{
    BYTE exe_result = 0x00;
    BYTE c = 0x00;
    BYTE c1 = 0x00;
    
    ANX7150_DEBUG("ANX7150: config i2s audio.");

    //select SCK as source
    ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, &c);
    c &=  ~ANX7150_HDMI_AUDCTRL1_CLK_SEL;
    ANX7150_DEBUG("select SCK as source, c = 0x%.2x\n",(WORD)c);
    ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, c);

    
    //config i2s channel
    ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, &c);
    c1 = s_ANX7150_audio_config.i2s_config.audio_channel;    // need BYTE[5:2]
    c1 &= 0x3c;
    c &= ~0x3c;
    c |= c1;
    ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, c);
    ANX7150_DEBUG("config i2s channel, c = 0x%.2x\n",(WORD)c);

    //config i2s format
    //ANX7150_i2c_read_p0_reg(ANX7150_I2S_CTRL_REG, &c);
    c = s_ANX7150_audio_config.i2s_config.i2s_format;
    ANX7150_i2c_write_p0_reg(ANX7150_I2S_CTRL_REG, c);
    ANX7150_DEBUG("config i2s format, c = 0x%.2x\n",(WORD)c);

    //map i2s fifo
     
    // TODO: config I2S channel map register according to system    

    
    //ANX7150_i2c_write_p0_reg(ANX7150_I2SCH_CTRL_REG, c);

    //swap right/left channel
    /*ANX7150_i2c_read_p0_reg(ANX7150_I2SCH_SWCTRL_REG, &c);
    c1 = 0x00;
    c1 &= 0xf0;
    c &= ~0xf0;
    c |= c1;
    ANX7150_i2c_write_p0_reg(ANX7150_I2SCH_SWCTRL_REG, c);
    ANX7150_DEBUG("map i2s ffio, c = 0x%.2x\n",(WORD)c);*/
    
    //down sample   
    ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL0_REG, &c);
    c1 = s_ANX7150_audio_config.down_sample;
    c1 &= 0x60;
    c &= ~0x60;
    c |= c1;
    ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL0_REG, c);
    ANX7150_DEBUG("down sample, c = 0x%.2x\n",(WORD)c);

    //config i2s channel status(5 regs)
    c = s_ANX7150_audio_config.i2s_config.Channel_status1;
    ANX7150_i2c_write_p0_reg(ANX7150_I2SCH_STATUS1_REG, c);
    c = s_ANX7150_audio_config.i2s_config.Channel_status2;
    ANX7150_i2c_write_p0_reg(ANX7150_I2SCH_STATUS2_REG, c);
    c = s_ANX7150_audio_config.i2s_config.Channel_status3;
    ANX7150_i2c_write_p0_reg(ANX7150_I2SCH_STATUS3_REG, c);
    c = s_ANX7150_audio_config.i2s_config.Channel_status4;
    ANX7150_i2c_write_p0_reg(ANX7150_I2SCH_STATUS4_REG, c);
	//ANX7150_DEBUG("@@@@@@@@config i2s channel status4, c = 0x%.2x\n",(unsigned int)c);//

    c = s_ANX7150_audio_config.i2s_config.Channel_status5;
    ANX7150_i2c_write_p0_reg(ANX7150_I2SCH_STATUS5_REG, c);
    ANX7150_DEBUG("config i2s channel status, c = 0x%.2x\n",(WORD)c);
    
    exe_result = ANX7150_i2s_input;
    //ANX7150_DEBUG("return = 0x%.2x\n",(WORD)exe_result);

    // open corresponding interrupt
    //ANX7150_i2c_read_p0_reg(ANX7150_INTR1_MASK_REG, &c);
    //ANX7150_i2c_write_p0_reg(ANX7150_INTR1_MASK_REG, (c | 0x22) );
    //ANX7150_i2c_read_p0_reg(ANX7150_INTR3_MASK_REG, &c);
    //ANX7150_i2c_write_p0_reg(ANX7150_INTR3_MASK_REG, (c | 0x20) );

    
    return exe_result;
}



BYTE ANX7150_Config_Spdif()
{
    BYTE exe_result = 0x00;
    BYTE c = 0x00;
    BYTE c1 = 0x00;
    BYTE c2 = 0x00;
    BYTE freq_mclk = 0x00;

    ANX7150_DEBUG("ANX7150: config SPDIF audio.");

        
    //Select MCLK
    ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, &c);
    c |= ANX7150_HDMI_AUDCTRL1_CLK_SEL;
    ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, c);

    //ANX7150_DEBUG("ANX7150: enable SPDIF audio.");
    //Enable SPDIF
    ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, &c);
    c |= ANX7150_HDMI_AUDCTRL1_SPDIFIN_EN;
    ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, c);

    //adjust MCLK phase in interrupt routine

    // adjust FS_FREQ   //FS_FREQ 
    c1 = s_ANX7150_audio_config.i2s_config.Channel_status4 & 0x0f;
    ANX7150_i2c_read_p0_reg(ANX7150_SPDIFCH_STATUS_REG, &c);
    c &= ANX7150_SPDIFCH_STATUS_FS_FREG;
    c = c >> 4;
    
    if( c != c1)
    {
        //ANX7150_DEBUG("adjust FS_FREQ by system!");
        ANX7150_i2c_read_p0_reg(ANX7150_I2SCH_STATUS4_REG, &c);
        c &= 0xf0;
        c |= c1;
        ANX7150_i2c_write_p0_reg(ANX7150_I2SCH_STATUS4_REG, c);
        
        //enable using FS_FREQ from 0x59
        ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, &c);
        c |= 0x02;
        ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, c);
    }
    
    // down sample
    ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL0_REG, &c);
    c1 = s_ANX7150_audio_config.down_sample;
    c1 &= 0x60;
    c &= ~0x60;
    c |= c1;
    ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL0_REG, c);

    if(s_ANX7150_audio_config.down_sample)      //zy 060816
    {
        // adjust FS_FREQ by system because down sample
        //ANX7150_DEBUG("adjust FS_FREQ by system because down sample!");
        
        c1 = s_ANX7150_audio_config.i2s_config.Channel_status4 & 0x0f;
        ANX7150_i2c_read_p0_reg(ANX7150_I2SCH_STATUS4_REG, &c);
        c &= 0xf0;
        c |= c1;
        ANX7150_i2c_write_p0_reg(ANX7150_I2SCH_STATUS4_REG, c);
    }
    
    
    // spdif is stable
    ANX7150_DEBUG("config SPDIF audio done");
    exe_result = ANX7150_spdif_input;

    // open corresponding interrupt
    ANX7150_i2c_read_p0_reg(ANX7150_INTR1_MASK_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_INTR1_MASK_REG, (c | 0x32) );
    //ANX7150_i2c_read_p0_reg(ANX7150_INTR3_MASK_REG, &c);
    //ANX7150_i2c_write_p0_reg(ANX7150_INTR3_MASK_REG, (c | 0xa1) );
    return exe_result;
}

BYTE ANX7150_Config_Super_Audio()
{
    BYTE exe_result = 0x00;
    BYTE c = 0x00;


    //ANX7150_DEBUG("ANX7150: config one BIT audio.");

    // select sck as source
    ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, &c);
    c &=  ~ANX7150_HDMI_AUDCTRL1_CLK_SEL;
    ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, c);

    // Enable stream  0x60
    c = s_ANX7150_audio_config.super_audio_config.one_BIT_ctrl;
    ANX7150_i2c_write_p0_reg(ANX7150_ONEBIT_AUD_CTRL_REG, c);

    
    // Map stream 0x61  
    // TODO: config super audio  map register according to system    
    
    exe_result = ANX7150_super_audio_input;
    return exe_result;

}

//*************** Config Packet ****************************
BYTE ANX7150_Config_Packet()
{
    BYTE exe_result = 0x00;     // There is no use in current solution
    BYTE info_packet_sel;
    BYTE c;

    
    info_packet_sel = s_ANX7150_packet_config.packets_need_config;
    ANX7150_DEBUG("info_packet_sel = 0x%.2x\n",(WORD) info_packet_sel);
    // New packet?
    if( info_packet_sel != 0x00)
    { 
    // avi infoframe
        if( info_packet_sel & ANX7150_avi_sel )
        {  
            c = s_ANX7150_packet_config.avi_info.pb_byte[1];  //color space
            c &= 0x9f;
            c |= (ANX7150_RGBorYCbCr << 5);
            s_ANX7150_packet_config.avi_info.pb_byte[1] = c | 0x10;
            c = s_ANX7150_packet_config.avi_info.pb_byte[4];// vid ID
            c = c & 0x80;
            s_ANX7150_packet_config.avi_info.pb_byte[4] = c | ANX7150_video_timing_id;
            c = s_ANX7150_packet_config.avi_info.pb_byte[5]; //repeat times
            c = c & 0xf0;
            c |= (ANX7150_tx_pix_rpt & 0x0f);
            s_ANX7150_packet_config.avi_info.pb_byte[5] = c;
            ANX7150_DEBUG("config avi infoframe packet."); 
            // Disable repeater
            ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL1_REG, &c);

            c &= ~ANX7150_INFO_PKTCTRL1_AVI_RPT;
            ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL1_REG, c);

            // Enable?wait:go
            ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL1_REG, &c);
            if(c & ANX7150_INFO_PKTCTRL1_AVI_EN)
            {
                //ANX7150_DEBUG("wait disable, config avi infoframe packet.");
                //return exe_result; //
            }

            // load packet data to regs
            ANX7150_Load_Infoframe( ANX7150_avi_infoframe,
                                    &(s_ANX7150_packet_config.avi_info));
            // Enable and repeater
            ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL1_REG, &c);
            c |= 0x30;
            ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL1_REG, c);

            // complete avi packet
            ANX7150_DEBUG("config avi infoframe packet done.");
            s_ANX7150_packet_config.packets_need_config &= ~ANX7150_avi_sel;
            
        }
        
    // audio infoframe
        if( info_packet_sel & ANX7150_audio_sel )
        {
            ANX7150_DEBUG("config audio infoframe packet.");

            // Disable repeater
            ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL2_REG, &c);
            c &= ~ANX7150_INFO_PKTCTRL2_AIF_RPT;
            ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL2_REG, c);
            
            // Enable?wait:go
            ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL2_REG, &c);
            if(c & ANX7150_INFO_PKTCTRL2_AIF_EN)
            {
                //ANX7150_DEBUG("wait disable, config audio infoframe packet.");
                //return exe_result;//
            }
            // config packet

            // load packet data to regs
            ANX7150_Load_Infoframe( ANX7150_audio_infoframe,
                                    &(s_ANX7150_packet_config.audio_info));
            // Enable and repeater
            ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL2_REG, &c);
            c |= 0x03;
            ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL2_REG, c);

            // complete avi packet

            ANX7150_DEBUG("config audio infoframe packet done.");
            s_ANX7150_packet_config.packets_need_config &= ~ANX7150_audio_sel;
            
        }

        // config other 4 packets
/*
        
        if( info_packet_sel & 0xfc )
        {
            ANX7150_DEBUG("other packets.");

            //find the current type need config
            if(info_packet_sel & ANX7150_spd_sel)    type_sel = ANX7150_spd_sel;
            else if(info_packet_sel & ANX7150_mpeg_sel)    type_sel = ANX7150_mpeg_sel;
            else if(info_packet_sel & ANX7150_acp_sel)    type_sel = ANX7150_acp_sel;
            else if(info_packet_sel & ANX7150_isrc1_sel)    type_sel = ANX7150_isrc1_sel;
            else if(info_packet_sel & ANX7150_isrc2_sel)    type_sel = ANX7150_isrc2_sel;
            else  type_sel = ANX7150_vendor_sel;
            

            // Disable repeater
            ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL2_REG, &c);
            c &= ~ANX7150_INFO_PKTCTRL2_AIF_RPT;
            ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL2_REG, c);

            switch(type_sel)
            {
                case ANX7150_spd_sel:
                    ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL1_REG, &c);
                    c &= ~ANX7150_INFO_PKTCTRL1_SPD_RPT;
                    ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL1_REG, c);

                    ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL1_REG, &c);
                    if(c & ANX7150_INFO_PKTCTRL1_SPD_EN)
                    {
                        ANX7150_DEBUG("wait disable, config spd infoframe packet.");
                        return exe_result;
                    }
                    break;
                    
                case ANX7150_mpeg_sel:
                    ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL2_REG, &c);
                    c &= ~ANX7150_INFO_PKTCTRL2_MPEG_RPT;
                    ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL2_REG, c);
                    
                    ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL2_REG, &c);
                    if(c & ANX7150_INFO_PKTCTRL2_MPEG_EN)
                    {
                        ANX7150_DEBUG("wait disable, config mpeg infoframe packet.");
                        return exe_result;
                    }
                    break;
                    
                case ANX7150_acp_sel:
                    ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL2_REG, &c);
                    c &= ~ANX7150_INFO_PKTCTRL2_UD0_RPT;
                    ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL2_REG, c);

                    ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL2_REG, &c);
                    if(c & ANX7150_INFO_PKTCTRL2_UD0_EN)
                    {
                        ANX7150_DEBUG("wait disable, config mpeg infoframe packet.");
                        return exe_result;
                    }
                    break;
                    
                case ANX7150_isrc1_sel:
                    ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL2_REG, &c);
                    c &= ~ANX7150_INFO_PKTCTRL2_UD0_RPT;
                    ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL2_REG, c);
                    ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL2_REG, &c);
                    if(c & ANX7150_INFO_PKTCTRL2_UD0_EN)
                    {
                        ANX7150_DEBUG("wait disable, config isrc1 packet.");
                        return exe_result;
                    }
                    break;
                    
                case ANX7150_isrc2_sel:
                    ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL2_REG, &c);
                    c &= ~ANX7150_INFO_PKTCTRL2_UD1_RPT;
                    ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL2_REG, c);
                    ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL2_REG, &c);
                    if(c & ANX7150_INFO_PKTCTRL2_UD1_EN)
                    {
                        ANX7150_DEBUG("wait disable, config isrc2 packet.");
                        return exe_result;
                    }
                    break;
                    
                case ANX7150_vendor_sel:
                    ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL2_REG, &c);
                    c &= ~ANX7150_INFO_PKTCTRL2_UD1_RPT;
                    ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL2_REG, c);
                    ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL2_REG, &c);
                    if(c & ANX7150_INFO_PKTCTRL2_UD1_EN)
                    {
                        ANX7150_DEBUG("wait disable, config vendor packet.");
                        return exe_result;
                    }
                    break;
                    
                default : break;
            }   
                
      
            // config packet
            // TODO: config packet in top level

            // load packet data to regs
            switch(type_sel)
            {
                case ANX7150_spd_sel:
                    ANX7150_Load_Infoframe( ANX7150_spd_infoframe,
                                            &(s_ANX7150_packet_config.spd_info));
                    ANX7150_DEBUG("config spd done.");
                    ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL1_REG, &c);
                    c |= 0xc0;
                    ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL1_REG, c);
                    break;
                    
                case ANX7150_mpeg_sel:
                    ANX7150_Load_Infoframe( ANX7150_mpeg_infoframe,
                                            &(s_ANX7150_packet_config.mpeg_info));
                    ANX7150_DEBUG("config mpeg done.");
                    ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL2_REG, &c);
                    c |= 0x0c;
                    ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL2_REG, c);
                    break;
                    
                case ANX7150_acp_sel:
                    ANX7150_Load_Packet( ANX7150_acp_packet,
                                            &(s_ANX7150_packet_config.acp_pkt));
                    ANX7150_DEBUG("config acp done.");
                    ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL2_REG, &c);
                    c |= 0x30;
                    ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL2_REG, c);
                    break;
                    
                case ANX7150_isrc1_sel:
                    ANX7150_Load_Packet( ANX7150_isrc1_packet,
                                            &(s_ANX7150_packet_config.acp_pkt));
                    ANX7150_DEBUG("config isrc1 done.");
                    ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL2_REG, &c);
                    c |= 0x30;
                    ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL2_REG, c);
                    break;
                    
                case ANX7150_isrc2_sel:
                    ANX7150_Load_Packet( ANX7150_isrc2_packet,
                                            &(s_ANX7150_packet_config.acp_pkt));
                    ANX7150_DEBUG("config isrc2 done.");
                    ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL2_REG, &c);
                    c |= 0xc0;
                    ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL2_REG, c);
                    break;
                    
                case ANX7150_vendor_sel:
                    ANX7150_Load_Infoframe( ANX7150_vendor_infoframe,
                                            &(s_ANX7150_packet_config.vendor_info));
                    ANX7150_DEBUG("config vendor done.");
                    ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL2_REG, &c);
                    c |= 0xc0;
                    ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL2_REG, c);
                    break;
                    
                default : break;
            }   

            // Enable and repeater
            ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL2_REG, &c);
            c |= 0x03;
            ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL2_REG, c);

            // complete config packet
            ANX7150_DEBUG("config other packets done.");
            s_ANX7150_packet_config.packets_need_config &= ~type_sel;
            
        }
        */
    }

	
    if( s_ANX7150_packet_config.packets_need_config  == 0x00)
    {
         ANX7150_DEBUG("config packets done\n");
         ANX7150_Set_System_State(ANX7150_HDCP_AUTHENTICATION);
    }

    
    return exe_result;
}



BYTE ANX7150_Load_Infoframe(packet_type member,
                            infoframe_struct *p)
{
    BYTE exe_result = 0x00;
    BYTE address[8] = {0x00,0x20,0x40,0x60,0x80,0x80,0xa0,0xa0};
    BYTE i;
    BYTE c;

    p->pb_byte[0] = ANX7150_Checksum(p);        

    // write infoframe to according regs
    ANX7150_i2c_write_p1_reg(address[member], p->type);
    ANX7150_i2c_write_p1_reg(address[member]+1, p->version);
    ANX7150_i2c_write_p1_reg(address[member]+2, p->length);
    
    for(i=0; i <= p->length; i++)
    {
        ANX7150_i2c_write_p1_reg(address[member]+3+i, p->pb_byte[i]);
        ANX7150_i2c_read_p1_reg(address[member]+3+i, &c);
    }
    return exe_result;
}

BYTE ANX7150_Checksum(infoframe_struct *p)
{
    BYTE checksum = 0x00;
    BYTE i;

    checksum = p->type + p->length + p->version;
    for(i=1; i <= p->length; i++)
    {
        checksum += p->pb_byte[i];
    }
    checksum = ~checksum;
    checksum += 0x01;

    return checksum;
}

/*
BYTE ANX7150_Load_Packet(packet_type member,
                            infoframe_struct *p)
{
    BYTE exe_result = 0x00;
    BYTE address[8] = {0x00,0x20,0x40,0x60,0x80,0x80,0xa0,0xa0};
    BYTE i;

    ANX7150_DEBUG("address  = 0x%.2x\n",(WORD) address[member]);

    // write packet to according regs
    ANX7150_i2c_write_p1_reg(address[member], p->type);

    ANX7150_i2c_write_p1_reg(address[member]+1, p->version);
    
    ANX7150_i2c_write_p1_reg(address[member]+2, p->length);
    
    for(i=0; i < 28; i++)
    {
      ANX7150_i2c_write_p1_reg(address[member]+3+i, p->pb_byte[i]);
    }
    return exe_result;
}
*/
//***************  end of Config Packet ****************************

//******************** HDCP process ********************************

void ANX7150_HDCP_Process(void)
{
    BYTE c;
	BYTE Bksv_valid=0;//HDCP CTS
    ANX7150_DEBUG("############ANX7150_HDCP_Process, ANX7150_HDCP_enable=%d\n",ANX7150_HDCP_enable);
    if(ANX7150_HDCP_enable) 
    { //HDCP_EN =1 means to do HDCP authentication,SWITCH4 = 0 means not to do HDCP authentication.
    
		ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL1_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL1_REG, c | 0x04);//power on HDCP, 090630

		ANX7150_i2c_read_p0_reg(ANX7150_INTR2_MASK_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_INTR2_MASK_REG, c |0x03);  
		msleep(10);//let unencrypted video play a while, required by HDCP CTS. SY//HDCP CTS
        ANX7150_Set_AVMute();//before auth, set_avmute
		msleep(10);//HDCP CTS
         
        if( !ANX7150_hdcp_init_done )
        {
				ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL1_REG, &c); //72:07.2 hdcp on//HDCP CTS
				ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL1_REG, (c | ANX7150_SYS_CTRL1_HDCPMODE));//HDCP CTS
            if(ANX7150_edid_result.is_HDMI)
	                ANX7150_Hardware_HDCP_Auth_Init();
            else
            {   //DVI, disable 1.1 feature and enable HDCP two special point check
	                ANX7150_i2c_read_p0_reg(ANX7150_HDCP_CTRL1_REG, &c);
	                ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL1_REG, 
	                                                     (c & (~ANX7150_HDCP_CTRL1_HDCP11_EN) | ANX7150_LINK_CHK_12_EN));
            }

			//HDCP CTS
			if(!ANX7150_BKSV_SRM())
			{
				ANX7150_Blue_Screen_Enable();
				ANX7150_Clear_AVMute();
				Bksv_valid=0;
				return;
			}
			else //SY.
			{
				Bksv_valid=1;
				ANX7150_i2c_read_p0_reg(ANX7150_HDCP_CTRL0_REG, &c);
				ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL0_REG, (c | 0x03));
			}

            ANX7150_hdcp_init_done = 1;
//HDCP CTS		  
        }


//HDCP CTS	
		if((Bksv_valid) && (!ANX7150_hdcp_auth_en))
        {
				ANX7150_DEBUG("enable hw hdcp");
				ANX7150_RST_DDCChannel();
				ANX7150_i2c_read_p0_reg(ANX7150_HDCP_CTRL0_REG, &c);
				ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL0_REG, (c | ANX7150_HDCP_CTRL0_HW_AUTHEN));
				ANX7150_hdcp_auth_en = 1;
        }

		 if((Bksv_valid) && (ANX7150_hdcp_wait_100ms_needed))
        {
            ANX7150_hdcp_wait_100ms_needed = 0;
            //disable audio
            ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, c & (~ANX7150_HDMI_AUDCTRL1_IN_EN));
            ANX7150_DEBUG("++++++++ANX7150_hdcp_wait_100ms_needed----------+++++++++\n");
            msleep(100);				          
			return;
        }
//HDCP CTS	

        if(ANX7150_hdcp_auth_pass) //HDCP CTS
        {
				//Clear the SRM_Check_Pass BIT, then when reauthentication occurs, firmware can catch it.
				ANX7150_i2c_read_p0_reg(ANX7150_HDCP_CTRL0_REG, &c);
				ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL0_REG, c & 0xfc);

            //Enable HDCP Hardware encryption
            if(!ANX7150_hdcp_encryption)
            {
                ANX7150_HDCP_Encryption_Enable();
            }
            if(ANX7150_send_blue_screen)
            {
                ANX7150_Blue_Screen_Disable();
            }
		     if(ANX7150_avmute_enable)
		     {
				  ANX7150_Clear_AVMute();
		     }
            //enable audio SY.
            ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, &c);
            c |= ANX7150_HDMI_AUDCTRL1_IN_EN;
            ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, c);
            ANX7150_DEBUG("@@@@@  HDCP Auth PASSED!   @@@@@\n");

			if (ANX7150_hdcp_bcaps & 0x40) //repeater
			{
				ANX7150_DEBUG("Find a repeater!");
				//actually it is KSVList check. we can't do SRM check due to the lack of SRM file. SY.
				if(!ANX7150_srm_checked)
				{
					if (!ANX7150_IS_KSVList_VLD())
					{
						ANX7150_DEBUG("ksvlist not good. disable encryption");
						ANX7150_HDCP_Encryption_Disable();
						ANX7150_Blue_Screen_Enable();
						ANX7150_Clear_AVMute();
						ANX7150_ksv_srm_pass = 0;
						ANX7150_Clean_HDCP();//SY.
						//remove below will pass 1b-05/1b-06
						//ANX7150_Set_System_State(ANX7150_WAIT_HOTPLUG);//SY.
						return;
					}
					ANX7150_srm_checked=1;
					ANX7150_ksv_srm_pass = 1;
				}
			}
			else
			{
				ANX7150_DEBUG("Find a receiver.");

			}

#if 1
				ANX7150_i2c_read_p0_reg(ANX7150_HDCP_CTRL0_REG, &c);
				ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL0_REG, (c| ANX7150_HDCP_CTRL0_SW_AUTHOK) & (~ANX7150_HDCP_CTRL0_HW_AUTHEN));//dis hw hdcp, en sw hdcp


#endif


			
        }
        else 							//HDCP CTS
        {
            ANX7150_DEBUG("#####   HDCP Auth FAILED!   #####\n");
		//also need to disable HW AUTHEN
				//ANX7150_hdcp_auth_en = 0;
				//ANX7150_hdcp_init_done = 0;
				//ANX7150_hdcp_wait_100ms_needed = 1; //update 080703
		
		     if(ANX7150_hdcp_encryption)
            {
                ANX7150_HDCP_Encryption_Disable();
            }
            if(!ANX7150_send_blue_screen)
            {
                ANX7150_Blue_Screen_Enable();
            }
            if(ANX7150_avmute_enable)
            {
                ANX7150_Clear_AVMute();
            }
            //disable audio
            ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, c & (~ANX7150_HDMI_AUDCTRL1_IN_EN));

			return;
        }

    }
    else				//HDCP CTS
    {
		ANX7150_DEBUG("hdcp pin is off.");
		if(ANX7150_send_blue_screen)
		{
			ANX7150_Blue_Screen_Disable();
		}
		if(ANX7150_avmute_enable)
		{
			ANX7150_Clear_AVMute();
		}
		//enable audio SY.
		ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, &c);
		c |= ANX7150_HDMI_AUDCTRL1_IN_EN;
		ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, c);
	}

//HDCP CTS
		ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL1_REG, &c); //72:07.1 hdmi or dvi mode
    	c = c & 0x02;
    	if(c == 0x02)
		{
      		ANX7150_DEBUG("end of ANX7150_HDCP_Process(): in HDMI mode.");
		}
    	else
    	{
			ANX7150_DEBUG("!end of ANX7150_HDCP_Process(): in DVI mode.");
			//To-Do: Config to DVI mode.
    	}
		ANX7150_Set_System_State(ANX7150_PLAY_BACK);
		ANX7150_Show_Video_Parameter();
//HDCP CTS
                
}
//******************** end of HDCP process ********************************


//************************Play back process   **************************
void ANX7150_PLAYBACK_Process(void)
{
    BYTE c;
        ANX7150_DEBUG("####ANX7150_PLAYBACK_Process enter\n");
    if((s_ANX7150_packet_config.packets_need_config != 0x00) && (ANX7150_edid_result.is_HDMI == 1))
    {
        ANX7150_DEBUG("####ANX7150_PLAYBACK_Process CONFIG PACKETS\n");
        ANX7150_Set_System_State(ANX7150_CONFIG_PACKETS);
    }
    
}
//******************** end of Play back process ********************************

void ANX7150_RST_DDCChannel(void)
{
    BYTE c;
    //Reset the DDC channel
    ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL2_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL2_REG, (c | ANX7150_SYS_CTRL2_DDC_RST));
    ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL2_REG, (c & (~ANX7150_SYS_CTRL2_DDC_RST)));
    ANX7150_i2c_write_p0_reg(ANX7150_DDC_ACC_CMD_REG, 0x00); //abort current operation
    ANX7150_i2c_write_p0_reg(ANX7150_DDC_ACC_CMD_REG, 0x06);//reset I2C command
//Clear FIFO
    ANX7150_i2c_write_p0_reg(ANX7150_DDC_ACC_CMD_REG, 0x05);
}


BYTE ANX7150_BKSV_SRM(void) 
{

    #if 1
    BYTE bksv[5],i,bksv_one,c1;
    ANX7150_InitDDC_Read(0x74, 0x00, 0x00, 0x05, 0x00);
    msleep(15);
    for(i = 0; i < 5; i ++)
    {
         ANX7150_i2c_read_p0_reg(ANX7150_DDC_FIFO_ACC_REG, &bksv[i]);      
    }
    
    bksv_one = 0;
    for(i = 0; i < 8; i++)
    {
        c1 = 0x01 << i;
        if(bksv[0] & c1)
            bksv_one ++;
        if(bksv[1] & c1)
            bksv_one ++;
        if(bksv[2] & c1)
            bksv_one ++;
        if(bksv[3] & c1)
            bksv_one ++;
        if(bksv[4] & c1)
            bksv_one ++;
    }
    //HDCP CTS
    if(bksv_one != 20)
    {
        ANX7150_DEBUG("BKSV check fail");
        return 0;
    }
    else
    {
		ANX7150_DEBUG("BKSV check OK");
    	return 1;
	}
    #endif

#if 0//HDCP CTS

    BYTE i,j,bksv_ones_count,bksv_data[Bksv_Data_Nums] = {0};
    ANX7150_DDC_Addr bksv_ddc_addr;
    WORD bksv_length;
    ANX7150_DDC_Type ddc_type;

    i = 0;
    j = 0;
    bksv_ones_count = 0;
    bksv_ddc_addr.dev_addr = HDCP_Dev_Addr;
    bksv_ddc_addr.sgmt_addr = 0;
    bksv_ddc_addr.offset_addr = HDCP_Bksv_Offset;
    bksv_length = Bksv_Data_Nums;
    ddc_type = DDC_Hdcp;
    
    if (!ANX7150_DDC_Read(bksv_ddc_addr, bksv_data, bksv_length, ddc_type)){
        /*Judge validity for Bksv*/
        while (i < Bksv_Data_Nums){
            while (j < 8){
                if (((bksv_data[i] >> j) & 0x01) == 1){
                    bksv_ones_count++;
                }
                j++;
                }
            i++;
            j = 0;
            }
        if (bksv_ones_count != 20){
            ANX7150_DEBUG ("!!!!BKSV 1s ¡Ù20\n");					//update  ANX7150_DEBUG ("!!!!BKSV 1s ¡Ù20\n");
            return 0;
        }
    }
    /*end*/
  
    ANX7150_DEBUG("bksv is ready.");
    // TODO: Compare the bskv[] value to the revocation list to decide if this value is a illegal BKSV. This is system depended.
    //If illegal, return 0; legal, return 1. Now just return 1
    return 1;
    #endif
}

BYTE ANX7150_IS_KSVList_VLD(void)
{
//HDCP CTS
#if 1
    ANX7150_DEBUG("ANX7150_IS_KSVList_VLD() is called.");
    ANX7150_InitDDC_Read(0x74, 0x00, 0x41, 0x02, 0x00); //Bstatus, two bytes
    msleep(5);
    ANX7150_i2c_read_p0_reg(ANX7150_DDC_FIFO_ACC_REG, &ANX7150_hdcp_bstatus[0]);
    ANX7150_i2c_read_p0_reg(ANX7150_DDC_FIFO_ACC_REG, &ANX7150_hdcp_bstatus[1]);
    if((ANX7150_hdcp_bstatus[0] & 0x80) | (ANX7150_hdcp_bstatus[1] & 0x08))
    {
        ANX7150_DEBUG("Max dev/cascade exceeded: ANX7150_hdcp_bstatus[0]: 0x%x,ANX7150_hdcp_bstatus[1]:0x%x\n", (WORD)ANX7150_hdcp_bstatus[0],(WORD)ANX7150_hdcp_bstatus[1]);
        return 0;//HDCP topology error. More than 127 RX are attached or more than seven levels of repeater are cascaded.
	}
	return 1;
   #endif
//HDCP CTS

    
}

void ANX7150_Hardware_HDCP_Auth_Init(void) 
{
    BYTE c;

    #if 1
    ANX7150_DDC_Type DDC_type;
    ANX7150_DDC_Addr ddc_bcaps_address;
    BYTE BCaps_Nums;
    #endif
    /*end*/
    ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL0_REG, 0x00);  
    // disable hdcp
    ANX7150_i2c_read_p0_reg(ANX7150_HDCP_CTRL0_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL0_REG, (c & (~ANX7150_HDCP_CTRL0_HW_AUTHEN)));
    

   #if 0
    // DDC reset
    ANX7150_RST_DDCChannel();

    ANX7150_InitDDC_Read(0x74, 0x00, 0x40, 0x01, 0x00);
    msleep(5);
    ANX7150_i2c_read_p0_reg(ANX7150_DDC_FIFO_ACC_REG, &ANX7150_hdcp_bcaps);
    ANX7150_DEBUG("ANX7150_hdcp_bcaps = 0x%.2x\n",
    (WORD)ANX7150_hdcp_bcaps);
    #endif


    #if 1
    DDC_type = DDC_Hdcp;
    ddc_bcaps_address.dev_addr = HDCP_Dev_Addr;
    ddc_bcaps_address.sgmt_addr = 0;
    ddc_bcaps_address.offset_addr = HDCP_Bcaps_Offset;
    BCaps_Nums = 1;

    if (!ANX7150_DDC_Read(ddc_bcaps_address, &ANX7150_hdcp_bcaps, BCaps_Nums, DDC_type)){
            ANX7150_DEBUG ("!!!!ANX7150_hdcp_bcaps = 0x%.2x\n", (WORD)ANX7150_hdcp_bcaps);
    }
    #endif
    /*end*/
   
    if(ANX7150_hdcp_bcaps & 0x02)  
    {   //enable 1.1 feature
        ANX7150_i2c_read_p0_reg(ANX7150_HDCP_CTRL1_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL1_REG, (c |ANX7150_HDCP_CTRL1_HDCP11_EN));
    } 
    else 
    {   //disable 1.1 feature and enable HDCP two special point check
        ANX7150_i2c_read_p0_reg(ANX7150_HDCP_CTRL1_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL1_REG, 
                                                     (c & (~ANX7150_HDCP_CTRL1_HDCP11_EN) | ANX7150_LINK_CHK_12_EN));
    }
    ANX7150_RST_DDCChannel();
    ANX7150_hdcp_auth_en = 0;
}


void ANX7150_Clean_HDCP(void)
{
    BYTE c;
    //mute TMDS link
    //ANX7150_i2c_read_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, &c);
    //ANX7150_i2c_write_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, c & (~ANX7150_TMDS_CLKCH_MUTE));

    //Disable hardware HDCP
    ANX7150_i2c_read_p0_reg(ANX7150_HDCP_CTRL0_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL0_REG, (c & (~ANX7150_HDCP_CTRL0_HW_AUTHEN)));

    //Reset HDCP logic
    ANX7150_i2c_read_p0_reg(ANX7150_SRST_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_SRST_REG, (c | ANX7150_SRST_HDCP_RST) );
    ANX7150_i2c_write_p0_reg(ANX7150_SRST_REG, (c & (~ANX7150_SRST_HDCP_RST)) );
    
    //Set ReAuth
    ANX7150_i2c_read_p0_reg(ANX7150_HDCP_CTRL0_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL0_REG, c |ANX7150_HDCP_CTRL0_RE_AUTH);
    ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL0_REG, c & (~ANX7150_HDCP_CTRL0_RE_AUTH));
    ANX7150_hdcp_auth_en = 0;
    //ANX7150_bksv_ready = 0;
    ANX7150_hdcp_auth_pass = 0;
    ANX7150_hdcp_auth_fail_counter =0 ;
    ANX7150_hdcp_encryption = 0;
    ANX7150_send_blue_screen = 0;
    ANX7150_hdcp_init_done = 0;
    ANX7150_hdcp_wait_100ms_needed = 1;
    ANX7150_auth_fully_pass = 0;
    ANX7150_srm_checked = 0;
    ANX7150_RST_DDCChannel();      
}

void ANX7150_Hardware_Reset() 
{

//    ANX7150_Resetn_Pin = 0;
//    msleep(20);
//    ANX7150_Resetn_Pin = 1;
//    msleep(10);

    //ANX7150_DEBUG("ANX7150 hardware reset OK");
     ANX7150_i2c_write_p0_reg( 0x08, 0x01);
     msleep(30);
     ANX7150_DEBUG("ANX7150 hardware reset OK\n");

}

void ANX7150_Set_System_State(BYTE ss) 
{
    ANX7150_system_state = ss;
    ANX7150_DEBUG("ANX7150 To System State: ");
    switch (ss) 
    {
        case ANX7150_INITIAL:
            ANX7150_DEBUG("ANX7150_INITIAL");
            break;
        case ANX7150_WAIT_HOTPLUG: 
            ANX7150_DEBUG("ANX7150_WAIT_HOTPLUG");
            break;
        case ANX7150_READ_PARSE_EDID:
            ANX7150_DEBUG("ANX7150_READ_PARSE_EDID");
            break;
        case ANX7150_WAIT_RX_SENSE:
            ANX7150_DEBUG("ANX7150_WAIT_RX_SENSE");
            break;
        case ANX7150_CONFIG_VIDEO:
            ANX7150_DEBUG("ANX7150_CONFIG_VIDEO");
            break;
        case ANX7150_CONFIG_AUDIO:
            ANX7150_DEBUG("ANX7150_CONFIG_AUDIO");
            break;
        case ANX7150_CONFIG_PACKETS:
            ANX7150_DEBUG("ANX7150_CONFIG_PACKETS");
            break;		
        case ANX7150_HDCP_AUTHENTICATION:
            ANX7150_DEBUG("ANX7150_HDCP_AUTHENTICATION");
            break;
        ////////////////////////////////////////////////
        // System ANX7150_RESET_LINK is kept for RX clock recovery error case, not used in normal case.
        case ANX7150_RESET_LINK:
            ANX7150_DEBUG("ANX7150_RESET_LINK");
            break;
        ////////////////////////////////////////////////        
        case ANX7150_PLAY_BACK:
            ANX7150_DEBUG("ANX7150_PLAY_BACK");
            break;
    }	
}

void ANX7150_Hardware_Initial() 
{
    BYTE c;
    ANX7150_DEBUG("+++ANX7150_Hardware_Initial+++\n");
    ANX7150_Hardware_Reset();
    //Power on I2C
    //ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL3_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL3_REG, 0x02);//(c | ANX7150_SYS_CTRL3_I2C_PWON));

    ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL2_REG, 0x00);
    ANX7150_i2c_write_p0_reg(ANX7150_SRST_REG, 0x00);
    //clear HDCP_HPD_RST
    ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL1_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL1_REG, c & 0xbf);
    //Power on Audio capture and Video capture module clock
    ANX7150_i2c_read_p0_reg(ANX7150_SYS_PD_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_SYS_PD_REG, (c | 0x06));
    

    //Enable auto set clock range for video PLL
    ANX7150_i2c_read_p0_reg(ANX7150_CHIP_CTRL_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_CHIP_CTRL_REG, (c & 0xfe));

    //Set registers value of Blue Screen when HDCP authentication failed--RGB mode,green field
    ANX7150_i2c_write_p0_reg(ANX7150_HDCP_BLUESCREEN0_REG, 0x10);
    ANX7150_i2c_write_p0_reg(ANX7150_HDCP_BLUESCREEN1_REG, 0xeb);
    ANX7150_i2c_write_p0_reg(ANX7150_HDCP_BLUESCREEN2_REG, 0x10);

        ANX7150_DEBUG("ANX7150_Hardware_Initial--->|||||||||0x80\n");   
	ANX7150_i2c_read_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, &c);
        ANX7150_DEBUG("ANX7150_TMDS_CLKCH_CONFIG_REG=%x\n",c);   
    ANX7150_i2c_write_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, (c | 0x80));
		
    ANX7150_i2c_read_p0_reg(ANX7150_PLL_CTRL0_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_PLL_CTRL0_REG, 0xa8);//(c & 0xf3)); //change to 0xa8

	ANX7150_i2c_read_p0_reg(ANX7150_PLL_TX_AMP, &c);
	ANX7150_i2c_write_p0_reg(ANX7150_PLL_TX_AMP, (c | 0x01));//TMDS swing

	ANX7150_i2c_write_p0_reg(ANX7150_PLL_CTRL1_REG,0x00);//Added for PLL unlock issue in high temperature - Feiw
    //if (ANX7150_AUD_HW_INTERFACE == 0x02) //spdif
    if((QFN48_EN ==1) && (s_ANX7150_audio_config.audio_type == 0x02))
    {
			ANX7150_i2c_read_p0_reg(ANX7150_I2S_CTRL_REG, &c);//for spdif input from SD0.
			ANX7150_i2c_write_p0_reg(ANX7150_I2S_CTRL_REG, (c | 0x10));//
	}
	else
	{
			ANX7150_i2c_read_p0_reg(ANX7150_I2S_CTRL_REG, &c);//for spdif input from SD0.
			ANX7150_i2c_write_p0_reg(ANX7150_I2S_CTRL_REG, (c&  0xef));//
	}

		//ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL1_REG, &c);
		ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL1_REG, 0);//c & 0xfb);//power down HDCP, 090630
		ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL3_REG, &c);
		ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL3_REG, c & 0xfe);//power down all, 090630
	
    ANX7150_Set_System_State(ANX7150_WAIT_HOTPLUG);
    ANX7150_DEBUG("---ANX7150_Hardware_Initial---\n");
}

void ANX7150_API_Initial() 
{
    ANX7150_DEBUG("+++++++++++ANX7150_API_Initial()++++++++++ \n"); 
    ANX7150_Variable_Initial(); 
    ANX7150_HW_Interface_Variable_Initial();
    ANX7150_Hardware_Initial();
}
 
/*void ANX7150_Interrupt_Information(BYTE c, BYTE n) 
{
    BYTE TX_is_HDMI,c1;
    
    ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL1_REG, &c1);
    TX_is_HDMI = c1 & 0x02;
    
    switch (n) 
    {
        case 1:
            if (c & ANX7150_INTR1_STATUS_CLK_CHG)
                ;//ANX7150_DEBUG("ANX7150_Int: Video input clock change detected.");
            if ((c & ANX7150_INTR1_STATUS_CTS_OVRWR) && (TX_is_HDMI == 0x02))
                ;//ANX7150_DEBUG("ANX7150_Int: Audio CTS is overwrite before sending by ACR packer.");
            if (c & ANX7150_INTR1_STATUS_HP_CHG)
                ANX7150_DEBUG("ANX7150_Int: Hotplug change detected.");
            if (c & ANX7150_INTR1_STATUS_SW_INT)
                ANX7150_DEBUG("ANX7150_Int: Software induced interrupt.");
            if ((c & ANX7150_INTR1_STATUS_SPDIF_ERR)&& (TX_is_HDMI == 0x02))
                ANX7150_DEBUG("ANX7150_Int: S/PDIF parity errors.");
            if ((c & ANX7150_INTR1_STATUS_AFIFO_OVER)&& (TX_is_HDMI == 0x02))
                ANX7150_DEBUG("ANX7150_Int: Audio FIFO is overrun.");
            if ((c & ANX7150_INTR1_STATUS_AFIFO_UNDER)&& (TX_is_HDMI == 0x02))
                ANX7150_DEBUG("ANX7150_Int: Audio FIFO is underrun.");
            if ((c & ANX7150_INTR1_STATUS_CTS_CHG)&& (TX_is_HDMI == 0x02))
                ANX7150_DEBUG("ANX7150_Int: Audio CTS changed.");
            break;
        case 2:
            if (c & ANX7150_INTR2_STATUS_AUTH_DONE)
                ANX7150_DEBUG("ANX7150_Int: HDCP authentication ended.");
            if (c & ANX7150_INTR2_STATUS_AUTH_CHG)
                ANX7150_DEBUG("ANX7150_Int: Hardware HDCP authentication state changed.");
            if (c & ANX7150_INTR2_STATUS_SHA_DONE)
                ANX7150_DEBUG("ANX7150_Int: Hardware HDCP computing V ended.");
            if (c & ANX7150_INTR2_STATUS_PLLLOCK_CHG)
                ANX7150_DEBUG("ANX7150_Int: PLL clock state changed.");
            if (c & ANX7150_INTR2_STATUS_BKSV_RDY)
                ANX7150_DEBUG("ANX7150_Int: BKSV ready for check.");
            if (c & ANX7150_INTR2_STATUS_HDCPENHC_CHK)
                ANX7150_DEBUG("ANX7150_Int: Enhanced link verification is need.");
            if (c & ANX7150_INTR2_STATUS_HDCPLINK_CHK)
                ANX7150_DEBUG("ANX7150_Int: Link integrity check is need.");
            if (c & ANX7150_INTR2_STATUS_ENCEN_CHG)
                //ANX7150_DEBUG("ANX7150_Int: ENC_EN changed detected.");
            break;
        case 3:
            if ((c & ANX7150_INTR3_STATUS_SPDIF_UNSTBL)&& (TX_is_HDMI == 0x02))
                ANX7150_DEBUG("ANX7150_Int: Not find expected preamble for SPDIF input.");
            if (c & ANX7150_INTR3_STATUS_RXSEN_CHG)
                ANX7150_DEBUG("ANX7150_Int: Receiver active sense changed.");
            if (c & ANX7150_INTR3_STATUS_VSYNC_DET)
                ANX7150_DEBUG("ANX7150_Int: VSYNC active edge detected.");
            if (c & ANX7150_INTR3_STATUS_DDC_NOACK)
                ANX7150_DEBUG("ANX7150_Int: DDC master not detected any ACK.");
            if (c & ANX7150_INTR3_STATUS_DDCACC_ERR)
                ANX7150_DEBUG("ANX7150_Int: DDC channel access error.");
            if ((c & ANX7150_INTR3_STATUS_AUDCLK_CHG)&& (TX_is_HDMI == 0x02))
                ANX7150_DEBUG("ANX7150_Int: Audio input clock changed.");
            if (c & ANX7150_INTR3_STATUS_VIDF_CHG)
                ANX7150_DEBUG("ANX7150_Int: Video input format changed.");
            if ((c & ANX7150_INTR3_STATUS_SPDIFBI_ERR )&& (TX_is_HDMI == 0x02))
                ANX7150_DEBUG("ANX7150_Int: SPDIF bi-phase error.");
            break;
    }
}
*/
BYTE int_s1, int_s2, int_s3;


/*
Please check programmer guide Revision 0.93 page 23 for flow chart
*/
 void ANX7150_Interrupt_Process() 
 {
    BYTE c,c1;

ANX7150_DEBUG("##Inside ANX7150_Interrupt_Process, ANX7150_system_state=%x\n",ANX7150_system_state);
		    if((ANX7150_system_state == ANX7150_INITIAL) || (ANX7150_system_state == ANX7150_WAIT_HOTPLUG))
		        return;

		/*
		    if(ANX7150_system_state == ANX7150_WAIT_HOTPLUG)
		    {
		        ANX7150_i2c_read_p0_reg(ANX7150_INTR_STATE_REG, &c);
		        if(c & 0x01)
		        	{
						ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL3_REG, &c);
				        ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL3_REG, c | 0x01);//power up all, 090630
						HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_PWDN2_REG, &c);	//en output of 8770,
					    HDMI_RX_WriteI2C_RX0(HDMI_RX_SYS_PWDN2_REG, c & 0xf8);
								
			            ANX7150_Hotplug_Change_Interrupt();

					}
		    }
		    */
		    //else 
		    {

		        if(int_s1 & ANX7150_INTR1_STATUS_HP_CHG)
		            ANX7150_Hotplug_Change_Interrupt();

		        //if(int_s1 & ANX7150_INTR1_STATUS_CLK_CHG) 
		            //ANX7150_Video_Clock_Change_Interrupt();

		        if(int_s3 & ANX7150_INTR3_STATUS_VIDF_CHG) 
		            ANX7150_Video_Format_Change_Interrupt();

		        if(int_s2 & ANX7150_INTR2_STATUS_AUTH_DONE) 
		            ANX7150_Auth_Done_Interrupt();

		        if(int_s2 & ANX7150_INTR2_STATUS_AUTH_CHG) 
		            ANX7150_Auth_Change_Interrupt();

		        ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL1_REG, &c);
		        c = c & 0x02;          // HDMI mode
		        if(c == 0x02)
		        {
		            //if(int_s3 & ANX7150_INTR3_STATUS_AUDCLK_CHG)
		                //ANX7150_Audio_CLK_Change_Interrupt();
		            if(int_s1 & ANX7150_INTR1_STATUS_AFIFO_OVER)
		                ANX7150_AFIFO_Overrun_Interrupt();

		          // SPDIF error
		            if((int_s3 & 0x81) || (int_s1 & ANX7150_INTR1_STATUS_SPDIF_ERR))
		            {
		                ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, &c1);
		                if( c1 & ANX7150_HDMI_AUDCTRL1_SPDIFIN_EN)    
		                {
		                    ANX7150_SPDIF_Error_Interrupt(int_s1,int_s3);
		                }
		            }
		            else
		            {
		                if(spdif_error_cnt > 0 && ANX7150_system_state == ANX7150_PLAY_BACK)    spdif_error_cnt --;
		                if(spdif_error_cnt > 0 && ANX7150_system_state < ANX7150_CONFIG_AUDIO)    spdif_error_cnt = 0x00;
		            }
		        }

		        if(int_s2 & ANX7150_INTR2_STATUS_PLLLOCK_CHG)
		            ANX7150_PllLock_Interrupt();

		        if(int_s3 & ANX7150_INTR3_STATUS_RXSEN_CHG)
		        {
		            ANX7150_Rx_Sense_Interrupt(); //060819
		        }

				if(int_s2 & ANX7150_INTR2_STATUS_HDCPLINK_CHK)
				{
					  if(ANX7150_HDCP_enable)
		            		ANX7150_HDCPLINK_CHK_Interrupt();
				}

		    }
		
    //}
}


BYTE ANX7150_Parse_EDID(void)
{
    BYTE i;
    ANX7150_GetEDIDLength();

    ANX7150_DEBUG("EDIDLength is %.u\n",  ANX7150_edid_length);

    ANX7150_Read_EDID();

    if(!(ANX7150_Parse_EDIDHeader()))
    {	
        ANX7150_DEBUG("BAD EDID Header, Stop parsing \n");
        ANX7150_edid_result.edid_errcode = ANX7150_EDID_BadHeader;
        return ANX7150_edid_result.edid_errcode;
    }

    if(!(ANX7150_Parse_EDIDVersion()))
    {
        ANX7150_DEBUG("EDID does not support 861B, Stop parsing\n");
        ANX7150_edid_result.edid_errcode = ANX7150_EDID_861B_not_supported;
        return ANX7150_edid_result.edid_errcode;
    }
		
    if(ANX7150_EDID_Checksum(0) == 0)
    {
        ANX7150_DEBUG("EDID Block one check sum error, Stop parsing\n");
        ANX7150_edid_result.edid_errcode = ANX7150_EDID_CheckSum_ERR;
        return ANX7150_edid_result.edid_errcode;
    }

    //ANX7150_Parse_BasicDis();
    ANX7150_Parse_DTDinBlockONE();
    if(ANX7150_EDID_Buf[0x7e] == 0)
    {
        ANX7150_DEBUG("No EDID extension blocks.\n");
        ANX7150_edid_result.edid_errcode = ANX7150_EDID_No_ExtBlock;
        return ANX7150_edid_result.edid_errcode;
    }
    ANX7150_Parse_NativeFormat();
    ANX7150_Parse_ExtBlock();

    if(ANX7150_edid_result.edid_errcode == 0x05)
        return ANX7150_edid_result.edid_errcode;

    if(ANX7150_edid_result.edid_errcode == 0x03)
        return ANX7150_edid_result.edid_errcode;

    ANX7150_DEBUG("EDID parsing finished!\n");

	{
        ANX7150_DEBUG("ANX7150_edid_result.edid_errcode = 0x%.2x\n",(WORD)ANX7150_edid_result.edid_errcode);
        ANX7150_DEBUG("ANX7150_edid_result.is_HDMI = 0x%.2x\n",(WORD)ANX7150_edid_result.is_HDMI);
        ANX7150_DEBUG("ANX7150_edid_result.ycbcr422_supported = 0x%.2x\n",(WORD)ANX7150_edid_result.ycbcr422_supported);
        ANX7150_DEBUG("ANX7150_edid_result.ycbcr444_supported = 0x%.2x\n",(WORD)ANX7150_edid_result.ycbcr444_supported);
        ANX7150_DEBUG("ANX7150_edid_result.supported_1080i_60Hz = 0x%.2x\n",(WORD)ANX7150_edid_result.supported_1080i_60Hz);
        ANX7150_DEBUG("ANX7150_edid_result.supported_1080i_50Hz = 0x%.2x\n",(WORD)ANX7150_edid_result.supported_1080i_50Hz);
        ANX7150_DEBUG("ANX7150_edid_result.supported_720p_60Hz = 0x%.2x\n",(WORD)ANX7150_edid_result.supported_720p_60Hz);
        ANX7150_DEBUG("ANX7150_edid_result.supported_720p_50Hz = 0x%.2x\n",(WORD)ANX7150_edid_result.supported_720p_50Hz);
        ANX7150_DEBUG("ANX7150_edid_result.supported_640x480p_60Hz = 0x%.2x\n",(WORD)ANX7150_edid_result.supported_640x480p_60Hz); 
        ANX7150_DEBUG("ANX7150_edid_result.supported_720x480p_60Hz = 0x%.2x\n",(WORD)ANX7150_edid_result.supported_720x480p_60Hz); 
        ANX7150_DEBUG("ANX7150_edid_result.supported_720x480i_60Hz = 0x%.2x\n",(WORD)ANX7150_edid_result.supported_720x480i_60Hz);   
        ANX7150_DEBUG("ANX7150_edid_result.supported_576p_50Hz = 0x%.2x\n",(WORD)ANX7150_edid_result.supported_576p_50Hz);
        ANX7150_DEBUG("ANX7150_edid_result.supported_576i_50Hz = 0x%.2x\n",(WORD)ANX7150_edid_result.supported_576i_50Hz);       
    if(!ANX7150_edid_result.edid_errcode)
    {
        for(i = 0; i < ANX7150_sau_length/3; i++)
        {
            ANX7150_DEBUG("ANX7150_edid_result.AudioChannel = 0x%.2x\n",(WORD)ANX7150_edid_result.AudioChannel[i]);
            ANX7150_DEBUG("ANX7150_edid_result.AudioFormat = 0x%.2x\n",(WORD)ANX7150_edid_result.AudioFormat[i]);
            ANX7150_DEBUG("ANX7150_edid_result.AudioFs = 0x%.2x\n",(WORD)ANX7150_edid_result.AudioFs[i]);
            ANX7150_DEBUG("ANX7150_edid_result.AudioLength = 0x%.2x\n",(WORD)ANX7150_edid_result.AudioLength[i]);
        }
        ANX7150_DEBUG("ANX7150_edid_result.SpeakerFormat = 0x%.2x\n",(WORD)ANX7150_edid_result.SpeakerFormat);
    }
}
}

void ANX7150_GetEDIDLength()
{
    BYTE edid_data_length;

    ANX7150_RST_DDCChannel();

     ANX7150_InitDDC_Read(0xa0, 0x00, 0x7e, 0x01, 0x00);
     /*ANX7150_i2c_read_p0_reg(ANX7150_DDC_FIFOCNT_REG, &c);
	 while(c==0)
	 	ANX7150_i2c_read_p0_reg(ANX7150_DDC_FIFOCNT_REG, &c);*/
	 	msleep(10);
	 
    ANX7150_i2c_read_p0_reg(ANX7150_DDC_FIFO_ACC_REG, &edid_data_length);

    ANX7150_edid_length = edid_data_length * 128 + 128;

}
void ANX7150_Read_EDID(void) 
{
    BYTE edid_segment,segmentpointer,k;

    ANX7150_RST_DDCChannel();

    edid_segment = ANX7150_edid_length / 256;
	if(edid_segment==0)																			//update
 		segmentpointer =0;
    else 
        segmentpointer = edid_segment - 1;
    //segmentpointer = edid_segment - 1;
    
    for(k = 0; k <= segmentpointer; k ++)
    {
        ANX7150_InitDDC_Read(0xa0, k, 0x00, 0x80, 0x00);
        ANX7150_DDC_Mass_Read(128, k);
        ANX7150_InitDDC_Read(0xa0, k, 0x80, 0x80, 0x00);
        ANX7150_DDC_Mass_Read(128, k + 1);
    }

    if((ANX7150_edid_length - 256 * edid_segment) == 0)
        ANX7150_DEBUG("Finish reading EDID");
    else
    {
        ANX7150_DEBUG("Read one more block(128 bytes).........");
        ANX7150_InitDDC_Read(0xa0, segmentpointer + 1, 0x00, 0x80, 0x00);
        ANX7150_DDC_Mass_Read(128, segmentpointer + 1);
        ANX7150_DEBUG("Finish reading EDID");
    }
}


void ANX7150_DDC_Mass_Read(WORD length, BYTE segment)
{
    WORD i, j;    
    BYTE c, c1,ddc_empty_cnt;

    i = length;
    while (i > 0) 
    {
        //check DDC FIFO statue
        ANX7150_i2c_read_p0_reg(ANX7150_DDC_CHSTATUS_REG, &c);
        if(c & ANX7150_DDC_CHSTATUS_DDC_OCCUPY)
        {
            //ANX7150_DEBUG("ANX7150 DDC channel is accessed by an external device, break!.");
            break;
        }
        if(c & ANX7150_DDC_CHSTATUS_FIFO_FULL)
            ANX7150_ddc_fifo_full = 1;
        else 
            ANX7150_ddc_fifo_full = 0;
        if(c & ANX7150_DDC_CHSTATUS_INPRO)
            ANX7150_ddc_progress = 1;
        else
            ANX7150_ddc_progress = 0;
        if(ANX7150_ddc_fifo_full) 
        {
            //ANX7150_DEBUG("DDC FIFO is full during edid reading");
            ANX7150_i2c_read_p0_reg(ANX7150_DDC_FIFOCNT_REG, &c);
            //ANX7150_DEBUG("FIFO counter is %.2x\n", (WORD) c);
            for(j=0; j<c; j++) 
            {
                ANX7150_i2c_read_p0_reg(ANX7150_DDC_FIFO_ACC_REG, &c1);
				  if(segment == 0)
				  	{
						ANX7150_EDID_Buf[length - i + j] = c1;
						//ANX7150_DEBUG("EDID[0x%.2x]=0x%.2x    ", (WORD)(length - i + j), (WORD) c1);
				  	}
				  else if(segment == 1)
				  	{
						ANX7150_EDID_Buf[length - i + j + 0x80] = c1;
						//ANX7150_DEBUG("EDID[0x%.2x]=0x%.2x    ", (WORD)(length - i + j + 0x80), (WORD) c1);
				  	}
                
                ANX7150_ddc_fifo_full = 0;
            }
			  i = i - c;
            //ANX7150_DEBUG("\n");
        } 
        else if(!ANX7150_ddc_progress) 
        {
            //ANX7150_DEBUG("ANX7150 DDC FIFO access finished.");
            ANX7150_i2c_read_p0_reg(ANX7150_DDC_FIFOCNT_REG, &c);
            //ANX7150_DEBUG("FIFO counter is %.2x\n", (WORD) c);
            if(!c)
            {
                i =0;
                break;
            }
            for(j=0; j<c; j++) 
            {
                ANX7150_i2c_read_p0_reg(ANX7150_DDC_FIFO_ACC_REG, &c1);
				  if(segment == 0)
				  	{
						ANX7150_EDID_Buf[length - i + j] = c1;
						//ANX7150_DEBUG("EDID[0x%.2x]=0x%.2x    ", (WORD)(length - i + j), (WORD) c1);
				  	}
				  else if(segment == 1)
				  	{
						ANX7150_EDID_Buf[length - i + j + 0x80] = c1;
						//ANX7150_DEBUG("EDID[0x%.2x]=0x%.2x    ", (WORD)(length - i + j + 0x80), (WORD) c1);
				  	}
            }
            i = i - c;
            //ANX7150_DEBUG("\ni=%d\n", i);
        } 
        else 
        {
            ddc_empty_cnt = 0x00;
            for(c1=0; c1<0x0a; c1++)
            {
                ANX7150_i2c_read_p0_reg(ANX7150_DDC_CHSTATUS_REG, &c);
                //ANX7150_DEBUG("DDC FIFO access is progressing.");
                //ANX7150_DEBUG("DDC Channel status is 0x%.2x\n",(WORD)c);
                if(c & ANX7150_DDC_CHSTATUS_FIFO_EMPT)
                    ddc_empty_cnt++;
                msleep(5);
                //ANX7150_DEBUG("ddc_empty_cnt =  0x%.2x\n",(WORD)ddc_empty_cnt);
            }
            if(ddc_empty_cnt >= 0x0a)
            i = 0;
        }
    }
}


BYTE ANX7150_Parse_EDIDHeader(void)
{
    BYTE i,temp;
    temp = 0;
    // the EDID header should begin with 0x00,0xff,0xff,0xff,0xff,0xff,0xff,0x00
    if((ANX7150_Read_EDID_BYTE(0, 0) == 0x00) && (ANX7150_Read_EDID_BYTE(0, 7) == 0x00))
    {
        for(i = 1; i < 7; i++)
        {
            if(ANX7150_Read_EDID_BYTE(0, i) != 0xff)
            {
                temp = 0x01;
                break;
            }
        }
    }
    else
    {
        temp = 0x01;
    }
    if(temp == 0x01)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
BYTE ANX7150_Parse_EDIDVersion(void)
{
     
    if(!((ANX7150_Read_EDID_BYTE(0, 0x12) == 1) && (ANX7150_Read_EDID_BYTE(0, 0x13) >= 3) ))
    {	
        return 0;
    }
    else
    {	
        return 1;
    }
}
BYTE ANX7150_Parse_ExtBlock()
{
    BYTE i,c;

    for(i = 0; i < ANX7150_Read_EDID_BYTE(0, 0x7e); i++)    //read in blocks
    {
        c = ANX7150_Read_EDID_BYTE(i/2, 0x80);
        if( c == 0x02)
        {
            ANX7150_ext_block_num = i + 1;
            ANX7150_Parse_DTDinExtBlock();
            ANX7150_Parse_STD();
            if(!(ANX7150_EDID_Checksum(ANX7150_ext_block_num)))
            {
                ANX7150_edid_result.edid_errcode = ANX7150_EDID_CheckSum_ERR;
                return ANX7150_edid_result.edid_errcode;
            }
        }
        else
        {
            ANX7150_edid_result.edid_errcode = ANX7150_EDID_ExtBlock_NotFor_861B;
            return ANX7150_edid_result.edid_errcode;
        }
    }
}
void ANX7150_Parse_DTDinBlockONE()
{
    BYTE i;
    for(i = 0; i < 18; i++)
    {
        ANX7150_edid_dtd[i] = ANX7150_Read_EDID_BYTE(0, (i + 0x36));
    }
    //ANX7150_DEBUG("Parse the first DTD in Block one:\n");
    ANX7150_Parse_DTD();

    if((ANX7150_Read_EDID_BYTE(0, 0x48) == 0) 
    && (ANX7150_Read_EDID_BYTE(0, 0x49) == 0)
    && (ANX7150_Read_EDID_BYTE(0, 0x4a) == 0))
    {
        ;//ANX7150_DEBUG("the second DTD in Block one is not used to descript video timing.\n");
    }
    else
    {
        for(i = 0; i < 18; i++)
        {
            ANX7150_edid_dtd[i] = ANX7150_Read_EDID_BYTE(0, (i + 0x48));
        }
        ANX7150_Parse_DTD();
    }

    if((ANX7150_Read_EDID_BYTE(0,0x5a) == 0)
    && (ANX7150_Read_EDID_BYTE(0,0x5b) == 0)
    && (ANX7150_Read_EDID_BYTE(0,0x5c) == 0))
    {
        ;//ANX7150_DEBUG("the third DTD in Block one is not used to descript video timing.\n");
    }
    else
    {
        for(i = 0; i < 18; i++)
        {
            ANX7150_edid_dtd[i] = ANX7150_Read_EDID_BYTE(0, (i + 0x5a));
        }
        ANX7150_Parse_DTD();
    }

    if((ANX7150_Read_EDID_BYTE(0,0x6c) == 0)
    && (ANX7150_Read_EDID_BYTE(0,0x6d) == 0)
    && (ANX7150_Read_EDID_BYTE(0,0x6e) == 0))
    {
        ;//ANX7150_DEBUG("the fourth DTD in Block one is not used to descript video timing.\n");
    }
    else
    {
        for(i = 0; i < 18; i++)
        {
            ANX7150_edid_dtd[i] = ANX7150_Read_EDID_BYTE(0,(i + 0x6c));
        }
        ANX7150_Parse_DTD();
    }
}

void ANX7150_Parse_DTDinExtBlock()
{
    BYTE i,DTDbeginAddr;
    DTDbeginAddr = ANX7150_Read_EDID_BYTE(ANX7150_ext_block_num/2, 0x82)
    + 0x80;
    while(DTDbeginAddr < (0x6c + 0x80))
    {   
        if((ANX7150_Read_EDID_BYTE(ANX7150_ext_block_num/2,DTDbeginAddr) == 0)
        && (ANX7150_Read_EDID_BYTE(ANX7150_ext_block_num/2,(DTDbeginAddr + 1)) == 0)
        && (ANX7150_Read_EDID_BYTE(ANX7150_ext_block_num/2,(DTDbeginAddr + 2)) == 0))
        {
            ;//ANX7150_DEBUG("this DTD in Extension Block is not used to descript video timing.\n");
        }
        else
        {
            for(i = 0; i < 18; i++)
            {
                ANX7150_edid_dtd[i] = ANX7150_Read_EDID_BYTE(ANX7150_ext_block_num/2,(i + DTDbeginAddr));
            }
            //ANX7150_DEBUG("Parse the DTD in Extension Block :\n");
            ANX7150_Parse_DTD();
        }
        DTDbeginAddr = DTDbeginAddr + 18;
    }
}

void ANX7150_Parse_DTD()
{
    WORD temp;
    unsigned long temp1,temp2;
    WORD Hresolution,Vresolution,Hblanking,Vblanking;
    WORD PixelCLK,Vtotal,H_image_size,V_image_size;
    BYTE Hz;
    //float Ratio;
    
    temp = ANX7150_edid_dtd[1];
    temp = temp << 8;
    PixelCLK = temp + ANX7150_edid_dtd[0];
    //	ANX7150_DEBUG("Pixel clock is 10000 * %u\n",  temp);

    temp = ANX7150_edid_dtd[4];
    temp = (temp << 4) & 0x0f00;
    Hresolution = temp + ANX7150_edid_dtd[2];
    //ANX7150_DEBUG("Horizontal Active is  %u\n",  Hresolution);

    temp = ANX7150_edid_dtd[4];
    temp = (temp << 8) & 0x0f00;
    Hblanking = temp + ANX7150_edid_dtd[3];
    //ANX7150_DEBUG("Horizontal Blanking is  %u\n",  temp);

    temp = ANX7150_edid_dtd[7];
    temp = (temp << 4) & 0x0f00;
    Vresolution = temp + ANX7150_edid_dtd[5];
   //ANX7150_DEBUG("Vertical Active is  %u\n",  Vresolution);

    temp = ANX7150_edid_dtd[7];
    temp = (temp << 8) & 0x0f00;
    Vblanking = temp + ANX7150_edid_dtd[6];
    //ANX7150_DEBUG("Vertical Blanking is  %u\n",  temp);

    temp = ANX7150_edid_dtd[11];
    temp = (temp << 2) & 0x0300;
    temp = temp + ANX7150_edid_dtd[8];
    //ANX7150_DEBUG("Horizontal Sync Offset is  %u\n",  temp);

    temp = ANX7150_edid_dtd[11];
    temp = (temp << 4) & 0x0300;
    temp = temp + ANX7150_edid_dtd[9];
    //ANX7150_DEBUG("Horizontal Sync Pulse is  %u\n",  temp);

    temp = ANX7150_edid_dtd[11];
    temp = (temp << 2) & 0x0030;
    temp = temp + (ANX7150_edid_dtd[10] >> 4);
    //ANX7150_DEBUG("Vertical Sync Offset is  %u\n",  temp);

    temp = ANX7150_edid_dtd[11];
    temp = (temp << 4) & 0x0030;
    temp = temp + (ANX7150_edid_dtd[8] & 0x0f);
    //ANX7150_DEBUG("Vertical Sync Pulse is  %u\n",  temp);

    temp = ANX7150_edid_dtd[14];
    temp = (temp << 4) & 0x0f00;
    H_image_size = temp + ANX7150_edid_dtd[12];
    //ANX7150_DEBUG("Horizontal Image size is  %u\n",  temp);

    temp = ANX7150_edid_dtd[14];
    temp = (temp << 8) & 0x0f00;
    V_image_size = temp + ANX7150_edid_dtd[13];
    //ANX7150_DEBUG("Vertical Image size is  %u\n",  temp);

    //ANX7150_DEBUG("Horizontal Border is  %bu\n",  ANX7150_edid_dtd[15]);

    //ANX7150_DEBUG("Vertical Border is  %bu\n",  ANX7150_edid_dtd[16]);
    
    temp1 = Hresolution + Hblanking;
    Vtotal = Vresolution + Vblanking;
    temp1 = temp1 * Vtotal;
    temp2 = PixelCLK;
    temp2 = temp2 * 10000;
		if (temp1 == 0)																												//update
       Hz=0;
    else 
        Hz = temp2 / temp1;
    //Hz = temp2 / temp1;
    if((Hz == 59) || (Hz == 60))
    {
        Hz = 60;
        //ANX7150_DEBUG("_______________Vertical Active is  %u\n",  Vresolution);
        if(Vresolution == 540)
            ANX7150_edid_result.supported_1080i_60Hz = 1;
        if(Vresolution == 1080)
            ANX7150_edid_result.supported_1080p_60Hz = 1;
        if(Vresolution == 720)
            ANX7150_edid_result.supported_720p_60Hz = 1;
        if((Hresolution == 640) && (Vresolution == 480))
            ANX7150_edid_result.supported_640x480p_60Hz = 1;
        if((Hresolution == 720) && (Vresolution == 480))
            ANX7150_edid_result.supported_720x480p_60Hz = 1;
        if((Hresolution == 720) && (Vresolution == 240))
            ANX7150_edid_result.supported_720x480i_60Hz = 1;
    }
    if(Hz == 50)
    {
        //ANX7150_DEBUG("+++++++++++++++Vertical Active is  %u\n",  Vresolution);
        if(Vresolution == 540)
            ANX7150_edid_result.supported_1080i_50Hz = 1;
        if(Vresolution == 1080)
            ANX7150_edid_result.supported_1080p_50Hz = 1;
        if(Vresolution == 720)
            ANX7150_edid_result.supported_720p_50Hz = 1;
        if(Vresolution == 576)
            ANX7150_edid_result.supported_576p_50Hz = 1;
        if(Vresolution == 288)
            ANX7150_edid_result.supported_576i_50Hz = 1;
    }
    //ANX7150_DEBUG("Fresh rate :% bu Hz\n", Hz);
    //Ratio = H_image_size;
    //Ratio = Ratio / V_image_size;
    //ANX7150_DEBUG("Picture ratio : %f \n", Ratio);
}
/*void ANX7150_Parse_BasicDis()
{
    BYTE temp;
    temp = ANX7150_Read_EDID_BYTE(0,0x18) & 0x18;
    	if(temp == 0x00)
    	//ANX7150_DEBUG("EDID Display type: mon/gray display.\n");
    else if(temp == 0x08)
    	//ANX7150_DEBUG("EDID Display type: RGB color display.\n");
    else if(temp == 0x10)
    	//ANX7150_DEBUG("EDID Display type: non-RGB color display.\n");
    else 
    	//ANX7150_DEBUG("EDID Display type: Undefined.\n");
    temp = ANX7150_Read_EDID_BYTE(0,0x18) & 0x02;
    if(temp == 0x00)
    	//ANX7150_DEBUG("EDID Preferred_timing: not supported.\n");
    else
    	//ANX7150_DEBUG("EDID Preferred_timing: supported.\n");
}
*/
void ANX7150_Parse_NativeFormat()
{
    BYTE temp;
    temp = ANX7150_Read_EDID_BYTE(0,0x83) & 0xf0;
   /*if(temp & 0x80)
    	;//ANX7150_DEBUG("DTV supports underscan.\n");
    if(temp & 0x40)
    	;//ANX7150_DEBUG("DTV supports BasicAudio.\n");*/
    if(temp & 0x20)
    {
        //ANX7150_DEBUG("DTV supports YCbCr 4:4:4.\n");
        ANX7150_edid_result.ycbcr444_supported= 1;
    }
    if(temp & 0x10)
    {
        //ANX7150_DEBUG("DTV supports YCbCr 4:2:2.\n");
        ANX7150_edid_result.ycbcr422_supported= 1;
    }
}

void ANX7150_Parse_STD()
{
    BYTE DTDbeginAddr;
    ANX7150_stdaddr = 0x84;
    DTDbeginAddr = ANX7150_Read_EDID_BYTE(ANX7150_ext_block_num/2,0x82) + 0x80;
    // ANX7150_DEBUG("Video DTDbeginAddr Register :%.2x\n", (WORD) DTDbeginAddr);
    while(ANX7150_stdaddr < DTDbeginAddr)
    {
        ANX7150_stdreg = ANX7150_Read_EDID_BYTE(ANX7150_ext_block_num/2,ANX7150_stdaddr); 
        switch(ANX7150_stdreg & 0xe0) 
        {
            case 0x20:
                ANX7150_Parse_AudioSTD();
                ANX7150_sau_length = ANX7150_stdreg & 0x1f;
                break;
            case 0x40:
               ANX7150_Parse_VideoSTD();
                ANX7150_svd_length = ANX7150_stdreg & 0x1f;
                break;
            case 0x80:
                ANX7150_Parse_SpeakerSTD();
               break;
            case 0x60:
                ANX7150_Parse_VendorSTD();
                break;
            default:
                break;
        }
        ANX7150_stdaddr = ANX7150_stdaddr + (ANX7150_stdreg & 0x1f) + 0x01;	
    }
}

void ANX7150_Parse_AudioSTD()
{
    BYTE i,AudioFormat,STDReg_tmp,STDAddr_tmp;
    STDReg_tmp = ANX7150_stdreg & 0x1f;
    STDAddr_tmp = ANX7150_stdaddr + 1;
    i = 0;
    while(i < STDReg_tmp)
    {
        AudioFormat = (ANX7150_Read_EDID_BYTE(ANX7150_ext_block_num/2,STDAddr_tmp ) & 0xF8) >> 3;
        ANX7150_edid_result.AudioChannel[i/3] = (ANX7150_Read_EDID_BYTE(ANX7150_ext_block_num/2,STDAddr_tmp) & 0x07) + 1;
        ANX7150_edid_result.AudioFormat[i/3] = AudioFormat;
        ANX7150_edid_result.AudioFs[i/3] = ANX7150_Read_EDID_BYTE(ANX7150_ext_block_num/2,(STDAddr_tmp + 1)) & 0x7f;

        if(AudioFormat == 1)
            ANX7150_edid_result.AudioLength[i/3] = ANX7150_Read_EDID_BYTE(ANX7150_ext_block_num/2,(STDAddr_tmp + 2)) & 0x07;
        else
            ANX7150_edid_result.AudioLength[i/3] = ANX7150_Read_EDID_BYTE(ANX7150_ext_block_num/2,(STDAddr_tmp + 2)) << 3;

        i = i + 3;
        STDAddr_tmp = STDAddr_tmp + 3;
    }
}

void ANX7150_Parse_VideoSTD()
{
    BYTE i,STDReg_tmp,STDAddr_tmp;
    BYTE SVD_ID[34];
    STDReg_tmp = ANX7150_stdreg & 0x1f;
    STDAddr_tmp = ANX7150_stdaddr + 1;
       i = 0;
    while(i < STDReg_tmp)
    {
        SVD_ID[i] = ANX7150_Read_EDID_BYTE(ANX7150_ext_block_num/2,STDAddr_tmp) & 0x7F;
        //ANX7150_DEBUG("ANX7150_edid_result.SVD_ID[%.2x]=0x%.2x\n",(WORD)i,(WORD)ANX7150_edid_result.SVD_ID[i]);
        //if(ANX7150_Read_EDID_BYTE(ANX7150_ext_block_num/2,STDAddr_tmp) & 0x80)
        //    ANX7150_DEBUG(" Native mode");
        if(SVD_ID[i] == 1)
            ANX7150_edid_result.supported_640x480p_60Hz = 1;
        else if(SVD_ID[i] == 4)
            ANX7150_edid_result.supported_720p_60Hz = 1;
        else if(SVD_ID[i] == 19)
            ANX7150_edid_result.supported_720p_50Hz = 1;
        else if(SVD_ID[i] == 16)
            ANX7150_edid_result.supported_1080p_60Hz = 1;
        else if(SVD_ID[i] == 31)
            ANX7150_edid_result.supported_1080p_50Hz = 1;
        else if(SVD_ID[i] == 5)
            ANX7150_edid_result.supported_1080i_60Hz = 1;
        else if(SVD_ID[i] == 20)
            ANX7150_edid_result.supported_1080i_50Hz = 1;
        else if((SVD_ID[i] == 2) ||(SVD_ID[i] == 3))
            ANX7150_edid_result.supported_720x480p_60Hz = 1;
        else if((SVD_ID[i] == 6) ||(SVD_ID[i] == 7))
            ANX7150_edid_result.supported_720x480i_60Hz = 1;
        else if((SVD_ID[i] == 17) ||(SVD_ID[i] == 18))
            ANX7150_edid_result.supported_576p_50Hz = 1;
        else if((SVD_ID[i] == 21) ||(SVD_ID[i] == 22))
            ANX7150_edid_result.supported_576i_50Hz = 1;

        i = i + 1;
        STDAddr_tmp = STDAddr_tmp + 1;
    }
}

void ANX7150_Parse_SpeakerSTD()
{
    ANX7150_edid_result.SpeakerFormat = ANX7150_Read_EDID_BYTE(ANX7150_ext_block_num/2,(ANX7150_stdaddr + 1)) ;
}

void ANX7150_Parse_VendorSTD()
{
    //BYTE c;
    if((ANX7150_Read_EDID_BYTE(ANX7150_ext_block_num/2,(ANX7150_stdaddr + 1)) == 0x03) 
    && (ANX7150_Read_EDID_BYTE(ANX7150_ext_block_num/2,(ANX7150_stdaddr + 2)) == 0x0c) 
    && (ANX7150_Read_EDID_BYTE(ANX7150_ext_block_num/2,(ANX7150_stdaddr + 3)) == 0x00))
    {
        ANX7150_edid_result.is_HDMI = 1;
        //ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL1_REG, &c);
        //ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL1_REG, c |ANX7150_SYS_CTRL1_HDMI);
    }    
    else
    {
        ANX7150_edid_result.is_HDMI = 0;
        //ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL1_REG, &c);
        //ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL1_REG, c & (~ANX7150_SYS_CTRL1_HDMI));
    }     
}

BYTE ANX7150_EDID_Checksum(BYTE block_number) 
{
    BYTE i, real_checksum;  
    BYTE edid_block_checksum;
	
    edid_block_checksum = 0;
    for(i = 0; i < 127; i ++) 
    {           
        if((block_number / 2) * 2 == block_number)
            edid_block_checksum = edid_block_checksum + ANX7150_Read_EDID_BYTE(block_number/2, i);
        else
            edid_block_checksum = edid_block_checksum + ANX7150_Read_EDID_BYTE(block_number/2, i + 0x80);
    }
    edid_block_checksum = (~edid_block_checksum) + 1;
   // ANX7150_DEBUG("edid_block_checksum = 0x%.2x\n",(WORD)edid_block_checksum);
    if((block_number / 2) * 2 == block_number)
        real_checksum = ANX7150_Read_EDID_BYTE(block_number/2, 0x7f);
    else
        real_checksum = ANX7150_Read_EDID_BYTE(block_number/2, 0xff);
    if(real_checksum == edid_block_checksum) 
        return 1;
    else 
	 return 0;
}    


void ANX7150_InitDDC_Read(BYTE devaddr, BYTE segmentpointer,
    BYTE offset, BYTE  access_num_Low,BYTE access_num_high)
{
    //Write slave device address
    ANX7150_i2c_write_p0_reg(ANX7150_DDC_SLV_ADDR_REG, devaddr);
    // Write segment address
    ANX7150_i2c_write_p0_reg(ANX7150_DDC_SLV_SEGADDR_REG, segmentpointer);
    //Write offset
    ANX7150_i2c_write_p0_reg(ANX7150_DDC_SLV_OFFADDR_REG, offset);
    //Write number for access
    ANX7150_i2c_write_p0_reg(ANX7150_DDC_ACCNUM0_REG, access_num_Low);
    ANX7150_i2c_write_p0_reg(ANX7150_DDC_ACCNUM1_REG, access_num_high);
    //Clear FIFO
    ANX7150_i2c_write_p0_reg(ANX7150_DDC_ACC_CMD_REG, 0x05);
    //EDDC sequential Read
    ANX7150_i2c_write_p0_reg(ANX7150_DDC_ACC_CMD_REG, 0x04);
}

BYTE ANX7150_Read_EDID_BYTE(BYTE segmentpointer,BYTE offset)
{
    /*BYTE c;
    ANX7150_InitDDC_Read(0xa0, segmentpointer, offset, 0x01, 0x00);
     ANX7150_i2c_read_p0_reg(ANX7150_DDC_FIFOCNT_REG, &c);
	 while(c==0)
    	ANX7150_i2c_read_p0_reg(ANX7150_DDC_FIFO_ACC_REG, &c);
    return c;*/

	 return ANX7150_EDID_Buf[offset];
}



void ANX7150_Timer_Process () 
{
    //if (timer_done) 
    {
//	ANX7150_DEBUG("timer_slot == %d\n",timer_slot);
//        timer_done = 0;
        if (timer_slot == 4) 
            timer_slot = 0;
        else 
            timer_slot ++;
        if (timer_slot == 0) 
        {
	    ANX7150_DEBUG("ANX7150_Timer_Slot1()\n");
            ANX7150_Timer_Slot1(); 
        }
        else if (timer_slot == 1) 
        {
	    ANX7150_DEBUG("ANX7150_Timer_Slot2()\n");
            ANX7150_Timer_Slot2(); 
        } 
        else if (timer_slot == 2) 
        {
	    ANX7150_DEBUG("ANX7150_Timer_Slot3()\n");
            ANX7150_Timer_Slot3(); 
        } 
        else if (timer_slot == 3) 
        {
	    ANX7150_DEBUG("ANX7150_Timer_Slot4()\n");
            ANX7150_Timer_Slot4();
        }
    }
}

/*
(1)Reset the ANX7150 chip
(2)Read the chip ID via I2C interface
*/
BIT ANX7150_Chip_Located(void)
{
        BYTE i;
        BYTE c, d1, d2;

        for (i=0; i<10; i++)
        {
//            ANX7150_Resetn_Pin = 0;
//            msleep(2);
//            ANX7150_Resetn_Pin = 1;
//            msleep(2);
     	    ANX7150_i2c_write_p0_reg( 0x08, 0x01);
	    msleep(30);

            c = ANX7150_i2c_read_p0_reg(ANX7150_DEV_IDL_REG, &d1);
            if (c) continue;

            c = ANX7150_i2c_read_p0_reg(ANX7150_DEV_IDH_REG, &d2);
            if (c) continue;

            if (d1 == 0x50 && d2 == 0x71) 
            {
                ANX7150_DEBUG("ANX7150 detected!");
                return 1;
            }
        }
        ANX7150_DEBUG("device not detected");
        return 0;
}


#if BIST_MODE_USED

void ANX7150_BIST() 
{
    BYTE c, c1,i,temp;

    if((ANX7150_system_state != ANX7150_INITIAL)
        && (ANX7150_system_state != ANX7150_WAIT_HOTPLUG)
        && (ANX7150_system_state != ANX7150_READ_PARSE_EDID)
        && (ANX7150_system_state != ANX7150_WAIT_RX_SENSE))
    {
        ANX7150_Reset_BIST_Setting();     
        ANX7150_Reset_AVI();
        ANX7150_Set_System_State(ANX7150_CONFIG_VIDEO);
    }

        if(ANX7150_system_state == ANX7150_INITIAL) 
            return;

        if(ANX7150_system_state == ANX7150_WAIT_HOTPLUG)
	{ 

	    ANX7150_DEBUG("!!!!!!ANX7150_WAIT_HOTPLUG\n");

	    ANX7150_i2c_read_p0_reg(ANX7150_INTR_STATE_REG, &c);
            if(c & 0x01)
        	{
			ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL3_REG, &c);
		        ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL3_REG, c | 0x01);//power up all, 090630
			//HDMI_RX_ReadI2C_RX0(HDMI_RX_SYS_PWDN2_REG, &c);	//en output of 8770,
			//HDMI_RX_WriteI2C_RX0(HDMI_RX_SYS_PWDN2_REG, c & 0xf8);				
	                ANX7150_Hotplug_Change_Interrupt();
		        ANX7150_DEBUG("ANX7150_BIST() -->HPD Det at timer slot1");
			}

            return;
	}
        
        if(ANX7150_system_state == ANX7150_READ_PARSE_EDID) 
        {
            ANX7150_i2c_read_p0_reg(ANX7150_HDCP_CTRL0_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL0_REG, (c &(~ANX7150_HDCP_CTRL0_HW_AUTHEN)));
            ANX7150_hdcp_auth_en = 0;
            ANX7150_RST_DDCChannel();        
            ANX7150_Parse_EDID();
            ANX7150_parse_edid_done = 1;
            ANX7150_Set_System_State(ANX7150_WAIT_RX_SENSE);
        }
        
        if(ANX7150_system_state == ANX7150_WAIT_RX_SENSE)
        {
            ANX7150_i2c_read_p0_reg(ANX7150_SYS_STATE_REG, &c);
            if(c & ANX7150_SYS_STATE_RSV_DET)
            {
                ANX7150_Set_System_State(ANX7150_CONFIG_VIDEO);//060819
            }
            else
            {
                ;//ANX7150_Set_System_State(ANX7150_CONFIG_VIDEO);//060819
            }
        }
     
        if(ANX7150_system_state == ANX7150_CONFIG_VIDEO) 
        {
            ANX7150_Clean_HDCP();
            if(0) 
					;           
			else 
            { //use ANX9011 video clock as clock source, ANX9011 must receive a 720P mode stream
                ANX7150_Config_Bist_Video(1);
            }
    	  //enable video input
    	     ANX7150_i2c_read_p0_reg(ANX7150_VID_CTRL_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_VID_CTRL_REG, c | ANX7150_VID_CTRL_IN_EN);
            ANX7150_i2c_read_p0_reg(ANX7150_VID_STATUS_REG, &c);
            if(!(c & ANX7150_VID_STATUS_VID_STABLE))
            {
                return;
            }

            if(ANX7150_edid_result.is_HDMI)
            {
                ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL1_REG, &c);
                ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL1_REG, c | 0x02);
            }
            //reset TMDS link
            ANX7150_i2c_read_p0_reg(ANX7150_SRST_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_SRST_REG, (c | ANX7150_TX_RST));
            ANX7150_i2c_write_p0_reg(ANX7150_SRST_REG, (c & (~ANX7150_TX_RST)));
            
            //Enable TMDS output
        ANX7150_DEBUG("ANX7150_BIST--->|||||||||0x80\n");
            ANX7150_i2c_read_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, &c);
        ANX7150_DEBUG("ANX7150_TMDS_CLKCH_CONFIG_REG=%x\n",c);
            ANX7150_i2c_write_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, (c | ANX7150_TMDS_CLKCH_MUTE));

            ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL1_REG, &c);
            if((c & 0x02) == 0x02)
                ANX7150_Set_System_State(ANX7150_CONFIG_AUDIO);
            if((c & 0x02) == 0x00)
                ANX7150_Set_System_State(ANX7150_HDCP_AUTHENTICATION);
        } 

        if(ANX7150_system_state == ANX7150_CONFIG_AUDIO) 
        {
          // ACR_N
            ANX7150_i2c_write_p1_reg(ANX7150_ACR_N1_SW_REG, 0x00);
            ANX7150_i2c_write_p1_reg(ANX7150_ACR_N2_SW_REG, 0x18);
            ANX7150_i2c_write_p1_reg(ANX7150_ACR_N3_SW_REG, 0x00);

            //Enable control of ACR
            ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL1_REG, &c);
            ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL1_REG, (c | ANX7150_INFO_PKTCTRL1_ACR_EN));

            //audio enable: 
            ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, &c);
            c |= ANX7150_HDMI_AUDCTRL1_IN_EN;
            ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, c);

            ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL0_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL0_REG, (c & 0xef));
            ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, (c | 0x01));
            ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDBIST_CTRL_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDBIST_CTRL_REG, (c | 0xcf));

            ANX7150_Set_System_State(ANX7150_CONFIG_PACKETS);
        }

        if(ANX7150_system_state == ANX7150_CONFIG_PACKETS)/* {
            s_ANX7150_packet_config.packets_need_config=1;
             i2c_read_p0_reg(0x5f, &c);
            if(c < 0x80) 
                ANX7150_RGBorYCbCr = ANX7150_RGB;
            else if(c >= 0xc0)
                ANX7150_RGBorYCbCr = ANX7150_YCbCr422;
            else
                ANX7150_RGBorYCbCr = ANX7150_YCbCr444;
            ANX7150_Config_Packet();}*/
        {
            s_ANX7150_packet_config.packets_need_config = 0x00;
          //config Y0, Y1 of AVI
            c1 = ANX7150_edid_result.ycbcr444_supported;
            temp = ANX7150_edid_result.ycbcr422_supported;
            c1 = (c1<<1) + temp;
          /*  i2c_read_p0_reg(0x5f,&c);
            if(c < 0x80) 
                c1 = 0;
            else if(c >= 0xc0)
                c1 = 1;
            else
                c1 = 2;*/
            
            ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL1_REG, &c);

            c &= ~ANX7150_INFO_PKTCTRL1_AVI_RPT;
            ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL1_REG, c);
            switch (c1) 
            {
                case 0: //config to RGB mode
                    ANX7150_avi_data[4] = ANX7150_avi_data[4] & 0x9f;
                    break;
                case 1: //config to YCbCr4:2:2 mode
                    ANX7150_avi_data[4] = (ANX7150_avi_data[4] & 0x9f) | 0x20;
                    break;
                case 2: //config to YCbCr4:4:4 mode
                    ANX7150_avi_data[4] = (ANX7150_avi_data[4] & 0x9f) | 0x40;
                    break;
                case 3: //config to YCbCr4:2:2 mode
                    ANX7150_avi_data[4] = (ANX7150_avi_data[4] & 0x9f) | 0x20;
            }

          //config AVI data byte4
            switch (switch_value) 
            { 
                case 0: //640x480p@60
                    ANX7150_avi_data[7] = 0x01;
                    ANX7150_avi_data[5] = 0x58; //ITU601, Pixcure Aspect Ration = 4:3, ACtive Format Aspect Ration same as picture aspect ratio.
                    break;
                case 1: //1280x720p@60
                    ANX7150_avi_data[7] = 0x04;
                    ANX7150_avi_data[5] = 0xa8; //ITU709, Pixcure Aspect Ration = 16:9, ACtive Format Aspect Ration same as picture aspect ratio.          
                    break;
                case 2: //1920x10800i@60
                    ANX7150_avi_data[7] = 0x05;
                    ANX7150_avi_data[5] = 0xa8; //ITU709, Pixcure Aspect Ration = 16:9, ACtive Format Aspect Ration same as picture aspect ratio.          
                    break;
                case 3: //1920x1080p@60
                    ANX7150_avi_data[7] = 0x10;
                    ANX7150_avi_data[5] = 0xa8; //ITU709, Pixcure Aspect Ration = 16:9, ACtive Format Aspect Ration same as picture aspect ratio.          
            }

          //Caculate Checksum
            ANX7150_avi_data[3] = 0;
            c =0;
            for(i=0; i<17;i++) 
            {
                c = c + ANX7150_avi_data[i];
            }
            c = (~c) +1;
            ANX7150_avi_data[3] = c;

            ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL1_REG, &c);
            if(c & ANX7150_INFO_PKTCTRL1_AVI_EN)
            {
                //ANX7150_DEBUG("wait disable, config avi infoframe packet.");
                return ;
            }

        //Config AVI infoframe
            for(i=0; i<17; i++ )
            {
                ANX7150_i2c_write_p1_reg(i, ANX7150_avi_data[i]);
            }

           //Enable send AVI InfoFrame 
            ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL1_REG, &c);
            ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL1_REG, (c | 0x30));    

            //Config Audio Infoframe
            ANX7150_Load_Infoframe( ANX7150_audio_infoframe,
                                   &(s_ANX7150_packet_config.audio_info));

            ANX7150_i2c_read_p1_reg(ANX7150_INFO_PKTCTRL2_REG, &c);
            c |= 0x03;
            ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL2_REG, c);

            //Config Channel Status
            c = s_ANX7150_audio_config.i2s_config.Channel_status1;
            ANX7150_i2c_write_p0_reg(ANX7150_I2SCH_STATUS1_REG, c);
            c = s_ANX7150_audio_config.i2s_config.Channel_status2;
            ANX7150_i2c_write_p0_reg(ANX7150_I2SCH_STATUS2_REG, c);
            c = s_ANX7150_audio_config.i2s_config.Channel_status3;
            ANX7150_i2c_write_p0_reg(ANX7150_I2SCH_STATUS3_REG, c);
            c = s_ANX7150_audio_config.i2s_config.Channel_status4;
            ANX7150_i2c_write_p0_reg(ANX7150_I2SCH_STATUS4_REG, c);
            c = s_ANX7150_audio_config.i2s_config.Channel_status5;
            ANX7150_i2c_write_p0_reg(ANX7150_I2SCH_STATUS5_REG, c);
            
            ANX7150_Set_System_State(ANX7150_HDCP_AUTHENTICATION);
        }
        
        if(ANX7150_system_state == ANX7150_HDCP_AUTHENTICATION)
        {
            ANX7150_HDCP_Process();
        }
        
        if(ANX7150_system_state == ANX7150_PLAY_BACK)
        {            
            ANX7150_PLAYBACK_Process();
        }
    
}




void ANX7150_Reset_BIST_Setting(void) 
{
    BYTE c;
    //Reset video mode register to disable up sampling.
    ANX7150_i2c_read_p0_reg(ANX7150_VID_MODE_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_VID_MODE_REG, (c & 0xfb));
}

void ANX7150_Config_Bist_Video(WORD bist_select_number) 
{
    WORD bist_data;
    BYTE c, c1;

    bist_data = bist_demo[bist_select_number].h_total_length;
    ANX7150_i2c_write_p0_reg(ANX7150_H_RESL_REG,  bist_data);
    ANX7150_i2c_write_p0_reg(ANX7150_H_RESH_REG,  (bist_data>>8));

    bist_data = bist_demo[bist_select_number].h_active_length;
    ANX7150_i2c_write_p0_reg(ANX7150_VID_PIXL_REG,  bist_data);
    ANX7150_i2c_write_p0_reg(ANX7150_VID_PIXH_REG,  (bist_data>>8));

    bist_data = bist_demo[bist_select_number].v_total_length;
    ANX7150_i2c_write_p0_reg(ANX7150_V_RESL_REG,  bist_data);
    ANX7150_i2c_write_p0_reg(ANX7150_V_RESH_REG,  (bist_data>>8));

    bist_data = bist_demo[bist_select_number].v_active_length;
    ANX7150_i2c_write_p0_reg(ANX7150_ACT_LINEL_REG,  bist_data);
    ANX7150_i2c_write_p0_reg(ANX7150_ACT_LINEH_REG,  (bist_data>>8));

    bist_data = bist_demo[bist_select_number].h_front_porch;
    ANX7150_i2c_write_p0_reg(ANX7150_H_FRONTPORCHL_REG,  bist_data);
    ANX7150_i2c_write_p0_reg(ANX7150_H_FRONTPORCHH_REG,  (bist_data>>8));

    bist_data = bist_demo[bist_select_number].h_sync_width;
    ANX7150_i2c_write_p0_reg(ANX7150_HSYNC_ACT_WIDTHL_REG,  bist_data);
    ANX7150_i2c_write_p0_reg(ANX7150_HSYNC_ACT_WIDTHH_REG,  (bist_data>>8));

    bist_data = bist_demo[bist_select_number].h_back_porch;
    ANX7150_i2c_write_p0_reg(ANX7150_H_BACKPORCHL_REG,  bist_data);
    ANX7150_i2c_write_p0_reg(ANX7150_H_BACKPORCHH_REG,  (bist_data>>8));

    bist_data = bist_demo[bist_select_number].v_front_porch;
    ANX7150_i2c_write_p0_reg(ANX7150_ACT_LINE2VSYNC_REG,  bist_data);

    bist_data = bist_demo[bist_select_number].v_sync_width;
    ANX7150_i2c_write_p0_reg(ANX7150_VSYNC_WID_REG,  bist_data);

    bist_data = bist_demo[bist_select_number].v_back_porch;
    ANX7150_i2c_write_p0_reg(ANX7150_VSYNC_TAIL2VIDLINE_REG,  bist_data);

    bist_data = bist_demo[bist_select_number].h_sync_polarity;
    ANX7150_i2c_read_p0_reg(ANX7150_VID_CAPCTRL1_REG, &c);
    if(bist_data == 1) 
    {
        ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL1_REG, (c | ANX7150_VID_CAPCTRL1_HSYNC_POL));
    } 
    else 
    {
        ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL1_REG, (c & 0xdf));
    }

    bist_data = bist_demo[bist_select_number].v_sync_polarity;
    ANX7150_i2c_read_p0_reg(ANX7150_VID_CAPCTRL1_REG, &c);
    if(bist_data == 1) 
    {
        ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL1_REG, (c | ANX7150_VID_CAPCTRL1_VSYNC_POL));
    } 
    else 
    {
        ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL1_REG, (c & 0xbf));
    }

    bist_data = bist_demo[bist_select_number].is_interlaced;
    ANX7150_i2c_read_p0_reg(ANX7150_VID_CAPCTRL1_REG, &c);
    if(bist_data == 1) 
    {
        ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL1_REG, (c | ANX7150_VID_CAPCTRL1_VID_TYPE));
    } 
    else 
    {
        ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL1_REG, (c & 0xf7));
    }

    c1 = ANX7150_edid_result.ycbcr444_supported;
    c1 = (c1<<1) + (BYTE) ANX7150_edid_result.ycbcr422_supported;
   /* i2c_read_p0_reg(0x5f,&c);
    if(c < 0x80) 
        c1 = 0;
    else if(c >= 0xc0)
        c1 = 1;
    else
        c1 = 2;*/

    ANX7150_i2c_write_p0_reg(ANX7150_VID_MODE_REG, 0x00);
    
    switch (c1) 
    {
        case 0: //select RGB mode
            ANX7150_i2c_read_p0_reg(ANX7150_VID_CTRL_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_VID_CTRL_REG,((c & 0x1f) | 0x20));
            break;
        case 1: //select YCbCr4:2:2 mode
            ANX7150_i2c_read_p0_reg(ANX7150_VID_CTRL_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_VID_CTRL_REG,((c & 0x1f) | 0xa0)); 
            break;
        case 2: //select YCbCr4:4:4 mode
            ANX7150_i2c_read_p0_reg(ANX7150_VID_CTRL_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_VID_CTRL_REG,((c & 0x1f) | 0xa0));
            //do upsample(from YCbCr4:2:2 to YCbCr4:4:4) 
            ANX7150_i2c_read_p0_reg(ANX7150_VID_MODE_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_VID_MODE_REG, (c |0x04));
            break;
        case 3: //Both YCbCr4:2:2 and YCbCr4:4:4 modes supported, then select YCbCr4:2:2 mode
            ANX7150_i2c_read_p0_reg(ANX7150_VID_CTRL_REG, &c);
            ANX7150_i2c_write_p0_reg(ANX7150_VID_CTRL_REG,((c & 0x1f) | 0xa0));
    }

    bist_data = bist_demo[bist_select_number].video_mode;
    ANX7150_i2c_read_p0_reg(ANX7150_VID_STATUS_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_VID_STATUS_REG, ((c & 0x3f) | (bist_data<<6))); 

    ANX7150_i2c_read_p0_reg(ANX7150_VID_CAPCTRL0_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL0_REG, (c | ANX7150_VID_CAPCTRL0_VIDBIST_EN));	


}

/*
AVI infoframe location
*/
void ANX7150_Reset_AVI(void) 
{
    ANX7150_avi_data[0] = 0x82; //type, HDMI defined
    ANX7150_avi_data[1] = 0x02; //Version
    ANX7150_avi_data[2] = 0x0d; //AVI data byte length is 13
    ANX7150_avi_data[3] = 0x00; //checksum
    ANX7150_avi_data[4] = 0x00; //data byte 1
    ANX7150_avi_data[5] = 0x08; //data byte 2
    ANX7150_avi_data[6] = 0x00; //data byte 3
    ANX7150_avi_data[7] = 0x01; //data byte 4
    ANX7150_avi_data[8] = 0x00; //data byte 5
    ANX7150_avi_data[9] = 0x00; //data byte 6
    ANX7150_avi_data[10] = 0x00; //data byte 7
    ANX7150_avi_data[11] = 0x00; //data byte 8
    ANX7150_avi_data[12] = 0x00; //data byte 9
    ANX7150_avi_data[13] = 0x00; //data byte 10
    ANX7150_avi_data[14] = 0x00; //data byte 11
    ANX7150_avi_data[15] = 0x00; //data byte 12
    ANX7150_avi_data[16] = 0x00; //data byte 13
    ANX7150_avi_data[17] = 0x00; //for firmware use
    ANX7150_avi_data[18] = 0x00; //for firmware use
}    
#endif 


/*DDC operate start*/


/*Function name  :ANX7150_DDC_Parameter_Validity()*/
/*Function  :Judge the validity for input parameter*/
/*Parameter  :   Addr,length*/
/*Return  :Judge result is DDC_Data_Addr_Err,DDC_Length_Err,DDC_NO_Err*/
BYTE ANX7150_DDC_Parameter_Validity(BYTE *Addr,WORD length)
{
    if (Addr == NULL){
        return DDC_Data_Addr_Err;
    }
    else if (length > DDC_Max_Length){
        return DDC_Length_Err;
    }
    else{
        return DDC_NO_Err;
    }
 
}

/*Function name :ANX7150_DDC_Set_Address()*/
/*Function  :Set address for DDC device*/
/*Parameter :   ddc_address,ddc_type*/
/*Return  :None*/
void ANX7150_DDC_Set_Address(ANX7150_DDC_Addr ddc_address, ANX7150_DDC_Type ddc_type)
{
    /*set DDC channel slave device address*/
    ANX7150_i2c_write_p0_reg(ANX7150_DDC_SLV_ADDR_REG, ddc_address.dev_addr);
    /*set DDC channel slave segment address,when ddc type is edid*/
    if (ddc_type == DDC_Edid)
        ANX7150_i2c_write_p0_reg(ANX7150_DDC_SLV_SEGADDR_REG, ddc_address.sgmt_addr);
    /*set DDC channel slave offset address*/
    ANX7150_i2c_write_p0_reg(ANX7150_DDC_SLV_OFFADDR_REG, ddc_address.offset_addr);
}

/*Function name :ANX7150_DDC_Set_Number()*/
/*Function  :Set number for DDC data access*/
/*Parameter :   length*/
/*Return  :None*/
void ANX7150_DDC_Set_Number(WORD length)
{
    BYTE length_low,length_high;

    ANX7150_DEBUG ("!!!!DDC_data_number :%x\n",length);
    length_high = (BYTE)((length >> 8) & 0xff);
    length_low = (BYTE)(length & 0xff);
    /*set number of bytes to DDC channel*/
    ANX7150_i2c_write_p0_reg(ANX7150_DDC_ACCNUM0_REG, length_low);
    ANX7150_i2c_write_p0_reg(ANX7150_DDC_ACCNUM1_REG, length_high);
}

/*Function name :ANX7150_DDC_Command()*/
/*Function  :Send command to DDC*/
/*Parameter :   DDC_command*/
/*Return  :None*/
void ANX7150_DDC_Command(BYTE DDC_command)
{
    /*set DDC command*/
    ANX7150_i2c_write_p0_reg(ANX7150_DDC_ACC_CMD_REG, DDC_command);
    
}

/*Function name :ANX7150_DDC_Check_Status*/
/*Function  :Check DDC status or report information of error*/
/*Parameter :   DDC_status_need_type,status_bit*/
/*Return  :always is 0 if DDC_status_need_status is "report",or return DDC status bit value
if DDC_status_need_type is "Judge"*/
BYTE ANX7150_DDC_Check_Status(ANX7150_DDC_Status_Check_Type DDC_status_need_type,
        BYTE status_bit)
{
    BYTE DDC_status,i,j;
    BYTE *status[8] = {
                                "!!!!DDC____An Error is Occurred!\n",
                                "!!!!DDC____channel is accessed by an external device!\n",
                                "!!!!DDC____Fifo is Full!\n",
                                "!!!!DDC____Fifo is Empty!\n",
                                "!!!!DDC____No Acknowledge detection!\n",
                                "!!!!DDC____Fifo is being read!\n",
                                "!!!!DDC____Fifo is being written!\n",
                                };
    
    ANX7150_i2c_read_p0_reg(ANX7150_DDC_CHSTATUS_REG, &DDC_status);
    
    if (DDC_status_need_type == report){
        for (i= 0,j=7; i < 8; i++,j--){
            if (DDC_status & (0x01 << i)){
                ANX7150_DEBUG("%s",status[j]);
                }
        }
        return 0;
    }
    else{
        return ((DDC_status >> status_bit) & 0x01 );
    }
}

/*Function name :ANX7150_DDC_Count_Compare()*/
/*Function :Check status for access count*/
/*Parameter :length*/
/*Return  :0 if check success,1 if check fail*/
BYTE ANX7150_DDC_Count_Compare(BYTE length)
{
    BYTE Fifo_Count;
    ANX7150_i2c_read_p0_reg(ANX7150_DDC_FIFOCNT_REG, &Fifo_Count);
    return ((Fifo_Count & 0x01f) == length ? 0 : 1);
}

/*Function name:    ANX7150_DDC_Read()*/
/*Function :    read data from DDC channel*/
/*Parameter :   ddc_address,DDC_data,length,DDC_type*/
/*Return  :Read reslut,ex.DDC_NO_Err,DDC_Status_Err*/
BYTE ANX7150_DDC_Read (ANX7150_DDC_Addr ddc_address, const BYTE *DDC_data, 
    WORD length, ANX7150_DDC_Type DDC_type)
{
    BYTE DDC_Err,fifo_read_data_compare;
    int data_cnt;
    int fifo_data_cnt;
    BYTE *Fifo_Read_P;

    DDC_Err = DDC_NO_Err;
    fifo_read_data_compare = 0;
    fifo_data_cnt = length;
    Fifo_Read_P = DDC_data;
    
    /*Judge validity for read address and length*/
    /*if (DDC_Err = ANX7150_DDC_Parameter_Validity(Fifo_Read_P, length))
        return (DDC_Err);
    */ 																																		//update    
        if (DDC_Err != ANX7150_DDC_Parameter_Validity(Fifo_Read_P, length))		//update
        return (ANX7150_DDC_Parameter_Validity(Fifo_Read_P, length));
				
    /*set DDC address*/
    ANX7150_DDC_Set_Address(ddc_address, DDC_type);
    /*set number for DDC read*/
    ANX7150_DDC_Set_Number(length);
    /*send "clear DDC fifo" command*/
    ANX7150_DDC_Command((BYTE)Clear_DDC_Fifo);
    
    /*check DDC channel status*/
    if (!ANX7150_DDC_Check_Status(Judge, DDC_Error_bit) &&
        !ANX7150_DDC_Check_Status(Judge, DDC_Occup_bit) &&
        !ANX7150_DDC_Check_Status(Judge, DDC_No_Ack_bit)){
        /*send "sequential byte read"command if check success*/
        ANX7150_DDC_Command((BYTE)Sequential_Byte_Read);
        /*delay*/
        msleep(DDC_Read_Delay);
    }
    else{
        ANX7150_DDC_Check_Status(report, NULL);
        return DDC_Status_Err;
    }
    
    /*read DDC fifo data*/
    do {
        /*read data from fifo if length <= DDC fifo depth*/
        if (fifo_data_cnt <= DDC_Fifo_Depth){

            fifo_read_data_compare = fifo_data_cnt;
            
            /*check bit DDC_Progress of DDC status,fifo count*/
            if (!ANX7150_DDC_Check_Status(Judge, DDC_Progress_bit) &&
                !ANX7150_DDC_Count_Compare(fifo_read_data_compare)){
                data_cnt = fifo_data_cnt;
                while (data_cnt--){
                    ANX7150_i2c_read_p0_reg(ANX7150_DDC_FIFO_ACC_REG, Fifo_Read_P++);
                }
            }
            else{
                ANX7150_DDC_Check_Status(report, NULL);
                return DDC_Status_Err;
            }
        }
        /*read data from fifo if length >DDC fifo depth*/
        else{
            /*check bit DDC_Progress of DDC status,Fifo_full*/
            if (ANX7150_DDC_Check_Status(Judge, DDC_Progress_bit) &&
                ANX7150_DDC_Check_Status(Judge, DDC_Fifo_Full_bit)){
                data_cnt = DDC_Fifo_Depth;
                while(data_cnt--){
                    ANX7150_i2c_read_p0_reg(ANX7150_DDC_FIFO_ACC_REG, Fifo_Read_P++);
                }
            }
            else{
                ANX7150_DDC_Check_Status(report, NULL);
                return DDC_Status_Err;
            }
                
        }
        fifo_data_cnt -= DDC_Fifo_Depth;
    }while(fifo_data_cnt > 0);
    
    return DDC_Err;
}

void ANX7150_Timer_Process_Re_init() 
{
	timer_slot =-1;
}


/*Function name:    ANX7150_DDC_Write()*/
/*Function :    write data to DDC channel*/
/*Parameter :   ddc_address,DDC_data,length,DDC_type*/
/*Return  :write reslut,ex.DDC_NO_Err,DDC_Status_Err*/
/*					
BYTE ANX7150_DDC_Write (ANX7150_DDC_Addr ddc_address, const BYTE *DDC_data, 
    WORD length, ANX7150_DDC_Type DDC_type)
{
    BYTE DDC_Err,fifo_write_data_compare;
    int data_cnt;
    int fifo_data_cnt;
    BYTE *Fifo_Write_P;
    
    DDC_Err = DDC_NO_Err;
    fifo_write_data_compare = 0;
    fifo_data_cnt = length;
    Fifo_Write_P = DDC_data;
    
    //Judge validity for write address and length
    if (DDC_Err = ANX7150_DDC_Parameter_Validity(Fifo_Write_P, length))
        return (DDC_Err);

    //set DDC address
    ANX7150_DDC_Set_Address(ddc_address, DDC_type);
    //set number for DDC write
    ANX7150_DDC_Set_Number(length);
    //send "clear DDC fifo" command
    ANX7150_DDC_Command((BYTE)Clear_DDC_Fifo);
    
    //check DDC channel status//
    if (!ANX7150_DDC_Check_Status(Judge, DDC_Error_bit) &&
        !ANX7150_DDC_Check_Status(Judge, DDC_Occup_bit) &&
        !ANX7150_DDC_Check_Status(Judge, DDC_No_Ack_bit)){

            do {
                //write data to fifo if data length <= DDC Fifo Depth/
                if (fifo_data_cnt <= DDC_Fifo_Depth){
                    
                    fifo_write_data_compare = fifo_data_cnt;
                    
                    while (fifo_data_cnt--){
                        //write data to DDC fifo
                        ANX7150_i2c_write_p0_reg(ANX7150_DDC_FIFO_ACC_REG, *(Fifo_Write_P++));
                    }
                    //send "sequential byte write"command after finish writing data to fifo
                    ANX7150_DDC_Command((BYTE)Sequential_Byte_Write);
                    //delay
                    msleep(DDC_Write_Delay);
                    //check DDC status when data length <= DDC Fifo Depth
                    if (ANX7150_DDC_Check_Status(Judge, DDC_Progress_bit) ||
                        ANX7150_DDC_Count_Compare(fifo_write_data_compare)){
                        ANX7150_DDC_Check_Status(report, NULL);
                        return DDC_Status_Err;
                    }
                }
                //write data to fifo if data length > DDC Fifo Depth
                else{
                    data_cnt = DDC_Fifo_Depth;
                    while(data_cnt--){
                        ANX7150_i2c_write_p0_reg(ANX7150_DDC_FIFO_ACC_REG, *(Fifo_Write_P++));
                    }
                    //send "sequential byte write"command after finish writing data to fifo
                    ANX7150_DDC_Command((BYTE)Sequential_Byte_Write);
                    //delay
                    msleep(DDC_Write_Delay);
                    //check DDC status when data length > DDC Fifo Depth
                    if (!ANX7150_DDC_Check_Status(Judge, DDC_Progress_bit) ||
                        !ANX7150_DDC_Check_Status(Judge, DDC_Fifo_Full_bit)){
                        ANX7150_DDC_Check_Status(report, NULL);
                        return DDC_Status_Err;
                    }
                    
                    fifo_data_cnt -= DDC_Fifo_Depth;
                }
                
            }while(fifo_data_cnt > 0);
    }
    else{
        ANX7150_DDC_Check_Status(report, NULL);
        return DDC_Status_Err;    
    }
   
    return DDC_Err;
}
*/

                                                                /*end*/

