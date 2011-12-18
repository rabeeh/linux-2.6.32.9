#ifndef _RT5610_H
#define _RT5610_H


//Index of Codec Register definition
#define RT5610_RESET						0X00			//RESET CODEC TO DEFAULT
#define RT5610_SPK_OUT_VOL					0X02			//SPEAKER OUT VOLUME
#define RT5610_HP_OUT_VOL					0X04			//HEADPHONE OUTPUT VOLUME
#define RT5610_PHONEIN_MONO_OUT_VOL			0X08			//PHONE INPUT/MONO OUTPUT VOLUME
#define RT5610_LINE_IN_VOL					0X0A			//LINE IN VOLUME
#define RT5610_STEREO_DAC_VOL				0X0C			//STEREO DAC VOLUME
#define RT5610_MIC_VOL						0X0E			//MICROPHONE VOLUME
#define RT5610_MIC_ROUTING_CTRL				0X10			//MIC ROUTING CONTROL
#define RT5610_ADC_REC_GAIN					0X12			//ADC RECORD GAIN
#define RT5610_ADC_REC_MIXER				0X14			//ADC RECORD MIXER CONTROL
#define RT5610_VOICE_DAC_OUT_VOL			0X18			//VOICE DAC OUTPUT VOLUME
#define RT5610_OUTPUT_MIXER_CTRL			0X1C			//OUTPUT MIXER CONTROL
#define RT5610_MIC_CTRL						0X22			//MICROPHONE CONTROL
#define RT5610_PD_CTRL_STAT					0X26			//POWER DOWN CONTROL/STATUS
#define RT5610_TONE_CTRL                               0x2a                       //tone control
#define RT5610_DAC_DPE_RATE                         0x2c                      //ac97's dac/dpe rate
#define RT5610_ADC_RATE                                 0x32                      //ac97's adc rate
//#define	RT5610_MAIN_SDP_CTRL				0X34			//MAIN SERIAL DATA PORT CONTROL(STEREO I2S)

#define RT5610_EXTEND_SDP_CTRL				0X36			//EXTEND SERIAL DATA PORT CONTROL(VOICE I2S/PCM)
#define	RT5610_PWR_MANAG_ADD1				0X3A			//POWER MANAGMENT ADDITION 1
#define RT5610_PWR_MANAG_ADD2				0X3C			//POWER MANAGMENT ADDITION 2
#define RT5610_PWR_MANAG_ADD3				0X3E			//POWER MANAGMENT ADDITION 3
#define RT5610_GEN_CTRL_REG1				0X40			//GENERAL PURPOSE CONTROL REGISTER 1
#define	RT5610_GEN_CTRL_REG2				0X42			//GENERAL PURPOSE CONTROL REGISTER 2
#define RT5610_PLL_CTRL						0X44			//PLL CONTROL
#define RT5610_GPIO_PIN_CONFIG				0X4C			//GPIO PIN CONFIGURATION
#define RT5610_GPIO_PIN_POLARITY			0X4E			//GPIO PIN POLARITY	
#define RT5610_GPIO_PIN_STICKY				0X50			//GPIO PIN STICKY	
#define RT5610_GPIO_PIN_WAKEUP				0X52			//GPIO PIN WAKE UP
#define RT5610_GPIO_PIN_STATUS				0X54			//GPIO PIN STATUS
#define RT5610_GPIO_PIN_SHARING				0X56			//GPIO PIN SHARING
#define	RT5610_OVER_TEMP_CURR_STATUS		0X58			//OVER TEMPERATURE AND CURRENT STATUS
#define RT5610_GPIO_OUT_CTRL				0X5C			//GPIO OUTPUT PIN CONTRL
#define RT5610_MISC_CTRL					0X5E			//MISC CONTROL
//#define	RT5610_STEREO_DAC_CLK_CTRL1			0X60			//STEREO DAC CLOCK CONTROL 1
//#define RT5610_STEREO_DAC_CLK_CTRL2			0X62			//STEREO DAC CLOCK CONTROL 2
#define RT5610_VOICE_DAC_PCMCLK_CTRL1		0X64			//VOICE/PCM DAC CLOCK CONTROL 1
#define RT5610_VOICE_DAC_PCMCLK_CTRL2		0X66			//VOICE/PCM DAC CLOCK CONTROL 2
#define RT5610_PSEDUEO_SPATIAL_CTRL			0X68			//PSEDUEO STEREO /SPATIAL EFFECT BLOCK CONTROL
#define RT5610_INDEX_ADDRESS				0X6A			//INDEX ADDRESS
#define RT5610_INDEX_DATA					0X6C			//INDEX DATA 
#define RT5610_EQ_STATUS					0X6E			//EQ STATUS
#define RT5610_TOUCH_PANEL_STATUS              0x78                    //touche panel indiction
#define RT5610_VENDOR_ID1	  		    	0x7C			//VENDOR ID1
#define RT5610_VENDOR_ID2	  		    	0x7E			//VENDOR ID2


//*************************************************************************************************
//Bit define of Codec Register
//*************************************************************************************************
//global definition
#define RT_L_MUTE						(0x1<<15)		//Mute Left Control
#define RT_L_ZC							(0x1<<14)		//Mute Left Zero-Cross Detector Control
#define RT_R_MUTE						(0x1<<7)		//Mute Right Control
#define RT_R_ZC							(0x1<<6)		//Mute Right Zero-Cross Detector Control
#define RT_M_HP_MIXER					(0x1<<15)		//Mute source to HP Mixer
#define RT_M_SPK_MIXER					(0x1<<14)		//Mute source to Speaker Mixer
#define RT_M_MONO_MIXER					(0x1<<13)		//Mute source to Mono Mixer

//Phone Input/MONO Output Volume(0x08)
#define M_PHONEIN_TO_HP_MIXER			(0x1<<15)		//Mute Phone In volume to HP mixer
#define M_PHONEIN_TO_SPK_MIXER			(0x1<<14)		//Mute Phone In volume to speaker mixer
#define M_MONO_OUT_VOL					(0x1<<7)		//Mute Mono output volume


//Mic Routing Control(0x10)
#define M_MIC1_TO_HP_MIXER				(0x1<<15)		//Mute MIC1 to HP mixer
#define M_MIC1_TO_SPK_MIXER				(0x1<<14)		//Mute MiC1 to SPK mixer
#define M_MIC1_TO_MONO_MIXER			(0x1<<13)		//Mute MIC1 to MONO mixer
#define MIC1_DIFF_INPUT_CTRL			(0x1<<12)		//MIC1 different input control
#define M_MIC2_TO_HP_MIXER				(0x1<<7)		//Mute MIC2 to HP mixer
#define M_MIC2_TO_SPK_MIXER				(0x1<<6)		//Mute MiC2 to SPK mixer
#define M_MIC2_TO_MONO_MIXER			(0x1<<5)		//Mute MIC2 to MONO mixer
#define MIC2_DIFF_INPUT_CTRL			(0x1<<4)		//MIC2 different input control

//ADC Record Gain(0x12)
#define M_ADC_L_TO_HP_MIXER				(0x1<<15)		//Mute left of ADC to HP Mixer
#define M_ADC_R_TO_HP_MIXER				(0x1<<14)		//Mute right of ADC to HP Mixer
#define M_ADC_L_TO_MONO_MIXER			(0x1<<13)		//Mute left of ADC to MONO Mixer
#define M_ADC_R_TO_MONO_MIXER			(0x1<<12)		//Mute right of ADC to MONO Mixer
#define ADC_L_GAIN_MASK					(0x1f<<7)		//ADC Record Gain Left channel Mask
#define ADC_L_ZC_DET					(0x1<<6)		//ADC Zero-Cross Detector Control
#define ADC_R_ZC_DET					(0x1<<5)		//ADC Zero-Cross Detector Control
#define ADC_R_GAIN_MASK					(0x1f<<0)		//ADC Record Gain Right channel Mask

//Voice DAC Output Volume(0x18)
#define M_V_DAC_TO_HP_MIXER				(0x1<<15)
#define M_V_DAC_TO_SPK_MIXER			(0x1<<14)
#define M_V_DAC_TO_MONO_MIXER			(0x1<<13)

//Output Mixer Control(0x1C)
#define	SPKL_INPUT_SEL_MASK				(0x3<<14)
#define	SPKL_INPUT_SEL_MONO				(0x3<<14)
#define	SPKL_INPUT_SEL_SPK				(0x2<<14)
#define	SPKL_INPUT_SEL_HPL				(0x1<<14)
#define	SPKL_INPUT_SEL_VMID				(0x0<<14)

#define SPK_OUT_SEL_CLASS_D				(0x1<<13)
#define SPK_OUT_SEL_CLASS_AB			(0x0<<13)

#define	SPKR_INPUT_SEL_MASK				(0x3<<11)
#define	SPKR_INPUT_SEL_MONO_MIXER		(0x3<<11)
#define	SPKR_INPUT_SEL_SPK_MIXER		(0x2<<11)
#define	SPKR_INPUT_SEL_HPR_MIXER		(0x1<<11)
#define	SPKR_INPUT_SEL_VMID				(0x0<<11)

#define HPL_INPUT_SEL_HPL_MIXER			(0x1<<9)
#define HPR_INPUT_SEL_HPR_MIXER			(0x1<<8)

#define MONO_INPUT_SEL_MASK				(0x3<<6)
#define MONO_INPUT_SEL_MONO_MIXER		(0x3<<6)
#define MONO_INPUT_SEL_SPK_MIXER		(0x2<<6)
#define MONO_INPUT_SEL_HP_MIXER			(0x1<<6)
#define MONO_INPUT_SEL_VMID				(0x0<<6)

//Micphone Control define(0x22)
#define MIC1		1
#define MIC2		2
#define MIC_BIAS_90_PRECNET_AVDD	1
#define	MIC_BIAS_75_PRECNET_AVDD	2

#define MIC1_BOOST_CONTROL_MASK		(0x3<<10)
#define MIC1_BOOST_CONTROL_BYPASS	(0x0<<10)
#define MIC1_BOOST_CONTROL_20DB		(0x1<<10)
#define MIC1_BOOST_CONTROL_30DB		(0x2<<10)
#define MIC1_BOOST_CONTROL_40DB		(0x3<<10)

#define MIC2_BOOST_CONTROL_MASK		(0x3<<8)
#define MIC2_BOOST_CONTROL_BYPASS	(0x0<<8)
#define MIC2_BOOST_CONTROL_20DB		(0x1<<8)
#define MIC2_BOOST_CONTROL_30DB		(0x2<<8)
#define MIC2_BOOST_CONTROL_40DB		(0x3<<8)

#define MIC1_BIAS_VOLT_CTRL_MASK	(0x1<<5)
#define MIC1_BIAS_VOLT_CTRL_90P		(0x0<<5)
#define MIC1_BIAS_VOLT_CTRL_75P		(0x1<<5)

#define MIC2_BIAS_VOLT_CTRL_MASK	(0x1<<4)
#define MIC2_BIAS_VOLT_CTRL_90P		(0x0<<4)
#define MIC2_BIAS_VOLT_CTRL_75P		(0x1<<4)

//PowerDown control of register(0x26)
//power management bits
#define RT_PWR_PR7					(0x1<<15)	//write this bit to power down the Speaker Amplifier
#define RT_PWR_PR6					(0x1<<14)	//write this bit to power down the Headphone Out and MonoOut
#define RT_PWR_PR5					(0x1<<13)	//write this bit to power down the internal clock(without PLL)
#define RT_PWR_PR3					(0x1<<11)	//write this bit to power down the mixer(vref/vrefout out off)
#define RT_PWR_PR2					(0x1<<10)	//write this bit to power down the mixer(vref/vrefout still on)
#define RT_PWR_PR1					(0x1<<9) 	//write this bit to power down the dac
#define RT_PWR_PR0					(0x1<<8) 	//write this bit to power down the adc
#define RT_PWR_REF					(0x1<<3)	//read only
#define RT_PWR_ANL					(0x1<<2)	//read only	
#define RT_PWR_DAC					(0x1<<1)	//read only
#define RT_PWR_ADC					(0x1)		//read only




//Extend Serial Data Port Control(0x36)
#define EXT_I2S_FUNC_ENABLE			(0x1<<15)		//Enable PCM interface on GPIO 1,3,4,5  0:GPIO function,1:Voice PCM interface
#define EXT_I2S_MODE_SEL			(0x1<<14)		//0:Master	,1:Slave
#define EXT_I2S_VOICE_ADC_EN		(0x1<<8)		//ADC_L=Stereo, ADC_R=Voice
#define EXT_I2S_BCLK_POLARITY		(0x1<<7)		//0:Normal 1:Invert
#define EXT_I2S_PCM_MODE			(0x1<<6)		//PCM    	0:mode A				,1:mode B 
												 	//Non PCM	0:Normal SADLRCK/SDALRCK,1:Invert SADLRCK/SDALRCK 
//Data Length Slection
#define EXT_I2S_DL_MASK				(0x3<<2)		//Extend i2s Data Length mask	
#define EXT_I2S_DL_32				(0x3<<2)		//32 bits
#define	EXT_I2S_DL_24				(0x2<<2)		//24 bits
#define EXT_I2S_DL_20				(0x1<<2)		//20 bits
#define EXT_I2S_DL_16				(0x0<<2)		//16 bits

//Voice Data Format
#define EXT_I2S_DF_MASK				(0x3)			//Extend i2s Data Format mask
#define EXT_I2S_DF_I2S				(0x0)			//I2S FORMAT 
#define EXT_I2S_DF_RIGHT			(0x1)			//RIGHT JUSTIFIED format
#define	EXT_I2S_DF_LEFT				(0x2)			//LEFT JUSTIFIED  format
#define EXT_I2S_DF_PCM				(0x3)			//PCM format

//Power managment addition 1 (0x3A),0:Disable,1:Enable
#define PWR_HI_R_LOAD_MONO			(0x1<<15)
#define PWR_HI_R_LOAD_HP			(0x1<<14)	
#define PWR_ZC_DET_PD				(0x1<<13)
#define PWR_MAIN_I2S				(0x1<<11)	
#define	PWR_MIC_BIAS1_DET			(0x1<<5)
#define	PWR_MIC_BIAS2_DET			(0x1<<4)
#define PWR_MIC_BIAS1				(0x1<<3)	
#define PWR_MIC_BIAS2				(0x1<<2)	
#define PWR_MAIN_BIAS				(0x1<<1)
#define PWR_DAC_REF					(0x1)


//Power managment addition 2(0x3C),0:Disable,1:Enable
#define EN_THREMAL_SHUTDOWN			(0x1<<15)
#define PWR_CLASS_AB				(0x1<<14)
#define PWR_MIXER_VREF				(0x1<<13)
#define PWR_PLL						(0x1<<12)
#define PWR_VOICE_CLOCK				(0x1<<10)
#define PWR_L_DAC_CLK				(0x1<<9)
#define PWR_R_DAC_CLK				(0x1<<8)
#define PWR_L_ADC_CLK_GAIN			(0x1<<7)
#define PWR_R_ADC_CLK_GAIN			(0x1<<6)
#define PWR_L_HP_MIXER				(0x1<<5)
#define PWR_R_HP_MIXER				(0x1<<4)
#define PWR_SPK_MIXER				(0x1<<3)
#define PWR_MONO_MIXER				(0x1<<2)
#define PWR_L_ADC_REC_MIXER			(0x1<<1)
#define PWR_R_ADC_REC_MIXER			(0x1)


//Power managment addition 3(0x3E),0:Disable,1:Enable
#define PWR_MONO_VOL				(0x1<<14)
#define PWR_SPK_LN_OUT				(0x1<<13)
#define PWR_SPK_RN_OUT				(0x1<<12)
#define PWR_HP_L_OUT				(0x1<<11)
#define PWR_HP_R_OUT				(0x1<<10)
#define PWR_SPK_L_OUT				(0x1<<9)
#define PWR_SPK_R_OUT				(0x1<<8)
#define PWR_LINE_IN_L				(0x1<<7)
#define PWR_LINE_IN_R				(0x1<<6)
#define PWR_PHONE_MIXER				(0x1<<5)
#define PWR_PHONE_VOL				(0x1<<4)
#define PWR_MIC1_VOL_CTRL			(0x1<<3)
#define PWR_MIC2_VOL_CTRL			(0x1<<2)
#define PWR_MIC1_BOOST				(0x1<<1)
#define PWR_MIC2_BOOST				(0x1)

//General Purpose Control Register 1(0x40)
#define GP_CLK_FROM_PLL					(0x1<<15)	//Clock source from PLL output
#define GP_CLK_FROM_MCLK				(0x0<<15)	//Clock source from MCLK output

#define GP_HP_AMP_CTRL_MASK				(0x3<<8)
#define GP_HP_AMP_CTRL_RATIO_100		(0x0<<8)		//1.00 Vdd
#define GP_HP_AMP_CTRL_RATIO_125		(0x1<<8)		//1.25 Vdd
#define GP_HP_AMP_CTRL_RATIO_150		(0x2<<8)		//1.50 Vdd

#define GP_SPK_D_AMP_CTRL_MASK			(0x3<<6)
#define GP_SPK_D_AMP_CTRL_RATIO_175		(0x0<<6)		//1.75 Vdd
#define GP_SPK_D_AMP_CTRL_RATIO_150		(0x1<<6)		//1.50 Vdd	
#define GP_SPK_D_AMP_CTRL_RATIO_125		(0x2<<6)		//1.25 Vdd
#define GP_SPK_D_AMP_CTRL_RATIO_100		(0x3<<6)		//1.00 Vdd

#define GP_SPK_AB_AMP_CTRL_MASK			(0x7<<3)
#define GP_SPK_AB_AMP_CTRL_RATIO_225	(0x0<<3)		//2.25 Vdd
#define GP_SPK_AB_AMP_CTRL_RATIO_200	(0x1<<3)		//2.00 Vdd
#define GP_SPK_AB_AMP_CTRL_RATIO_175	(0x2<<3)		//1.75 Vdd
#define GP_SPK_AB_AMP_CTRL_RATIO_150	(0x3<<3)		//1.50 Vdd
#define GP_SPK_AB_AMP_CTRL_RATIO_125	(0x4<<3)		//1.25 Vdd	
#define GP_SPK_AB_AMP_CTRL_RATIO_100	(0x5<<3)		//1.00 Vdd

//General Purpose Control Register 2(0x42)
#define GP2_VOICE_TO_DIG_PATH_EN		(0x1<<15)
#define GP2_CLASS_AB_SEL_DIFF			(0x0<<13)		//Differential Mode of Class AB
#define GP2_CLASS_D_SEL_SINGLE			(0x1<<13)		//Single End mode of Class AB
#define GP2_PLL_PRE_DIV_1				(0x0<<0)		//PLL pre-Divider 1		
#define GP2_PLL_PRE_DIV_2				(0x1<<0)		//PLL pre-Divider 2

//PLL Control(0x44)
#define PLL_M_CODE_MASK				0xF				//PLL M code mask
#define PLL_K_CODE_MASK				(0x7<<4)		//PLL K code mask
#define PLL_BYPASS_N				(0x1<<7)		//bypass PLL N code
#define PLL_N_CODE_MASK				(0xFF<<8)		//PLL N code mask

#define PLL_CTRL_M_VAL(m)		((m)&0xf)
#define PLL_CTRL_K_VAL(k)		(((k)&0x7)<<4)
#define PLL_CTRL_N_VAL(n)		(((n)&0xff)<<8)


//GPIO Pin Configuration(0x4C)
#define GPIO_1						(0x1<<1)
#define	GPIO_2						(0x1<<2)
#define	GPIO_3						(0x1<<3)
#define GPIO_4						(0x1<<4)
#define GPIO_5						(0x1<<5)


////INTERRUPT CONTROL(0x5E)
#define DISABLE_FAST_VREG			(0x1<<15)

#define AVC_TARTGET_SEL_MASK		(0x3<<12)
#define	AVC_TARTGET_SEL_NONE		(0x0<<12)
#define	AVC_TARTGET_SEL_R 			(0x1<<12)
#define	AVC_TARTGET_SEL_L			(0x2<<12)
#define	AVC_TARTGET_SEL_BOTH		(0x3<<12)




//Voice DAC PCM Clock Control 1(0x64)
#define VOICE_MCLK_SEL_MASK			(0x1<<15)
#define VOICE_MCLK_SEL_MCLK_INPUT	(0x0<<15)
#define VOICE_MCLK_SEL_PLL_OUTPUT	(0x1<<15)

#define VOICE_SYSCLK_SEL_MASK		(0x1<<14)
#define VOICE_SYSCLK_SEL_MCLK		(0x0<<14)
#define VOICE_SYSCLK_SEL_EXTCLK		(0x1<<14)

#define VOICE_WCLK_DIV_MASK			(0x1<<13)
#define VOICE_WCLK_DIV_32			(0x0<<13)
#define VOICE_WCLK_DIV_64			(0x1<<13)

#define VOICE_EXTCLK_OUT_DIV_MASK   (0x7<<8)
#define VOICE_EXTCLK_OUT_DIV_1		(0x0<<8)
#define VOICE_EXTCLK_OUT_DIV_2   	(0x1<<8)
#define VOICE_EXTCLK_OUT_DIV_4   	(0x2<<8)
#define VOICE_EXTCLK_OUT_DIV_8   	(0x3<<8)
#define VOICE_EXTCLK_OUT_DIV_16   	(0x4<<8)

#define VOICE_SCLK_DIV1_MASK		(0xF<<4)
#define VOICE_SCLK_DIV1_1			(0x0<<4)
#define VOICE_SCLK_DIV1_2			(0x1<<4)
#define VOICE_SCLK_DIV1_3			(0x2<<4)
#define VOICE_SCLK_DIV1_4			(0x3<<4)
#define VOICE_SCLK_DIV1_5			(0x4<<4)
#define VOICE_SCLK_DIV1_6			(0x5<<4)
#define VOICE_SCLK_DIV1_7			(0x6<<4)
#define VOICE_SCLK_DIV1_8			(0x7<<4)
#define VOICE_SCLK_DIV1_9			(0x8<<4)
#define VOICE_SCLK_DIV1_10			(0x9<<4)
#define VOICE_SCLK_DIV1_11			(0xA<<4)
#define VOICE_SCLK_DIV1_12			(0xB<<4)
#define VOICE_SCLK_DIV1_13			(0xC<<4)
#define VOICE_SCLK_DIV1_14			(0xD<<4)
#define VOICE_SCLK_DIV1_15			(0xE<<4)
#define VOICE_SCLK_DIV1_16			(0xF<<4)

#define VOICE_SCLK_DIV2_MASK		(0x7)
#define VOICE_SCLK_DIV2_2			(0x0)
#define VOICE_SCLK_DIV2_4			(0x1)
#define VOICE_SCLK_DIV2_8			(0x2)
#define VOICE_SCLK_DIV2_16			(0x3)
#define VOICE_SCLK_DIV2_32			(0x4)

//Voice DAC PCM Clock Control 2(0x66)
#define VOICE_CLK_FILTER_SLAVE_DIV_MASK (0x1<<15)
#define VOICE_CLK_FILTER_SLAVE_DIV_1 	(0x0<<15)
#define VOICE_CLK_FILTER_SLAVE_DIV_2	(0x1<<15)	

#define VOICE_FILTER_CLK_F_MASK		(0x1<<14)
#define VOICE_FILTER_CLK_F_MCLK		(0x0<<14)
#define VOICE_FILTER_CLK_F_VBCLK	(0X1<<14)

#define VOICE_AD_DA_FILTER_SEL_MASK  (0x1<<13)
#define VOICE_AD_DA_FILTER_SEL_128X	(0x0<<13)
#define VOICE_AD_DA_FILTER_SEL_64X	(0x1<<13)

#define VOICE_CLK_FILTER_DIV1_MASK	(0xF<<4)
#define VOICE_CLK_FILTER_DIV1_1		(0x0<<4)
#define VOICE_CLK_FILTER_DIV1_2		(0x1<<4)
#define VOICE_CLK_FILTER_DIV1_3		(0x2<<4)
#define VOICE_CLK_FILTER_DIV1_4		(0x3<<4)
#define VOICE_CLK_FILTER_DIV1_5		(0x4<<4)
#define VOICE_CLK_FILTER_DIV1_6		(0x5<<4)
#define VOICE_CLK_FILTER_DIV1_7		(0x6<<4)
#define VOICE_CLK_FILTER_DIV1_8		(0x7<<4)
#define VOICE_CLK_FILTER_DIV1_9		(0x8<<4)
#define VOICE_CLK_FILTER_DIV1_10	(0x9<<4)
#define VOICE_CLK_FILTER_DIV1_11	(0xA<<4)
#define VOICE_CLK_FILTER_DIV1_12	(0xB<<4)
#define VOICE_CLK_FILTER_DIV1_13	(0xC<<4)
#define VOICE_CLK_FILTER_DIV1_14	(0xD<<4)
#define VOICE_CLK_FILTER_DIV1_15	(0xE<<4)
#define VOICE_CLK_FILTER_DIV1_16	(0xF<<4)

#define VOICE_CLK_FILTER_DIV2_MASK	(0x7)
#define VOICE_CLK_FILTER_DIV2_2		(0x0)
#define VOICE_CLK_FILTER_DIV2_4		(0x1)
#define VOICE_CLK_FILTER_DIV2_8		(0x2)
#define VOICE_CLK_FILTER_DIV2_16	(0x3)
#define VOICE_CLK_FILTER_DIV2_32	(0x4)


//Psedueo Stereo & Spatial Effect Block Control(0x68)
#define SPATIAL_CTRL_EN				(0x1<<15)
#define ALL_PASS_FILTER_EN			(0x1<<14)
#define PSEUDO_STEREO_EN			(0x1<<13)
#define STEREO_EXPENSION_EN			(0x1<<12)

#define SPATIAL_GAIN_MASK			(0x3<<6)
#define SPATIAL_GAIN_1_0			(0x0<<6)
#define SPATIAL_GAIN_1_5			(0x1<<6)
#define SPATIAL_GAIN_2_0			(0x2<<6)

#define SPATIAL_RATIO_MASK			(0x3<<4)
#define SPATIAL_RATIO_0_0			(0x0<<4)
#define SPATIAL_RATIO_0_66			(0x1<<4)
#define SPATIAL_RATIO_1_0			(0x2<<4)

#define APF_MASK					(0x3)
#define APF_FOR_48K					(0x3)
#define APF_FOR_44_1K				(0x2)
#define APF_FOR_32K					(0x1)



struct rt5610_setup_data {
	unsigned short i2c_address;
	unsigned short i2c_bus;
};


#define RT5610_PLL_FR_MCLK			0


extern struct snd_soc_dai rt5610_dai[2];	
extern struct snd_soc_codec_device soc_codec_dev_rt5610;
extern int rt5610_reset(struct snd_soc_codec *codec, int try_warm);
#endif
