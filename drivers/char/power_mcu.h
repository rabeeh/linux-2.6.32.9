#ifndef __POWER_MCU_H__
#define __POWER_MCU_H__

#include <linux/power_supply.h>
#include <linux/rtc.h>

#define EC_NO_CONNECTION (-123)


//_W  word
//_B  byte
/*
enum {
	MCU_VERSION_B = 0 ,        //version of ec
	MCU_STATUS_B,          //status
	MCU_RARC_B,           //remianing active relavtie capacity
	MCU_RSRC_B,           //remaining standby relative capacity
	MCU_RAAC_W ,        //remaining active absolute capacity
	MCU_RSAC_W ,        //remaining standby absolute capacity
	MCU_IAVG_W ,        //average current register
	MCU_TEMP_W,        //temperature
	MCU_VOLT_W,        //voltage
	MCU_CURRENT_W,  //current
	MCU_ACR_W ,         //accumulated current
	MCU_ACRL_W ,       //low accumulated current
	MCU_AS_B,            //age scalar
	MCU_FULL_W,         //full capacity
	MCU_AE_W,            //active empty
	MCU_SE_W,           //standby empty
	MCU_TIME_LOW_W ,  //low 16bit of rtc
	MCU_TIME_MID1_W ,  //low mid 16bit of rtc
	MCU_TIME_MID2_W ,  //high mid 16bit of rtc
	MCU_TIME_HIGH_W , //high 16bit of rtc
	MCU_END,
};
*/
/*
enum I2C_MCU_DATA {
                   MCU_VERSION = 0x00,                               // MCU VERSION
                   MCU_STATUS = 0x01,                           // DS STATUS
                   MCU_RAAC_MSB = 0x02,                     
                   MCU_RAAC_LSB = 0x03,             
                   MCU_RSAC_MSB = 0x04,                           
                   MCU_RSAC_LSB = 0x05,      
                   MCU_RARC = 0x06,    
                   MCU_RSRC = 0x07,
                   MCU_IAVG_MSB = 0x08,  
                   MCU_IAVG_LSB = 0x09,
                   MCU_TEMP_MSB = 0x0a,
                   MCU_TEMP_LSB = 0x0b,       
                   MCU_VOLT_MSB = 0x0c,
                   MCU_VOLT_LSB = 0x0d,
                   MCU_CURRENT_MSB = 0x0e,
                   MCU_CURRENT_LSB = 0x0f,             // Shut down command
                   MCU_ACR_MSB = 0x10,            // Sleep command
                   MCU_ACR_LSB = 0x11,
                   MCU_ACRL_MSB = 0x12,
                   MCU_ACRL_LSB = 0x13,
                   MCU_AS = 0x14,
                   MCU_SFR = 0x15,
                   MCU_FULL_MSB = 0x16,
                   MCU_FULL_LSB = 0x17,
                   MCU_AE_MSB = 0x18,
                   MCU_AE_LSB = 0x19,
                   MCU_SE_MSB = 0x1a,
                   MCU_SE_LSB = 0x1b,
                   
                   MCU_END = 0x61,
};
*/
enum I2C_MCU_DATA 
{
	MCU_OFFSET_BYTE = 0x00,
	MCU_VERSION = 0x00,                               // MCU VERSION
	MCU_DS_STATUS = 0x01,                           // DS STATUS
	MCU_DS_RARC = 0x02,                     
	MCU_DS_RSRC = 0x03,
	MCU_DS_AS = 0x04,
	MCU_DS_SFR = 0x05,
	MCU_DS_CONTROL = 0x06,
	MCU_DS_AB = 0x07,
	MCU_DS_VCHG = 0x08,
	MCU_DS_IMIN = 0x09,
	MCU_DS_VAE = 0x0a,
	MCU_DS_IAE = 0x0b,
	MCU_DS_RSNSP = 0x0c,
	MCU_BATT_ABSENT = 0x0d, //1--battery is not avaiable 0--battery is avaiable
	MCU_AC_IN = 0x0e,
	MCU_BATT_PROTECT= 0x0f,
	MCU_BATT_CYCLE = 0x10,
	MCU_OFFSET_WORD = 0x12,
	MCU_DS_RAAC = 0x12,
	MCU_DS_IAVG = 0x16,
	MCU_DS_VOLT1 = 0x18,
	MCU_DS_CURRENT = 0x1a,
	MCU_DS_ACR = 0x1c,
	MCU_DS_ACRL = 0x1e,
	MCU_DS_FULL = 0x20,
	MCU_DS_AE = 0x22,
	MCU_DS_SE = 0x24,
	MCU_DS_AC = 0x26,
	MCU_DS_TEMP = 0x28,
	MCU_DS_FULL40 = 0x2c,
	//RTC
	MCU_RTC_LSB = 0x2e,
	MCU_RTC_MSB = 0x30,
	MCU_BATT_FULL = 0x32,
	MCU_BATT_FAULT = 0x33,
	//ALARM
	MCU_ALARM_LSB = 0x36,
	MCU_ALARM_MSB = 0x38,
	
	MCU_DS_VOLT2= 0x3e,
	MCU_ALM_UPDATE=0x41,              // Update ALARM: Write MCU_ALARM_LLSB = 0x36,MCU_ALARM_LMSB  = 0x38,         MCU_ALARM_HLSB = 0x3a,   MCU_ALARM_HMSB= 0x3c, then wirte MCU_ALM_UPDATE=0x41,
	MCU_END,

};

struct avengers_lite_device_info 
{
	struct device *dev;
	unsigned long update_time;	/* jiffies when data read */
	int ec_status;
	int full_counter;

	unsigned char VERSION;
        int STATUS;
	unsigned char RARC;
	unsigned char RSRC;
	unsigned char AS;
	unsigned char SFR;
	unsigned char CONTROL;
	unsigned char AB;
	unsigned char VCHG;
	unsigned char IMIN;
	unsigned char VAE;
	unsigned char IAE;
	unsigned char RSNSP;
	unsigned char BATT_ABSENT;
	unsigned char AC_IN;
	unsigned char BATT_PROTECT;
	unsigned char BATT_CYCLE;
	unsigned short RAAC;
	unsigned short RSAC;
	unsigned short IAVG;
	unsigned short VOLT1;
	short CURRENT;
	unsigned short ACR;
	unsigned short ACRL;
	unsigned short FULL;
	unsigned short AE;
	unsigned short SE;
	unsigned short AC;
	unsigned short TEMP;
	unsigned short FULL40;
	unsigned short RTC_LLSB;
	unsigned short RTC_LMSB;
	unsigned short RTC_HLSB;
	unsigned short RTC_HMSB;
	unsigned short ALARM_LLSB;
	unsigned short ALARM_LMSB;
	unsigned short ALARM_HLSB;
	unsigned short ALARM_HMSB;
	unsigned short VOLT2;
	unsigned short BATT_FULL;
	unsigned short BATT_FAULT;
	struct rtc_time time;
	unsigned long raw_time_low;
	unsigned long raw_time_high;
       
	struct power_supply bat;
	struct workqueue_struct *monitor_wqueue;
	struct delayed_work monitor_work;
};

#define MASK_CHGTF 0x80
#define MASK_AEF 0x40
#define MASK_SEF 0x20

#define MASK_BATT_OV 0x80


extern int power_mcu_read(unsigned char reg,void *pval);
//extern int power_mcu_read_word(unsigned char reg, unsigned short *pval);
extern int power_mcu_write_byte(unsigned char reg, unsigned char val);
extern int power_mcu_write_word(unsigned char reg, unsigned short val);
void power_mcu_write_rtc( u32 rtc );
void power_mcu_read_rtc( u32 *rtc );
#endif
