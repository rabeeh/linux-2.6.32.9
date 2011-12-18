/*
 *	dove_ac97avng.c  --  SoC audio for Dove Avengers MID board
 *
 *	Marvell Orion Alsa SOC Sound driver
 *
 *	Author: Brian Hsu
 *	Copyright (C) 2009 Marvell Ltd.
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
//#include <asm/arch/pxa-regs.h>
//#include <asm/arch/hardware.h>

#include "../codecs/rt5610.h"
#include "../pxa/pxa2xx-pcm.h"
#include "../pxa/pxa2xx-ac97.h"

static struct platform_device *dove_ac97_snd_device;

static struct snd_soc_dai_link dove_ac97_dai[] = {
	{
	 .name = "AC97",
	 .stream_name = "AC97 HiFi",
	 .cpu_dai = &pxa_ac97_dai[PXA2XX_DAI_AC97_HIFI],
	 .codec_dai = &rt5610_dai,
	 },
};

static struct snd_soc_card dove = {
	.name = "Dove-AC97-RT5611",
	.platform = &pxa2xx_soc_platform,
	.dai_link = dove_ac97_dai,
	.num_links = ARRAY_SIZE(dove_ac97_dai),
};

static struct snd_soc_device dove_ac97_snd_devdata = {
	.card = &dove,
	.codec_dev = &soc_codec_dev_rt5610,
};

struct platform_device rt5610_codec_dev = {
	.name           = "rt5610-codec",
	.id             = 0,
};

static int dove_ac97_snd_probe(struct platform_device *pdev)
{
	struct resource *res;
	int ret = 0;

        if (!machine_is_dove_rd_avng() && !machine_is_dove_rd_avng_z0())
                return -ENODEV;

	dove_ac97_snd_device = platform_device_alloc("soc-audio", 1);
	if (!dove_ac97_snd_device)
		return -ENOMEM;

	platform_set_drvdata(dove_ac97_snd_device, &dove_ac97_snd_devdata);
	dove_ac97_snd_devdata.dev = &dove_ac97_snd_device->dev;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	ret = platform_device_add_resources(dove_ac97_snd_device, res, 1);
	
	//printk(KERN_ERR "dove_ac97_avng: add resource ret=%x\n", ret);

	
	ret = platform_device_add(dove_ac97_snd_device);

	if (ret)
		platform_device_put(dove_ac97_snd_device);

	ret = platform_device_register(&rt5610_codec_dev);
	if(ret)
                platform_device_unregister(&rt5610_codec_dev);


	return ret;

}

static int dove_ac97_snd_remove(struct platform_device *pdev)

{
	platform_device_unregister(&rt5610_codec_dev);
	platform_device_unregister(dove_ac97_snd_device);
	return 0;
}
static struct platform_driver dove_ac97_snd_driver = {
	.probe = dove_ac97_snd_probe,
	.remove = dove_ac97_snd_remove,
	.driver = {
		   .name = "rt5611_snd",
		   },

};
static int __init dove_ac97_snd_init(void)
{
	if (!machine_is_dove_rd_avng())
		return -ENODEV;

	return platform_driver_register(&dove_ac97_snd_driver);
}

static void __exit dove_ac97_snd_exit(void)
{
	platform_driver_unregister(&dove_ac97_snd_driver);

}
module_init(dove_ac97_snd_init);
module_exit(dove_ac97_snd_exit);

/* Module information */
MODULE_AUTHOR(" Brian Hsu <bhsu@marvell.com>");
MODULE_DESCRIPTION("ALSA SoC Dove-AC97");
MODULE_LICENSE("GPL");
