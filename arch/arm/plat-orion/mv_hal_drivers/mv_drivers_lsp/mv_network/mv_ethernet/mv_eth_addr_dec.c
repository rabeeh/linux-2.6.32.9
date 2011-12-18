/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell 
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.


********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or 
modify this File in accordance with the terms and conditions of the General 
Public License Version 2, June 1991 (the "GPL License"), a copy of which is 
available along with the File in the license.txt file or by writing to the Free 
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or 
on the worldwide web at http://www.gnu.org/licenses/gpl.txt. 

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED 
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY 
DISCLAIMED.  The GPL License provides additional details about this warranty 
disclaimer.
*******************************************************************************/



#include "mvCommon.h"  /* Should be included before mvSysHwConfig */
#include "mvOs.h"
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <plat/mv_eth.h>

#include "eth/mvEth.h"
#include "eth-phy/mvEthPhy.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)

/*
** Address decoding driver.
*/

static void mv_eth_set_port_addr_dec(int port, struct mbus_dram_target_info *dram)
{
	int i;
	u32 win_enable;
	u32 win_protect;

	for (i = 0; i < ETH_MAX_DECODE_WIN; i++) {
		MV_REG_WRITE(ETH_WIN_BASE_REG(port,i),0);
		MV_REG_WRITE(ETH_WIN_SIZE_REG(port,i),0);
		if (i < ETH_MAX_HIGH_ADDR_REMAP_WIN)
			MV_REG_WRITE(ETH_WIN_REMAP_REG(port,i),0);
	}

	win_enable = 0x3f;
	win_protect = 0;

	for (i = 0; i < dram->num_cs; i++) {
		struct mbus_dram_window *cs = dram->cs + i;
		
		MV_REG_WRITE(ETH_WIN_BASE_REG(port,i),(cs->base & 0xffff0000) |
				(cs->mbus_attr << 8) | dram->mbus_dram_target_id);
		MV_REG_WRITE(ETH_WIN_SIZE_REG(port,i),(cs->size - 1) & 0xffff0000);

		win_enable &= ~(1 << i);
		win_protect |= 3 << (2 * i);
	}
	MV_REG_WRITE(ETH_BASE_ADDR_ENABLE_REG(port), win_enable);

	return;
}

static int mv_eth_addr_dec_probe(struct platform_device *pdev)
{
	struct mv_eth_addr_dec_platform_data *pd = pdev->dev.platform_data;
	int port = pdev->id;

	mv_eth_set_port_addr_dec(port, pd->dram);

	return 0;
}

static int mv_eth_addr_dec_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver mv_eth_addr_dec_driver = {
	.probe		= mv_eth_addr_dec_probe,
	.remove		= mv_eth_addr_dec_remove,
	.driver = {
		.name	= MV_ETH_ADDR_DEC_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init mv_eth_addr_dec_init_module(void)
{
	int rc;

	rc = platform_driver_register(&mv_eth_addr_dec_driver);

	return rc;
}
module_init(mv_eth_addr_dec_init_module);

static void __exit mv_eth_addr_dec_cleanup_module(void)
{
	platform_driver_unregister(&mv_eth_addr_dec_driver);
}
module_exit(mv_eth_addr_dec_cleanup_module);

MODULE_ALIAS("platform:" MV_ETH_ADDR_DEC_NAME);

#endif /* 2.6.26 */
