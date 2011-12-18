# kernel sources 
TOPDIR  := $(shell /bin/pwd)

ifndef KSRC
KSRC  := $(TOPDIR)/../..
endif

include $(KSRC)/.config

CFLAGS  := $(EXTRA_CFLAGS)
CFLAGS  += -I$(KSRC)/arch/arm/plat-orion/mv_hal_drivers/mv_drivers_lsp/mv_cesa

ifndef CROSS_COMPILE
CROSS_COMPILE := arm-none-linux-gnueabi-
endif

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld

TARGETS=mv_cesa_tool

all : Makefile $(TARGETS)
	
mv_cesa_tool:mv_cesa_tool.c
	$(CC) $(CFLAGS) mv_cesa_tool.c -o mv_cesa_tool 
clean:
	rm -f $(TARGETS) *.o

