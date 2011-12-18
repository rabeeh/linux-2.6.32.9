#ifndef __DOVEFB_GPIO__
extern int dovefb_save_regbase(void *reg_base, unsigned int id);
extern int dovefb_set_gpio(unsigned int lcd_id, unsigned int mask,
	unsigned int output);
extern int dovefb_get_current_setting(unsigned int lcd_id,
	unsigned int *mask,
	unsigned int *output);
extern int dovefb_set_pin_state(unsigned int lcd_id,
	unsigned int pin, unsigned int state);

/*
 * set lcd gpio pin state
 * pin, which pin want to set, 0~7.
 * state, what state want to set, 0 or 1.
 * if success, return 0.
 * if failed, other value.
 * int set_lcd_gpio_pin(unsgined int pin, unsigned int state);
 */
#define set_lcd_gpio_pin(pin, state)	dovefb_set_pin_state(0, pin, state)

#endif /* __DOVEFB_GPIO__ */
