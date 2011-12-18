/*
 *
 *	Marvell Orion Alsa Sound driver
 *
 *	Author: Maen Suleiman
 *	Copyright (C) 2008 Marvell Ltd.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
#include <sound/driver.h>
#endif
#include <sound/core.h>
#include <sound/initval.h>
#include <sound/control.h>
#include <sound/pcm.h>
#include <sound/asoundef.h>
#include <sound/asound.h>

#include "twsi/mvTwsi.h"
#include "audio/dac/mvCLAudioCodec.h"
#include "boardEnv/mvBoardEnvLib.h"



/*
 * Initialize the audio decoder.
 */
int
cs42l51_init(MV_U8 port)
{
    MV_AUDIO_CODEC_DEV codec_params;
    unsigned char reg_data;
    
    codec_params.chanNum = mvBoardA2DTwsiChanNumGet(port);
    codec_params.ADCMode = MV_I2S_MODE;
    codec_params.DACDigitalIFFormat = MV_I2S_UP_TO_24_BIT;
    codec_params.twsiSlave.moreThen256 = MV_FALSE;
    codec_params.twsiSlave.validOffset = MV_TRUE;
    codec_params.twsiSlave.slaveAddr.address = mvBoardA2DTwsiAddrGet(port);
    codec_params.twsiSlave.slaveAddr.type = mvBoardA2DTwsiAddrTypeGet(port);
    if(mvCLAudioCodecInit(&codec_params) == MV_FALSE)
    {
        printk("Error - Cannot initialize audio decoder.at address =0x%x",
				codec_params.twsiSlave.slaveAddr.address);
        return -1;
    }

#ifdef CONFIG_MACH_DOVE_RD_AVNG
    /* Default Gain */
    mvCLAudioCodecRegSet(&codec_params,0x8,
                         mvCLAudioCodecRegGet(&codec_params,0x8)|0xE0);
#endif

    /* Use the signal processor.               */
    mvCLAudioCodecRegSet(&codec_params,0x9,0x40);
    
    /* Unmute PCM-A & PCM-B and set default      */
    mvCLAudioCodecRegSet(&codec_params,0x10,0x10);
    mvCLAudioCodecRegSet(&codec_params,0x11,0x10);

    /* default for AOUTx*/
    mvCLAudioCodecRegSet(&codec_params,0x16,0x05);
    mvCLAudioCodecRegSet(&codec_params,0x17,0x05);

    /* swap channels */
    mvCLAudioCodecRegSet(&codec_params,0x18,0xff);
    if (0) {
	    int i;
	    for (i=1; i<= 0x21 ; i++) {
		    reg_data = mvCLAudioCodecRegGet(&codec_params,i);
		    printk("CLS reg=0x%02x val=0x%02x\n",i,reg_data);
	    }
	    
    }
    
    return 0;
}

#define MVAUD_NUM_VOLUME_STEPS  (40)
static MV_U8 auddec_volume_mapping[MVAUD_NUM_VOLUME_STEPS] =
{
	0x19,	0xB2,	0xB7,	0xBD,	0xC3,	0xC9,	0xCF,	0xD5,
	0xD8,	0xE1,	0xE7,	0xED,	0xF3,	0xF9,	0xFF,	0x00,	
	0x01,	0x02,	0x03,	0x04,	0x05,	0x06,	0x07,	0x08,	
	0x09,	0x0A,	0x0B,	0x0C,	0x0D,	0x0E,	0x0F,	0x10,	
	0x11,	0x12,	0x13,	0x14,	0x15,	0x16,	0x17,	0x18
};


/*
 * Get the audio decoder volume for both channels.
 * 0 is lowest volume, MVAUD_NUM_VOLUME_STEPS-1 is the highest volume.
 */

void
cs42l51_vol_get(MV_U8 port, MV_U8 *vol_list)
{
    MV_AUDIO_CODEC_DEV  codec_params;
    MV_U8   reg_data;
    MV_U32  i;
    MV_U32  vol_idx;

    codec_params.chanNum = mvBoardA2DTwsiChanNumGet(port);
    codec_params.ADCMode = MV_I2S_MODE;
    codec_params.DACDigitalIFFormat = MV_I2S_UP_TO_24_BIT;
    codec_params.twsiSlave.moreThen256 = MV_FALSE;
    codec_params.twsiSlave.validOffset = MV_TRUE;
	codec_params.twsiSlave.slaveAddr.address = mvBoardA2DTwsiAddrGet(port);
	codec_params.twsiSlave.slaveAddr.type = mvBoardA2DTwsiAddrTypeGet(port);


    for(vol_idx = 0; vol_idx < 2; vol_idx++)
    {
        reg_data = mvCLAudioCodecRegGet(&codec_params,0x16 + vol_idx);

       /*printk("\tVolume was get: 0x%x.\n",
                                reg_data);*/

        /* Look for the index that mapps to this dB value.  */
        for(i = 0; i < MVAUD_NUM_VOLUME_STEPS; i++)
        {
		if (reg_data == auddec_volume_mapping[i])
			break;
		if (( auddec_volume_mapping[i] > auddec_volume_mapping[MVAUD_NUM_VOLUME_STEPS-1]) 
					&& (reg_data > auddec_volume_mapping[i]) 
					&& (reg_data < auddec_volume_mapping[i+1]))
			break;
		 
        }

        vol_list[vol_idx] = i;
       /*printk("\tvol_list[%d] = %d.\n",vol_idx,
                                vol_list[vol_idx]);*/

    }

    return;
}


/*
 * Set the audio decoder volume for both channels.
 * 0 is lowest volume, MVAUD_NUM_VOLUME_STEPS-1 is the highest volume.
 */
void
cs42l51_vol_set(MV_U8 port, MV_U8 *vol_list)
{
    MV_AUDIO_CODEC_DEV  codec_params;
    MV_U32  vol_idx;

    codec_params.chanNum = mvBoardA2DTwsiChanNumGet(port);
    codec_params.ADCMode = MV_I2S_MODE;
    codec_params.DACDigitalIFFormat = MV_I2S_UP_TO_24_BIT;
    codec_params.twsiSlave.moreThen256 = MV_FALSE;
    codec_params.twsiSlave.validOffset = MV_TRUE;
    codec_params.twsiSlave.slaveAddr.address = mvBoardA2DTwsiAddrGet(port);
    codec_params.twsiSlave.slaveAddr.type = mvBoardA2DTwsiAddrTypeGet(port);

   
    for(vol_idx = 0; vol_idx < 2; vol_idx++)
    {
        /*printk("\tvol_list[%d] = %d.\n",vol_idx,
                                vol_list[vol_idx]);*/

        if(vol_list[vol_idx] >= MVAUD_NUM_VOLUME_STEPS)
		vol_list[vol_idx] = MVAUD_NUM_VOLUME_STEPS -1;

        mvCLAudioCodecRegSet(&codec_params,0x16 + vol_idx,
                             auddec_volume_mapping[vol_list[vol_idx]]);

        /*printk("\tVolume was set to 0x%x.\n",
                                auddec_volume_mapping[vol_list[vol_idx]]);*/
    }

    return;
}

#ifdef CONFIG_MACH_DOVE_RD_AVNG

#define lineIn		0
#define internalMic	1
#define phoneIn		2

/*
 * Set the audio codec's ADC path.
 */
void
cs42l51_input_select(MV_U8 port, int type)
{
    MV_AUDIO_CODEC_DEV codec_params;

    codec_params.chanNum = mvBoardA2DTwsiChanNumGet(port);
    codec_params.ADCMode = MV_I2S_MODE;
    codec_params.DACDigitalIFFormat = MV_I2S_UP_TO_24_BIT;
    codec_params.twsiSlave.moreThen256 = MV_FALSE;
    codec_params.twsiSlave.validOffset = MV_TRUE;
    codec_params.twsiSlave.slaveAddr.address = mvBoardA2DTwsiAddrGet(port);
    codec_params.twsiSlave.slaveAddr.type = mvBoardA2DTwsiAddrTypeGet(port);

    if (type == internalMic) {
        /* Set PDN bit first */
        mvCLAudioCodecRegSet(&codec_params,0x02,0x01);
        /* Wait until codec is in standby mode */
        mdelay(10);
        /* Enable PDN_PGAB and PDN_ADCB also */
        mvCLAudioCodecRegSet(&codec_params,0x02,0x15);
        /* Enable PDN_MICB and disable PDN_MICA and PDN_MICBIAS */
        mvCLAudioCodecRegSet(&codec_params,0x03,0xA8);
        /* MICBIAS_SEL set to AIN2B, disable MICB_BOOST and enable MICA_BOOST */
        mvCLAudioCodecRegSet(&codec_params,0x05,0x11);
        /* Set AIN3A->Pre Amp->PGAA and AIN1B->PGAB */
        mvCLAudioCodecRegSet(&codec_params,0x07,0x30);
        /* Disable PDN bit last */
        mvCLAudioCodecRegSet(&codec_params,0x02,0x14);
    } else if (type == phoneIn) {
        /* Set PDN bit first */
        mvCLAudioCodecRegSet(&codec_params,0x02,0x01);
        /* Wait until codec is in standby mode */
        mdelay(10);
        /* Enable PDN_PGAA and PDN_ADCA also */
        mvCLAudioCodecRegSet(&codec_params,0x02,0x0B);
        /* Enable PDN_MICA and disable PDN_MICB and PDN_MICBIAS */
        mvCLAudioCodecRegSet(&codec_params,0x03,0xA4);
        /* MICBIAS_SEL set to AIN2B, disable MICA_BOOST and enable MICB_BOOST */
        mvCLAudioCodecRegSet(&codec_params,0x05,0x12);
        /* Set AIN3B->Pre Amp->PGAB and AIN1A->PGAA */
        mvCLAudioCodecRegSet(&codec_params,0x07,0xC0);
        /* Disable PDN bit last */
        mvCLAudioCodecRegSet(&codec_params,0x02,0x0A);
    } else {
        /* default to LineIn */
        /* Set PDN bit first */
        mvCLAudioCodecRegSet(&codec_params,0x02,0x01);
        /* Wait until codec is in standby mode */
        mdelay(10);
        /* Enable PDN_MICB, PDN_MICA, and PDN_MICBIAS */
        mvCLAudioCodecRegSet(&codec_params,0x03,0xAE);
        /* MICBIAS_SEL set to AIN3B, disable both MICB_BOOST and MICA_BOOST */
        mvCLAudioCodecRegSet(&codec_params,0x05,0x0);
        /* Set AIN1A->PGAA and AIN1B->PGAB */
        mvCLAudioCodecRegSet(&codec_params,0x07,0x0);
        /* Disable PDN bit last */
        mvCLAudioCodecRegSet(&codec_params,0x02,0x0);
    }

    if (0) {
	    int i;
	    MV_U8 reg_data;

	    for (i=1; i<= 0x21 ; i++) {
		    reg_data = mvCLAudioCodecRegGet(&codec_params,i);
		    printk("CLS reg=0x%02x val=0x%02x\n",i,reg_data);
	    }
    }
}
#endif
