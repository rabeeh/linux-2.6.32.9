#include <linux/module.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/rtc.h>
#include "../char/power_mcu.h"


static unsigned int cache_time = 1000;
module_param(cache_time, uint, 0644);


int avengers_rtc_read_time(struct rtc_time *tm );
int avengers_rtc_set_time(struct rtc_time *tm ) ;

MODULE_PARM_DESC(cache_time, "cache time in milliseconds");



static int avengers_lite_init_status( struct avengers_lite_device_info *di )
{
	di->ec_status = POWER_SUPPLY_STATUS_UNKNOWN;
	di->STATUS = EC_NO_CONNECTION ;
	di->VERSION = -1;
	di->RARC = 0;
	di->BATT_FAULT = 1;
	di->AC_IN = 0;
	di->BATT_PROTECT = -1;
	di->CURRENT = -1;
	di->VOLT1 = 0;
	di->VOLT2 = 0;
	di->TEMP = 0;

	return 0;

}



static int avengers_lite_read_status(struct avengers_lite_device_info *di)
{
	int ret;
	unsigned char temp_byte;
	short temp_word;


	if (di->update_time && time_before(jiffies, di->update_time +  msecs_to_jiffies(cache_time)))
		return 0;

	di->update_time = jiffies;

	ret = 0;

	if ( power_mcu_read( MCU_VERSION , &di->VERSION ) == -EFAULT )
	{
		di->STATUS = EC_NO_CONNECTION;
	}
	else
	{
		di->STATUS = 0;
	}

       if ( di->STATUS != EC_NO_CONNECTION )
       {

	       //power_mcu_read( MCU_VERSION , &di->VERSION );
	       //printk("********************************%d\n",di->VERSION);

	       power_mcu_read( MCU_DS_RARC , &temp_byte );
	       if (( temp_byte < di->RARC )||(temp_byte > di->RARC ))
	       {
		       ret = 1;
		       di->RARC = temp_byte;
	       }
	       //power_mcu_read( MCU_DS_RSRC , &di->RSRC );
	       //power_mcu_read( MCU_DS_AS , &di->AS );
	       //power_mcu_read( MCU_DS_SFR , &di->SFR );
	       //power_mcu_read( MCU_DS_CONTROL , &di->CONTROL );
	       //power_mcu_read( MCU_DS_AB , &di->AB );
	       //power_mcu_read( MCU_DS_VCHG , &di->VCHG );
	       //power_mcu_read( MCU_DS_IMIN , &di->IMIN );
	       //power_mcu_read( MCU_DS_VAE , &di->VAE );
	       //power_mcu_read( MCU_DS_IAE , &di->IAE );
	       //power_mcu_read( MCU_DS_RSNSP , &di->RSNSP );
	       power_mcu_read( MCU_BATT_ABSENT , &temp_byte );
	       if ( temp_byte != di->BATT_ABSENT )
	       {
		       ret = 1;
		       di->BATT_ABSENT = temp_byte;
	       }

	       power_mcu_read( MCU_AC_IN , &temp_byte );
	       if ( temp_byte != di->AC_IN )
	       {
		       ret = 1;
		       di->AC_IN = temp_byte;
	       }

	       power_mcu_read( MCU_BATT_PROTECT , &temp_byte );
	       if( temp_byte != di->BATT_PROTECT )
	       {
		       ret = 1;
		       di->BATT_PROTECT = temp_byte;
	       }
	      
	       //power_mcu_read( MCU_BATT_CYCLE , &di->BATT_CYCLE );
	       //power_mcu_read( MCU_DS_RAAC , &di->RAAC );
	       //power_mcu_read( MCU_DS_RSAC , &di->RSAC );
	       //power_mcu_read( MCU_DS_IAVG , &di->IAVG );
	       power_mcu_read( MCU_DS_VOLT1 , &temp_word );
	       temp_word = temp_word >> 5;
//	       if (( temp_word > di->VOLT1 + 1 )||( temp_word < di->VOLT1 - 1 ))
	       {
		       ret = 1;
	       	       di->VOLT1 = temp_word;

		}

	       power_mcu_read( MCU_DS_CURRENT , &temp_word );

	       if ((( temp_word < 0 ) && ( di->CURRENT > 0)) || ((temp_word >0 )&&(di->CURRENT <0)))
	       {
		       ret = 1;
		       di->CURRENT = temp_word;
	       }
	       if (( temp_word > di->CURRENT + 5 )||( temp_word < di->CURRENT - 5 ))
	       {
		       ret = 1;
		       di->CURRENT = temp_word;
		}

 		printk("MCU_BATT_ABSENT=0x%x,MCU_AC_IN=0x%x,MCU_BATT_PROTECT=0x%x,MCU_DS_VOLT1=0x%x,MCU_DS_CURRENT=0x%x,RARC=0x%x\n",di->BATT_ABSENT,di->AC_IN,di->BATT_PROTECT,di->VOLT1,di->CURRENT,di->RARC);

	       //power_mcu_read( MCU_DS_ACR , &di->ACR );
	       //power_mcu_read( MCU_DS_ACRL , &di->ACRL ):
	       //power_mcu_read( MCU_DS_FULL , &di->FULL );
	       //power_mcu_read( MCU_DS_AE , &di->AE );
	       //power_mcu_read( MCU_DS_SE , &di->SE );
	       //power_mcu_read( MCU_DS_AC , &di->AC );
	       power_mcu_read( MCU_DS_TEMP , &temp_word );
	       temp_word = temp_word >> 5;
	       if (( temp_word > di->TEMP + 5 )||( temp_word < di->TEMP - 5 ))
	       {
		       ret = 1;
		       di->TEMP = temp_word;
		}

	       //power_mcu_read( MCU_DS_FULL40 , &di->FULL40 );
#if 0
	       power_mcu_read( MCU_DS_VOLT2 , &temp_word );
	       temp_word = temp_word >> 5;
	       if (( temp_word > di->VOLT2 + 1 )||( temp_word < di->VOLT2 - 1 ))
	       {
		       ret = 1;
		       di->VOLT2 = temp_word;
		}
#endif	       

//	       power_mcu_read( MCU_BATT_FULL , &di->BATT_FULL );

/*	       power_mcu_write_word( MCU_RTC_LLSB , 0xff );
	       power_mcu_write_word( MCU_RTC_LMSB , 0x44 );
	       power_mcu_write_word( MCU_RTC_HLSB , 0x11 );
	       power_mcu_write_word( MCU_RTC_HMSB , 0x0);
	       //power_mcu_write_byte( MCU_RTC_STOP , 0 );

	       power_mcu_read( MCU_RTC_LLSB , &di->RTC_LLSB );
	       power_mcu_read( MCU_RTC_LMSB , &di->RTC_LMSB );
	       power_mcu_read( MCU_RTC_HLSB , &di->RTC_HLSB );
	       power_mcu_read( MCU_RTC_HMSB , &di->RTC_HMSB );
	       printk("llsb0x%x,lmsb0x%x,hlsb0x%x,hmsb0x%x\n",di->RTC_LLSB,di->RTC_LMSB,di->RTC_HLSB,di->RTC_HMSB);
*/

       }

	return ret;
}


static void avengers_lite_update_status(struct avengers_lite_device_info *di)
{
	int old_charge_status = di->ec_status ;
	short current_temp;

	if ( avengers_lite_read_status(di) != 0 )
	{
		kobject_uevent( &di->bat.dev->kobj , KOBJ_CHANGE );
	}

	if (di->ec_status == POWER_SUPPLY_STATUS_UNKNOWN)
		di->full_counter = 0;

       current_temp = di->CURRENT ;

	if (power_supply_am_i_supplied(&di->bat))
       {
	       if (current_temp > 0 )
	       {
		       di->ec_status = POWER_SUPPLY_STATUS_CHARGING;
		       di->full_counter = 0;
	       }
	       else if (current_temp < 0 )
	       {
		       if (di->ec_status != POWER_SUPPLY_STATUS_NOT_CHARGING)
			       dev_notice(di->dev, "not enough power to charge\n");
		       di->ec_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		       di->full_counter = 0;
	       }
	       else if (current_temp < 10000 && di->ec_status != POWER_SUPPLY_STATUS_FULL)
               {

			/* Don't consider the battery to be full unless
			 * we've seen the current < 10 mA at least two
			 * consecutive times. */

			di->full_counter++;

			if (di->full_counter < 2)
                     	{
				di->ec_status = POWER_SUPPLY_STATUS_CHARGING;
			}
			else
			{
				di->ec_status = POWER_SUPPLY_STATUS_FULL;
			}
		}
	}
       else
       {
		di->ec_status = POWER_SUPPLY_STATUS_DISCHARGING;
		di->full_counter = 0;
	}

	if (di->ec_status != old_charge_status)
		power_supply_changed(&di->bat);


}


static void avengers_lite_work(struct work_struct *work)
{
	struct avengers_lite_device_info *di = container_of(work,
		struct avengers_lite_device_info, monitor_work.work);
	const int interval = 1;

	dev_dbg(di->dev, "%s\n", __func__);

	avengers_lite_update_status(di);
	queue_delayed_work(di->monitor_wqueue, &di->monitor_work, interval);
}

#define to_avengers_lite_device_info(x) container_of((x), struct avengers_lite_device_info, bat);

static void avengers_lite_external_power_changed(struct power_supply *psy)
{
	struct avengers_lite_device_info *di = to_avengers_lite_device_info(psy);

	dev_dbg(di->dev, "%s\n", __func__);

	cancel_delayed_work(&di->monitor_work);
	queue_delayed_work(di->monitor_wqueue, &di->monitor_work, 1);
}

static int avengers_lite_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	struct avengers_lite_device_info *di = to_avengers_lite_device_info(psy);

	avengers_lite_read_status(di);

	switch (psp)
       	{
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		//val->intval = di->VOLT1 + di->VOLT2 ;
		val->intval = di->VOLT1 *2;	
		break;

	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = di->CURRENT ;
		break;

	case POWER_SUPPLY_PROP_CHARGE_FULL:
		if (( di->CURRENT < 50 ) && ( di->CURRENT>0))
			val->intval = 1 ;
		else
			val->intval = 0;
		break;

	case POWER_SUPPLY_PROP_CHARGE_EMPTY:
		if ( di->RARC < 5 )
			val->intval = 1;
		else
			val->intval = 0;
		break;

	case POWER_SUPPLY_PROP_CHARGE_NOW:
		if ( di->CURRENT > 50 )
			val->intval = 1;
		else
			val->intval = 0;
		break;

	case POWER_SUPPLY_PROP_TEMP:
		val->intval = di->TEMP;
		break;
       case POWER_SUPPLY_PROP_ONLINE:
		val->intval = di->AC_IN ; // 1---ac online 0---ac offline
		break;
       case POWER_SUPPLY_PROP_PRESENT:
              val->intval = !di->BATT_ABSENT ; //battery is online/offline
              break;
       case POWER_SUPPLY_PROP_CAPACITY:
              val->intval = di->RARC;
              break;
       case POWER_SUPPLY_PROP_STATUS: /*1--charging 2--discharging 4--Full 0--Unkhown 3--Not charging*/
              if ( di->STATUS == EC_NO_CONNECTION )
                val->intval = 3;
              else
              {
                    if ( di->CURRENT < 0 )
                        val->intval = 2;
                    else
                        val->intval = 1;
                    if ( di->RARC > 95 )
                        val->intval = 4;
              }

              break;
       case POWER_SUPPLY_PROP_HEALTH://3
       /*3--Dead 1--Good 2--Overheat 5--Unspeified failure 0--Unknown 4--Over voltage*/
              if ( di->BATT_PROTECT & MASK_BATT_OV )
              {
                    val->intval = 4 ;
              }
              else
                    val->intval = 1;

              break;
       case POWER_SUPPLY_PROP_TECHNOLOGY:
              val->intval = 2;
              break;
	default:
		return -EINVAL;
	}

	return 0;
}

static enum power_supply_property avengers_lite_battery_props[] = {
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CHARGE_EMPTY,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_TECHNOLOGY,

};




static int avengers_lite_power_probe(struct platform_device *pdev)
{
	int retval = 0;
	struct avengers_lite_device_info *di;
	
	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di) {
		retval = -ENOMEM;
		goto di_alloc_failed;
	}

	platform_set_drvdata(pdev, di);

	di->dev		= &pdev->dev;
	di->bat.name	   = pdev->name;
	di->bat.type	   = POWER_SUPPLY_TYPE_BATTERY;
	di->bat.properties     = avengers_lite_battery_props;
	di->bat.num_properties = ARRAY_SIZE(avengers_lite_battery_props);
	di->bat.get_property   = avengers_lite_get_property;
	di->bat.external_power_changed =  avengers_lite_external_power_changed;

	di->ec_status = POWER_SUPPLY_STATUS_UNKNOWN;

	avengers_lite_init_status( di );

	retval = power_supply_register(&pdev->dev, &di->bat);
	if (retval) {
		dev_err(di->dev, "failed to register battery\n");
		goto batt_failed;
	}

	INIT_DELAYED_WORK(&di->monitor_work, avengers_lite_work);
	//di->monitor_wqueue = create_singlethread_workqueue(pdev->dev.bus_id);
	di->monitor_wqueue = create_singlethread_workqueue(dev_name(&pdev->dev));
	if (!di->monitor_wqueue) {
		retval = -ESRCH;
		goto workqueue_failed;
	}
	queue_delayed_work(di->monitor_wqueue, &di->monitor_work, 1);

	goto success;

workqueue_failed:
	power_supply_unregister(&di->bat);
batt_failed:
	kfree(di);
di_alloc_failed:
success:
	return retval;
}

static int avengers_lite_power_remove(struct platform_device *pdev)
{
	struct avengers_lite_device_info *di = platform_get_drvdata(pdev);

	cancel_rearming_delayed_workqueue(di->monitor_wqueue , &di->monitor_work);
	destroy_workqueue(di->monitor_wqueue);
	power_supply_unregister(&di->bat);

	return 0;
}

static int avengers_lite_power_suspend(struct platform_device *pdev,
				  pm_message_t state)
{
	struct avengers_lite_device_info *di = platform_get_drvdata(pdev);

	di->ec_status = POWER_SUPPLY_STATUS_UNKNOWN;

	return 0;
}

static int avengers_lite_power_resume(struct platform_device *pdev)
{
	struct avengers_lite_device_info *di = platform_get_drvdata(pdev);

	di->ec_status = POWER_SUPPLY_STATUS_UNKNOWN;
	power_supply_changed(&di->bat);

	cancel_delayed_work(&di->monitor_work);
	queue_delayed_work(di->monitor_wqueue, &di->monitor_work, HZ);

	return 0;
}

static struct platform_driver avengers_lite_power_driver = {
	.driver = {
		.name = "battery",
	},
	.probe	  = avengers_lite_power_probe,
	.remove   = avengers_lite_power_remove,
	.suspend  = avengers_lite_power_suspend,
	.resume	  = avengers_lite_power_resume,
};

static int __init avengers_lite_power_init(void)
{
	return platform_driver_register(&avengers_lite_power_driver);
}

static void __exit avengers_lite_power_exit(void)
{
	platform_driver_unregister(&avengers_lite_power_driver);
}

module_init(avengers_lite_power_init);
module_exit(avengers_lite_power_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Danny <dsong4@marvell.com");
MODULE_DESCRIPTION("avengers lite power driver");
