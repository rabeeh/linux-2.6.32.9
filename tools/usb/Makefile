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

TARGETS=testusb

all : Makefile $(TARGETS)
	
testusb: testusb.c
	$(CC) $(CFLAGS) -static testusb.c -o testusb -lpthread 

clean:
	rm -f $(TARGETS) *.o

