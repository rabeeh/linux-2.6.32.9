/*
 * dove_db.c  --  SoC audio for Dove development board
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
#include <mach/pxa-regs.h>
//#include <mach/hardware.h>

#include "../codecs/ad1980.h"
#include "../pxa/pxa2xx-pcm.h"
#include "../pxa/pxa2xx-ac97.h"
#include <mach/dove.h>

static struct snd_soc_card dovedb;

static struct snd_soc_dai_link dovedb_dai[] = {
{
	.name = "AC97",
	.stream_name = "AC97 HiFi",
	.cpu_dai = &pxa_ac97_dai[PXA2XX_DAI_AC97_HIFI],
	.codec_dai = &ad1980_dai,
},
{
	.name = "AC97",
	.stream_name = "AC97 LFE",
	.cpu_dai = &pxa_ac97_surround_dai[PXA3XX_DAI_AC97_LFE],
	.codec_dai = &ad1980_dai,
},
{
	.name = "AC97",
	.stream_name = "AC97 Surround",
	.cpu_dai = &pxa_ac97_surround_dai[PXA3XX_DAI_AC97_SURROUND],
	.codec_dai = &ad1980_dai,
},
};

static struct snd_soc_card dovedb = {
	.name = "DoveDB",
	.platform = &pxa2xx_soc_platform,
	.dai_link = dovedb_dai,
	.num_links = ARRAY_SIZE(dovedb_dai),
};

static struct snd_soc_device dovedb_snd_devdata = {
	.card = &dovedb,
	.codec_dev = &soc_codec_dev_ad1888,
};

static struct platform_device *dovedb_snd_device;

extern u32 chip_rev;
static int __init dovedb_init(void)
{
	int ret;

	if (!(machine_is_dove_db() && (chip_rev < DOVE_REV_A0)) && !machine_is_dove_db_z0())
		return -ENODEV;

	dovedb_snd_device = platform_device_alloc("soc-audio", 0);
	if (!dovedb_snd_device)
		return -ENOMEM;

	platform_set_drvdata(dovedb_snd_device, &dovedb_snd_devdata);
	dovedb_snd_devdata.dev = &dovedb_snd_device->dev;
	ret = platform_device_add(dovedb_snd_device);

	if (ret)
		platform_device_put(dovedb_snd_device);

	return ret;
}

static void __exit dovedb_exit(void)
{
	platform_device_unregister(dovedb_snd_device);
}

module_init(dovedb_init);
module_exit(dovedb_exit);

/* Module information */
MODULE_AUTHOR("Shadi Ammouri");
MODULE_DESCRIPTION("ALSA SoC Dove-DB");
MODULE_LICENSE("GPL");
