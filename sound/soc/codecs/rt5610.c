#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>

#include <sound/ac97_codec.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <asm/div64.h>

#include "rt5610.h"
#define AUDIO_NAME "rt5610"
#define RT5610_VERSION "0.03"
//#define RT5610_DEBUG
#ifdef RT5610_DEBUG
#define dbg(format, args...) \
	printk(KERN_ERR "%s(%d): "format"\n", __FUNCTION__, __LINE__, ##args)
#else
#define dbg(format, arg...) do {} while (0)
#endif
#define err(format, arg...) \
	printk(KERN_ERR AUDIO_NAME ": " format "\n" , ## arg)
#define info(format, arg...) \
	printk(KERN_INFO AUDIO_NAME ": " format "\n" , ## arg)
#define warn(format, arg...) \
	printk(KERN_WARNING AUDIO_NAME ": " format "\n" , ## arg)
	
static struct snd_soc_codec *codec;
bool rt5610_codec_resumed=0;
bool spk_enable=0;
bool hs_det=0;
unsigned int  hs_irq;
struct work_struct hsdet_event_work;
struct workqueue_struct *hsdet_workq;

/* codec private data */
struct rt5610_priv {
	unsigned int pll_id;
	unsigned int pll_in;
	unsigned int pll_out;
	unsigned int vpcm_sysclk;
};



#define USE_DAPM_CONTROL 0

struct rt5610_init_reg {
	char name[30];
	u16 reg_value;
	u8 reg_index;
};

static struct rt5610_init_reg init_data_list[] = 
{
	{"HP Output Volume", 0x9090, RT5610_HP_OUT_VOL},
	{"SPK Output Volume", 0x8080, RT5610_SPK_OUT_VOL},
	{"Stereo DAC Volume", 0x6808, RT5610_STEREO_DAC_VOL},
	{"Output Mixer Control", 0x6b40, RT5610_OUTPUT_MIXER_CTRL},
	{"Mic Control", 0x0500, RT5610_MIC_CTRL},
	{"Voice DAC Volume", 0x6800, RT5610_VOICE_DAC_OUT_VOL},
	{"ADC Rec Mixer", 0x3f3f, RT5610_ADC_REC_MIXER},
	{"General Control", 0x00e8, RT5610_GEN_CTRL_REG1},
	
};
#define RT5610_INIT_REG_NUM ARRAY_SIZE(init_data_list)

/*
 * rt5610 register cache
 * We can't read the RT5610 register space when we
 * are using 2 wire for device control, so we cache them instead.
 */
static const u16 rt5610_reg[] = {
	0x59b4, 0x8080, 0x8080, 0x0000, // 6
	0xc880, 0xe808, 0xe808, 0x0808, // e
	0xe0e0, 0xf58b, 0x7f7f, 0x0000, // 16
	0xe800, 0x0000, 0x0000, 0x0000, // 1e
	0x0000, 0x0000, 0x0000, 0xef00, // 26
	0x0000, 0x0000, 0xbb80, 0x0000, // 2e
	0x0000, 0xbb80, 0x0000, 0x0000, // 36
	0x0000, 0x0000, 0x0000, 0x0000, // 3e
	0x0428, 0x0000, 0x0000, 0x0000, // 46
	0x0000, 0x0000, 0x2e3e, 0x2e3e, // 4e
	0x0000, 0x0000, 0x003a, 0x0000, // 56
	0x0cff, 0x0000, 0x0000, 0x0000, // 5e
	0x0000, 0x0000, 0x2130, 0x0010, // 66
	0x0053, 0x0000, 0x0000, 0x0000, // 6e
	0x0000, 0x0000, 0x008c, 0x3f00, // 76
	0x0000, 0x0000, 0x10ec, 0x1003, // 7e
};



/* virtual HP mixers regs */
#define HPL_MIXER	0x80
#define HPR_MIXER	0x82

static u16 reg80=0x2,reg82=0x2;

/*
 * read rt5610 register cache
 */
static inline unsigned int rt5610_read_reg_cache(struct snd_soc_codec *codec,
	unsigned int reg)
{
	u16 *cache = codec->reg_cache;
	if (reg < 1 || reg > (ARRAY_SIZE(rt5610_reg) + 1))
		return -1;
	return cache[reg/2];
}


/*
 * write rt5610 register cache
 */

static inline void rt5610_write_reg_cache(struct snd_soc_codec *codec,
	unsigned int reg, unsigned int value)
{
	u16 *cache = codec->reg_cache;
	if (reg < 0 || reg > 0x7e)
		return;
	cache[reg/2] = value;
}



static unsigned int rt5610_read(struct snd_soc_codec *codec, 
		unsigned int reg)
{
	if (reg == 0x80)
		return reg80;
	else if (reg == 0x82)
		return reg82;

	return soc_ac97_ops.read(codec->ac97, reg);	
}


static int rt5610_write(struct snd_soc_codec *codec, unsigned int reg,
	unsigned int val)
{
	u16 *cache = codec->reg_cache;

	dbg("reg=0x%x, val=0x%x\n", reg, val);
	if (reg == 0x80) {
		reg80 = val;
		return 0;
	}
	else if (reg == 0x82){
		reg82 = val;
		return 0;
	}
		
	if (reg < 0x7c)
		soc_ac97_ops.write(codec->ac97, reg, val);
	reg = reg >> 1;
	if (reg < (ARRAY_SIZE(rt5610_reg)))
		cache[reg] = val;
	return 0;
}

int rt5610_reset(struct snd_soc_codec *codec, int try_warm)
{
	if (try_warm && soc_ac97_ops.warm_reset) {
		soc_ac97_ops.warm_reset(codec->ac97);
		if (rt5610_read(codec, 0) == rt5610_reg[0])
			return 1;
	}

	soc_ac97_ops.reset(codec->ac97);
	if(rt5610_read(codec, 0) != rt5610_reg[0])
		return -EIO;
	mdelay(10);
	return 0;
}
EXPORT_SYMBOL_GPL(rt5610_reset);
	
#define rt5610_write_mask(c, reg, value, mask) snd_soc_update_bits(c, reg, mask, value)

static int rt5610_reg_init(struct snd_soc_codec *codec)
{
	int i;

	for (i = 0; i < RT5610_INIT_REG_NUM; i++)
		rt5610_write(codec, init_data_list[i].reg_index, init_data_list[i].reg_value);
	return 0;
}

static const char *rt5610_spkl_pga[] = {"Vmid","HPL mixer","SPK mixer","Mono Mixer"};
static const char *rt5610_spkr_pga[] = {"Vmid","HPR mixer","SPK mixer","Mono Mixer"};
static const char *rt5610_hpl_pga[]  = {"Vmid","HPL mixer"};
static const char *rt5610_hpr_pga[]  = {"Vmid","HPR mixer"};
static const char *rt5610_mono_pga[] = {"Vmid","HP mixer","SPK mixer","Mono Mixer"};
static const char *rt5610_amp_type_sel[] = {"Class AB","Class D"};
static const char *rt5610_mic_boost_sel[] = {"Bypass","20db","30db","40db"};

static const struct soc_enum rt5610_enum[] = {
SOC_ENUM_SINGLE(RT5610_OUTPUT_MIXER_CTRL, 14, 4, rt5610_spkl_pga), /* spk left input sel 0 */	
SOC_ENUM_SINGLE(RT5610_OUTPUT_MIXER_CTRL, 11, 4, rt5610_spkr_pga), /* spk right input sel 1 */	
SOC_ENUM_SINGLE(RT5610_OUTPUT_MIXER_CTRL, 9, 2, rt5610_hpl_pga), /* hp left input sel 2 */	
SOC_ENUM_SINGLE(RT5610_OUTPUT_MIXER_CTRL, 8, 2, rt5610_hpr_pga), /* hp right input sel 3 */	
SOC_ENUM_SINGLE(RT5610_OUTPUT_MIXER_CTRL, 6, 4, rt5610_mono_pga), /* mono input sel 4 */
SOC_ENUM_SINGLE(RT5610_MIC_CTRL			, 10,4, rt5610_mic_boost_sel), /*Mic1 boost sel 5 */
SOC_ENUM_SINGLE(RT5610_MIC_CTRL			, 8,4,rt5610_mic_boost_sel), /*Mic2 boost sel 6 */
SOC_ENUM_SINGLE(RT5610_OUTPUT_MIXER_CTRL,13,2,rt5610_amp_type_sel), /*Speaker AMP sel 7 */
};

static int rt5610_amp_sel_put(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned short val;
	unsigned short mask, bitmask;

	for (bitmask = 1; bitmask < e->max; bitmask <<= 1)
		;
	if (ucontrol->value.enumerated.item[0] > e->max - 1)
		return -EINVAL;
	val = ucontrol->value.enumerated.item[0] << e->shift_l;
	mask = (bitmask - 1) << e->shift_l;
	if (e->shift_l != e->shift_r) {
		if (ucontrol->value.enumerated.item[1] > e->max - 1)
			return -EINVAL;
		val |= ucontrol->value.enumerated.item[1] << e->shift_r;
		mask |= (bitmask - 1) << e->shift_r;
	}

	 snd_soc_update_bits(codec, e->reg, mask, val);
	 val &= (0x1 << 13);
	 if (val == 0)
		 rt5610_write_mask(codec, 0x3c, 0x4000, 0x4000);
	 else
	 	 rt5610_write_mask(codec, 0x3c, 0x0000, 0x4000);

	return 0;
}

static int rt5610_vpcm_enable_control(struct snd_soc_codec *codec, int enable)
{
	return 0;
}

/*extend interface for voice pcm control*/
static int rt5610_vpcm_state_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int mode = rt5610_read(codec, HPL_MIXER);

	if (((mode & 0x8000) >> 15) == (ucontrol->value.integer.value[0]))
		return 0;
	mode &= 0x7fff;
	mode |= (ucontrol->value.integer.value[0] << 15);
	rt5610_write(codec, HPL_MIXER, mode);
	rt5610_vpcm_enable_control(codec, ucontrol->value.integer.value[0]);		
	return 0;
}

static const struct snd_kcontrol_new rt5610_snd_controls[] = {
SOC_DOUBLE("SPKOUT Playback Volume", 	RT5610_SPK_OUT_VOL, 8, 0, 31, 1),	
SOC_DOUBLE("SPKOUT Playback Switch", 	RT5610_SPK_OUT_VOL, 15, 7, 1, 1),
SOC_DOUBLE("HPOUT Playback Volume", RT5610_HP_OUT_VOL, 8, 0, 31, 1),
SOC_DOUBLE("HPOUT Playback Switch", RT5610_HP_OUT_VOL,15, 7, 1, 1),
//SOC_SINGLE("Mono Playback Volume", 		RT5610_PHONEIN_MONO_OUT_VOL, 0, 31, 1),
//SOC_SINGLE("Mono Playback Switch", 		RT5610_PHONEIN_MONO_OUT_VOL, 7, 1, 1),
SOC_DOUBLE("PCM Playback Volume", 		RT5610_STEREO_DAC_VOL, 8, 0, 31, 1),
SOC_DOUBLE("PCM Playback Switch", 		RT5610_STEREO_DAC_VOL,15, 7, 1, 1),
//SOC_DOUBLE("Line In Volume", 			RT5610_LINE_IN_VOL, 8, 0, 31, 1),
SOC_SINGLE("Mic Playback Volume", 				RT5610_MIC_VOL, 8, 31, 1),
//SOC_SINGLE("Mic 2 Volume", 				RT5610_MIC_VOL, 0, 31, 1),
//SOC_ENUM("Mic 1 Boost", 				rt5610_enum[5]),
//SOC_ENUM("Mic 2 Boost", 				rt5610_enum[6]),
//SOC_ENUM_EXT("Speaker Amp Type",			rt5610_enum[7], snd_soc_get_enum_double, rt5610_amp_sel_put),
//SOC_SINGLE("Phone In Volume", 			RT5610_PHONEIN_MONO_OUT_VOL, 8, 31, 1),
SOC_DOUBLE("PCM Capture Volume", 			RT5610_ADC_REC_GAIN, 7, 0, 31, 0),
//SOC_SINGLE_EXT("Voice PCM Switch", HPL_MIXER, 15, 1, 1, snd_soc_get_volsw, rt5610_vpcm_state_put),
};



/* add non dapm controls */
static int rt5610_add_controls(struct snd_soc_codec *codec)
{
	int err, i;

	for (i = 0; i < ARRAY_SIZE(rt5610_snd_controls); i++) {
		err = snd_ctl_add(codec->card,
				snd_soc_cnew(&rt5610_snd_controls[i],codec, NULL));
		if (err < 0)
			return err;
	}
	return 0;
}

#if USE_DAPM_CONTROL

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

	u16 l, r, lineIn,mic1,mic2, phone, pcm,voice;

	l = rt5610_read(w->codec, HPL_MIXER);
	r = rt5610_read(w->codec, HPR_MIXER);
	lineIn = rt5610_read(w->codec, RT5610_LINE_IN_VOL);
	mic2 = rt5610_read(w->codec, RT5610_MIC_ROUTING_CTRL);
	phone = rt5610_read(w->codec,RT5610_PHONEIN_MONO_OUT_VOL);
	pcm = rt5610_read(w->codec, RT5610_STEREO_DAC_VOL);
	voice = rt5610_read(w->codec, RT5610_VOICE_DAC_OUT_VOL);

	if (event & SND_SOC_DAPM_PRE_REG)
		return 0;
	if (l & 0x1 || r & 0x1)
		rt5610_write(w->codec, RT5610_VOICE_DAC_OUT_VOL, voice & 0x7fff);
	else
		rt5610_write(w->codec, RT5610_VOICE_DAC_OUT_VOL, voice | 0x8000);

	if (l & 0x2 || r & 0x2)
		rt5610_write(w->codec, RT5610_STEREO_DAC_VOL, pcm & 0x7fff);
	else
		rt5610_write(w->codec, RT5610_STEREO_DAC_VOL, pcm | 0x8000);

	if (l & 0x4 || r & 0x4)
		rt5610_write(w->codec, RT5610_MIC_ROUTING_CTRL, mic2 & 0xff7f);
	else
		rt5610_write(w->codec, RT5610_MIC_ROUTING_CTRL, mic2 | 0x0080);

	mic1 = rt5610_read(w->codec, RT5610_MIC_ROUTING_CTRL);
	if (l & 0x8 || r & 0x8)
		rt5610_write(w->codec, RT5610_MIC_ROUTING_CTRL, mic1 & 0x7fff);
	else
		rt5610_write(w->codec, RT5610_MIC_ROUTING_CTRL, mic1 | 0x8000);

	if (l & 0x10 || r & 0x10)
		rt5610_write(w->codec, RT5610_PHONEIN_MONO_OUT_VOL, phone & 0x7fff);
	else
		rt5610_write(w->codec, RT5610_PHONEIN_MONO_OUT_VOL, phone | 0x8000);

	if (l & 0x20 || r & 0x20)
		rt5610_write(w->codec, RT5610_LINE_IN_VOL, lineIn & 0x7fff);
	else
		rt5610_write(w->codec, RT5610_LINE_IN_VOL, lineIn | 0x8000);

	return 0;
}

/* Left Headphone Mixers */
static const struct snd_kcontrol_new rt5610_hpl_mixer_controls[] = {
SOC_DAPM_SINGLE("LineIn Playback Switch", HPL_MIXER, 5, 1, 0),
SOC_DAPM_SINGLE("PhoneIn Playback Switch", HPL_MIXER, 4, 1, 0),
SOC_DAPM_SINGLE("Mic1 Playback Switch", HPL_MIXER, 3, 1, 0),
SOC_DAPM_SINGLE("Mic2 Playback Switch", HPL_MIXER, 2, 1, 0),
SOC_DAPM_SINGLE("PCM Playback Switch", HPL_MIXER, 1, 1, 0),
SOC_DAPM_SINGLE("Voice Playback Switch", HPL_MIXER, 0, 1, 0),
SOC_DAPM_SINGLE("RecordL Playback Switch", RT5610_ADC_REC_GAIN, 15, 1,1),
};

/* Right Headphone Mixers */
static const struct snd_kcontrol_new rt5610_hpr_mixer_controls[] = {
SOC_DAPM_SINGLE("LineIn Playback Switch", HPR_MIXER, 5, 1, 0),
SOC_DAPM_SINGLE("PhoneIn Playback Switch", HPR_MIXER, 4, 1, 0),
SOC_DAPM_SINGLE("Mic1 Playback Switch", HPR_MIXER, 3, 1, 0),
SOC_DAPM_SINGLE("Mic2 Playback Switch", HPR_MIXER, 2, 1, 0),
SOC_DAPM_SINGLE("PCM Playback Switch", HPR_MIXER, 1, 1, 0),
SOC_DAPM_SINGLE("Voice Playback Switch", HPR_MIXER, 0, 1, 0),
SOC_DAPM_SINGLE("RecordR Playback Switch", RT5610_ADC_REC_GAIN, 14, 1,1),
};

//Left Record Mixer
static const struct snd_kcontrol_new rt5610_captureL_mixer_controls[] = {
SOC_DAPM_SINGLE("Mic1 Capture Switch", RT5610_ADC_REC_MIXER, 14, 1, 1),
SOC_DAPM_SINGLE("Mic2 Capture Switch", RT5610_ADC_REC_MIXER, 13, 1, 1),
SOC_DAPM_SINGLE("LineInL Capture Switch",RT5610_ADC_REC_MIXER,12, 1, 1),
SOC_DAPM_SINGLE("Phone Capture Switch", RT5610_ADC_REC_MIXER, 11, 1, 1),
SOC_DAPM_SINGLE("HPMixerL Capture Switch", RT5610_ADC_REC_MIXER,10, 1, 1),
SOC_DAPM_SINGLE("SPKMixer Capture Switch",RT5610_ADC_REC_MIXER,9, 1, 1),
SOC_DAPM_SINGLE("MonoMixer Capture Switch",RT5610_ADC_REC_MIXER,8, 1, 1),
};


//Right Record Mixer
static const struct snd_kcontrol_new rt5610_captureR_mixer_controls[] = {
SOC_DAPM_SINGLE("Mic1 Capture Switch", RT5610_ADC_REC_MIXER, 6, 1, 1),
SOC_DAPM_SINGLE("Mic2 Capture Switch", RT5610_ADC_REC_MIXER, 5, 1, 1),
SOC_DAPM_SINGLE("LineInR Capture Switch",RT5610_ADC_REC_MIXER,4, 1, 1),
SOC_DAPM_SINGLE("Phone Capture Switch", RT5610_ADC_REC_MIXER, 3, 1, 1),
SOC_DAPM_SINGLE("HPMixerR Capture Switch", RT5610_ADC_REC_MIXER,2, 1, 1),
SOC_DAPM_SINGLE("SPKMixer Capture Switch",RT5610_ADC_REC_MIXER,1, 1, 1),
SOC_DAPM_SINGLE("MonoMixer Capture Switch",RT5610_ADC_REC_MIXER,0, 1, 1),
};

/* Speaker Mixer */
static const struct snd_kcontrol_new rt5610_speaker_mixer_controls[] = {
SOC_DAPM_SINGLE("LineIn Playback Switch", RT5610_LINE_IN_VOL, 14, 1, 1),
SOC_DAPM_SINGLE("PhoneIn Playback Switch", RT5610_PHONEIN_MONO_OUT_VOL, 14, 1, 1),
SOC_DAPM_SINGLE("Mic1 Playback Switch", RT5610_MIC_ROUTING_CTRL, 14, 1, 1),
SOC_DAPM_SINGLE("Mic2 Playback Switch", RT5610_MIC_ROUTING_CTRL, 6, 1, 1),
SOC_DAPM_SINGLE("PCM Playback Switch", RT5610_STEREO_DAC_VOL, 14, 1, 1),
SOC_DAPM_SINGLE("Voice Playback Switch", RT5610_VOICE_DAC_OUT_VOL, 14, 1, 1),
};


/* Mono Mixer */
static const struct snd_kcontrol_new rt5610_mono_mixer_controls[] = {
SOC_DAPM_SINGLE("LineIn Playback Switch", RT5610_LINE_IN_VOL, 13, 1, 1),
SOC_DAPM_SINGLE("Mic1 Playback Switch", RT5610_MIC_ROUTING_CTRL, 13, 1, 1),
SOC_DAPM_SINGLE("Mic2 Playback Switch", RT5610_MIC_ROUTING_CTRL, 5, 1, 1),
SOC_DAPM_SINGLE("PCM Playback Switch", RT5610_STEREO_DAC_VOL, 13, 1, 1),
SOC_DAPM_SINGLE("Voice Playback Switch", RT5610_VOICE_DAC_OUT_VOL, 13, 1, 1),
SOC_DAPM_SINGLE("RecordL Playback Switch", RT5610_ADC_REC_GAIN, 13, 1,1),
SOC_DAPM_SINGLE("RecordR Playback Switch", RT5610_ADC_REC_GAIN, 12, 1,1),
};

/* mono output mux */
static const struct snd_kcontrol_new rt5610_mono_mux_controls =
SOC_DAPM_ENUM("Route", rt5610_enum[4]);

/* speaker left output mux */
static const struct snd_kcontrol_new rt5610_hp_spkl_mux_controls =
SOC_DAPM_ENUM("Route", rt5610_enum[0]);

/* speaker right output mux */
static const struct snd_kcontrol_new rt5610_hp_spkr_mux_controls =
SOC_DAPM_ENUM("Route", rt5610_enum[1]);

/* headphone left output mux */
static const struct snd_kcontrol_new rt5610_hpl_out_mux_controls =
SOC_DAPM_ENUM("Route", rt5610_enum[2]);

/* headphone right output mux */
static const struct snd_kcontrol_new rt5610_hpr_out_mux_controls =
SOC_DAPM_ENUM("Route", rt5610_enum[3]);

static const struct snd_soc_dapm_widget rt5610_dapm_widgets[] = {
SND_SOC_DAPM_MUX("Mono Out Mux", SND_SOC_NOPM, 0, 0,
	&rt5610_mono_mux_controls),
SND_SOC_DAPM_MUX("Left Speaker Out Mux", SND_SOC_NOPM, 0, 0,
	&rt5610_hp_spkl_mux_controls),
SND_SOC_DAPM_MUX("Right Speaker Out Mux", SND_SOC_NOPM, 0, 0,
	&rt5610_hp_spkr_mux_controls),
SND_SOC_DAPM_MUX("Left Headphone Out Mux", SND_SOC_NOPM, 0, 0,
	&rt5610_hpl_out_mux_controls),
SND_SOC_DAPM_MUX("Right Headphone Out Mux", SND_SOC_NOPM, 0, 0,
	&rt5610_hpr_out_mux_controls),
SND_SOC_DAPM_MIXER_E("Left HP Mixer",RT5610_PWR_MANAG_ADD2, 5, 0,
	&rt5610_hpl_mixer_controls[0], ARRAY_SIZE(rt5610_hpl_mixer_controls),
	mixer_event, SND_SOC_DAPM_POST_REG),
SND_SOC_DAPM_MIXER_E("Right HP Mixer",RT5610_PWR_MANAG_ADD2, 4, 0,
	&rt5610_hpr_mixer_controls[0], ARRAY_SIZE(rt5610_hpr_mixer_controls),
	mixer_event, SND_SOC_DAPM_POST_REG),
SND_SOC_DAPM_MIXER("Mono Mixer", RT5610_PWR_MANAG_ADD2, 2, 0,
	&rt5610_mono_mixer_controls[0], ARRAY_SIZE(rt5610_mono_mixer_controls)),
SND_SOC_DAPM_MIXER("Speaker Mixer", RT5610_PWR_MANAG_ADD2,3,0,
	&rt5610_speaker_mixer_controls[0],
	ARRAY_SIZE(rt5610_speaker_mixer_controls)),	
SND_SOC_DAPM_MIXER("Left Record Mixer", RT5610_PWR_MANAG_ADD2,1,0,
	&rt5610_captureL_mixer_controls[0],
	ARRAY_SIZE(rt5610_captureL_mixer_controls)),	
SND_SOC_DAPM_MIXER("Right Record Mixer", RT5610_PWR_MANAG_ADD2,0,0,
	&rt5610_captureR_mixer_controls[0],
	ARRAY_SIZE(rt5610_captureR_mixer_controls)),				
SND_SOC_DAPM_DAC("Left DAC", "Left HiFi Playback", RT5610_PWR_MANAG_ADD2,9, 0),
SND_SOC_DAPM_DAC("Right DAC", "Right HiFi Playback", RT5610_PWR_MANAG_ADD2, 8, 0),
/*rory add 090706*/
SND_SOC_DAPM_DAC("Voice DAC", "Voice Playback", RT5610_PWR_MANAG_ADD2, 10, 0),
/*rory end*/
SND_SOC_DAPM_MIXER("IIS Mixer", SND_SOC_NOPM, 0, 0, NULL, 0),
SND_SOC_DAPM_MIXER("HP Mixer", SND_SOC_NOPM, 0, 0, NULL, 0),
/*rory add 090706*/
SND_SOC_DAPM_MIXER("Line Mixer", SND_SOC_NOPM, 0, 0, NULL, 0),
/*rory end*/
SND_SOC_DAPM_ADC("Left ADC", "Left HiFi Capture", RT5610_PWR_MANAG_ADD2, 7, 0),
SND_SOC_DAPM_ADC("Right ADC", "Right HiFi Capture", RT5610_PWR_MANAG_ADD2, 6, 0),
SND_SOC_DAPM_PGA("Left Headphone", RT5610_PWR_MANAG_ADD3, 11, 0, NULL, 0),
SND_SOC_DAPM_PGA("Right Headphone", RT5610_PWR_MANAG_ADD3, 10, 0, NULL, 0),
SND_SOC_DAPM_PGA("Left Speaker", RT5610_PWR_MANAG_ADD3, 9, 0, NULL, 0),
SND_SOC_DAPM_PGA("Right Speaker", RT5610_PWR_MANAG_ADD3, 8, 0, NULL, 0),
SND_SOC_DAPM_PGA("Mono Out", RT5610_PWR_MANAG_ADD3, 14, 0, NULL, 0),
SND_SOC_DAPM_PGA("Left Line In", RT5610_PWR_MANAG_ADD3, 7, 0, NULL, 0),
SND_SOC_DAPM_PGA("Right Line In", RT5610_PWR_MANAG_ADD3, 6, 0, NULL, 0),
SND_SOC_DAPM_PGA("Phone In PGA", RT5610_PWR_MANAG_ADD3, 5, 0, NULL, 0),
SND_SOC_DAPM_PGA("Phone In Mixer", RT5610_PWR_MANAG_ADD3, 4, 0, NULL, 0),
SND_SOC_DAPM_PGA("Mic 1 PGA", RT5610_PWR_MANAG_ADD3, 3, 0, NULL, 0),
SND_SOC_DAPM_PGA("Mic 2 PGA", RT5610_PWR_MANAG_ADD3, 2, 0, NULL, 0),
SND_SOC_DAPM_PGA("Mic 1 Pre Amp", RT5610_PWR_MANAG_ADD3, 1, 0, NULL, 0),
SND_SOC_DAPM_PGA("Mic 2 Pre Amp", RT5610_PWR_MANAG_ADD3, 0, 0, NULL, 0),
SND_SOC_DAPM_PGA("HP Amp", RT5610_PWR_MANAG_ADD1, 14, 0, NULL, 0),
SND_SOC_DAPM_PGA("SPKL Amp", RT5610_PWR_MANAG_ADD3, 13, 0, NULL, 0),
SND_SOC_DAPM_PGA("SPKR Amp", RT5610_PWR_MANAG_ADD3, 12, 0, NULL, 0),
SND_SOC_DAPM_MICBIAS("Mic Bias1", RT5610_PWR_MANAG_ADD1, 3, 0),
SND_SOC_DAPM_MICBIAS("Mic Bias2", RT5610_PWR_MANAG_ADD1, 2, 0),
SND_SOC_DAPM_OUTPUT("MONO"),
SND_SOC_DAPM_OUTPUT("HPL"),
SND_SOC_DAPM_OUTPUT("HPR"),
SND_SOC_DAPM_OUTPUT("SPKL"),
SND_SOC_DAPM_OUTPUT("SPKR"),
SND_SOC_DAPM_INPUT("LINEL"),
SND_SOC_DAPM_INPUT("LINER"),
SND_SOC_DAPM_INPUT("PHONEIN"),
SND_SOC_DAPM_INPUT("MIC1"),
SND_SOC_DAPM_INPUT("MIC2"),
SND_SOC_DAPM_INPUT("PCMIN"),
SND_SOC_DAPM_VMID("VMID"),
};

static const struct snd_soc_dapm_route audio_map[] = {
	/* left HP mixer */
	{"Left HP Mixer"	,	"LineIn Playback Switch"		,	"Left Line In"},
	{"Left HP Mixer"	,	"PhoneIn Playback Switch"		,	"Phone In PGA"},
	{"Left HP Mixer"	,	"Mic1 Playback Switch"			,	"Mic 1 PGA"},
	{"Left HP Mixer"	,	"Mic2 Playback Switch"			,	"Mic 2 PGA"},
	{"Left HP Mixer"	,	"PCM Playback Switch"			,	"Left DAC"},
	{"Left HP Mixer"	,	"Voice Playback Switch"			,	"Voice DAC"},
	{"Left HP Mixer"	,	"RecordL Playback Switch"		,	"Left Record Mixer"},
	
	/* right HP mixer */
	{"Right HP Mixer"	,	"LineIn Playback Switch"		,	"Right Line In"},
	{"Right HP Mixer"	,	"PhoneIn Playback Switch"		,	"Phone In PGA"},
	{"Right HP Mixer"	,	"Mic1 Playback Switch"			,	"Mic 1 PGA"},
	{"Right HP Mixer"	,	"Mic2 Playback Switch"			,	"Mic 2 PGA"},
	{"Right HP Mixer"	,	"PCM Playback Switch"			,	"Right DAC"},
	{"Right HP Mixer"	,	"Voice Playback Switch"			,	"Voice DAC"},
	{"Right HP Mixer"	,	"RecordR Playback Switch"		,	"Right Record Mixer"},
	
	/* virtual mixer - mixes left & right channels for spk and mono */
	{"IIS Mixer"		,	NULL						,	"Left DAC"},
	{"IIS Mixer"		,	NULL						,	"Right DAC"},
	{"Line Mixer"		,	NULL						,	"Right Line In"},
	{"Line Mixer"		,	NULL						,	"Left Line In"},
	{"HP Mixer"		,	NULL						,	"Left HP Mixer"},
	{"HP Mixer"		,	NULL						,	"Right HP Mixer"},
	
	/* speaker mixer */
	{"Speaker Mixer"	,	"LineIn Playback Switch"		,	"Line Mixer"},
	{"Speaker Mixer"	,	"PhoneIn Playback Switch"		,	"Phone In PGA"},
	{"Speaker Mixer"	,	"Mic1 Playback Switch"			,	"Mic 1 PGA"},
	{"Speaker Mixer"	,	"Mic2 Playback Switch"			,	"Mic 2 PGA"},
	{"Speaker Mixer"	,	"PCM Playback Switch"			,	"IIS Mixer"},
	{"Speaker Mixer"	,	"Voice Playback Switch"			,	"Voice DAC"},

	/* mono mixer */
	{"Mono Mixer"		,	"LineIn Playback Switch"		,	"Line Mixer"},
	//{"Mono Mixer", "PhoneIn Playback Switch","Phone In PGA"},
	{"Mono Mixer"		,	"Mic1 Playback Switch"			,	"Mic 1 PGA"},
	{"Mono Mixer"		,	"Mic2 Playback Switch"			,	"Mic 2 PGA"},
	{"Mono Mixer"		,	"PCM Playback Switch"			,	"IIS Mixer"},
	{"Mono Mixer"		,	"Voice Playback Switch"			,	"Voice DAC"},
	{"Mono Mixer"		,	"RecordL Playback Switch"		,	"Left Record Mixer"},
	{"Mono Mixer"		,	"RecordR Playback Switch"		,	"Right Record Mixer"},
	
	/*Left record mixer */
	{"Left Record Mixer"	,	"Mic1 Capture Switch"		,	"Mic 1 Pre Amp"},
	{"Left Record Mixer"	,	"Mic2 Capture Switch"		,	"Mic 2 Pre Amp"},
	{"Left Record Mixer"	, 	"LineInL Capture Switch"	,	"LINEL"},
	{"Left Record Mixer"	,	"Phone Capture Switch"		,	"Phone In Mixer"},
	{"Left Record Mixer"	, 	"HPMixerL Capture Switch"	,	"Left HP Mixer"},
	{"Left Record Mixer"	, 	"SPKMixer Capture Switch"	,	"Speaker Mixer"},
	{"Left Record Mixer"	, 	"MonoMixer Capture Switch"	,	"Mono Mixer"},
	
	/*Right record mixer */
	{"Right Record Mixer"	, 	"Mic1 Capture Switch"		,	"Mic 1 Pre Amp"},
	{"Right Record Mixer"	,	 "Mic2 Capture Switch"		,	"Mic 2 Pre Amp"},
	{"Right Record Mixer"	, 	"LineInR Capture Switch"	,	"LINER"},
	{"Right Record Mixer"	, 	"Phone Capture Switch"		,	"Phone In Mixer"},
	{"Right Record Mixer"	, 	"HPMixerR Capture Switch"	,	"Right HP Mixer"},
	{"Right Record Mixer"	, 	"SPKMixer Capture Switch"	,	"Speaker Mixer"},
	{"Right Record Mixer"	,	 "MonoMixer Capture Switch"	,	"Mono Mixer"},	

	/* headphone left mux */
	{"Left Headphone Out Mux"	,	 "HPL mixer"			,	 "Left HP Mixer"},

	/* headphone right mux */
	{"Right Headphone Out Mux", "HPR mixer", "Right HP Mixer"},

	/* speaker left mux */
	{"Left Speaker Out Mux", "HPL mixer", "Left HP Mixer"},
	{"Left Speaker Out Mux", "SPK mixer", "Speaker Mixer"},
	{"Left Speaker Out Mux", "Mono Mixer", "Mono Mixer"},

	/* speaker right mux */
	{"Right Speaker Out Mux", "HPR mixer", "Right HP Mixer"},
	{"Right Speaker Out Mux", "SPK mixer", "Speaker Mixer"},
	{"Right Speaker Out Mux", "Mono Mixer", "Mono Mixer"},

	/* mono mux */
	{"Mono Out Mux", "HP mixer", "HP Mixer"},
	{"Mono Out Mux", "SPK mixer", "Speaker Mixer"},
	{"Mono Out Mux", "Mono Mixer", "Mono Mixer"},
	
	/* output pga */
	{"HPL", NULL, "HP Amp"},
	{"HP Amp", NULL, "Left Headphone"},
	{"Left Headphone", NULL, "Left Headphone Out Mux"},
	{"HPR", NULL, "HP Amp"},
	{"HP Amp", NULL, "Right Headphone"},
	{"Right Headphone", NULL, "Right Headphone Out Mux"},
	{"SPKL", NULL, "SPKL Amp"},
	{"SPKL Amp", NULL, "Left Speaker"},
	{"Left Speaker", NULL, "Left Speaker Out Mux"},
	{"SPKR", NULL, "SPKR Amp"},
	{"SPKR Amp", NULL, "Right Speaker"},
	{"Right Speaker", NULL, "Right Speaker Out Mux"},
	{"MONO", NULL, "Mono Out"},
	{"Mono Out", NULL, "Mono Out Mux"},

	/* input pga */
	{"Left Line In", NULL, "LINEL"},
	{"Right Line In", NULL, "LINER"},
	{"Phone In PGA", NULL, "Phone In Mixer"},
	{"Phone In Mixer", NULL, "PHONEIN"},
	{"Mic 1 Pre Amp", NULL, "MIC1"},
	{"Mic 2 Pre Amp", NULL, "MIC2"},	
	{"Mic 1 PGA", NULL, "Mic 1 Pre Amp"},
	{"Mic 2 PGA", NULL, "Mic 2 Pre Amp"},

	/* left ADC */
	{"Left ADC", NULL, "Left Record Mixer"},

	/* right ADC */
	{"Right ADC", NULL, "Right Record Mixer"},
	
};

static int rt5610_add_widgets(struct snd_soc_codec *codec)
{
	snd_soc_dapm_new_controls(codec, rt5610_dapm_widgets,
				ARRAY_SIZE(rt5610_dapm_widgets));
	snd_soc_dapm_add_routes(codec, audio_map, ARRAY_SIZE(audio_map));
	snd_soc_dapm_new_widgets(codec);
}

#else
static int rt5610_pcm_hw_prepare(struct snd_pcm_substream *substream,
		struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	int stream = substream->stream;
	dbg("");

	switch (stream)
	{
		case SNDRV_PCM_STREAM_PLAYBACK:
			rt5610_write_mask(codec, 0x3c, 0x0330, 0x0330);
			rt5610_write_mask(codec, 0x3e, 0x3f00, 0x3f00);
			rt5610_write_mask(codec, 0x3a, 0x4000, 0x4000);
			rt5610_write_mask(codec, 0x02, 0x0000, 0x8080);
			rt5610_write_mask(codec, 0x04, 0x0000, 0x8080);
			break;
		case SNDRV_PCM_STREAM_CAPTURE:
			rt5610_write_mask(codec, 0x3a, 0x0008, 0x0008);
			rt5610_write_mask(codec, 0x3c, 0x00c3, 0x00c3);
			rt5610_write_mask(codec, 0x3e, 0x0002, 0x0002);
			break;
	}

	return 0;
}

static int rt5610_vpcm_hw_prepare(struct snd_pcm_substream *substream, 
		struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	int stream = substream->stream;
	dbg("");

	switch (stream)
	{
		case SNDRV_PCM_STREAM_PLAYBACK:
			rt5610_write_mask(codec, 0x3c, 0x0430, 0x0430);
			rt5610_write_mask(codec, 0x3e, 0x3f00, 0x3f00);
			rt5610_write_mask(codec, 0x3a, 0x4000, 0x4000);
			rt5610_write_mask(codec, 0x02, 0x0000, 0x8080);
			rt5610_write_mask(codec, 0x04, 0x0000, 0x8080);
			break;
		case SNDRV_PCM_STREAM_CAPTURE:
			rt5610_write_mask(codec, 0x3a, 0x0008, 0x0008);
			rt5610_write_mask(codec, 0x3c, 0x0041, 0x0041);
			rt5610_write_mask(codec, 0x3e, 0x3f00, 0x3f00);
			break;
	}

	return 0;
}
#endif
/* PLL divisors */
struct _pll_div {
	u32 pll_in;
	u32 pll_out;
	u16 regvalue;
};

static const struct _pll_div codec_pll_div[] = {
	
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


static int rt5610_set_pll(struct snd_soc_codec *codec, 
		int pll_id, unsigned int freq_in, unsigned int freq_out)
{
	int ret = -EINVAL;
	int i;
	
	if (pll_id != RT5610_PLL_FR_MCLK)
		return -EINVAL;

	//rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD2, 0x0000,0x1000);	//disable PLL power	
	
	if (!freq_in || !freq_out) {

		return 0;
	}		

	for (i = 0; i < ARRAY_SIZE(codec_pll_div); i++) {
			
		if (codec_pll_div[i].pll_in == freq_in && codec_pll_div[i].pll_out == freq_out)
			{
				
				rt5610_write_mask(codec, RT5610_GEN_CTRL_REG2, 0x0000, 0x4000);			 	
			 	rt5610_write(codec,RT5610_PLL_CTRL,codec_pll_div[i].regvalue);//set PLL parameter 	
			 	rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD2, 0x1000,0x1000);	//enable PLL power	
				ret = 0;
			}
	}
	return ret;
}
static int rt5610_set_dai_pll(struct snd_soc_dai *codec_dai,
		int pll_id, unsigned int freq_in, unsigned int freq_out)
{
	int ret = -EINVAL;
	struct snd_soc_codec *codec = codec_dai->codec;
	struct rt5610_priv *rt5610 = codec->private_data;
	dbg("");

	ret = rt5610_set_pll(codec, pll_id, freq_in, freq_out);
	if (ret < 0) {
		err( "pll unmatched\n");
		return ret;
	}
	
	rt5610_write_mask(codec,RT5610_GEN_CTRL_REG1,0x8000,0x8000);//Codec sys-clock from PLL 

	rt5610->pll_id = pll_id;
	rt5610->pll_in = freq_in;
	rt5610->pll_out = freq_out;

	/* wait 10ms AC97 link frames for the link to stabilise */
	schedule_timeout_interruptible(msecs_to_jiffies(10));
	
	return ret;
}

static int rt5610_vpcm_set_dai_pll(struct snd_soc_dai *codec_dai,
		int pll_id, unsigned int freq_in, unsigned int freq_out)
{

	int ret = -EINVAL;
	struct snd_soc_codec *codec = codec_dai->codec;
	struct rt5610_priv *rt5610 = codec->private_data;
	dbg("");

	ret = rt5610_set_pll(codec, pll_id, freq_in, freq_out);
	if (ret < 0){
		err("pll unmatched\n");
		return ret;
	}
	
	rt5610_write_mask(codec, RT5610_VOICE_DAC_PCMCLK_CTRL1, 0x8000, 0xc000);         //voice sysclk from pll
	rt5610->pll_id = pll_id;
	rt5610->pll_in = freq_in;
	rt5610->pll_out = freq_out;
	return ret;
}


struct _coeff_div {
	u32 mclk;
	u32 rate;
	u16 fs;
	u16 regvalue1;
	u16 regvalue2;
};



static const struct _coeff_div coeff_div_vpcm[] = {
	/*8k*/
	{24576000, 8000, 384*8, 0x2153, 0x4013},
	/*16k*/
	{24576000, 16000, 384*4, 0x2123, 0x4013},
};


static int get_coeff_vpcm(int mclk, int rate)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(coeff_div_vpcm); i++) {
		if ((coeff_div_vpcm[i].rate == rate) && (coeff_div_vpcm[i].mclk == mclk))
			return i;
	}
	return -EINVAL;
}



static int rt5610_vpcm_set_dai_sysclk(struct snd_soc_dai *codec_dai,
		int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct rt5610_priv *rt5610 = codec->private_data;
	dbg("");

	rt5610->vpcm_sysclk= freq;
	return 0;
}


static int rt5610_vpcm_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	u16 iface = 0;

	iface |= 0x8000;				/*vopcm interace*/
	dbg("");

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK)
	{
		case SND_SOC_DAIFMT_CBM_CFM:
			break;
		case SND_SOC_DAIFMT_CBS_CFS:
			iface |= 0x4000;
			break;
		default:
			return -EINVAL;
			
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK)
	{
		case SND_SOC_DAIFMT_I2S:
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
			iface |= 0x0043;
			break;
		default:
			return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
		case SND_SOC_DAIFMT_NB_NF:
			break;
		case SND_SOC_DAIFMT_IB_NF:
			iface |= 0x0800;
			break;
		default:
			return -EINVAL;
	}
	
	rt5610_write(codec, RT5610_EXTEND_SDP_CTRL, iface);
	return 0;
}

static int rt5610_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct rt5610_priv *rt5610 = codec->private_data;
	struct snd_soc_dapm_widget *w;
	int stream = substream->stream;

	dbg("");
	if (stream == SNDRV_PCM_STREAM_CAPTURE) {
		list_for_each_entry(w, &codec->dapm_widgets, list)
		{
			if (!w->sname)
				continue;
			if (!strcmp(w->name, "Right ADC"))
				strcpy(w->sname, "Right HiFi Capture");
		}
		rt5610_write_mask(codec, 0x36, 0x0000, 0x0100);
	}

	rt5610_write_mask(codec, 0x3a, 0x0001, 0x0001);    /*power on  dac ref*/
	
	return 0;
}


static int rt5610_vpcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct rt5610_priv *rt5610 = codec->private_data;
	struct snd_soc_dapm_widget *w;
	int stream = substream->stream;
	u16 iface = rt5610_read(codec, RT5610_EXTEND_SDP_CTRL) & 0xfff3;
	int coeff = get_coeff_vpcm(rt5610->vpcm_sysclk, params_rate(params));
	
	dbg("");

	if (stream == SNDRV_PCM_STREAM_CAPTURE) {
		list_for_each_entry(w, &codec->dapm_widgets, list)
		{
			if (!w->sname)
				continue;
			if (!strcmp(w->name, "Right ADC"))
				strcpy(w->sname, "Right Voice Capture");
		}
		rt5610_write_mask(codec, 0x36, 0x0100, 0x0100);	
	}
	
	iface |= 0x8000;
	switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S16_LE:
			iface |= 0x0000;
			break;
		case SNDRV_PCM_FORMAT_S20_3LE:
			iface |= 0x0004;
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			iface |= 0x0008;
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			iface |= 0x000c;
			break;
	}

	rt5610_write(codec, RT5610_EXTEND_SDP_CTRL, iface);
	rt5610_write_mask(codec, 0x3a, 0x0801, 0x0801);    /*power on i2s and dac ref*/
	if (coeff >=0)
	{
		rt5610_write_mask(codec, RT5610_VOICE_DAC_PCMCLK_CTRL1, coeff_div_vpcm[coeff].regvalue1, 0x3fff);
		rt5610_write(codec, RT5610_VOICE_DAC_PCMCLK_CTRL2, coeff_div_vpcm[coeff].regvalue2);
	}
	else
	{
		err("cant find matched sysclk and rate config\n");
		return -EINVAL;
	}
	return 0;	
}

static int rt5610_pcm_shutdown(struct snd_pcm_substream *substream,
		struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	dbg("");

	#if !USE_DAPM_CONTROL
	rt5610_write_mask(codec, 0x02, 0x8080, 0x8080);
	rt5610_write_mask(codec, 0x04, 0x8080, 0x8080);
	rt5610_write_mask(codec, 0x08, 0x0080, 0x0080);
	rt5610_write(codec, 0x3a, 0x0002);
	rt5610_write(codec, 0x3c, 0x2000);
	rt5610_write(codec, 0x3e, 0x0000);		
	#endif

	return 0;
}

static int rt5610_vpcm_shutdown(struct snd_pcm_substream *substream, 
		struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	dbg("");

	rt5610_write(codec, 0x2e, 0x0000);
#if !USE_DAPM_CONTROL
	rt5610_write_mask(codec, 0x02, 0x8080, 0x8080);
	rt5610_write_mask(codec, 0x04, 0x8080, 0x8080);
	rt5610_write_mask(codec, 0x08, 0x0080, 0x0080);
	rt5610_write(codec, 0x3a, 0x0002);
	rt5610_write(codec, 0x3c, 0x2000);
	rt5610_write(codec, 0x3e, 0x0000);		
#endif
	return 0;
}
#if !USE_DAPM_CONTROL
static int rt5610_set_bias_level(struct snd_soc_codec *codec, 
			enum snd_soc_bias_level level)
{
	switch (level) {
		case SND_SOC_BIAS_ON:
			rt5610_write_mask(codec, 0x50, GPIO_3, GPIO_3); //enable sticky
			rt5610_write_mask(codec, 0x52, GPIO_3, GPIO_3); //set as wake up	
		case SND_SOC_BIAS_PREPARE:
			break;
		case SND_SOC_BIAS_STANDBY:
		case SND_SOC_BIAS_OFF:
			rt5610_write_mask(codec, 0x02, 0x8080, 0x8080);
			rt5610_write_mask(codec, 0x04, 0x8080, 0x8080);
			rt5610_write_mask(codec, 0x08, 0x0080, 0x0080);
			rt5610_write(codec, 0x3a, 0x0002);
			rt5610_write(codec, 0x3c, 0x2000);
			rt5610_write(codec, 0x3e, 0x0000);			
			break;		
	}
	codec->bias_level = level;
	return 0;
}
#else
static int rt5610_set_bias_level(struct snd_soc_codec *codec, 
			enum snd_soc_bias_level level)
{
	switch (level) {
		case SND_SOC_BIAS_ON:
		case SND_SOC_BIAS_PREPARE:
			break;
		case SND_SOC_BIAS_STANDBY:
		case SND_SOC_BIAS_OFF:
			rt5610_write_mask(codec, 0x3a, 0x0000, 0x0801);
			rt5610_write_mask(codec,RT5610_PWR_MANAG_ADD2, 0x0000,0x1000);	//disable PLL power
			break;		
	}
	codec->bias_level = level;
	return 0;
}
#endif


#define RT5610_HIFI_RATES SNDRV_PCM_RATE_48000
#define RT5610_VOICE_RATES (SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_8000)


#define RT5610_HIFI_FORMATS (SNDRV_PCM_FMTBIT_S16_LE)
#define RT5610_VOICE_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
	SNDRV_PCM_FMTBIT_S24_LE)

static struct snd_soc_dai_ops rt5610_hifi_ops = {
	.hw_params = rt5610_pcm_hw_params,
	.set_pll = rt5610_set_dai_pll,
	.shutdown = rt5610_pcm_shutdown,
#if !USE_DAPM_CONTROL 
	.prepare = rt5610_pcm_hw_prepare,
#endif
};

static struct snd_soc_dai_ops rt5610_pcm_ops = {
	.hw_params = rt5610_vpcm_hw_params,
	.set_fmt = rt5610_vpcm_set_dai_fmt, 
	.set_sysclk = rt5610_vpcm_set_dai_sysclk,
	.set_pll = rt5610_vpcm_set_dai_pll,
	.shutdown = rt5610_vpcm_shutdown,
#if !USE_DAPM_CONTROL 
	.prepare = rt5610_vpcm_hw_prepare,
#endif
};
struct snd_soc_dai rt5610_dai[2] = { 
	
	{
		.name = "RT5610 HiFi",
		.playback = {
			.stream_name = "HiFi Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RT5610_HIFI_RATES,
			.formats = RT5610_HIFI_FORMATS,},
		.capture = {
			.stream_name = "HiFi Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RT5610_HIFI_RATES,
			.formats = RT5610_HIFI_FORMATS,},
		.ops = &rt5610_hifi_ops,
	},

	{
		.name = "RT5610 Voice",		
		.playback = {
			.stream_name = "Voice Playback",
			.channels_min = 1,
			.channels_max =2,
			.rates = RT5610_VOICE_RATES,
			.formats = RT5610_VOICE_FORMATS
		},
		.capture = {
			.stream_name = "Voice Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RT5610_VOICE_RATES,
			.formats = RT5610_VOICE_FORMATS,
		},
		.ops = &rt5610_pcm_ops,
	},	
};


EXPORT_SYMBOL_GPL(rt5610_dai);

//Power On SPK Amplifeir, this is just for Avenger-Dove-RT5630 Amplifier control.
//The Apm enable pin is connected to ALC5610-GPIO1 because of no enough DOVE GPIO numbers can be used.
//Remove the code if the board is not Avenger-Dove
int spk_amplifier_enable(bool enable)
{
	if(codec)
	{
		dbg("enable=%d\n",enable);

		rt5610_write_mask(codec, 0x4c, 0x0000, 0x0002); //Config GPIO1 as output

		if (enable && !hs_det)
			 rt5610_write_mask(codec, 0x5c, 0x0002, 0x0002); //ON
		else
			 rt5610_write_mask(codec, 0x5c, 0x0000, 0x0002); //OFF
		spk_enable=enable;
	}	
	else
		printk(KERN_INFO"Amp controller from AUDIO GPIO is not supported in this platform!\n");
	return 0;

}
EXPORT_SYMBOL_GPL(spk_amplifier_enable);

static int rt5610_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;

	rt5610_codec_resumed=0;

	/* we only need to suspend if we are a valid card */
	if(!codec->card)
		return 0;
		
	rt5610_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	return 0;
}
static int rt5610_resume(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;
	struct rt5610_priv *rt5610 = codec->private_data;
	int ret, i;
	u16 *cache = codec->reg_cache;

	rt5610_reset(codec, 0);
	ret = rt5610_reset(codec, 1);
	if (ret < 0) {
		err("could not reset AC97 codec\n");
		return ret;
	}

	rt5610_write(codec, RT5610_PD_CTRL_STAT, 0); 
	rt5610_set_bias_level(codec, SND_SOC_BIAS_STANDBY);\
		
	/* do we need to re-start the PLL ? */
	if (rt5610->pll_out)
		rt5610_set_pll(codec, rt5610->pll_id, rt5610->pll_in, rt5610->pll_out);

	/*only synchronise the codec if warm reset failed*/
	if (ret == 0) {
		for (i = 2; i < ARRAY_SIZE(rt5610_reg) << 1; i += 2) {
			if (i > 0x66)
				break;
			soc_ac97_ops.write(codec->ac97, i, cache[i >> 1]);
		}		
	}

	if (codec->suspend_bias_level == SND_SOC_BIAS_ON)
		rt5610_set_bias_level(codec, SND_SOC_BIAS_ON);
	rt5610_codec_resumed=1;
	return ret;
	
}


/*
 * initialise the RT5610 driver
 * register the mixer and dsp interfaces with the kernel
 */
static void rt5610_hsdet_irq_worker(struct work_struct *work)
{
	
	u16 status, pol;
	dbg("rt5610_hsdet_irq_worker\n");
	
	status=rt5610_read(codec, 0x54);
	pol=rt5610_read(codec, 0x4e);

	dbg("GPIO3(pol,status)=%x,%x\n",(pol & GPIO_3)>>3,(status & GPIO_3)>>3);

	if (!(GPIO_3 & status))
	{
		dbg("AC97 codec IRQ: Not Headset, quit.");
		enable_irq(hs_irq);

		return ;
	}

	if (GPIO_3 & pol) //current state: phone jack-in
	{
		dbg("phone jack-in\n");
		hs_det=1;
		rt5610_write_mask(codec, 0x4e, 0, GPIO_3); //Low active, to detect phone jack-in
		if (spk_enable)
			rt5610_write_mask(codec, 0x5c, 0, 0x0002); //SPK CLASS D AMP OFF

	}
	else //current state: phone jack-out
	{
		dbg("phone jack-out\n");
		hs_det=0;
		rt5610_write_mask(codec, 0x4e, GPIO_3, GPIO_3); //High active, , to detect phone jack-out
		if (spk_enable)
			rt5610_write_mask(codec, 0x5c, 0x0002, 0x0002); //SPK CLASS D AMP ON
	}
	rt5610_write_mask(codec, 0x54, 0, GPIO_2|GPIO_3); //clear status

	enable_irq(hs_irq);


}

static irqreturn_t  rt5610_hsdet_int(int irq, void *dev_id)
{
	dbg("enter rt5610_hsdet_int handle.\n");

	if (!work_pending(&hsdet_event_work)) {
		disable_irq_nosync(hs_irq);
		queue_work(hsdet_workq, &hsdet_event_work);
	}
	return IRQ_HANDLED;
}

static int rt5610_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	int ret;

	codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (codec == NULL)
		return -ENOMEM;
	mutex_init(&codec->mutex);
	socdev->card->codec = codec;
	codec->reg_cache = kmemdup(rt5610_reg, sizeof(rt5610_reg), GFP_KERNEL);
	if (codec->reg_cache == NULL) {
		ret = -ENOMEM;
		goto cache_err;
	}
	codec->reg_cache_size = sizeof(rt5610_reg);
	codec->reg_cache_step = 2;

	codec->private_data = kzalloc(sizeof(struct rt5610_priv), GFP_KERNEL);
	if (codec->private_data == NULL) {
		ret = -ENOMEM;
		goto priv_err;
	}

	codec->name = "RT5610";
	codec->owner = THIS_MODULE;
	codec->dai = rt5610_dai;
	codec->num_dai = ARRAY_SIZE(rt5610_dai);
	codec->read = rt5610_read;
	codec->write = rt5610_write;
	codec->set_bias_level = rt5610_set_bias_level;
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	ret = snd_soc_new_ac97_codec(codec, &soc_ac97_ops, 0);
	if (ret < 0)
		goto codec_err;

	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0)
		goto pcm_err;

#ifndef TOUCHSCREEN_RT5611
	// Suppose ALC5611 has been RESET in TSC Driver Init
	// It does not RESET again.
	rt5610_reset(codec, 0);
	ret = rt5610_reset(codec, 1);
	if (ret < 0) {
		err("FAIL to reset rt5610\n");
		goto reset_err;
	}
#endif

	rt5610_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	rt5610_write(codec, 0x26, 0x0000);
	rt5610_write(codec, 0x3a, 0x0002);            //main bias
	rt5610_write(codec, 0x3c, 0x2000);            //vref
	rt5610_reg_init(codec);

	rt5610_add_controls(codec);
	#if USE_DAPM_CONTROL
	rt5610_add_widgets(codec);
	#endif
	ret = snd_soc_init_card(socdev);
	if (ret < 0)
		goto reset_err;
//HSDET INT{	
	rt5610_write_mask(codec, 0x54, 0, GPIO_3); //clear status	
	rt5610_write_mask(codec, 0x4e, GPIO_3, GPIO_3); //High active, for headset_det
	rt5610_write_mask(codec, 0x4c, GPIO_3, GPIO_3); // GPIO3 as input
	rt5610_write_mask(codec, 0x50, GPIO_3, GPIO_3); //enable sticky
	
	hsdet_workq = create_singlethread_workqueue("rt5610_snd");
	if (hsdet_workq == NULL) {
		err("Failed to create workqueue\n");
		return -1;
	}
	INIT_WORK(&hsdet_event_work, rt5610_hsdet_irq_worker);
	
	hs_irq=platform_get_irq(pdev,0);
	dbg(KERN_ERR "rt5610 irq=0x%x\n",hs_irq);
	if (request_irq(hs_irq, rt5610_hsdet_int, IRQF_SHARED,
		"rt5610_hsdet", socdev))	{
		err("Failed to register rt5610_headset_det interrupt");
		return -1;
	}
//}HSDET INT	
	dbg("rt5610: initial ok\n");
	return 0;
	
reset_err:
	snd_soc_free_pcms(socdev);
	
pcm_err:
	snd_soc_free_ac97_codec(codec);
	
codec_err:
	kfree(codec->private_data);
	
priv_err:
	kfree(codec->reg_cache);
	
cache_err:
	kfree(socdev->card->codec);
	socdev->card->codec = NULL;
	return ret;
}

static int rt5610_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;

	if (codec == NULL)
		return 0;
	
	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);
	snd_soc_free_ac97_codec(codec);
	kfree(codec->private_data);
	kfree(codec->reg_cache);
	kfree(codec);
	return 0;
}

struct snd_soc_codec_device soc_codec_dev_rt5610 = {
	.probe = 	rt5610_probe,
	.remove = 	rt5610_remove,
	.suspend = 	rt5610_suspend,
	.resume =	rt5610_resume,
};

EXPORT_SYMBOL_GPL(soc_codec_dev_rt5610);

static int __devinit rt5610_dev_probe(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(rt5610_dai); i++)
		rt5610_dai[i].dev = &pdev->dev;

	return snd_soc_register_dais(rt5610_dai, ARRAY_SIZE(rt5610_dai));
}

static int __devexit rt5610_dev_remove(struct platform_device *pdev)
{
	snd_soc_unregister_dais(rt5610_dai, ARRAY_SIZE(rt5610_dai));

	return 0;
}

static struct platform_driver rt5610_driver = {
	.probe		= rt5610_dev_probe,
	.remove		= __devexit_p(rt5610_dev_remove),
	.driver		= {
		.name	= "rt5610-codec",
		.owner	= THIS_MODULE,
	},
};

static int __init rt5610_modinit(void)
{
	return platform_driver_register(&rt5610_driver);
}

static void __exit rt5610_exit(void)
{
	platform_driver_unregister(&rt5610_driver);
}

module_init(rt5610_modinit);
module_exit(rt5610_exit);
MODULE_LICENSE("GPL");

