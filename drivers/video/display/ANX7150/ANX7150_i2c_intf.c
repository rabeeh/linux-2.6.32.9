
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>

#include "ANX7150_i2c_intf.h"
#include "ANX7150.h"
extern struct i2c_client *g_client;
//ANX7150 I2C address 
//{0x72/2,0x7A/2} or {0x76/2+0x7E/2}
BYTE ANX7150_i2c_write_p0_reg(BYTE offset, BYTE d)
{
    int rc;
    rc=i2c_smbus_write_byte_data(g_client, offset, d);
    msleep(10);
    return 1;

}

BYTE ANX7150_i2c_read_p0_reg(BYTE offset, BYTE *d)
{
    int rc;
    rc=i2c_smbus_read_byte_data(g_client, offset);
    *d=rc;
    msleep(10);	
    return 1;
     
}

BYTE ANX7150_i2c_write_p1_reg(BYTE offset, BYTE d)
{
    int rc;
    g_client-> addr=0x3F;
    rc=i2c_smbus_write_byte_data(g_client, offset, d);
    g_client-> addr=0x3B;
    msleep(10);
    return 1;

}

BYTE ANX7150_i2c_read_p1_reg(BYTE offset, BYTE *d)
{
    int rc;
    g_client-> addr=0x3F;
    rc=i2c_smbus_read_byte_data(g_client, offset);
    g_client-> addr=0x3B;
    *d=rc;
    msleep(10);
    return 1;
     
}
