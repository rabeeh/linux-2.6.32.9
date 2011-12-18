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

#ifndef __PXA_AC97_REGS_H
#define __PXA_AC97_REGS_H

/*
 * AC97 Controller registers
 */

#define POCR		__AC97(0x0)  	/* PCM Out Control Register */
#define POCR_FEIE	(1 << 3)	/* FIFO Error Interrupt Enable */
#define POCR_FSRIE	(1 << 1)	/* FIFO Service Request Int Enable */

#define PICR		__AC97(0x4)  	/* PCM In Control Register */
#define PICR_FEIE	(1 << 3)	/* FIFO Error Interrupt Enable */
#define PICR_FSRIE	(1 << 1)	/* FIFO Service Request Int Enable */

#define MCCR		__AC97(0x8)  	/* Mic In Control Register */
#define MCCR_FEIE	(1 << 3)	/* FIFO Error Interrupt Enable */
#define MCCR_FSRIE	(1 << 1)	/* FIFO Service Request Int Enable */

#define GCR		__AC97(0xC)  /* Global Control Register */
#if defined(CONFIG_PXA3xx) || defined(CONFIG_ARCH_DOVE)
#define GCR_CLKBPB	(1 << 31)	/* Internal clock enable */
#endif
#define GCR_nDMAEN	(1 << 24)	/* non DMA Enable */
#define GCR_CDONE_IE	(1 << 19)	/* Command Done Interrupt Enable */
#define GCR_SDONE_IE	(1 << 18)	/* Status Done Interrupt Enable */
#define GCR_SECRDY_IEN	(1 << 9)	/* Secondary Ready Interrupt Enable */
#define GCR_PRIRDY_IEN	(1 << 8)	/* Primary Ready Interrupt Enable */
#define GCR_SECRES_IEN	(1 << 5)	/* Secondary Resume Interrupt Enable */
#define GCR_PRIRES_IEN	(1 << 4)	/* Primary Resume Interrupt Enable */
#define GCR_ACLINK_OFF	(1 << 3)	/* AC-link Shut Off */
#define GCR_WARM_RST	(1 << 2)	/* AC97 Warm Reset */
#define GCR_COLD_RST	(1 << 1)	/* AC'97 Cold Reset (0 = active) */
#define GCR_GIE		(1 << 0)	/* Codec GPI Interrupt Enable */

#define POSR		__AC97(0x10)	/* PCM Out Status Register */
#define POSR_FIFOE	(1 << 4)	/* FIFO error */
#define POSR_FSR	(1 << 2)	/* FIFO Service Request */

#define PISR		__AC97(0x14)  	/* PCM In Status Register */
#define PISR_FIFOE	(1 << 4)	/* FIFO error */
#define PISR_EOC	(1 << 3)	/* DMA End-of-Chain (exclusive clear) */
#define PISR_FSR	(1 << 2)	/* FIFO Service Request */

#define MCSR		__AC97(0x18)  	/* Mic In Status Register */
#define MCSR_FIFOE	(1 << 4)	/* FIFO error */
#define MCSR_EOC	(1 << 3)	/* DMA End-of-Chain (exclusive clear) */
#define MCSR_FSR	(1 << 2)	/* FIFO Service Request */

#define GSR		__AC97(0x1C)  	/* Global Status Register */
#define GSR_CDONE	(1 << 19)	/* Command Done */
#define GSR_SDONE	(1 << 18)	/* Status Done */
#define GSR_RDCS	(1 << 15)	/* Read Completion Status */
#define GSR_BIT3SLT12	(1 << 14)	/* Bit 3 of slot 12 */
#define GSR_BIT2SLT12	(1 << 13)	/* Bit 2 of slot 12 */
#define GSR_BIT1SLT12	(1 << 12)	/* Bit 1 of slot 12 */
#define GSR_SECRES	(1 << 11)	/* Secondary Resume Interrupt */
#define GSR_PRIRES	(1 << 10)	/* Primary Resume Interrupt */
#define GSR_SCR		(1 << 9)	/* Secondary Codec Ready */
#define GSR_PCR		(1 << 8)	/*  Primary Codec Ready */
#define GSR_MCINT	(1 << 7)	/* Mic In Interrupt */
#define GSR_POINT	(1 << 6)	/* PCM Out Interrupt */
#define GSR_PIINT	(1 << 5)	/* PCM In Interrupt */
#define GSR_ACOFFD	(1 << 3)	/* AC-link Shut Off Done */
#define GSR_MOINT	(1 << 2)	/* Modem Out Interrupt */
#define GSR_MIINT	(1 << 1)	/* Modem In Interrupt */
#define GSR_GSCI	(1 << 0)	/* Codec GPI Status Change Interrupt */

#define CAR		__AC97(0x20)	/* CODEC Access Register */
#define CAR_CAIP	(1 << 0)	/* Codec Access In Progress */

#define PCSDR		__AC97(0x2C)	/* PCM Surround Data Register */
#define PCCLDR		__AC97(0x38)	/* PCM Center / LFE Data Register */

#define PCDR		__AC97(0x40)	/* PCM FIFO Data Register */
#define MCDR		__AC97(0x60)	/* Mic-in FIFO Data Register */

#define MOCR		__AC97(0x100)  	/* Modem Out Control Register */
#define MOCR_FEIE	(1 << 3)	/* FIFO Error */
#define MOCR_FSRIE	(1 << 1)	/* FIFO Service Request Interrupt En */

#define MICR		__AC97(0x108)	/* Modem In Control Register */
#define MICR_FEIE	(1 << 3)	/* FIFO Error */
#define MICR_FSRIE	(1 << 1)	/* FIFO Service Request Interrupt En */

#define MOSR		__AC97(0x110)	/* Modem Out Status Register */
#define MOSR_FIFOE	(1 << 4)	/* FIFO error */
#define MOSR_FSR	(1 << 2)	/* FIFO Service Request */

#define MISR		__AC97(0x118)	/* Modem In Status Register */
#define MISR_FIFOE	(1 << 4)	/* FIFO error */
#define MISR_EOC	(1 << 3)	/* DMA End-of-Chain (exclusive clear) */
#define MISR_FSR	(1 << 2)	/* FIFO Service Request */

#define MODR		__AC97(0x140)	/* Modem FIFO Data Register */

#define PAC_REG_BASE	__AC97(0x200)	/* Primary Audio Codec */
#define SAC_REG_BASE	__AC97(0x300)	/* Secondary Audio Codec */
#define PMC_REG_BASE	__AC97(0x400)	/* Primary Modem Codec */
#define SMC_REG_BASE	__AC97(0x500)	/* Secondary Modem Codec */

#endif
