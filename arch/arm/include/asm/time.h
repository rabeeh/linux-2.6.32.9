/*
 * include/asm-arm/plat-orion/time.h
 *
 * Marvell Orion SoC time handling.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __ASM_PLAT_ORION_TIME_H
#define __ASM_PLAT_ORION_TIME_H
#define   TIMER_CTRL		(TIMER_VIRT_BASE + 0x0000)
#define    TIMER0_EN		0x0001
#define    TIMER0_RELOAD_EN	0x0002
#define    TIMER1_EN		0x0004
#define    TIMER1_RELOAD_EN	0x0008
#define   TIMER0_RELOAD		(TIMER_VIRT_BASE + 0x0010)
#define   TIMER0_VAL		(TIMER_VIRT_BASE + 0x0014)
#define   TIMER1_RELOAD		(TIMER_VIRT_BASE + 0x0018)
#define   TIMER1_VAL		(TIMER_VIRT_BASE + 0x001c)
#define   TIMER_WD_RELOAD		(TIMER_VIRT_BASE + 0x0020)
#define   TIMER_WD_VAL		(TIMER_VIRT_BASE + 0x0024)

void orion_time_init(unsigned int irq, unsigned int tclk);


#endif
