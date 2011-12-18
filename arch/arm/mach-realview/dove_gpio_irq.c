#include <linux/string.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/io.h>

#include <asm/types.h>
#include <asm/arch/hardware.h>
#include <asm/arch/irqs.h>
#include <asm/arch/platform.h>
#include <asm/arch/dove_gpio_irq.h>

/*
*	GPIO controlling API
*/
#define GPIODATA_OFFSET 0x0		// PrimeCell GPIO data register
#define GPIODIR_OFFSET 0x400	// PrimeCell GPIO data direction register
#define GPIOIS_OFFSET 0x404		// PrimeCell GPIO interrupt sense register
#define GPIOIBE_OFFSET 0x408	// PrimeCell GPIO interrupt both edges register
#define GPIOIEV_OFFSET 0x40c	// PrimeCell GPIO interrupt event register
#define GPIOIE_OFFSET 0x410		// PrimeCell GPIO interrupt mask
#define GPIORIS_OFFSET 0x414	// PrimeCell GPIO raw interrupt status
#define GPIOMIS_OFFSET 0x418	// PrimeCell GPIO masked interrupt status
#define GPIOIC_OFFSET 0x41c		// PrimeCell GPIO interrupt clear

static u32 gpio_base_addr;

/*
*	config the gpio pin for interrupt
*/
static void gpio_pin_int_config(dove_gpio0_pin pin, gpio_intr_gen intr_gen, gpio_intr_trig intr_trig)
{
	u8 reg_value, set_value = 0;

	// config GPIOIBE
	reg_value = ioread8(gpio_base_addr + GPIOIBE_OFFSET);
	set_value = 1 << (int)pin;
	switch(intr_trig) {
		case GPIO_TRIG_RISING_EDGE:
		case GPIO_TRIG_FALLING_EDGE:
		case GPIO_TRIG_HIGH_LEVEL:
		case GPIO_TRIG_LOW_LEVEL:
			reg_value = reg_value & (~set_value);
			break;
		case GPIO_TRIG_BOTH_EDGE:
			reg_value = reg_value | set_value;
			break;
		default:
			break;
	}
	iowrite8(reg_value, gpio_base_addr + GPIOIBE_OFFSET);

	// config GPIOIEV
	reg_value = ioread8(gpio_base_addr + GPIOIEV_OFFSET);
	set_value = 1 << (int)pin;
	switch(intr_trig) {
		case GPIO_TRIG_RISING_EDGE:
		case GPIO_TRIG_HIGH_LEVEL:
			reg_value = reg_value | set_value;
			break;
		case GPIO_TRIG_FALLING_EDGE:
		case GPIO_TRIG_LOW_LEVEL:
			reg_value = reg_value & (~set_value);
			break;
		case GPIO_TRIG_BOTH_EDGE:
		default:
			break;
	}
	iowrite8(reg_value, gpio_base_addr + GPIOIEV_OFFSET);

	// config GPIOIS
	reg_value = ioread8(gpio_base_addr + GPIOIS_OFFSET);
	set_value = 1 << (int)pin;
	switch(intr_gen) {
		case GPIO_INTR_EDGE_TRIGGER:
			reg_value = reg_value & (~set_value);
			break;
		case GPIO_INTR_LEVEL_TRIGGER:
			reg_value = reg_value | set_value;
			break;
		default:
			break;
	}
	iowrite8(reg_value, gpio_base_addr + GPIOIS_OFFSET);

	// clear all interrupt
	set_value = 0xff;
	iowrite8(reg_value, gpio_base_addr + GPIOIC_OFFSET);

	// config GPIOIE
	reg_value = ioread8(gpio_base_addr + GPIOIE_OFFSET);
	set_value = 1 << (int)pin;
	reg_value = reg_value | set_value;
	iowrite8(reg_value, gpio_base_addr + GPIOIE_OFFSET);

	return;
}

static  u32 gpio_get_int_src(void)
{
	u32 status;
	
	// read GPIOMIS
	status = ioread8(gpio_base_addr + GPIOMIS_OFFSET);

	return status;
}

static void gpio_clear_int(dove_gpio0_pin pin)
{
	u8 set_value;
	
	// clear GPIOIC
	set_value = 1 << (int)pin;
	iowrite8(set_value, gpio_base_addr + GPIOIC_OFFSET);

	return;
}

/*
*	Dove FlareonGV use these gpio0 pins as interrupt on Realview EB
*	
*	GPIO0_0: LCD
*	GPIO0_3: VMETA
*	GPIO0_7: GPU
*/
#define NR_DOVE_GPIO_IRQ 3
#define DOVE_GPIO0_LCD 0
#define DOVE_GPIO0_VMETA 1
#define DOVE_GPIO0_GPU 2
static struct dove_gpio_irq_handler irq_handles[NR_DOVE_GPIO_IRQ];

int dove_gpio_request(dove_gpio0_pin pin, struct dove_gpio_irq_handler *handle)
{
	int nr_irq = 0;
	int ret = 0;

	// check for GPIO pins
	if((pin != DOVE_GPIO0_0) && (pin != DOVE_GPIO0_3) && (pin != DOVE_GPIO0_7))
		return -EINVAL;

	// check for device
	if((handle == NULL) || (handle->dev_name == NULL)) {
		printk(KERN_WARNING  "%s: Attempt to assing null device to GPIO0 %d.\n", 
			__func__, (int)pin);
		ret = -EINVAL;
		goto out;
	}

	// assign interrupt handles
	switch(pin) {
		case DOVE_GPIO0_0:
			nr_irq = DOVE_GPIO0_LCD;
			break;
		case DOVE_GPIO0_3:
			nr_irq = DOVE_GPIO0_VMETA;
			break;
		case DOVE_GPIO0_7:
			nr_irq = DOVE_GPIO0_GPU;
			break;
		default:
			break;
	}

	if((irq_handles[nr_irq].dev_name == NULL) ||
		(!strcmp(irq_handles[nr_irq].dev_name, handle->dev_name))) {
		irq_handles[nr_irq].dev_name = handle->dev_name;
		irq_handles[nr_irq].dev_id = handle->dev_id;
		irq_handles[nr_irq].handler = handle->handler;
		irq_handles[nr_irq].intr_gen = handle->intr_gen;
		irq_handles[nr_irq].intr_trig = handle->intr_trig;
	} else {
		printk(KERN_WARNING "%s: GPIO0 %d already allocated to %s!\n", 
			__func__, (int)pin, irq_handles[nr_irq].dev_name);
		ret = -EBUSY;
		goto out;
	}

	// enable interrupt
	gpio_pin_int_config(pin, handle->intr_gen, handle->intr_trig);

out:
	return ret;
}
EXPORT_SYMBOL(dove_gpio_request);

int dove_gpio_free(dove_gpio0_pin pin, char *dev_name)
{
	u8 reg_value, set_value;
	int nr_irq;

	// check for GPIO pins
	if((pin != DOVE_GPIO0_0) || (pin != DOVE_GPIO0_3) || (pin != DOVE_GPIO0_7))
		return -EINVAL;

	// check for device
	if(dev_name == NULL)
		return -EINVAL;

	// release GPIO interrupts
	switch(pin) {
		case DOVE_GPIO0_0:
			nr_irq = DOVE_GPIO0_LCD;
			break;
		case DOVE_GPIO0_3:
			nr_irq = DOVE_GPIO0_VMETA;
			break;
		case DOVE_GPIO0_7:
			nr_irq = DOVE_GPIO0_GPU;
			break;
		default:
			break;
	}

	if(irq_handles[nr_irq].dev_name == NULL)
		return -ENXIO;

	if(strcmp(irq_handles[nr_irq].dev_name, dev_name)) {
		printk(KERN_WARNING "%s: Attempt to release GPIO0 %d failed!\n",
			__func__, (int)pin);
		return -ENXIO;
	}

	// config GPIOIE
	reg_value = ioread8(gpio_base_addr + GPIOIE_OFFSET);
	set_value = 1 << (int)pin;
	reg_value = reg_value & (~set_value);
	iowrite8(reg_value, gpio_base_addr + GPIOIE_OFFSET);

	irq_handles[nr_irq].dev_name = NULL;
	irq_handles[nr_irq].dev_id = NULL;
	irq_handles[nr_irq].handler = NULL;

	return 0;
}
EXPORT_SYMBOL(dove_gpio_free);

static irqreturn_t realview_dove_gpio_irqhandler(int irq, void *dev_id)//, struct pt_regs *reg)
{
	u32 int_status;
	int nr_irq;
	dove_gpio0_pin intr_pin = DOVE_GPIO0_0;

	// get interrupt status
	int_status = gpio_get_int_src();

	if(int_status & (1 << (int)DOVE_GPIO0_0)) {
		nr_irq = DOVE_GPIO0_LCD;
		intr_pin = DOVE_GPIO0_0;
	} else if(int_status & (1 << (int)DOVE_GPIO0_3)) {
		nr_irq = DOVE_GPIO0_VMETA;
		intr_pin = DOVE_GPIO0_3;
	} else if(int_status & (1 << (int)DOVE_GPIO0_7)) {
		nr_irq = DOVE_GPIO0_GPU;
		intr_pin = DOVE_GPIO0_7;
	} else {
		printk(KERN_WARNING "%s: didn't use GPIO0 %d as interrupt\n",
				__func__, int_status);
		goto out;
	}

	if(irq_handles[nr_irq].handler == NULL) {
		printk(KERN_WARNING "%s: No interrupt handler for GPIO0 %d\n", 
			__func__, int_status);
		goto out;
	}
	
	irq_handles[nr_irq].handler((int)int_status, (void*)irq_handles[nr_irq].dev_id);

out:
	// clear interrupt
	gpio_clear_int(intr_pin);

	return IRQ_HANDLED;
}

static int __init dove_gpio_irq_init(void)
{
	int reg_value;
	int result;
	int i;

	/* get the gpio0 base addr */
	gpio_base_addr = (u32)ioremap(REALVIEW_EB_GPIO0_BASE, 0xfff);
	/* PLiao add for loading galcore module unstable issue, force all GPIO0 pins to be input */
	reg_value = 0;
	iowrite8(reg_value, gpio_base_addr + GPIODIR_OFFSET);
	/* End of add */
	/* configure the SYS_IOSEL for GPIO0 */
	reg_value = ioread32(IO_ADDRESS(REALVIEW_SYS_IOSEL));
	reg_value = (reg_value & (~0x00700000)) | 0x00100000;
	iowrite32(reg_value, IO_ADDRESS(REALVIEW_SYS_IOSEL));

	/* config GPIODIR */
	reg_value = ioread8(gpio_base_addr + GPIODIR_OFFSET);
	reg_value &= ~(DOVE_GPIO0_0 | DOVE_GPIO0_3 | DOVE_GPIO0_7);
	iowrite8(reg_value, gpio_base_addr + GPIODIR_OFFSET);
	
	/* Request IRQ for GPIO 0 */
	result = request_irq(IRQ_EB_GPIO0, realview_dove_gpio_irqhandler, 0, DOVE_GPIO_DRV_NAME, NULL);
	if(result < 0) {
		printk(KERN_ERR "%s: can not request IRQ(%d)\n", DOVE_GPIO_DRV_NAME, IRQ_EB_GPIO0);
		return result;
	}

	/* Initialize interrupt handlers */
	for(i=0; i<NR_DOVE_GPIO_IRQ; i++) {
		irq_handles[i].dev_name = NULL;
		irq_handles[i].dev_id = NULL;
		irq_handles[i].handler = NULL;
	}

	return 0;
}

static void __exit dove_gpio_irq_exit(void)
{
	/* free requested IRQ */
	free_irq(IRQ_EB_GPIO0, NULL);

	/* unmap the gpio0 address */
	iounmap((void*)gpio_base_addr);
}

module_init(dove_gpio_irq_init);
module_exit(dove_gpio_irq_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Dove GPIO driver");
