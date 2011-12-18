/*
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/freezer.h>
#include <linux/workqueue.h>


//Extra include for ANX7150 here
#include "ANX7150_i2c_intf.h"
#include "ANX7150_Sys7150.h"
#include "ANX7150_System_Config.h"
#include "HDMI_RX_DRV.h"
#include "ANX7150.h"
#include <video/kg2.h>
int bist_reset=1;

struct kg2_device_info {
	struct device *dev;
};

#define REG_MIDL 0
#define REG_MIDH 1
#define REG_PIDH 2
#define REG_PIDL 3

//int ANX7150_shutdown=0;
struct i2c_client *g_client;

static const struct i2c_device_id anx7150_register_id[] = {
     { "anx7150_i2c", 0 },
     { }
};

MODULE_DEVICE_TABLE(i2c, anx7150_i2c_id);




static struct i2c_driver anx7150_i2c_driver;
static struct i2c_client *anx7150_i2c_client;

static struct i2c_client *get_anx7150_i2c_client(void)
{
	return anx7150_i2c_client;
}

static struct workqueue_struct * anx7150_workqueue;
static struct delayed_work anx7150_work;
static long last_jiffies;


static int anx7150_reg_get(int offset, unsigned char *pDataBuf, int datasize)
{
	int rc;
	struct i2c_client *i2c_client;

	if((i2c_client = get_anx7150_i2c_client()))
	{
		rc = i2c_smbus_read_i2c_block_data(i2c_client, offset, datasize, pDataBuf);
		if (rc < 0)
			return -EIO;
	}else{
		return -ENODEV;
	}
    	return 0;	
}

static int anx7150_reg_set(int offset, unsigned char data)
{
	struct i2c_client *i2c_client;

	if((i2c_client = get_anx7150_i2c_client()))
	{
		i2c_smbus_write_byte_data(i2c_client, offset, data);

	}else{
		return -ENODEV;
	}
    	return 0;	
}




int BIST3(void)
{
//HW_Initial
BYTE c;
int i;
    ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL3_REG, 0x02);
    ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL2_REG, 0x00);
    ANX7150_i2c_write_p0_reg(ANX7150_SRST_REG, 0x00);
    ANX7150_i2c_read_p0_reg(ANX7150_SYS_PD_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_SYS_PD_REG, (c | 0x06));
    ANX7150_i2c_read_p0_reg(ANX7150_CHIP_CTRL_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_CHIP_CTRL_REG, (c & 0xfe));
    ANX7150_i2c_read_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, (c | 0x80));
    ANX7150_i2c_write_p0_reg(ANX7150_PLL_CTRL0_REG, 0xa8);
    ANX7150_i2c_read_p0_reg(ANX7150_PLL_TX_AMP, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_PLL_TX_AMP, (c | 0x01));
    ANX7150_i2c_write_p0_reg(ANX7150_PLL_CTRL1_REG,0x00);
    ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL1_REG, 0x00);
    ANX7150_i2c_write_p0_reg(0x13, 0x11);//enable BIST

//HPD

    ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL3_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL3_REG, c | 0x01);//power up all, 090630
    ANX7150_i2c_read_p0_reg(ANX7150_SYS_STATE_REG, &c);
    if (c & ANX7150_SYS_STATE_HP == 0)
    { }//TODO: if the HDMI cable is not plugged }
    
    ANX7150_i2c_read_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, c & (~ANX7150_HDMI_AUDCTRL1_IN_EN));
        ANX7150_i2c_read_p0_reg(ANX7150_VID_CTRL_REG, &c);

        ANX7150_i2c_write_p0_reg(ANX7150_VID_CTRL_REG, c & (~ANX7150_VID_CTRL_IN_EN));
        ANX7150_i2c_read_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, &c);

        ANX7150_i2c_write_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, c & (~ANX7150_TMDS_CLKCH_MUTE));
        ANX7150_i2c_read_p0_reg(ANX7150_HDCP_CTRL0_REG, &c);

        ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL0_REG, (c & (~ANX7150_HDCP_CTRL0_HW_AUTHEN)));
        ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL1_REG, &c);
        ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL1_REG, c | 0x03); //HDMI
	//ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL1_REG, (c | 0x01)&0xFD); //DVI

//RX Sense

        ANX7150_i2c_read_p0_reg(ANX7150_SYS_STATE_REG, &c);
        if(c & ANX7150_SYS_STATE_RSV_DET == 0)
        {} //TODO: Receive sense is not active }

//Config Video

    ANX7150_i2c_write_p0_reg(ANX7150_VID_MODE_REG, 0x00);
    ANX7150_i2c_read_p0_reg(ANX7150_SYS_STATE_REG, &c);
    if(!(c & 0x02))
    {}//TODO: Input clock not detected}

    ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL4_REG, 0x00);
    ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL0_REG, 0x10);
    ANX7150_i2c_write_p0_reg(ANX7150_VID_CAPCTRL1_REG, 0x00);// patch
    ANX7150_i2c_write_p0_reg(ANX7150_VID_MODE_REG, 0x1C);
    ANX7150_i2c_write_p0_reg(ANX7150_VID_CTRL_REG, 0x3C);
    ANX7150_i2c_read_p0_reg(ANX7150_SRST_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_SRST_REG, c | ANX7150_SRST_SW_RST);
    ANX7150_i2c_write_p0_reg(ANX7150_SRST_REG, (c & (~ANX7150_SRST_SW_RST)));
    ANX7150_i2c_read_p0_reg(ANX7150_SRST_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_SRST_REG, (c | ANX7150_TX_RST));
    ANX7150_i2c_write_p0_reg(ANX7150_SRST_REG, (c & (~ANX7150_TX_RST)));
    ANX7150_i2c_read_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, &c);
    ANX7150_i2c_write_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, (c | ANX7150_TMDS_CLKCH_MUTE));
    
    
    //ANX7150_Config_Bist_Video(1);
//Now it is in PLAYBACK state¡K..

    //patch
    ANX7150_i2c_write_p0_reg(0x71, 0x0f); // It's necessary!?, 0x0f

    ANX7150_i2c_write_p0_reg(0x11, 0x00);//ALex: must be 0x00
    ANX7150_i2c_write_p0_reg(0x12, 0x3c);

// Config Audio
//Single tone
/*
            ANX7150_i2c_write_p1_reg(ANX7150_ACR_N1_SW_REG, 0x00);
            ANX7150_i2c_write_p1_reg(ANX7150_ACR_N2_SW_REG, 0x18);
            ANX7150_i2c_write_p1_reg(ANX7150_ACR_N3_SW_REG, 0x00);
            //Enable control of ACR
            ANX7150_i2c_write_p1_reg(ANX7150_INFO_PKTCTRL1_REG, 0x01);
            //audio enable: 
            ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, 0x81);
            ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL0_REG, 0x01);
            ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDBIST_CTRL_REG, 0xCF);
*/
//pass through
            ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL0_REG, 0x01);
            ANX7150_i2c_write_p0_reg(ANX7150_HDMI_AUDCTRL1_REG, 0xC1);
            ANX7150_i2c_write_p0_reg(ANX7150_SPDIFCH_STATUS_REG, 0x00);//0x55, 0xXY

}

int BIST(void)
{
BYTE c;
int j=100;

    int rc=-1;
    if(bist_reset)
    {
        i2c_smbus_write_byte_data(g_client, 0x1a, 0x01);
	  msleep(10);
	  i2c_smbus_write_byte_data(g_client, 0x1a, 0x00);
	  msleep(10);
        {
            i2c_smbus_write_byte_data(g_client, 0x18, 0x00);
            //Temporary remove it//HDMI_RX_Initialize ();
            i2c_smbus_write_byte_data(g_client, 0x16, 0x00);
        }
	//HW reset
        rc=i2c_smbus_write_byte_data(g_client, 0x08,1);
	msleep(60);
        ANX7150_API_Initial();
        bist_reset = 0;
    }

	if(!EXT_INT_EN)
	{
		int_s1=i2c_smbus_read_byte_data(g_client, ANX7150_INTR1_STATUS_REG);
		i2c_smbus_write_byte_data(g_client, ANX7150_INTR1_STATUS_REG, int_s1);

		int_s2=i2c_smbus_read_byte_data(g_client, ANX7150_INTR2_STATUS_REG);
		i2c_smbus_write_byte_data(g_client, ANX7150_INTR2_STATUS_REG, int_s2);

		int_s3=i2c_smbus_read_byte_data(g_client, ANX7150_INTR3_STATUS_REG);
		i2c_smbus_write_byte_data(g_client, ANX7150_INTR3_STATUS_REG, int_s3);

		ANX7150_INT_Done = 1;		
	}

    if(ANX7150_INT_Done)
	{
	    ANX7150_Interrupt_Process();
		ANX7150_INT_Done = 0;
	}
    ANX7150_i2c_read_p0_reg(ANX7150_INTR_STATE_REG, &c);
    ANX7150_BIST();
	msleep(500);
	return 1;
}


//int anx7150_thread (void) 
void anx7150_work_func(struct work_struct * work)
{
/*
   int j=100,k=100;

    while (1) 
    {
*/
        set_freezable();
	unsigned long interval = jiffies_to_msecs((unsigned long)((long) jiffies - last_jiffies));
//	printk("anx7150: anx7150_work_func() is called, intervel: %lu msec.\n", interval);
	last_jiffies = (long) jiffies;

	while(BIST_EN)
	{
		BIST();
		msleep(500);
	}

	if (1) 
	{
		if(!ANX7150_shutdown)
		{
			ANX7150_API_Input_Changed();
			ANX7150_Task();
			if(1)
			{
				if( ANX7150_parse_edid_done == 1 
				    &&   ANX7150_system_config_done == 0)
				{
					//system should config all the parameters here
					//ANX7150_API_PRINT_EDID_Parsing_Result();
					ANX7150_API_System_Config();
					ANX7150_system_config_done = 1;
				}
			}
			else
			{
				ANX7150_system_config_done = 0;
			}
		}
	}

	queue_delayed_work(anx7150_workqueue, &anx7150_work, msecs_to_jiffies(500));
/*
        msleep(500);

    }

return 0;
*/
}

static int anx7150_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int rc,i;

	memcpy(&g_client, &client, sizeof(client));	
	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_I2C_BLOCK)) {
		dev_err(&client->dev, "i2c bus does not support the anx7150\n");
		rc = -ENODEV;
		goto exit;
	}

	//HW reset
	rc=i2c_smbus_write_byte_data(client, 0x08,1);
	msleep(50);

	rc=i2c_smbus_read_byte_data(client, 0x02);
	printk("##ANX7150..REG[%x]=%x\n",0x02,rc);
	rc=i2c_smbus_read_byte_data(client, 0x03);
	printk("##ANX7150..REG[%x]=%x\n",0x03,rc);
	msleep(2000);

	ANX7150_API_DetectDevice();
	ANX7150_API_Initial();
	ANX7150_API_ShutDown(0);
	ANX7150_API_HDCP_ONorOFF(HDCP_EN);

	if (BIST3_EN)
    		BIST3();
	else
	{

		anx7150_workqueue = create_singlethread_workqueue("ANX7150_WORKQUEUE");

		if (anx7150_workqueue == NULL)
		{
			printk("anx7150: Failed to creat work queue.\n");
			goto exit;
		}

		last_jiffies = (long) jiffies;
		INIT_DELAYED_WORK(&anx7150_work, anx7150_work_func);
		queue_delayed_work(anx7150_workqueue, &anx7150_work, 0);
/*

		rc=kernel_thread(anx7150_thread, NULL, CLONE_VM);

		if (rc < 0)
			printk("!!!Fail to creat ANX7150 thread!!!\n");
		else
			printk("##ANX7150 thread ok!!!\n");
*/
	}

	return 0;
	
 exit:
	return rc;
}

static int anx7150_i2c_remove(struct i2c_client *client)
{
	destroy_workqueue(anx7150_workqueue);

	anx7150_i2c_client = NULL;
	return 0;
}

static int anx7150_suspend(struct i2c_client *client)
{
	ANX7150_Timer_Process_Re_init();
	return 0;
}
static int anx7150_resume(struct i2c_client *client)
{
	ANX7150_API_Initial();
	ANX7150_Timer_Process_Re_init();
	return 0;
}

static struct i2c_driver anx7150_driver = {
     .driver = {
	  .name   = "anx7150_i2c",
	  .owner  = THIS_MODULE,
     },
     .probe          = anx7150_i2c_probe,
     .remove         = anx7150_i2c_remove,
     .suspend        = anx7150_suspend,
     .resume         = anx7150_resume,
     .id_table       = anx7150_register_id,
};

static int __init dove_anx7150_init(void)
{
	int ret;
	if ((ret = i2c_add_driver(&anx7150_driver)) < 0)
	{
		return ret;		

	}
	return ret;
}

static void __exit dove_anx7150_exit(void)
{
	i2c_del_driver(&anx7150_driver);
}

module_init(dove_anx7150_init);
module_exit(dove_anx7150_exit);

MODULE_AUTHOR("Marvell");
MODULE_DESCRIPTION("ANX7150 driver");
MODULE_LICENSE("GPL");
