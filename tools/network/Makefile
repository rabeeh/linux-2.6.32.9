# kernel sources 
TOPDIR  := $(shell /bin/pwd)

ifndef KSRC
KSRC  := $(TOPDIR)/../..
endif

have_config := $(wildcard $(KSRC)/.config)

ifneq ($(strip $(have_config)),) 
include $(KSRC)/.config
endif

CFLAGS  := $(EXTRA_CFLAGS)
CFLAGS  += -I$(KSRC)/arch/arm/plat-orion/mv_hal_drivers/mv_drivers_lsp/mv_network/mv_ethernet/

ifndef CROSS_COMPILE
CROSS_COMPILE := arm-none-linux-gnueabi-
endif

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld

TARGETS=mv_eth_tool phy_reg

ifeq ($(CONFIG_MV_GATEWAY),y)
CFLAGS += -DCONFIG_MV_GATEWAY
endif

ifeq ($(CONFIG_MV_ETHERNET),y)
CFLAGS += -DCONFIG_MV_ETHERNET
endif

all : Makefile $(TARGETS)
	
mv_eth_tool: mv_eth_tool.c
	$(CC) $(CFLAGS) mv_eth_tool.c -o mv_eth_tool
phy_reg: phy_reg.c
	 $(CC)  phy_reg.c -o phy_reg

clean:
	rm -f $(TARGETS) *.o

