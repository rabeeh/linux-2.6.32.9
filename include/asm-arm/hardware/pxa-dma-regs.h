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

#ifndef __PXA_DMA_REGS_H
#define __PXA_DMA_REGS_H

/*
 * DMA Controller
 */

#define DCSR0		__DMA(0)  /* DMA Control / Status Register for Channel 0 */
#define DCSR1		__DMA(4)  /* DMA Control / Status Register for Channel 1 */
#define DCSR2		__DMA(8)  /* DMA Control / Status Register for Channel 2 */
#define DCSR3		__DMA(0xc)  /* DMA Control / Status Register for Channel 3 */
#define DCSR4		__DMA(0x10)  /* DMA Control / Status Register for Channel 4 */
#define DCSR5		__DMA(0x14)  /* DMA Control / Status Register for Channel 5 */
#define DCSR6		__DMA(0x18)  /* DMA Control / Status Register for Channel 6 */
#define DCSR7		__DMA(0x1c)  /* DMA Control / Status Register for Channel 7 */
#define DCSR8		__DMA(0x20)  /* DMA Control / Status Register for Channel 8 */
#define DCSR9		__DMA(0x24)  /* DMA Control / Status Register for Channel 9 */
#define DCSR10		__DMA(0x28)  /* DMA Control / Status Register for Channel 10 */
#define DCSR11		__DMA(0x2c)  /* DMA Control / Status Register for Channel 11 */
#define DCSR12		__DMA(0x30)  /* DMA Control / Status Register for Channel 12 */
#define DCSR13		__DMA(0x34)  /* DMA Control / Status Register for Channel 13 */
#define DCSR14		__DMA(0x38)  /* DMA Control / Status Register for Channel 14 */
#define DCSR15		__DMA(0x3c)  /* DMA Control / Status Register for Channel 15 */

#define DCSR(x)		__DMA2(0, (x) << 2)

#define DCSR_RUN	(1 << 31)	/* Run Bit (read / write) */
#define DCSR_NODESC	(1 << 30)	/* No-Descriptor Fetch (read / write) */
#define DCSR_STOPIRQEN	(1 << 29)	/* Stop Interrupt Enable (read / write) */
#ifdef CONFIG_PXA27x
#define DCSR_EORIRQEN	(1 << 28)       /* End of Receive Interrupt Enable (R/W) */
#define DCSR_EORJMPEN	(1 << 27)       /* Jump to next descriptor on EOR */
#define DCSR_EORSTOPEN	(1 << 26)       /* STOP on an EOR */
#define DCSR_SETCMPST	(1 << 25)       /* Set Descriptor Compare Status */
#define DCSR_CLRCMPST	(1 << 24)       /* Clear Descriptor Compare Status */
#define DCSR_CMPST	(1 << 10)       /* The Descriptor Compare Status */
#define DCSR_EORINTR	(1 << 9)        /* The end of Receive */
#endif
#define DCSR_REQPEND	(1 << 8)	/* Request Pending (read-only) */
#define DCSR_STOPSTATE	(1 << 3)	/* Stop State (read-only) */
#define DCSR_ENDINTR	(1 << 2)	/* End Interrupt (read / write) */
#define DCSR_STARTINTR	(1 << 1)	/* Start Interrupt (read / write) */
#define DCSR_BUSERR	(1 << 0)	/* Bus Error Interrupt (read / write) */

#define DALGN		__DMA(0xa0)  /* DMA Alignment Register */
#define DINT		__DMA(0xf0)  /* DMA Interrupt Register */

#define DRCMR(n)	(*(((n) < 64) ? \
			&__DMA2(0x100, ((n) & 0x3f) << 2) : \
			&__DMA2(0x1100, ((n) & 0x3f) << 2)))

#define DRCMR0		__DMA(0x100)  /* Request to Channel Map Register for DREQ 0 */
#define DRCMR1		__DMA(0x104)  /* Request to Channel Map Register for DREQ 1 */
#define DRCMR2		__DMA(0x108)  /* Request to Channel Map Register for I2S receive Request */
#define DRCMR3		__DMA(0x10c)  /* Request to Channel Map Register for I2S transmit Request */
#define DRCMR4		__DMA(0x110)  /* Request to Channel Map Register for BTUART receive Request */
#define DRCMR5		__DMA(0x114)  /* Request to Channel Map Register for BTUART transmit Request. */
#define DRCMR6		__DMA(0x118)  /* Request to Channel Map Register for FFUART receive Request */
#define DRCMR7		__DMA(0x11c)  /* Request to Channel Map Register for FFUART transmit Request */
#define DRCMR8		__DMA(0x120)  /* Request to Channel Map Register for AC97 microphone Request */
#define DRCMR9		__DMA(0x124)  /* Request to Channel Map Register for AC97 modem receive Request */
#define DRCMR10		__DMA(0x128)  /* Request to Channel Map Register for AC97 modem transmit Request */
#define DRCMR11		__DMA(0x12c)  /* Request to Channel Map Register for AC97 audio receive Request */
#define DRCMR12		__DMA(0x130)  /* Request to Channel Map Register for AC97 audio transmit Request */
#define DRCMR13		__DMA(0x134)  /* Request to Channel Map Register for SSP receive Request */
#define DRCMR14		__DMA(0x138)  /* Request to Channel Map Register for SSP transmit Request */
#define DRCMR15		__DMA(0x13c)  /* Request to Channel Map Register for SSP2 receive Request */
#define DRCMR16		__DMA(0x140)  /* Request to Channel Map Register for SSP2 transmit Request */
#define DRCMR17		__DMA(0x144)  /* Request to Channel Map Register for ICP receive Request */
#define DRCMR18		__DMA(0x148)  /* Request to Channel Map Register for ICP transmit Request */
#define DRCMR19		__DMA(0x14c)  /* Request to Channel Map Register for STUART receive Request */
#define DRCMR20		__DMA(0x150)  /* Request to Channel Map Register for STUART transmit Request */
#define DRCMR21		__DMA(0x154)  /* Request to Channel Map Register for MMC receive Request */
#define DRCMR22		__DMA(0x158)  /* Request to Channel Map Register for MMC transmit Request */
#define DRCMR23		__DMA(0x15c)  /* Reserved */
#define DRCMR24		__DMA(0x160)  /* Reserved */
#define DRCMR25		__DMA(0x164)  /* Request to Channel Map Register for USB endpoint 1 Request */
#define DRCMR26		__DMA(0x168)  /* Request to Channel Map Register for USB endpoint 2 Request */
#define DRCMR27		__DMA(0x16C)  /* Request to Channel Map Register for USB endpoint 3 Request */
#define DRCMR28		__DMA(0x170)  /* Request to Channel Map Register for USB endpoint 4 Request */
#define DRCMR29		__DMA(0x174)  /* Reserved */
#define DRCMR30		__DMA(0x178)  /* Request to Channel Map Register for USB endpoint 6 Request */
#define DRCMR31		__DMA(0x17C)  /* Request to Channel Map Register for USB endpoint 7 Request */
#define DRCMR32		__DMA(0x180)  /* Request to Channel Map Register for USB endpoint 8 Request */
#define DRCMR33		__DMA(0x184)  /* Request to Channel Map Register for USB endpoint 9 Request */
#define DRCMR34		__DMA(0x188)  /* Reserved */
#define DRCMR35		__DMA(0x18C)  /* Request to Channel Map Register for USB endpoint 11 Request */
#define DRCMR36		__DMA(0x190)  /* Request to Channel Map Register for USB endpoint 12 Request */
#define DRCMR37		__DMA(0x194)  /* Request to Channel Map Register for USB endpoint 13 Request */
#define DRCMR38		__DMA(0x198)  /* Request to Channel Map Register for USB endpoint 14 Request */
#define DRCMR39		__DMA(0x19C)  /* Reserved */
#define DRCMR66		__DMA(0x1108)  /* Request to Channel Map Register for SSP3 receive Request */
#define DRCMR67		__DMA(0x110C)  /* Request to Channel Map Register for SSP3 transmit Request */
#define DRCMR68		__DMA(0x1110)  /* Request to Channel Map Register for Camera FIFO 0 Request */
#define DRCMR69		__DMA(0x1114)  /* Request to Channel Map Register for Camera FIFO 1 Request */
#define DRCMR70		__DMA(0x1118)  /* Request to Channel Map Register for Camera FIFO 2 Request */
/* Request to Channel Map Register for AC97 surround transmit Request */
#define DRCMR95		__DMA(0x117C)
/* Request to Channel Map Register for AC97 center / lfe transmit Request*/
#define DRCMR96		__DMA(0x1180)

#define DRCMRRXSADR	DRCMR2
#define DRCMRTXSADR	DRCMR3
#define DRCMRRXBTRBR	DRCMR4
#define DRCMRTXBTTHR	DRCMR5
#define DRCMRRXFFRBR	DRCMR6
#define DRCMRTXFFTHR	DRCMR7
#define DRCMRRXMCDR	DRCMR8
#define DRCMRRXMODR	DRCMR9
#define DRCMRTXMODR	DRCMR10
#define DRCMRRXPCDR	DRCMR11
#define DRCMRTXPCDR	DRCMR12
#define DRCMRRXSSDR	DRCMR13
#define DRCMRTXSSDR	DRCMR14
#define DRCMRRXSS2DR   DRCMR15
#define DRCMRTXSS2DR   DRCMR16
#define DRCMRRXICDR	DRCMR17
#define DRCMRTXICDR	DRCMR18
#define DRCMRRXSTRBR	DRCMR19
#define DRCMRTXSTTHR	DRCMR20
#define DRCMRRXMMC	DRCMR21
#define DRCMRTXMMC	DRCMR22
#define DRCMRRXSS3DR   DRCMR66
#define DRCMRTXSS3DR   DRCMR67
#define DRCMRUDC(x)	DRCMR((x) + 24)

#define DRCMR_MAPVLD	(1 << 7)	/* Map Valid (read / write) */
#define DRCMR_CHLNUM	0x1f		/* mask for Channel Number (read / write) */

#define DDADR0		__DMA(0x200)  /* DMA Descriptor Address Register Channel 0 */
#define DSADR0		__DMA(0x204)  /* DMA Source Address Register Channel 0 */
#define DTADR0		__DMA(0x208)  /* DMA Target Address Register Channel 0 */
#define DCMD0		__DMA(0x20c)  /* DMA Command Address Register Channel 0 */
#define DDADR1		__DMA(0x210)  /* DMA Descriptor Address Register Channel 1 */
#define DSADR1		__DMA(0x214)  /* DMA Source Address Register Channel 1 */
#define DTADR1		__DMA(0x218)  /* DMA Target Address Register Channel 1 */
#define DCMD1		__DMA(0x21c)  /* DMA Command Address Register Channel 1 */
#define DDADR2		__DMA(0x220)  /* DMA Descriptor Address Register Channel 2 */
#define DSADR2		__DMA(0x224)  /* DMA Source Address Register Channel 2 */
#define DTADR2		__DMA(0x228)  /* DMA Target Address Register Channel 2 */
#define DCMD2		__DMA(0x22c)  /* DMA Command Address Register Channel 2 */
#define DDADR3		__DMA(0x230)  /* DMA Descriptor Address Register Channel 3 */
#define DSADR3		__DMA(0x234)  /* DMA Source Address Register Channel 3 */
#define DTADR3		__DMA(0x238)  /* DMA Target Address Register Channel 3 */
#define DCMD3		__DMA(0x23c)  /* DMA Command Address Register Channel 3 */
#define DDADR4		__DMA(0x240)  /* DMA Descriptor Address Register Channel 4 */
#define DSADR4		__DMA(0x244)  /* DMA Source Address Register Channel 4 */
#define DTADR4		__DMA(0x248)  /* DMA Target Address Register Channel 4 */
#define DCMD4		__DMA(0x24c)  /* DMA Command Address Register Channel 4 */
#define DDADR5		__DMA(0x250)  /* DMA Descriptor Address Register Channel 5 */
#define DSADR5		__DMA(0x254)  /* DMA Source Address Register Channel 5 */
#define DTADR5		__DMA(0x258)  /* DMA Target Address Register Channel 5 */
#define DCMD5		__DMA(0x25c)  /* DMA Command Address Register Channel 5 */
#define DDADR6		__DMA(0x260)  /* DMA Descriptor Address Register Channel 6 */
#define DSADR6		__DMA(0x264)  /* DMA Source Address Register Channel 6 */
#define DTADR6		__DMA(0x268)  /* DMA Target Address Register Channel 6 */
#define DCMD6		__DMA(0x26c)  /* DMA Command Address Register Channel 6 */
#define DDADR7		__DMA(0x270)  /* DMA Descriptor Address Register Channel 7 */
#define DSADR7		__DMA(0x274)  /* DMA Source Address Register Channel 7 */
#define DTADR7		__DMA(0x278)  /* DMA Target Address Register Channel 7 */
#define DCMD7		__DMA(0x27c)  /* DMA Command Address Register Channel 7 */
#define DDADR8		__DMA(0x280)  /* DMA Descriptor Address Register Channel 8 */
#define DSADR8		__DMA(0x284)  /* DMA Source Address Register Channel 8 */
#define DTADR8		__DMA(0x288)  /* DMA Target Address Register Channel 8 */
#define DCMD8		__DMA(0x28c)  /* DMA Command Address Register Channel 8 */
#define DDADR9		__DMA(0x290)  /* DMA Descriptor Address Register Channel 9 */
#define DSADR9		__DMA(0x294)  /* DMA Source Address Register Channel 9 */
#define DTADR9		__DMA(0x298)  /* DMA Target Address Register Channel 9 */
#define DCMD9		__DMA(0x29c)  /* DMA Command Address Register Channel 9 */
#define DDADR10		__DMA(0x2a0)  /* DMA Descriptor Address Register Channel 10 */
#define DSADR10		__DMA(0x2a4)  /* DMA Source Address Register Channel 10 */
#define DTADR10		__DMA(0x2a8)  /* DMA Target Address Register Channel 10 */
#define DCMD10		__DMA(0x2ac)  /* DMA Command Address Register Channel 10 */
#define DDADR11		__DMA(0x2b0)  /* DMA Descriptor Address Register Channel 11 */
#define DSADR11		__DMA(0x2b4)  /* DMA Source Address Register Channel 11 */
#define DTADR11		__DMA(0x2b8)  /* DMA Target Address Register Channel 11 */
#define DCMD11		__DMA(0x2bc)  /* DMA Command Address Register Channel 11 */
#define DDADR12		__DMA(0x2c0)  /* DMA Descriptor Address Register Channel 12 */
#define DSADR12		__DMA(0x2c4)  /* DMA Source Address Register Channel 12 */
#define DTADR12		__DMA(0x2c8)  /* DMA Target Address Register Channel 12 */
#define DCMD12		__DMA(0x2cc)  /* DMA Command Address Register Channel 12 */
#define DDADR13		__DMA(0x2d0)  /* DMA Descriptor Address Register Channel 13 */
#define DSADR13		__DMA(0x2d4)  /* DMA Source Address Register Channel 13 */
#define DTADR13		__DMA(0x2d8)  /* DMA Target Address Register Channel 13 */
#define DCMD13		__DMA(0x2dc)  /* DMA Command Address Register Channel 13 */
#define DDADR14		__DMA(0x2e0)  /* DMA Descriptor Address Register Channel 14 */
#define DSADR14		__DMA(0x2e4)  /* DMA Source Address Register Channel 14 */
#define DTADR14		__DMA(0x2e8)  /* DMA Target Address Register Channel 14 */
#define DCMD14		__DMA(0x2ec)  /* DMA Command Address Register Channel 14 */
#define DDADR15		__DMA(0x2f0)  /* DMA Descriptor Address Register Channel 15 */
#define DSADR15		__DMA(0x2f4)  /* DMA Source Address Register Channel 15 */
#define DTADR15		__DMA(0x2f8)  /* DMA Target Address Register Channel 15 */
#define DCMD15		__DMA(0x2fc)  /* DMA Command Address Register Channel 15 */

#define DDADR(x)	__DMA2(0x200, (x) << 4)
#define DSADR(x)	__DMA2(0x204, (x) << 4)
#define DTADR(x)	__DMA2(0x208, (x) << 4)
#define DCMD(x)		__DMA2(0x20c, (x) << 4)

#define DDADR_DESCADDR	0xfffffff0	/* Address of next descriptor (mask) */
#define DDADR_STOP	(1 << 0)	/* Stop (read / write) */

#define DCMD_INCSRCADDR	(1 << 31)	/* Source Address Increment Setting. */
#define DCMD_INCTRGADDR	(1 << 30)	/* Target Address Increment Setting. */
#define DCMD_FLOWSRC	(1 << 29)	/* Flow Control by the source. */
#define DCMD_FLOWTRG	(1 << 28)	/* Flow Control by the target. */
#define DCMD_STARTIRQEN	(1 << 22)	/* Start Interrupt Enable */
#define DCMD_ENDIRQEN	(1 << 21)	/* End Interrupt Enable */
#define DCMD_ENDIAN	(1 << 18)	/* Device Endian-ness. */
#define DCMD_BURST8	(1 << 16)	/* 8 byte burst */
#define DCMD_BURST16	(2 << 16)	/* 16 byte burst */
#define DCMD_BURST32	(3 << 16)	/* 32 byte burst */
#define DCMD_WIDTH1	(1 << 14)	/* 1 byte width */
#define DCMD_WIDTH2	(2 << 14)	/* 2 byte width (HalfWord) */
#define DCMD_WIDTH4	(3 << 14)	/* 4 byte width (Word) */
#define DCMD_WIDTH8	(0 << 14)	/* 8 byte width */
#define DCMD_OVERREAD	(1 << 13)	/* Over read data */
#define DCMD_LENGTH	0x01fff		/* length mask (max = 8K - 1) */

#define DWBR(x)		__DMA2(0x4000, (x) << 3)
#define DWBR_ADDR(x)	__ADDR2(0x4000, (x) << 3)
#define DWCR(x)		__DMA2(0x4004, (x) << 3)
#define DWCR_ADDR(x)	__ADDR2(0x4004, (x) << 3)

#define DWCR_WINEN	(1 << 0)
#define DWCR_WRBL	(1 << 1)
#define DWCR_TARG_OFFS	4
#define DWCR_TARG_MASK	(0xF << DWCR_TARG_OFFS)
#define DWCR_ATTR_OFFS	8
#define DWCR_ATTR_MASK	(0xFF << DWCR_ATTR_OFFS)
#define DWCR_SIZE_OFFS	16
#define DWCR_SIZE_MASK	(0xFFFF << DWCR_SIZE_OFFS)

#endif
