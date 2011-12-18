#
# Makefile for the Marvell USB device controller
#

ifdef CONFIG_MV_HAL_RULES_PATH
include $(srctree)/$(subst ",,$(CONFIG_MV_HAL_RULES_PATH))
endif

USB_DIR := ../mv_hal/usb/
USB_PATH := $(srctree)/arch/arm/plat-orion/mv_hal_drivers/mv_hal/usb

USB_DEV_DIR := $(USB_DIR)/device

EXTRA_CFLAGS  += -I$(USB_PATH)/api -I$(USB_PATH)/common -I$(USB_PATH)/device

OBJS        := mv_udc_main.o ../$(USB_DEV_DIR)/mvUsbDevCh9.o ../$(USB_DEV_DIR)/mvUsbDevMain.o 	\
               ../$(USB_DEV_DIR)/mvUsbDevRecv.o ../$(USB_DEV_DIR)/mvUsbDevSend.o ../$(USB_DEV_DIR)/mvUsbDevUtl.o 	\
	       ../$(USB_DEV_DIR)/mvUsbHsDevUtl.o  ../$(USB_DEV_DIR)/mvUsbHsDevMain.o ../$(USB_DEV_DIR)/mvUsbHsDevCncl.o

mv_udc-objs := $(OBJS)
obj-$(CONFIG_USB_MRVL) := mv_udc.o

