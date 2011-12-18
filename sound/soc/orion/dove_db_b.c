/*
 *	dove_db_b.c  --  AC97 SoC audio for Dove DB-88AP510-BP-B
 *
 *	Marvell Orion Alsa SOC Sound driver
 *
 *	Author: Saeed Bishara
 *	Copyright (C) 2010 Marvell Ltd.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <asm/mach-types.h>
#include <mach/dove.h>

#include "../codecs/ac97.h"
#include "../pxa/pxa2xx-pcm.h"
#include "../pxa/pxa2xx-ac97.h"

static struct platform_device *dove_ac97_snd_device;

static int machine_init(struct snd_soc_codec *codec)
{
	snd_soc_dapm_sync(codec);
	return 0;
}

static struct snd_soc_dai_link dove_ac97_dai[] = {
	{
		.name = "AC97",
		.stream_name = "AC97 HiFi",
		.cpu_dai = &pxa_ac97_dai[PXA2XX_DAI_AC97_HIFI],
		.codec_dai = &ac97_dai,
		.init = machine_init,
	},
	{
		.name = "AC97",
		.stream_name = "AC97 LFE",
		.cpu_dai = &pxa_ac97_surround_dai[PXA3XX_DAI_AC97_LFE],
		.codec_dai = &ac97_dai,
	},
	{
		.name = "AC97",
		.stream_name = "AC97 Surround",
		.cpu_dai = &pxa_ac97_surround_dai[PXA3XX_DAI_AC97_SURROUND],
		.codec_dai = &ac97_dai,
	},
};

static struct snd_soc_card dove = {
	.name = "Dove-AC97-RT655",
	.platform = &pxa2xx_soc_platform,
	.dai_link = dove_ac97_dai,
	.num_links = ARRAY_SIZE(dove_ac97_dai),
};

static struct snd_soc_device dove_ac97_snd_devdata = {
	.card = &dove,
 	.codec_dev = &soc_codec_dev_ac97,
};

extern u32 chip_rev;
static int __init dove_db_b_init(void)
{
	int ret = 0;

        if (!machine_is_dove_db_b() && !(machine_is_dove_db() && (chip_rev >= DOVE_REV_A0)))
                return -ENODEV;

	ac97_dai.playback.rates = SNDRV_PCM_RATE_48000;
	ac97_dai.capture.rates = SNDRV_PCM_RATE_48000;
	dove_ac97_snd_device = platform_device_alloc("soc-audio", 0);
	if (!dove_ac97_snd_device)
		return -ENOMEM;

	platform_set_drvdata(dove_ac97_snd_device, &dove_ac97_snd_devdata);
	dove_ac97_snd_devdata.dev = &dove_ac97_snd_device->dev;

	ret = platform_device_add(dove_ac97_snd_device);
	if (ret)
		platform_device_put(dove_ac97_snd_device);

	return ret;
}

static void __exit dove_db_b_exit(void)
{
	platform_device_unregister(dove_ac97_snd_device);
}

module_init(dove_db_b_init);
module_exit(dove_db_b_exit);

/* Module information */
MODULE_AUTHOR(" Brian Hsu <bhsu@marvell.com>");
MODULE_DESCRIPTION("ALSA SoC Dove-AC97");
MODULE_LICENSE("GPL");
