/*  $Date: 2007/08/07 16:05:10 $
 *  $Revision: 1.4 $
 *
 */

/*
* Copyright (C) 2007 Bosch Sensortec GmbH
*
* BMA020 acceleration sensor API
*
* Usage:	Application Programming Interface for BMA020 configuration and data read out
*
* Author:	Lars.Beseke@bosch-sensortec.com
*
* Disclaimer
*
* Common:
* Bosch Sensortec products are developed for the consumer goods industry. They may only be used
* within the parameters of the respective valid product data sheet.  Bosch Sensortec products are
* provided with the express understanding that there is no warranty of fitness for a particular purpose.
* They are not fit for use in life-sustaining, safety or security sensitive systems or any system or device
* that may lead to bodily harm or property damage if the system or device malfunctions. In addition,
* Bosch Sensortec products are not fit for use in products which interact with motor vehicle systems.
* The resale and/or use of products are at the purchaser’s own risk and his own responsibility. The
* examination of fitness for the intended use is the sole responsibility of the Purchaser.
*
* The purchaser shall indemnify Bosch Sensortec from all third party claims, including any claims for
* incidental, or consequential damages, arising from any product use not covered by the parameters of
* the respective valid product data sheet or not approved by Bosch Sensortec and reimburse Bosch
* Sensortec for all costs in connection with such claims.
*
* The purchaser must monitor the market for the purchased products, particularly with regard to
* product safety and inform Bosch Sensortec without delay of all security relevant incidents.
*
* Engineering Samples are marked with an asterisk (*) or (e). Samples may vary from the valid
* technical specifications of the product series. They are therefore not intended or fit for resale to third
* parties or for use in end products. Their sole purpose is internal client testing. The testing of an
* engineering sample may in no way replace the testing of a product series. Bosch Sensortec
* assumes no liability for the use of engineering samples. By accepting the engineering samples, the
* Purchaser agrees to indemnify Bosch Sensortec from all claims arising from the use of engineering
* samples.
*
* Special:
* This software module (hereinafter called "Software") and any information on application-sheets
* (hereinafter called "Information") is provided free of charge for the sole purpose to support your
* application work. The Software and Information is subject to the following terms and conditions:
*
* The Software is specifically designed for the exclusive use for Bosch Sensortec products by
* personnel who have special experience and training. Do not use this Software if you do not have the
* proper experience or training.
*
* This Software package is provided `` as is `` and without any expressed or implied warranties,
* including without limitation, the implied warranties of merchantability and fitness for a particular
* purpose.
*
* Bosch Sensortec and their representatives and agents deny any liability for the functional impairment
* of this Software in terms of fitness, performance and safety. Bosch Sensortec and their
* representatives and agents shall not be liable for any direct or indirect damages or injury, except as
* otherwise stipulated in mandatory applicable law.
*
* The Information provided is believed to be accurate and reliable. Bosch Sensortec assumes no
* responsibility for the consequences of use of such Information nor for any infringement of patents or
* other rights of third parties which may result from its use. No license is granted by implication or
* otherwise under any patent or patent rights of Bosch. Specifications mentioned in the Information are
* subject to change without notice.
*
* It is not allowed to deliver the source code of the Software to any third party without permission of
* Bosch Sensortec.
*/


/*! \file bma020.c
    \brief This file contains all function implementations for the BMA020 API

    Details.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>

#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <asm/system.h>
#include <mach/gpio.h>

//#include <mach/mfp-pxa168.h> //JP
#include <mach/irqs.h>   //JP

#include <linux/input-polldev.h>
#include <linux/hwmon.h>


#include <linux/bma020.h>

//#include <asm/hardware.h>
//#include <asm/arch/mfp.h>
//#include <linux/hwmon.h>
//#include <linux/hwmon-vid.h>
//#include <linux/hwmon-sysfs.h>




/*****************************************************************************
 *                              BMA020 I2C Client Driver
 *****************************************************************************/
//#define INPUT_POLL			//for input_poll interface, otherwise ioctl interface

#define DEBUG_INFO
#define DEBUG

/** select one of below **/
//#define DEBUG_INPUT_POLL		//for input_poll interface
//#define DEBUG_IOCTL			//for ioctl interface

#define G_INT_PIN 0x17

#if 0
//old i2c attach method
#define	BMA020_ADDRESS	(0x38)			//0x70/71
static unsigned short ignore[]={I2C_CLIENT_END};
static unsigned short normal_addr[]={BMA020_ADDRESS,I2C_CLIENT_END,};
static struct i2c_client_address_data addr_data={
	.normal_i2c	= normal_addr,
	.probe		= ignore,
	.ignore		= ignore,
};
//static unsigned short normal_i2c[] = {BMA020_ADDRESS, I2C_CLIENT_END };
#endif

static struct i2c_client *g_client;

static DEFINE_MUTEX(bma020_lock);

static bma020_t *p_bma020 = NULL;				/**< pointer to BMA020 device structure  */


//***************************i2c bus function**************************************//

char i2c_bma020_read_buf( u8 dev_addr,u8 reg_addr, u8* buf,u8 cnt)
{
	i2c_master_send(g_client,&reg_addr,1);		//i2c_smbus_write_byte(g_client,reg_addr);
	return i2c_master_recv(g_client, buf ,cnt);
}

char i2c_bma020_write_buf( u8 dev_addr,u8 reg_addr, u8* buf,u8 cnt)
{
	int i;
	char *t_buf = kmalloc(cnt+1, GFP_KERNEL);
	t_buf[0] = reg_addr;
	for(i=0; i< cnt; i++)
		t_buf[i+1] = buf[i];
	//here issue cnt always be 1!
	//printk(KERN_ALERT"********** %s *********\n",__FUNCTION__); //JP
	return i2c_smbus_write_byte_data(g_client,reg_addr,*buf); //JP
	//i2c_master_send(g_client, &reg_addr, 1); //JP
	//i2c_master_send(g_client, t_buf, cnt+1); //JP
	
}


//*************************bma020 hw configuration***********************************//
int bma020_detect(void)
{
	int comres=0;
	unsigned char data;


	//get chip info.
	p_bma020->dev_addr = BMA020_I2C_ADDR;	/* preset SM380 I2C_addr */
	p_bma020->bus_write = i2c_bma020_write_buf;
	p_bma020->bus_read = i2c_bma020_read_buf;

	comres += p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, CHIP_ID__REG, &data, 1);/* read Chip Id */

	p_bma020->chip_id = BMA020_GET_BITSLICE(data, CHIP_ID);	/* get bitslice */

	comres += p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ML_VERSION__REG, &data, 1); /* read Version reg */
	p_bma020->ml_version = BMA020_GET_BITSLICE(data, ML_VERSION);	/* get ML Version */
	p_bma020->al_version = BMA020_GET_BITSLICE(data, AL_VERSION);/* get AL Version */

	return comres;
}

/** Perform soft reset of BMA020 via bus command
*/
int bma020_soft_reset(void)
{
	int comres;
	unsigned char data=0;
	if (p_bma020==0)
		return E_BMA_NULL_PTR;
	data = BMA020_SET_BITSLICE(data, SOFT_RESET, 1);
	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, SOFT_RESET__REG, &data,1);
	return comres;
}

/**	start BMA020s integrated selftest function
   \param st 1 = selftest0, 3 = selftest1 (see also)
	 \return result of bus communication function

	 \see BMA020_SELF_TEST0_ON
	 \see BMA020_SELF_TEST1_ON

 */
int bma020_selftest(unsigned char st)
{
	int comres;
	unsigned char data;
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, SELF_TEST__REG, &data, 1);
	data = BMA020_SET_BITSLICE(data, SELF_TEST, st);
	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, SELF_TEST__REG, &data, 1);
	return comres;

}



/**	set bma020s range
 \param range
 \return  result of bus communication function

 \see BMA020_RANGE_2G
 \see BMA020_RANGE_4G
 \see BMA020_RANGE_8G
*/
int bma020_set_range(char range)
{
   int comres = 0;
   unsigned char data;

   if (p_bma020==0)
	    return E_BMA_NULL_PTR;

   if (range<3) {
		comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, RANGE__REG, &data, 1 );
		data = BMA020_SET_BITSLICE(data, RANGE, range);
		comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, RANGE__REG, &data, 1);
   }
   return comres;

}


/* readout select range from BMA020
   \param *range pointer to range setting
   \return result of bus communication function
   \see BMA020_RANGE_2G, BMA020_RANGE_4G, BMA020_RANGE_8G
   \see bma020_set_range()
*/
int bma020_get_range(unsigned char *range)
{

	int comres = 0;

	if (p_bma020==0)
		return E_BMA_NULL_PTR;
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, RANGE__REG, range, 1 );

	*range = BMA020_GET_BITSLICE(*range, RANGE);

	return comres;

}



/** set BMA020s operation mode
   \param mode 0 = normal, 2 = sleep, 3 = auto wake up
   \return result of bus communication function
   \note Available constants see below
   \see BMA020_MODE_NORMAL, BMA020_MODE_SLEEP, BMA020_MODE_WAKE_UP
	 \see bma020_get_mode()
*/
int bma020_set_mode(unsigned char mode) {

	int comres=0;
	unsigned char data1, data2;
 	//printk(KERN_ALERT"func:%s\n",__func__); //JP
	//printk(KERN_ALERT"func:%s\n", __FUNCTION__); //JP
	//printk(KERN_ALERT"000000000000000\n"); //JP
	if (p_bma020==0)
		return E_BMA_NULL_PTR;
	
 	//printk(KERN_ALERT"11111111111\n"); //JP

	if (mode<4 || mode!=1) {
		comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, WAKE_UP__REG, &data1, 1 );
		data1  = BMA020_SET_BITSLICE(data1, WAKE_UP, mode);
		comres += p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, SLEEP__REG, &data2, 1 );
		data2  = BMA020_SET_BITSLICE(data2, SLEEP, (mode>>1));

		//printk(KERN_ALERT"before write\n"); //JP

		comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, WAKE_UP__REG, &data1, 1);
		comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, SLEEP__REG, &data2, 1);
		p_bma020->mode = mode;
	}
 	//printk(KERN_ALERT"222222222222\n"); //JP
	return comres;

}



/** get selected mode
   \return used mode
   \note this function returns the mode stored in \ref bma020_t structure
   \see BMA020_MODE_NORMAL, BMA020_MODE_SLEEP, BMA020_MODE_WAKE_UP
   \see bma020_set_mode()

*/
unsigned char bma020_get_mode(void)
{
	if (p_bma020==0)
		return E_BMA_NULL_PTR;
	return p_bma020->mode;

}

/** set BMA020 internal filter bandwidth
   \param bw bandwidth (see bandwidth constants)
   \return result of bus communication function
   \see #define BMA020_BW_25HZ, BMA020_BW_50HZ, BMA020_BW_100HZ, BMA020_BW_190HZ, BMA020_BW_375HZ,
   \BMA020_BW_750HZ, BMA020_BW_1500HZ
   \see bma020_get_bandwidth()
*/
int bma020_set_bandwidth(char bw)
{
	int comres = 0;
	unsigned char data;


	if (p_bma020==0)
		return E_BMA_NULL_PTR;

	if (bw<8) {
		comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, BANDWIDTH__REG, &data, 1 );
		data = BMA020_SET_BITSLICE(data, BANDWIDTH, bw);
		comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, BANDWIDTH__REG, &data, 1 );
	}
    return comres;


}

/** read selected bandwidth from BMA020
 \param *bw pointer to bandwidth return value
 \return result of bus communication function
 \see #define BMA020_BW_25HZ, BMA020_BW_50HZ, BMA020_BW_100HZ, BMA020_BW_190HZ, BMA020_BW_375HZ,
 \BMA020_BW_750HZ, BMA020_BW_1500HZ
 \see bma020_set_bandwidth()
*/
int bma020_get_bandwidth(unsigned char *bw) {
	int comres = 1;
	if (p_bma020==0)
		return E_BMA_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, BANDWIDTH__REG, bw, 1 );

	*bw = BMA020_GET_BITSLICE(*bw, BANDWIDTH);

	return comres;

}

/** set BMA020 auto wake up pause
  \param wup wake_up_pause parameters
	\return result of bus communication function
	\see BMA020_WAKE_UP_PAUSE_20MS, BMA020_WAKE_UP_PAUSE_80MS, BMA020_WAKE_UP_PAUSE_320MS,
	\BMA020_WAKE_UP_PAUSE_2560MS
	\see bma020_get_wake_up_pause()
*/

int bma020_set_wake_up_pause(unsigned char wup)
{
	int comres=0;
	unsigned char data;

	if (p_bma020==0)
		return E_BMA_NULL_PTR;


	    comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, WAKE_UP_PAUSE__REG, &data, 1 );
		data = BMA020_SET_BITSLICE(data, WAKE_UP_PAUSE, wup);
		comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, WAKE_UP_PAUSE__REG, &data, 1 );
	return comres;
}

/** read BMA020 auto wake up pause from image
  \param *wup wake up pause read back pointer
	\see BMA020_WAKE_UP_PAUSE_20MS, BMA020_WAKE_UP_PAUSE_80MS, BMA020_WAKE_UP_PAUSE_320MS,
	\BMA020_WAKE_UP_PAUSE_2560MS
	\see bma020_set_wake_up_pause()
*/
int bma020_get_wake_up_pause(unsigned char *wup)
{
    int comres = 1;
	unsigned char data;
	if (p_bma020==0)
		return E_BMA_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, WAKE_UP_PAUSE__REG, &data,  1 );

	*wup = BMA020_GET_BITSLICE(data, WAKE_UP_PAUSE);

	return comres;

}


/* Thresholds and Interrupt Configuration */


/** set low-g interrupt threshold
   \param th set the threshold
   \note the threshold depends on configured range. A macro \ref BMA020_LG_THRES_IN_G() for range to
   \register value conversion is available.
   \see BMA020_LG_THRES_IN_G()
   \see bma020_get_low_g_threshold()
*/
int bma020_set_low_g_threshold(unsigned char th)
{

	int comres;

	if (p_bma020==0)
		return E_BMA_NULL_PTR;

	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, LG_THRES__REG, &th, 1);
	return comres;

}


/** get low-g interrupt threshold
   \param *th get the threshold  value from sensor image
   \see bma020_set_low_g_threshold()
*/
int bma020_get_low_g_threshold(unsigned char *th)
{

	int comres=1;
	if (p_bma020==0)
		return E_BMA_NULL_PTR;

		comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, LG_THRES__REG, th, 1);

	return comres;

}


/** set low-g interrupt countdown
   \param cnt get the countdown value from sensor image
   \see bma020_get_low_g_countdown()
*/
int bma020_set_low_g_countdown(unsigned char cnt)
{
	int comres=0;
	unsigned char data;

	if (p_bma020==0)
		return E_BMA_NULL_PTR;
  comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, COUNTER_LG__REG, &data, 1 );
  data = BMA020_SET_BITSLICE(data, COUNTER_LG, cnt);
	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, COUNTER_LG__REG, &data, 1 );
	return comres;
}


/** get low-g interrupt countdown
   \param cnt get the countdown  value from sensor image
   \see bma020_set_low_g_countdown()
*/
int bma020_get_low_g_countdown(unsigned char *cnt)
{
    int comres = 1;
	unsigned char data;
	if (p_bma020==0)
		return E_BMA_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, COUNTER_LG__REG, &data,  1 );
	*cnt = BMA020_GET_BITSLICE(data, COUNTER_LG);

	return comres;
}

/** set high-g interrupt countdown
   \param cnt get the countdown value from sensor image
   \see bma020_get_high_g_countdown()
*/
int bma020_set_high_g_countdown(unsigned char cnt)
{
	int comres=1;
	unsigned char data;

	if (p_bma020==0)
		return E_BMA_NULL_PTR;

        comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, COUNTER_HG__REG, &data, 1 );
	data = BMA020_SET_BITSLICE(data, COUNTER_HG, cnt);
	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, COUNTER_HG__REG, &data, 1 );
	return comres;
}

/** get high-g interrupt countdown
   \param cnt get the countdown  value from sensor image
   \see bma020_set_high_g_countdown()
*/
int bma020_get_high_g_countdown(unsigned char *cnt)
{
    int comres = 0;
	unsigned char data;
	if (p_bma020==0)
		return E_BMA_NULL_PTR;
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, COUNTER_HG__REG, &data,  1 );

	*cnt = BMA020_GET_BITSLICE(data, COUNTER_HG);

	return comres;

}


/** configure low-g duration value
	\param dur low-g duration in miliseconds
	\see bma020_get_low_g_duration(), bma020_get_high_g_duration(), bma020_set_high_g_duration()

*/
int bma020_set_low_g_duration(unsigned char dur)
{
	int comres=0;
	if (p_bma020==0)
		return E_BMA_NULL_PTR;


	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, LG_DUR__REG, &dur, 1);

	return comres;
}


int bma020_set_low_g_hyst(unsigned char hyst)
{
	int comres=0;
	unsigned char data;
	if (p_bma020==0)
		return E_BMA_NULL_PTR;

        comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, LG_HYST__REG, &data, 1 );
	data = BMA020_SET_BITSLICE(data, LG_HYST , hyst);
	#ifdef DEBUG_INFO
	printk("##!!## hyst=0x%x\n",data);
	#endif
	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, LG_HYST__REG, &data, 1 );

	return comres;
}

int bma020_set_high_g_hyst(unsigned char hyst)
{
	int comres=0;
	unsigned char data;
	if (p_bma020==0)
		return E_BMA_NULL_PTR;

        comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, HG_HYST__REG, &data, 1 );
	data = BMA020_SET_BITSLICE(data, HG_HYST , hyst);
	#ifdef DEBUG_INFO
	printk("##!!## hyst=0x%x\n",data);
	#endif
	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, HG_HYST__REG, &data, 1 );

	return comres;
}

/** read out low-g duration value from sensor image
	\param dur low-g duration in miliseconds
	\see bma020_set_low_g_duration(), bma020_get_high_g_duration(), bma020_set_high_g_duration()

*/
int bma020_get_low_g_duration(unsigned char *dur) {

	int comres=0;
	if (p_bma020==0)
		return E_BMA_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, LG_DUR__REG, dur, 1);
	return comres;

}




/** set low-g interrupt threshold
   \param th set the threshold
   \note the threshold depends on configured range. A macro \ref BMA020_HG_THRES_IN_G() for range to
   \register value conversion is available.
   \see BMA020_HG_THRES_IN_G()
   \see bma020_get_high_g_threshold()
*/
int bma020_set_high_g_threshold(unsigned char th)
{

	int comres=0;

	if (p_bma020==0)
		return E_BMA_NULL_PTR;

	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, HG_THRES__REG, &th, 1);
	return comres;

}

/** get high-g interrupt threshold
   \param *th get the threshold  value from sensor image
   \see bma020_set_high_g_threshold()
*/
int bma020_get_high_g_threshold(unsigned char *th)
{

	int comres=0;
	if (p_bma020==0)
		return E_BMA_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, HG_THRES__REG, th, 1);

	return comres;

}



/** configure high-g duration value
	\param dur high-g duration in miliseconds
	\see  bma020_get_high_g_duration(), bma020_set_low_g_duration(), bma020_get_low_g_duration()

*/
int bma020_set_high_g_duration(unsigned char dur)
{
	int comres=0;

	if (p_bma020==0)
		return E_BMA_NULL_PTR;

	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, HG_DUR__REG, &dur, 1);
	return comres;
}


/** read out high-g duration value from sensor image
	\param dur high-g duration in miliseconds
	\see  bma020_set_high_g_duration(), bma020_get_low_g_duration(), bma020_set_low_g_duration(),

*/
int bma020_get_high_g_duration(unsigned char *dur) {

	int comres=0;
	if (p_bma020==0)
		return E_BMA_NULL_PTR;

        comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, HG_DUR__REG, dur, 1);

	return comres;

}




/**  set threshold value for any_motion feature
		\param th set the threshold a macro \ref BMA020_ANY_MOTION_THRES_IN_G()  is available for that
		\see BMA020_ANY_MOTION_THRES_IN_G()
*/
int bma020_set_any_motion_threshold(unsigned char th)
{
	int comres=0;

	if (p_bma020==0)
		return E_BMA_NULL_PTR;

	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, ANY_MOTION_THRES__REG, &th, 1);

	return comres;
}


/**  get threshold value for any_motion feature
		\param *th read back any_motion threshold from image register
		\see BMA020_ANY_MOTION_THRES_IN_G()
*/
int bma020_get_any_motion_threshold(unsigned char *th)
{

	int comres=0;
	if (p_bma020==0)
		return E_BMA_NULL_PTR;
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ANY_MOTION_THRES__REG, th, 1);

	return comres;

}

/**  set counter value for any_motion feature
		\param amc set the counter value, constants are available for that
		\see BMA020_ANY_MOTION_DUR_1, BMA020_ANY_MOTION_DUR_3, BMA020_ANY_MOTION_DUR_5,
		\BMA020_ANY_MOTION_DUR_7
*/
int bma020_set_any_motion_count(unsigned char amc)
{
	int comres=0;
	unsigned char data;
	if (p_bma020==0)
		return E_BMA_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ANY_MOTION_DUR__REG, &data, 1 );
	data = BMA020_SET_BITSLICE(data, ANY_MOTION_DUR, amc);
	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, ANY_MOTION_DUR__REG, &data, 1 );
	return comres;
}


/**  get counter value for any_motion feature from image register
		\param *amc readback pointer for counter value
		\see BMA020_ANY_MOTION_DUR_1, BMA020_ANY_MOTION_DUR_3, BMA020_ANY_MOTION_DUR_5,
		\BMA020_ANY_MOTION_DUR_7
*/
int bma020_get_any_motion_count(unsigned char *amc)
{
    int comres = 0;
	unsigned char data;
	if (p_bma020==0)
		return E_BMA_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ANY_MOTION_DUR__REG, &data,  1 );

	*amc = BMA020_GET_BITSLICE(data, ANY_MOTION_DUR);
	return comres;

}



/** set the interrupt mask for BMA020's interrupt features in one mask
	\param mask input for interrupt mask
	\see BMA020_INT_ALERT, BMA020_INT_ANY_MOTION, BMA020_INT_EN_ADV_INT, BMA020_INT_NEW_DATA,
	\BMA020_INT_LATCH, BMA020_INT_HG, BMA020_INT_LG
*/
int bma020_set_interrupt_mask(unsigned char mask)
{
	int comres=0;
	unsigned char data[4];

	if (p_bma020==0)
		return E_BMA_NULL_PTR;
	data[0] = mask & BMA020_CONF1_INT_MSK;
	data[2] = ((mask<<1) & BMA020_CONF2_INT_MSK);


	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, BMA020_CONF1_REG, &data[1], 1);
	comres += p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, BMA020_CONF2_REG, &data[3], 1);
	data[1] &= (~BMA020_CONF1_INT_MSK);
	data[1] |= data[0];
	data[3] &=(~(BMA020_CONF2_INT_MSK));
	data[3] |= data[2];

	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, BMA020_CONF1_REG, &data[1], 1);
	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, BMA020_CONF2_REG, &data[3], 1);

	return comres;
}




/** set the shadow_dis for BMA020's shadow_dis bit
	\param ssd input for on/off control
	\see BMA020_SHADOW_DIS_OFF, BMA020_SHADOW_DIS_ON
*/
int bma020_set_shadow_dis(unsigned char ssd)
{
	   int comres = 0;
	   unsigned char data;

	   if (p_bma020==0)
		    return E_BMA_NULL_PTR;

	   if ((ssd==1)||(ssd==0)) {
			comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, BMA020_CONF2_REG, &data, 1 );
			data = BMA020_SET_BITSLICE(data, SHADOW_DIS, ssd);
			comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, BMA020_CONF2_REG, &data, 1);
	   }
	   return comres;

}

/** get the current interrupt mask settings from BMA020 image registers
	\param *mask return variable pointer for interrupt mask
	\see BMA020_INT_ALERT, BMA020_INT_ANY_MOTION, BMA020_INT_EN_ADV_INT, BMA020_INT_NEW_DATA,
	\BMA020_INT_LATCH, BMA020_INT_HG, BMA020_INT_LG
*/
int bma020_get_interrupt_mask(unsigned char *mask)
{
	int comres=0;
	unsigned char data;

	if (p_bma020==0)
		return E_BMA_NULL_PTR;

	p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, BMA020_CONF1_REG, &data,1);
	*mask = data & BMA020_CONF1_INT_MSK;
	p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, BMA020_CONF2_REG, &data,1);
	*mask = *mask | ((data & BMA020_CONF2_INT_MSK)>>1);

	return comres;
}


/** resets the BMA020 interrupt status
		\note this feature can be used to reset a latched interrupt

*/
int bma020_reset_interrupt(void)
{
	#ifdef DEBUG_INFO
	printk("##!!## file=%s,func=%s\n",__FILE__,__FUNCTION__);
	#endif
	int comres=0;
	unsigned char data=(1<<RESET_INT__POS);

	if (p_bma020==0)
		return E_BMA_NULL_PTR;

	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, RESET_INT__REG, &data, 1);
	return comres;

}





/* Data Readout */

/** X-axis acceleration data readout
	\param *a_x pointer for 16 bit 2's complement data output (LSB aligned)
*/
int bma020_read_accel_x(short *a_x)
{
	int comres;
	unsigned char data[2];


	if (p_bma020==0)
		return E_BMA_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ACC_X_LSB__REG, data, 2);
	*a_x = BMA020_GET_BITSLICE(data[0],ACC_X_LSB) | BMA020_GET_BITSLICE(data[1],ACC_X_MSB)<<ACC_X_LSB__LEN;
	*a_x = *a_x << (sizeof(short)*8-(ACC_X_LSB__LEN+ACC_X_MSB__LEN));
	*a_x = *a_x >> (sizeof(short)*8-(ACC_X_LSB__LEN+ACC_X_MSB__LEN));
	return comres;

}



/** Y-axis acceleration data readout
	\param *a_y pointer for 16 bit 2's complement data output (LSB aligned)
*/
int bma020_read_accel_y(short *a_y)
{
	int comres;
	unsigned char data[2];


	if (p_bma020==0)
		return E_BMA_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ACC_Y_LSB__REG, data, 2);
	*a_y = BMA020_GET_BITSLICE(data[0],ACC_Y_LSB) | BMA020_GET_BITSLICE(data[1],ACC_Y_MSB)<<ACC_Y_LSB__LEN;
	*a_y = *a_y << (sizeof(short)*8-(ACC_Y_LSB__LEN+ACC_Y_MSB__LEN));
	*a_y = *a_y >> (sizeof(short)*8-(ACC_Y_LSB__LEN+ACC_Y_MSB__LEN));
	return comres;
}


/** Z-axis acceleration data readout
	\param *a_z pointer for 16 bit 2's complement data output (LSB aligned)
*/
int bma020_read_accel_z(short *a_z)
{
	int comres;
	unsigned char data[2];

	if (p_bma020==0)
		return E_BMA_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ACC_Z_LSB__REG, data, 2);
	*a_z = BMA020_GET_BITSLICE(data[0],ACC_Z_LSB) | BMA020_GET_BITSLICE(data[1],ACC_Z_MSB)<<ACC_Z_LSB__LEN;
	*a_z = *a_z << (sizeof(short)*8-(ACC_Z_LSB__LEN+ACC_Z_MSB__LEN));
	*a_z = *a_z >> (sizeof(short)*8-(ACC_Z_LSB__LEN+ACC_Z_MSB__LEN));
	return comres;
}


/** X,Y and Z-axis acceleration data readout
	\param *acc pointer to \ref bma020acc_t structure for x,y,z data readout
	\note data will be read by multi-byte protocol into a 6 byte structure
*/
int bma020_read_accel_xyz(bma020acc_t * acc)
{
	int comres;
	unsigned char data[6];


	if (p_bma020==0)
		return E_BMA_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ACC_X_LSB__REG, &data[0],6);

	acc->x = BMA020_GET_BITSLICE(data[0],ACC_X_LSB) | (BMA020_GET_BITSLICE(data[1],ACC_X_MSB)<<ACC_X_LSB__LEN);
	acc->x = acc->x << (sizeof(short)*8-(ACC_X_LSB__LEN+ACC_X_MSB__LEN));
	acc->x = acc->x >> (sizeof(short)*8-(ACC_X_LSB__LEN+ACC_X_MSB__LEN));

	acc->y = BMA020_GET_BITSLICE(data[2],ACC_Y_LSB) | (BMA020_GET_BITSLICE(data[3],ACC_Y_MSB)<<ACC_Y_LSB__LEN);
	acc->y = acc->y << (sizeof(short)*8-(ACC_Y_LSB__LEN + ACC_Y_MSB__LEN));
	acc->y = acc->y >> (sizeof(short)*8-(ACC_Y_LSB__LEN + ACC_Y_MSB__LEN));


	acc->z = BMA020_GET_BITSLICE(data[4],ACC_Z_LSB) | (BMA020_GET_BITSLICE(data[5],ACC_Z_MSB)<<ACC_Z_LSB__LEN);
	acc->z = acc->z << (sizeof(short)*8-(ACC_Z_LSB__LEN+ACC_Z_MSB__LEN));
	acc->z = acc->z >> (sizeof(short)*8-(ACC_Z_LSB__LEN+ACC_Z_MSB__LEN));

	return comres;

}



/** check current interrupt status from interrupt status register in BMA020 image register
	\param *ist pointer to interrupt status byte
	\see BMA020_INT_STATUS_HG, BMA020_INT_STATUS_LG, BMA020_INT_STATUS_HG_LATCHED,
	\BMA020_INT_STATUS_LG_LATCHED, BMA020_INT_STATUS_ALERT, BMA020_INT_STATUS_ST_RESULT
*/
int bma020_get_interrupt_status(unsigned char * ist)
{

	int comres=0;
	if (p_bma020==0)
		return E_BMA_NULL_PTR;
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, BMA020_STATUS_REG, ist, 1);
	return comres;
}

/** enable/ disable low-g interrupt feature
		\param onoff enable=1, disable=0
*/

int bma020_set_low_g_int(unsigned char onoff) {
	int comres;
	unsigned char data;
	if(p_bma020==0)
		return E_BMA_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ENABLE_LG__REG, &data, 1);
	data = BMA020_SET_BITSLICE(data, ENABLE_LG, onoff);

	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, ENABLE_LG__REG, &data, 1);

	return comres;

}

/** enable/ disable high-g interrupt feature
		\param onoff enable=1, disable=0
*/

int bma020_set_high_g_int(unsigned char onoff) {
	int comres;
	unsigned char data;
	if(p_bma020==0)
		return E_BMA_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ENABLE_HG__REG, &data, 1);
	data = BMA020_SET_BITSLICE(data, ENABLE_HG, onoff);
	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, ENABLE_HG__REG, &data, 1);

	return comres;
}



/** enable/ disable any_motion interrupt feature
		\param onoff enable=1, disable=0
		\note for any_motion interrupt feature usage see also \ref bma020_set_advanced_int()
*/
int bma020_set_any_motion_int(unsigned char onoff) {
	int comres;
	unsigned char data;
	if(p_bma020==0)
		return E_BMA_NULL_PTR;
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, EN_ANY_MOTION__REG, &data, 1);
	data = BMA020_SET_BITSLICE(data, EN_ANY_MOTION, onoff);
	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, EN_ANY_MOTION__REG, &data, 1);

	return comres;

}

/** enable/ disable alert-int interrupt feature
		\param onoff enable=1, disable=0
		\note for any_motion interrupt feature usage see also \ref bma020_set_advanced_int()
*/
int bma020_set_alert_int(unsigned char onoff) {
	int comres;
	unsigned char data;
	if(p_bma020==0)
		return E_BMA_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ALERT__REG, &data, 1);
	data = BMA020_SET_BITSLICE(data, ALERT, onoff);

	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, ALERT__REG, &data, 1);

	return comres;

}


/** enable/ disable advanced interrupt feature
		\param onoff enable=1, disable=0
		\see bma020_set_any_motion_int()
		\see bma020_set_alert_int()
*/
int bma020_set_advanced_int(unsigned char onoff) {
	int comres;
	unsigned char data;
	if(p_bma020==0)
		return E_BMA_NULL_PTR;
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ENABLE_ADV_INT__REG, &data, 1);
	data = BMA020_SET_BITSLICE(data, EN_ANY_MOTION, onoff);

	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, ENABLE_ADV_INT__REG, &data, 1);

	return comres;

}

/** enable/disable latched interrupt for all interrupt feature (global option)
	\param latched (=1 for latched interrupts), (=0 for unlatched interrupts)
*/

int bma020_latch_int(unsigned char latched) {
	int comres;
	unsigned char data;
	if(p_bma020==0)
		return E_BMA_NULL_PTR;
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, LATCH_INT__REG, &data, 1);

	#ifdef DEBUG_INFO
	printk("read1=0x%x\n",data);
	#endif
	data = BMA020_SET_BITSLICE(data, LATCH_INT, latched);
	#ifdef DEBUG_INFO
	printk("read2=0x%x\n",data);
	#endif

	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, LATCH_INT__REG, &data, 1);

	return comres;

}



/** enable/ disable new data interrupt feature
		\param onoff enable=1, disable=0
*/

int bma020_set_new_data_int(unsigned char onoff) {

//printk("bma020_set_new_data_int onoff:%d \n",onoff);

	int comres;
	unsigned char data;
	if(p_bma020==0)
		return E_BMA_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, NEW_DATA_INT__REG, &data, 1);
	data = BMA020_SET_BITSLICE(data, NEW_DATA_INT, onoff);
	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, NEW_DATA_INT__REG, &data, 1);

	return comres;

}



/** read function for raw register access

		\param addr register address
		\param *data pointer to data array for register read back
		\param len number of bytes to be read starting from addr

*/

int bma020_read_reg(unsigned char addr, unsigned char *data, unsigned char len)
{

	int comres;
	if (p_bma020==0)
		return E_BMA_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, addr, data, len);
	return comres;

}


/** write function for raw register access
		\param addr register address
		\param *data pointer to data array for register write
		\param len number of bytes to be written starting from addr
*/
int bma020_write_reg(unsigned char addr, unsigned char *data, unsigned char len)
{

	int comres;

	if (p_bma020==0)
		return E_BMA_NULL_PTR;

	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, addr, data, len);

	return comres;

}


// the new_data_int occur every 330us, CNT_MAX is adopted to reduce the sampling rate for new data int
#define CNT_MAX   (100)
#if 0
static irqreturn_t  bma020_interrupt(int irq, void *dev_id)
{
	static int cnt=0;
	if(p_bma020->int_kind == NORMAL_INT){
		if(cnt >= CNT_MAX){
			cnt=0;
			wake_up_interruptible(&(p_bma020->int_wait));
		}
		else{
			cnt++;
		}
		//wake_up_interruptible(&(p_bma020->int_wait));
		bma020_reset_interrupt();
	}
	else if(p_bma020->int_kind == ADV_INT){
		wake_up_interruptible(&(p_bma020->int_wait));
	}
	//bma020_reset_interrupt();
//	schedule_work(&p_bma020->irq_work);
	return IRQ_HANDLED;
}

static void bma020_irq_work(struct work_struct *work)
{
	/*
	bma020_read_accel_xyz(bma020_dev.mem+cnt);
	static int cnt=0;
	cnt++;
	if(cnt==2000){
		cnt=0;
		printk("-->%d---\n",cnt);
	}
	*/
	bma020acc_t data;
	static int cnt=0;
	cnt++;
	if(cnt==2000){
		bma020_read_accel_xyz(&data);
		printk("x=%d:  ",data.x);
		printk("y=%d:  ",data.y);
		printk("z=%d:  ",data.z);
		printk("\n");
	}
}
#endif
static struct fasync_struct *async = NULL;


static int app_fasync(int fd,struct file *filp,int mode)
{
	#ifdef DEBUG_INFO
	printk("##!!## file=%s,func=%s\n",__FILE__,__FUNCTION__);
	#endif
	return fasync_helper(fd,filp,mode,&async);

}

static irqreturn_t  bma020_interrupt(int irq, void *dev_id)
{

	if(async)
		kill_fasync(&async,SIGIO,POLL_IN);
	#ifdef DEBUG_INFO
	printk("##!!## file=%s,func=%s\n",__FILE__,__FUNCTION__);
	printk("##!!## bma int fasync\n");
	#endif

//	bma020_reset_interrupt();

	return IRQ_HANDLED;

}

//=======================================================================================


static struct i2c_device_id bma020_idtable[]=
{
	{"bma020",0},
	{},
};

struct i2c_driver bma020_driver  =
{
	.driver = {
		.name		= "bma020",
	},
	.id_table		= bma020_idtable,
	.probe			= bma020_i2c_probe,
	.remove			= bma020_i2c_remove,
};



void bma020_driver_test(void)
{
	bma020acc_t data;
	int tmp;
	char s;

	bma020_soft_reset();

	tmp=bma020_selftest(0);
	printk("bustest num=%d\n",tmp);

	bma020_set_range(BMA020_RANGE_2G);
	//bma020_set_range(BMA020_RANGE_8G);
	bma020_set_mode(0);
	bma020_set_bandwidth(BMA020_BW_25HZ);

	for(;;){
		bma020_get_range(&s);
		printk("range:%d\n",s);
		bma020_read_accel_xyz(&data);
		printk("x=%d:  ",data.x);
		printk("y=%d:  ",data.y);
		printk("z=%d:  ",data.z);
		printk("\n");
		msleep(1000);
	}
}

//static int __devinit bma020_i2c_probe(struct i2c_client *client)
static int __devinit bma020_i2c_probe(struct i2c_client *client, const struct i2c_device_id *dev_id)
{
	int ret,i;
	struct irq_desc *desc;

	//printk(KERN_ALERT"probing... \n");
	#ifdef DEBUG_INFO
	printk("##!!## bma020_i2c_probe \n");
	#endif

	//initialize global ptr
	g_client = client;

	//make sure it's a bma020 sensor
	bma020_detect();
	if(p_bma020->chip_id != 0x02){
		printk(KERN_ERR "bma020 chip id error\n");
		return -1;
	}
	else 
		printk(KERN_ALERT"bma020 detected, ID=%x\n",p_bma020->chip_id);

	#ifdef DEBUG_INFO
	printk(KERN_NOTICE "bma020 detect by i2c bus\n");
	#endif

	printk("bma020: bma020 attatch to i2c bus, addr=0x%x, chip id=0x%x \n",
		p_bma020->dev_addr,p_bma020->chip_id);

	#ifdef DEBUG_INFO
	printk("chip id: 0x%x\n",p_bma020->chip_id);
	printk("ml version: 0x%x\n",p_bma020->ml_version);
	printk("al version: 0x%x\n",p_bma020->al_version);
	#endif

	//bma gpio interrupt initialization
	//p_bma020->gpio = mfp_to_gpio(MFP_PIN_GPIO111); //JP
	p_bma020->gpio = G_INT_PIN;      //JP
	//g_client->irq = gpio_to_irq(p_bma020->gpio); //JP
	g_client->irq = IRQ_DOVE_GPIO_16_23;  //JP

	gpio_direction_input(p_bma020->gpio);	//set gpio pin as input

	#ifdef DEBUG_INFO
	printk("irq=%d\n",g_client->irq);
	printk("gpio=%d\n",p_bma020->gpio);
	#endif

	//IRQF_TRIGGER_RISING, IRQF_TRIGGER_FALLING, IRQF_TRIGGER_HIGH, IRQF_TRIGGER_LOW
	//ret = request_irq(g_client->irq, bma020_interrupt, IRQF_DISABLED | IRQF_TRIGGER_RISING,
	//	"bma020_irq", NULL);
	
        ret = request_irq(g_client->irq, bma020_interrupt, IRQF_SAMPLE_RANDOM | IRQF_TRIGGER_RISING,
              "bma020_irq", NULL);

	if (ret){
		printk(KERN_ERR "bma020 request irq failed\n");
		goto IRQ_ERR;
	}

	/*************************for simple test**************************/
	#ifdef DEBUG_IOCTL
	bma020_driver_test();
	#endif


//	INIT_WORK(&p_bma020->irq_work, bma020_irq_work);
//	bma020_latch_int(1);
//	bma020_set_mode(BMA020_MODE_SLEEP);	//set to sleep mode

	return 0;

IRQ_ERR:
	return ret;

}

static int __devexit bma020_i2c_remove(struct i2c_client *client)
{
	if(client->irq){
		free_irq(client->irq, client);
		client->irq=0;
	}

	g_client = NULL;
	return 0;
}




static int i2c_bma020_init(void)
{
	#ifdef DEBUG_INFO
	printk(KERN_ALERT"bma020 try attach to i2c bus.\n");
	#endif

	int ret;
	if ( (ret = i2c_add_driver(&bma020_driver)) ) {   //JP
	//if( ret = (i2c_register_driver(THIS_MODULE, &bma020_driver)) ){    //JP
		printk(KERN_ERR "bma020: Driver registration failed,"
			"module not inserted.\n");
		return ret;
	}
	else
		printk(KERN_ERR"driver registrated succussfully! ret= %d\n",ret);
	return 0;
}

static void i2c_bma020_exit(void)
{
	i2c_del_driver(&bma020_driver);
}


//**************************application interface*************************//

int app_open(struct inode *inode, struct file *filp)
{

		unsigned char data1,data2;

		printk(KERN_ALERT"##!!## open entry\n");
		bma020_set_mode(BMA020_MODE_NORMAL);	//set to normal mode
		//printk(KERN_ALERT"111111111111\n");
		msleep(2);		//delay for 1ms
		//printk(KERN_ALERT"222222222222\n");
		bma020_soft_reset();
		//printk(KERN_ALERT"333333333333\n");
		msleep(5);		//delay for >=1.3ms;
		bma020_set_range(BMA020_RANGE_2G);
		bma020_set_bandwidth(BMA020_BW_25HZ);
			bma020_set_shadow_dis(BMA020_SHADOW_DIS_ON);	//free read sequence

		//bma020_selftest(0);

		//test customer reserved reg at addr CUSTOMER1_REG, CUSTOMER2_REG
		data1=0x55;
		data2=0xAA;
		p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, CUSTOMER_RESERVED1__REG, &data1, 1);
		p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, CUSTOMER_RESERVED2__REG, &data2, 1 );
		p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, CUSTOMER_RESERVED1__REG, &data1, 1 );
		p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, CUSTOMER_RESERVED2__REG, &data2, 1 );

		if((0x55!=data1)||(0xAA!=data2)){
			printk("bma020 open err.\n");
			return -1;
		}

		//initialize the wait queue head
		init_waitqueue_head(&p_bma020->int_wait);

		bma020_set_mode(BMA020_MODE_NORMAL);	//set to normal mode
		msleep(1);		//delay for 1ms
		bma020_soft_reset();
		printk(KERN_ALERT"bma020_soft_reset\n"); //JP
		msleep(5);		//delay for >=1.3ms;

                printk(KERN_ALERT"##!!## open quit \n");
		return 0;
}

int app_release(struct inode *inode, struct file *filp)
{
//printk("##!!## release entry\n");
		bma020_set_mode(BMA020_MODE_NORMAL);	//set to normal mode
		msleep(1);		//delay for 1ms
		bma020_soft_reset();
		msleep(5);		//delay for >=1.3ms;
		bma020_set_range(BMA020_RANGE_2G);
		bma020_set_bandwidth(BMA020_BW_25HZ);
		//bma020_set_shadow_dis(BMA020_SHADOW_DIS_ON);	//free read sequence

		bma020_set_mode(BMA020_MODE_SLEEP);//sleep

		return app_fasync(-1,filp,0);

//printk("##!!## release quit\n");

		return 0;
}




//ppos: range in [0~0x15], assign address offset in mem map of BMA020
static ssize_t app_read(struct file *filp, char __user *buf, size_t count,loff_t *ppos)
{

	char local[0x16];
	int i;
	int pos=*(ppos);
	int read_len=count;
	int ret;
        bma020acc_t acc_data;  //JP

	if((pos>0x15L)||(pos<0))
		return 0;
	if((pos+read_len)>(0x16L))
		read_len=(0x16L - pos);
	#if 0
	pos = 0x06;  //JP
	ret= bma020_read_reg(pos, local, read_len);  //JP
	#endif
        ret = bma020_read_accel_xyz(&acc_data);   //JP
	//printk("##%x, %x, %x\n",acc_data.x,acc_data.y,acc_data.z);
	local[0] = (acc_data.x & 0xFF) ;
	local[1] = (acc_data.x >> 8 & 0xFF);
	local[2] = (acc_data.y & 0xFF) ;
	local[3] = (acc_data.y >> 8 & 0xFF);
	local[4] = (acc_data.z & 0xFF);
	local[5] = (acc_data.z >> 8 & 0xFF);
	if(copy_to_user((char*)buf,local,ret)){
		return  -EFAULT;
	}

	return ret;

}
static ssize_t app_lseek(struct file *filp, loff_t off, int whence)
{
	
}


//arg: data format should be 'char' for 'SET_XXX' and 'char *' for 'GET_XXX'
static int app_ioctl(struct inode *inodep, struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned char val;
	bma020acc_t acc_data;

	printk(KERN_ALERT"##!!##ioctl entry\n"); //JP
	//printk(KERN_ALERT"cmd: %d  arg: %d\n",cmd , (*(int *)arg)); //JP
	switch (cmd)
	{
		case SET_RST:
		case SET_NORMAL:
			bma020_set_mode(BMA020_MODE_NORMAL);
			msleep(1);		//delay for 1ms
			//bma020_soft_reset();
			msleep(2);		//delay for >=1.3ms
			bma020_reset_interrupt();
			break;

		case GET_MODE://return BMA020_MODE_NORMAL, BMA020_MODE_SLEEP, BMA020_MODE_WAKE_UP
			val=bma020_get_mode();
			if(copy_to_user((char*)arg,&val,sizeof(val)))
				return -EFAULT;
			break;

		case SET_SLEEP:
			bma020_set_mode(BMA020_MODE_SLEEP);
			break;

		case SET_WAKEUP_DUR://para BMA020_WAKE_UP_PAUSE_20MS, BMA020_WAKE_UP_PAUSE_80MS,
					//BMA020_WAKE_UP_PAUSE_320MS, BMA020_WAKE_UP_PAUSE_2560MS
			val = (unsigned char)arg;
			bma020_set_wake_up_pause(val);
			break;

		case SET_AUTO_WAKEUP:
			bma020_set_mode(BMA020_MODE_WAKE_UP);
			break;

		case GET_G://return BMA020_RANGE_2G;BMA020_RANGE_4G;BMA020_RANGE_8G
			bma020_get_range(&val);
			printk(KERN_ALERT"get range: %x \n", val); //JP
			if(copy_to_user((char*)arg,&val,sizeof(val)))
				return -EFAULT;
			break;

		case SET_G://para BMA020_RANGE_2G;BMA020_RANGE_4G;BMA020_RANGE_8G
			val = (unsigned char)arg;
			bma020_set_range(val);
			break;

		case SET_BANDWIDTH:
			val = (unsigned char)arg;
			bma020_set_bandwidth(val);
			break;

		case GET_BANDWIDTH:
			bma020_get_bandwidth(&val);
			if(copy_to_user((char*)arg,&val,sizeof(val)))
				return -EFAULT;
			break;

		case GET_WAKEUP_DUR://BMA020_WAKE_UP_PAUSE_20MS, BMA020_WAKE_UP_PAUSE_80MS,
					//BMA020_WAKE_UP_PAUSE_320MS, BMA020_WAKE_UP_PAUSE_2560MS
			bma020_get_wake_up_pause(&val);
			if(copy_to_user((char*)arg,&val,sizeof(val)))
				return -EFAULT;
			break;

		case SET_LOW_G_THRESHOULD://ref to BMA020_LG_THRES_IN_G()
			val = (unsigned char)arg;
			#ifdef DEBUG_INFO
			printk("##!!## low g thr= %d \n",val);
			#endif
			bma020_set_low_g_threshold(val);
			break;

		case GET_LOW_G_THRESHOULD://
			bma020_get_low_g_threshold(&val);
			if(copy_to_user((char*)arg,&val,sizeof(val)))
				return -EFAULT;
			break;

		case SET_LOW_G_COUNTDOWN:
			val = (unsigned char)arg;
			bma020_set_low_g_countdown(val);
			break;

		case GET_LOW_G_COUNTDOWN:
			bma020_get_low_g_countdown(&val);
			if(copy_to_user((char*)arg,&val,sizeof(val)))
				return -EFAULT;
			break;


		case SET_LOW_G_DUR:
			val = (unsigned char)arg;
			bma020_set_low_g_duration(val);
			break;

		case GET_LOW_G_DUR:
			bma020_get_low_g_duration(&val);
			if(copy_to_user((char*)arg,&val,sizeof(val)))
				return -EFAULT;
			break;

		case SET_HIGH_G_DUR:
			val = (unsigned char)arg;
			bma020_set_high_g_duration(val);
			break;

		case GET_HIGH_G_DUR:
			bma020_get_high_g_duration(&val);
			if(copy_to_user((char*)arg,&val,sizeof(val)))
				return -EFAULT;
			break;

		case SET_HIGH_G_THRESHOULD://BMA020_HG_THRES_IN_G()
			val = (unsigned char)arg;
			#ifdef DEBUG_INFO
			printk("##!!## high g thr= %d \n",val);
			#endif
			bma020_set_high_g_threshold(val);
			break;

		case GET_HIGH_G_THRESHOULD:
			bma020_get_high_g_threshold(&val);
			if(copy_to_user((char*)arg,&val,sizeof(val)))
				return -EFAULT;
			break;

		case SET_HIGH_G_COUNTDOWN:
			val = (unsigned char)arg;
			bma020_set_high_g_countdown(val);
			break;

		case GET_HIGH_G_COUNTDOWN:
			bma020_get_high_g_countdown(&val);
			if(copy_to_user((char*)arg,&val,sizeof(val)))
				return -EFAULT;
			break;

		case SET_HIGH_G_HYST:
			val = (unsigned char)arg;
			bma020_set_high_g_hyst(val);
			break;
		case SET_LOW_G_HYST:
			val = (unsigned char)arg;
			bma020_set_low_g_hyst(val);
			break;

		case SET_ANY_MOTION_THRESHOULD://BMA020_ANY_MOTION_THRES_IN_G()
			val = (unsigned char)arg;
			bma020_set_any_motion_threshold(val);
			break;

		case GET_ANY_MOTION_THRESHOULD://BMA020_ANY_MOTION_THRES_IN_G()
			bma020_get_any_motion_threshold(&val);
			if(copy_to_user((char*)arg,&val,sizeof(val)))
				return -EFAULT;
			break;

		case SET_ANY_MOTION_COUNT://BMA020_ANY_MOTION_DUR_1, BMA020_ANY_MOTION_DUR_3,
						//BMA020_ANY_MOTION_DUR_5, BMA020_ANY_MOTION_DUR_7
			val = (unsigned char)arg;
			bma020_set_any_motion_count(val);
			break;

		case GET_ANY_MOTION_COUNT://BMA020_ANY_MOTION_DUR_1, BMA020_ANY_MOTION_DUR_3,
						//BMA020_ANY_MOTION_DUR_5, BMA020_ANY_MOTION_DUR_7
			bma020_get_any_motion_count(&val);
			if(copy_to_user((char*)arg,&val,sizeof(val)))
				return -EFAULT;
			break;

		/*
		case SET_INTERRUPT_MASK://BMA020_INT_ALERT, BMA020_INT_ANY_MOTION, BMA020_INT_EN_ADV_INT,
					//BMA020_INT_NEW_DATA, BMA020_INT_LATCH, BMA020_INT_HG, BMA020_INT_LG
			val = (unsigned char)arg;
			bma020_set_interrupt_mask(val);
			break;

		case GET_INTERRUPT_MASK://BMA020_INT_ALERT, BMA020_INT_ANY_MOTION, BMA020_INT_EN_ADV_INT,
					//BMA020_INT_NEW_DATA, BMA020_INT_LATCH, BMA020_INT_HG, BMA020_INT_LG
			bma020_get_interrupt_mask(&val);
			if(copy_to_user((char*)arg,&val,sizeof(val)))
				return -EFAULT;
			break;
		*/

		case RESET_INT://resets the BMA020 interrupt status, can be used to reset a latched interrupt
			bma020_reset_interrupt();
			break;

		case READ_ACCEL_XYZ:// X,Y and Z-axis acceleration data readout, ref bma020acc_t structure for x,y,z data readout
			bma020_read_accel_xyz(&acc_data);
			//debug
			/*
			printk("x=%d:  ",acc_data.x);
			printk("y=%d:  ",acc_data.y);
			printk("z=%d:  ",acc_data.z);
			printk("\n");
			*/
			if(copy_to_user((char *)arg, &acc_data, sizeof(acc_data)))
				return -EFAULT;
			break;

		case GET_INT_STATUS://BMA020_INT_STATUS_HG, BMA020_INT_STATUS_LG, BMA020_INT_STATUS_HG_LATCHED,
					//BMA020_INT_STATUS_LG_LATCHED, BMA020_INT_STATUS_ALERT, BMA020_INT_STATUS_ST_RESULT
			bma020_get_interrupt_status(&val);
			if(copy_to_user((char*)arg,&val,sizeof(val)))
				return -EFAULT;
			break;

		case SET_LOW_G_INT://param onoff, enable=1, disable=0
			val = (unsigned char)arg;
			bma020_set_low_g_int(val);
			break;

		case SET_HIGH_G_INT://param onoff enable=1, disable=0
			val = (unsigned char)arg;
			bma020_set_high_g_int(val) ;
			break;

		case SET_ANY_MOTION_INT://param onoff enable=1, disable=0
			val = (unsigned char)arg;
			bma020_set_any_motion_int(val);
			break;

		case SET_ADVANCED_INT://param onoff enable=1, disable=0
			val = (unsigned char)arg;
			bma020_set_advanced_int(val) ;
			break;

		case LATCH_INT://param latched (=1 for latched interrupts), (=0 for unlatched interrupts)
			val = (unsigned char)arg;
			bma020_latch_int(val) ;
			break;

		case SET_NEW_DATA_INT://param onoff enable=1, disable=0
			val = (unsigned char)arg;
			bma020_set_new_data_int(val);
			break;

		case WAIT_FOR_INT:

			interruptible_sleep_on(&(p_bma020->int_wait));

			if(signal_pending(current))
				return -ERESTARTSYS;
			bma020_read_accel_xyz(&acc_data);
			if(copy_to_user((char *)arg, &acc_data, sizeof(acc_data)))
				return -EFAULT;
			//debug
			/*
			printk("x=%d:  ",acc_data.x);
			printk("y=%d:  ",acc_data.y);
			printk("z=%d:  ",acc_data.z);
			printk("\n");
			*/
			break;

		default:
			return  - EINVAL;
	}
//printk("##!!##ioctl quit\n");
	return 0;
}


/*************************************************************/
#ifdef INPUT_POLL
static void bma020_idev_poll(struct input_polled_dev *dev)
{
	bma020acc_t acc_data;
	struct input_dev *idev;

	mutex_lock(&bma020_lock);

	idev=dev->input;

	bma020_read_accel_xyz(&acc_data);

	//debug
	#ifdef DEBUG_INPUT_POLL
	printk("x=%d:  ",acc_data.x);
	printk("y=%d:  ",acc_data.y);
	printk("z=%d:  ",acc_data.z);
	printk("\n");
	#endif

	input_report_rel(idev, REL_X, acc_data.x);
	input_report_rel(idev, REL_Y, acc_data.y);
	input_report_rel(idev, REL_Z, acc_data.z);
	input_sync(idev);

	mutex_unlock(&bma020_lock);

}
#endif


#define BMA020_POLL_INTERVAL 200  //msec


//***************************cdev register****************************//
static const struct file_operations app_fops =
{
	  .owner = THIS_MODULE,
	  .read = app_read,
	  //.write=,
	  .ioctl = app_ioctl,
	  //.poll = ,
	  .open = app_open,
	  .release = app_release,
	  .fasync = app_fasync,
};


static int register_as_cdev(void)//return 0 for success
{
	int ret,devno;

	p_bma020->app_major=55;		//major dev no.

	//apply for dev no.
	devno = MKDEV(p_bma020->app_major, 0);
	ret = register_chrdev_region(devno, 1, "bma020_device");//

	#if 0
	if (p_bma020->app_major ){
		devno = MKDEV(p_bma020->app_major, 0);
		ret = register_chrdev_region(devno, 1, "bma020_device");
	}
	else {
		ret = alloc_chrdev_region(&devno, 0, 1, "bma020_device");
		p_bma020->app_major = MAJOR(devno);
	}
	#endif

	if (ret < 0){
		printk(KERN_ERR "Failed to register bma020 cdev.\n");
		goto ERROR0;
	}
	printk(KERN_ALERT"register bma cdev, major: %d \n",p_bma020->app_major);

	cdev_init(&(p_bma020->bma_cdev), &app_fops);
	p_bma020->bma_cdev.owner = THIS_MODULE;

	ret = cdev_add(&(p_bma020->bma_cdev), devno, 1);//
	if (ret){
		printk(KERN_ERR "Error cdev bma020 added\n");
		goto ERROR1;
	}

	return 0;
ERROR1:
	unregister_chrdev_region(MKDEV(p_bma020->app_major, 0), 1);
ERROR0:
	return ret;
}

static void unregister_as_cdev(void)
{
	cdev_del(&(p_bma020->bma_cdev));
	unregister_chrdev_region(MKDEV(p_bma020->app_major, 0), 1);
	printk(KERN_ALERT"Driver unregistered!\n");
}
#ifdef INPUT_POLL
static int register_as_hwmon(void)
{
	struct input_dev *idev;
	int ret;

	//register input poll dev

	p_bma020->bma020_idev = input_allocate_polled_device();//
	if (!(p_bma020->bma020_idev)) {
		ret = -ENOMEM;
		goto ERROR2;
	}

	p_bma020->bma020_idev->poll = bma020_idev_poll;
	p_bma020->bma020_idev->poll_interval = BMA020_POLL_INTERVAL;

	/* initialize the input device */
	idev=p_bma020->bma020_idev->input;
	idev->name = "bma020_input";
	idev->phys = "bma020/input0";
	idev->dev.parent = & g_client->dev;

	idev->id.bustype = BUS_HOST;

	input_set_capability(idev, EV_REL, REL_X);
	input_set_capability(idev, EV_REL, REL_Y);
	input_set_capability(idev, EV_REL, REL_Z);

	ret = input_register_polled_device(p_bma020->bma020_idev);//

	if (ret)
		goto ERROR3;

	//initialize sensor to normal operation mode
	bma020_set_mode(BMA020_MODE_NORMAL);	//set to normal mode
	msleep(2);		//delay for 1ms
	bma020_soft_reset();
	msleep(2);		//delay for >=1.3ms;
	bma020_set_range(BMA020_RANGE_2G);
	bma020_set_bandwidth(BMA020_BW_25HZ);
	bma020_set_shadow_dis(BMA020_SHADOW_DIS_ON);	//free read sequence

	return 0;

ERROR3:
	 input_free_polled_device(p_bma020->bma020_idev);
ERROR2:
	return ret;

}
#endif

void unregister_as_hwmon(void)
{
	input_unregister_polled_device(p_bma020->bma020_idev);
	input_free_polled_device(p_bma020->bma020_idev);
}




static int __init app_init(void)
{
	int ret;
	bma020_t *new_bma020=NULL;

	printk(KERN_ALERT"ima020 init..... \n");
	//*****************initialize bma020 device*******************
	new_bma020 = kzalloc(sizeof(bma020_t), GFP_KERNEL );

	if ( !new_bma020 )  {
		printk(KERN_ALERT"Failed to allocate bma020 device.\n");
		ret = -ENOMEM;
		goto ERROR0;
	}
	//initialize global ptr
	p_bma020=new_bma020;

	//===================================================
#ifndef INPUT_POLL
	//register as char device, combine file_operations
	ret = register_as_cdev();//
	if(ret)
		goto ERROR1;
	else
		printk(KERN_ALERT"register as cdev\n");
#endif
	//attach to i2c bus
	ret = i2c_bma020_init();//
	if(ret){
	#ifdef INPUT_POLL
		goto ERROR1;
	#else
		goto ERROR2;
	#endif
	}
	printk(KERN_ALERT"All inited\n");
#ifdef INPUT_POLL
	//register as input hardware monitor device, create input poll event
	ret = register_as_hwmon();//
	if(ret)
		goto ERROR3;
#endif

	return 0;

#ifdef INPUT_POLL
ERROR3:
	i2c_bma020_exit();
#endif

#ifndef INPUT_POLL
ERROR2:
	unregister_as_cdev();
#endif

ERROR1:
	kfree(new_bma020);

ERROR0:
	p_bma020=NULL;
	return ret;

}

static void __exit  app_exit(void)
{
#ifdef INPUT_POLL
	unregister_as_hwmon();
#endif
	i2c_bma020_exit();
#ifndef INPUT_POLL
	unregister_as_cdev();
#endif
	kfree(p_bma020);
	p_bma020=NULL;
}



module_init(app_init);
module_exit(app_exit);

MODULE_DESCRIPTION("BMA020 I2C Client driver");
MODULE_LICENSE("GPL");


