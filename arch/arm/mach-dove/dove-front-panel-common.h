/*
 * arch/arm/mach-dove/dove-front-panel-common.h
 *
 * Marvell Dove MV88F6781-RD/DB Board Setup for device on front panel
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef __ARCH_DOVE_FRONT_PANEL_COMMON_H
#define __ARCH_DOVE_FRONT_PANEL_COMMON_H
extern struct gpio_mouse_platform_data tact_dove_fp_data;

extern struct spi_board_info __initdata dove_fp_spi_devs[];

extern struct cafe_cam_platform_data dove_cafe_cam_data;

int __init dove_fp_spi_devs_num(void);

int __init dove_fp_ts_gpio_setup(void);

void __init dove_fp_clcd_init(void);

#endif
