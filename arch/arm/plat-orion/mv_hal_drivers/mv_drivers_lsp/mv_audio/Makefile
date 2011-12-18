#
# Makefile for the Marvell Audio ALSA Device Driver
#

ifdef CONFIG_MV_HAL_RULES_PATH
include $(srctree)/$(subst ",,$(CONFIG_MV_HAL_RULES_PATH))
endif

obj-$(CONFIG_SND_MRVL_AUDIO) += snd-mv88fx.o
snd-mv88fx-objs		     += mv88fx-pcm.o  cs42l51-hal.o mv88fx-hal.o

