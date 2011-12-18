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
#include <sound/core.h>
#include <sound/initval.h>
#include <sound/control.h>
#include <sound/pcm.h>
#include <sound/asoundef.h>
#include <sound/asound.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
#include <sound/driver.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#include <plat/i2s-orion.h>
#endif
#include <asm/dma.h>
#include <asm/io.h>
#include <asm/scatterlist.h>
#include <asm/sizes.h>

#include "mv88fx-pcm.h"
/*#define AUDIO_REG_BASE	0x0*/
#include "audio/mvAudio.h"
#include "ctrlEnv/sys/mvSysAudio.h"

#ifdef CONFIG_MACH_DOVE_RD_AVNG
#include <asm/gpio.h>
#include <asm/mach-types.h>
#endif

int cs42l51_init(MV_U8 unit);

#ifdef CONFIG_MACH_DOVE_RD_AVNG
void cs42l51_input_select(MV_U8, int);
#endif

int mv88fx_snd_hw_init(struct mv88fx_snd_chip	*chip)
{
	/* Clear ints */
	if(chip->pdata->i2s_rec || chip->pdata->i2s_play)
		if(cs42l51_init(chip->port))
			return -ENODEV;
		
        mv88fx_snd_writel(chip, MV_AUDIO_INT_CAUSE_REG(chip->port), 0xffffffff);
        mv88fx_snd_writel(chip, MV_AUDIO_INT_MASK_REG(chip->port), 0);
        mv88fx_snd_writel(chip, MV_AUDIO_SPDIF_REC_INT_CAUSE_MASK_REG(chip->port), 
				0);
	
	if (MV_OK != mvAudioInit(chip->port))
		return -EIO;

        /* Disable all playback/recording */
	mv88fx_snd_bitreset(chip, MV_AUDIO_PLAYBACK_CTRL_REG(chip->port),
			    (APCR_PLAY_I2S_ENABLE_MASK | 
			     APCR_PLAY_SPDIF_ENABLE_MASK));

	mv88fx_snd_bitreset(chip, MV_AUDIO_RECORD_CTRL_REG(chip->port),
			    (ARCR_RECORD_SPDIF_EN_MASK | 
			     ARCR_RECORD_I2S_EN_MASK));
	
	if (MV_OK != mvSPDIFRecordTclockSet(chip->port)) {
		mv88fx_snd_debug("Marvell ALSA driver ERR. mvSPDIFRecordTclockSet failed\n");
                return -ENOMEM;
	}

        return 0;
}

int mv88fx_snd_hw_playback_set(struct mv88fx_snd_chip	*chip)
{
        struct mv88fx_snd_stream *audio_stream = 
		chip->stream[PLAYBACK]; 
	struct snd_pcm_substream *substream = audio_stream->substream;
	struct snd_pcm_runtime *runtime = substream->runtime;
        
	MV_AUDIO_PLAYBACK_CTRL	pcm_play_ctrl;
	MV_I2S_PLAYBACK_CTRL	i2s_play_ctrl;
	MV_SPDIF_PLAYBACK_CTRL	spdif_play_ctrl;
	MV_AUDIO_FREQ_DATA 	dco_ctrl;


	dco_ctrl.offset = chip->dco_ctrl_offst;
	
	switch(audio_stream->rate) {
	case 44100:
		dco_ctrl.baseFreq = AUDIO_FREQ_44_1KH;
		break;
	case 48000:
		dco_ctrl.baseFreq = AUDIO_FREQ_48KH;
		break;
	case 96000:
		dco_ctrl.baseFreq = AUDIO_FREQ_96KH;
		break;
	default:
		snd_printk("Requested rate %d is not supported\n", 
				runtime->rate); return -1;
	}

	pcm_play_ctrl.burst = (chip->burst == 128)?AUDIO_128BYTE_BURST:
				AUDIO_32BYTE_BURST;
	
	pcm_play_ctrl.loopBack = chip->loopback;
	
	if (audio_stream->stereo) {
		pcm_play_ctrl.monoMode = AUDIO_PLAY_MONO_OFF;
	}
	else {
		switch (audio_stream->mono_mode) {
		case MONO_LEFT:
			pcm_play_ctrl.monoMode = AUDIO_PLAY_LEFT_MONO;
			break;
		case MONO_RIGHT:
			pcm_play_ctrl.monoMode = AUDIO_PLAY_RIGHT_MONO;
			break;
		case MONO_BOTH:
		default:
			pcm_play_ctrl.monoMode = AUDIO_PLAY_BOTH_MONO;
			break;
		}
	}
	
	if (audio_stream->format == SAMPLE_16IN16) {
		pcm_play_ctrl.sampleSize = SAMPLE_16BIT;
		i2s_play_ctrl.sampleSize = SAMPLE_16BIT;
	}
	else if (audio_stream->format == SAMPLE_24IN32) {
		pcm_play_ctrl.sampleSize = SAMPLE_24BIT;
		i2s_play_ctrl.sampleSize = SAMPLE_24BIT;
		
	}
	else if (audio_stream->format == SAMPLE_32IN32) {
		pcm_play_ctrl.sampleSize = SAMPLE_32BIT;
		i2s_play_ctrl.sampleSize = SAMPLE_32BIT;
	}
	else {
		snd_printk("Requested format %d is not supported\n", runtime->format);
		return -1;
	}

	/* buffer and period sizes in frame */
	pcm_play_ctrl.bufferPhyBase = audio_stream->dma_addr;
	pcm_play_ctrl.bufferSize = audio_stream->dma_size;
	pcm_play_ctrl.intByteCount = audio_stream->period_size;

	/* I2S playback streem stuff */
	/*i2s_play_ctrl.sampleSize = pcm_play_ctrl.sampleSize;*/
	i2s_play_ctrl.justification = I2S_JUSTIFIED;
	i2s_play_ctrl.sendLastFrame = 0;

	spdif_play_ctrl.nonPcm = MV_FALSE;
	
	spdif_play_ctrl.validity = chip->ch_stat_valid;

	if (audio_stream->stat_mem) {
		spdif_play_ctrl.userBitsFromMemory = MV_TRUE;
		spdif_play_ctrl.validityFromMemory = MV_TRUE;
		spdif_play_ctrl.blockStartInternally = MV_FALSE;
	} else {
		spdif_play_ctrl.userBitsFromMemory = MV_FALSE;
		spdif_play_ctrl.validityFromMemory = MV_FALSE;
		spdif_play_ctrl.blockStartInternally = MV_TRUE;
	}

	/* If this is non-PCM sound, mute I2S channel */
	spin_lock_irq(&chip->reg_lock);

	if (!(mv88fx_snd_readl(chip, MV_AUDIO_PLAYBACK_CTRL_REG(chip->port)) & 
			(APCR_PLAY_I2S_ENABLE_MASK | APCR_PLAY_SPDIF_ENABLE_MASK))) {

	     if (MV_OK != mvAudioDCOCtrlSet(chip->port, &dco_ctrl)) {
		snd_printk("Failed to initialize DCO clock control.\n");
		return -1;
	}
	}

	if (audio_stream->clock_src == DCO_CLOCK)
	     while ((mv88fx_snd_readl(chip, MV_AUDIO_SPCR_DCO_STATUS_REG(chip->port)) & 
			ASDSR_DCO_LOCK_MASK) == 0);
	else if (audio_stream->clock_src == SPCR_CLOCK)
	     while ((mv88fx_snd_readl(chip, MV_AUDIO_SPCR_DCO_STATUS_REG(chip->port)) & 
			ASDSR_SPCR_LOCK_MASK) == 0);
			
	if (MV_OK != mvAudioPlaybackControlSet(chip->port, &pcm_play_ctrl)) {
		snd_printk("Failed to initialize PCM playback control.\n");
		return -1;
	}

	if (MV_OK != mvI2SPlaybackCtrlSet(chip->port, &i2s_play_ctrl)) {
		snd_printk("Failed to initialize I2S playback control.\n");
		return -1;
	}

	mvSPDIFPlaybackCtrlSet(chip->port, &spdif_play_ctrl);

	spin_unlock_irq(&chip->reg_lock);

#if 0 
	mv88fx_snd_debug("runtime->xfer_align=%d\n",(int)runtime->xfer_align);
	mv88fx_snd_debug("runtime->format=%d\n",runtime->format);
	mv88fx_snd_debug("runtime->channels=%d\n",runtime->channels);
	mv88fx_snd_debug("runtime->rate=%d\n",runtime->rate);
	mv88fx_snd_debug("runtime->dma_addr=0x%x\n",runtime->dma_addr);

	mv88fx_snd_debug("runtime->dma_area=0x%x\n",(unsigned int)runtime->dma_area);
	mv88fx_snd_debug("runtime->frame_bits=0x%x\n",runtime->frame_bits);
	mv88fx_snd_debug("runtime->period_size=0x%x\n",
		   (unsigned int)runtime->period_size);
	mv88fx_snd_debug("runtime->buffer_size=0x%x\n",
		   (unsigned int)runtime->buffer_size); 
	mv88fx_snd_debug("bufferPhyBase=0x%x\n",runtime->dma_addr);
	mv88fx_snd_debug("bufferSize=0x%x\n",pcm_play_ctrl.bufferSize);
	mv88fx_snd_debug("intByteCount=0x%x.\n",pcm_play_ctrl.intByteCount);

#endif
	
	return 0;
}


int mv88fx_snd_hw_capture_set(struct mv88fx_snd_chip	*chip)
{
        struct mv88fx_snd_stream *audio_stream = 
		chip->stream[CAPTURE]; 
	struct snd_pcm_substream *substream = audio_stream->substream;
	struct snd_pcm_runtime *runtime = substream->runtime;
	
	MV_AUDIO_RECORD_CTRL 	pcm_rec_ctrl;
	MV_I2S_RECORD_CTRL	i2s_rec_ctrl;
	MV_AUDIO_FREQ_DATA	dco_ctrl;

#ifdef CONFIG_MACH_DOVE_RD_AVNG
	int phoneInDetected;
#endif

	dco_ctrl.offset = chip->dco_ctrl_offst;

	switch(audio_stream->rate) {
	case 44100:
		dco_ctrl.baseFreq = AUDIO_FREQ_44_1KH;
		break;
	case 48000:
		dco_ctrl.baseFreq = AUDIO_FREQ_48KH;
		break;
	case 96000:
		dco_ctrl.baseFreq = AUDIO_FREQ_96KH;
		break;
	default:
		snd_printk("Requested rate %d is not supported\n", 
				runtime->rate); return -1;
	}
	
	pcm_rec_ctrl.burst = (chip->burst == 128)?AUDIO_128BYTE_BURST:
				AUDIO_32BYTE_BURST;

	if (audio_stream->format == SAMPLE_16IN16) {
		pcm_rec_ctrl.sampleSize = SAMPLE_16BIT;
	}
	else if (audio_stream->format == SAMPLE_24IN32) {
		pcm_rec_ctrl.sampleSize = SAMPLE_24BIT;
	}
	else if (audio_stream->format == SAMPLE_32IN32) {
		pcm_rec_ctrl.sampleSize = SAMPLE_32BIT;
	}
	else {
		snd_printk("Requested format %d is not supported\n", runtime->format);
		return -1;
	}

	pcm_rec_ctrl.mono = (audio_stream->stereo) ? MV_FALSE : MV_TRUE;

	if (pcm_rec_ctrl.mono) {
		switch (audio_stream->mono_mode) {
		case MONO_LEFT:
			pcm_rec_ctrl.monoChannel = AUDIO_REC_LEFT_MONO;
			break;
		default:
		case MONO_RIGHT:
			pcm_rec_ctrl.monoChannel = AUDIO_REC_RIGHT_MONO;
			break;
		}
		
	}
	else pcm_rec_ctrl.monoChannel = AUDIO_REC_LEFT_MONO;
		

	pcm_rec_ctrl.bufferPhyBase = audio_stream->dma_addr;
	pcm_rec_ctrl.bufferSize = audio_stream->dma_size;
	
	pcm_rec_ctrl.intByteCount = audio_stream->period_size;

	/* I2S record streem stuff */
	i2s_rec_ctrl.sample = pcm_rec_ctrl.sampleSize;
	i2s_rec_ctrl.justf = I2S_JUSTIFIED;

	spin_lock_irq(&chip->reg_lock);

	/* set clock only if record is not enabled*/
	if (!(mv88fx_snd_readl(chip, MV_AUDIO_RECORD_CTRL_REG(chip->port)) & 
			(ARCR_RECORD_SPDIF_EN_MASK | ARCR_RECORD_I2S_EN_MASK))) {

		if (MV_OK != mvAudioDCOCtrlSet(chip->port, &dco_ctrl)) {
		snd_printk("Failed to initialize DCO clock control.\n");
		return -1;
	}
	}
	
	if (MV_OK != mvAudioRecordControlSet(chip->port, &pcm_rec_ctrl)) {
		snd_printk("Failed to initialize PCM record control.\n");
		return -1;
	}

	if (MV_OK != mvI2SRecordCntrlSet(chip->port, &i2s_rec_ctrl)) {
		snd_printk("Failed to initialize I2S record control.\n");
		return -1;
	}

#ifdef CONFIG_MACH_DOVE_RD_AVNG
	if (machine_is_dove_rd_avng()) {
		phoneInDetected = gpio_get_value(53);
		
		if (phoneInDetected < 0) {
			snd_printk("Failed to detect phone-in.\n");
		} else {
			int input_type;
			snd_printk("detect phone-in.\n");
			if (! phoneInDetected) {
				input_type = 2;	/* external MIC */
			} else {
				input_type = 1;	/* internal MIC */
			}
			cs42l51_input_select(chip->port, input_type);
		}
	}
#endif

	spin_unlock_irq(&chip->reg_lock);

#if 0

	mv88fx_snd_debug("pcm_rec_ctrl.bufferSize=0x%x\n",(int)pcm_rec_ctrl.bufferSize);
	mv88fx_snd_debug("pcm_rec_ctrl.intByteCount=0x%x\n",(int)pcm_rec_ctrl.intByteCount);

	mv88fx_snd_debug("runtime->xfer_align=%d\n",(int)runtime->xfer_align);
	mv88fx_snd_debug("runtime->format=%d\n",runtime->format);
	mv88fx_snd_debug("runtime->channels=%d\n",runtime->channels);
	mv88fx_snd_debug("runtime->rate=%d\n",runtime->rate);
	mv88fx_snd_debug("runtime->dma_addr=0x%x\n",runtime->dma_addr);

	mv88fx_snd_debug("runtime->dma_area=0x%x\n",(unsigned int)runtime->dma_area);
	mv88fx_snd_debug("runtime->frame_bits=0x%x\n",runtime->frame_bits);
	mv88fx_snd_debug("runtime->period_size=0x%x\n",
		   (unsigned int)runtime->period_size);
	mv88fx_snd_debug("runtime->buffer_size=0x%x\n",
		   (unsigned int)runtime->buffer_size); 
	mv88fx_snd_debug("bufferPhyBase=0x%x\n",runtime->dma_addr);

#endif
	

	return 0;

}
