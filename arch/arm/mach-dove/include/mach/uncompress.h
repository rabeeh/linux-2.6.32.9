/*
 * include/asm-arm/arch-dove/uncompress.h
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <mach/dove.h>

#if defined(CONFIG_MACH_DOVE_RD_AVNG) || defined(CONFIG_MACH_DOVE_RD_AVNG_Z0) ||\
	defined(CONFIG_MACH_DOVE_RD_AVNG_NB_Z0) || defined(CONFIG_MACH_DOVE_DB)
#define UART_THR ((volatile unsigned char *)(DOVE_UART0_PHYS_BASE + 0x0))
#define UART_LSR ((volatile unsigned char *)(DOVE_UART0_PHYS_BASE + 0x14))
#else
#define UART_THR ((volatile unsigned char *)(DOVE_UART1_PHYS_BASE + 0x0))
#define UART_LSR ((volatile unsigned char *)(DOVE_UART1_PHYS_BASE + 0x14))
#endif

#define LSR_THRE	0x20

static void putc(const char c)
{
	int i;

	for (i = 0; i < 0x1000; i++) {
		/* Transmit fifo not full? */
		if (*UART_LSR & LSR_THRE)
			break;
	}

	*UART_THR = c;
}

static void flush(void)
{
}

/*
 * nothing to do
 */
#define arch_decomp_setup()
#define arch_decomp_wdog()
