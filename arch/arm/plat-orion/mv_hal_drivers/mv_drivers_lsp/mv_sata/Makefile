#
# Makefile for the Marvell egiga ethernet driver
#

ifdef CONFIG_MV_HAL_RULES_PATH
include $(srctree)/$(subst ",,$(CONFIG_MV_HAL_RULES_PATH))
endif

IAL_OBJS        := mvLinuxIalLib.o mvLinuxIalHt.o mvLinuxIalOs.o mvIALCommon.o mvIALCommonUtils.o mvLinuxIalSmart.o
SAL_OBJS        := mvScsiAtaLayer.o

obj-$(CONFIG_SCSI_MVSATA) := mvsata.o
#drivers-$(CONFIG_SCSI_MVSATA) := mvsata.o
mvsata-y        := $(IAL_OBJS) $(SAL_OBJS)

INCLUDE_DIRS    := -Idrivers/scsi
EXTRA_CFLAGS += -DLINUX -D__LINUX__ $(INCLUDE_DIRS)
