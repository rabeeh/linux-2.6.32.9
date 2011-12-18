/*
 * Driver for Realtek Touch Screen Controller ALC5611
 * Author	: Brian Hsu bhsu@marvell.com)
 * Date		: 20-10-2009
 */

/* #define DEBUG */

#include <linux/rt5611_ts.h>
/*
#define RT5611_TS_DEBUG(format, args...) \
	printk(KERN_DEBUG "%s(%d): "format"\n", __FUNCTION__, __LINE__, ##args)
*/

#define RT5611_TS_DEBUG(format, args...)

/* #define SWAP_X */
#ifdef CONFIG_ANDROID
#define SWAP_Y
#else
/* #define SWAP_Y */
#endif

#define ABS(X) ((X) < 0 ? (-1 * (X)) : (X))
static const u16 rt5611_reg_defalt_00h = 0x59b4;

static int abs_x[3] = {0, 4095, 0};
static int abs_y[3] = {0, 4095, 0};
static int abs_p[3] = {0,  150, 0};

/*
 * ADC sample delay times in uS
 */
static const int delay_table[] = {
	84,    /*	0x00b=>4 AC97 frames(20.8us) */
	167,  /*	0x01b=>8 */
	333,  /*	0x10b=>16 */
	667,  /*	0x11b=>32 */
};

static u16 delay_sel = (CB1_DEL_16F >> 7); /* default delay selection */

static inline void poll_delay(int d)
{

	d = (((d <= 3) && (d >= 0)) ? d : ((d > 3) ? 3 : 0));
	udelay(delay_table[d] + 3 * AC97_LINK_FRAME);
}

static bool rt5611_ts_resumed=0;
extern bool rt5610_codec_resumed;

int rt5611_ts_reg_read(struct rt5611_ts *rt, u16 reg)
{
	int val = 0;
	val = soc_ac97_ops.read(rt->codec->ac97, reg);

	RT5611_TS_DEBUG("rt5611_ts_reg_read: (reg,val)=(%x,%x)\n", reg, val);

	return val;
}
void rt5611_ts_reg_write(struct rt5611_ts *rt, u16 reg, u16 val)
{
	RT5611_TS_DEBUG("rt5611_ts_reg_write: (reg,val)=(%x,%x)\n", reg, val);

	soc_ac97_ops.write(rt->codec->ac97, reg, val);
	return;
}

bool rt5611_ts_reg_write_mask(struct rt5611_ts *rt, u16 reg, u16 val, u16 mask)
{
	bool RetVal = 0;
	u16 data = 0;
	if (!mask)
		return RetVal;

	/*RT5611_TS_DEBUG("rt5611_ts_reg_write_mask: (reg,val,mask)=(%x,%x,%x)\n",reg,val,mask);*/

	if (mask != 0xFFFF) {/* portion mask */
		data = soc_ac97_ops.read(rt->codec->ac97, reg);
		data &= ~mask;
		data |= val & mask;
		soc_ac97_ops.write(rt->codec->ac97, reg, data);
	} else  /* All mask */
		soc_ac97_ops.write(rt->codec->ac97, reg, data);

	return RetVal;
}

static void rt5611_ts_phy_init(struct rt5611_ts *rt)
{
	RT5611_TS_DEBUG("rt5611_ts_phy_init\n");

	rt5611_ts_reg_write(rt, RT_TP_CTRL_BYTE1, CB1_DEFALT);
	rt5611_ts_reg_write(rt, RT_TP_CTRL_BYTE2, CB2_DEFALT);
	rt5611_ts_reg_write_mask(rt, RT_GPIO_PIN_STICKY, 0, RT_GPIO_BIT13);
	rt5611_ts_reg_write_mask(rt, RT_PD_CTRL_STAT, 0,PD_CTRL_VREF|PD_CTRL_ADC); //enable ADC/Vref power down 

}

static int rt5611_ts_poll_coord(struct rt5611_ts *rt, struct rt5611_ts_data *data)
{
	bool FoundXSample = 0, FoundYSample = 0, FoundPSample = 0;
	u16 val = 0;
	u8 i = 0;

	RT5611_TS_DEBUG("rt5611_ts_poll_coord\n");

	data->x = MAX_ADC_VAL;
	data->y = MAX_ADC_VAL;
	data->p = MAX_ADC_VAL;

	poll_delay(delay_sel);
	for (i = 0; i < MAX_CONVERSIONS; i++) {
		val = rt5611_ts_reg_read(rt, RT_TP_INDICATION);
		if (!(val & CB3_PD_STATUS))
			return RC_PENUP;

		if (!FoundXSample && ((val&CB3_ADCSRC_MASK) == CB3_ADCSRC_X)) {
			data->x = val & CB3_ADC_DATA; /* get X sample */
			FoundXSample = 1;
		} else if (!FoundYSample && ((val&CB3_ADCSRC_MASK) == CB3_ADCSRC_Y)) {
			data->y = val & CB3_ADC_DATA; /* get Y sample */
			FoundYSample = 1;
		} else if (!FoundPSample && ((val&CB3_ADCSRC_MASK) == CB3_ADCSRC_PRE)) {
			data->p = val & CB3_ADC_DATA;  /* get Presure sample */
			FoundPSample = 1;
		} else if ((val&CB3_ADCSRC_MASK) == CB3_ADCSRC_NON) {
			//when busy, using a smaller delay
			udelay(100);
			continue;
		}
			

		if (FoundXSample && FoundYSample && FoundPSample)
			break;
		/*wait some delay- delay for conversion */
		poll_delay(delay_sel);
	}
	if (i == MAX_CONVERSIONS)
		return RC_AGAIN;
	else
		return RC_VALID;
}

#ifdef POLLING_MODE
static int rt5611_ts_poll_sample(struct rt5611_ts *rt, int adcsel, int *sample)
{
	u16 val = 0;
	unsigned short adccr = 0;

	*sample = MAX_ADC_VAL;
	RT5611_TS_DEBUG("rt5611_ts_poll_sample(POLLING_MODE)\n");

	/* enable ADC target */
	switch (adcsel) {
	case RT_ADC_X:
		adccr = CB2_X_EN;
		break;
	case RT_ADC_Y:
		adccr = CB2_Y_EN;
		break;
	case RT_ADC_P:
		adccr = CB2_PRESSURE_EN;
		break;
	default:
		return -EINVAL;		
	}

	/* set internal Pull-up resistor and Polling mode */
   	adccr |= CB2_PPR_64 | CB2_POLL_TRIG;

  	rt5611_ts_reg_write(rt, RT_TP_CTRL_BYTE2, adccr);
	do{
		val = rt5611_ts_reg_read(rt, RT_TP_CTRL_BYTE2);		
	}while (val & CB2_POLL_TRIG);

	val = rt5611_ts_reg_read(rt, RT_TP_INDICATION);
	if (val & CB3_PD_STATUS)
	{
		*sample = val & CB3_ADC_DATA;
		return RC_VALID;
	} else
		return RC_PENUP;
}
#endif

static int rt5611_ts_poll_touch(struct rt5611_ts *rt, struct rt5611_ts_data *data)
{
	int rc = 0;
	RT5611_TS_DEBUG("rt5611_ts_poll_touch\n");

#ifdef POLLING_MODE /* poll mode */

		rc = rt5611_ts_poll_sample(rt, RT_ADC_X, &data->x);
		if (rc != RC_VALID)
			return rc;
		rc = rt5611_ts_poll_sample(rt, RT_ADC_Y, &data->y);
		if (rc != RC_VALID)
			return rc;
		rc = rt5611_ts_poll_sample(rt, RT_ADC_P,&data->p);
		if (rc != RC_VALID)
			return rc;

#else	/* continuous mode */
		rc = rt5611_ts_poll_coord(rt, data);
		return rc;
#endif
}

static void rt5611_ts_enable(struct rt5611_ts *rt, int enable)
{
	RT5611_TS_DEBUG("rt5611_ts_enable(%d)\n", enable);
	u16 pdstatus=0;
	if (enable) {
		pdstatus=rt5611_ts_reg_read(rt, RT_PD_CTRL_STAT);
		if (!(pdstatus & (PD_CTRL_STATUS_VREF|PD_CTRL_STATUS_ADC))) // 1:ready, 0:not ready
			rt5611_ts_reg_write_mask(rt, RT_PD_CTRL_STAT, 0,
			PD_CTRL_VREF|PD_CTRL_ADC); //enable ADC/Vref power down 
	
		/* power on main BIAS */
		rt5611_ts_reg_write_mask(rt, RT_PWR_MANAG_ADD1, 
					 PWR_MAIN_BIAS | PWR_IP_ENA,
					 PWR_MAIN_BIAS | PWR_IP_ENA);
		/* enable touch panel and ADC and VREF of all analog */
		rt5611_ts_reg_write_mask(rt, RT_PWR_MANAG_ADD2, 
					 PWR_MIXER_VREF | PWR_TP_ADC,
					 PWR_MIXER_VREF |PWR_TP_ADC);
#ifdef POLLING_MODE	/* poll mode */
		/* set touch control register1 */
		rt5611_ts_reg_write(rt, RT_TP_CTRL_BYTE1, 
				    POW_TP_CTRL_1 | CB1_CR2 | CB1_CLK_DIV64 | CB1_DEL_8F);
	
#else	/* continuous mode */
		/* set touch control register1 */
		rt5611_ts_reg_write(rt, RT_TP_CTRL_BYTE1, POW_TP_CTRL_1 | CB1_CR1 |
				    CB1_CLK_DIV64 | CB1_DEL_16F |
				    CB1_SLOT_READBACK|CB1_PRES_CURR_375UA);
		/* set touch control register2 */
		rt5611_ts_reg_write(rt, RT_TP_CTRL_BYTE2, CB2_MODE_SEL | CB2_X_EN |
				    CB2_Y_EN | CB2_PRESSURE_EN|CB2_PPR_64);		
#endif
		/* set GPIO 2 to output pin */
		rt5611_ts_reg_write_mask(rt, RT_GPIO_PIN_CONFIG, RT_GPIO_BIT13, RT_GPIO_BIT2 | RT_GPIO_BIT13);

		/* set GPIO 2 to IRQ function */
		rt5611_ts_reg_write_mask(rt, RT_GPIO_PIN_SHARING, GPIO2_PIN_SHARING_IRQ, GPIO2_PIN_SHARING_MASK);	

		/* set pen down to sticky mode */
		rt5611_ts_reg_write_mask(rt ,RT_GPIO_PIN_STICKY, RT_GPIO_BIT13, RT_GPIO_BIT13);	
		
		/* set Pen down detect have Wake-up function */
		rt5611_ts_reg_write_mask(rt, RT_GPIO_PIN_WAKEUP, RT_GPIO_BIT13, RT_GPIO_BIT13);	

		/* Enable GPIO wakeup control */
		rt5611_ts_reg_write_mask(rt, RT_MISC_CTRL, GPIO_WAKEUP_CTRL, GPIO_WAKEUP_CTRL);	

		udelay(300);
	}else{
		rt5611_ts_reg_write_mask(rt, RT_PWR_MANAG_ADD1, 0, PWR_IP_ENA);
		rt5611_ts_reg_write_mask(rt, RT_PWR_MANAG_ADD2, 0, PWR_TP_ADC);
	}
}

/*
 * The touchscreen sample reader.
 */
static int rt5611_ts_read_samples(struct rt5611_ts *rt)
{
	struct rt5611_ts_data data;
	int rc = 0;

	RT5611_TS_DEBUG("rt5611_ts_read_samples\n");

	mutex_lock(&rt->codec->mutex);

       rt5611_ts_enable(rt, RT_TP_ENABLE);

	rc = rt5611_ts_poll_touch(rt, &data);
	if (rc & RC_PENUP) {
		if (rt->pen_is_down) {
			rt->pen_is_down = 0;
			RT5611_TS_DEBUG("rt5611_ts: pen up\n");
			input_report_abs(rt->input_dev, ABS_PRESSURE, 0);
			input_report_key(rt->input_dev, BTN_TOUCH, 0);
			input_sync(rt->input_dev);
		} else { /*rt->pen_is_down=0 */
			/* We need high frequency updates only while
			 * pen is down, the user never will be able to
			 * touch screen faster than a few times per
			 * second... On the other hand, when the user
			 * is actively working with the touchscreen we
			 * don't want to lose the quick response. So we
			 * will slowly increase sleep time after the
			 * pen is up and quicky restore it to ~one task
			 * switch when pen is down again.
			 */
			RT5611_TS_DEBUG("rt5611_ts: rt->pen_is_down=0\n");
			if (rt->ts_reader_interval < HZ / 10)
				rt->ts_reader_interval++;		
		}

	} else if (rc & RC_VALID) {
		
#ifdef SWAP_X
		data.x = 4095 - data.x;
#endif
#ifdef SWAP_Y
		data.y = 4095 - data.y;
#endif
		
		input_report_abs(rt->input_dev, ABS_X, data.x);
		input_report_abs(rt->input_dev, ABS_Y, data.y);
		input_report_abs(rt->input_dev, ABS_PRESSURE, data.p);
		input_report_key(rt->input_dev, BTN_TOUCH, 1);
		input_sync(rt->input_dev);
		RT5611_TS_DEBUG("pen down: x=%d, y=%d, p=%d\n",
				data.x, data.y, data.p);

		rt->pen_is_down = 1;
		rt->ts_reader_interval = rt->ts_reader_min_interval;
	}

	rt5611_ts_enable(rt, RT_TP_DISABLE);

	mutex_unlock(&rt->codec->mutex);
	return rc;
}

static void rt5611_ts_reader(struct work_struct *work)
{
	int rc = 0;
	struct rt5611_ts *rt = container_of(work, struct rt5611_ts, ts_reader.work);

#ifdef CONFIG_PM
	/* Wait for codec resume done. To avoid system hang due to AC97 not wakeup (clock enable) and reset*/
	if(rt5611_ts_resumed && !rt5610_codec_resumed)
	{
	       queue_delayed_work(rt->ts_workq, &rt->ts_reader,  100);
		   return ;
	}
#endif
	RT5611_TS_DEBUG("rt5611_ts_reader\n");

	do {
		rc = rt5611_ts_read_samples(rt);
	} while (rc & RC_AGAIN);

	/* Enable timer polling:*/
	if (rt->pen_is_down || !rt->pen_irq)
		queue_delayed_work(rt->ts_workq, &rt->ts_reader,
				   rt->ts_reader_interval);

	/* Pen Up, enable IRQ if IRQ has been assigned */
	if (!rt->pen_is_down && rt->pen_irq) {
		rt5611_ts_reg_write_mask(rt, RT_GPIO_PIN_STATUS, 0,
					 RT_GPIO_BIT2 | RT_GPIO_BIT13);
		enable_irq(rt->pen_irq);
	}
}

static irqreturn_t  rt5611_ts_pen_interrupt(int irq, void *dev_id)
{
	struct  rt5611_ts *rt = dev_id;
	RT5611_TS_DEBUG("rt5611_ts_pen_interrupt\n");

	if (!work_pending(&rt->pen_event_work)) {
		disable_irq_nosync(irq);
		queue_work(rt->ts_workq, &rt->pen_event_work);
	}
	return IRQ_HANDLED;
}
/*
 * Handle a pen down interrupt.
 */
static void rt5611_ts_pen_irq_worker(struct work_struct *work)
{
	struct rt5611_ts *rt = container_of(work, struct rt5611_ts, pen_event_work);
	int pen_was_down = rt->pen_is_down;
	u16 status, pol;

	RT5611_TS_DEBUG("rt5611_ts_pen_irq_worker\n");

	mutex_lock(&rt->codec->mutex);
	status = rt5611_ts_reg_read(rt, RT_GPIO_PIN_STATUS);
	pol = rt5611_ts_reg_read(rt, RT_GPIO_PIN_POLARITY);

	if (RT_GPIO_BIT13 & pol & status)
		rt->pen_is_down = 1;
	else
		rt->pen_is_down = 0;

	mutex_unlock(&rt->codec->mutex);

	/* Data is not availiable immediately on pen down */
	queue_delayed_work(rt->ts_workq, &rt->ts_reader, 1);

	/* Let ts_reader report the pen up for debounce. */
	if (!rt->pen_is_down && pen_was_down)
		rt->pen_is_down = 1;
}



/*
 * initialise pen IRQ handler and workqueue
 */
static int  rt5611_ts_init_pen_irq(struct  rt5611_ts *rt)
{

	RT5611_TS_DEBUG("rt5611_ts_init_pen_irq\n");
	/* If an interrupt is supplied an IRQ enable operation must also be
	 * provided. */

	if (request_irq(rt->pen_irq, rt5611_ts_pen_interrupt, IRQF_SHARED,
			"rt5611_ts-pen", rt)) {
		dev_err(rt->dev,
			"Failed to register pen down interrupt, polling");
		rt->pen_irq = 0;
		return -EINVAL;
	}


	return 0;
}

static int rt5611_ts_input_open(struct input_dev *idev)
{
	struct rt5611_ts *rt = input_get_drvdata(idev);

	RT5611_TS_DEBUG("rt5611_ts_input_open\n");
	rt->ts_workq = create_singlethread_workqueue("rt5611_ts");
	if (rt->ts_workq == NULL) {
		dev_err(rt->dev,
			"Failed to create workqueue\n");
		return -EINVAL;
	}

	rt5611_ts_enable(rt, RT_TP_ENABLE);

	INIT_DELAYED_WORK(&rt->ts_reader, rt5611_ts_reader);
	INIT_WORK(&rt->pen_event_work, rt5611_ts_pen_irq_worker);

	rt->ts_reader_min_interval = (HZ >= 100 ? HZ / 100 : 1);
	rt->ts_reader_interval = rt->ts_reader_min_interval;

	rt->pen_is_down = 0;
	if (rt->pen_irq)
		rt5611_ts_init_pen_irq(rt);
	else
		dev_err(rt->dev, "No IRQ specified\n");

	/* If we either don't have an interrupt for pen down events or
	 * failed to acquire it then we need to poll.
	 */
	if (rt->pen_irq == 0)
		queue_delayed_work(rt->ts_workq, &rt->ts_reader,
				   rt->ts_reader_interval);

	return 0;
}

static void rt5611_ts_input_close(struct input_dev *idev)
{
	struct rt5611_ts *rt = input_get_drvdata(idev);
	RT5611_TS_DEBUG("rt5611_ts_input_close\n");

	if (rt->pen_irq) {
		free_irq(rt->pen_irq, rt);
	}

	rt->pen_is_down = 0;


	/* ts_reader rearms itself so we need to explicitly stop it
	 * before we destroy the workqueue.
	 */
	cancel_delayed_work_sync(&rt->ts_reader);

	destroy_workqueue(rt->ts_workq);


}


static int rt5611_ts_reset(struct snd_soc_codec *codec, int try_warm)
{
	RT5611_TS_DEBUG("rt5611_ts_reset(try warm=%d)\n", try_warm);

	if (try_warm && soc_ac97_ops.warm_reset) {
		soc_ac97_ops.warm_reset(codec->ac97);
		if (soc_ac97_ops.read(codec->ac97, 0) == rt5611_reg_defalt_00h)
			return 1;
	}

	soc_ac97_ops.reset(codec->ac97);
	if (soc_ac97_ops.read(codec->ac97, 0) != rt5611_reg_defalt_00h)
		return -EIO;
        mdelay(10);
	return 0;
}

static int rt5611_ts_probe(struct platform_device *pdev)
{
	struct rt5611_ts *rt;
	int ret = 0;
	RT5611_TS_DEBUG("rt5611_ts_probe\n");


	rt = kzalloc(sizeof(struct rt5611_ts), GFP_KERNEL);
	if (!rt)
		return -ENOMEM;
	rt->codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (rt->codec == NULL)
		return -ENOMEM;

	mutex_init(&rt->codec->mutex);
	ret = snd_soc_new_ac97_codec(rt->codec, &soc_ac97_ops, 0);
	if (ret < 0)
		goto alloc_err;

	rt->dev = &pdev->dev;
	platform_set_drvdata(pdev, rt);

	rt->pen_irq = platform_get_irq(pdev, 0);
	RT5611_TS_DEBUG("rt5611_ts: Pendown IRQ=%x\n", rt->pen_irq);

	rt5611_ts_reset(rt->codec, 0); /* Cold RESET */
	ret = rt5611_ts_reset(rt->codec, 1); /* Warm RESET */
	if (ret < 0) {
		printk(KERN_ERR "FAIL to reset rt5611\n");
		goto reset_err;
	}

	rt->id = rt5611_ts_reg_read(rt, RT_VENDOR_ID2);
	RT5611_TS_DEBUG("rt5611_ts: Vendor id=%08X\n", rt->id);

	/* set up physical characteristics */
	rt5611_ts_phy_init(rt);
	/* load gpio cache */

	rt->input_dev = input_allocate_device();
	if (rt->input_dev == NULL) {
		ret = -ENOMEM;
		goto alloc_err;
	}

	/* set up touch configuration */
	rt->input_dev->name = "rt5611 touchscreen";
	rt->input_dev->phys = "rt5611";
	rt->input_dev->open = rt5611_ts_input_open;
	rt->input_dev->close = rt5611_ts_input_close;

	rt->input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	rt->input_dev->absbit[0] = BIT_MASK(ABS_X) | BIT_MASK(ABS_Y) | BIT_MASK(ABS_PRESSURE);
	rt->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	input_set_abs_params(rt->input_dev, ABS_X, abs_x[0], abs_x[1], abs_x[2], 0);
	input_set_abs_params(rt->input_dev, ABS_Y, abs_y[0], abs_y[1], abs_y[2], 0);
	input_set_abs_params(rt->input_dev, ABS_PRESSURE, abs_p[0], abs_p[1],
			     abs_p[2], 0);

	input_set_drvdata(rt->input_dev, rt);
	rt->input_dev->dev.parent = pdev->dev.parent;

	ret = input_register_device(rt->input_dev);
	if (ret < 0)
		goto dev_alloc_err;

	return ret;

reset_err:
	snd_soc_free_ac97_codec(rt->codec);
 dev_alloc_err:
	input_free_device(rt->input_dev);
 alloc_err:
	kfree(rt);

	return ret;
}


static int rt5611_ts_remove(struct platform_device *pdev)
{
	struct rt5611_ts *rt = dev_get_drvdata(&pdev->dev);
	RT5611_TS_DEBUG("rt5611_ts_remove\n");

	input_unregister_device(rt->input_dev);
	kfree(rt);

	return 0;
}

#ifdef CONFIG_PM
static int rt5611_ts_resume(struct platform_device *pdev)
{
	struct rt5611_ts *rt = platform_get_drvdata(pdev);
	RT5611_TS_DEBUG("rt5611_ts_resume\n");
        queue_delayed_work(rt->ts_workq, &rt->ts_reader,  100);
	rt5611_ts_resumed=1;
	return 0;
}

static int rt5611_ts_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct rt5611_ts *rt = platform_get_drvdata(pdev);

	RT5611_TS_DEBUG("rt5611_ts_suspend\n");
		cancel_delayed_work_sync(&rt->ts_reader);

	if (rt->pen_irq)
		disable_irq(rt->pen_irq);
	rt5611_ts_resumed=0;
	return 0;
}
#else
#define rt5611_ts_suspend		NULL
#define rt5611_ts_resume		NULL
#endif

static struct platform_driver rt5611_ts_driver = {
	.driver = {
		.name =		"rt5611_ts",
		.owner =		THIS_MODULE,
	},
	.probe =		rt5611_ts_probe,
	.remove =	rt5611_ts_remove,
	.suspend =	rt5611_ts_suspend,
	.resume =	rt5611_ts_resume,
};
static int __init rt5611_ts_init(void)
{
	return platform_driver_register(&rt5611_ts_driver);
}

static void __exit rt5611_ts_exit(void)
{
	platform_driver_unregister(&rt5611_ts_driver);
}

module_init(rt5611_ts_init);
module_exit(rt5611_ts_exit);

/* Module information */
MODULE_AUTHOR("bhsu");
MODULE_DESCRIPTION("Realtek ACL66511-Touch Screen Driver");
MODULE_LICENSE("GPL");
