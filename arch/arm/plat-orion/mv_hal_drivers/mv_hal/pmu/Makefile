include $(srctree)/$(subst ",,$(CONFIG_MV_HAL_RULES_PATH))
# Objects list
obj-$(CONFIG_MV_INCLUDE_PMU) += mvPmu.o
obj-$(CONFIG_MV_INCLUDE_PMU) += mvPmu_asm.o
obj-$(CONFIG_MV_INCLUDE_PMU) += mvSram.o

AFLAGS_mvPmu_asm.o := -I$(srctree)/arch/arm/plat-orion/mv_hal_drivers/linux_oss -I$(srctree)/arch/arm/plat-orion/mv_hal_drivers/common -I$(srctree)/arch/arm/plat-orion/mv_hal_drivers/mv_hal -I$(srctree)/arch/arm/plat-orion/mv_hal_drivers/mv_hal/sata/CoreDriver/  -I$(srctree)/arch/arm/mach-dove/mv_hal_support/dove_family -I$(srctree)/arch/arm/mach-dove/mv_hal_support -I$(srctree)/arch/arm/mach-dove/mv_hal_support/config -I$(srctree)/arch/arm/mach-dove/mv_hal_support/mv_hal_sys -DMV_LINUX -DMV_CPU_LE -DMV_ARM -DLINUX
