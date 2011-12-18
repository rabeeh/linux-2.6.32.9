/*
 * arch/arm/mach-dove/twsi.c
 *
 * TWSI MUX functions for Marvell Dove 88F6781 SoC
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/io.h>
#include <mach/dove.h>

#include "twsi.h"

static void dove_twsi_option_set(dove_twsi_option option, unsigned int enable)
{
	u32	reg;
	unsigned int addr;
	unsigned int mask;

	switch (option)
	{
	case DOVE_TWSI_OPTION1:
		addr = DOVE_GLOBAL_CONFIG_1;
		mask = DOVE_TWSI_ENABLE_OPTION1;
		break;
	case DOVE_TWSI_OPTION2:
		addr = DOVE_GLOBAL_CONFIG_2;
		mask = DOVE_TWSI_ENABLE_OPTION2;
		break;
	case DOVE_TWSI_OPTION3:
		addr = DOVE_GLOBAL_CONFIG_2;
		mask = DOVE_TWSI_ENABLE_OPTION3;
		break;
	default:
		printk("error: unknown TWSI option %d\n", option);
		return;
	}

	reg = readl(addr);

	if (enable)
		reg |= mask;
	else
		reg &= ~mask;

	writel(reg, addr);	
}

static unsigned int port_id_old = -1;

int dove_select_exp_port(unsigned int port_id)
{
	if(port_id_old != port_id)
	{	       
		/* disable all*/
		dove_twsi_option_set(DOVE_TWSI_OPTION1, 0);
		dove_twsi_option_set(DOVE_TWSI_OPTION2, 0);
		dove_twsi_option_set(DOVE_TWSI_OPTION3, 0);

		/* enable requested port*/
		dove_twsi_option_set(port_id, 1);
		port_id_old = port_id;
	}

	return 0;
}

void dove_reset_exp_port()
{
	port_id_old = -1;
}
