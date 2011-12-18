obj-y				+= clock.o common.o addr-map.o irq.o pcie.o mpp.o \
				sdhci_cam_mbus.o
obj-$(CONFIG_MACH_DOVE_RD_AVNG)	+= dove-rd-avng-setup.o
obj-$(CONFIG_MACH_DOVE_RD_AVNG_V3)	+= dove-rd-avng-v3-setup.o
obj-$(CONFIG_MACH_DOVE_RD)	+= dove-rd-setup.o dove-front-panel-common.o
obj-$(CONFIG_MACH_DOVE_DB)	+= dove-db-setup.o dove-front-panel-common.o
obj-$(CONFIG_MACH_DOVE_DB_B)	+= dove-db-setup.o dove-front-panel-common.o
obj-$(CONFIG_MACH_DOVE_VIDEOPLUG)	+= dove-videoplug-setup.o
obj-$(CONFIG_I2C_MV64XXX_PORT_EXPANDER)	+= twsi.o
obj-$(CONFIG_DOVE_PM_SUPPORT)	+= pm.o
obj-$(CONFIG_CPU_FREQ_DOVE)	+= cpufreq.o
obj-$(CONFIG_HIBERNATION)	+= cpu.o swsusp.o
obj-$(CONFIG_SENSORS_DOVE)	+= hwmon.o
obj-$(CONFIG_FB_DOVE_CLCD)	+= clcd.o
obj-$(CONFIG_FB_DOVE_CLCD1)	+= clcd.o
obj-$(CONFIG_MV_INCLUDE_MPP_SYSFS)	+= avng_mpp.o
obj-$(CONFIG_CPU_IDLE)		+= cpuidle.o
obj-$(CONFIG_PROC_FS)		+= dump_cp15_regs.o
obj-$(CONFIG_MV_EXT_VGA_CLOCK)	+= idt5v49ee503.o

include $(srctree)/$(subst ",,$(CONFIG_MV_HAL_RULES_PATH))

obj-y           += mv_hal_support/
obj-m		+= gc600_driver_dove/
