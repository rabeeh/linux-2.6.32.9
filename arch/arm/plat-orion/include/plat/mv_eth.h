/*
 * Marvell Ethernet platform device data definition file.
 */

#ifndef __ASM_PLAT_ORION_MV_ETH_H
#define __ASM_PLAT_ORION_MV_ETH_H

#include <linux/dmaengine.h>
#include <linux/mbus.h>

#define MV_ETH_ADDR_DEC_NAME	"mv_eth_addr_decode"

struct mbus_dram_target_info;

struct mv_eth_addr_dec_platform_data {
	struct mbus_dram_target_info	*dram;
};

#endif
