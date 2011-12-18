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

#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/mbus.h>

#define SDHCI_CAM_MBUS_NAME   "sdhci_cam_mbus"
/*
 * driver for the MBUS Address decoding of the sdhci and camera controllers
 */


#define WINDOW_CTRL(i)          (0x30 + ((i) << 4))
#define WINDOW_BASE(i)          (0x34 + ((i) << 4))

static void sdhci_cam_conf_mbus_windows(void __iomem *base,
					struct mbus_dram_target_info *dram)
{
	int i;
	
	for (i = 0; i < 4; i++) {
		writel(0, base + WINDOW_CTRL(i));
		writel(0, base + WINDOW_BASE(i));
	}
	
	for (i = 0; i < dram->num_cs; i++) {
		struct mbus_dram_window *cs = dram->cs + i;
		
		BUG_ON(i >= 4);
		writel(((cs->size - 1) & 0xffff0000) |
		       (cs->mbus_attr << 8) |
		       (dram->mbus_dram_target_id << 4) | 1,
		       base + WINDOW_CTRL(i));
                writel(cs->base, base + WINDOW_BASE(i));
	}
}

static int sdhci_cam_mbus_probe(struct platform_device *pdev)
{
	struct mbus_dram_target_info    *dram = pdev->dev.platform_data;
	void __iomem *base;
        struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        if (res == NULL)
                return -ENOMEM;

        base = devm_ioremap(&pdev->dev, res->start, res->end - res->start + 1);
        if (base == NULL)
		return -ENOMEM;

	platform_set_drvdata(pdev, base);
        /*
         * (Re-)program MBUS remapping windows if we are asked to.
         */
        if (dram != NULL)
                sdhci_cam_conf_mbus_windows(base, dram);

	return 0;
}

static int sdhci_cam_mbus_remove(struct platform_device *pdev)
{
	// Nothing to do meanwhile...
	return 0;
}

#ifdef CONFIG_PM
static int sdhci_cam_mbus_suspend(struct platform_device *pdev,
				      pm_message_t state)
{
	return 0;
}
static int sdhci_cam_mbus_resume(struct platform_device *pdev)
{
	struct mbus_dram_target_info    *dram = pdev->dev.platform_data;
	void __iomem *base = platform_get_drvdata(pdev);

        /*
         * (Re-)program MBUS remapping windows if we are asked to.
         */
        if (dram != NULL)
                sdhci_cam_conf_mbus_windows(base, dram);
	return 0;
}

#else
#define sdhci_cam_mbus_suspend	NULL
#define sdhci_cam_mbus_resume	NULL
#endif

static struct platform_driver sdhci_cam_mbus_driver = {
	.probe		= sdhci_cam_mbus_probe,
	.remove		= sdhci_cam_mbus_remove,
	.suspend	= sdhci_cam_mbus_suspend,
	.resume		= sdhci_cam_mbus_resume,
	.driver = {
		.name	= SDHCI_CAM_MBUS_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init sdhci_cam_mbus_init_module(void)
{
	int rc;

	rc = platform_driver_register(&sdhci_cam_mbus_driver);

	return rc;
}
module_init(sdhci_cam_mbus_init_module);

static void __exit sdhci_cam_mbus_cleanup_module(void)
{
	platform_driver_unregister(&sdhci_cam_mbus_driver);
}
module_exit( sdhci_cam_mbus_cleanup_module);

MODULE_ALIAS("platform:" SDHCI_CAM_MBUS_NAME);

