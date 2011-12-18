#
# Makefile for the Marvell Gigabit Ethernet driver
#

ifdef CONFIG_MV_HAL_RULES_PATH
include $(srctree)/$(subst ",,$(CONFIG_MV_HAL_RULES_PATH))
endif

obj-$(CONFIG_MV_ETHERNET) += mv_netdev_plat.o mv_ethernet.o mv_eth_addr_dec.o
obj-$(CONFIG_MV_ETH_PROC) += mv_eth_proc.o
obj-$(CONFIG_MV_GATEWAY) +=  mv_gateway.o
obj-$(CONFIG_MV_GTW_IGMP) += mv_gtw_igmp.o

