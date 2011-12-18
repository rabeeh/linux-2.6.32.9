/*
 *
 *	Marvell Alsa SoC Codec driver
 *
 *	Author: Yuval Elmaliah
 *	Copyright (C) 2008 Marvell Ltd.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#ifndef _CS42l51_H
#define _CS42l51_H

/* register assignement */
#define   CS42L51_REG_ID                           0x1
#define   CS42L51_REG_POWER_CNTRL                  0x2
#define   CS42L51_REG_MIC_POWER_CNTRL              0x3
#define   CS42L51_REG_IF_CNTRL                     0x4
#define   CS42L51_REG_MIC_CNTRL                    0x5
#define   CS42L51_REG_ADC_CNTRL                    0x6
#define   CS42L51_REG_ADC_INPUT_INV_MUTE           0x7
#define   CS42L51_REG_DAC_OUTPUT_CTRL              0x8
#define   CS42L51_REG_DAC_CTRL                     0x9
#define   CS42L51_REG_ALCA_PGAA_CTRL               0xa
#define   CS42L51_REG_ALCB_PGAB_CTRL               0xb
#define   CS42L51_REG_ADCA_ATTENUATOR              0xc
#define   CS42L51_REG_ADCB_ATTENUATOR              0xd
#define   CS42L51_REG_ADCA_MIXER_VOL_CNTRL         0xe
#define   CS42L51_REG_ADCB_MIXER_VOL_CNTRL         0xf
#define   CS42L51_REG_PCMA_MIXER_VOL_CNTRL         0x10
#define   CS42L51_REG_PCMB_MIXER_VOL_CNTRL         0x11
#define   CS42L51_REG_BEEP_FREQ_TIMING_CFG         0x12
#define   CS42L51_REG_BEEP_OFF_TIME                0x13
#define   CS42L51_REG_BEEP_TONE_CFG                0x14
#define   CS42L51_REG_TONE_CTRL                    0x15
#define   CS42L51_REG_VOL_OUTA_CTRL                0x16
#define   CS42L51_REG_VOL_OUTB_CTRL                0x17
#define   CS42L51_REG_CHANNEL_MIXER                0x18
#define   CS42L51_REG_LIMITER_THRESHOLD            0x19
#define   CS42L51_REG_LIMITER_RELEASE_RATE         0x1a
#define   CS42L51_REG_LIMITER_ATTACK_RATE          0x1b
#define   CS42L51_REG_ALC_ENABLE_ATTACK_RATE       0x1c
#define   CS42L51_REG_ALC_RELEASE_RATE             0x1d
#define   CS42L51_REG_ALC_THRESHOLD                0x1e
#define   CS42L51_REG_NOISE_GATE_CFG               0x1f
#define   CS42L51_REG_STATUS                       0x20
#define   CS42L51_REG_CHARGE_PUMP_FREQ             0x21

#define CS42L51_REG_MAX CS42L51_REG_CHARGE_PUMP_FREQ

struct cs42l51_setup_data {
	unsigned short i2c_address;
};

/* ASoC DAI */
extern struct snd_soc_dai cs42l51_dai;
extern struct snd_soc_codec_device soc_codec_dev_cs42l51;

#endif
