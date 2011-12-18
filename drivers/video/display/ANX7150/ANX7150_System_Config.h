
#include <linux/i2c.h>
#include "ANX7150_Sys7150.h"

//user interface define begins

//select video hardware interface
#define ANX7150_VID_HW_INTERFACE 0x03//0x00:RGB and YcbCr 4:4:4 Formats with Separate Syncs (24-bpp mode)
                                                                 //0x01:YCbCr 4:2:2 Formats with Separate Syncs(16-bbp)
                                                                 //0x02:YCbCr 4:2:2 Formats with Embedded Syncs(No HS/VS/DE)
                                                                 //0x03:YC Mux 4:2:2 Formats with Separate Sync Mode1(bit15:8 and bit 3:0 are used)
                                                                 //0x04:YC Mux 4:2:2 Formats with Separate Sync Mode2(bit11:0 are used)
                                                                 //0x05:YC Mux 4:2:2 Formats with Embedded Sync Mode1(bit15:8 and bit 3:0 are used)
                                                                 //0x06:YC Mux 4:2:2 Formats with Embedded Sync Mode2(bit11:0 are used)
                                                                 //0x07:RGB and YcbCr 4:4:4 DDR Formats with Separate Syncs
                                                                 //0x08:RGB and YcbCr 4:4:4 DDR Formats with Embedded Syncs
                                                                 //0x09:RGB and YcbCr 4:4:4 Formats with Separate Syncs but no DE
                                                                 //0x0a:YCbCr 4:2:2 Formats with Separate Syncs but no DE
//select input color space
#define ANX7150_INPUT_COLORSPACE 0x01//0x00: input color space is RGB
                                                                //0x01: input color space is YCbCr422
                                                                //0x02: input color space is YCbCr444
//select input pixel clock edge for DDR mode
#define ANX7150_IDCK_EDGE_DDR 0x00  //0x00:use rising edge to latch even numbered pixel data//jack wen
                                                                //0x01:use falling edge to latch even numbered pixel data 

//select audio hardware interface
#define ANX7150_AUD_HW_INTERFACE 0x02//0x01:audio input comes from I2S
                                                                  //0x02:audio input comes from SPDIF
                                                                  //0x04:audio input comes from one bit audio
//select MCLK and Fs relationship if audio HW interface is I2S
#define ANX7150_MCLK_Fs_RELATION 0x01//0x00:MCLK = 128 * Fs
                                                                //0x01:MCLK = 256 * Fs
                                                                //0x02:MCLK = 384 * Fs
                                                                //0x03:MCLK = 512 * Fs			//wen updated error

#define ANX7150_AUD_CLK_EDGE 0x00  //0x00:use MCLK and SCK rising edge to latch audio data
                                                                //0x08, revised by wen. //0x80:use MCLK and SCK falling edge to latch audio data
//select I2S channel numbers if audio HW interface is I2S
#define ANX7150_I2S_CH0_ENABLE 0x01 //0x01:enable channel 0 input; 0x00: disable
#define ANX7150_I2S_CH1_ENABLE 0x00 //0x01:enable channel 0 input; 0x00: disable
#define ANX7150_I2S_CH2_ENABLE 0x00 //0x01:enable channel 0 input; 0x00: disable
#define ANX7150_I2S_CH3_ENABLE 0x00 //0x01:enable channel 0 input; 0x00: disable
//select I2S word length if audio HW interface is I2S
#define ANX7150_I2S_WORD_LENGTH 0x0b
                                        //0x02 = 16bits; 0x04 = 18 bits; 0x08 = 19 bits; 0x0a = 20 bits(maximal word length is 20bits); 0x0c = 17 bits;
                                        // 0x03 = 20bits(maximal word length is 24bits); 0x05 = 22 bits; 0x09 = 23 bits; 0x0b = 24 bits; 0x0d = 21 bits;

//select I2S format if audio HW interface is I2S
#define ANX7150_I2S_SHIFT_CTRL 0x00//0x00: fist bit shift(philips spec)
                                                                //0x01:no shift
#define ANX7150_I2S_DIR_CTRL 0x00//0x00:SD data MSB first
                                                            //0x01:LSB first
#define ANX7150_I2S_WS_POL 0x00//0x00:left polarity when word select is low
                                                        //0x01:left polarity when word select is high
#define ANX7150_I2S_JUST_CTRL 0x00//0x00:data is left justified
                                                             //0x01:data is right justified

//user interface define ends

extern BYTE ANX7150_new_HW_interface_parameter;

void ANX7150_API_System_Config(void);
void ANX7150_8770_Video_Config(void);
void ANX7150_8770_Packets_Config(void);
void ANX7150_8770_Audio_Config(void);
void ANX7150_API_Video_Config(BYTE video_id,BYTE input_pixel_rpt_time);
void ANX7150_API_AUD_CHStatus_Config(BYTE MODE,BYTE PCM_MODE,BYTE SW_CPRGT,BYTE NON_PCM,
    BYTE PROF_APP,BYTE CAT_CODE,BYTE CH_NUM,BYTE SOURCE_NUM,BYTE CLK_ACCUR,BYTE Fs);
int ANX7150_API_DetectDevice(void);//BIT ANX7150_API_DetectDevice(void);
void ANX7150_API_HoldSystemConfig(int bHold_ANX7150);//void ANX7150_API_HoldSystemConfig(BIT bHold_ANX7150);
void ANX7150_API_ShutDown(int bShutDown_ANX7150);//void ANX7150_API_ShutDown(BIT bShutDown_ANX7150);
void ANX7150_API_HDCP_ONorOFF(int HDCP_ONorOFF);//void ANX7150_API_HDCP_ONorOFF(BIT HDCP_ONorOFF);
void ANX7150_API_Packets_Config(BYTE pkt_sel);//void ANX7150_API_Packets_Config(BYTE pkt_sel);
void ANX7150_API_AVI_Config(BYTE pb1,BYTE pb2,BYTE pb3,BYTE pb4,BYTE pb5,
    BYTE pb6,BYTE pb7,BYTE pb8,BYTE pb9,BYTE pb10,BYTE pb11,BYTE pb12,BYTE pb13);
void ANX7150_API_AUD_INFO_Config(BYTE pb1,BYTE pb2,BYTE pb3,BYTE pb4,BYTE pb5,
    BYTE pb6,BYTE pb7,BYTE pb8,BYTE pb9,BYTE pb10);
void ANX7150_API_Input_Changed(void);
//void ANX7150_User_Change_Input ();		//wen
void ANX7150_API_Set_AVMute(void);
void ANX7150_API_Clean_HDCP(void);
void ANX7150_API_Audio_Config(BYTE aud_fs);

