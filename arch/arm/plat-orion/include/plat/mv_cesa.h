/*
 * Marvell Cesa platform device data definition file.
 */

#ifndef __ASM_PLAT_ORION_MV_CESA_H
#define __ASM_PLAT_ORION_MV_CESA_H

#include <linux/dmaengine.h>
#include <linux/mbus.h>


struct mbus_dram_target_info;

struct mv_cesa_addr_dec_platform_data {
	struct mbus_dram_target_info	*dram;
};

#endif
