/*
 * A V4L2 driver for OmniVision OV7680 cameras.
 *
 * This file may be distributed under the terms of the GNU General
 * Public License, version 2.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-i2c-drv.h>


MODULE_DESCRIPTION("A low-level driver for OmniVision ov7680 sensors");
MODULE_LICENSE("GPL");

static int debug;
module_param(debug, bool, 0644);
MODULE_PARM_DESC(debug, "Debug level (0-1)");

/*
 * Use a more agressive configuration instead of the default one.
 */
#define ALT_CONFIG_SET

/*
 * Basic window sizes.  These probably belong somewhere more globally
 * useful.
 */
#define VGA_WIDTH	640
#define VGA_HEIGHT	480
#define QVGA_WIDTH	320
#define QVGA_HEIGHT	240
#define CIF_WIDTH	352
#define CIF_HEIGHT	288
#define QCIF_WIDTH	176
#define	QCIF_HEIGHT	144

/*
 * Our nominal (default) frame rate.
 */
#define OV7680_FRAME_RATE 30

/*
 * The 7680 sits on i2c with ID 0x42
 */
#define OV7680_I2C_ADDR 0x42

/* Registers */
#define REG_CLKRC	0x11	/* Clocl control */
#define   CLK_EXT	  0x40	  /* Use external clock directly */
#define   CLK_SCALE	  0x3f	  /* Mask for internal clock scale */

#define REG_COM12	0x12	/* Control 12 */
#define   COM12_RESET	  0x80	  /* Register reset */

#define REG_PIDH	0xA
#define REG_PIDL	0xB

#define REG_0C		0xC
#define   REG_0C_HFLIP	(1 << 6)
#define   REG_0C_VFLIP	(1 << 7)
#define REG_MIDH	0x1C
#define REG_MIDL	0x1D

/*
 * Information we maintain about a known sensor.
 */
struct ov7680_format_struct;  /* coming later */
struct ov7680_info {
	struct v4l2_subdev sd;
	struct ov7680_format_struct *fmt;  /* Current format */
	unsigned char sat;		/* Saturation value */
	int hue;			/* Hue value */
};

static inline struct ov7680_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct ov7680_info, sd);
}



/*
 * The default register settings, as obtained from OmniVision.  There
 * is really no making sense of most of these - lots of "reserved" values
 * and such.
 *
 * These settings give VGA YUYV.
 */

struct regval_list {
	unsigned char reg_num;
	unsigned char value;
};

static struct regval_list ov7680_default_regs[] = {
	{REG_COM12, COM12_RESET },

	/* Clock */
	{0x11, 0x00},   /* out=int/2 */
	{0x29, 0x0a},

	{REG_COM12, 0x0},
	{0x1e, 0x91},
	{0x14, 0xb2},
	{0x63, 0x0b},
	{0x64, 0x0f},
	{0x65, 0x07},
	{0x0c, 0x06},
	{0x82, 0x9a},
	{0x22, 0x40},
	{0x23, 0x20},
	{0x40, 0x10},
	{0x4a, 0x42},
#ifdef ALT_CONFIG_SET
	{0x4b, 0x64},
#else
	{0x4b, 0x42},
#endif
	{0x67, 0x50},
	{0x6b, 0x00},
	{0x6c, 0x00},
	{0x6d, 0x00},
	{0x6e, 0x00},
	{0x83, 0x00},
	{0x0c, 0x16},
	{0x82, 0x98},
	{0x3e, 0x30},
	{0x81, 0x06},

	{0x42, 0x4a},
	{0x45, 0x40},
	{0x3f, 0x46},
	{0x48, 0x20},
	{0x21, 0x23},
	{0x5a, 0x10},
#ifdef ALT_CONFIG_SET
	{0x5c, 0xe0},
#endif
	{0x27, 0x33},
	{0x4d, 0x1d},
	{0x42, 0x4a},
	{0x4e, 0x54},	/* IO2.8v */

	/* awb */
#ifdef ALT_CONFIG_SET
	{0x8F, 0x54},
	{0x90, 0x69},
	{0x91, 0x0f},
	{0x92, 0x99},
	{0x93, 0x98},
	{0x94, 0x0f},
	{0x95, 0x0f},
	{0x96, 0xff},
	{0x97, 0x00},
	{0x98, 0x10},
	{0x99, 0x20},
	{0x65, 0x87},
	{0x66, 0x02},
	{0x67, 0x5c},
	{0x32, 0xa0},
	{0x33, 0x00},
	{0x2f, 0x16},
	{0x30, 0x2a},
	{0x38, 0x84},
	{0x55, 0x86},
	{0x58, 0x83},
	{0x13, 0xb7},
	{0x38, 0x84},
	{0x59, 0x0e},
	{0x5f, 0x20},
	{0x56, 0x40},
	{0x51, 0x00},
	{0x57, 0x22},
	{0x59, 0x07},
	{0x5f, 0xa3},
	{0x59, 0x08},
	{0x5f, 0x54},
	{0x52, 0x30},
	{0x56, 0x29},
	{0x53, 0x20},
	{0x54, 0x30},
	{0x59, 0x00},
	{0x5f, 0xf0},
	{0x59, 0x01},
	{0x5f, 0xf0},
	{0x59, 0x0f},
	{0x5f, 0x20},
	{0x5f, 0x00},
	{0x59, 0x10},
	{0x5f, 0x7e},
	{0x59, 0x0a},
	{0x5f, 0x80},
	{0x59, 0x0b},
	{0x5f, 0x01},
	{0x59, 0x0C},
	{0x5f, 0x07},
	{0x5f, 0x0f},
	{0x59, 0x0d},
	{0x5f, 0x20},
	{0x59, 0x09},
	{0x5f, 0x30},
	{0x59, 0x02},
	{0x5f, 0x80},
	{0x59, 0x03},
	{0x5f, 0x60},
	{0x59, 0x04},
	{0x5f, 0xf0},
	{0x59, 0x05},
	{0x5f, 0x80},
	{0x59, 0x06},
	{0x5f, 0x04},
	{0x59, 0x26},
	{0x59, 0x0b},
	{0x5f, 0x31},
	{0x55, 0xa8},
	{0x27, 0xb3},
	{0x40, 0x23},
	{0x4d, 0x1d},
#else
	{0x97, 0x02},
	{0x96, 0xf8},
	{0x98, 0x37},
	{0x9a, 0x52},
	{0x99, 0x33},
	{0x9b, 0x42},
	{0x94, 0x0e},
	{0x92, 0x8c},
	{0x93, 0x96},
	{0x95, 0x0e},
	{0x8f, 0x55},
	{0x90, 0x58},
	{0x91, 0x0c},
	{0x89, 0x5c},
	{0x8b, 0x12},

	{0x65, 0x87},
	/* {0x66, 0x02}, */
	{0x67, 0x5c},
	{0x32, 0xa0},
	{0x33, 0x00},
	{0x2f, 0x16},
	{0x30, 0x2a},
	{0x38, 0x84},
	{0x55, 0x86},
	{0x58, 0x83},
	{0x13, 0xff},	/* ;f7 */
	{0x38, 0x84},

	{0x56, 0x40},
	{0x22, 0x70},
	{0x51, 0x00},
	{0x57, 0x2c},
	{0x59, 0x07},
	{0x5f, 0xa2},
	{0x59, 0x08},
	{0x5f, 0x54},
	{0x52, 0x30},
	{0x53, 0x20},
	{0x54, 0x30},
	{0x59, 0x00},
	{0x5f, 0xf0},
	{0x59, 0x01},
	{0x5f, 0xf0},
	{0x59, 0x0f},
	{0x5f, 0x20},
	{0x5f, 0x00},
	{0x59, 0x10},
	{0x5f, 0x7e},
	{0x59, 0x0a},
	{0x5f, 0x80},
	{0x59, 0x0b},
	{0x5f, 0x01},
	{0x59, 0x0c},
	{0x5f, 0x07},
	{0x5f, 0x0f},
	{0x59, 0x0d},
	{0x5f, 0x20},
	{0x59, 0x0e},
	{0x5f, 0x20},
	{0x59, 0x09},
	{0x5f, 0x30},
	{0x59, 0x02},
	{0x5f, 0x80},
	{0x59, 0x03},
	{0x5f, 0x60},
	{0x59, 0x04},
	{0x5f, 0xf0},
	{0x59, 0x05},
	{0x5f, 0x80},
	{0x59, 0x06},
	{0x5f, 0x04},
	{0x59, 0x26},
	{0x59, 0x0b},
	{0x5f, 0x31},
	{0x55, 0xa8},
	{0x56, 0x29},

	{0x27, 0xb3},
	{0x40, 0x23},
	{0x4d, 0x2d},
#endif

	/* Color Matrix */
#ifdef ALT_CONFIG_SET
	{0xb7, 0x98},
	{0xb8, 0x98},
	{0xb9, 0x00},
	{0xba, 0x28},
	{0xbb, 0x70},
	{0xbc, 0x98},
	{0xbd, 0x5a},
#else
	{0xba, 0x18},
	{0xbb, 0x80},
	{0xbc, 0x98},
	{0xb7, 0x90},
	{0xb8, 0x8f},
	{0xb9, 0x01},
	{0xbd, 0x5a},
#endif

	{0xbe, 0xb0},
	{0xbf, 0x9d},
	{0xc0, 0x13},
	{0xc1, 0x16},
	{0xc2, 0x7b},
	{0xc3, 0x91},
	{0xc4, 0x1e},
	{0xc5, 0x9d},
	{0xc6, 0x9a},
	{0xc7, 0x03},
	{0xc8, 0x2e},
	{0xc9, 0x91},
	{0xca, 0xbf},
	{0xcb, 0x1e},

	/* Gamma */
#ifdef ALT_CONFIG_SET
	{0xaf, 0x1e},
	{0xa0, 0x06},
	{0xa1, 0x18},
	{0xa2, 0x2a},
	{0xa3, 0x50},
	{0xa4, 0x5f},
	{0xa5, 0x6c},
	{0xa6, 0x79},
	{0xa7, 0x84},
	{0xa8, 0x8d},
	{0xa9, 0x96},
	{0xaa, 0xa5},
	{0xab, 0xb0},
	{0xac, 0xc6},
	{0xad, 0xd4},
	{0xae, 0xea},
#else
	{0xaf, 0x30},
	{0xa0, 0x01},
	{0xa1, 0x0f},
	{0xa2, 0x1d},
	{0xa3, 0x3c},
	{0xa4, 0x4a},
	{0xa5, 0x5a},
	{0xa6, 0x69},
	{0xa7, 0x77},
	{0xa8, 0x83},
	{0xa9, 0x8e},
	{0xaa, 0x9f},
	{0xab, 0xac},
	{0xac, 0xc1},
	{0xad, 0xcc},
	{0xae, 0xd0},
#endif

	{0x89, 0x5c},
	{0x8a, 0x11},
#ifdef ALT_CONFIG_SET
	{0x8b, 0x92},
#else
	{0x8b, 0x12},
#endif
	{0x8c, 0x11},
	{0x8d, 0x52},
#ifdef ALT_CONFIG_SET
	{0x96, 0xff},
	{0x97, 0x00},
	{0x9c, 0xf0},
#else
	{0x96, 0xf8},
	{0x97, 0x02},
	{0x9c, 0x64},
#endif
	{0x9d, 0xf0},
	{0x9e, 0xf0},

#ifdef ALT_CONFIG_SET
	{0xb2, 0x06},
	{0xb3, 0x03},
	{0xb4, 0x05},
	{0xb5, 0x04},
	{0xb6, 0x02},	/* ; 03 */

	{0xd5, 0x06},
	{0xd6, 0x10},
#else
	{0xb2, 0x03},
	{0xb3, 0x02},
	{0xb4, 0x65},
	{0xb5, 0x02},
	{0xb6, 0x02},
#endif
	{0xdb, 0x40},
	{0xdc, 0x40},
	{0xdf, 0x09},

#ifdef ALT_CONFIG_SET
	{0x11, 0x00},
#endif
	{0x24, 0x40},
#ifdef ALT_CONFIG_SET
	{0x25, 0x30},
	{0x26, 0x81},
#else
	{0x25, 0x38},
	{0x26, 0x82},
	{0x4f, 0x4d},
	{0x50, 0x40},
#endif

	{0x5a, 0x14},
#ifdef ALT_CONFIG_SET
	{0x5b, 0xe7},
	{0x5c, 0x1f},
#else
	{0x5b, 0x00},
	{0x5c, 0x1c},
#endif
	{0x5d, 0x30},
	{0x81, 0x07},

	/* Lens Correction */
#ifdef ALT_CONFIG_SET
	{0x31, 0x0f},
	{0x32, 0x00},
	{0x35, 0x15},
	{0x37, 0x5d},	/* for color R */
	{0x34, 0x48},	/* for color G */
	{0x36, 0x50},	/* for color B */
#else
	{0x31, 0x0f},
	{0x32, 0x8c},
	{0x33, 0x37},
	{0x35, 0x10},
	{0x37, 0x48},
	{0x34, 0x3b},
	{0x36, 0x3a},
#endif

	{0x84, 0x02},

	/* 30fps */
	{0x2a, 0xb0},
	{0x2b, 0x0b},
	{0x4f, 0x9a},	/* banding value */
	{0x50, 0x80},
	{0x21, 0x23},   /* banding step */

	/* General control */
	{0x14, 0xa0},	/* 8x max gain */
	{0x15, 0x64},	/* disable night mode */

#ifndef ALT_CONFIG_SET
	/* Contrast adjustment */
	{0xd5, 0x06},
	{0xd6, 0x00},
	{0xd7, 0x25},
	{0xdf, 0x01},
	{0xdb, 0x4c},
	{0xdc, 0x40},

	/* AE Range */
	{0x24, 0x50},
	{0x25, 0x40},
	{0x26, 0x83},

	/* Black level */
	{0x66, 0x02},	/* BLC */
	{0x80, 0x7f},
	{0x85, 0x01},
#endif

	{ 0xff, 0xff },	/* END MARKER */
};


/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 * RGB656 and YUV422 come from OV; RGB444 is homebrewed.
 *
 * IMPORTANT RULE: the first entry must be for COM7, see ov7680_s_fmt for why.
 */


static struct regval_list ov7680_fmt_yuv422[] = {
	{ REG_COM12, 0x0 },  /* Selects YUV mode */
	{ 0x0c, 0x16 },
	{ 0xff, 0xff },
};


/*
 * Low-level register I/O.
 */

static int ov7680_read(struct v4l2_subdev *sd, unsigned char reg,
		unsigned char *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret >= 0) {
		*value = (unsigned char)ret;
		ret = 0;
	}
	return ret;
}


static int ov7680_write(struct v4l2_subdev *sd, unsigned char reg,
		unsigned char value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = i2c_smbus_write_byte_data(client, reg, value);

	if (reg == REG_COM12 && (value & COM12_RESET))
		msleep(2);  /* Wait for reset to run */
	return ret;
}


/*
 * Write a list of register settings; ff/ff stops the process.
 */
static int ov7680_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	while (vals->reg_num != 0xff || vals->value != 0xff) {
		int ret = ov7680_write(sd, vals->reg_num, vals->value);
		if (ret < 0)
			return ret;
		vals++;
	}
	return 0;
}


/*
 * Stuff that knows about the sensor.
 */
static int ov7680_reset(struct v4l2_subdev *sd, u32 val)
{
	ov7680_write(sd, REG_COM12, COM12_RESET);
	msleep(1);
	return 0;
}


static int ov7680_init(struct v4l2_subdev *sd, u32 val)
{
	return ov7680_write_array(sd, ov7680_default_regs);
}



static int ov7680_detect(struct v4l2_subdev *sd)
{
	unsigned char v;
	int ret;

	ret = ov7680_read(sd, REG_MIDH, &v);
	if (ret < 0)
		return ret;
	if (v != 0x7f) /* OV manuf. id. */
		return -ENODEV;
	ret = ov7680_read(sd, REG_MIDL, &v);
	if (ret < 0)
		return ret;
	if (v != 0xa2)
		return -ENODEV;
	/*
	 * OK, we know we have an OmniVision chip...but which one?
	 */
	ret = ov7680_read(sd, REG_PIDH, &v);
	if (ret < 0)
		return ret;
	if (v != 0x76)  /* PIDH = 0x76 */
		return -ENODEV;
	ret = ov7680_read(sd, REG_PIDL, &v);
	if (ret < 0)
		return ret;
	if (v != 0x80)  /* PIDL = 0x80 */
		return -ENODEV;

	ret = ov7680_init(sd, 0);

	return ret;
}


/*
 * Store information about the video data format.  The color matrix
 * is deeply tied into the format, so keep the relevant values here.
 * The magic matrix nubmers come from OmniVision.
 */
static struct ov7680_format_struct {
	__u8 *desc;
	__u32 pixelformat;
	struct regval_list *regs;
	int bpp;   /* Bytes per pixel */
} ov7680_formats[] = {
	{
		.desc		= "YUYV 4:2:2",
		.pixelformat	= V4L2_PIX_FMT_YUYV,
		.regs 		= ov7680_fmt_yuv422,
		.bpp		= 2,
	},
};
#define N_OV7680_FMTS ARRAY_SIZE(ov7680_formats)

static struct regval_list ov7680_vga_regs[] = {
	{ 0x81, 0x07 },
	{ 0x84, 0x62 },
	{ 0xd0, 0xa4 },
	{ 0xd1, 0x78 },
	{ 0xd2, 0xa0 },
	{ 0xd3, 0x78 },
	{ 0xd4, 0x20 },
	{ 0xff, 0xff },
};

static struct regval_list ov7680_cif_regs[] = {
	{ 0x81, 0xff },
	{ 0x84, 0x02 },
	{ 0xd0, 0xa4 },
	{ 0xd1, 0x78 },
	{ 0xd2, 0x58 },
	{ 0xd3, 0x48 },
	{ 0xd4, 0x20 },
	{ 0xff, 0xff },
};

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */
static struct ov7680_win_size {
	int	width;
	int	height;
	int	hstart;		/* Start/size values for the camera.  Note */
	int	hsize;		/* that they do not always make complete */
	int	vstart;		/* sense to humans, but evidently the sensor */
	int	vsize;		/* will do the right thing... */
	struct regval_list *regs; /* Regs to tweak */
/* h/vref stuff */
} ov7680_win_sizes[] = {
	/* VGA */
	{
		.width		= VGA_WIDTH,
		.height		= VGA_HEIGHT,
		.hstart		= 104,		/* Value from Omnivision */
		.hsize		= ((VGA_WIDTH+16)/4),
		.vstart		=  14,
		.vsize		= ((VGA_HEIGHT+8)/2),
		.regs 		= ov7680_vga_regs,
	},
	/* CIF */
	{
		.width		= CIF_WIDTH,
		.height		= CIF_HEIGHT,
		.hstart		= 104,		/* Value from Omnivision */
		.hsize		= ((VGA_WIDTH+16)/4),
		.vstart		=  14,
		.vsize		= ((VGA_HEIGHT+8)/2),
		.regs 		= ov7680_cif_regs,
	},
};

#define N_WIN_SIZES (ARRAY_SIZE(ov7680_win_sizes))

static int ov7680_enum_fmt(struct v4l2_subdev *sd, struct v4l2_fmtdesc *fmt)
{
	struct ov7680_format_struct *ofmt;

	if (fmt->index >= N_OV7680_FMTS)
		return -EINVAL;

	ofmt = ov7680_formats + fmt->index;
	fmt->flags = 0;
	strcpy(fmt->description, ofmt->desc);
	fmt->pixelformat = ofmt->pixelformat;
	return 0;
}


static int ov7680_try_fmt_internal(struct v4l2_subdev *sd,
		struct v4l2_format *fmt,
		struct ov7680_format_struct **ret_fmt,
		struct ov7680_win_size **ret_wsize)
{
	int index;
	struct ov7680_win_size *wsize;
	struct v4l2_pix_format *pix = &fmt->fmt.pix;

	for (index = 0; index < N_OV7680_FMTS; index++)
		if (ov7680_formats[index].pixelformat == pix->pixelformat)
			break;
	if (index >= N_OV7680_FMTS) {
		/* default to first format */
		index = 0;
		pix->pixelformat = ov7680_formats[0].pixelformat;
	}
	if (ret_fmt != NULL)
		*ret_fmt = ov7680_formats + index;
	/*
	 * Fields: the OV devices claim to be progressive.
	 */
	pix->field = V4L2_FIELD_NONE;
	/*
	 * Round requested image size down to the nearest
	 * we support, but not below the smallest.
	 */
	for (wsize = ov7680_win_sizes; wsize < ov7680_win_sizes + N_WIN_SIZES;
	     wsize++)
		if (pix->width >= wsize->width && pix->height >= wsize->height)
			break;
	if (wsize >= ov7680_win_sizes + N_WIN_SIZES)
		wsize--;   /* Take the smallest one */
	if (ret_wsize != NULL)
		*ret_wsize = wsize;
	/*
	 * Note the size we'll actually handle.
	 */
	pix->width = wsize->width;
	pix->height = wsize->height;
	pix->bytesperline = pix->width*ov7680_formats[index].bpp;
	pix->sizeimage = pix->height*pix->bytesperline;
	return 0;
}

static int ov7680_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	return ov7680_try_fmt_internal(sd, fmt, NULL, NULL);
}

/*
 * Set a format.
 */
static int ov7680_s_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int ret;
	struct ov7680_format_struct *ovfmt;
	struct ov7680_win_size *wsize;
	struct ov7680_info *info = to_state(sd);

	ret = ov7680_try_fmt_internal(sd, fmt, &ovfmt, &wsize);
	if (ret)
		return ret;
	/*
	 * write the array.
	 */
	if (wsize->regs)
		ret += ov7680_write_array(sd, wsize->regs);
	ret += ov7680_write_array(sd, ovfmt->regs);
	info->fmt = ovfmt;
	return ret;
}

/*
 * Implement G/S_PARM.  There is a "high quality" mode we could try
 * to do someday; for now, we just do the frame rate tweak.
 */
static int ov7680_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	unsigned char clkrc;
	int ret;

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	ret = ov7680_read(sd, REG_CLKRC, &clkrc);
	if (ret < 0)
		return ret;
	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->timeperframe.numerator = 1;
	cp->timeperframe.denominator = OV7680_FRAME_RATE;
	if ((clkrc & CLK_EXT) == 0 && (clkrc & CLK_SCALE) > 1)
		cp->timeperframe.denominator /= (clkrc & CLK_SCALE);
	return 0;
}

static int ov7680_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct v4l2_fract *tpf = &cp->timeperframe;
	unsigned char clkrc;
	int ret, div;

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	if (cp->extendedmode != 0)
		return -EINVAL;
	/*
	 * CLKRC has a reserved bit, so let's preserve it.
	 */
	ret = ov7680_read(sd, REG_CLKRC, &clkrc);
	if (ret < 0)
		return ret;
	if (tpf->numerator == 0 || tpf->denominator == 0)
		div = 1;  /* Reset to full rate */
	else
		div = (tpf->numerator*OV7680_FRAME_RATE)/tpf->denominator;
	if (div == 0)
		div = 1;
	else if (div > CLK_SCALE)
		div = CLK_SCALE;
	clkrc = (clkrc & 0xc0) | div;
	tpf->numerator = 1;
	tpf->denominator = OV7680_FRAME_RATE/div;
	return ov7680_write(sd, REG_CLKRC, clkrc);
}



/*
 * Code for dealing with controls.
 */
static int ov7680_g_hflip(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	unsigned char v = 0;

	ret = ov7680_read(sd, REG_0C, &v);
	*value = (v & REG_0C_HFLIP) == REG_0C_HFLIP;
	return ret;
}


static int ov7680_s_hflip(struct v4l2_subdev *sd, int value)
{
	unsigned char v = 0;
	int ret;

	ret = ov7680_read(sd, REG_0C, &v);
	if (value)
		v |= REG_0C_HFLIP;
	else
		v &= ~REG_0C_HFLIP;
	msleep(10);  /* FIXME */
	ret += ov7680_write(sd, REG_0C, v);
	return ret;
}



static int ov7680_g_vflip(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	unsigned char v = 0;

	ret = ov7680_read(sd, REG_0C, &v);
	*value = (v & REG_0C_VFLIP) == REG_0C_VFLIP;
	return ret;
}


static int ov7680_s_vflip(struct v4l2_subdev *sd, int value)
{
	unsigned char v = 0;
	int ret;

	ret = ov7680_read(sd, REG_0C, &v);
	if (value)
		v |= REG_0C_VFLIP;
	else
		v &= ~REG_0C_VFLIP;
	msleep(10);  /* FIXME */
	ret += ov7680_write(sd, REG_0C, v);
	return ret;
}

static int ov7680_queryctrl(struct v4l2_subdev *sd,
		struct v4l2_queryctrl *qc)
{
	/* Fill in min, max, step and default value for these controls. */
	switch (qc->id) {
	case V4L2_CID_VFLIP:
	case V4L2_CID_HFLIP:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	}
	return -EINVAL;
}

static int ov7680_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_VFLIP:
		return ov7680_g_vflip(sd, &ctrl->value);
	case V4L2_CID_HFLIP:
		return ov7680_g_hflip(sd, &ctrl->value);
	}
	return -EINVAL;
}

static int ov7680_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_VFLIP:
		return ov7680_s_vflip(sd, ctrl->value);
	case V4L2_CID_HFLIP:
		return ov7680_s_hflip(sd, ctrl->value);
	}
	return -EINVAL;
}

static int ov7680_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_OV7680, 0);
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ov7680_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = ov7680_read(sd, reg->reg & 0xff, &val);
	reg->val = val;
	reg->size = 1;
	return ret;
}

static int ov7680_s_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ov7680_write(sd, reg->reg & 0xff, reg->val & 0xff);
	return 0;
}
#endif

/* ----------------------------------------------------------------------- */

static const struct v4l2_subdev_core_ops ov7680_core_ops = {
	.g_chip_ident = ov7680_g_chip_ident,
	.g_ctrl = ov7680_g_ctrl,
	.s_ctrl = ov7680_s_ctrl,
	.queryctrl = ov7680_queryctrl,
	.reset = ov7680_reset,
	.init = ov7680_init,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = ov7680_g_register,
	.s_register = ov7680_s_register,
#endif
};

static const struct v4l2_subdev_video_ops ov7680_video_ops = {
	.enum_fmt = ov7680_enum_fmt,
	.try_fmt = ov7680_try_fmt,
	.s_fmt = ov7680_s_fmt,
	.s_parm = ov7680_s_parm,
	.g_parm = ov7680_g_parm,
};

static const struct v4l2_subdev_ops ov7680_ops = {
	.core = &ov7680_core_ops,
	.video = &ov7680_video_ops,
};

/* ----------------------------------------------------------------------- */

static int ov7680_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct ov7680_info *info;
	int ret;

	info = kzalloc(sizeof (struct ov7680_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	v4l2_i2c_subdev_init(sd, client, &ov7680_ops);

	/* Make sure it's an ov7680 */
	ret = ov7680_detect(sd);
	if (ret) {
		v4l_dbg(1, debug, client,
			"chip found @ 0x%x (%s) is not an ov7680 chip.\n",
			client->addr << 1, client->adapter->name);
		kfree(info);
		return ret;
	}
	v4l_info(client, "chip found @ 0x%02x (%s)\n",
			client->addr << 1, client->adapter->name);

	info->fmt = &ov7680_formats[0];
	info->sat = 128;	/* Review this */

	return 0;
}


static int ov7680_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);

	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id ov7680_id[] = {
	{ "ov7680", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov7680_id);

static struct v4l2_i2c_driver_data v4l2_i2c_data = {
	.name = "ov7680",
	.probe = ov7680_probe,
	.remove = ov7680_remove,
	.id_table = ov7680_id,
};
