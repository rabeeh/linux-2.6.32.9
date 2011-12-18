/*
 * lirc_dove.c
 *
 *
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/serial_reg.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>

#include <asm/system.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/fcntl.h>
#include <linux/spinlock.h>

#include "../kcompat.h"
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35)
#include <media/lirc.h>
#include <media/lirc_dev.h>
#else
#include "../lirc.h"
#include "../lirc_dev/lirc_dev.h"
#endif

#define LIRC_DRIVER_NAME "lirc_dove"

struct lirc_dove {
	int signal_pin;
	int signal_pin_change;
	u8 on;
	u8 off;
	long (*send_pulse)(unsigned long length);
	void (*send_space)(long length);
	int features;
	spinlock_t lock;
};

#define LIRC_HOMEBREW		0
#define LIRC_IRDEO		1
#define LIRC_IRDEO_REMOTE	2
#define LIRC_ANIMAX		3
#define LIRC_IGOR		4
#define LIRC_NSLU2		5

#define RS_ISR_PASS_LIMIT 256
static int sense = 1;//-1;  /* -1 = auto, 0 = active high, 1 = active low */
/*
 * A long pulse code from a remote might take up to 300 bytes.  The
 * daemon should read the bytes as soon as they are generated, so take
 * the number of keys you think you can push before the daemon runs
 * and multiply by 300.  The driver will warn you if you overrun this
 * buffer.  If you have a slow computer or non-busmastering IDE disks,
 * maybe you will need to increase this.
 */

/* This MUST be a power of two!  It has to be larger than 1 as well. */

#define RBUF_LEN 256

static struct timeval lasttv = {0, 0};

static struct lirc_buffer rbuf;

static unsigned int freq = 38000;
static unsigned int duty_cycle = 50;

/* Initialized in init_timing_params() */
static unsigned long period;
static unsigned long pulse_width;
static unsigned long space_width;

/* does anybody have information on other platforms ? */
/* 256 = 1<<8 */
#define LIRC_SERIAL_TRANSMITTER_LATENCY 256

static int init_timing_params(unsigned int new_duty_cycle,
		unsigned int new_freq)
{
/*
 * period, pulse/space width are kept with 8 binary places -
 * IE multiplied by 256.
 */
	if (256 * 1000000L / new_freq * new_duty_cycle / 100 <=
	    LIRC_SERIAL_TRANSMITTER_LATENCY)
		return -EINVAL;
	if (256 * 1000000L / new_freq * (100 - new_duty_cycle) / 100 <=
	    LIRC_SERIAL_TRANSMITTER_LATENCY)
		return -EINVAL;
	duty_cycle = new_duty_cycle;
	freq = new_freq;
	period = 256 * 1000000L / freq;
	pulse_width = period * duty_cycle / 100;
	space_width = period - pulse_width;
	printk("in init_timing_params, freq=%d pulse=%ld, "
		"space=%ld\n", freq, pulse_width, space_width);
	return 0;
}


static void rbwrite(int l)
{
	if (lirc_buffer_full(&rbuf)) {
		/* no new signals will be accepted */
		printk("Buffer overrun\n");
		return;
	}
	lirc_buffer_write(&rbuf, (void *)&l);
}

static void frbwrite(int l)
{
/* simple noise filter */
	static int pulse, space;
	static unsigned int ptr;
#if 1
	if (ptr > 0 && (l & PULSE_BIT)) {
		pulse += l & PULSE_MASK;
		if (pulse > 250) {
			rbwrite(space);
			rbwrite(pulse | PULSE_BIT);
			ptr = 0;
			pulse = 0;
		}
		return;
	}
	if (!(l & PULSE_BIT)) {
		if (ptr == 0) {
			if (l > 20000) {
				space = l;
				ptr++;
				return;
			}
		} else {
			if (l > 20000) {
				space += pulse;
				if (space > PULSE_MASK)
					space = PULSE_MASK;
				space += l;
				if (space > PULSE_MASK)
					space = PULSE_MASK;
				pulse = 0;
				return;
			}
			rbwrite(space);
			rbwrite(pulse | PULSE_BIT);
			ptr = 0;
			pulse = 0;
		}
	}
#endif
	rbwrite(l);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19)
static irqreturn_t irq_handler(int i, void *blah)
#else
static irqreturn_t irq_handler(int i, void *blah, struct pt_regs *regs)
#endif
{
	u32 reg;
	int negate = 0;
	struct timeval tv;
	int counter, dcd;
	long deltv;
	int data;
	static int last_dcd = -1;
	static int count = 0;
	count++;
	/* First thing first - flip the bit */
	reg = readl(GPIO_IN_POL(19));
	if (reg & (1<< 19)) {reg &= ~(1<<19);negate=1;}
	else reg |= (1<< 19);
	writel(reg, GPIO_IN_POL(19));
	/* Now read the actual data */
	counter = 0;
	do {
		counter++;
		if (counter > RS_ISR_PASS_LIMIT) {
			printk(KERN_WARNING LIRC_DRIVER_NAME ": AIEEEE: "
			       "We're caught!\n");
			break;
		}
		if (1 /*(status & hardware[type].signal_pin_change) && sense != -1*/) {
			/* get current time */
			do_gettimeofday(&tv);

			/* New mode, written by Trent Piepho
			   <xyzzy@u.washington.edu>. */

			/*
			 * The old format was not very portable.
			 * We now use an int to pass pulses
			 * and spaces to user space.
			 *
			 * If PULSE_BIT is set a pulse has been
			 * received, otherwise a space has been
			 * received.  The driver needs to know if your
			 * receiver is active high or active low, or
			 * the space/pulse sense could be
			 * inverted. The bits denoted by PULSE_MASK are
			 * the length in microseconds. Lengths greater
			 * than or equal to 16 seconds are clamped to
			 * PULSE_MASK.  All other bits are unused.
			 * This is a much simpler interface for user
			 * programs, as well as eliminating "out of
			 * phase" errors with space/pulse
			 * autodetection.
			 */

			/* calc time since last interrupt in microseconds */
			dcd = (negate /*reg & (1<< 19)*/) /*//(status & hardware[type].signal_pin)*/ ? 0 : 1;

			if (dcd == last_dcd) {
				printk(KERN_WARNING LIRC_DRIVER_NAME
				": ignoring spike: %d %d %lx %lx %lx %lx\n",
				dcd, sense,
				tv.tv_sec, lasttv.tv_sec,
				tv.tv_usec, lasttv.tv_usec);
				continue;
			}

			deltv = tv.tv_sec-lasttv.tv_sec;
			if (tv.tv_sec < lasttv.tv_sec ||
			    (tv.tv_sec == lasttv.tv_sec &&
			     tv.tv_usec < lasttv.tv_usec)) {
				printk(KERN_WARNING LIRC_DRIVER_NAME
				       ": AIEEEE: your clock just jumped "
				       "backwards\n");
				printk(KERN_WARNING LIRC_DRIVER_NAME
				       ": %d %d %lx %lx %lx %lx\n",
				       dcd, sense,
				       tv.tv_sec, lasttv.tv_sec,
				       tv.tv_usec, lasttv.tv_usec);
				data = PULSE_MASK;
			} else if (deltv > 15) {
				data = PULSE_MASK; /* really long time */
				if (!(dcd^sense)) {
					/* sanity check */
					printk ("ERROR\n");
					printk(KERN_WARNING LIRC_DRIVER_NAME
					       ": AIEEEE: "
					       "%d %d %lx %lx %lx %lx\n",
					       dcd, sense,
					       tv.tv_sec, lasttv.tv_sec,
					       tv.tv_usec, lasttv.tv_usec);
					/*
					 * detecting pulse while this
					 * MUST be a space!
					 */
					sense = sense ? 0 : 1;
				}
			} else
				data = (int) (deltv*1000000 +
					       tv.tv_usec -
					       lasttv.tv_usec);
			frbwrite(dcd^sense ? data : (data|PULSE_BIT));
#if 0 // Original code below - WTF ??!!
			lasttv = tv;
#else
			memcpy (&lasttv, &tv, sizeof(struct timeval));
#endif
			last_dcd = dcd;

			wake_up_interruptible(&rbuf.wait_poll);
		}
	} while (0);//!(sinp(UART_IIR) & UART_IIR_NO_INT)); /* still pending ? */
	return IRQ_HANDLED;
}


static int hardware_init_port(void)
{
	return 0;
}

static int init_port(void)
{

	/* Initialize pulse/space widths */
	init_timing_params(duty_cycle, freq);

	return 0;
}

static int set_use_inc(void *data)
{
	int result;

	/* initialize timestamp */
	do_gettimeofday(&lasttv);

	result = request_irq(gpio_to_irq(19), irq_handler,
			     IRQF_DISABLED,
			     LIRC_DRIVER_NAME, (void *)NULL);//&hardware);

	switch (result) {
	case -EBUSY:
		printk(KERN_ERR LIRC_DRIVER_NAME ": IRQ %d busy\n", gpio_to_irq(19));
		return -EBUSY;
	case -EINVAL:
		printk(KERN_ERR LIRC_DRIVER_NAME
		       ": Bad irq number or handler\n");
		return -EINVAL;
	default:
		break;
	};

	return 0;

}

static void set_use_dec(void *data)
{	
	free_irq(gpio_to_irq(19), NULL);
}

static ssize_t lirc_write(struct file *file, const char *buf,
			 size_t n, loff_t *ppos)
{
#if 0
int i, count;
	unsigned long flags;
	long delta = 0;
	int *wbuf;

	if (!(hardware[type].features & LIRC_CAN_SEND_PULSE))
		return -EBADF;

	count = n / sizeof(int);
	if (n % sizeof(int) || count % 2 == 0)
		return -EINVAL;
	wbuf = memdup_user(buf, n);
	if (IS_ERR(wbuf))
		return PTR_ERR(wbuf);
	spin_lock_irqsave(&hardware[type].lock, flags);
	if (type == LIRC_IRDEO) {
		/* DTR, RTS down */
		on();
	}
	for (i = 0; i < count; i++) {
		if (i%2)
			hardware[type].send_space(wbuf[i] - delta);
		else
			delta = hardware[type].send_pulse(wbuf[i]);
	}
	off();
	spin_unlock_irqrestore(&hardware[type].lock, flags);
	kfree(wbuf);
#endif
	return n;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35)
static int lirc_ioctl(struct inode *node, struct file *filep, unsigned int cmd,
		      unsigned long arg)
#else
static long lirc_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
#endif
{
	int result;
	__u32 value;
	switch (cmd) {
	case LIRC_GET_SEND_MODE:
//		if (!(hardware[type].features&LIRC_CAN_SEND_MASK))
//			return -ENOIOCTLCMD;

//		result = put_user(LIRC_SEND2MODE
//				  (hardware[type].features&LIRC_CAN_SEND_MASK),
//				  (__u32 *) arg);
		if (result)
			return result;
		break;

	case LIRC_SET_SEND_MODE:
//		if (!(hardware[type].features&LIRC_CAN_SEND_MASK))
//			return -ENOIOCTLCMD;

		result = get_user(value, (__u32 *) arg);
		if (result)
			return result;
		/* only LIRC_MODE_PULSE supported */
		if (value != LIRC_MODE_PULSE)
			return -ENOSYS;
		break;

	case LIRC_GET_LENGTH:
		return -ENOSYS;
		break;

	case LIRC_SET_SEND_DUTY_CYCLE:
//		if (!(hardware[type].features&LIRC_CAN_SET_SEND_DUTY_CYCLE))
//			return -ENOIOCTLCMD;

		result = get_user(value, (__u32 *) arg);
		if (result)
			return result;
		if (value <= 0 || value > 100)
			return -EINVAL;
		return init_timing_params(value, freq);
		break;

	case LIRC_SET_SEND_CARRIER:
//		if (!(hardware[type].features&LIRC_CAN_SET_SEND_CARRIER))
//			return -ENOIOCTLCMD;

		result = get_user(value, (__u32 *) arg);
		if (result)
			return result;
		if (value > 500000 || value < 20000)
			return -EINVAL;
		return init_timing_params(duty_cycle, value);
		break;

	default:
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35)
		return lirc_dev_fop_ioctl(node, filep, cmd, arg);
#else
		return lirc_dev_fop_ioctl(filep, cmd, arg);
#endif
	}
	return 0;
}

static const struct file_operations lirc_fops = {
	.owner		= THIS_MODULE,
	.write		= lirc_write,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35)
	.ioctl		= lirc_ioctl,
#else
	.unlocked_ioctl	= lirc_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= lirc_ioctl,
#endif
#endif
	.read		= lirc_dev_fop_read,
	.poll		= lirc_dev_fop_poll,
	.open		= lirc_dev_fop_open,
	.release	= lirc_dev_fop_close,
};

static struct lirc_driver driver = {
	.name		= LIRC_DRIVER_NAME,
	.minor		= -1,
	.code_length	= 1,
	.sample_rate	= 0,
	.data		= NULL,
	.add_to_buf	= NULL,
	.rbuf		= &rbuf,
	.set_use_inc	= set_use_inc,
	.set_use_dec	= set_use_dec,
	.fops		= &lirc_fops,
	.dev		= NULL,
	.owner		= THIS_MODULE,
};

static struct platform_device *lirc_dove_dev;

static int __devinit lirc_dove_probe(struct platform_device *dev)
{
	return 0;
}

static int __devexit lirc_dove_remove(struct platform_device *dev)
{
	return 0;
}
#if 0 // Not now
static int lirc_serial_suspend(struct platform_device *dev,
			       pm_message_t state)
{
	/* Set DLAB 0. */
	soutp(UART_LCR, sinp(UART_LCR) & (~UART_LCR_DLAB));

	/* Disable all interrupts */
	soutp(UART_IER, sinp(UART_IER) &
	      (~(UART_IER_MSI|UART_IER_RLSI|UART_IER_THRI|UART_IER_RDI)));

	/* Clear registers. */
	sinp(UART_LSR);
	sinp(UART_RX);
	sinp(UART_IIR);
	sinp(UART_MSR);

	return 0;
}
#endif
/* twisty maze... need a forward-declaration here... */
static void lirc_dove_exit(void);
#if 0 // Not now
static int lirc_serial_resume(struct platform_device *dev)
{
	unsigned long flags;

	if (hardware_init_port() < 0) {
		lirc_serial_exit();
		return -EINVAL;
	}

	spin_lock_irqsave(&hardware[type].lock, flags);
	/* Enable Interrupt */
	do_gettimeofday(&lasttv);
	soutp(UART_IER, sinp(UART_IER)|UART_IER_MSI);
	off();

	lirc_buffer_clear(&rbuf);

	spin_unlock_irqrestore(&hardware[type].lock, flags);

	return 0;
}
#endif
static struct platform_driver lirc_dove_driver = {
	.probe		= lirc_dove_probe,
	.remove		= __devexit_p(lirc_dove_remove),
	.suspend	= NULL,//lirc_serial_suspend,
	.resume		= NULL,//lirc_serial_resume,
	.driver		= {
		.name	= "lirc_dove",
		.owner	= THIS_MODULE,
	},
};

static int __init lirc_dove_init(void)
{
	int result;

	/* Init read buffer. */
	result = lirc_buffer_init(&rbuf, sizeof(int), RBUF_LEN);
	if (result < 0)
		return -ENOMEM;

	result = platform_driver_register(&lirc_dove_driver);
	if (result) {
		printk("lirc register returned %d\n", result);
		goto exit_buffer_free;
	}

	lirc_dove_dev = platform_device_alloc("lirc_dove", 0);
	if (!lirc_dove_dev) {
		result = -ENOMEM;
		goto exit_driver_unregister;
	}

	result = platform_device_add(lirc_dove_dev);
	if (result)
		goto exit_device_put;

	return 0;

exit_device_put:
	platform_device_put(lirc_dove_dev);
exit_driver_unregister:
	platform_driver_unregister(&lirc_dove_driver);
exit_buffer_free:
	lirc_buffer_free(&rbuf);
	return result;
}

static void lirc_dove_exit(void)
{
	platform_device_unregister(lirc_dove_dev);
	platform_driver_unregister(&lirc_dove_driver);
	lirc_buffer_free(&rbuf);
}

static int __init lirc_dove_init_module(void)
{
	int result;

	result = lirc_dove_init();
	if (result)
		return result;

#if 0
	if (!softcarrier) {
		switch (type) {
		case LIRC_HOMEBREW:
		case LIRC_IGOR:
#ifdef CONFIG_LIRC_SERIAL_NSLU2
		case LIRC_NSLU2:
#endif
			hardware[type].features &=
				~(LIRC_CAN_SET_SEND_DUTY_CYCLE|
				  LIRC_CAN_SET_SEND_CARRIER);
			break;
		}
	}
#endif
	result = init_port();
	if (result < 0)
		goto exit_dove_exit;
	driver.features = LIRC_CAN_REC_MODE2;//LIRC_CAN_REC_RAW | LIRC_CAN_REC_PULSE;// FIXME hardware[type].features;
	driver.dev = &lirc_dove_dev->dev;
	driver.minor = lirc_register_driver(&driver);
	if (driver.minor < 0) {
		printk(KERN_ERR  LIRC_DRIVER_NAME
		       ": register_chrdev failed!\n");
		result = -EIO;
		goto exit_release;
	}
	return 0;
exit_release:
	//release_region(io, 8);
exit_dove_exit:
	lirc_dove_exit();
	return result;
}

static void __exit lirc_dove_exit_module(void)
{
	lirc_dove_exit();
#if 0
	if (iommap != 0)
		release_mem_region(iommap, 8 << ioshift);
	else
		release_region(io, 8);
#endif
	lirc_unregister_driver(driver.minor);
	printk("cleaned up module\n");
}


module_init(lirc_dove_init_module);
module_exit(lirc_dove_exit_module);

MODULE_DESCRIPTION("Infra-red receiver driver for Dove GPIO.");
MODULE_AUTHOR("Rabeeh Khoury (rabeeh@solid-run.com)");
MODULE_LICENSE("GPL");
