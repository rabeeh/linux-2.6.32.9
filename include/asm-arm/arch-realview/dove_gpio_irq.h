#ifndef DOVE_GPIO_IRQ_H
#define DOVE_GPIO_IRQ_H

#define DOVE_GPIO_DRV_NAME "RealView GPIO0"

typedef enum {
	DOVE_GPIO0_0 = 0,		// LCD
	DOVE_GPIO0_1,
	DOVE_GPIO0_2,
	DOVE_GPIO0_3,		// VMETA
	DOVE_GPIO0_4,
	DOVE_GPIO0_5,
	DOVE_GPIO0_6,
	DOVE_GPIO0_7		// GPU
} dove_gpio0_pin;

typedef enum {
	GPIO_INTR_EDGE_TRIGGER,
	GPIO_INTR_LEVEL_TRIGGER
}gpio_intr_gen;

typedef enum {
	GPIO_TRIG_RISING_EDGE,
	GPIO_TRIG_FALLING_EDGE,
	GPIO_TRIG_BOTH_EDGE,
	GPIO_TRIG_HIGH_LEVEL,
	GPIO_TRIG_LOW_LEVEL
}gpio_intr_trig;

struct dove_gpio_irq_handler {
	char *dev_name;
	char *dev_id;
	void (*handler)(int , void *);	// irq, dev_id
	gpio_intr_gen intr_gen; 
	gpio_intr_trig intr_trig;
};

extern int dove_gpio_request(dove_gpio0_pin pin, struct dove_gpio_irq_handler *handle);
extern int dove_gpio_free(dove_gpio0_pin pin, char *dev_name);

#endif /* DOVE_GPIO_IRQ_H */
