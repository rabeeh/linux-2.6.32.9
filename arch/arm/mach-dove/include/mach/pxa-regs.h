/*
 *  linux/include/asm-arm/arch-pxa/pxa-regs.h
 *
 *  Author:	Nicolas Pitre
 *  Created:	Jun 15, 2001
 *  Copyright:	MontaVista Software Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __PXA_REGS_H
#define __PXA_REGS_H
/*
 * DMA Controller
 */
#define __DMA(x)	*((volatile u32 *)(DOVE_PDMA_VIRT_BASE + (x)))
#define __ADDR(x)	(DOVE_PDMA_VIRT_BASE + (x))
#define __DMA2(x, y)	*((volatile u32 *)(DOVE_PDMA_VIRT_BASE + (x) + (y)))
#define __ADDR2(x, y)	(DOVE_PDMA_VIRT_BASE + (x) + (y))

#include <asm/hardware/pxa-dma-regs.h>

/*
 * AC97 Controller
 */
#define __AC97(x)	*((volatile u32 *)(DOVE_AC97_VIRT_BASE + (x)))
#define AC97_REG_4_PDMA(x)	(0x58190000 | (x & 0xFFFF))
#include <asm/hardware/pxa-ac97-regs.h>

#endif
