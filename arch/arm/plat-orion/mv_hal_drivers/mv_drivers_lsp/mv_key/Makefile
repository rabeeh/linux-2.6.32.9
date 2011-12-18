#
# Makefile for the Marvell Key
#
ifdef CONFIG_MV_HAL_RULES_PATH
include $(srctree)/$(subst ",,$(CONFIG_MV_HAL_RULES_PATH))
endif

obj-$(CONFIG_MV_TOUCH_KEY)	+= mv_touch_key.o
obj-$(CONFIG_MV_TOUCH_SLIDER)	+= mv_touch_slider.o
obj-$(CONFIG_MV_GPIO_KEY)	+= mv_gpio_key.o
obj-$(CONFIG_MV_EC_KEY)		+= mv_ec_key.o

