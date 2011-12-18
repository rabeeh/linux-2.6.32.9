#*******************************************************************************
# Marvell GPL License Option
#
# If you received this File from Marvell, you may opt to use, redistribute and/or 
# modify this File in accordance with the terms and conditions of the General 
# Public License Version 2, June 1991 (the "GPL License"), a copy of which is 
# available along with the File in the license.txt file or by writing to the Free 
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or 
# on the worldwide web at http://www.gnu.org/licenses/gpl.txt. 
#
# THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED 
# WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY 
# DISCLAIMED.  The GPL License provides additional details about this warranty 
# disclaimer.
#*******************************************************************************/
include $(srctree)/$(subst ",,$(CONFIG_MV_HAL_RULES_PATH))

# Objects list

SUBMAKE=@$(SHELL) $(srctree)/$(src)/mv_submake.sh

COMMON_OBJS		= mvDebug.o mvCommon.o mvStack.o
OSSERVICES_OBJS		= mvOs.o
AUDIO_HAL_OBJS		= mvAudio.o
DRAM_HAL_OBJS		= mvDramTmng.o
PMU_HAL_OBJS		= mvPmu.o mvPmu_asm.o mvSram.o
USB_HAL_OBJS		= mvUsb.o
ETH_HAL_OBJS		= mvEth.o mvEthDebug.o
ETHPHY_HAL_OBJS		= mvEthPhy.o
CESA_HAL_OBJS		= mvCesa.o mvCesaDebug.o mvMD5.o mvSHA1.o
CESA_TEST_HAL_OBJS	= mvCesaTest.o
CESA_AES_HAL_OBJS	= mvAesAlg.o mvAesApi.o
NFC_HAL_OBJS		= mvNfc.o
PDMA_HAL_OBJS		= mvPdma.o
SATA_HAL_OBJS		= mvSata.o mvStorageDev.o mvLog.o

PHONY += dummy dummy_end FORCE

mv_submake: FORCE
	$(SUBMAKE) $(COMMON_DIR) $(COMMON_OBJS)
	$(SUBMAKE) $(OSSERV_DIR) $(OSSERVICES_OBJS)
	$(SUBMAKE) $(HAL_AUDIO_DIR) cond CONFIG_MV_INCLUDE_AUDIO $(AUDIO_HAL_OBJS)
	$(SUBMAKE) $(HAL_PMU_DIR) cond CONFIG_MV_INCLUDE_PMU $(PMU_HAL_OBJS) opt "AFLAGS_mvPmu_asm.o := $(EXTRA_INCLUDE) $(MV_DEFINE)"
	$(SUBMAKE) $(HAL_DDRMC_DIR) cond CONFIG_MV_DDRMMC $(DRAM_HAL_OBJS)
	$(SUBMAKE) $(HAL_USB_DIR) cond CONFIG_MV_INCLUDE_USB $(USB_HAL_OBJS)
	$(SUBMAKE) $(HAL_ETH_DIR) cond CONFIG_MV_INCLUDE_GIG_ETH $(ETH_HAL_OBJS)
	$(SUBMAKE) $(HAL_ETHPHY_DIR) cond CONFIG_MV_INCLUDE_GIG_ETH $(ETHPHY_HAL_OBJS)
	$(SUBMAKE) $(HAL_CESA_DIR) cond CONFIG_MV_INCLUDE_CESA $(CESA_HAL_OBJS) cond CONFIG_MV_CESA_TEST $(CESA_TEST_HAL_OBJS)
	$(SUBMAKE) $(HAL_CESA_AES_DIR) cond CONFIG_MV_INCLUDE_CESA $(CESA_AES_HAL_OBJS)
	$(SUBMAKE) $(HAL_NFC_DIR) cond CONFIG_MV_INCLUDE_NFC $(NFC_HAL_OBJS)
	$(SUBMAKE) $(HAL_PDMA_DIR) cond CONFIG_MV_INCLUDE_NFC $(PDMA_HAL_OBJS)
	$(SUBMAKE) $(HAL_SATA_DIR) cond CONFIG_SCSI_MVSATA $(SATA_HAL_OBJS)

mv_submake_end: FORCE
	@rm $(COMMON_DIR)/Makefile
	@rm $(OSSERV_DIR)/Makefile
	@rm $(HAL_AUDIO_DIR)/Makefile
	@rm $(HAL_PMU_DIR)/Makefile
	@rm $(HAL_DDRMC_DIR)/Makefile
	@rm $(HAL_USB_DIR)/Makefile
	@rm $(HAL_ETH_DIR)/Makefile
	@rm $(HAL_ETHPHY_DIR)/Makefile
	@rm $(HAL_CESA_DIR)/Makefile
	@rm $(HAL_CESA_AES_DIR)/Makefile
	@rm $(HAL_NFC_DIR)/Makefile
	@rm $(HAL_PDMA_DIR)/Makefile
	@rm $(HAL_SATA_DIR)/Makefile

$(obj)/dummy.o: mv_submake

$(obj)/dummy_end.o: mv_submake_end

ifneq ($(KBUILD_BUILTIN),)
obj-y +=  										\
		common/ linux_oss/ mv_hal/audio/ mv_hal/pmu/ mv_hal/ddr/ddrmc/		\
		mv_hal/usb/ mv_hal/eth/gbe/ mv_hal/eth-phy/ mv_hal/cesa/		\
		mv_hal/cesa/AES/ mv_hal/nfc/ mv_hal/pdma/ mv_hal/sata/CoreDriver/
endif
# core drivers part

obj-$(CONFIG_MV_USE_XOR_ENGINE) 	+= mv_drivers_lsp/mv_xor/
obj-$(CONFIG_MV_CESA) 			+= mv_drivers_lsp/mv_cesa/

# The rest of the drivers are compiled through the driver dir directly.

obj-y   				+= mv_drivers_lsp/mv_key/


# Opens:
#	Flags (PMU)
#	Conditional compilation

