/*
 * linux/sound/pxa3xx-ac97-Surround.c -- AC97 surround support for the
 * Marvell PXA3xx chip.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/clk.h>
#include <linux/delay.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/ac97_codec.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include <asm/irq.h>
#include <linux/mutex.h>
#include <mach/hardware.h>
#include <mach/pxa-regs.h>
#include <mach/pxa2xx-gpio.h>
#include <sound/pxa2xx-lib.h>
#include "pxa2xx-pcm.h"
#include "pxa2xx-ac97.h"
#include <asm/mach-types.h>

static struct pxa2xx_pcm_dma_params pxa2xx_ac97_pcm_surround_out = {
	.name			= "AC97 PCM Surround out",
	.dev_addr		= __PREG(PCSDR),
	.drcmr			= &DRCMR95,
	.dcmd			= DCMD_INCSRCADDR | DCMD_FLOWTRG |
				  DCMD_BURST32 | DCMD_WIDTH4,
};


static struct pxa2xx_pcm_dma_params pxa2xx_ac97_pcm_lfe_out = {
	.name			= "AC97 PCM LFE out",
	.dev_addr		= __PREG(PCCLDR),
	.drcmr			= &DRCMR96,
	.dcmd			= DCMD_INCSRCADDR | DCMD_FLOWTRG |
				  DCMD_BURST32 | DCMD_WIDTH4,
};

static int pxa3xx_ac97_surround_probe(struct platform_device *pdev,
		struct snd_soc_dai *dai)
{
	return 0;
}

static void pxa3xx_ac97_surround_remove(struct platform_device *pdev,
		struct snd_soc_dai *dai)
{
}

static int pxa2xx_ac97_hw_surround_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		cpu_dai->dma_data = &pxa2xx_ac97_pcm_surround_out;

	return 0;
}

static int pxa2xx_ac97_hw_lfe_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		cpu_dai->dma_data = &pxa2xx_ac97_pcm_lfe_out;

	return 0;
}

#define PXA2XX_AC97_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
		SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 | \
		SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000)

static struct snd_soc_dai_ops pxa2xx_ac97_surround_ops = {
	.hw_params	= pxa2xx_ac97_hw_surround_params,
};

static struct snd_soc_dai_ops pxa2xx_ac97_lfe_ops = {
	.hw_params	= pxa2xx_ac97_hw_lfe_params,
};

/*
 * There is only 1 physical AC97 interface for pxa2xx, but it
 * has extra fifo's that can be used for aux DACs and ADCs.
 */
struct snd_soc_dai pxa_ac97_surround_dai[] = {
{
	.name = "pxa2xx-surround-ac97",
	.id = 0,
	.ac97_control = 1,
	.probe = pxa3xx_ac97_surround_probe,
	.remove = pxa3xx_ac97_surround_remove,
	.playback = {
		.stream_name = "AC97 Surround Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = PXA2XX_AC97_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,},
	.ops = &pxa2xx_ac97_surround_ops,
},
{
	.name = "pxa2xx-lfe-ac97",
	.id = 1,
	.ac97_control = 1,
	.playback = {
		.stream_name = "AC97 LFE Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = PXA2XX_AC97_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,},
	.ops = &pxa2xx_ac97_lfe_ops,
},
};
EXPORT_SYMBOL_GPL(pxa_ac97_surround_dai);

static int __init pxa_ac97_surround_init(void)
{
	if (machine_is_dove_rd_avng())
		return -ENODEV;
	return snd_soc_register_dais(pxa_ac97_surround_dai,
			ARRAY_SIZE(pxa_ac97_surround_dai));
}
module_init(pxa_ac97_surround_init);

static void __exit pxa_ac97_surround_exit(void)
{
	snd_soc_unregister_dais(pxa_ac97_surround_dai,
			ARRAY_SIZE(pxa_ac97_surround_dai));
}
module_exit(pxa_ac97_surround_exit);


MODULE_AUTHOR("Shadi Ammouri");
MODULE_DESCRIPTION("AC97 surround driver for the Intel PXA3xx chip");
MODULE_LICENSE("GPL");

