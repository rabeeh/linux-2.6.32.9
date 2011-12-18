#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include "cs42l51.h"

#define AUDIO_NAME "cs42l51"

#define CS42L51_CACHE_SIZE (CS42L51_REG_MAX+1)


/*
#define CS42L51_DEBUG(format, args...) \
	printk(KERN_DEBUG "%s(%d): "format"\n", __FUNCTION__, __LINE__, ##args)
*/

#define CS42L51_DEBUG(format, args...)

/* codec private data */
struct cs42l51_priv {
	unsigned int sysclk;
	struct snd_soc_codec *codec;
};

static unsigned int cs42l51_read_reg_cache(struct snd_soc_codec *codec,
					   unsigned int reg)
{
	u8 *cache = codec->reg_cache;
	if (reg > CS42L51_REG_MAX)
		return -1;
	return cache[reg];
}

static void cs42l51_write_reg_cache(struct snd_soc_codec *codec,
				    u8 reg, u8 value)
{
	u8 *cache = codec->reg_cache;
	if (reg > CS42L51_REG_MAX)
		return;
	cache[reg] = value;
}

static int cs42l51_write(struct snd_soc_codec *codec, unsigned int reg,
			 unsigned int value)
{
	u8 data[2];

	reg &= 0x7f;
	data[0] = reg & 0xff;
	data[1] = value & 0xff;

	cs42l51_write_reg_cache(codec, data[0], data[1]);
	if (codec->hw_write(codec->control_data, data, 2) == 2)
		return 0;
	else
		return -EIO;
}

static unsigned int cs42l51_read(struct snd_soc_codec *codec, unsigned int reg)
{
	return cs42l51_read_reg_cache(codec, reg);
}

static unsigned int cs42l51_read_no_cache(struct snd_soc_codec *codec,
					  unsigned int reg)
{
	struct i2c_msg msg[2];
	u8 data[2];
	struct i2c_client *i2c_client = codec->control_data;
	int ret;

	i2c_client = (struct i2c_client *)codec->control_data;

	data[0] = reg & 0xff;
	msg[0].addr = i2c_client->addr;
	msg[0].flags = 0;
	msg[0].buf = &data[0];
	msg[0].len = 1;

	msg[1].addr = i2c_client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = &data[1];
	msg[1].len = 1;

	ret = i2c_transfer(i2c_client->adapter, &msg[0], 2);
	return (ret == 2) ? data[1] : -EIO;
}

static void cs42l51_fill_cache(struct snd_soc_codec *codec)
{
	u8 *cache = codec->reg_cache;
	unsigned int reg;

	for (reg = 1; reg <= CS42L51_REG_MAX; reg++)
		cache[reg] = cs42l51_read_no_cache(codec, reg);

}


static void cs42l51_sync_cache(struct snd_soc_codec *codec)
{
	u8 *cache = codec->reg_cache;
	unsigned int reg;

	for (reg = 1; reg <= CS42L51_REG_MAX; reg++)
		cs42l51_write(codec, reg, cache[reg]);
}


static void cs42l51_set_bits(struct snd_soc_codec *codec, u32 addr,
			     u32 start_bit, u32 end_bit, u32 value)
{
	u32 mask;
	u32 new_value;
	u32 old_value;

	old_value = cs42l51_read(codec, addr);
	mask = ((1 << (end_bit + 1 - start_bit)) - 1) << start_bit;
	new_value = old_value & (~mask);
	new_value |= (mask & (value << start_bit));
	cs42l51_write(codec, addr, new_value);
}

static u8 cs42l51_volume_mapping[] = {
	0x19, 0xB2, 0xB7, 0xBD, 0xC3, 0xC9, 0xCF, 0xD5,
	0xD8, 0xE1, 0xE7, 0xED, 0xF3, 0xF9, 0xFF, 0x00,
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
	0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
	0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18
};

#define CS42L51_NUM_VOLUME_STEPS  ARRAY_SIZE(cs42l51_volume_mapping)

static u8 cs42l51_reg2vol(u8 reg)
{
	u8 i;

	for (i = 0; i < CS42L51_NUM_VOLUME_STEPS; i++) {
		if (reg == cs42l51_volume_mapping[i])
			return i;
		if ((cs42l51_volume_mapping[i] >
		     cs42l51_volume_mapping[CS42L51_NUM_VOLUME_STEPS - 1])
		    && (reg > cs42l51_volume_mapping[i])
		    && (reg < cs42l51_volume_mapping[i + 1]))
			return i;

	}
	return 0;
}

static u8 cs42l51_vol2reg(u8 vol)
{
	if (vol >= CS42L51_NUM_VOLUME_STEPS)
		vol = CS42L51_NUM_VOLUME_STEPS - 1;
	return cs42l51_volume_mapping[vol];
}

static int cs42l51_vol_info(struct snd_kcontrol *kcontrol,
			    struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 39;

	return 0;
}

static int cs42l51_vol_get(struct snd_kcontrol *kcontrol,
			   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	u8 reg;
	u8 vol;

	reg = snd_soc_read(codec, CS42L51_REG_VOL_OUTA_CTRL);
	vol = cs42l51_reg2vol(reg);
	ucontrol->value.integer.value[0] = vol;

	reg = snd_soc_read(codec, CS42L51_REG_VOL_OUTB_CTRL);
	vol = cs42l51_reg2vol(reg);
	ucontrol->value.integer.value[1] = vol;
	return 0;
}

static int cs42l51_vol_set(struct snd_kcontrol *kcontrol,
			   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	u8 reg;
	u8 vol;

	vol = (u8) ucontrol->value.integer.value[0];
	reg = cs42l51_vol2reg(vol);
	snd_soc_update_bits(codec, CS42L51_REG_VOL_OUTA_CTRL, 0xFF, reg);

	vol = (u8) ucontrol->value.integer.value[1];
	reg = cs42l51_vol2reg(vol);
	snd_soc_update_bits(codec, CS42L51_REG_VOL_OUTB_CTRL, 0xFF, reg);

	return 0;
}

static struct snd_kcontrol_new cs42l51_snd_controls[] = {
	{
	 .iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	 .name = "Playback DAC Volume",
	 .info = cs42l51_vol_info,
	 .get = cs42l51_vol_get,
	 .put = cs42l51_vol_set},
};

/* add non dapm controls */
static int cs42l51_add_controls(struct snd_soc_codec *codec)
{
	int err, i;

	CS42L51_DEBUG("");

	for (i = 0; i < ARRAY_SIZE(cs42l51_snd_controls); i++) {
		err = snd_ctl_add(codec->card,
				  snd_soc_cnew(&cs42l51_snd_controls[i], codec,
					       NULL));

		if (err < 0)
			return err;
	}
	return 0;
}

static int cs42l51_startup(struct snd_pcm_substream *stream,
				struct snd_soc_dai *dai)
{
	CS42L51_DEBUG("");
	return 0;
}

static void cs42l51_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	CS42L51_DEBUG("");
}

static int cs42l51_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *hw_params,
				struct snd_soc_dai *dai)
{
	CS42L51_DEBUG("");
	return 0;
}

static int cs42l51_hw_free(struct snd_pcm_substream *stream,
				struct snd_soc_dai *dai)
{
	CS42L51_DEBUG("");
	return 0;
}

static int cs42l51_prepare(struct snd_pcm_substream *stream,
				struct snd_soc_dai *dai)
{
	CS42L51_DEBUG("");
	return 0;
}

static int cs42l51_trigger(struct snd_pcm_substream *stream, int cmd,
				struct snd_soc_dai *dai)
{
	CS42L51_DEBUG("");
	return 0;
}

static int cs42l51_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;

	CS42L51_DEBUG("mute=%d", mute);

	if (mute)
		cs42l51_set_bits(codec, CS42L51_REG_DAC_OUTPUT_CTRL, 0, 1, 3);
	else
		cs42l51_set_bits(codec, CS42L51_REG_DAC_OUTPUT_CTRL, 0, 1, 0);

	return 0;
}

static int cs42l51_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	CS42L51_DEBUG("clk_id=%d freq=%u dir=%d", clk_id, freq, dir);
	return 0;
}

static int cs42l51_set_dai_fmt(struct snd_soc_dai *dai,
			       unsigned int fmt)
{
	struct snd_soc_codec *codec = dai->codec;

	CS42L51_DEBUG("fmt=%u", fmt);

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_LEFT_J:
		cs42l51_set_bits(codec, CS42L51_REG_IF_CNTRL, 3, 5, 0);
		break;
	case SND_SOC_DAIFMT_I2S:
		cs42l51_set_bits(codec, CS42L51_REG_IF_CNTRL, 3, 5, 1);
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		cs42l51_set_bits(codec, CS42L51_REG_IF_CNTRL, 3, 5, 2);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}


static int cs42l51_set_bias_level(struct snd_soc_codec *codec,
				 enum snd_soc_bias_level level)
{
	CS42L51_DEBUG("bias level transition: %d -> %d",
		codec->bias_level, level);
	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
		cs42l51_set_bits(codec, CS42L51_REG_POWER_CNTRL, 0, 0, 0);
		break;
	case SND_SOC_BIAS_STANDBY:
	case SND_SOC_BIAS_OFF:
		cs42l51_set_bits(codec, CS42L51_REG_POWER_CNTRL, 0, 0, 1);
		break;
	}
	codec->bias_level = level;
	return 0;
}

static struct snd_soc_dai_ops cs42l51_dai_ops = {
		.startup = cs42l51_startup,
		.shutdown = cs42l51_shutdown,
		.hw_params = cs42l51_hw_params,
		.hw_free = cs42l51_hw_free,
		.prepare = cs42l51_prepare,
		.trigger = cs42l51_trigger,
		.digital_mute = cs42l51_mute,
		.set_sysclk = cs42l51_set_dai_sysclk,
		.set_fmt = cs42l51_set_dai_fmt,
};


struct snd_soc_dai cs42l51_dai = {
	.name = "CS42L51",
	.playback = {
		     .stream_name = "Playback",
		     .rate_min = 44100,
		     .rate_max = 96000,
		     .channels_min = 1,
		     .channels_max = 2,
		     .rates = (SNDRV_PCM_RATE_44100 |
			       SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000),
		     .formats = (SNDRV_PCM_FMTBIT_S16_LE |
				 SNDRV_PCM_FMTBIT_S24_LE |
				 SNDRV_PCM_FMTBIT_S32_LE),
		     },
	.capture = {
		    .stream_name = "Capture",
		    .rate_min = 44100,
		    .rate_max = 96000,
		    .channels_min = 1,
		    .channels_max = 2,
		    .rates = (SNDRV_PCM_RATE_44100 |
			      SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000),
		    .formats = (SNDRV_PCM_FMTBIT_S16_LE |
				SNDRV_PCM_FMTBIT_S24_LE |
				SNDRV_PCM_FMTBIT_S32_LE),
		    },
	/* pcm operations */
	.ops = &cs42l51_dai_ops,
};
EXPORT_SYMBOL_GPL(cs42l51_dai);

static int cs42l51_init(struct snd_soc_device *socdev)
{
	struct snd_soc_codec *codec = socdev->card->codec;
	u8 reg;
	int ret;

	CS42L51_DEBUG("");

	codec->owner = THIS_MODULE;
	codec->name = "CS42L51";
	codec->dai = &cs42l51_dai;
	codec->set_bias_level = cs42l51_set_bias_level;
	codec->num_dai = 1;
	codec->reg_cache_size = CS42L51_CACHE_SIZE;
	codec->reg_cache = kzalloc(codec->reg_cache_size, GFP_KERNEL);
	if (!codec->reg_cache)
		return -ENOMEM;

	cs42l51_fill_cache(codec);

	ret = -ENODEV;

	reg = cs42l51_read(codec, CS42L51_REG_ID);
	printk(KERN_INFO "cs42l51: chipid/revision = %x\n", reg);

	/* Set the digital Interface format to I2S-up-to-24-bit */
	cs42l51_set_bits(codec, CS42L51_REG_IF_CNTRL, 3, 5, 1);

	/* Set the ADC Mode */
	cs42l51_set_bits(codec, CS42L51_REG_IF_CNTRL, 2, 2, 1);

	/* init the chip */
	/* use signal processor */
	cs42l51_write(codec, CS42L51_REG_DAC_CTRL, 0x40);
	/* Unmute PCM-A & PCM-B and set default */
	cs42l51_write(codec, CS42L51_REG_PCMA_MIXER_VOL_CNTRL, 0x10);
	cs42l51_write(codec, CS42L51_REG_PCMB_MIXER_VOL_CNTRL, 0x10);
	/* default for AOUTx */
	cs42l51_write(codec, CS42L51_REG_VOL_OUTA_CTRL, cs42l51_vol2reg(4));
	cs42l51_write(codec, CS42L51_REG_VOL_OUTB_CTRL, cs42l51_vol2reg(4));
	/* swap channels */
	cs42l51_write(codec, CS42L51_REG_CHANNEL_MIXER, 0xff);

	cs42l51_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	/* register pcms */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		pr_err("failed to create pcms");
		goto pcm_err;
	}
	cs42l51_add_controls(codec);


	ret = snd_soc_init_card(socdev);
	if (ret < 0) {
		pr_err("failed to register card\n");
		goto card_err;
	}
	return 0;

card_err:
	snd_soc_free_pcms(socdev);

pcm_err:
	kfree(codec->reg_cache);
	return ret;
}

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)

static const struct i2c_device_id cs42l51_i2c_table[] = {
	{"cs42l51", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, cs42l51_i2c_table);

static struct i2c_driver cs42l51_i2c_driver;
static struct snd_soc_device *cs42l51_socdev;

static int cs42l51_i2c_probe(struct i2c_client *client,
			     const struct i2c_device_id *id)
{
	struct snd_soc_device *socdev = cs42l51_socdev;
	struct snd_soc_codec *codec = socdev->card->codec;
	struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
	int ret;

	CS42L51_DEBUG("");

	if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_EMUL))
		return -EIO;

	i2c_set_clientdata(client, codec);
	codec->control_data = client;

	codec->read = cs42l51_read;
	codec->write = cs42l51_write;

	ret = cs42l51_init(socdev);
	if (ret < 0)
		goto err;

	return ret;

err:
	kfree(codec);
	return ret;
}

static int cs42l51_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static struct i2c_driver cs42l51_i2c_driver = {
	.driver = {
		   .name = "cs42l51",
		   .owner = THIS_MODULE,
		   },
	.probe = cs42l51_i2c_probe,
	.remove = cs42l51_i2c_remove,
	.id_table = cs42l51_i2c_table,
};

#endif

static int cs42l51_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec;
	struct cs42l51_priv *cs;
	struct cs42l51_setup_data *setup;
	int ret;

	CS42L51_DEBUG("");

	ret = -ENOMEM;
	setup = socdev->codec_data;
	codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (codec == NULL)
		goto out;

	cs = kzalloc(sizeof(struct cs42l51_priv), GFP_KERNEL);
	if (cs == NULL) {
		kfree(codec);
		goto out;
	}
	ret = 0;

	cs->codec = codec;
	codec->private_data = cs;
	socdev->card->codec = codec;
	mutex_init(&codec->mutex);
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	cs42l51_socdev = socdev;
#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	if (setup && setup->i2c_address) {
		codec->hw_write = (hw_write_t) i2c_master_send;
		ret = i2c_add_driver(&cs42l51_i2c_driver);
		if (ret)
			printk(KERN_ERR "can't add i2c driver\n");
	}
#endif

out:
	return ret;
}

/* power down chip */
static int cs42l51_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;

	CS42L51_DEBUG("");

	snd_soc_free_pcms(socdev);
#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	i2c_del_driver(&cs42l51_i2c_driver);
#endif
	kfree(codec->private_data);
	kfree(codec);

	return 0;
}

static int cs42l51_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;

	CS42L51_DEBUG("event=%d", state.event);

	cs42l51_set_bias_level(codec, SND_SOC_BIAS_OFF);

	return 0;
}

static int cs42l51_resume(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;

	CS42L51_DEBUG("");

	cs42l51_sync_cache(codec);
	cs42l51_set_bias_level(codec, codec->suspend_bias_level);

	return 0;
}

struct snd_soc_codec_device soc_codec_dev_cs42l51 = {
	.probe = cs42l51_probe,
	.remove = cs42l51_remove,
	.suspend = cs42l51_suspend,
	.resume = cs42l51_resume,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_cs42l51);

static int __init cs42l51_modinit(void)
{
	CS42L51_DEBUG("");
	return snd_soc_register_dai(&cs42l51_dai);
}
module_init(cs42l51_modinit);

static void __exit cs42l51_modexit(void)
{
	CS42L51_DEBUG("");
	snd_soc_unregister_dai(&cs42l51_dai);
}
module_exit(cs42l51_modexit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ASoC CS42l51 codec driver");
MODULE_AUTHOR("Yuval Elmaliah <eyuval@marvell.com>");
