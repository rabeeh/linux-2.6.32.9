#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/ac97_codec.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include "rt655.h"

#define RT655_VERSION "0.2"
//#define RT655_DEBUG 1
#if defined(RT655_DEBUG)
#define DBG(format, arg...) \
	printk(KERN_DEBUG format "\n", ## arg)
#else
#define DBG(format, arg...) do {} while(0)
#endif

#define rt655_write_mask(c, reg, value, mask) snd_soc_update_bits(c, reg, mask, value)
static const u16 rt655_reg[] = {
	0x0000, 0x8000, 0x0000, 0x8000,//0
	0x0000, 0x8000, 0x8008, 0x8008,
	0x8808, 0x8808, 0x0000, 0x8808,//1
	0x8808, 0x0000, 0x8000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x000f,//2
	0x09c4, 0x05f0, 0xbb80, 0xbb80,
	0xbb80, 0xbb80, 0x0000, 0x8080,//3
	0x8080, 0x2000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,//4
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,//5
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0808, 0x0808,//6
	0x0AEA, 0x0040, 0x0000, 0x0004,
	0x0000, 0x0000, 0x0000, 0x0000,//7
	0x0000, 0x60a2, 0x414c, 0x4760,
};
static unsigned int rt655_read(struct snd_soc_codec *codec,
	unsigned int reg);
static int rt655_write(struct snd_soc_codec *codec,
	unsigned int reg, unsigned int val);
static int rt655_write_page(struct snd_soc_codec *codec, 
	unsigned int reg, unsigned int val, int page_id);
static int rt655_read_page(struct snd_soc_codec *codec, 
	unsigned int reg, int page_id);
static int rt655_write_page_mask(struct snd_soc_codec *codec, 
	unsigned int reg, unsigned int val, unsigned int mask, int page_id);
int rt655_reset(struct snd_soc_codec *codec, int try_warm);


static int rt655_prepare(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai);
static int rt655_shutdown(struct snd_pcm_substream *substream, 
		struct snd_soc_dai *codec_dai);
static int rt655_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai);

static int rt655_set_bias_level(struct snd_soc_codec *codec, 
			enum snd_soc_bias_level level);
static int rt655_soc_probe(struct platform_device *pdev);
static int rt655_soc_remove(struct platform_device *pdev);
static int rt655_soc_suspend(struct platform_device *pdev);
static int rt655_soc_resume(struct platform_device *pdev);

static int rt655_channels = 6;


u16 Set_Codec_Reg_Init[][2]={
	{RT655_REC_SEL			,0x0101}, //default reocrd source from Mic	
	{RT655_FRONT_DAC_VOL	,0x0808}, //set Front DAC to 0DB and unmute 
	{RT655_SUR_DAC_VOL		,0x0808}, //set Surround  DAC to 0DB and unmute 
	{RT655_CEN_LFE_DAC_VOL	,0x0808}, //set Center/LFE DAC to 0DB and unmute 
	{RT655_MASTER_VOL		,0x8000}, //front channel mute by default	
	{RT655_SUR_VOL			,0x8080}, //surround channel mute by default
	{RT655_CEN_LFE_VOL		,0x8080}, //Cent/LFE channel mute by default
};
	

static int rt655_init_reg(struct snd_soc_codec *codec) 
{
	int i;

	DBG("enter %s", __func__);
	for(i=0;i<ARRAY_SIZE(Set_Codec_Reg_Init);i++)
	{
		rt655_write(codec,Set_Codec_Reg_Init[i][0],Set_Codec_Reg_Init[i][1]);		
	}

	return 0;
}
static unsigned int rt655_read(struct snd_soc_codec *codec, unsigned int reg)
{
	return soc_ac97_ops.read(codec->ac97, reg);
}

static int rt655_write(struct snd_soc_codec *codec, 
	unsigned int reg, unsigned int val)
{
	u16 *cache = codec->reg_cache;

	soc_ac97_ops.write(codec->ac97, reg, val);
	reg = reg >> 1;
	if (reg < (ARRAY_SIZE(rt655_reg)))
		cache[reg] = val;

	return 0;	
}

static int rt655_read_page(struct snd_soc_codec *codec, 
	unsigned int reg, int page_id)
{
	rt655_write_mask(codec, 0x24, page_id, 0x000f);
	return rt655_read(codec, reg);
}

static int rt655_write_page(struct snd_soc_codec *codec, 
	unsigned int reg, unsigned int val, int page_id)
{
	rt655_write_mask(codec, 0x24, page_id, 0x000f);
	return rt655_write(codec, reg, val);
}

static int rt655_write_page_mask(struct snd_soc_codec *codec, 
	unsigned int reg, unsigned int val, unsigned int mask, int page_id)
{
	int change;
	unsigned int old, new;

	old = rt655_read_page(codec, reg, page_id);
	new = (old & ~mask) | val;
	change = old != new;
	if (change)
		rt655_write_page(codec, reg, val, page_id);
	
	return change;
}


int rt655_reset(struct snd_soc_codec *codec, int try_warm)
{
	
	DBG("enter %s", __func__);
	if (try_warm && soc_ac97_ops.warm_reset) {
		soc_ac97_ops.warm_reset(codec->ac97);
		if (codec->read(codec,0x7c) == 0x414c)
			return 1;
	}

	soc_ac97_ops.reset(codec->ac97);
	if (codec->read(codec, 0x7c) != 0x414c)
		return -EIO;
	return 0;
}

EXPORT_SYMBOL_GPL(rt655_reset);

struct snd_soc_codec_device soc_codec_dev_rt655 = {
	.probe = rt655_soc_probe,
	.remove = rt655_soc_remove,
	.suspend = rt655_soc_suspend,
	.resume = rt655_soc_resume,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_rt655);


#define RT655_HIFI_FORMAT SNDRV_PCM_FMTBIT_S16_LE
//ALC655 only support 48k
#define RT655_HIFI_RATES SNDRV_PCM_RATE_48000		

static struct snd_soc_dai_ops rt655_dai_ops = {
	.prepare = rt655_prepare,
	.hw_params	= rt655_hw_params,
	.shutdown = rt655_shutdown,
};
struct snd_soc_dai rt655_dai = 	{
	.name = "RT655",
	.playback = {
		.stream_name = "Hifi Playback",
		.channels_min = 1,
		.channels_max = 6,
		.rates= RT655_HIFI_RATES,
		.formats = RT655_HIFI_FORMAT,
	},
	.capture = {
		.stream_name = "Hifi Capture",
		.channels_min = 1,
		.channels_max = 2, 
		.rates = RT655_HIFI_RATES,
		.formats = RT655_HIFI_FORMAT,
	},
	.ops = &rt655_dai_ops,
};
EXPORT_SYMBOL_GPL(rt655_dai);
static const char *rt655_record_source[] = {"Mic", "CD", "Mute", "AUX", 
					    "Line", "Stereo Mixer", "Mono Mixer","Phone"};
static const struct soc_enum rt655_record_enum = SOC_ENUM_DOUBLE(RT655_REC_SEL, 8, 0, 8, rt655_record_source);
static const struct snd_kcontrol_new rt655_snd_ac97_controls[] = {
SOC_DOUBLE("Front Playback Switch", RT655_MASTER_VOL, 15, 7, 1, 1),
SOC_DOUBLE("Front Playback Volume", RT655_MASTER_VOL, 8, 0, 31, 1),
SOC_DOUBLE("SURR Playback Volume", RT655_SUR_VOL, 8, 0, 31, 1),
SOC_DOUBLE("SURR Playback Switch", RT655_SUR_VOL, 15, 7, 1, 1),
SOC_DOUBLE("CEN_LFE Playback Volume", RT655_CEN_LFE_VOL, 8, 0, 31, 1),
SOC_DOUBLE("CEN_LFE Playback Switch", RT655_CEN_LFE_VOL, 15, 7, 1, 1),
SOC_DOUBLE("Front DAC Playback Volume", RT655_FRONT_DAC_VOL, 8, 0, 31, 1),
SOC_DOUBLE("Front DAC Playback Switch", RT655_FRONT_DAC_VOL, 15, 7, 1, 1),
SOC_DOUBLE("SURR DAC Playback Volume", RT655_SUR_DAC_VOL, 8, 0, 31, 1),
SOC_DOUBLE("SURR DAC Playback Switch", RT655_SUR_DAC_VOL, 15, 7, 1, 1),
SOC_DOUBLE("CEN_LFE DAC Playback Volume", RT655_CEN_LFE_DAC_VOL, 8, 0, 31, 1),
SOC_DOUBLE("CEN_LFE DAC Playback Switch", RT655_CEN_LFE_DAC_VOL, 15, 7, 1, 1),
SOC_SINGLE("Mono Playback Switch", RT655_MONO_OUT_VOL, 15, 1, 1),
SOC_SINGLE("Mono Playback Volume", RT655_MONO_OUT_VOL, 0, 31, 1),
SOC_SINGLE("PC BEEP Playback Volume", RT655_PCBEEP_VOL, 1, 15, 1),
SOC_SINGLE("PC BEEP Playback Switch", RT655_PCBEEP_VOL, 15, 1, 1),
SOC_SINGLE("Phone Capture Volume", RT655_PHONE_VOL, 0, 31, 1),
SOC_SINGLE("Phone Capture Switch", RT655_PHONE_VOL, 15, 1, 1),
SOC_SINGLE("Mic Capture Volume", RT655_MIC_VOL, 0, 31, 1),
SOC_SINGLE("Mic Capture Switch", RT655_MIC_VOL, 15, 1, 1),
SOC_DOUBLE("Line Capture Switch", RT655_LINEIN_VOL, 15, 7, 1, 1),
SOC_DOUBLE("Line Capture Volume", RT655_LINEIN_VOL, 8, 0, 31, 1),
SOC_DOUBLE("CD Capture Switch", RT655_CD_VOL, 15, 7, 1, 1),
SOC_DOUBLE("CD Capture Volume", RT655_CD_VOL, 8, 0, 31, 1),
SOC_DOUBLE("AUX Capture Switch", RT655_AUX_VOL, 15, 7, 1, 1),
SOC_DOUBLE("AUX Capture Volume", RT655_AUX_VOL, 8, 0, 31, 1),
SOC_SINGLE("Record Gain Capture Switch", RT655_REC_GAIN, 15, 1, 1),
SOC_DOUBLE("Record Gain Capture Volume", RT655_REC_GAIN, 8, 0, 15, 0),
SOC_ENUM("Record Capture Switch", rt655_record_enum),
};
static int rt655_soc_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;
	int ret;

	printk(KERN_INFO "rt655 soc codec, version is %s\n", RT655_VERSION);
	socdev->card->codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (socdev->card->codec == NULL)
		return -ENOMEM;
	codec = socdev->card->codec;
	mutex_init(&codec->mutex);
	
	codec->reg_cache = kmemdup(rt655_reg, sizeof(rt655_reg), GFP_KERNEL);
	if (codec->reg_cache == NULL) {
		ret = -ENOMEM;
		goto cache_err;
	}
	codec->reg_cache_size = ARRAY_SIZE(rt655_reg) * 2;
	codec->reg_cache_step = 2;
/*
	codec->private_data = kzalloc(sizeof(struct rt655_priv), GFP_KERNEL);
	if (codec->private_data == NULL) {
		ret = -ENOMEM;
		goto priv_err;
	}
*/
	codec->name = "rt655";
	codec->owner = THIS_MODULE;
	codec->dai = &rt655_dai;
	codec->num_dai = 1;
	codec->read = rt655_read;
	codec->write = rt655_write;
	codec->set_bias_level = rt655_set_bias_level;
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);
	
	ret = snd_soc_new_ac97_codec(codec, &soc_ac97_ops, 0);
	if (ret < 0) {
		printk(KERN_ERR "rt655: failed to register ac97 codec\n");
		goto codec_err;
	}
	
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0)
		goto pcm_err;

	ret = rt655_reset(codec, 0);
	if (ret < 0) {
		printk(KERN_ERR "FAIL to reset rt655\n");
		goto reset_err;
	}
	
	rt655_write(codec, 0x26, 0x0000);
	rt655_init_reg(codec);	//initize some register
	rt655_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	
	snd_soc_add_controls(codec, rt655_snd_ac97_controls,
				ARRAY_SIZE(rt655_snd_ac97_controls));
	ret = snd_soc_init_card(socdev);
	if (ret < 0) {
		printk(KERN_ERR "rt655: failed to register card\n");
		goto reset_err;
	}
	return 0;
reset_err:
	snd_soc_free_pcms(socdev);

pcm_err:
	snd_soc_free_ac97_codec(codec);

codec_err:
	kfree(codec->reg_cache);

cache_err:
	kfree(socdev->card->codec);
	socdev->card->codec = NULL;
	return ret;
}

static int rt655_soc_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;

	DBG("enter %s", __func__);
	if (codec == NULL)
		return 0;

	snd_soc_dapm_free(socdev);
	snd_soc_free_pcms(socdev);
	snd_soc_free_ac97_codec(codec);
	kfree(codec->reg_cache);
	kfree(codec);
	return 0;
}

static int rt655_soc_suspend(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;

	DBG("enter %s", __func__);
	rt655_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

static int rt655_soc_resume(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;
	int i, ret;
	u16 *cache = codec->reg_cache;

	DBG("enter %s", __func__);
	ret = rt655_reset(codec, 1);
	if (ret < 0) {
		printk(KERN_ERR "could not reset AC97 codec\n");
		return ret;
	}

	rt655_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	if (ret == 0) {
		/* Sync reg_cache with the hardware after cold reset */
		for (i = 2; i < ARRAY_SIZE(rt655_reg) << 1; i += 2) {
			if (i == 0x26)
				continue;
			soc_ac97_ops.write(codec->ac97, i, cache[i>>1]);
		}
	}

	if (codec->suspend_bias_level == SND_SOC_BIAS_ON)
		rt655_set_bias_level(codec, SND_SOC_BIAS_ON);

	return ret;
}

static int rt655_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
#if 0
	switch (params_channels(params)) {
		case 2:
			
			rt655_channels=2;
			rt655_write_mask(codec,RT655_MULT_CHAN_CTRL,0x0000,0x0030);		
			
			break;

		case 4:

			rt655_channels=4;
			rt655_write_mask(codec,RT655_MULT_CHAN_CTRL,0x0010,0x0030);

			break;

		case 6:

			rt655_channels=6;
			rt655_write_mask(codec,RT655_MULT_CHAN_CTRL,0x0030,0x0030);

			break;

	}
#else
			rt655_channels=6;
			rt655_write_mask(codec,RT655_MULT_CHAN_CTRL,0x0030,0x0030);

#endif
	return 0;
}


static int rt655_prepare(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec =  dai->codec;
	int stream;

	DBG("enter %s", __func__);

	stream = substream->stream;
	switch(stream) {
		case SNDRV_PCM_STREAM_PLAYBACK:

				switch (rt655_channels) {
					case 2:			

						rt655_write_mask(codec, RT655_SUR_VOL	 , 0x8080, 0x8080);
						rt655_write_mask(codec, RT655_CEN_LFE_VOL, 0x8080, 0x8080);
						rt655_write_mask(codec, RT655_MASTER_VOL , 0x0000, 0x8000);	
			
					break;

					case 4:

						rt655_write_mask(codec, RT655_CEN_LFE_VOL, 0x8080, 0x8080);
						rt655_write_mask(codec, RT655_SUR_VOL	 , 0x0000, 0x8080);						
						rt655_write_mask(codec, RT655_MASTER_VOL , 0x0000, 0x8000);	

					break;

					case 6:

						rt655_write_mask(codec, RT655_SUR_VOL	 , 0x0000, 0x8080);
						rt655_write_mask(codec, RT655_CEN_LFE_VOL, 0x0000, 0x8080);
						rt655_write_mask(codec, RT655_MASTER_VOL , 0x0000, 0x8000);	

					break;
				}	

			break;

		case SNDRV_PCM_STREAM_CAPTURE:

			break;
	}	
	return 0;
}
static int rt655_shutdown(struct snd_pcm_substream *substream, 
	struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	int stream;

	DBG("enter %s", __func__);

	stream = substream->stream;
	switch(stream) {
		case SNDRV_PCM_STREAM_PLAYBACK:
			//mute all output
			rt655_write_mask(codec, RT655_SUR_VOL	 , 0x8080, 0x8080);
			rt655_write_mask(codec, RT655_CEN_LFE_VOL, 0x8080, 0x8080);
			rt655_write_mask(codec, RT655_MASTER_VOL , 0x8000, 0x8000);	
			break;
		case SNDRV_PCM_STREAM_CAPTURE:
			break;

	}
	return 0;
}

static int rt655_set_bias_level(struct snd_soc_codec *codec, 
			enum snd_soc_bias_level level)
{
	DBG("enter %s", __func__);
	
	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
		break;
	case SND_SOC_BIAS_STANDBY:
		rt655_write(codec, 0x26, 0x0000);
		break;
	case SND_SOC_BIAS_OFF:
		rt655_write(codec, 0x26, 0xffff);
		break;
	}
	return 0;
}
MODULE_DESCRIPTION("ASoC RT655 driver");
MODULE_AUTHOR("rory_yin@realsil.com.cn");
MODULE_LICENSE("GPL");
