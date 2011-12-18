#
# Makefile for the Marvell CESA driver
#

ifeq ($(CONFIG_ARCH_DOVE),y)
include $(srctree)/$(subst ",,$(CONFIG_MV_HAL_RULES_PATH))
else
#include $(MV_RULES_PATH)/mvRules.mk
endif

ifeq ($(CONFIG_MV_CESA_OCF),y)
EXTRA_CFLAGS    += -I$(srctree)/crypto/ocf
endif

obj-$(CONFIG_MV_CESA_TOOL) += cesa_dev.o
obj-$(CONFIG_MV_CESA_OCF)  += cesa_ocf_drv.o

