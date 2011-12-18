# kernel sources 
TOPDIR  := $(shell /bin/pwd)

ifndef KSRC
KSRC  := $(TOPDIR)/../..
endif

#include $(KSRC)/.config

ifndef CROSS_COMPILE
CROSS_COMPILE := arm-none-linux-gnueabi-
endif

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld

TARGETS=devmem2

all : Makefile $(TARGETS)
	
devmem2: devmem2.c
	$(CC) $(CFLAGS) devmem2.c -o devmem2

clean:
	rm -f $(TARGETS) *.o

