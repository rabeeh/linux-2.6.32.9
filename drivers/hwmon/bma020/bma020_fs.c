/*
*  bma020_fs.c - ST BMA020 accelerometer driver
*
*  Copyright (C) 2007-2008 Yan Burman
*  Copyright (C) 2008 Eric Piel
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/dmi.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/freezer.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/i2c.h>
#include <asm/atomic.h>
#include <asm/mach-types.h>
#include <linux/fs.h>
#include <asm/system.h>
#include <mach/gpio.h>
//#include <mach/gpio_ec.h>  //JP
//#include <mach/mfp-pxa168.h> //JP
#include <mach/irqs.h>

#include "bma020_fs.h"
/* #define DEBUG_INFO  */

static struct i2c_bma020 bma_dev;
static struct i2c_client *g_client;

//**********************i2c bus function*****************************//

static int i2c_bma020_read_buf(u8 reg_addr, u8 *buf, u8 cnt)/* ret 0 for success */
{
	int ret = 0;

	if (NULL == g_client)	/* No global client pointer? */
		return -EFAULT;
	/*
	if (-1 == set_bus_busy())
		return -EINVAL;
	*/
	ret = i2c_master_send(g_client, &reg_addr, 1);/* i2c_smbus_write_byte(g_client,reg_addr); */
	/* set_bus_free(); */

	if (ret < 0)
		return -EIO;

	/*
	if (-1 == set_bus_busy())
		return -EINVAL;
	*/
	ret = i2c_master_recv(g_client, buf, cnt);
	/* set_bus_free(); */

	if (ret < 0)
		return -EIO;

	return 0;
}

static int i2c_bma020_write_buf(u8 reg_addr, u8 *buf, u8 cnt)/* ret 0 for success */
{
	int ret = 0;
	int status;

	if (NULL == g_client)	/* No global client pointer? */
		return -EFAULT;

	/*
	if (-1 == set_bus_busy())
		return -EINVAL;
	*/
	ret = i2c_smbus_write_byte_data(g_client, reg_addr, *buf);/* 0 for success */
	/* set_bus_free(); */

	if (0 == ret)
		return 0;
	else
		return -EIO;
}

//*************************** low level operation **************************************//
static int bma020_detect(void)
{
	int comres = 0;
	unsigned char data;
	/* get chip info. */
	bma_dev.dev_addr = BMA020_I2C_ADDR;	/* preset SM380 I2C_addr */
	comres |= i2c_bma020_read_buf(CHIP_ID__REG, &data, 1);/* read Chip Id */
	bma_dev.chip_id = BMA020_GET_BITSLICE(data, CHIP_ID);	/* get bitslice */
	comres |= i2c_bma020_read_buf(ML_VERSION__REG, &data, 1); /* read Version reg */
	bma_dev.ml_version = BMA020_GET_BITSLICE(data, ML_VERSION);	/* get ML Version */
	bma_dev.al_version = BMA020_GET_BITSLICE(data, AL_VERSION);/* get AL Version */

	return comres;
}

/** Perform soft reset of BMA020 via bus command
*/
int bma020_soft_reset(void)
{
	int comres;
	unsigned char data = 0;
	data = BMA020_SET_BITSLICE(data, SOFT_RESET, 1);
	comres = i2c_bma020_write_buf(SOFT_RESET__REG, &data, 1);
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
	comres = i2c_bma020_read_buf(SELF_TEST__REG, &data, 1);
	data = BMA020_SET_BITSLICE(data, SELF_TEST, st);
	comres |= i2c_bma020_write_buf(SELF_TEST__REG, &data, 1);
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

	if (range < 3) {
		comres = i2c_bma020_read_buf(RANGE__REG, &data, 1);
		data = BMA020_SET_BITSLICE(data, RANGE, range);
		comres |= i2c_bma020_write_buf(RANGE__REG, &data, 1);
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

	comres = i2c_bma020_read_buf(RANGE__REG, range, 1);
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
int bma020_set_mode(unsigned char mode)
{
	int comres = 0;
	unsigned char data1, data2;

	if (mode < 4 || mode != 1) {
		comres = i2c_bma020_read_buf(WAKE_UP__REG, &data1, 1);
		data1  = BMA020_SET_BITSLICE(data1, WAKE_UP, mode);
		comres |= i2c_bma020_read_buf(SLEEP__REG, &data2, 1);
		data2  = BMA020_SET_BITSLICE(data2, SLEEP, (mode >> 1));

		comres |= i2c_bma020_write_buf(WAKE_UP__REG, &data1, 1);
		comres |= i2c_bma020_write_buf(SLEEP__REG, &data2, 1);
		bma_dev.mode = mode;
	}
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
	return bma_dev.mode;
}

/** set BMA020 internal filter bandwidth
\param bw bandwidth (see bandwidth constants)
\return result of bus communication function
\see #define BMA020_BW_25HZ, BMA020_BW_50HZ, BMA020_BW_100HZ,
\BMA020_BW_190HZ, BMA020_BW_375HZ,
\BMA020_BW_750HZ, BMA020_BW_1500HZ
\see bma020_get_bandwidth()
*/
int bma020_set_bandwidth(char bw)
{
	int comres = 0;
	unsigned char data;
	if (bw < 8) {
		comres = i2c_bma020_read_buf(BANDWIDTH__REG, &data, 1);
		data = BMA020_SET_BITSLICE(data, BANDWIDTH, bw);
		comres |= i2c_bma020_write_buf(BANDWIDTH__REG, &data, 1);
	}
	return comres;
}

/** read selected bandwidth from BMA020
\param *bw pointer to bandwidth return value
\return result of bus communication function
\see #define BMA020_BW_25HZ, BMA020_BW_50HZ, BMA020_BW_100HZ,
\BMA020_BW_190HZ, BMA020_BW_375HZ,BMA020_BW_750HZ, BMA020_BW_1500HZ
\see bma020_set_bandwidth()
*/
int bma020_get_bandwidth(unsigned char *bw)
{
	int comres = 1;

	comres = i2c_bma020_read_buf(BANDWIDTH__REG, bw, 1);
	*bw = BMA020_GET_BITSLICE(*bw, BANDWIDTH);
	return comres;
}

/** set BMA020 auto wake up pause
\param wup wake_up_pause parameters
\return result of bus communication function
\see BMA020_WAKE_UP_PAUSE_20MS, BMA020_WAKE_UP_PAUSE_80MS,
\BMA020_WAKE_UP_PAUSE_320MS,BMA020_WAKE_UP_PAUSE_2560MS
\see bma020_get_wake_up_pause()
*/

int bma020_set_wake_up_pause(unsigned char wup)
{
	int comres = 0;
	unsigned char data;

	comres = i2c_bma020_read_buf(WAKE_UP_PAUSE__REG, &data, 1);
	data = BMA020_SET_BITSLICE(data, WAKE_UP_PAUSE, wup);
	comres |= i2c_bma020_write_buf(WAKE_UP_PAUSE__REG, &data, 1);
	return comres;
}

/** read BMA020 auto wake up pause from image
\param *wup wake up pause read back pointer
\see BMA020_WAKE_UP_PAUSE_20MS, BMA020_WAKE_UP_PAUSE_80MS,
\BMA020_WAKE_UP_PAUSE_320MS,BMA020_WAKE_UP_PAUSE_2560MS
\see bma020_set_wake_up_pause()
*/
int bma020_get_wake_up_pause(unsigned char *wup)
{
	int comres = 1;
	unsigned char data;
	comres = i2c_bma020_read_buf(WAKE_UP_PAUSE__REG, &data,  1);
	*wup = BMA020_GET_BITSLICE(data, WAKE_UP_PAUSE);
	return comres;
}

/* Thresholds and Interrupt Configuration */
/** set low-g interrupt threshold
\param th set the threshold
\note the threshold depends on configured range. A macro
\ref BMA020_LG_THRES_IN_G() for range to
\register value conversion is available.
\see BMA020_LG_THRES_IN_G()
\see bma020_get_low_g_threshold()
*/
int bma020_set_low_g_threshold(unsigned char th)
{
	int comres;
	comres = i2c_bma020_write_buf(LG_THRES__REG, &th, 1);
	return comres;
}

/** get low-g interrupt threshold
\param *th get the threshold  value from sensor image
\see bma020_set_low_g_threshold()
*/
int bma020_get_low_g_threshold(unsigned char *th)
{
	int comres = 1;
	comres = i2c_bma020_read_buf(LG_THRES__REG, th, 1);
	return comres;
}

/** set low-g interrupt countdown
\param cnt get the countdown value from sensor image
\see bma020_get_low_g_countdown()
*/
int bma020_set_low_g_countdown(unsigned char cnt)
{
	int comres = 0;
	unsigned char data;
	comres = i2c_bma020_read_buf(COUNTER_LG__REG, &data, 1);
	data = BMA020_SET_BITSLICE(data, COUNTER_LG, cnt);
	comres |= i2c_bma020_write_buf(COUNTER_LG__REG, &data, 1);
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
	comres = i2c_bma020_read_buf(COUNTER_LG__REG, &data,  1);
	*cnt = BMA020_GET_BITSLICE(data, COUNTER_LG);
	return comres;
}

/** set high-g interrupt countdown
\param cnt get the countdown value from sensor image
\see bma020_get_high_g_countdown()
*/
int bma020_set_high_g_countdown(unsigned char cnt)
{
	int comres = 1;
	unsigned char data;
	comres = i2c_bma020_read_buf(COUNTER_HG__REG, &data, 1);
	data = BMA020_SET_BITSLICE(data, COUNTER_HG, cnt);
	comres |= i2c_bma020_write_buf(COUNTER_HG__REG, &data, 1);
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
	comres = i2c_bma020_read_buf(COUNTER_HG__REG, &data,  1);
	*cnt = BMA020_GET_BITSLICE(data, COUNTER_HG);
	return comres;
}

/** configure low-g duration value
\param dur low-g duration in miliseconds
\see bma020_get_low_g_duration(), bma020_get_high_g_duration(),
\bma020_set_high_g_duration()
*/
int bma020_set_low_g_duration(unsigned char dur)
{
	int comres = 0;
	comres = i2c_bma020_write_buf(LG_DUR__REG, &dur, 1);
	return comres;
}

int bma020_set_low_g_hyst(unsigned char hyst)
{
	int comres = 0;
	unsigned char data;

	comres = i2c_bma020_read_buf(LG_HYST__REG, &data, 1);
	data = BMA020_SET_BITSLICE(data, LG_HYST , hyst);
#ifdef DEBUG_INFO
	printk(KERN_INFO"hyst=0x%x\n", data);
#endif
	comres |= i2c_bma020_write_buf(LG_HYST__REG, &data, 1);
	return comres;
}

int bma020_set_high_g_hyst(unsigned char hyst)
{
	int comres = 0;
	unsigned char data;

	comres = i2c_bma020_read_buf(HG_HYST__REG, &data, 1);
	data = BMA020_SET_BITSLICE(data, HG_HYST , hyst);
#ifdef DEBUG_INFO
	printk(KERN_INFO"hyst=0x%x\n", data);
#endif
	comres |= i2c_bma020_write_buf(HG_HYST__REG, &data, 1);
	return comres;
}

/** read out low-g duration value from sensor image
\param dur low-g duration in miliseconds
\see bma020_set_low_g_duration(), bma020_get_high_g_duration(),
\bma020_set_high_g_duration()
*/
int bma020_get_low_g_duration(unsigned char *dur)
{
	int comres = 0;
	comres = i2c_bma020_read_buf(LG_DUR__REG, dur, 1);
	return comres;
}

/** set low-g interrupt threshold
\param th set the threshold
\note the threshold depends on configured range. A macro
\ref BMA020_HG_THRES_IN_G() for range to
\register value conversion is available.
\see BMA020_HG_THRES_IN_G()
\see bma020_get_high_g_threshold()
*/
int bma020_set_high_g_threshold(unsigned char th)
{
	int comres = 0;
	comres = i2c_bma020_write_buf(HG_THRES__REG, &th, 1);
	return comres;
}

/** get high-g interrupt threshold
\param *th get the threshold  value from sensor image
\see bma020_set_high_g_threshold()
*/
int bma020_get_high_g_threshold(unsigned char *th)
{
	int comres = 0;
	comres = i2c_bma020_read_buf(HG_THRES__REG, th, 1);
	return comres;
}

/** configure high-g duration value
\param dur high-g duration in miliseconds
\see  bma020_get_high_g_duration(), bma020_set_low_g_duration(),
\bma020_get_low_g_duration()
*/
int bma020_set_high_g_duration(unsigned char dur)
{
	int comres = 0;
	comres = i2c_bma020_write_buf(HG_DUR__REG, &dur, 1);
	return comres;
}

/** read out high-g duration value from sensor image
\param dur high-g duration in miliseconds
\see  bma020_set_high_g_duration(), bma020_get_low_g_duration(), bma020_set_low_g_duration(),
*/
int bma020_get_high_g_duration(unsigned char *dur)
{
	int comres = 0;
	comres = i2c_bma020_read_buf(HG_DUR__REG, dur, 1);
	return comres;
}

/**  set threshold value for any_motion feature
\param th set the threshold a macro \ref BMA020_ANY_MOTION_THRES_IN_G()  is available for that
\see BMA020_ANY_MOTION_THRES_IN_G()
*/
int bma020_set_any_motion_threshold(unsigned char th)
{
	int comres = 0;
	comres = i2c_bma020_write_buf(ANY_MOTION_THRES__REG, &th, 1);
	return comres;
}

/**  get threshold value for any_motion feature
\param *th read back any_motion threshold from image register
\see BMA020_ANY_MOTION_THRES_IN_G()
*/
int bma020_get_any_motion_threshold(unsigned char *th)
{
	int comres = 0;
	comres = i2c_bma020_read_buf(ANY_MOTION_THRES__REG, th, 1);
	return comres;
}

/**  set counter value for any_motion feature
\param amc set the counter value, constants are available for that
\see BMA020_ANY_MOTION_DUR_1, BMA020_ANY_MOTION_DUR_3, BMA020_ANY_MOTION_DUR_5,
\BMA020_ANY_MOTION_DUR_7
*/
int bma020_set_any_motion_count(unsigned char amc)
{
	int comres = 0;
	unsigned char data;
	comres = i2c_bma020_read_buf(ANY_MOTION_DUR__REG, &data, 1);
	data = BMA020_SET_BITSLICE(data, ANY_MOTION_DUR, amc);
	comres = i2c_bma020_write_buf(ANY_MOTION_DUR__REG, &data, 1);
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
	comres = i2c_bma020_read_buf(ANY_MOTION_DUR__REG, &data,  1);
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
	int comres = 0;
	unsigned char data[4];
	data[0] = mask & BMA020_CONF1_INT_MSK;
	data[2] = ((mask<<1) & BMA020_CONF2_INT_MSK);
	comres = i2c_bma020_read_buf(BMA020_CONF1_REG, &data[1], 1);
	comres |= i2c_bma020_read_buf(BMA020_CONF2_REG, &data[3], 1);
	data[1] &= (~BMA020_CONF1_INT_MSK);
	data[1] |= data[0];
	data[3] &= (~(BMA020_CONF2_INT_MSK));
	data[3] |= data[2];
	comres |= i2c_bma020_write_buf(BMA020_CONF1_REG, &data[1], 1);
	comres |= i2c_bma020_write_buf(BMA020_CONF2_REG, &data[3], 1);
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
	if ((ssd == 1) || (ssd == 0)) {
		comres = i2c_bma020_read_buf(BMA020_CONF2_REG, &data, 1);
		data = BMA020_SET_BITSLICE(data, SHADOW_DIS, ssd);
		comres |= i2c_bma020_write_buf(BMA020_CONF2_REG, &data, 1);
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
	int comres = 0;
	unsigned char data;
	comres = i2c_bma020_read_buf(BMA020_CONF1_REG, &data, 1);
	*mask = data & BMA020_CONF1_INT_MSK;
	comres |= i2c_bma020_read_buf(BMA020_CONF2_REG, &data, 1);
	*mask = *mask | ((data & BMA020_CONF2_INT_MSK)>>1);
	return comres;
}

/** resets the BMA020 interrupt status
\note this feature can be used to reset a latched interrupt
*/
int bma020_reset_interrupt(void)
{
#ifdef DEBUG_INFO
	printk(KERN_INFO"file=%s, func=%s\n", __FILE__, __FUNCTION__);
#endif
	int comres = 0;
	unsigned char data = (1 << RESET_INT__POS);
	comres = i2c_bma020_write_buf(RESET_INT__REG, &data, 1);
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
	comres = i2c_bma020_read_buf(ACC_X_LSB__REG, data, 2);
	if (comres)
		return -EIO;
	*a_x = BMA020_GET_BITSLICE(data[0], ACC_X_LSB) |
		BMA020_GET_BITSLICE(data[1], ACC_X_MSB) << ACC_X_LSB__LEN;
	*a_x = *a_x << (sizeof(short)*8 - (ACC_X_LSB__LEN + ACC_X_MSB__LEN));
	*a_x = *a_x >> (sizeof(short)*8 - (ACC_X_LSB__LEN + ACC_X_MSB__LEN));
	return comres;
}

/** Y-axis acceleration data readout
\param *a_y pointer for 16 bit 2's complement data output (LSB aligned)
*/
int bma020_read_accel_y(short *a_y)
{
	int comres;
	unsigned char data[2];

	comres = i2c_bma020_read_buf(ACC_Y_LSB__REG, data, 2);
	if (comres)
		return -EIO;
	*a_y = BMA020_GET_BITSLICE(data[0], ACC_Y_LSB) |
		BMA020_GET_BITSLICE(data[1], ACC_Y_MSB) << ACC_Y_LSB__LEN;
	*a_y = *a_y << (sizeof(short)*8 - (ACC_Y_LSB__LEN + ACC_Y_MSB__LEN));
	*a_y = *a_y >> (sizeof(short)*8 - (ACC_Y_LSB__LEN + ACC_Y_MSB__LEN));
	return comres;
}

/** Z-axis acceleration data readout
\param *a_z pointer for 16 bit 2's complement data output (LSB aligned)
*/
int bma020_read_accel_z(short *a_z)
{
	int comres;
	unsigned char data[2];

	comres = i2c_bma020_read_buf(ACC_Z_LSB__REG, data, 2);
	if (comres)
		return -EIO;
	*a_z = BMA020_GET_BITSLICE(data[0], ACC_Z_LSB) |
		BMA020_GET_BITSLICE(data[1], ACC_Z_MSB) << ACC_Z_LSB__LEN;
	*a_z = *a_z << (sizeof(short)*8 - (ACC_Z_LSB__LEN + ACC_Z_MSB__LEN));
	*a_z = *a_z >> (sizeof(short)*8 - (ACC_Z_LSB__LEN + ACC_Z_MSB__LEN));
	return comres;
}

/** X,Y and Z-axis acceleration data readout
\param *acc pointer to \ref bma020acc_t structure for x,y,z data readout
\note data will be read by multi-byte protocol into a 6 byte structure
*/
int bma020_read_accel_xyz(bma020acc_t *acc)
{
	int comres;
	unsigned char data[6];
	comres = i2c_bma020_read_buf(ACC_X_LSB__REG, &data[0], 6);
	if (comres)
		return -EIO;
	acc->x = BMA020_GET_BITSLICE(data[0], ACC_X_LSB) |
		(BMA020_GET_BITSLICE(data[1], ACC_X_MSB) << ACC_X_LSB__LEN);
	acc->x = acc->x << (sizeof(short)*8 - (ACC_X_LSB__LEN + ACC_X_MSB__LEN));
	acc->x = acc->x >> (sizeof(short)*8 - (ACC_X_LSB__LEN + ACC_X_MSB__LEN));
	acc->y = BMA020_GET_BITSLICE(data[2], ACC_Y_LSB) |
		(BMA020_GET_BITSLICE(data[3], ACC_Y_MSB) << ACC_Y_LSB__LEN);
	acc->y = acc->y << (sizeof(short)*8 - (ACC_Y_LSB__LEN + ACC_Y_MSB__LEN));
	acc->y = acc->y >> (sizeof(short)*8 - (ACC_Y_LSB__LEN + ACC_Y_MSB__LEN));
	acc->z = BMA020_GET_BITSLICE(data[4], ACC_Z_LSB) |
		(BMA020_GET_BITSLICE(data[5], ACC_Z_MSB) << ACC_Z_LSB__LEN);
	acc->z = acc->z << (sizeof(short)*8 - (ACC_Z_LSB__LEN + ACC_Z_MSB__LEN));
	acc->z = acc->z >> (sizeof(short)*8 - (ACC_Z_LSB__LEN + ACC_Z_MSB__LEN));
	return comres;
}

/** check current interrupt status from interrupt status register in BMA020 image register
\param *ist pointer to interrupt status byte
\see BMA020_INT_STATUS_HG, BMA020_INT_STATUS_LG, BMA020_INT_STATUS_HG_LATCHED,
\BMA020_INT_STATUS_LG_LATCHED, BMA020_INT_STATUS_ALERT, BMA020_INT_STATUS_ST_RESULT
*/
int bma020_get_interrupt_status(unsigned char *ist)
{
	int comres = 0;
	comres = i2c_bma020_read_buf(BMA020_STATUS_REG, ist, 1);
	return comres;
}

/** enable/ disable low-g interrupt feature
\param onoff enable=1, disable=0
*/
int bma020_set_low_g_int(unsigned char onoff)
{
	int comres;
	unsigned char data;

	comres = i2c_bma020_read_buf(ENABLE_LG__REG, &data, 1);
	data = BMA020_SET_BITSLICE(data, ENABLE_LG, onoff);
	comres |= i2c_bma020_write_buf(ENABLE_LG__REG, &data, 1);
	return comres;
}

/** enable/ disable high-g interrupt feature
\param onoff enable=1, disable=0
*/
int bma020_set_high_g_int(unsigned char onoff)
{
	int comres;
	unsigned char data;

	comres = i2c_bma020_read_buf(ENABLE_HG__REG, &data, 1);
	data = BMA020_SET_BITSLICE(data, ENABLE_HG, onoff);
	comres |= i2c_bma020_write_buf(ENABLE_HG__REG, &data, 1);
	return comres;
}

/** enable/ disable any_motion interrupt feature
\param onoff enable=1, disable=0
\note for any_motion interrupt feature usage see also \ref bma020_set_advanced_int()
*/
int bma020_set_any_motion_int(unsigned char onoff)
{
	int comres;
	unsigned char data;
	comres = i2c_bma020_read_buf(EN_ANY_MOTION__REG, &data, 1);
	data = BMA020_SET_BITSLICE(data, EN_ANY_MOTION, onoff);
	comres |= i2c_bma020_write_buf(EN_ANY_MOTION__REG, &data, 1);
	return comres;
}

/** enable/ disable alert-int interrupt feature
\param onoff enable=1, disable=0
\note for any_motion interrupt feature usage see also \ref bma020_set_advanced_int()
*/
int bma020_set_alert_int(unsigned char onoff)
{
	int comres;
	unsigned char data;

	comres = i2c_bma020_read_buf(ALERT__REG, &data, 1);
	data = BMA020_SET_BITSLICE(data, ALERT, onoff);
	comres |= i2c_bma020_write_buf(ALERT__REG, &data, 1);
	return comres;
}

/** enable/ disable advanced interrupt feature
\param onoff enable=1, disable=0
\see bma020_set_any_motion_int()
\see bma020_set_alert_int()
*/
int bma020_set_advanced_int(unsigned char onoff)
{
	int comres;
	unsigned char data;
	comres = i2c_bma020_read_buf(ENABLE_ADV_INT__REG, &data, 1);
	data = BMA020_SET_BITSLICE(data, EN_ANY_MOTION, onoff);
	comres |= i2c_bma020_write_buf(ENABLE_ADV_INT__REG, &data, 1);
	return comres;
}

/** enable/disable latched interrupt for all interrupt feature (global option)
\param latched (=1 for latched interrupts), (=0 for unlatched interrupts)
*/

int bma020_latch_int(unsigned char latched)
{
	int comres;
	unsigned char data;
	comres = i2c_bma020_read_buf(LATCH_INT__REG, &data, 1);

#ifdef DEBUG_INFO
	printk("read1=0x%x\n", data);
#endif
	data = BMA020_SET_BITSLICE(data, LATCH_INT, latched);
#ifdef DEBUG_INFO
	printk("read2=0x%x\n", data);
#endif
	comres |= i2c_bma020_write_buf(LATCH_INT__REG, &data, 1);
	return comres;
}

int bma020_set_new_data_int(unsigned char onoff)
{
	/* printk("bma020_set_new_data_int onoff:%d \n", onoff); */
	int comres;
	unsigned char data;
	comres = i2c_bma020_read_buf(NEW_DATA_INT__REG, &data, 1);
	data = BMA020_SET_BITSLICE(data, NEW_DATA_INT, onoff);
	comres |= i2c_bma020_write_buf(NEW_DATA_INT__REG, &data, 1);
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

	comres = i2c_bma020_read_buf(addr, data, len);
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
	comres = i2c_bma020_write_buf(addr, data, len);
	return comres;
}

/* Maximum value our axis may get for the input device (signed 10 bits) */
#define ACC_MAX_VAL 512

static int bma020_remove_fs(void);
static int bma020_add_fs(struct device *device);

/**
* bma020_get_axis - For the given axis, give the value converted
* @axis:      1,2,3 - can also be negative
* @hw_values: raw values returned by the hardware
*
* Returns the converted value.
*/
static inline int bma020_get_axis(s8 axis, int hw_values[3])
{
	if (axis > 0)
		return hw_values[axis - 1];
	else
		return -hw_values[-axis - 1];
}

#ifdef DEBUG_INFO
void bma020_driver_test(void)

{
	bma020acc_t data;
	int tmp;
	char s;
	int i = 20;
	bma020_soft_reset();
	tmp = bma020_selftest(0);
	printk("bustest num=%d\n", tmp);
	bma020_set_range(BMA020_RANGE_2G);
	/* bma020_set_range(BMA020_RANGE_8G); */
	bma020_set_mode(0);
	bma020_set_bandwidth(BMA020_BW_25HZ);

	while (i-- > 0) {
		bma020_get_range(&s);
		printk("range:%d\n", s);
		bma020_read_accel_xyz(&data);
		printk("x=%d:  ", data.x);
		printk("y=%d:  ", data.y);
		printk("z=%d:  ", data.z);
		printk("\n");
		msleep(1000);
	}
}
#endif

/**
* bma020_get_xyz_axis_adjusted - Get X, Y and Z axis values from the accelerometer
* @handle: the handle to the device
* @x:      where to store the X axis value
* @y:      where to store the Y axis value
* @z:      where to store the Z axis value
*
* Note that 40Hz input device can eat up about 10% CPU at 800MHZ
*/
static int bma020_get_xyz_axis_adjusted(int *x, int *y, int *z)
{
	int position[3];
	int ret;

	bma020acc_t acc;
	ret = bma020_read_accel_xyz(&acc);
	if (ret)
		return -EIO;
	position[0] = acc.x;
	position[1] = acc.y;
	position[2] = acc.z;

	*x = bma020_get_axis(bma_dev.ac.x, position);
	*y = bma020_get_axis(bma_dev.ac.y, position);
	*z = bma020_get_axis(bma_dev.ac.z, position);
	return 0;
}

static int bma020_poweroff(void)
{
	//printk(KERN_INFO"bma020 poweroff.\n");
	bma020_set_mode(BMA020_MODE_SLEEP);/* set to sleep mode instead of power off */
	return 0;
}

static int bma020_poweron(void)
{
	unsigned char data1, data2;
	int ret = 0;
	//printk(KERN_INFO"bma020 poweron.\n");

	ret |= bma020_set_mode(BMA020_MODE_NORMAL);	/* set to normal mode */
	msleep(2);		/* delay for 1ms */
	ret |= bma020_soft_reset();
	msleep(5);		/* delay for >=1.3ms; */
	ret |= bma020_set_range(bma_dev.range);
	ret |= bma020_set_bandwidth(bma_dev.bandwidth);
	/* bma020_set_shadow_dis(BMA020_SHADOW_DIS_ON);	//free read sequence */

	/* bma020_selftest(0); */
	/* test customer reserved reg at addr CUSTOMER1_REG, CUSTOMER2_REG */
	data1 = 0x55;
	data2 = 0xAA;
	ret |= i2c_bma020_write_buf(CUSTOMER_RESERVED1__REG, &data1, 1);
	ret |= i2c_bma020_write_buf(CUSTOMER_RESERVED2__REG, &data2, 1);
	ret |= i2c_bma020_read_buf(CUSTOMER_RESERVED1__REG, &data1, 1);
	ret |= i2c_bma020_read_buf(CUSTOMER_RESERVED2__REG, &data2, 1);
	if (ret)
		return ret;

	if ((0x55 != data1) || (0xAA != data2)) {
		//printk(KERN_ERR"bma020 poweron err.\n");
		bma020_set_mode(BMA020_MODE_SLEEP);	/* set to normal mode */
		return -1;
	}

	ret |= bma020_set_mode(BMA020_MODE_NORMAL);	/* set to normal mode */
	msleep(1);		/* delay for 1ms */
	return ret;
	/* bma020_soft_reset(); */
	/* msleep(5);		//delay for >=1.3ms; */
}


static int bma020_kthread(void *data)
{
	int x, y, z;
	int ret;
	static int x_last = 0, y_last = 0, z_last = 0;
	
	while (!kthread_should_stop()) {
		ret = bma020_get_xyz_axis_adjusted(&x, &y, &z);
		if (ret) {/* retry if this time failed */
			try_to_freeze();
			msleep_interruptible(bma_dev.sample_interval);
			return;
		}

		x = (x * 1 + x_last * 3) / 4;
		y = (y * 1 + y_last * 3) / 4;
		z = (z * 1 + z_last * 3) / 4;
		x_last = x;
		y_last = y;
		z_last = z;

#ifdef DEBUG_INFO
		//printk("x=%d, y=%d, z=%d \n", x, y, z);
#endif
#if 1
		input_report_abs(bma_dev.idev, ABS_X, y - bma_dev.xcalib);
		input_report_abs(bma_dev.idev, ABS_Y, x - bma_dev.ycalib);
		input_report_abs(bma_dev.idev, ABS_Z, -z - bma_dev.zcalib);
#else
		input_report_abs(bma_dev.idev, ABS_X, x);
		input_report_abs(bma_dev.idev, ABS_Y, y);
		input_report_abs(bma_dev.idev, ABS_Z, z);
#endif
		input_sync(bma_dev.idev);

		//try_to_freeze();  //masked by JP
		msleep_interruptible(bma_dev.sample_interval);
	}

}


/*
* To be called before starting to use the device. It makes sure that the
* device will always be on until a call to bma020_decrease_use(). Not to be
* used from interrupt context.
*/
static void bma020_increase_use(void)
{
	printk(KERN_INFO"%s\n", __FUNCTION__);
	mutex_lock(&bma_dev.lock);
	bma_dev.usage++;
	if (bma_dev.usage == 1) {
		if (!bma_dev.is_on) {
			if (!bma020_poweron()) {
				/* 0 for success power on */
				bma_dev.is_on = 1;
				bma_dev.kthread = kthread_run(bma020_kthread, NULL, "bma020");
			} else {
				bma_dev.is_on = 0;
				bma_dev.usage = 0;
			}
		}
	}
	mutex_unlock(&bma_dev.lock);
}

/*
* To be called whenever a usage of the device is stopped.
* It will make sure to turn off the device when there is not usage.
*/
static void bma020_decrease_use(void)
{
	printk(KERN_INFO"%s\n", __FUNCTION__);
	mutex_lock(&bma_dev.lock);
	if (bma_dev.usage == 0) {
		mutex_unlock(&bma_dev.lock);
		return;
	}
	bma_dev.usage--;
	if (bma_dev.usage == 0) {
		bma020_poweroff();
		bma_dev.is_on = 0;
		kthread_stop(bma_dev.kthread);
	}
	mutex_unlock(&bma_dev.lock);
}

static inline void bma020_calibrate_joystick(void)
{
	bma020_get_xyz_axis_adjusted(&bma_dev.xcalib, &bma_dev.ycalib, &bma_dev.zcalib);
}

static int bma020_joystick_enable(void)
{
	int err;

	if (bma_dev.idev)
		return -EINVAL;

	bma_dev.idev = input_allocate_device();
	if (!bma_dev.idev)
		return -ENOMEM;

	bma_dev.idev->name       = "BOSCH BMA020 Accelerometer";
	bma_dev.idev->name       = "bma020_input";
	bma_dev.idev->phys       = "bma020/input0";
	bma_dev.idev->id.bustype = BUS_I2C;
	bma_dev.idev->id.vendor  = 0;
	bma_dev.idev->dev.parent = bma_dev.device;
	/*
	   bma_dev.idev->open       = bma020_joystick_open;
	   bma_dev.idev->close      = bma020_joystick_close;
	   */
	__set_bit(EV_ABS, bma_dev.idev->evbit);
	__set_bit(ABS_X, bma_dev.idev->absbit);
	__set_bit(ABS_Y, bma_dev.idev->absbit);
	__set_bit(ABS_Z, bma_dev.idev->absbit);

	__set_bit(EV_SYN, bma_dev.idev->evbit);
	__set_bit(EV_KEY, bma_dev.idev->evbit);

	input_set_abs_params(bma_dev.idev, ABS_X,
			-ACC_MAX_VAL, ACC_MAX_VAL, 0, 0);
	input_set_abs_params(bma_dev.idev, ABS_Y,
			-ACC_MAX_VAL, ACC_MAX_VAL, 0, 0);
	input_set_abs_params(bma_dev.idev, ABS_Z,
			-ACC_MAX_VAL, ACC_MAX_VAL, 0, 0);

	err = input_register_device(bma_dev.idev);
	if (err) {
		printk(KERN_ERR "regist input driver error\n");
		input_free_device(bma_dev.idev);
		bma_dev.idev = NULL;
	}

	return err;
}

static void bma020_joystick_disable(void)
{
	if (!bma_dev.idev)
		return;

	input_unregister_device(bma_dev.idev);
	bma_dev.idev = NULL;
}


/*
* Initialise the accelerometer and the various subsystems.
* Should be rather independant of the bus system.
*/
static int bma020_init_device(struct i2c_bma020 *dev)
{
	bma020_add_fs(dev->device);

	if (bma020_joystick_enable())
		printk(KERN_ERR "bma020: joystick initialization failed\n");

	return 0;
}

static int bma020_dmi_matched(const struct dmi_system_id *dmi)
{
	bma_dev.ac = *((struct axis_conversion *)dmi->driver_data);
	printk(KERN_INFO "bma020: hardware type %s found.\n", dmi->ident);

	return 1;
}

/* Represents, for each axis seen by userspace, the corresponding hw axis (+1).
* If the value is negative, the opposite of the hw value is used. */
static struct axis_conversion bma020_axis_normal = {1, 2, 3};
static struct axis_conversion bma020_axis_y_inverted = {1, -2, 3};
static struct axis_conversion bma020_axis_x_inverted = {-1, 2, 3};
static struct axis_conversion bma020_axis_z_inverted = {1, 2, -3};
static struct axis_conversion bma020_axis_xy_rotated_left = {-2, 1, 3};
static struct axis_conversion bma020_axis_xy_swap_inverted = {-2, -1, 3};

#define AXIS_DMI_MATCH(_ident, _name, _axis) {		\
	.ident = _ident,				\
	.callback = bma020_dmi_matched,		\
	.matches = {					\
		DMI_MATCH(DMI_PRODUCT_NAME, _name)	\
	},						\
	.driver_data = &bma020_axis_##_axis		\
}
static struct dmi_system_id bma020_dmi_ids[] = {
	/* product names are truncated to match all kinds of a same model */
	AXIS_DMI_MATCH("NC64x0", "HP Compaq nc64", x_inverted),
	AXIS_DMI_MATCH("NC84x0", "HP Compaq nc84", z_inverted),
	AXIS_DMI_MATCH("NX9420", "HP Compaq nx9420", x_inverted),
	AXIS_DMI_MATCH("NW9440", "HP Compaq nw9440", x_inverted),
	AXIS_DMI_MATCH("NC2510", "HP Compaq 2510", y_inverted),
	AXIS_DMI_MATCH("NC8510", "HP Compaq 8510", xy_swap_inverted),
	AXIS_DMI_MATCH("HP2133", "HP 2133", xy_rotated_left),
	{ NULL, }
	/* Laptop models without axis info (yet):
	 * "NC651xx" "HP Compaq 651"
	 * "NC671xx" "HP Compaq 671"
	 * "NC6910" "HP Compaq 6910"
	 * HP Compaq 8710x Notebook PC / Mobile Workstation
	 * "NC2400" "HP Compaq nc2400"
	 * "NX74x0" "HP Compaq nx74"
	 * "NX6325" "HP Compaq nx6325"
	 * "NC4400" "HP Compaq nc4400"
	 */
};

static int bma020_add(struct device *device)
{
	if (!device)
		return -EINVAL;
	bma_dev.device = device;

	/* ##!!## axis direction should be checked */
	/* If possible use a "standard" axes order */
	if (dmi_check_system(bma020_dmi_ids) == 0) {
		printk(KERN_INFO "bma020" ": laptop model unknown, "
				"using default axes configuration\n");
		bma_dev.ac = bma020_axis_normal;
	}
	return bma020_init_device(&bma_dev);
}


/*********************** Sysfs stuff ***********************/
static ssize_t bma020_acceleration_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int x, y, z;
	if (bma_dev.is_on != 0) {
		bma020_get_xyz_axis_adjusted(&x, &y, &z);
	} else {
		x = 0; y = 0; z = 0;
	}

	if (x < ACC_MAX_VAL && x > -ACC_MAX_VAL &&
		y < ACC_MAX_VAL && y > -ACC_MAX_VAL &&
		z < ACC_MAX_VAL && z > -ACC_MAX_VAL) {
		return sprintf(buf, "(%d,%d,%d)\n", x, y, z);
	} else
		return 0;
}

static ssize_t bma020_calibrate_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "(%d,%d,%d)\n", bma_dev.xcalib, bma_dev.ycalib, bma_dev.zcalib);
}

static ssize_t bma020_calibrate_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
#ifdef DEBUG_INFO
	printk(KERN_INFO"func=%s\n", __FUNCTION__);
	printk(KERN_INFO"msg=%s\n", buf);
#endif
	if (bma_dev.is_on != 0)
		bma020_calibrate_joystick();

	return count;
}

static ssize_t bma020_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (bma_dev.is_on == 0)
		return sprintf(buf, "off\n");
	else
		return sprintf(buf, "on,user %d\n", bma_dev.usage);
}

static ssize_t bma020_status_set(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	if (strcmp(buf, "on\n") == 0)
		bma020_increase_use();
	else if (strcmp(buf, "off\n") == 0)
		bma020_decrease_use();
	else
		printk(KERN_INFO"unrecognized, using on/off\n");
	return count;
}


static ssize_t bma020_interval_set(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned int val = 0;
	char msg[256] = "\0";
	if (count > 256)
		count = 256;
	memcpy(msg, buf, count-1);/* skip last "\n"  */
	msg[count-1] = '\0';
	val = (unsigned int) simple_strtoul(msg, NULL, 10);
	if (val < 5)	/* TODO:avoid i2c too busy */
		val = 5;
	bma_dev.sample_interval = val;/* double increase sample rate */
	printk(KERN_INFO "bma020 sample interval %d ms\n", bma_dev.sample_interval);
	return count;
}

static ssize_t bma020_interval_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", bma_dev.sample_interval);
}

static ssize_t bma020_range_set(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned int val = 0;
	char msg[10] = "\0";
	if (count > 10)
		count = 10;
	memcpy(msg, buf, count-1);/* skip last "\n" */
	msg[count-1] = '\0';
	val = (unsigned int) simple_strtoul(msg, NULL, 10);
	if (val >= 2)
		bma_dev.range = BMA020_RANGE_8G;
	else
		bma_dev.range = val;/* 0 for BMA020_RANGE_2G, 1 for BMA020_RANGE_4G */
	if (bma_dev.is_on)/* activate immediately? */
		bma020_set_range(bma_dev.range);

	printk(KERN_INFO"bma020 range %d\n", bma_dev.range);
	return count;
}

static ssize_t bma020_range_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", bma_dev.range);
}

static ssize_t bma020_bandwidth_set(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned int val = 0;
	char msg[10] = "\0";
	if (count > 10)
		count = 10;
	memcpy(msg, buf, count-1);/* skip last "\n" */
	msg[count-1] = '\0';
	val = (unsigned int) simple_strtoul(msg, NULL, 10);
	printk("val=%d\n", val);
	if (val >= 6)
		bma_dev.bandwidth = BMA020_BW_1500HZ;
	else
		bma_dev.bandwidth = val;
	if (bma_dev.is_on)/* activate immediately? */
		bma020_set_bandwidth(bma_dev.bandwidth);

	printk(KERN_INFO"bma020 bandwidth %d\n", bma_dev.bandwidth);
	return count;
}

static ssize_t bma020_bandwidth_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", bma_dev.bandwidth);
}


static DEVICE_ATTR(status, S_IRUGO|S_IWUGO, bma020_status_show, bma020_status_set);
static DEVICE_ATTR(acceleration, S_IRUGO, bma020_acceleration_show, NULL);
static DEVICE_ATTR(calibrate, S_IRUGO|S_IWUGO, bma020_calibrate_show,
		bma020_calibrate_store);
static DEVICE_ATTR(interval, S_IRUGO|S_IWUGO, bma020_interval_show,
		bma020_interval_set);
static DEVICE_ATTR(range, S_IRUGO|S_IWUGO, bma020_range_show,
		bma020_range_set);
static DEVICE_ATTR(bandwidth, S_IRUGO|S_IWUGO, bma020_bandwidth_show,
		bma020_bandwidth_set);
/* static DEVICE_ATTR(rate, S_IRUGO, bma020_rate_show, NULL); */

static struct attribute *bma020_attributes[] = {
	&dev_attr_status.attr,
	&dev_attr_acceleration.attr,
	&dev_attr_calibrate.attr,
	&dev_attr_interval.attr,
	&dev_attr_range.attr,
	&dev_attr_bandwidth.attr,
	NULL
};

static struct attribute_group bma020_attribute_group = {
	.attrs = bma020_attributes
};

static int bma020_add_fs(struct device *device)
{
	return sysfs_create_group(&bma_dev.device->kobj, &bma020_attribute_group);
}

static int bma020_remove_fs(void)
{
	sysfs_remove_group(&bma_dev.device->kobj, &bma020_attribute_group);
	return 0;
}


#ifdef	CONFIG_PM
static int bma020_suspend(struct i2c_client *client, pm_message_t state)
{
	bma020_poweroff();
	return 0;
}

static int bma020_resume(struct i2c_client *client)
{
	bma020_poweron();
	return 0;
}
#else
#define	bma020_suspend		NULL
#define	bma020_resume		NULL
#endif

static int __devinit bma020_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
#ifdef DEBUG_INFO
	printk(KERN_INFO"bma020_i2c_probe \n");
#endif
	int retry;

	/* initialize global ptr */
	g_client = client;

	bma_dev.usage = 0;
	bma_dev.is_on = 0;
	bma_dev.range = BMA020_RANGE_2G;
	bma_dev.bandwidth = BMA020_BW_25HZ;
	bma_dev.sample_interval = 1000;/* ms */
	mutex_init(&bma_dev.lock);/* be called before bma020_increase_use() */

	/* make sure it's a bma020 sensor */
	for (retry = 0; retry < 30; retry++) {
		bma020_poweron();
		bma020_detect();
		bma020_poweroff();
		if (bma_dev.chip_id == 0x02)
			break;
	}
	if (bma_dev.chip_id != 0x02) {
		printk(KERN_ERR "bma020 chip id error\n");
		return -1;
	}

	printk(KERN_INFO "bma020:attatch to i2c bus, addr=0x%x, chip id=0x%x \n",
			bma_dev.dev_addr, bma_dev.chip_id);

#ifdef DEBUG_INFO
	printk(KERN_INFO "chip id: 0x%x\n", bma_dev.chip_id);
	printk(KERN_INFO "ml version: 0x%x\n", bma_dev.ml_version);
	printk(KERN_INFO "al version: 0x%x\n", bma_dev.al_version);
#endif

	/* bma gpio interrupt initialization */
	//bma_dev.gpio = mfp_to_gpio(MFP_PIN_GPIO111);
	/* Remove bma_dev.gpio = "CONSTANT" because drvier should be independent from platform..
	if (machine_is_dove_avng_v3())
		bma_dev.gpio = 0x35;
	else
		bma_dev.gpio = 0x17;
	*/
	//g_client->irq = gpio_to_irq(bma_dev.gpio);
	bma_dev.gpio = irq_to_gpio(g_client->irq);
	//gpio_direction_input(bma_dev.gpio);	/* set gpio pin as input */

#ifdef DEBUG_INFO
	printk(KERN_INFO "irq=%d\n", g_client->irq);
	printk(KERN_INFO "gpio=%d\n", bma_dev.gpio);
#endif

#ifdef DEBUG_INFO
	bma020_poweron();
	bma020_driver_test();/* show some measured data for simple test */
	bma020_poweroff();
#endif

	bma020_add(&client->dev);
	return 0;
}

static int bma020_remove(struct i2c_client *client)
{
	bma020_joystick_disable();
	bma020_remove_fs();
	bma020_poweroff();
	bma_dev.usage = 0;
	bma_dev.is_on = 0;

	return 0;
}

static const struct i2c_device_id bma020_id[] = {
	{ "bma020", 0 },
	{ }
};

static struct i2c_driver bma020_driver = {
	.driver = {
		.name	= "bma020",
	},
	.id_table 	= bma020_id,
	.probe		= bma020_probe,
	.remove		= bma020_remove,
	.suspend	= bma020_suspend,
	.resume		= bma020_resume,
};

static int __init bma020_i2c_init(void)
{
#ifdef DEBUG_INFO
	printk(KERN_INFO"func=%s \n", __FUNCTION__);
#endif
	return i2c_add_driver(&bma020_driver);
}

static void __exit bma020_i2c_exit(void)
{
	i2c_del_driver(&bma020_driver);
}

/*====================================================================*/

MODULE_DESCRIPTION("BOSCH BMA020 three-axis digital accelerometer (I2C) driver");
MODULE_LICENSE("GPL");

module_init(bma020_i2c_init);
module_exit(bma020_i2c_exit);

/*====================================================================*/
