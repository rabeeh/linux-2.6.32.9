#ifndef __ASM_ARCH_DOVE_BL_H
#define __ASM_ARCH_DOVE_BL_H
#include <linux/ioport.h>

struct dovebl_platform_data {
	int	default_intensity;
	int	gpio_pm_control; /* enable LCD/panel power management via gpio*/

	resource_size_t lcd_start;	/* lcd power control reg base. */
	resource_size_t lcd_end;	/* end of reg map. */
	unsigned long lcd_offset;	/* register offset */
	unsigned long lcd_mapped;	/* pa = 0, va = 1 */
	unsigned long lcd_mask;		/* mask */
	unsigned long lcd_on;		/* value to enable lcd power */
	unsigned long lcd_off;		/* value to disable lcd power */

	resource_size_t blpwr_start;	/* backlight pwr ctrl reg base. */
	resource_size_t blpwr_end;	/* end of reg map. */
	unsigned long blpwr_offset;	/* register offset */
	unsigned long blpwr_mapped;	/* pa = 0, va = 1 */
	unsigned long blpwr_mask;	/* mask */
	unsigned long blpwr_on;		/* value to enable bl power */
	unsigned long blpwr_off;	/* value to disable bl power */

	resource_size_t btn_start;	/* brightness control reg base. */
	resource_size_t btn_end;	/* end of reg map. */
	unsigned long btn_offset;	/* register offset */
	unsigned long btn_mapped;	/* pa = 0, va = 1 */
	unsigned long btn_mask;	/* mask */
	unsigned long btn_level;	/* how many level can be configured. */
	unsigned long btn_min;	/* min value */
	unsigned long btn_max;	/* max value */
	unsigned long btn_inc;	/* increment */
};
#endif /* __ASM_ARCH_DOVE_BL_H */
