/*
 * rt5623.c  --  RT5623 ALSA Soc Audio driver
 *
 * Copyright 2008 Realtek Microelectronics
 *
 * Author: flove <flove@realtek.com> Ethan <eku@marvell.com>
 *
 * Based on WM8753.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <asm/div64.h>

#include "rt5623.h"

#define AUDIO_NAME "rt5623"
#define RT5623_VERSION "0.01"


/*
 * Debug
 */
#ifdef CONFIG_SND_DEBUG
#define s3cdbg(x...) printk(x)
#else
#define s3cdbg(x...)
#endif

#define RT5623_DEBUG 0

#ifdef RT5623_DEBUG
#define dbg(format, arg...) \
	printk(KERN_DEBUG AUDIO_NAME ": " format "\n" , ## arg)
#else
#define dbg(format, arg...) do {} while (0)
#endif
#define err(format, arg...) \
	printk(KERN_ERR AUDIO_NAME ": " format "\n" , ## arg)
#define info(format, arg...) \
	printk(KERN_INFO AUDIO_NAME ": " format "\n" , ## arg)
#define warn(format, arg...) \
	printk(KERN_WARNING AUDIO_NAME ": " format "\n" , ## arg)

static int caps_charge = 2000;
module_param(caps_charge, int, 0);
MODULE_PARM_DESC(caps_charge, "RT5623 cap charge time (msecs)");



/* codec private data */
struct rt5623_priv {
	unsigned int sysclk;
};


static struct snd_soc_device *rt5623_socdev;


/*
 * rt5623 register cache
 * We can't read the RT5623 register space when we
 * are using 2 wire for device control, so we cache them instead.
 */
static const u16 rt5623_reg[0x7E/2];


/* virtual HP mixers regs */
#define HPL_MIXER	0x80
#define HPR_MIXER	0x82

static u16 reg80=0,reg82=0;

static u16 Set_Codec_Reg_Init[][2]={
	{RT5623_AUDIO_INTERFACE	,0x8000},// 0x34 set I2S codec to slave mode
	{RT5623_STEREO_DAC_VOL	,0x0808},// 0x0c default stereo DAC volume to 0db
	{RT5623_OUTPUT_MIXER_CTRL,0x1740},// 0x1c default output mixer control	
//	{RT5623_ADC_REC_MIXER	,0x3f3f},// 0x14 set record source is Mic1 by default
	{RT5623_ADC_REC_MIXER	,0x5f5f},// 0x14 set record source is Mic2 by default
	{RT5623_MIC_CTRL		,0x0500},// 0x22 set Mic1,Mic2 boost 20db	
	{RT5623_SPK_OUT_VOL		,0x8080},// 0x02 default speaker volume to 0db 
	{RT5623_HP_OUT_VOL		,0x8080},// 0x04 default HP volume to 0db
	{RT5623_ADC_REC_GAIN            ,0xf891},// record gain 9 db
};

#define SET_CODEC_REG_INIT_NUM	ARRAY_SIZE(Set_Codec_Reg_Init)

/*
 * read rt5623 register cache
 */
static inline unsigned int rt5623_read_reg_cache(struct snd_soc_codec *codec,
	unsigned int reg)
{
	u16 *cache = codec->reg_cache;
	if (reg < 1 || reg > (ARRAY_SIZE(rt5623_reg) + 1))
		return -1;
	return cache[reg/2];
}

/*
 * write rt5623 register cache
 */
static inline void rt5623_write_reg_cache(struct snd_soc_codec *codec,
	unsigned int reg, unsigned int value)
{
	u16 *cache = codec->reg_cache;
	if (reg < 0 || reg > 0x7e)
		return;
	cache[reg/2] = value;
}


static int rt5623_write(struct snd_soc_codec *codec, unsigned int reg,
	unsigned int value)
{
	u8 data[3];

	if(reg>0x7E)
	{
		if(reg==HPL_MIXER)
			reg80=value;
		else if(reg==HPR_MIXER)
			reg82=value;
		else
			return -EIO;
	
		return 0;	
	}	

//	printk("rt5623 write reg=%x,value=%x\n",reg,value);
	data[0] = reg;
	data[1] = (0xFF00 & value) >> 8;
	data[2] = 0x00FF & value;

	if (codec->hw_write(codec->control_data, data, 3) == 3)
	{
		rt5623_write_reg_cache (codec, reg, value);		
//		printk("rt5623 write OK\n");
		return 0;
	}
	else
	{
//		printk("rt5623 write faile\n");
		return -EIO;
	}
}

static unsigned int rt5623_read(struct snd_soc_codec *codec, unsigned int reg)
{
	u8 data[2]={0};
	unsigned int value=0x0;

	if(reg>0x7E)
	{
		if(reg==HPL_MIXER)
		 return	reg80;
		else if(reg==HPR_MIXER)
		 return	reg82;
		else
		 return -EIO;
		 
		 return -EIO;	
	}

	data[0] = reg;
	if(codec->hw_write(codec->control_data, data, 1) ==1)
	{
		i2c_master_recv(codec->control_data, data, 2);
		value = (data[0]<<8) | data[1];		
//		printk("rt5623 read reg%x=%x\n",reg,value);
		
		return value;
	}
	else
	{
//		printk("rt5623 read faile\n");
		return -EIO;			
	}		

}

static int rt5623_write_mask(struct snd_soc_codec *codec, unsigned int reg,unsigned int value,unsigned int mask)
{
	
	unsigned char RetVal=0;
	unsigned  int CodecData;

	if(!mask)
		return 0; 

	if(mask!=0xffff)
	 {
		CodecData=rt5623_read(codec,reg);		
		CodecData&=~mask;
		CodecData|=(value&mask);
		RetVal=rt5623_write(codec,reg,CodecData);
	 }		
	else
	{
		RetVal=rt5623_write(codec,reg,value);
	}

	return RetVal;
}

#if 0
//*****************************************************************************
//
//function AudioOutEnable:Mute/Unmute audio out channel
//							WavOutPath:output channel
//							Mute :Mute/Unmute output channel											
//
//*****************************************************************************
static int rt5623_AudioOutEnable(struct snd_soc_codec *codec,unsigned short int WavOutPath,int Mute)
{
	int RetVal=0;	


	if(Mute)
	{
		switch(WavOutPath)
		{
			case RT_WAVOUT_ALL_ON:

				RetVal=rt5623_write_mask(codec,RT5623_SPK_OUT_VOL,RT_L_MUTE|RT_R_MUTE,RT_L_MUTE|RT_R_MUTE);	//Mute Speaker right/left channel
				RetVal=rt5623_write_mask(codec,RT5623_HP_OUT_VOL,RT_L_MUTE|RT_R_MUTE,RT_L_MUTE|RT_R_MUTE);	//Mute headphone right/left channel
				RetVal=rt5623_write_mask(codec,RT5623_MONO_AUX_OUT_VOL,RT_L_MUTE|RT_R_MUTE,RT_L_MUTE|RT_R_MUTE);	//Mute Aux/Mono right/left channel
				RetVal=rt5623_write_mask(codec,RT5623_STEREO_DAC_VOL,RT_M_HP_MIXER|RT_M_SPK_MIXER|RT_M_MONO_MIXER
															  ,RT_M_HP_MIXER|RT_M_SPK_MIXER|RT_M_MONO_MIXER);	//Mute DAC to HP,Speaker,Mono Mixer
		
			break;
		
			case RT_WAVOUT_HP:

				RetVal=rt5623_write_mask(codec,RT5623_HP_OUT_VOL,RT_L_MUTE|RT_R_MUTE,RT_L_MUTE|RT_R_MUTE);	//Mute headphone right/left channel
			
			break;

			case RT_WAVOUT_SPK:
				
				RetVal=rt5623_write_mask(codec,RT5623_SPK_OUT_VOL,RT_L_MUTE|RT_R_MUTE,RT_L_MUTE|RT_R_MUTE);	//Mute Speaker right/left channel			

			break;
			
			case RT_WAVOUT_AUXOUT:

				RetVal=rt5623_write_mask(codec,RT5623_MONO_AUX_OUT_VOL,RT_L_MUTE|RT_R_MUTE,RT_L_MUTE|RT_R_MUTE);	//Mute AuxOut right/left channel

			break;

			case RT_WAVOUT_MONO:

				RetVal=rt5623_write_mask(codec,RT5623_MONO_AUX_OUT_VOL,RT_L_MUTE,RT_L_MUTE);	//Mute MonoOut channel		

			break;

			case RT_WAVOUT_DAC:

				RetVal=rt5623_write_mask(codec,RT5623_STEREO_DAC_VOL,RT_M_HP_MIXER|RT_M_SPK_MIXER|RT_M_MONO_MIXER
															  ,RT_M_HP_MIXER|RT_M_SPK_MIXER|RT_M_MONO_MIXER);	//Mute DAC to HP,Speaker,Mono Mixer				
			break;

			default:

				return 0;

		}
	}
	else
	{
		switch(WavOutPath)
		{

			case RT_WAVOUT_ALL_ON:

				RetVal=rt5623_write_mask(codec,RT5623_SPK_OUT_VOL		,0,RT_L_MUTE|RT_R_MUTE);	//Mute Speaker right/left channel
				RetVal=rt5623_write_mask(codec,RT5623_HP_OUT_VOL 		,0,RT_L_MUTE|RT_R_MUTE);	//Mute headphone right/left channel
				RetVal=rt5623_write_mask(codec,RT5623_MONO_AUX_OUT_VOL,0,RT_L_MUTE|RT_R_MUTE);	//Mute Aux/Mono right/left channel
				RetVal=rt5623_write_mask(codec,RT5623_STEREO_DAC_VOL	,0,RT_M_HP_MIXER|RT_M_SPK_MIXER|RT_M_MONO_MIXER);	//Mute DAC to HP,Speaker,Mono Mixer
		
			break;
		
			case RT_WAVOUT_HP:

				RetVal=rt5623_write_mask(codec,RT5623_HP_OUT_VOL,0,RT_L_MUTE|RT_R_MUTE);	//UnMute headphone right/left channel	
					
			break;

			case RT_WAVOUT_SPK:
				
				RetVal=rt5623_write_mask(codec,RT5623_SPK_OUT_VOL,0,RT_L_MUTE|RT_R_MUTE);	//unMute Speaker right/left channel			

			break;
			
			case RT_WAVOUT_AUXOUT:

				RetVal=rt5623_write_mask(codec,RT5623_MONO_AUX_OUT_VOL,0,RT_L_MUTE|RT_R_MUTE);//unMute AuxOut right/left channel

			break;

			case RT_WAVOUT_MONO:

				RetVal=rt5623_write_mask(codec,RT5623_MONO_AUX_OUT_VOL,0,RT_L_MUTE);	//unMute MonoOut channel		

			break;

			case RT_WAVOUT_DAC:

				RetVal=rt5623_write_mask(codec,RT5623_STEREO_DAC_VOL,0,RT_M_HP_MIXER|RT_M_SPK_MIXER|RT_M_MONO_MIXER);	//unMute DAC to HP,Speaker,Mono Mixer

			default:
				return 0;
		}

	}
	
	return RetVal;
}
#endif
//*****************************************************************************
//
//function:Enable/Disable ADC input source control
//
//*****************************************************************************
static int Enable_ADC_Input_Source(struct snd_soc_codec *codec,unsigned short int ADC_Input_Sour,int Enable)
{
	int bRetVal=0;
	
	if(Enable)
	{
		//Enable ADC source 
		bRetVal=rt5623_write_mask(codec,RT5623_ADC_REC_MIXER,0,ADC_Input_Sour);
	}
	else
	{
		//Disable ADC source		
		bRetVal=rt5623_write_mask(codec,RT5623_ADC_REC_MIXER,ADC_Input_Sour,ADC_Input_Sour);
	}

	return bRetVal;
}


#define rt5623_reset(c) rt5623_write(c, 0x0, 0)
/*
 * RT5623 Controls
 */

static const char *rt5623_spk_n_sour_sel[] = {"RN", "RP","LN","Vmid"};
static const char *rt5623_spkout_input_sel[] = {"Vmid", "HP mixer","Speaker Mixer","Mono Mixer"};
static const char *rt5623_hpl_out_input_sel[] = {"Vmid", "HP Left mixer"};
static const char *rt5623_hpr_out_input_sel[] = {"Vmid", "HP Right mixer"};
static const char *rt5623_mono_aux_out_input_sel[] = {"Vmid", "HP mixer","Speaker Mixer","Mono Mixer"};
static const char *rt5623_mic_Boost_ctrl_sel[] = {"Bypass", "+20DB","+30DB","+40DB"};

static const struct soc_enum rt5623_enum[] = {
SOC_ENUM_SINGLE(RT5623_OUTPUT_MIXER_CTRL, 14, 4, rt5623_spk_n_sour_sel),// 0
SOC_ENUM_SINGLE(RT5623_OUTPUT_MIXER_CTRL, 10, 4, rt5623_spkout_input_sel),// 1
SOC_ENUM_SINGLE(RT5623_OUTPUT_MIXER_CTRL, 9, 2, rt5623_hpl_out_input_sel),// 2
SOC_ENUM_SINGLE(RT5623_OUTPUT_MIXER_CTRL, 8, 2, rt5623_hpr_out_input_sel),// 3
SOC_ENUM_SINGLE(RT5623_OUTPUT_MIXER_CTRL, 6, 4, rt5623_mono_aux_out_input_sel),// 4
SOC_ENUM_SINGLE(RT5623_MIC_CTRL, 10, 4, rt5623_mic_Boost_ctrl_sel),// 5
SOC_ENUM_SINGLE(RT5623_MIC_CTRL, 8, 4, rt5623_mic_Boost_ctrl_sel),// 6
};	

static const struct snd_kcontrol_new rt5623_snd_controls[] = {
SOC_DOUBLE("Speaker Playback Volume", 	RT5623_SPK_OUT_VOL, 8, 0, 31, 1),	
SOC_DOUBLE("Speaker Playback Switch", 	RT5623_SPK_OUT_VOL, 15, 7, 1, 1),
SOC_DOUBLE("Headphone Playback Volume", RT5623_HP_OUT_VOL, 8, 0, 31, 1),
SOC_DOUBLE("Headphone Playback Switch", RT5623_HP_OUT_VOL,15, 7, 1, 1),
//SOC_DOUBLE("Auxout Playback Volume", 	RT5623_MONO_AUX_OUT_VOL,8, 0, 31, 1),
//SOC_DOUBLE("Auxout Playback Switch", 	RT5623_MONO_AUX_OUT_VOL,15, 7, 1, 1),
SOC_DOUBLE("PCM Playback Volume", 		RT5623_STEREO_DAC_VOL, 8, 0, 31, 1),
SOC_DOUBLE("PCM Playback Switch", 		RT5623_STEREO_DAC_VOL,15, 7, 1, 1),
//SOC_DOUBLE("LineIn Capture Volume",		RT5623_LINE_IN_VOL, 8, 0, 31, 1),
//SOC_DOUBLE("AuxIn Capture Volume",		RT5623_AUXIN_VOL, 8, 0, 31, 1),
//SOC_SINGLE("Mic1 Capture Volume",		RT5623_MIC_VOL, 8, 31, 1),
//SOC_SINGLE("Mic2 Capture Volume",		RT5623_MIC_VOL, 0, 31, 1),
SOC_DOUBLE("Rec Capture Volume",		RT5623_ADC_REC_GAIN, 7, 0, 31, 0),
//SOC_ENUM("Mic 1 Boost", 				rt5623_enum[5]),
SOC_ENUM("Mic 2 Boost", 				rt5623_enum[6]),

};


/* add non dapm controls */
static int rt5623_add_controls(struct snd_soc_codec *codec)
{
	int err, i;

	for (i = 0; i < ARRAY_SIZE(rt5623_snd_controls); i++) {
		err = snd_ctl_add(codec->card,
				snd_soc_cnew(&rt5623_snd_controls[i],codec, NULL));
		if (err < 0)
			return err;
	}
	return 0;
}

/*
 * _DAPM_ Controls
 */

/* We have to create a fake left and right HP mixers because
 * the codec only has a single control that is shared by both channels.
 * This makes it impossible to determine the audio path using the current
 * register map, thus we add a new (virtual) register to help determine the
 * audio route within the device.
 */
static int mixer_event (struct snd_soc_dapm_widget *w, 
	struct snd_kcontrol *kcontrol, int event)
{
	u16 l=0, r=0, lineIn,mic1,mic2, auxin,pcm;
	struct snd_soc_device *socdev = rt5623_socdev;
	struct snd_soc_codec *codec = socdev->card->codec;
	
	l = rt5623_read(codec, HPL_MIXER);
	r = rt5623_read(codec, HPR_MIXER);
	lineIn = rt5623_read(codec, RT5623_LINE_IN_VOL);
	mic1 = rt5623_read(codec, RT5623_MIC_ROUTING_CTRL);
	mic2 = rt5623_read(codec, RT5623_MIC_ROUTING_CTRL);
	auxin = rt5623_read(codec,RT5623_AUXIN_VOL);
	pcm = rt5623_read(codec, RT5623_STEREO_DAC_VOL);

	if (event & SND_SOC_DAPM_PRE_REG)
		return 0;

	if (l & 0x1 || r & 0x1)
		rt5623_write(codec, RT5623_STEREO_DAC_VOL, pcm & 0x7fff);		
	else
		rt5623_write(codec, RT5623_STEREO_DAC_VOL, pcm | 0x8000);

	if (l & 0x2 || r & 0x2)
		rt5623_write(codec, RT5623_MIC_ROUTING_CTRL, mic2 & 0xf7ff);
	else
		rt5623_write(codec, RT5623_MIC_ROUTING_CTRL, mic2 | 0x0800);

	if (l & 0x4 || r & 0x4)
		rt5623_write(codec, RT5623_MIC_ROUTING_CTRL, mic1 & 0x7fff);
	else
		rt5623_write(codec, RT5623_MIC_ROUTING_CTRL, mic1 | 0x8000);

	if (l & 0x8 || r & 0x8)
		rt5623_write(codec, RT5623_AUXIN_VOL, auxin & 0x7fff);
	else
		rt5623_write(codec, RT5623_AUXIN_VOL, auxin | 0x8000);

	if (l & 0x10 || r & 0x10)
		rt5623_write(codec, RT5623_LINE_IN_VOL, lineIn & 0x7fff);
	else
		rt5623_write(codec, RT5623_LINE_IN_VOL, lineIn | 0x8000);

	return 0;
}

/* Left Headphone Mixers */
static const struct snd_kcontrol_new rt5623_hpl_mixer_controls[] = {
SOC_DAPM_SINGLE("LineIn Playback Switch", HPL_MIXER, 4, 1, 0),
SOC_DAPM_SINGLE("AUXIN Playback Switch", HPL_MIXER, 3, 1, 0),
SOC_DAPM_SINGLE("Mic1 Playback Switch", HPL_MIXER, 2, 1, 0),
SOC_DAPM_SINGLE("Mic2 Playback Switch", HPL_MIXER, 1, 1, 0),
SOC_DAPM_SINGLE("PCM Playback Switch", HPL_MIXER, 0, 1, 0),
SOC_DAPM_SINGLE("RecordL Playback Switch", RT5623_ADC_REC_GAIN, 15, 1,1),
};


/* Right Headphone Mixers */
static const struct snd_kcontrol_new rt5623_hpr_mixer_controls[] = {
SOC_DAPM_SINGLE("LineIn Playback Switch", HPR_MIXER, 5, 1, 0),
SOC_DAPM_SINGLE("AUXIN Playback Switch", HPR_MIXER, 4, 1, 0),
SOC_DAPM_SINGLE("Mic1 Playback Switch", HPR_MIXER, 3, 1, 0),
SOC_DAPM_SINGLE("Mic2 Playback Switch", HPR_MIXER, 2, 1, 0),
SOC_DAPM_SINGLE("PCM Playback Switch", HPR_MIXER, 1, 1, 0),
SOC_DAPM_SINGLE("RecordR Playback Switch", RT5623_ADC_REC_GAIN, 14, 1,1),
};

//Left Record Mixer
static const struct snd_kcontrol_new rt5623_captureL_mixer_controls[] = {
SOC_DAPM_SINGLE("Mic1 Capture Switch", RT5623_ADC_REC_MIXER, 14, 1, 1),
SOC_DAPM_SINGLE("Mic2 Capture Switch", RT5623_ADC_REC_MIXER, 13, 1, 1),
SOC_DAPM_SINGLE("LineInL Capture Switch",RT5623_ADC_REC_MIXER,12, 1, 1),
SOC_DAPM_SINGLE("AuxInL Capture Switch", RT5623_ADC_REC_MIXER, 11, 1, 1),
SOC_DAPM_SINGLE("HPMixerL Capture Switch", RT5623_ADC_REC_MIXER,10, 1, 1),
SOC_DAPM_SINGLE("SPKMixer Capture Switch",RT5623_ADC_REC_MIXER,9, 1, 1),
SOC_DAPM_SINGLE("MonoMixer Capture Switch",RT5623_ADC_REC_MIXER,8, 1, 1),
};

//Right Record Mixer
static const struct snd_kcontrol_new rt5623_captureR_mixer_controls[] = {
SOC_DAPM_SINGLE("Mic1 Capture Switch", RT5623_ADC_REC_MIXER, 6, 1, 1),
SOC_DAPM_SINGLE("Mic2 Capture Switch", RT5623_ADC_REC_MIXER, 5, 1, 1),
SOC_DAPM_SINGLE("LineInR Capture Switch",RT5623_ADC_REC_MIXER,4, 1, 1),
SOC_DAPM_SINGLE("AuxInLR Capture Switch", RT5623_ADC_REC_MIXER, 3, 1, 1),
SOC_DAPM_SINGLE("HPMixerR Capture Switch", RT5623_ADC_REC_MIXER,2, 1, 1),
SOC_DAPM_SINGLE("SPKMixer Capture Switch",RT5623_ADC_REC_MIXER,1, 1, 1),
SOC_DAPM_SINGLE("MonoMixer Capture Switch",RT5623_ADC_REC_MIXER,0, 1, 1),
};


/* Speaker Mixer */
static const struct snd_kcontrol_new rt5623_speaker_mixer_controls[] = {
SOC_DAPM_SINGLE("LineIn Playback Switch", RT5623_LINE_IN_VOL, 14, 1, 1),
SOC_DAPM_SINGLE("AuxIn Playback Switch", RT5623_AUXIN_VOL, 14, 1, 1),
SOC_DAPM_SINGLE("Mic1 Playback Switch", RT5623_MIC_ROUTING_CTRL, 14, 1, 1),
SOC_DAPM_SINGLE("Mic2 Playback Switch", RT5623_MIC_ROUTING_CTRL, 6, 1, 1),
SOC_DAPM_SINGLE("PCM Playback Switch", RT5623_STEREO_DAC_VOL, 14, 1, 1),
};


/* Mono Mixer */
static const struct snd_kcontrol_new rt5623_mono_mixer_controls[] = {
SOC_DAPM_SINGLE("LineIn Playback Switch", RT5623_LINE_IN_VOL, 13, 1, 1),
SOC_DAPM_SINGLE("AuxIn Playback Switch", RT5623_AUXIN_VOL, 13, 1, 1),
SOC_DAPM_SINGLE("Mic1 Playback Switch", RT5623_MIC_ROUTING_CTRL, 13, 1, 1),
SOC_DAPM_SINGLE("Mic2 Playback Switch", RT5623_MIC_ROUTING_CTRL, 5, 1, 1),
SOC_DAPM_SINGLE("PCM Playback Switch", RT5623_STEREO_DAC_VOL, 13, 1, 1),
SOC_DAPM_SINGLE("RecordL Playback Switch", RT5623_ADC_REC_GAIN, 13, 1,1),
SOC_DAPM_SINGLE("RecordR Playback Switch", RT5623_ADC_REC_GAIN, 12, 1,1),
};

/* auxout output mux */
static const struct snd_kcontrol_new rt5623_auxout_mux_controls =
SOC_DAPM_ENUM("Route", rt5623_enum[4]);


/* speaker output mux */
static const struct snd_kcontrol_new rt5623_spkout_mux_controls =
SOC_DAPM_ENUM("Route", rt5623_enum[1]);


/* headphone left output mux */
static const struct snd_kcontrol_new rt5623_hpl_out_mux_controls =
SOC_DAPM_ENUM("Route", rt5623_enum[2]);


/* headphone right output mux */
static const struct snd_kcontrol_new rt5623_hpr_out_mux_controls =
SOC_DAPM_ENUM("Route", rt5623_enum[3]);


static const struct snd_soc_dapm_widget rt5623_dapm_widgets[] = {	
SND_SOC_DAPM_MUX("Aux Out Mux", SND_SOC_NOPM, 0, 0,
	&rt5623_auxout_mux_controls),
SND_SOC_DAPM_MUX("Speaker Out Mux", SND_SOC_NOPM, 0, 0,
	&rt5623_spkout_mux_controls),
SND_SOC_DAPM_MUX("Left Headphone Out Mux", SND_SOC_NOPM, 0, 0,
	&rt5623_hpl_out_mux_controls),
SND_SOC_DAPM_MUX("Right Headphone Out Mux", SND_SOC_NOPM, 0, 0,
	&rt5623_hpr_out_mux_controls),
SND_SOC_DAPM_MIXER_E("Left HP Mixer",RT5623_PWR_MANAG_ADD2, 5, 0,
	&rt5623_hpl_mixer_controls[0], ARRAY_SIZE(rt5623_hpl_mixer_controls),
	mixer_event, SND_SOC_DAPM_POST_REG),
SND_SOC_DAPM_MIXER_E("Right HP Mixer",RT5623_PWR_MANAG_ADD2, 4, 0,
	&rt5623_hpr_mixer_controls[0], ARRAY_SIZE(rt5623_hpr_mixer_controls),
	mixer_event, SND_SOC_DAPM_POST_REG),
SND_SOC_DAPM_MIXER("Mono Mixer", RT5623_PWR_MANAG_ADD2, 2, 0,
	&rt5623_mono_mixer_controls[0], ARRAY_SIZE(rt5623_mono_mixer_controls)),
SND_SOC_DAPM_MIXER("Speaker Mixer", RT5623_PWR_MANAG_ADD2,3,0,
	&rt5623_speaker_mixer_controls[0],
	ARRAY_SIZE(rt5623_speaker_mixer_controls)),	
SND_SOC_DAPM_MIXER("Left Record Mixer", RT5623_PWR_MANAG_ADD2,1,0,
	&rt5623_captureL_mixer_controls[0],
	ARRAY_SIZE(rt5623_captureL_mixer_controls)),	
SND_SOC_DAPM_MIXER("Right Record Mixer", RT5623_PWR_MANAG_ADD2,0,0,
	&rt5623_captureR_mixer_controls[0],
	ARRAY_SIZE(rt5623_captureR_mixer_controls)),				
SND_SOC_DAPM_DAC("Left DAC", "Left HiFi Playback", RT5623_PWR_MANAG_ADD2,9, 0),
SND_SOC_DAPM_DAC("Right DAC", "Right HiFi Playback", RT5623_PWR_MANAG_ADD2, 8, 0),	
SND_SOC_DAPM_MIXER("IIS Mixer", SND_SOC_NOPM, 0, 0, NULL, 0),
SND_SOC_DAPM_MIXER("HP Mixer", SND_SOC_NOPM, 0, 0, NULL, 0),
SND_SOC_DAPM_MIXER("AuxIn Mixer", SND_SOC_NOPM, 0, 0, NULL, 0),
SND_SOC_DAPM_MIXER("Line Mixer", SND_SOC_NOPM, 0, 0, NULL, 0),
SND_SOC_DAPM_ADC("Left ADC", "Left HiFi Capture", RT5623_PWR_MANAG_ADD2, 7, 0),
SND_SOC_DAPM_ADC("Right ADC", "Right HiFi Capture", RT5623_PWR_MANAG_ADD2, 6, 0),
SND_SOC_DAPM_PGA("Left Headphone", RT5623_PWR_MANAG_ADD3, 10, 0, NULL, 0),
SND_SOC_DAPM_PGA("Right Headphone", RT5623_PWR_MANAG_ADD3, 9, 0, NULL, 0),
SND_SOC_DAPM_PGA("Speaker Out", RT5623_PWR_MANAG_ADD3, 12, 0, NULL, 0),
SND_SOC_DAPM_PGA("Left Aux Out", RT5623_PWR_MANAG_ADD3, 14, 0, NULL, 0),
SND_SOC_DAPM_PGA("Right Aux Out", RT5623_PWR_MANAG_ADD3, 13, 0, NULL, 0),
SND_SOC_DAPM_PGA("Left Line In", RT5623_PWR_MANAG_ADD3, 7, 0, NULL, 0),
SND_SOC_DAPM_PGA("Right Line In", RT5623_PWR_MANAG_ADD3, 6, 0, NULL, 0),
SND_SOC_DAPM_PGA("Left Aux In",RT5623_PWR_MANAG_ADD3, 5, 0, NULL, 0),
SND_SOC_DAPM_PGA("Right Aux In",RT5623_PWR_MANAG_ADD3, 4, 0, NULL, 0),
SND_SOC_DAPM_PGA("Mic 1 PGA", RT5623_PWR_MANAG_ADD3, 3, 0, NULL, 0),
SND_SOC_DAPM_PGA("Mic 2 PGA", RT5623_PWR_MANAG_ADD3, 2, 0, NULL, 0),
SND_SOC_DAPM_PGA("Mic 1 Pre Amp", RT5623_PWR_MANAG_ADD3, 1, 0, NULL, 0),
SND_SOC_DAPM_PGA("Mic 2 Pre Amp", RT5623_PWR_MANAG_ADD3, 0, 0, NULL, 0),
SND_SOC_DAPM_MICBIAS("Mic Bias1", RT5623_PWR_MANAG_ADD1, 11, 0),
SND_SOC_DAPM_OUTPUT("AUXOUT"),
SND_SOC_DAPM_OUTPUT("HPL"),
SND_SOC_DAPM_OUTPUT("HPR"),
SND_SOC_DAPM_OUTPUT("SPKOUT"),
SND_SOC_DAPM_INPUT("LINEL"),
SND_SOC_DAPM_INPUT("LINER"),
SND_SOC_DAPM_INPUT("AUXINL"),
SND_SOC_DAPM_INPUT("AUXINR"),
SND_SOC_DAPM_INPUT("MIC1"),
SND_SOC_DAPM_INPUT("MIC2"),
SND_SOC_DAPM_VMID("VMID"),
};


static const struct snd_soc_dapm_route intercon[] = {
	/* left HP mixer */
	{"Left HP Mixer", "LineIn Playback Switch", "LINEL"},
	{"Left HP Mixer", "AuxIn Playback Switch","AUXIN"},
	{"Left HP Mixer", "Mic1 Playback Switch","MIC1"},
	{"Left HP Mixer", "Mic2 Playback Switch","MIC2"},
	{"Left HP Mixer", "PCM Playback Switch","Left DAC"},
	{"Left HP Mixer", "RecordL Playback Switch","Left Record Mixer"},
	
	/* right HP mixer */
	{"Right HP Mixer", "LineIn Playback Switch", "LINER"},
	{"Right HP Mixer", "AuxIn Playback Switch","AUXIN"},
	{"Right HP Mixer", "Mic1 Playback Switch","MIC1"},
	{"Right HP Mixer", "Mic2 Playback Switch","MIC2"},
	{"Right HP Mixer", "PCM Playback Switch","Right DAC"},
	{"Right HP Mixer", "RecordR Playback Switch","Right Record Mixer"},
	
	/* virtual mixer - mixes left & right channels for spk and mono */
	{"IIS Mixer", NULL, "Left DAC"},
	{"IIS Mixer", NULL, "Right DAC"},
	{"Line Mixer", NULL, "Right Line In"},
	{"Line Mixer", NULL, "Left Line In"},
	{"HP Mixer", NULL, "Left HP Mixer"},
	{"HP Mixer", NULL, "Right HP Mixer"},
	{"AuxIn Mixer",NULL,"Left Aux In"},
	{"AuxIn Mixer",NULL,"Right Aux In"},
	{"AuxOut Mixer",NULL,"Left Aux Out"},
	{"AuxOut Mixer",NULL,"Right Aux Out"},
	
	/* speaker mixer */
	{"Speaker Mixer", "LineIn Playback Switch","Line Mixer"},
	{"Speaker Mixer", "AuxIn Playback Switch","AuxIn Mixer"},
	{"Speaker Mixer", "Mic1 Playback Switch","MIC1"},
	{"Speaker Mixer", "Mic2 Playback Switch","MIC2"},
	{"Speaker Mixer", "PCM Playback Switch","IIS Mixer"},

	/* mono mixer */
	{"Mono Mixer", "LineIn Playback Switch","Line Mixer"},
	{"Mono Mixer", "AuxIn Playback Switch","AuxIn Mixer"},
	{"Mono Mixer", "Mic1 Playback Switch","MIC1"},
	{"Mono Mixer", "Mic2 Playback Switch","MIC2"},
	{"Mono Mixer", "PCM Playback Switch","IIS Mixer"},
	{"Mono Mixer", "RecordL Playback Switch","Left Record Mixer"},
	{"Mono Mixer", "RecordR Playback Switch","Right Record Mixer"},
	
	/*Left record mixer */
	{"Left Record Mixer", "Mic1 Capture Switch","Mic 1 Pre Amp"},
	{"Left Record Mixer", "Mic2 Capture Switch","Mic 2 Pre Amp"},
	{"Left Record Mixer", "LineInL Capture Switch","LINEL"},
	{"Left Record Mixer", "Phone Capture Switch","PHONEIN"},
	{"Left Record Mixer", "HPMixerL Capture Switch","Left HP Mixer"},
	{"Left Record Mixer", "SPKMixer Capture Switch","Speaker Mixer"},
	{"Left Record Mixer", "MonoMixer Capture Switch","Mono Mixer"},
	
	/*Right record mixer */
	{"Right Record Mixer", "Mic1 Capture Switch","Mic 1 Pre Amp"},
	{"Right Record Mixer", "Mic2 Capture Switch","Mic 2 Pre Amp"},
	{"Right Record Mixer", "LineInR Capture Switch","LINER"},
	{"Right Record Mixer", "Phone Capture Switch","PHONEIN"},
	{"Right Record Mixer", "HPMixerR Capture Switch","Right HP Mixer"},
	{"Right Record Mixer", "SPKMixer Capture Switch","Speaker Mixer"},
	{"Right Record Mixer", "MonoMixer Capture Switch","Mono Mixer"},	

	/* headphone left mux */
	{"Left Headphone Out Mux", "HP Left mixer", "Left HP Mixer"},

	/* headphone right mux */
	{"Right Headphone Out Mux", "HP Right mixer", "Right HP Mixer"},

	/* speaker out mux */
	{"Speaker Out Mux", "HP mixer", "HP Mixer"},
	{"Speaker Out Mux", "Speaker Mixer", "Speaker Mixer"},
	{"Speaker Out Mux", "Mono Mixer", "Mono Mixer"},

	/* Aux Out mux */
	{"Aux Out Mux", "HP mixer", "HP Mixer"},
	{"Aux Out Mux", "Speaker Mixer", "Speaker Mixer"},
	{"Aux Out Mux", "Mono Mixer", "Mono Mixer"},
	
	/* output pga */
	{"HPL", NULL, "Left Headphone"},
	{"Left Headphone", NULL, "Left Headphone Out Mux"},
	{"HPR", NULL, "Right Headphone"},
	{"Right Headphone", NULL, "Right Headphone Out Mux"},
	{"SPKOUT", NULL, "Speaker Out"},
	{"Speaker Out", NULL, "Speaker Out Mux"},
	{"AUXOUT", NULL, "AuxOut Mixer"},
	{"Left Aux Out", NULL, "Aux Out Mux"},
	{"Right Aux Out", NULL, "Aux Out Mux"},

	/* input pga */
	{"Left Line In", NULL, "LINEL"},
	{"Right Line In", NULL, "LINER"},
	{"Left Aux In", NULL, "AUXINL"},
	{"Right Aux In", NULL, "AUXINR"},
	{"Mic 1 Pre Amp", NULL, "MIC1"},
	{"Mic 2 Pre Amp", NULL, "MIC2"},	
	{"Mic 1 PGA", NULL, "Mic 1 Pre Amp"},
	{"Mic 2 PGA", NULL, "Mic 2 Pre Amp"},

	/* left ADC */
	{"Left ADC", NULL, "Left Record Mixer"},

	/* right ADC */
	{"Right ADC", NULL, "Right Record Mixer"},
	
	/* terminator */
	{NULL, NULL, NULL},	
};


static int rt5623_add_widgets(struct snd_soc_codec *codec)
{
	int i;

//	snd_soc_dapm_new_controls(codec, rt5623_dapm_widgets, ARRAY_SIZE(rt5623_dapm_widgets));

	/* set up audio path interconnects */
//	snd_soc_dapm_add_routes(codec, intercon, ARRAY_SIZE(intercon));

	snd_soc_dapm_new_widgets(codec);
	return 0;
}


#if 0
/* PLL divisors */
struct _pll_div {
	u32 pll_in;
	u32 pll_out;
	u16 regvalue;
};

static const struct _pll_div codec_master_pll_div[] = {
	
	{  2048000,  8192000,	0x0ea0},		
	{  3686400,  8192000,	0x4e27},	
	{ 12000000,  8192000,	0x456b},   
	{ 13000000,  8192000,	0x495f},
	{ 13100000,	 8192000,	0x0320},	
	{  2048000,  11289600,	0xf637},
	{  3686400,  11289600,	0x2f22},	
	{ 12000000,  11289600,	0x3e2f},   
	{ 13000000,  11289600,	0x4d5b},
	{ 13100000,	 11289600,	0x363b},	
	{  2048000,  16384000,	0x1ea0},
	{  3686400,  16384000,	0x9e27},	
	{ 12000000,  16384000,	0x452b},   
	{ 13000000,  16384000,	0x542f},
	{ 13100000,	 16384000,	0x03a0},	
	{  2048000,  16934400,	0xe625},
	{  3686400,  16934400,	0x9126},	
	{ 12000000,  16934400,	0x4d2c},   
	{ 13000000,  16934400,	0x742f},
	{ 13100000,	 16934400,	0x3c27},			
	{  2048000,  22579200,	0x2aa0},
	{  3686400,  22579200,	0x2f20},	
	{ 12000000,  22579200,	0x7e2f},   
	{ 13000000,  22579200,	0x742f},
	{ 13100000,	 22579200,	0x3c27},		
	{  2048000,  24576000,	0x2ea0},
	{  3686400,  24576000,	0xee27},	
	{ 12000000,  24576000,	0x2915},   
	{ 13000000,  24576000,	0x772e},
	{ 13100000,	 24576000,	0x0d20},	
};

static const struct _pll_div codec_slave_pll_div[] = {
	
	{  1024000,  16384000,  0x3ea0},	
	{  1411200,  22579200,	0x3ea0},
	{  1536000,	 24576000,	0x3ea0},	
	{  2048000,  16384000,  0x1ea0},	
	{  2822400,  22579200,	0x1ea0},
	{  3072000,	 24576000,	0x1ea0},
			
};


static int rt5623_set_dai_pll(struct snd_soc_dai *codec_dai,
		int pll_id, unsigned int freq_in, unsigned int freq_out)
{
	int i;
	struct snd_soc_codec *codec = codec_dai->codec;

	if (pll_id < RT5623_PLL_FR_MCLK || pll_id > RT5623_PLL_FR_BCK)
		return -ENODEV;

	rt5623_write_mask(codec,RT5623_PWR_MANAG_ADD2, 0x0000,0x1000);	//disable PLL power	
	
	if (!freq_in || !freq_out) {

		return 0;
	}		

	if (pll_id == RT5623_PLL_FR_MCLK) {	//codec is master mode
	
		for (i = 0; i < ARRAY_SIZE(codec_master_pll_div); i++) {
			
			if (codec_master_pll_div[i].pll_in == freq_in && codec_master_pll_div[i].pll_out == freq_out)
			 {
			 	rt5623_write(codec,RT5623_GLOBAL_CLK_CTRL_REG,0x0000);//PLL source from MCLK
			 	
			 	rt5623_write(codec,RT5623_PLL_CTRL,codec_master_pll_div[i].regvalue);//set PLL parameter
			 	
				rt5623_write_mask(codec,RT5623_PWR_MANAG_ADD2, 0x1000,0x1000);	//enable PLL power	
				
				rt5623_write(codec,RT5623_GLOBAL_CLK_CTRL_REG,0x8000);//Codec sys-clock from PLL 	 	
			 	
				break;
			}
		}
	
	} else {		//codec is slave mode 

		for (i = 0; i < ARRAY_SIZE(codec_slave_pll_div); i++) {
			
			if (codec_slave_pll_div[i].pll_in == freq_in && codec_slave_pll_div[i].pll_out == freq_out)
		 	{
		 		rt5623_write(codec,RT5623_GLOBAL_CLK_CTRL_REG,0x4000);//PLL source from Bitclk
		 		
			 	rt5623_write(codec,RT5623_PLL_CTRL,codec_master_pll_div[i].regvalue);//set PLL parameter
			 	
				rt5623_write_mask(codec,RT5623_PWR_MANAG_ADD2, 0x1000,0x1000);	//enable PLL power
				
				rt5623_write(codec,RT5623_GLOBAL_CLK_CTRL_REG,0xc000);//Codec sys-clock from PLL 
				break;
			}
		}
	}	
	
	return 0;
}

struct _coeff_div {
	u32 mclk;
	u32 rate;
	u16 fs;
	u16 regvalue;
};


/* codec hifi mclk (after PLL) clock divider coefficients */
static const struct _coeff_div coeff_div[] = {
	/* 8k */
	{ 8192000,  8000, 256*4, 0x2a69},
	{12288000,  8000, 384*4, 0x2c6b},

	/* 11.025k */
	{11289600, 11025, 256*4, 0x2a69},
	{16934400, 11025, 384*4, 0x2c6b},

	/* 16k */
	{16384000, 16000, 256*4, 0x2a69},
	{24576000, 16000, 384*4, 0x2c6b},

	/* 22.05k */
	{11289600, 22050, 256*2, 0x1a69},
	{16934400, 22050, 384*2, 0x1c6b},

	/* 32k */
	{16384000, 32000, 256*2, 0x1a69},
	{24576000, 32000, 384*2, 0x1c6b},

	/* 44.1k */
	{22579200, 44100, 256*2, 0x1a69},

	/* 48k */
	{24576000, 48000, 256*2, 0x1a69},

};

static int get_coeff(int mclk, int rate)
{
	int i;
	
//	printk("get_coeff mclk=%d,rate=%d\n",mclk,rate);

	for (i = 0; i < ARRAY_SIZE(coeff_div); i++) {
		if (coeff_div[i].rate == rate && coeff_div[i].mclk == mclk)
			return i;
	}
	return -EINVAL;
}
#endif

/*
 * Clock after PLL and dividers
 */
static int rt5623_set_dai_sysclk(struct snd_soc_dai *codec_dai,
		int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct rt5623_priv *rt5623 = codec->private_data;

	switch (freq) {
	case  8192000:	
	case 11289600:
	case 12288000:
	case 16384000:
	case 16934400:
	case 18432000:
	case 22579200:	
	case 24576000:
		rt5623->sysclk = freq;
		return 0;
	}
	return -EINVAL;
}


static int rt5623_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	u16 iface = 0;

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		iface = 0x0000;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		iface = 0x8000;
		break;
	default:
		return -EINVAL;
	}

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		iface |= 0x0000;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		iface |= 0x0001;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		iface |= 0x0002;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		iface |= 0x0003;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		iface |= 0x4003;
		break;
	default:
		return -EINVAL;
	}

	/* clock inversion */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_IB_IF:
		iface |= 0x0080;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		iface |= 0x0080;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		break;
	default:
		return -EINVAL;
	}

	rt5623_write_mask(codec,RT5623_AUDIO_INTERFACE, iface,0x4803);
	return 0;
}



static int rt5623_pcm_hw_params(struct snd_pcm_substream *substream,struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->card->codec;
//	struct rt5623_priv *rt5623 = codec->private_data;
	u16 iface=rt5623_read(codec,RT5623_AUDIO_INTERFACE)&0xfff3;
//	int coeff = get_coeff(rt5623->sysclk, params_rate(params));

//	printk("rt5623_pcm_hw_params\n");

	/* bit size */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
//		printk("S16_LE\n");
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
//		printk("S20_3LE\n");
		iface |= 0x0004;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
//		printk("S24_LE\n");
		iface |= 0x0008;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
//		printk("S32_LE\n");
		iface |= 0x000c;
		break;
	}

	/* set iface & srate */
	rt5623_write(codec, RT5623_AUDIO_INTERFACE, iface);
//	if (coeff >= 0)
//		rt5623_write(codec, RT5623_STEREO_AD_DA_CLK_CTRL,coeff_div[coeff].regvalue);

	return 0;
}

static int rt5623_pcm_hw_trigger(struct snd_pcm_substream *substream,int cmd,
				 struct snd_soc_dai *dai)
{
//	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->card->codec;
	int ret = 0;
	struct rt5623_setup_data *setup = socdev->codec_data;

//	printk("%s\n",__func__);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		if(substream->stream==SNDRV_PCM_STREAM_PLAYBACK)
		{
//			rt5623_ChangeCodecPowerStatus(codec,POWER_STATE_D1_PLAYBACK);
//			rt5623_AudioOutEnable(codec,RT_WAVOUT_SPK,0);
//			rt5623_AudioOutEnable(codec,RT_WAVOUT_HP,0);
		}
		else
		{
			if(setup->mic2_input)
			{//mic2 input
				Enable_ADC_Input_Source(codec,RT_WAVIN_L_LINE_IN|RT_WAVIN_R_LINE_IN,0);
				Enable_ADC_Input_Source(codec,RT_WAVIN_L_MIC2|RT_WAVIN_R_MIC2,1);
			}
			else
			{ //line in
				Enable_ADC_Input_Source(codec,RT_WAVIN_L_MIC2|RT_WAVIN_R_MIC2,0);
				Enable_ADC_Input_Source(codec,RT_WAVIN_L_LINE_IN|RT_WAVIN_R_LINE_IN,1);
			}
		}	
		break;

	case SNDRV_PCM_TRIGGER_STOP:
//		printk("%s : stop\n",__func__);		
		break;

	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:

		break;

	case SNDRV_PCM_TRIGGER_RESUME:

		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:

		break;

	default:
		ret = -EINVAL;
	}

	return ret;
}



static int rt5623_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	u16 mute_reg = rt5623_read(codec, RT5623_MISC_CTRL) & 0xfff3;


	if (mute)
	{
//		printk("rt5623 mute\n");
		rt5623_write(codec, RT5623_MISC_CTRL, mute_reg | 0xc);
	}	
	else
	{
//		printk("rt5623 unmute\n");
		rt5623_write(codec, RT5623_MISC_CTRL, mute_reg);

	}	
	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////
// RT5623 DPAM for KW / Dove Avenger
/////////////////////////////////////////////////////////////////////////////////////////////

#define RT5623_ADD1_POWER_D0 \
	PWR_MAIN_I2S_EN | PWR_ZC_DET_PD_EN | PWR_MIC1_BIAS_EN | PWR_SHORT_CURR_DET_EN | \
	PWR_SOFTGEN_EN | PWR_DEPOP_BUF_HP | PWR_HP_OUT_AMP | PWR_HP_OUT_ENH_AMP

#define RT5623_ADD1_POWER_D0_PLAYBACK \
	PWR_MAIN_I2S_EN | PWR_ZC_DET_PD_EN | \
	PWR_SOFTGEN_EN | PWR_DEPOP_BUF_HP | PWR_HP_OUT_AMP | PWR_HP_OUT_ENH_AMP

#define RT5623_ADD1_POWER_D0_CAPTURE \
	PWR_MAIN_I2S_EN | PWR_ZC_DET_PD_EN | PWR_MIC1_BIAS_EN | PWR_SHORT_CURR_DET_EN | \
	PWR_SOFTGEN_EN

#define RT5623_ADD2_POWER_D0 \
	PWR_LINEOUT | PWR_VREF | PWR_DAC_REF_CIR | PWR_L_DAC_CLK | PWR_R_DAC_CLK | \
	PWR_L_ADC_CLK_GAIN | PWR_R_ADC_CLK_GAIN | PWR_L_HP_MIXER | PWR_R_HP_MIXER | \
	PWR_L_ADC_REC_MIXER | PWR_R_ADC_REC_MIXER

#define RT5623_ADD2_POWER_D0_PLAYBACK \
	PWR_LINEOUT | PWR_VREF | PWR_DAC_REF_CIR | PWR_L_DAC_CLK | PWR_R_DAC_CLK | \
	PWR_L_HP_MIXER | PWR_R_HP_MIXER

#define RT5623_ADD2_POWER_D0_CAPTURE \
	PWR_VREF | \
	PWR_L_ADC_CLK_GAIN | PWR_R_ADC_CLK_GAIN | \
	PWR_L_ADC_REC_MIXER | PWR_R_ADC_REC_MIXER

#define RT5623_ADD3_POWER_D0 \
	PWR_MAIN_BIAS | PWR_SPK_OUT | PWR_HP_L_OUT_VOL | PWR_HP_R_OUT_VOL | \
	PWR_LINEIN_L_VOL | PWR_LINEIN_R_VOL | PWR_MIC2_BOOST_MIXER

#define RT5623_ADD3_POWER_D0_PLAYBACK \
	PWR_MAIN_BIAS | PWR_SPK_OUT | PWR_HP_L_OUT_VOL | PWR_HP_R_OUT_VOL
#define RT5623_ADD3_POWER_D0_CAPTURE \
	PWR_MAIN_BIAS | PWR_LINEIN_L_VOL | PWR_LINEIN_R_VOL | PWR_MIC2_BOOST_MIXER

#define RT5623_ADD1_POWER_D3HOT	        PWR_MIC1_BIAS_EN
#define RT5623_ADD2_POWER_D3HOT 	PWR_VREF
#define RT5623_ADD3_POWER_D3HOT 	PWR_MAIN_BIAS


#define RT5623_ADD1_POWER_D3COLD	0
#define RT5623_ADD2_POWER_D3COLD	0
#define RT5623_ADD3_POWER_D3COLD	0



static void enable_power_depop(struct snd_soc_codec *codec)
{

	//Power On Main Bias
//	rt5623_write_mask(codec, RT5623_PWR_MANAG_ADD3, PWR_MAIN_BIAS, PWR_MAIN_BIAS);

	//HP L/R MUTE
	rt5623_write_mask(codec,RT5623_HP_OUT_VOL,RT_L_MUTE|RT_R_MUTE,RT_L_MUTE|RT_R_MUTE);
	//SPK L/R MUTE
	rt5623_write_mask(codec,RT5623_SPK_OUT_VOL,RT_L_MUTE|RT_R_MUTE,RT_L_MUTE|RT_R_MUTE);

	//Power On Softgen
	rt5623_write_mask(codec, RT5623_PWR_MANAG_ADD1, PWR_SOFTGEN_EN, PWR_SOFTGEN_EN);

	//Power On Vref
//	rt5623_write_mask(codec, RT5623_PWR_MANAG_ADD2, PWR_VREF, PWR_VREF);

	//Power On HP L/R vol
	rt5623_write(codec, RT5623_PWR_MANAG_ADD3, RT5623_ADD3_POWER_D0);

	//enable HP Depop2
	rt5623_write_mask(codec,RT5623_MISC_CTRL,HP_DEPOP_MODE2_EN,HP_DEPOP_MODE2_EN);

	msleep(500);

//	rt5623_write_mask(codec, RT5623_PWR_MANAG_ADD1, PWR_HP_OUT_AMP | PWR_HP_OUT_ENH_AMP , PWR_HP_OUT_AMP | PWR_HP_OUT_ENH_AMP);


	rt5623_write(codec, RT5623_PWR_MANAG_ADD2, RT5623_ADD2_POWER_D0);
	rt5623_write(codec, RT5623_PWR_MANAG_ADD1, RT5623_ADD1_POWER_D0);

	//disable HP Depop2
	rt5623_write_mask(codec,RT5623_MISC_CTRL,0,HP_DEPOP_MODE2_EN);

	//HP L/R UNMUTE
	rt5623_write_mask(codec,RT5623_HP_OUT_VOL,0,RT_L_MUTE|RT_R_MUTE);
	//SPEAKER UNMUTE
	rt5623_write_mask(codec,RT5623_SPK_OUT_VOL,0,RT_L_MUTE|RT_R_MUTE);

}



static int rt5623_set_bias_level(struct snd_soc_codec *codec,
				      enum snd_soc_bias_level level)
{
	switch (level) {
	case SND_SOC_BIAS_ON:
		/* vref/mid, osc on, dac unmute */
		enable_power_depop(codec);
		break;
	case SND_SOC_BIAS_PREPARE:
		break;
	case SND_SOC_BIAS_STANDBY:
		/* everything off except vref/vmid, */
		rt5623_write(codec, RT5623_PWR_MANAG_ADD2, RT5623_ADD2_POWER_D3HOT);
		rt5623_write(codec, RT5623_PWR_MANAG_ADD3, RT5623_ADD3_POWER_D3HOT);
		rt5623_write(codec, RT5623_PWR_MANAG_ADD1, RT5623_ADD1_POWER_D3HOT);
		break;
	case SND_SOC_BIAS_OFF:
		/* everything off, dac mute, inactive */
		rt5623_write(codec, RT5623_PWR_MANAG_ADD2, RT5623_ADD2_POWER_D3COLD);
		rt5623_write(codec, RT5623_PWR_MANAG_ADD3, RT5623_ADD3_POWER_D3COLD);
		rt5623_write(codec, RT5623_PWR_MANAG_ADD1, RT5623_ADD1_POWER_D3COLD);
		break;
	}
	codec->bias_level = level;
	return 0;
}


#define RT5623_RATES SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000


#define RT5623_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)


static struct snd_soc_dai_ops rt5623_dai_ops = {
		.hw_params = rt5623_pcm_hw_params,
		.trigger = rt5623_pcm_hw_trigger,
		.digital_mute = rt5623_mute,
		.set_fmt = rt5623_set_dai_fmt,
		.set_sysclk = rt5623_set_dai_sysclk,
//		.set_pll = rt5623_set_dai_pll,
};

struct snd_soc_dai rt5623_dai = {
	.name = "RT5623",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rate_min =	44100,
		.rate_max =	96000,
		.rates = RT5623_RATES,
		.formats = RT5623_FORMATS,},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rate_min =	44100,
		.rate_max =	96000,
		.rates = RT5623_RATES,
		.formats = RT5623_FORMATS,},

	.ops = &rt5623_dai_ops,
};
EXPORT_SYMBOL_GPL(rt5623_dai);


static void rt5623_work(struct work_struct *work)
{
	struct snd_soc_codec *codec =
		container_of(work, struct snd_soc_codec, delayed_work.work);
	rt5623_set_bias_level(codec, codec->bias_level);
}

static int rt5623_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;

	/* we only need to suspend if we are a valid card */
	if(!codec->card)
		return 0;
		
	rt5623_set_bias_level(codec, SND_SOC_BIAS_OFF);

	return 0;
}

static int rt5623_resume(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;
	int i;
	u8 data[3];
	u16 *cache = codec->reg_cache;

	/* we only need to resume if we are a valid card */
	if(!codec->card)
		return 0;

	/* Sync reg_cache with the hardware */	

	for (i = 0; i < ARRAY_SIZE(rt5623_reg); i++) {
		if (i == RT5623_RESET)
			continue;
		data[0] =i*2;	
		data[1] = (0xFF00 & cache[i]) >> 8;
		data[2] = 0x00FF & cache[i];	
		codec->hw_write(codec->control_data, data, 3);
	}	

	rt5623_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	/* charge rt5623 caps */
	if(codec->suspend_bias_level == SND_SOC_BIAS_ON) {
		rt5623_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
		codec->bias_level = SND_SOC_BIAS_ON;
		schedule_delayed_work(&codec->delayed_work,
			msecs_to_jiffies(caps_charge));

	}

	return 0;
}

/*
 * initialise the RT5623 driver
 * register the mixer and dsp interfaces with the kernel
 */
static int rt5623_init(struct snd_soc_device *socdev)
{
	struct snd_soc_codec *codec = socdev->card->codec;
	int  ret = 0,i=0;

	codec->name = "RT5623";
	codec->owner = THIS_MODULE;
	codec->read = rt5623_read;
	codec->write = rt5623_write;
	codec->set_bias_level = rt5623_set_bias_level;
	codec->dai = &rt5623_dai;
	codec->num_dai = 1;
	codec->reg_cache_size = sizeof(rt5623_reg);
	codec->reg_cache = kmemdup(rt5623_reg, sizeof(rt5623_reg), GFP_KERNEL);

	if (codec->reg_cache == NULL)
		return -ENOMEM;
		
	rt5623_reset(codec);

	rt5623_dai.dev = codec->dev;

	/* register pcms */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		printk(KERN_ERR "rt5623: failed to create pcms\n");
		goto pcm_err;
	}

	rt5623_write(codec, RT5623_PWR_MANAG_ADD3, 0x8000);//enable Main bias
	rt5623_write(codec, RT5623_PWR_MANAG_ADD2, 0x2000);//enable Vref

	/* power on device */
	rt5623_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	schedule_delayed_work(&codec->delayed_work,
		msecs_to_jiffies(caps_charge));


	//initize codec register
	for(i=0;i<SET_CODEC_REG_INIT_NUM;i++)
	{
		rt5623_write(codec,Set_Codec_Reg_Init[i][0],Set_Codec_Reg_Init[i][1]);
		
	}
	
	rt5623_add_controls(codec);
	rt5623_add_widgets(codec);
	
	ret = snd_soc_init_card(socdev);

	if (ret < 0) {
		printk(KERN_ERR "rt5623: failed to register card\n");
		goto card_err;
	}

	return ret;

card_err:
	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);
pcm_err:
	kfree(codec->reg_cache);
	return ret;
}


/* If the i2c layer weren't so broken, we could pass this kind of data
   around */


#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)

/*
 * RT5623 2 wire address is determined by A1 pin
 * state during powerup.
 *    low  = 0x1a
 *    high = 0x1b
 */
static const struct i2c_device_id rt5623_i2c_table[] = {
	{"rt5623", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, rt5623_i2c_table);


static struct i2c_driver rt5623_i2c_driver;

static int rt5623_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct snd_soc_device *socdev = rt5623_socdev;
	struct rt5623_setup_data *setup = socdev->codec_data;
	struct snd_soc_codec *codec = socdev->card->codec;	
	struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
	int ret;

	if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_EMUL))
		return -EIO;

	i2c_set_clientdata(client, codec);
	codec->control_data = client;
	codec->dev = &client->dev;

	ret = rt5623_init(socdev);			
		
	if (ret < 0) {
		err("failed to initialise RT5623\n");
		goto err;
	}

	return ret;

err:
	kfree(codec->private_data);
	kfree(codec);
	return ret;
}

static int rt5623_i2c_remove(struct i2c_client *client)
{
	struct snd_soc_codec *codec = i2c_get_clientdata(client);
	kfree(codec->reg_cache);
	kfree(client);
	return 0;
}


/*  i2c codec control layer */
static struct i2c_driver rt5623_i2c_driver = {
	.driver = {
		.name = "rt5623",
		.owner = THIS_MODULE,
	},
	.probe = rt5623_i2c_probe,
	.remove =  rt5623_i2c_remove,
	.id_table = rt5623_i2c_table,
};


#endif

static int rt5623_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct rt5623_setup_data *setup;
	struct snd_soc_codec *codec;
	struct rt5623_priv *rt5623;
	int ret = 0;

	info("RT5623 Audio Codec %s",RT5623_VERSION);

	setup = socdev->codec_data;
	codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (codec == NULL)
		return -ENOMEM;

	rt5623 = kzalloc(sizeof(struct rt5623_priv), GFP_KERNEL);
	if (rt5623 == NULL) {
		kfree(codec);
		return -ENOMEM;
	}

	codec->private_data = rt5623;
	socdev->card->codec = codec;
	mutex_init(&codec->mutex);
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);
	rt5623_socdev = socdev;
	INIT_DELAYED_WORK(&codec->delayed_work, rt5623_work);

#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)

	codec->hw_write = (hw_write_t)i2c_master_send;

	ret = i2c_add_driver(&rt5623_i2c_driver);
	if (ret != 0)
		printk(KERN_ERR "can't add i2c rt5623 driver");
#else
		/* Add other interfaces here */
#endif
	return ret;
}

/*
 * This function forces any delayed work to be queued and run.
 */
static int run_delayed_work(struct delayed_work *dwork)
{
	int ret;

	/* cancel any work waiting to be queued. */
	ret = cancel_delayed_work(dwork);

	/* if there was any work waiting then we run it now and
	 * wait for it's completion */
	if (ret) {
		schedule_delayed_work(dwork, 0);
		flush_scheduled_work();
	}
	return ret;
}

/* power down chip */
static int rt5623_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;

	if (codec->control_data)
		rt5623_set_bias_level(codec, SND_SOC_BIAS_OFF);
	run_delayed_work(&codec->delayed_work);
	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);
#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)
	i2c_del_driver(&rt5623_i2c_driver);
#endif
	kfree(codec->private_data);
	kfree(codec);

	return 0;
}

struct snd_soc_codec_device soc_codec_dev_rt5623 = {
	.probe = 	rt5623_probe,
	.remove = 	rt5623_remove,
	.suspend = 	rt5623_suspend,
	.resume =	rt5623_resume,
};

EXPORT_SYMBOL_GPL(soc_codec_dev_rt5623);

static int __init rt5623_modinit(void)
{
	return snd_soc_register_dai(&rt5623_dai);
}
module_init(rt5623_modinit);

static void __exit rt5623_modexit(void)
{
	snd_soc_unregister_dai(&rt5623_dai);
}
module_exit(rt5623_modexit);

MODULE_DESCRIPTION("ASoC RT5623 driver");
MODULE_AUTHOR("flove , Ethan");
MODULE_LICENSE("GPL");

