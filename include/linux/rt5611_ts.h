#ifndef _RT5611_TS_H
#define _RT5611_TS_H


#include <sound/core.h>
#include <sound/ac97_codec.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/input.h>	
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/pm.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#define MAX_CONVERSIONS 					10
#define AC97_LINK_FRAME 					21 // 1 frame=20.8us
#define MAX_ADC_VAL						0xFFF //12-bit

/* The returned sample is valid */
#define RC_VALID			0x0001
/* More data is available; call the sample gathering function again */
#define RC_AGAIN		0x0002
/* The pen is up (the first RC_VALID without RC_PENUP means pen is down) */
#define RC_PENUP		0x0004


#define RT5611_ID1 	0x10EC  //vendor ID
#define RT5611_ID2 	0x1003 //device ID


#define RT_TP_ENABLE		1 
#define RT_TP_DISABLE		0 


#define RT_ADC_X					0x0001
#define RT_ADC_Y					0x0002
#define RT_ADC_P					0x0003
#define RT_PD_CTRL_STAT					0X26			//POWER DOWN CONTROL/STATUS

#define RT_PWR_MANAG_ADD1				0X3A			//POWER MANAGMENT ADDITION 1
#define RT_PWR_MANAG_ADD2				0X3C			//POWER MANAGMENT ADDITION 2
#define RT_GPIO_PIN_CONFIG				0X4C			//GPIO PIN CONFIGURATION
#define RT_GPIO_PIN_POLARITY			0X4E			//GPIO PIN POLARITY/TYPE	
#define RT_GPIO_PIN_STICKY				0X50			//GPIO PIN STICKY	
#define RT_GPIO_PIN_WAKEUP				0X52			//GPIO PIN WAKE UP
#define RT_GPIO_PIN_STATUS				0X54			//GPIO PIN STATUS
#define RT_GPIO_PIN_SHARING				0X56			//GPIO PIN SHARING
#define RT_OVER_TEMP_CURR_STATUS		0X58			//OVER TEMPERATURE AND CURRENT STATUS
#define RT_GPIO_OUT_CTRL				0X5C			//GPIO OUTPUT PIN CONTRL
#define RT_MISC_CTRL					0X5E			//MISC CONTROL
#define RT_TP_CTRL_BYTE1				0X74			//TOUCH PANEL CONTROL BYTE 1
#define RT_TP_CTRL_BYTE2				0X76			//TOUCH PANEL CONTROL BYTE 2
#define RT_TP_INDICATION				0X78			//TOUCH PANEL INDICATION
#define RT_VENDOR_ID1	  		    	0x7C			//VENDOR ID1
#define RT_VENDOR_ID2	  		    	0x7E			//VENDOR ID2

//Define bit of GPIO function
#define	RT_GPIO_BIT0				0x0001				
#define	RT_GPIO_BIT1				0x0002				//GPIO 1 Control/Status	
#define	RT_GPIO_BIT2				0x0004				//GPIO 2 Control/Status	
#define	RT_GPIO_BIT3				0x0008				//GPIO 3 Control/Status	
#define	RT_GPIO_BIT4				0x0010				//GPIO 4 Control/Status	
#define	RT_GPIO_BIT5				0x0020				//GPIO 5 Control/Status
#define	RT_GPIO_BIT6				0x0040				
#define	RT_GPIO_BIT7				0x0080
#define	RT_GPIO_BIT8				0x0100
#define	RT_GPIO_BIT9				0x0200				//MICBIAS2 Control/Status
#define	RT_GPIO_BIT10				0x0400				//MICBIAS1 Control/Status
#define	RT_GPIO_BIT11				0x0800				//Over temperature Control/Status
#define	RT_GPIO_BIT12				0x1000				
#define	RT_GPIO_BIT13				0x2000				//Pen down detect Control/Status
#define	RT_GPIO_BIT14				0x4000
#define	RT_GPIO_BIT15				0x8000

//Power Down CTRL/STATUS  (0x26),0:Normal,1:Power Down
#define PD_CTRL_VREF				(0x1<<11)
#define PD_CTRL_ADC					(0x1<<8)
#define PD_CTRL_STATUS_VREF		(0x1<<3)
#define PD_CTRL_STATUS_ADC		(0x1<<0)


//Power managment addition 1 (0x3A),0:Disable,1:Enable
#define PWR_MAIN_BIAS				(0x1<<1)
#define PWR_IP_ENA					(0x1<<12)
	

//Power managment addition 2(0x3C),0:Disable,1:Enable
#define PWR_MIXER_VREF				(0x1<<13)
#define PWR_TP_ADC					(0x1<<11)

//Pin Sharing(0x56)
#define GPIO2_PIN_SHARING_MASK		(0x03<<2)	//GPIO2 Pin sharing mask
#define GPIO2_PIN_SHARING_IRQ		(0x00<<2)	//IRQ out
#define GPIO2_PIN_SHARING_GPIO		(0x01<<2)	//GPIO enable	
#define GPIO2_PIN_SHARING_IR		(0x10<<2)	//IR out	

//MISC CONTROL(0x5E)
#define GPIO_WAKEUP_CTRL			(0x01<<1)	//Enable GPIO wakeup Control
#define IRQOUT_INVERT_CTRL			(0x01	)	//IRQOUT inverter control

//Touch Panel Control Byte 1(0x74)
#define POW_TP_CTRL_MASK			(0x3<<14)
#define POW_TP_CTRL_0				(0x0<<14)	//All off
#define POW_TP_CTRL_1				(0x1<<14)	//Aux_ADC off,pen down detect with wake_up,Aux_ADC on with pen down
#define POW_TP_CTRL_2				(0x2<<14)	//Aux_ADC off,pen down detect on without wake up 
#define POW_TP_CTRL_3				(0x3<<14)	//AUX_ADC on,pen down detect on

#define CB1_PRES_CURR_MASK		(0x000f<<10)//Presusure measurement source current
#define CB1_PRES_CURR_OFF			(0x0000<<10)//OFF	
#define CB1_PRES_CURR_25UA			(0x0001<<10)//25uA
#define CB1_PRES_CURR_50UA			(0x0002<<10)//50uA
#define CB1_PRES_CURR_375UA		(0x000f<<10)//375uA


#define CB1_PDPOL					(0x0001<<9)	//Pendown polarity 0:non-inverted 1:inverted

#define	CB1_DEL_MASK				(0x0003<<7)		//touch panel measure delay mask
#define	CB1_DEL_4F					(0x0000<<7)		//delay 4 frames
#define	CB1_DEL_8F					(0x0001<<7)		//delay 8 frames	
#define	CB1_DEL_16F				(0x0002<<7)		//delay 16 frames
#define	CB1_DEL_32F				(0x0003<<7)		//delay 32 frames

#define CB1_SLOT_READBACK			(0x0001<<6)		//Enable slot realback

#define CB1_SLOT_SEL				(0x0001<<5)		//AC97 SLOT select 0:Slot5 1:Slot6

#define CB1_CLK_DIV_MASK			(0x0007<<2)		//AUX_ADC Clock Divider MASK
#define CB1_CLK_DIV64				(0x0003<<2)		//AUX_ADC Clock divide	64
#define CB1_CLK_DIV80				(0x0004<<2)		//AUX_ADC Clock divide	80
#define CB1_CLK_DIV96				(0x0005<<2)		//AUX_ADC Clock divide	96
#define CB1_CLK_DIV112				(0x0006<<2)		//AUX_ADC Clock divide 	112
#define CB1_CLK_DIV128				(0x0007<<2)		//AUX_ADC Clock divide 	128

#define CB1_CR_MASK					(0x0003<<0)		//Conversion rate Mask
#define CB1_CR0						(0x0000<<0)		//	 93.75Hz,512 frame
#define CB1_CR1						(0x0001<<0)		//	124.67Hz,384 frame
#define CB1_CR2						(0x0002<<0)		//	187.5 Hz,256 frame
#define CB1_CR3						(0x0003<<0)		//	374.0 Hz,128 frame

#define CB1_DEFALT 					0x008C

//Touch Panel Control Byte 2(0x76)

#define CB2_POLL_TRIG				(0x0001<<15)		//write 1 initiaties a measurement in polling mode.

#define CB2_MODE_SEL				(0x0001<<14)		//0:Polling mode  1:Continuous mode

#define CB2_PPR_MASK				(0x003f<<8)		//internal Pull-up resistor for pen-down detection MASK
#define CB2_PPR_1					(0x0000<<8)		// 1k	Ohm
#define CB2_PPR_2					(0x0001<<8)		// 2k	Ohm
#define CB2_PPR_3					(0x0002<<8)		// 3k	Ohm
#define CB2_PPR_64					(0x003f<<8)		//64k	Ohm


#define CB2_AUX_EN					(0x0001<<7)			//AUX measument enable/disable
	
#define	CB2_PRESSURE_EN			(0x0001<<3)			//Presure measurement enable/disable
#define	CB2_Y_EN					(0x0001<<2)			//Y measurement enable/disable
#define	CB2_X_EN					(0x0001<<1)			//X measurement enable/disable
#define   CB2_AUX_SEL				(0x0001)			//AUX3/AUX4 measure selection	

#define CB2_DEFALT 					0x3F00

//Touch Panel Indication(0x78)
#define CB3_PD_STATUS				(0x0001<<15)		//pendown status after reading 

#define CB3_ADCSRC_MASK			(0x0007<<12)
#define CB3_ADCSRC_NON				(0x0000)			//No Data
#define CB3_ADCSRC_X				(0x1000)			//X co-ordinate measurement
#define CB3_ADCSRC_Y				(0x2000)			//Y co-ordinate measurement
#define CB3_ADCSRC_PRE				(0x3000)			//Pressure measurement
#define CB3_ADCSRC_AUX				(0x7000)			//AUX	measurement

#define CB3_ADC_DATA				(0x0FFF)			//AUX_ADC Data Report

struct  rt5611_ts_data {
    int x;
    int y;
    int p;
};

struct rt5611_ts {
	u16 id;
	u16 cb[2],pm, misc,gpio[6];		/* Cached registers */
	struct input_dev *input_dev;	/* touchscreen input device */
	struct snd_soc_codec *codec;
	struct device *dev;		/* ALSA device */
	struct delayed_work ts_reader;  /* Used to poll touchscreen */
	unsigned long ts_reader_interval; /* Current interval for timer */
	unsigned long ts_reader_min_interval; /* Minimum interval */
	unsigned int pen_irq;		/* Pen IRQ number in use */
	struct workqueue_struct *ts_workq;
	struct work_struct pen_event_work;
	unsigned pen_is_down:1;		/* Pen is down */
	u16 suspend_mode;               /* PRP in suspend mode */
};

#endif
