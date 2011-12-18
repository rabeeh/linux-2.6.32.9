/*
    This driver supports Chrontel CH7025/CH7026 TV/VGA Encoder.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/proc_fs.h>
#include <video/ch7025_composite.h>

#define CH7025_DEVICE_ID_CMD		0x00
#define CH7025_DEVICE_REVISION_ID_CMD	0x01
#define CH7025_OUTPUT_FORMAT		0x0D
#define CH7025_DID 0x55
#define CH7026_DID 0x54

static const struct i2c_device_id ch7025_id[] = {
	{ "ch7025_i2c", 0 },
	{ }
};

struct i2c_client *ch7025_client = NULL;
static struct input_stream_info gInputInfo;
static int gEnable;
unsigned char REG_MAP_320_240[ ][2] = 
{
	{ 0x07, 0x18 }, { 0x12, 0x40 }, { 0x4D, 0x03 }, { 0x4E, 0xC5 },
	{ 0x4F, 0x7F }, { 0x50, 0x7B }, { 0x51, 0x59 }, { 0x52, 0x12 },
	{ 0x53, 0x1B }, { 0x55, 0xE5 }, { 0x5E, 0x80 },
	{ 0x09, 0x80 }, { 0x16, 0xF6 }, { 0x1C, 0x64 }, { 0x21, 0x51 },
	{ 0x32, 0x77 }, { 0x33, 0x07 }, { 0x34, 0xF5 }, { 0x36, 0x32 },
	{ 0x3D, 0x98 }, 
};
#define REG_MAP_320_240_LENGTH ( sizeof(REG_MAP_320_240) / (2*sizeof(unsigned char)) )
unsigned char REG_MAP_640_480[ ][2] = 
{
	{ 0x07, 0x18 }, { 0x0D, 0x80 }, { 0x0F, 0x1A }, { 0x10, 0x80 },
	{ 0x11, 0x20 }, { 0x12, 0x40 }, { 0x13, 0x10 }, { 0x14, 0x60 },
	{ 0x15, 0x11 }, { 0x16, 0xE0 }, { 0x17, 0x0D }, { 0x19, 0x0A },
	{ 0x1A, 0x02 }, { 0x4D, 0x03 }, { 0x4E, 0xC5 }, { 0x4F, 0x7F },
	{ 0x50, 0x7B }, { 0x51, 0x59 }, { 0x52, 0x12 }, { 0x53, 0x1B },
	{ 0x55, 0xE5 }, { 0x5E, 0x80 },
	{ 0x09, 0x80 }, { 0x16, 0xEA }, { 0x1C, 0x64 }, { 0x21, 0x51 },
	{ 0x32, 0x77 }, { 0x36, 0x2E }, { 0x3D, 0x98 }, { 0x3E, 0x20 },
};
#define REG_MAP_640_480_LENGTH ( sizeof(REG_MAP_640_480) / (2*sizeof(unsigned char)) )
unsigned char REG_MAP_800_600[ ][2] = //800x600
{
	{ 0x06, 0x6B }, { 0x07, 0x18 }, { 0x0D, 0x80 }, { 0x0F, 0x1B },
	{ 0x10, 0x20 }, { 0x11, 0xC0 }, { 0x12, 0x40 }, { 0x13, 0x32 },
	{ 0x14, 0x3C }, { 0x15, 0x12 }, { 0x16, 0x58 }, { 0x17, 0x6C },
	{ 0x19, 0x08 }, { 0x1A, 0x04 }, { 0x4D, 0x03 }, { 0x4E, 0xC5 },
	{ 0x4F, 0x7F }, { 0x50, 0x7B }, { 0x51, 0x59 }, { 0x52, 0x12 }, 
	{ 0x53, 0x1B }, { 0x55, 0xE5 }, { 0x5E, 0x80 }, { 0x69, 0x64 },
	{ 0x09, 0x80 }, { 0x13, 0x48 }, { 0x17, 0x76 }, { 0x19, 0x09 },
	{ 0x1C, 0x64 }, { 0x21, 0x51 }, { 0x22, 0xD8 }, { 0x32, 0x77 },
	{ 0x33, 0x07 }, { 0x34, 0xFB }, { 0x36, 0x32 }, { 0x3D, 0x98 },
	{ 0x3E, 0x20 },
};
#define REG_MAP_800_600_LENGTH ( sizeof(REG_MAP_800_600) / (2*sizeof(unsigned char)) )
unsigned char REG_MAP_1024_600[ ][2] = 
{
	{ 0x06, 0x6B }, { 0x07, 0x18 }, { 0x0D, 0x80 }, { 0x0E, 0xE4 },
	{ 0x0F, 0x24 }, { 0x10, 0x00 }, { 0x11, 0xB0 }, { 0x12, 0x40 },
	{ 0x13, 0x26 }, { 0x14, 0x64 }, { 0x15, 0x12 }, { 0x16, 0x5D },
	{ 0x17, 0x6C }, { 0x19, 0x08 }, { 0x1A, 0x04 }, { 0x4D, 0x03 },
	{ 0x4E, 0xC5 }, { 0x4F, 0x7F }, { 0x50, 0x7B }, { 0x51, 0x59 },
	{ 0x52, 0x12 }, { 0x53, 0x1B }, { 0x55, 0xE5 }, { 0x5E, 0x80 },
	{ 0x69, 0x60 },
	{ 0x09, 0x80 }, { 0x1C, 0x63 }, { 0x21, 0x51 }, { 0x22, 0xD8 },
	{ 0x32, 0x77 }, { 0x33, 0x07 }, { 0x34, 0xFB }, { 0x35, 0x08 }, 
	{ 0x36, 0x35 }, { 0x3D, 0x98 }, { 0x3E, 0x20 },

};
#define REG_MAP_1024_600_LENGTH ( sizeof(REG_MAP_1024_600) / (2*sizeof(unsigned char)) )

void ch7025_write_data(u8 cmd, u8 val)
{
	i2c_smbus_write_byte_data(ch7025_client, (u8)cmd, (u8)val);
}
int ch7025_read_data(u8 cmd)
{
	return i2c_smbus_read_byte_data(ch7025_client, cmd);
}

void ch7025_enable(bool enable)
{
	int i;
	int regmap_length, val;
	unsigned char *reg_map;

	if(ch7025_client==NULL){
		printk("can't find ch7025 chip\n");
		return;
	}
	val = ch7025_read_data(0x00);
	if ( (val != CH7025_DID ) && (val != CH7026_DID))
	{
		printk("ch7025 vendor ID error!");
		return;
	}
	/* init sequence. */
	ch7025_write_data(0x02, 0x01);
	ch7025_write_data(0x02, 0x03);
	ch7025_write_data(0x03, 0x00);
	ch7025_write_data(0x04, 0x39);

	if(!enable) { 
		gEnable = 0;
		return;
	}
	gEnable = 1;
	/* the settings programmer wanna set. */
	if( (gInputInfo.xres==1024) && (gInputInfo.yres==600) ){
		reg_map = REG_MAP_1024_600;
		regmap_length = REG_MAP_1024_600_LENGTH;
	}else if( (gInputInfo.xres==800) && (gInputInfo.yres==600) ){
		reg_map = REG_MAP_800_600;
		regmap_length = REG_MAP_800_600_LENGTH;	
	}else if( (gInputInfo.xres==640) && (gInputInfo.yres=480) ){
		reg_map = REG_MAP_640_480;
		regmap_length = REG_MAP_640_480_LENGTH;	
	}else{
		reg_map = REG_MAP_320_240;
		regmap_length = REG_MAP_320_240_LENGTH;	
	}

	for (i = 0 ; i < regmap_length ; i++ )
		ch7025_write_data(reg_map[2*i], reg_map[2*i+1]);

	if(gInputInfo.iformat > CH7025_RGB888){//input format setting
		i = ch7025_read_data(0x0C);
		i = (i&0xf0) | (gInputInfo.iformat&0xf);
		ch7025_write_data(0x0C, i);
		if( (gInputInfo.iformat>=CH7025_YCbCr422_8bit) 
		     && (gInputInfo.iformat<=CH7025_YCbCr444_8bit)){
			i = (ch7025_read_data(0x0D) | 0x20);
			ch7025_write_data(0x0D, i);	 
		}
	}
	if(gInputInfo.swap > CH7025_RGB_ORDER){//output format setting
		i = ch7025_read_data(0x0C);
		i = (i&0x8f) | ((gInputInfo.swap&0x7)<<4);
		ch7025_write_data(0x0C, i);
	}
	if(gInputInfo.oformat > CH7025_NTSC_M){//output format setting
		i = ch7025_read_data(0x0D);
		i = (i&0xf0) | (gInputInfo.oformat&0xf);
		ch7025_write_data(0x0D, i);
	}
	
	/* finalize sequence. */
	ch7025_write_data(0x7D, 0x62);
	ch7025_write_data(0x04, 0x38);
	if(gInputInfo.xres > 720)
		ch7025_write_data(0x06, 0x69);
	else
		ch7025_write_data(0x06, 0x71);	
	ch7025_write_data(0x03, 0x00);
	ch7025_write_data(0x03, 0x00);
	ch7025_write_data(0x03, 0x00);
	ch7025_write_data(0x03, 0x00);
	ch7025_write_data(0x03, 0x00);
	ch7025_write_data(0x06, 0x70);
	ch7025_write_data(0x02, 0x02);
	ch7025_write_data(0x02, 0x03);
	ch7025_write_data(0x04, 0x00);

}
void ch7025_set_input_stream(struct input_stream_info *info)
{
	gInputInfo.xres = info->xres;
	gInputInfo.yres = info->yres;
	gInputInfo.iformat = info->iformat;
	gInputInfo.oformat = info->oformat;
	gInputInfo.swap = info->swap;
}

int Limit(int val, int min, int max)
{
	if(val < min) return min;
	else if(val > max) return max;
	else return val;
}
static ssize_t show_AllParam(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	int len = 0;
	int i;
	s32 rd;

	if ((rd = (int)ch7025_read_data(CH7025_DEVICE_ID_CMD)) < 0) {
          	return -EIO;
     	}
	len += sprintf(buf+len, "DEVICE ID (0x00) = 0x%02x\n", rd);
	
	len += sprintf(buf+len, "\nDump CH7025 registers(HEX):\n");
	for(i=0; i<0x80; i++){
		if ((rd = (int)ch7025_read_data((u8)i)) < 0) {
			return -EIO;
		}
		len  += sprintf(buf+len, "%02x ", rd);
		if(i%16 == 15)
			len  += sprintf(buf+len, "\n");
	}

	return len;
}
static ssize_t show_Resolution(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	//horizontal
	int it, ha, va;
	it = ch7025_read_data(0x0F) & 0x07;
	ha = (it << 8) | ch7025_read_data(0x10);
	//vertical
	it = ch7025_read_data(0x15) & 0x07;
	va = (it << 8) | ch7025_read_data(0x16);
	
	return sprintf(buf, "horizontal = %d, vertical=%d\n", ha, va);
	
}
static ssize_t show_Saturation(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	int sat = ch7025_read_data(0x2F) & 0x7F ;

	return sprintf(buf, "Saturation = %d\n", sat);
}
static ssize_t store_Saturation(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int sat;
	u8 old;

	sscanf(buf, "%d", &sat);
	old = ch7025_read_data(0x2F);
	old = (old & 0x80) | (Limit(sat,0,127) & 0x7F);
	ch7025_write_data(0x2F,old);

	return count;
}
static ssize_t show_Hue(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	int hue = ch7025_read_data(0x2E) & 0x7F;

	return sprintf(buf, "Hue = %d\n", hue);
}
static ssize_t store_Hue(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int hue;
	u8 old;

    	sscanf(buf, "%d", &hue);
	old = ch7025_read_data(0x2E);
	old = (old & 0x80) | (Limit(hue,0,127) & 0x7F);
	ch7025_write_data(0x2E,old);
	
	return count;
}
static ssize_t show_Contrast(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	int contrast = ch7025_read_data(0x30) & 0x7F;

	return sprintf(buf, "contrast = %d\n", contrast);
}
static ssize_t store_Contrast(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int cta;
	u8 old;

	sscanf(buf, "%d", &cta);
	old = ch7025_read_data(0x30);
	old = (old & 0x80) | (Limit(cta,0,127) & 0x7F);
	ch7025_write_data(0x30,old);

	return count;
}
static ssize_t show_Brightness(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	int brightness = ch7025_read_data(0x31) & 0xFF;

	return sprintf(buf, "brightness = %d\n", brightness);
}
static ssize_t store_Brightness(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int bri;

	sscanf(buf, "%d", &bri);
	ch7025_write_data(0x31, (Limit(bri,0,255) & 0xFF));

	return count;
}

static ssize_t store_SetInputSignal(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	sscanf(buf, "%d %d %d %d %d", &(gInputInfo.xres), &(gInputInfo.yres), 
			&(gInputInfo.iformat), &(gInputInfo.oformat), &(gInputInfo.swap));
	
	return count;
}
static ssize_t store_ResetChip(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int enable;
	sscanf(buf, "%d", &enable);
	ch7025_enable((bool)enable);
	
	return count;
}


static DEVICE_ATTR(AllParam, S_IRUGO, show_AllParam, NULL);
static DEVICE_ATTR(Resolution, S_IRUGO, show_Resolution, NULL);
static DEVICE_ATTR(Saturation, S_IWUSR | S_IRUGO, show_Saturation, store_Saturation);
static DEVICE_ATTR(Hue, S_IWUSR | S_IRUGO, show_Hue, store_Hue);
static DEVICE_ATTR(Contrast, S_IWUSR | S_IRUGO, show_Contrast, store_Contrast);
static DEVICE_ATTR(Brightness, S_IWUSR | S_IRUGO, show_Brightness, store_Brightness);
static DEVICE_ATTR(SetInputSignal, S_IWUSR , NULL, store_SetInputSignal);
static DEVICE_ATTR(ResetChip, S_IWUSR , NULL, store_ResetChip);

static struct attribute *ch7025_attributes[] = {
	&dev_attr_AllParam.attr,
	&dev_attr_Resolution.attr,
	&dev_attr_Saturation.attr,
	&dev_attr_Hue.attr,
	&dev_attr_Contrast.attr,
	&dev_attr_Brightness.attr,
	&dev_attr_SetInputSignal,
	&dev_attr_ResetChip,	
	NULL
};

static const struct attribute_group ch7025_attr_group = {
	.attrs = ch7025_attributes,
};
static int __devinit ch7025_probe(struct i2c_client *client,
			       const struct i2c_device_id *id)
{
	u8 val;
	int err;
	struct i2c_adapter *adapter = client->adapter;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;
	
	//Make sure CH7025/26B in the system:
	val = i2c_smbus_read_byte_data(client, 0x00);
	if ( (val != CH7025_DID ) && (val != CH7026_DID))
	{
		dev_err(&client->dev, "Failed to initialize Chrontel CH7025/CH7026 TV/VGA Encoder");
		return -EIO; // CH7025/26B was not found
	}
	
	//initialize variable
	gInputInfo.xres = 320;
	gInputInfo.yres = 240;
	gInputInfo.iformat = CH7025_RGB888;
	gInputInfo.oformat = CH7025_NTSC_M;
	gInputInfo.swap = CH7025_RGB_ORDER;
	gEnable = 0;
	ch7025_client = client;

	printk("Chrontel CH7025/CH7026 TV/VGA Encoder driver loaded successfully\n");
	
	err = sysfs_create_group(&client->dev.kobj, &ch7025_attr_group);
	
	if(err)
		printk("ch7025 sysfs create fail!\n");	

	return 0;
}
static int ch7025_remove(struct i2c_client *client)
{
	sysfs_remove_group(&client->dev.kobj, &ch7025_attr_group);
	return 0;
}
static int ch7025_suspend(struct i2c_client *client)
{
	return 0;
}
static int ch7025_resume(struct i2c_client *client)
{
	ch7025_enable(gEnable);
	return 0;
}
static struct i2c_driver ch7025_driver = {
	.driver = {
		.name   = "ch7025_i2c",
		.owner  = THIS_MODULE,
	},
	.probe          = ch7025_probe,
	.remove         = ch7025_remove,
	.suspend        = ch7025_suspend,
	.resume         = ch7025_resume,
	.id_table       = ch7025_id,
};

static int __init ch7025_init(void)
{
	return i2c_add_driver(&ch7025_driver);
}

static void __exit ch7025_exit(void)
{
	i2c_del_driver(&ch7025_driver);
}

MODULE_AUTHOR("Marcel Chang <marcelc@marvell.com>");
MODULE_DESCRIPTION("Chrontel CH7025/CH7026 TV/VGA Encoder");
MODULE_LICENSE("GPL");

module_init(ch7025_init);
module_exit(ch7025_exit);
