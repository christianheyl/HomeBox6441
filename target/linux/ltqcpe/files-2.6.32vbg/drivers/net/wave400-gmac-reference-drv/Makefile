
###################################################
# Makefile for the Synopsys GMAC driver
###################################################

obj-$(CONFIG_SYNOPGMACHOST_PCI) += synop_GMAC_ether.o
synop_GMAC_ether-objs := synopGMAC_ahb_bus_interface.o synopGMAC_Host.o synopGMAC_plat.o synopGMAC_network_interface.o synopGMAC_Dev.o cpu_stat.o cpustat_osdep.o

#EXTRA_CFLAGS += -DENH_DESC -DENH_DESC_8W -DIPC_OFFLOAD
#EXTRA_CFLAGS += -DENH_DESC -DIPC_OFFLOAD

# for cpu statistics:
#EXTRA_CFLAGS += -DENH_DESC -DIPC_OFFLOAD -DDO_CPU_STAT -DCONFIG_AHB_INTERFACE -DCONFIG_ARCH_NPU
#for register dump:
#EXTRA_CFLAGS += -DENH_DESC -DIPC_OFFLOAD -DCONFIG_AHB_INTERFACE -DCONFIG_ARCH_NPU -DWAVE400_DUMP_REGS

# normal:
EXTRA_CFLAGS += -DENH_DESC -DIPC_OFFLOAD -DCONFIG_AHB_INTERFACE -DCONFIG_ARCH_NPU
