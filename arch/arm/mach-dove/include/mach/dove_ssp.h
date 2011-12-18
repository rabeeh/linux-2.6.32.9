#ifndef __ASM_ARCH_DOVE_SSP_H
#define __ASM_ARCH_DOVE_SSP_H


struct dove_ssp_platform_data {
	u8 use_dma;
	u8 use_loopback;
	u32 dss;
	u32 scr;
	u32 frf;
	u32 rft;
	u32 tft;
};
#endif /* __ASM_ARCH_DOVE_SSP_H */
