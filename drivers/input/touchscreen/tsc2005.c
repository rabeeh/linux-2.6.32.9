/*
 *	Driver for TI Touch Screen Controller TSC2005 
 *	 
 * 
 * TSC Interfaced with Atmel AT91sam9261ek board,
 * with PENIRQ connected to AT91_PIN_PB0 and on
 * SPI0.2 (spi0 chip_select2)
 * modalias: "tsc2005"
 * 
 * 
 * Author	: Pavan Savoy (pavan_savoy@mindtree.com)
 * Date		: 15-05-2008
 * Author	: Ethan Ku (eku@marvell.com)
 */

/*
 *	Header file also has the configuration/build options
 */
#include <linux/spi/tsc200x.h>
#include <linux/irq.h>

#define TSC2005_VDD_LOWER_27

#ifdef TSC2005_VDD_LOWER_27
#define TSC2005_HZ     (10000000)
#else
#define TSC2005_HZ     (25000000)
#endif

#define TSC2005_CMD	(0x80)
#define TSC2005_REG	(0x00)

#define TSC2005_CMD_STOP	(1 << 0)
#define TSC20025_CMD_SWRST      (1 << 1)
#define TSC2005_CMD_10BIT	(0 << 2)
#define TSC2005_CMD_12BIT	(1 << 2)

#define TSC2005_CMD_SCAN_XYZZ	(0 << 3)
#define TSC2005_CMD_SCAN_XY	(1 << 3)
#define TSC2005_CMD_SCAN_X	(2 << 3)
#define TSC2005_CMD_SCAN_Y	(3 << 3)
#define TSC2005_CMD_SCAN_ZZ	(4 << 3)
#define TSC2005_CMD_AUX_SINGLE	(5 << 3)
#define TSC2005_CMD_TEMP1	(6 << 3)
#define TSC2005_CMD_TEMP2	(7 << 3)
#define TSC2005_CMD_AUX_CONT	(8 << 3)
#define TSC2005_CMD_TEST_X_CONN	(9 << 3)
#define TSC2005_CMD_TEST_Y_CONN	(10 << 3)
/* command 11 reserved */
#define TSC2005_CMD_TEST_SHORT	(12 << 3)
#define TSC2005_CMD_DRIVE_XX	(13 << 3)
#define TSC2005_CMD_DRIVE_YY	(14 << 3)
#define TSC2005_CMD_DRIVE_YX	(15 << 3)

#define TSC2005_REG_X		(0 << 3)
#define TSC2005_REG_Y		(1 << 3)
#define TSC2005_REG_Z1		(2 << 3)
#define TSC2005_REG_Z2		(3 << 3)
#define TSC2005_REG_AUX		(4 << 3)
#define TSC2005_REG_TEMP1	(5 << 3)
#define TSC2005_REG_TEMP2	(6 << 3)
#define TSC2005_REG_STATUS	(7 << 3)
#define TSC2005_REG_AUX_HIGH	(8 << 3)
#define TSC2005_REG_AUX_LOW	(9 << 3)
#define TSC2005_REG_TEMP_HIGH	(10 << 3)
#define TSC2005_REG_TEMP_LOW	(11 << 3)
#define TSC2005_REG_CFR0	(12 << 3)
#define TSC2005_REG_CFR1	(13 << 3)
#define TSC2005_REG_CFR2	(14 << 3)
#define TSC2005_REG_FUNCTION	(15 << 3)

#define TSC2005_REG_PND0	(1 << 1)
#define TSC2005_REG_READ	(0x01)
#define TSC2005_REG_WRITE	(0x00)


#define TSC2005_CFR0_LONGSAMPLING	(1)
#define TSC2005_CFR0_DETECTINWAIT	(1 << 1)
#define TSC2005_CFR0_SENSETIME_32US	(0)
#define TSC2005_CFR0_SENSETIME_96US	(1 << 2)
#define TSC2005_CFR0_SENSETIME_544US	(1 << 3)
#define TSC2005_CFR0_SENSETIME_2080US	(1 << 4)
#define TSC2005_CFR0_SENSETIME_2656US	(0x001C)
#define TSC2005_CFR0_PRECHARGE_20US	(0x0000)
#define TSC2005_CFR0_PRECHARGE_84US	(0x0020)
#define TSC2005_CFR0_PRECHARGE_276US	(0x0040)
#define TSC2005_CFR0_PRECHARGE_1044US	(0x0080)
#define TSC2005_CFR0_PRECHARGE_1364US	(0x00E0)
#define TSC2005_CFR0_STABTIME_0US	(0x0000)
#define TSC2005_CFR0_STABTIME_100US	(0x0100)
#define TSC2005_CFR0_STABTIME_500US	(0x0200)
#define TSC2005_CFR0_STABTIME_1MS	(0x0300)
#define TSC2005_CFR0_STABTIME_5MS	(0x0400)
#define TSC2005_CFR0_STABTIME_100MS	(0x0700)
#define TSC2005_CFR0_CLOCK_4MHZ		(0x0000)
#define TSC2005_CFR0_CLOCK_2MHZ		(0x0800)
#define TSC2005_CFR0_CLOCK_1MHZ		(0x1000)
#define TSC2005_CFR0_RESOLUTION12	(0x2000)
#define TSC2005_CFR0_STATUS		(0x4000)
#define TSC2005_CFR0_PENMODE_INITIATE_BY_TSC2005	(0x8000)
#define TSC2005_CFR0_PENMODE_INITIATE_BY_HOST		(0x0000)

#define TSC2005_CFR0_INITVALUE	(\
				TSC2005_CFR0_LONGSAMPLING |		\
				TSC2005_CFR0_SENSETIME_544US |		\
				TSC2005_CFR0_PRECHARGE_276US |		\
				TSC2005_CFR0_STABTIME_1MS  |		\
				TSC2005_CFR0_CLOCK_4MHZ    |		\
				TSC2005_CFR0_RESOLUTION12  |		\
				TSC2005_CFR0_PENMODE_INITIATE_BY_TSC2005)//0x3340


#define TSC2005_CFR1_BATCHDELAY_0MS	(0x0000)
#define TSC2005_CFR1_BATCHDELAY_1MS	(0x0001)
#define TSC2005_CFR1_BATCHDELAY_2MS	(0x0002)
#define TSC2005_CFR1_BATCHDELAY_4MS	(0x0003)

#define TSC2005_CFR1_BATCHDELAY_10MS	(0x0004)
#define TSC2005_CFR1_BATCHDELAY_20MS	(0x0005)
#define TSC2005_CFR1_BATCHDELAY_40MS	(0x0006)
#define TSC2005_CFR1_BATCHDELAY_100MS	(0x0007)

#define TSC2005_CFR1_INITVALUE	TSC2005_CFR1_BATCHDELAY_1MS

#define TSC2005_CFR2_MAVE_TEMP	(0x0001)
#define TSC2005_CFR2_MAVE_AUX	(0x0002)
#define TSC2005_CFR2_MAVE_Z	(0x0004)
#define TSC2005_CFR2_MAVE_Y	(0x0008)
#define TSC2005_CFR2_MAVE_X	(0x0010)
#define TSC2005_CFR2_AVG_1	(0x0000)
#define TSC2005_CFR2_AVG_3	(0x0400)
#define TSC2005_CFR2_AVG_7	(0x0800)
#define TSC2005_CFR2_MEDIUM_1	(0x0000)
#define TSC2005_CFR2_MEDIUM_3	(0x1000)
#define TSC2005_CFR2_MEDIUM_7	(0x2000)
#define TSC2005_CFR2_MEDIUM_15	(0x3000)

#define TSC2005_CFR2_IRQ_DAV	(0x4000)
#define TSC2005_CFR2_IRQ_PEN	(0x8000)
#define TSC2005_CFR2_IRQ_PENDAV	(0x0000)


#define TSC2005_CFR2_INITVALUE	(TSC2005_CFR2_MAVE_X | \
				 TSC2005_CFR2_MAVE_Y | \
				 TSC2005_CFR2_MEDIUM_7 |\
				 TSC2005_CFR2_AVG_3 |\
				 TSC2005_CFR2_IRQ_DAV)


#define LOBYTE(x) ((unsigned char) (x))
#define HIBYTE(x) ((unsigned char) ((x) >> 8))

static const u8 tsc2005_cfr_tbl[3][3] = {
	{ 
		TSC2005_REG | TSC2005_REG_CFR0 | TSC2005_REG_PND0 | TSC2005_REG_WRITE,
		HIBYTE(TSC2005_CFR0_INITVALUE), 
		LOBYTE(TSC2005_CFR0_INITVALUE) 
	},
	{ 
		TSC2005_REG | TSC2005_REG_CFR1 | TSC2005_REG_PND0 | TSC2005_REG_WRITE,
		HIBYTE(TSC2005_CFR1_INITVALUE), 
		LOBYTE(TSC2005_CFR1_INITVALUE) 
	},
	{ 
		TSC2005_REG | TSC2005_REG_CFR2 | TSC2005_REG_PND0 | TSC2005_REG_WRITE,
		HIBYTE(TSC2005_CFR2_INITVALUE), 
		LOBYTE(TSC2005_CFR2_INITVALUE) 
	},
};


static const u8 tsc2005_read_reg[] = {
	(TSC2005_REG | TSC2005_REG_X | TSC2005_REG_READ),
	(TSC2005_REG | TSC2005_REG_Y | TSC2005_REG_READ),
	(TSC2005_REG | TSC2005_REG_Z1 | TSC2005_REG_READ),
	(TSC2005_REG | TSC2005_REG_Z2 | TSC2005_REG_READ),
};
#define NUM_READ_REGS	(sizeof(tsc2005_read_reg)/sizeof(tsc2005_read_reg[0]))

/* 
 * Driver Data Structure 
 */
struct tsc2005 {
	char			phys[32];
/* 
 * An SPI & an Input driver 
 */
	struct spi_device *spi_dev;
	struct input_dev *input_dev;
/* 
 * SPI message & transfer related data 
 */

	struct spi_message	read_msg;
	struct spi_transfer	read_xfer[NUM_READ_REGS][2];
	u8                      data[NUM_READ_REGS][2];

	u16 x,y;
/* 
 * Timer data 
 */
	struct timer_list defer_irq_timer;
	struct timer_list penup_timer;

	int x_plate_ohms;
/* 
 * PENIRQ & function returning Line Status 
 */
	//	int (*get_pendown_state)(void);
	int penirq;

/* 
 * Power state- Device suspended 
 */
	unsigned disabled:1;
/* 
 * Spin_lock 
 */
	spinlock_t lock;
};



//#define SWAP_X
//#define SWAP_Y


#define TS_RECT_SIZE	8

#define INTERRUPT_DELAY   10  //msec
#define PENUP_TIMEOUT   100 //msec

//#define DEBUG


/* 
 * SPI completion handler - called on read_z2 
 */

static void tsc2005_complete(void *tsc)
{
	struct tsc2005 *ts = tsc;
	struct input_dev *ip = ts->input_dev;
	u16 x, y, z1,z2;
	unsigned int touch_pressure = 0;
	u32 inside_rect =0;

	#ifdef DEBUG
	printk("[%s]\n",__func__);
	#endif

	spin_lock_irq(&ts->lock);

	x = (ts->data[0][0] << 8) + ts->data[0][1];
	y = (ts->data[1][0] << 8) + ts->data[1][1];
	z1 = (ts->data[2][0] << 8) + ts->data[2][1];
	z2 = (ts->data[3][0] << 8) + ts->data[3][1];

#ifdef SWAP_X
	x = 4096 - x;
#endif
#ifdef SWAP_Y
	y = 4096 - y;
#endif

#ifdef DEBUG
//	printk("x:y:z1:z2=%d:%d:%d:%d\n",x,y,z1,z2);
	printk("x:y:z1:z2=%x:%x:%x:%x\n",x,y,z1,z2);
#endif

	if(z1){
/*
 * Formula - 1 used for Calculation of Touch_Pressure (R-Touch)
 * 450 being XPlate_Resistane - since ADS7846 environment had the same
 */
		
		touch_pressure = x*(z2-z1) / z1;
		touch_pressure = touch_pressure * ts->x_plate_ohms / 4096;
#ifdef DEBUG
		printk("touch_pressure = %d\n",touch_pressure);
#endif
	}



/*
 * Uncomment following line to report Actual Pressure
 * Although not an absolute must, make sure z1, z2 are consitant
 * before uncommenting
 */
/*
 * #define report_actual_pressure
 */

	if (touch_pressure)
	{


		inside_rect = ( x > (int)(ts->x - TS_RECT_SIZE) &&
				x < (int)(ts->x + TS_RECT_SIZE) &&				
				y > (int)(ts->y - TS_RECT_SIZE) &&
				y < (int)(ts->y + TS_RECT_SIZE));
		if(inside_rect)
		{
			x = ts->x;
			y = ts->y;
#ifdef DEBUG
			printk("inside rect\n");
#endif
		}else{
			ts->x = x;
			ts->y = y;
#ifdef DEBUG
			printk("move\n");
#endif
		
		}
		input_report_abs(ip, ABS_X, x);
		input_report_abs(ip, ABS_Y, y);	  
		input_report_abs(ip, ABS_PRESSURE, touch_pressure /*7500 */ );
		input_report_key(ip, BTN_TOUCH, 1);
		input_sync(ip);
		mod_timer(&ts->penup_timer, jiffies + msecs_to_jiffies(PENUP_TIMEOUT));
	}


	spin_unlock_irq(&ts->lock);
	mod_timer(&ts->defer_irq_timer, jiffies + msecs_to_jiffies(INTERRUPT_DELAY));

}				/* end of tsc2005_complete */

/* 
 * The Timer function -
 * 	Submit previously setup SPI transaction here 
 * @params:
 *	input:
 *		data: tsc2005_data structure
 *	output:
 */
static void tsc2005_defer_irq_timer_handler(unsigned long handle)
{
	struct tsc2005 *ts = (void *)handle;

#ifdef DEBUG
	printk("delay irq\n");
#endif
	spin_lock_irq(&ts->lock);
	enable_irq(ts->penirq);
	spin_unlock_irq(&ts->lock);
	return;
}


static void tsc2005_penup_timer_handler(unsigned long handle)
{
	struct tsc2005 *ts = (void *)handle;
	struct input_dev *ip = ts->input_dev;

#ifdef DEBUG
        printk("penup\n");
#endif
	spin_lock_irq(&ts->lock);
	input_report_abs(ip, ABS_X, 0);
	input_report_abs(ip, ABS_Y, 0);	  
	input_report_abs(ip, ABS_PRESSURE, 0);
	input_report_key(ip, BTN_TOUCH, 0);
	input_sync(ip);
	spin_unlock_irq(&ts->lock);



	return;
}



/* 
 * IRQ Handler -
 * 	Disable IRQ & detect cause of IRQ (pendown or penup),
 * 	either start the timer or report penup & enable_irq
 * @params:
 *	input:
 *		IRQ - which caused the interrupt, 
 *		data: tsc2005_data structure
 *	output:
 *		IRQ_HANDLED
 */
static irqreturn_t tsc2005_irq(int irq, void *handle)
{
	struct tsc2005 *tsc = handle;
	unsigned long flags = 0;

#ifdef DEBUG
	printk("tsc2005_irq\n");
#endif
	spin_lock_irqsave(&tsc->lock, flags);	

	disable_irq_nosync(irq);

	spi_async(tsc->spi_dev, &tsc->read_msg);

	mod_timer(&tsc->penup_timer, jiffies + msecs_to_jiffies(PENUP_TIMEOUT));
	spin_unlock_irqrestore(&tsc->lock, flags);
	return IRQ_HANDLED;
}




static int tsc2005_check_reg(struct tsc2005 *ts)
{
	unsigned char wr_cmd[3];
	unsigned short cfr0_val;
	unsigned short cfr1_val;
	unsigned short cfr2_val;
	struct spi_message *m;
	struct spi_transfer *x;
	struct spi_message m_temp;
	struct spi_transfer x_temp[2];
	int status;

	m = &m_temp;
	x = &x_temp[0];
       
	wr_cmd[0] = (TSC2005_REG | TSC2005_REG_CFR0 | TSC2005_REG_READ),
	spi_message_init(m);

	memset(x, 0, sizeof(*x));
	x->tx_buf = &wr_cmd[0];
	x->len = 1;
	x->bits_per_word = 8;
	spi_message_add_tail(x, m);

	x++;
	memset(x, 0, sizeof(*x));
	x->rx_buf = &cfr0_val;
	x->len = 2;
//	x->bits_per_word = 16;
	x->bits_per_word = 8;
	spi_message_add_tail(x, m);

	status = spi_sync(ts->spi_dev,m);
	if (status != 0) {
		return -1;
	}

	if((be16_to_cpu(cfr0_val) &0x3fff) != (TSC2005_CFR0_INITVALUE & 0x3fff))
	{
		printk("cfr0: write = %x, read = %x\n",TSC2005_CFR0_INITVALUE,be16_to_cpu(cfr0_val));
		return -1;
	}

	//read cfr1
	x = &x_temp[0];
	wr_cmd[0] = (TSC2005_REG | TSC2005_REG_CFR1 | TSC2005_REG_READ),
	spi_message_init(m);
	memset(x, 0, sizeof(*x));
	x->tx_buf = &wr_cmd[0];
	x->len = 1;
	x->bits_per_word = 8;
	spi_message_add_tail(x, m);

	x++;
	memset(x, 0, sizeof(*x));
	x->rx_buf = &cfr1_val;
	x->len = 2;
//	x->bits_per_word = 16;
	x->bits_per_word = 8;
	spi_message_add_tail(x, m);

	status = spi_sync(ts->spi_dev,m);
	if (status != 0) {
		return -1;
	}

	if(be16_to_cpu(cfr1_val) != TSC2005_CFR1_INITVALUE)
	{
		printk("cfr1 : write = %x, read = %x\n",TSC2005_CFR1_INITVALUE,be16_to_cpu(cfr1_val));
		return -1;
	}
	//read cfr2
	x = &x_temp[0];
	wr_cmd[0] = (TSC2005_REG | TSC2005_REG_CFR2 | TSC2005_REG_READ),
	spi_message_init(m);
	memset(x, 0, sizeof(*x));
	x->tx_buf = &wr_cmd[0];
	x->len = 1;
	x->bits_per_word = 8;
	spi_message_add_tail(x, m);

	x++;
	memset(x, 0, sizeof(*x));
	x->rx_buf = &cfr2_val;
	x->len = 2;
//	x->bits_per_word = 16;
	x->bits_per_word = 8;
	spi_message_add_tail(x, m);

	status = spi_sync(ts->spi_dev,m);

	if (status != 0) {
		return -1;
	}

	if(be16_to_cpu(cfr2_val) != TSC2005_CFR2_INITVALUE)
	{
		printk("cfr2 : write = %x, read = %x\n",TSC2005_CFR2_INITVALUE,be16_to_cpu(cfr2_val));
		return -1;
	}


	return 0;

}

static void tsc2005_ts_setup_spi_xfer(struct tsc2005 *ts)
{
	struct spi_message *m;
	struct spi_transfer *x;
	int i;

	m = &ts->read_msg;
	spi_message_init(m);

	for (i = 0; i < NUM_READ_REGS; i++) {

		x = &ts->read_xfer[i][0];
		memset(x,0,sizeof(*x));
		x->tx_buf = &tsc2005_read_reg[i];
		x->len = 1;
		x->bits_per_word = 8;
		spi_message_add_tail(x,m);

		x = &ts->read_xfer[i][1];
		memset(x,0,sizeof(*x));
		
		x->rx_buf = &ts->data[i][0];
		x->len = 2;
		x->bits_per_word = 16;
		x->cs_change = i < (NUM_READ_REGS -1 );

		spi_message_add_tail(x, m);

	}

	m->complete = tsc2005_complete;
	m->context = ts;
}

/*
 *	SPI sync() based calls cannot be executed in Interrupt contexts
 * 	hence the reading of X, Y, Z1 & Z2 co-ordinates require a list
 * 	of SPI transfers setup
 * 	If TSC_MODE_1 is selected, the select_scan message should also
 * 	be a part of that list.
 * 	Delays in function wont affect the performance since the function
 * 	is called only at the initialisation of driver.
 * @params:
 *	input:
 *		data: tsc2005_data structure
 *	output:
 *		function return status
 */
static int tsc2005_spi_setup(struct tsc2005 *ts)
{
	int status;
	unsigned char wr_cmd[3];
	unsigned short cfr0_val;

	struct spi_message *m;
	struct spi_transfer *x;

	struct spi_message m_temp;
	struct spi_transfer x_temp[2];
	int i;

        #ifdef DEBUG
	printk("[%s]\n",__func__);
	#endif

	cfr0_val = 0;
	status = 0;
	wr_cmd[0] = wr_cmd[1] = wr_cmd[2] = 0;


	for(i=0;i<3;i++)
	{
		m = &m_temp;
		x = &x_temp[0];
		wr_cmd[0] = tsc2005_cfr_tbl[i][0];
		wr_cmd[1] = tsc2005_cfr_tbl[i][1];
		wr_cmd[2] = tsc2005_cfr_tbl[i][2];

		spi_message_init(m);
		memset(x, 0, sizeof(*x));
		x->tx_buf = &wr_cmd[0];
		x->len = 1;
		x->bits_per_word = 8;
		spi_message_add_tail(x, m);

		x++;
		memset(x, 0, sizeof(*x));
		x->tx_buf = &wr_cmd[1];
		x->len = 2;
//		x->bits_per_word = 16;
		x->bits_per_word = 8;
		spi_message_add_tail(x, m);

#ifdef DEBUG
		printk("wr = %x\n", (wr_cmd[1] << 8) + wr_cmd[2]);
#endif
		status = spi_sync(ts->spi_dev,m);
		if (status != 0) {
			goto err_status;
		}
	}
	m = &m_temp;
	x = &x_temp[0];
	wr_cmd[0] = TSC2005_CMD | TSC2005_CMD_12BIT | TSC2005_CMD_SCAN_XYZZ;
	x->tx_buf = &wr_cmd[0];
	x->rx_buf = NULL;
	x->len = 1;
	x->bits_per_word = 8;
	spi_message_init(m);
	spi_message_add_tail(x,m);
	spi_sync(ts->spi_dev,m);

	if(tsc2005_check_reg(ts) != 0)
		goto err_status;

	tsc2005_ts_setup_spi_xfer(ts);


	return 0;
      err_status:
	return -1;
}




/*	
 * 	Start of functions which were registered as a
 *	part of tsc2005 spi_driver.
 * 
 *  Should be Called, if the device has been added to list of
 *  SPI devices in board_init. [Even without an actual Slave device]
 * @params:
 *	input:
 *		SPI device structure allocated for the device
 *	output:
 *		function return status
 */
static int __devinit tsc2005_probe(struct spi_device *spi)
{
	struct tsc2005 *ts;
	struct input_dev *input_dev;
	int err;
	struct tsc2005_platform_data *pdata = spi->dev.platform_data;

	/*
	 * Check for Max_speed of SPI bus 
	 */
        #ifdef DEBUG
	printk("[%s]\n",__func__);
	#endif

	if (!pdata) {
		dev_dbg(&spi->dev, "no platform data?\n");
		return -ENODEV;
	}

	if (spi->max_speed_hz > TSC2005_HZ) {
		dev_dbg(&spi->dev, "f(sample) %d KHz?\n",
			(spi->max_speed_hz) / 1000);
		return -EINVAL;
	}


	/* 
	 * Set up the SPI Interface 
	 */
	spi->bits_per_word = 8;
	/*
	 * Refer to TSC Slave device CPOL and CPHA
	 */
	spi->mode = SPI_MODE_0;
	err = spi_setup(spi);
	if (err < 0) {
		return err;
	}

	ts = kzalloc(sizeof(struct tsc2005), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!ts || !input_dev) {
		err = -ENOMEM;
		goto err_free_mem;
	}

	spin_lock_init(&ts->lock);
	ts->x_plate_ohms = pdata->x_plate_ohms ? : 280;

	dev_set_drvdata(&spi->dev, ts);
	spi->dev.power.power_state = PMSG_ON;

	ts->spi_dev = spi;
	ts->input_dev = input_dev;

	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input_dev->absbit[0] = BIT_MASK(ABS_X) | BIT_MASK(ABS_Y) | BIT_MASK(ABS_PRESSURE);
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);


	input_set_abs_params(input_dev, ABS_X, 0, 4096, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, 4096, 0, 0);

	/*
	 * Should Ideally be recieved from platform_data
	 * initialised in board_init.
	 * Have same values of ads7846 since the platform is same
	 */
	input_set_abs_params(input_dev, ABS_PRESSURE, 100, 15000, 0, 0);

	snprintf(ts->phys, sizeof(ts->phys),
		 "%s/input0", dev_name(&spi->dev));

	input_dev->name = "TI TSC2005 TouchScreen Controller";
	input_dev->phys = ts->phys;
	input_dev->dev.parent = &spi->dev;
	err = input_register_device(input_dev);
	if (err) {
		goto err_free_mem;
	}
	/*
	 * Initialise TIMER
	 */
	init_timer(&ts->defer_irq_timer);
	ts->defer_irq_timer.data = (unsigned long)ts;
	ts->defer_irq_timer.function = tsc2005_defer_irq_timer_handler;


	init_timer(&ts->penup_timer);
	ts->penup_timer.data = (unsigned long)ts;
	ts->penup_timer.function = tsc2005_penup_timer_handler;

	ts->penirq = ts->spi_dev->irq;


/*
 * Setup SPI transfers here
 */
	if (tsc2005_spi_setup(ts) != 0) {
		/* 
		 * Perform h/w reset of TSC
		 */
		printk("tsc2005_spi_setup failed\n");
		goto err_dev_unregister;
		
	}
/*
 * Request IRQ here, 
 * ts->penirq should hold the IRQ upon which the TSC shall
 * Interrupt
 */



	if (request_irq(ts->penirq, tsc2005_irq, 0,
			spi->dev.driver->name, ts)) {
		dev_dbg(&spi->dev, "irq %d busy?\n", spi->irq);
		err = -EBUSY;
		goto err_free_irq;
	}

	printk("tsc2005 touch screen driver load \n");
	return 0;

err_free_irq:
	free_irq(ts->penirq, ts);
	
err_dev_unregister:
	input_unregister_device(ts->input_dev);

err_free_mem:
	input_free_device(input_dev);
	kfree(ts);
	return err;

}

/*	
 *	Remove spi device from the system 
 *	Make sure all interfaces/data structures initialised in _probe
 *	are closed / terminated / freed here
 * @params:
 *	input:
 *		SPI device structure allocated for the device
 *	output:
 *		function return status
 */

static int __devexit tsc2005_remove(struct spi_device *spi)
{
	struct tsc2005 *ts = dev_get_drvdata(&spi->dev);
/*
 * Undo things done in _probe
 */
        #ifdef DEBUG
	printk("tsc2005 remove\n");
	#endif

	input_unregister_device(ts->input_dev);
	free_irq(ts->penirq, ts);
	input_free_device(ts->input_dev);
	del_timer_sync(&ts->defer_irq_timer);
	del_timer_sync(&ts->penup_timer);
	kfree(ts);

	return 0;
}


#ifdef CONFIG_PM
static int tsc2005_suspend(struct spi_device *spi, pm_message_t mesg)
{
	struct tsc2005 *ts = dev_get_drvdata(&spi->dev);
	spin_lock_irq(&ts->lock);
	ts->disabled = 1;
	disable_irq(ts->penirq);
	spin_unlock_irq(&ts->lock);
	return 0;
}

static int tsc2005_resume(struct spi_device *spi)
{
	struct tsc2005 *ts = dev_get_drvdata(&spi->dev);
	spin_lock_irq(&ts->lock);
	ts->disabled = 0;
	enable_irq(ts->penirq);
	spin_unlock_irq(&ts->lock);

	if (tsc2005_spi_setup(ts) != 0) {
		/* 
		 * Perform h/w reset of TSC
		 */
		printk("tsc2005 resume failed\n");
		return -1;
		
	}
	return 0;
}

#endif
static struct spi_driver tsc2005_driver = {

	.driver = {
/*
 * Name: As in board_init function for probe
 */
		   .name = "tsc2005",
		   .bus = &spi_bus_type,
		   .owner = THIS_MODULE,
		   },
#ifdef CONFIG_PM
	.suspend = tsc2005_suspend,
	.resume = tsc2005_resume,
#endif
	.probe = tsc2005_probe,
	.remove = __devexit_p(tsc2005_remove),
};

/*
 *	Init and the _exit of this driver module
 * 	Register / Un-Register Driver as SPI Protocol Driver 
 */

static int __init tsc2005_init(void)
{
        #ifdef DEBUG
	printk("tsc2005_init\n");
	#endif
	return spi_register_driver(&tsc2005_driver);
}

module_init(tsc2005_init);

static void __exit tsc2005_exit(void)
{
	spi_unregister_driver(&tsc2005_driver);
}

module_exit(tsc2005_exit);

MODULE_DESCRIPTION("TSC2005 TouchScreen Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pavan_Savoy@mindtree.com");
MODULE_AUTHOR("eku@marvell.com");
