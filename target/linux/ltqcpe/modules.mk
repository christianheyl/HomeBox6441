
LTQCPE_MENU:=Lantiq
SUBTARGET_SUFFIX:=$(shell echo $(subst .,_,$(subst -,_,$(subst /,_,$(SUBTARGET)))) | cut -d_ -f2-)
include $(INCLUDE_DIR)/netfilter.mk

define KernelPackage/ltqcpe_nand
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=LTQ NAND Support
 DEPENDS:=@TARGET_ltqcpe
 KCONFIG:= \
	CONFIG_MTD_NAND=y \
	CONFIG_MTD_IFX_NAND=y \
	CONFIG_MTD_IFX_NOR=n \
	CONFIG_MTD_ROOTFS_SPLIT=n
endef
# CONFIG_MTD_NAND_VERIFY_WRITE

define KernelPackage/ltqcpe_nand/description
  Kernel Support NAND 
endef

$(eval $(call KernelPackage,ltqcpe_nand))

define KernelPackage/ltqcpe_mlcnand
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=IFX MLC NAND Support
 DEPENDS:=@TARGET_ltqcpe
 KCONFIG:= \
	CONFIG_MTD_NAND=y \
	CONFIG_MTD_IFX_NAND_LATCH_ENABLE=y \
	CONFIG_MTD_IFX_MLCNAND=y \
	CONFIG_MTD_MLC_SPARE_MODE=y \
	CONFIG_MTD_MLC_ECC_4B_MODE=y \
	CONFIG_MTD_MLC_SAFE_MODE=y \
	CONFIG_MTD_ROOTFS_SPLIT=n
endef
# CONFIG_MTD_NAND_VERIFY_WRITE enabled for some nand chips to do a stable write.
# Can be removed if performance is impacted.

define KernelPackage/ltqcpe_mlcnand/description
  Kernel Support for MLC NAND 
endef

$(eval $(call KernelPackage,ltqcpe_mlcnand))



define KernelPackage/ltqcpe_imq
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=LTQ Support for IMQ 
 KCONFIG:= \
	CONFIG_IMQ \
	CONFIG_IMQ_BEHAVIOR_AA=y \
	CONFIG_IMQ_NUM_DEVS=3 \
	CONFIG_NETFILTER_XT_TARGET_IMQ=y
FILES:=$(LINUX_DIR)/drivers/net/imq.$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/ltqcpe_imq/description
  Kernel Support IMQ
endef

$(eval $(call KernelPackage,ltqcpe_imq))

define KernelPackage/ltqcpe_qos
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=LTQ Support for Qos
 #DEPENDS:= +ebtables +ebtables-utils 
 KCONFIG:= \
	CONFIG_IFX_IPQOS=y \
	CONFIG_NET_SCHED=y \
	CONFIG_NET_SCH_FIFO=y \
	CONFIG_NET_SCH_CLK_GETTIMEOFDAY=y \
	CONFIG_NET_SCH_CBQ=y \
	CONFIG_NET_SCH_HTB=y \
	CONFIG_NET_SCH_RED=y \
	CONFIG_NET_SCH_DSMARK=y \
	CONFIG_NET_CLS=y \
	CONFIG_NET_CLS_FW=y \
	CONFIG_NET_CLS_U32=y \
	CONFIG_NETFILTER_XT_MATCH_U32=y \
	CONFIG_LTQ_ALG_QOS=y \
	CONFIG_IP_NF_TARGET_TOS=y \
	CONFIG_NETFILTER_XT_TARGET_DSCP=y \
	CONFIG_BRIDGE_NETFILTER=n \
	CONFIG_NET_SCH_ESFQ=n \
	CONFIG_NET_SCH_TEQL=n \
	CONFIG_NET_SCH_HFSC=n

endef

define KernelPackage/ltqcpe_qos/description
  Kernel Support for QoS. This package enables classifier and queuing disciplines (HTB, CBQ, FIFO etc) in Kernel configuration.
endef

$(eval $(call KernelPackage,ltqcpe_qos))

define KernelPackage/ltqcpe_pci
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=LTQ Supoort PCI
 KCONFIG:= \
	CONFIG_PCI=y \
	CONFIG_IFX_PCI=y \
	CONFIG_IFX_PCI_HW_SWAP=y \
	CONFIG_IFX_PCI_INTERNAL_CLK_SRC_33=y
endef

define KernelPackage/ltqcpe_pci/description
  Kernel Support LTQ PCI
endef

$(eval $(call KernelPackage,ltqcpe_pci))


define KernelPackage/ltqcpe_switchapi
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=LTQ Switch API Support
 DEPENDS:=@TARGET_ltqcpe_platform_ar9||TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_danube||TARGET_ltqcpe_platform_amazon_se
 KCONFIG:= \
	CONFIG_IFX_ETHSW_API=y \
	CONFIG_EXTERNAL_T3G=y
endef

define KernelPackage/ltqcpe_switchapi/description
 Kernel support for LTQ Switch API
endef

$(eval $(call KernelPackage,ltqcpe_switchapi))

define KernelPackage/ltqcpe_vr9_7port
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=VR9 or AR10 7PORT switch support
 DEPENDS:=@TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_ar10
 KCONFIG:= \
	CONFIG_IFX_7PORT_SWITCH=y
endef

define KernelPackage/ltqcpe_vr9_7port/description
Kernel support for VR9 7 port switch
endef

$(eval $(call KernelPackage,ltqcpe_vr9_7port))

define KernelPackage/ltqcpe_eth_drv_builtin
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:= LTQ Ethernet Driver (Built-in Support)
 DEPENDS:= @!PACKAGE_kmod-ltqcpe_eth_drv_mod
 KCONFIG:= \
	CONFIG_IFX_7PORT_SWITCH=y \
	CONFIG_IFX_3PORT_SWITCH=y \
	CONFIG_IFX_ETOP_ETHERNET=y
endef

define KernelPackage/ltqcpe_eth_drv_builtin/description
 Kernel support for LTQ Ethernet Driver (Built-in Support)
endef

$(eval $(call KernelPackage,ltqcpe_eth_drv_builtin))

define KernelPackage/ltqcpe_eth_drv_mod
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:= LTQ Ethernet Driver (Loadable Module Support)
 DEPENDS:=@!PACKAGE_kmod-fs-nfs-root
 KCONFIG:= \
	CONFIG_IFX_7PORT_SWITCH=m \
	CONFIG_IFX_3PORT_SWITCH=m \
	CONFIG_IFX_ETOP_ETHERNET=m
  FILES:=$(LINUX_DIR)/drivers/net/ifxmips_eth_drv.$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/ltqcpe_eth_drv_mod/description
 Kernel support for LTQ Ethernet Driver (Loadable Module Support)
endef

$(eval $(call KernelPackage,ltqcpe_eth_drv_mod))


define KernelPackage/ltqcpe_dma_descr
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=DMA DESCRITPTOR NUMBER
 DEPENDS:=@!PACKAGE_kmod-ltqcpe_dma_descr_atheros
 KCONFIG:= \
	CONFIG_IFX_EXTRA_CFG=y \
	CONFIG_IFX_DMA_DESCRIPTOR_NUMBER=255
endef
# Original CONFIG_IFX_DMA_DESCRIPTOR_NUMBER=64

define KernelPackage/ltqcpe_dma_descr/description
 Configure DMA DESCRIPTOR NUMBER
endef

$(eval $(call KernelPackage,ltqcpe_dma_descr))

define KernelPackage/ltqcpe_dma_descr_atheros
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=DMA DESCRITPTOR NUMBER
 DEPENDS:=@PACKAGE_atheros
 KCONFIG:= \
	CONFIG_IFX_EXTRA_CFG=y \
	CONFIG_IFX_DMA_DESCRIPTOR_NUMBER=255
endef

define KernelPackage/ltqcpe_dma_descr_atheros/description
 Configure DMA DESCRIPTOR NUMBER
endef

$(eval $(call KernelPackage,ltqcpe_dma_descr_atheros))

define KernelPackage/ltqcpe_ase_optimization
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=amazon_se ram optimizations
 DEPENDS:=@TARGET_ltqcpe_platform_amazon_se
 KCONFIG:= \
	CONFIG_IFX_A2_DESC_NUM=y
endef

define KernelPackage/ltqcpe_ase_optimization/description
 Configure descriptors
endef

$(eval $(call KernelPackage,ltqcpe_ase_optimization))

define KernelPackage/ltqcpe_spi
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=LTQ SPI Support
 DEPENDS:=@TARGET_ltqcpe
 KCONFIG:= \
	CONFIG_SPI=y \
	CONFIG_SPI_DEBUG=n \
	CONFIG_SPI_MASTER=y \
	CONFIG_SPI_BITBANG=n \
	CONFIG_IFX_SPI=y \
	CONFIG_IFX_SPI_DEBUG=n \
	CONFIG_IFX_SPI_ASYNCHRONOUS=y \
	CONFIG_IFX_SPI_EEPROM=y \
	CONFIG_IFX_SPI_AT45_FLASH=n 
endef

define KernelPackage/ltqcpe_spi/description
 Kernel support for LTQ SPI
endef

$(eval $(call KernelPackage,ltqcpe_spi))

define KernelPackage/ltqcpe_spi_flash
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=LTQ SPI flash Support
 DEPENDS:=@TARGET_ltqcpe
 KCONFIG:= \
	CONFIG_IFX_SPI_EEPROM=n \
	CONFIG_IFX_SPI_FLASH=y \
	CONFIG_MTD_ROOTFS_SPLIT=n \
	CONFIG_MTD_IFX_NOR=n
endef

define KernelPackage/ltqcpe_spi_flash/description
 Kernel support for LTQ SPI Flash
endef

$(eval $(call KernelPackage,ltqcpe_spi_flash))

define KernelPackage/ltqcpe_complex_mtd
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=LTQ Complex MTD Mappings Support
 DEPENDS:=@TARGET_ltqcpe
 KCONFIG:= \
 	CONFIG_MTD_COMPLEX_MAPPINGS=y
endef

define KernelPackage/ltqcpe_complex_mtd/description
 Kernel support for Complex MTD Mappings
endef

$(eval $(call KernelPackage,ltqcpe_complex_mtd))

define KernelPackage/ltqcpe_nor
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=LTQ NOR Support
 DEPENDS:=@TARGET_ltqcpe
 KCONFIG:= \
	CONFIG_MTD_IFX_NOR=y \
	CONFIG_MTD_ROOTFS_SPLIT=n
endef

define KernelPackage/ltqcpe_nor/description
 Kernel support for LTQ NOR
endef

$(eval $(call KernelPackage,ltqcpe_nor))


define KernelPackage/atm_stack
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=ATM stack
  DEPENDS:=@TARGET_ltqcpe
  KCONFIG:= \
	CONFIG_ATM=y \
	CONFIG_ATM_CLIP=y \
	CONFIG_ATM_DRIVERS=y \
	CONFIG_ATM_BR2684=y \
	CONFIG_ATM_MPOA=y \
	CONFIG_PPPOATM=y \
	CONFIG_ATM_LANE=y \
	CONFIG_IFX_OAM=y
endef

define KernelPackage/atm_stack/description
 Kernel built-in support for ATM stack
endef

$(eval $(call KernelPackage,atm_stack))


define KernelPackage/ltqcpe_atm_builtin
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=LTQ ATM Support (built-in)
  DEPENDS:=@!PACKAGE_kmod-ltqcpe_atm_mod @TARGET_ltqcpe +kmod-atm_stack
  KCONFIG:= \
	CONFIG_IFX_ATM=y \
	CONFIG_IFX_ATM_TASKLET=y
endef

define KernelPackage/ltqcpe_atm_builtin/description
 Kernel support for LTQ ATM - built-in
endef

$(eval $(call KernelPackage,ltqcpe_atm_builtin))

define KernelPackage/ltqcpe_atm_retx
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=LTQ ATM ReTransmission Support
  DEPENDS:=@PACKAGE_kmod-ltqcpe_atm_builtin||PACKAGE_kmod-ltqcpe_atm_mod
  KCONFIG:= \
	CONFIG_IFX_ATM_RETX=y
endef

define KernelPackage/ltqcpe_atm_retx/description
 Kernel support for LTQ ATM ReTransmission
endef

$(eval $(call KernelPackage,ltqcpe_atm_retx))

define KernelPackage/ltqcpe_atm_mod
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=LTQ ATM Support (pluggable module)
  DEPENDS:=@!PACKAGE_kmod-fs-nfs-root @TARGET_ltqcpe +kmod-atm_stack
  KCONFIG += \
	CONFIG_IFX_ATM \
	CONFIG_IFX_ATM_TASKLET=y
  FILES:=$(LINUX_DIR)/drivers/atm/ifxmips_atm/ifxmips_atm.$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/ltqcpe_atm_mod/description
 Kernel module for LTQ ATM - pluggable module
endef

$(eval $(call KernelPackage,ltqcpe_atm_mod))


define KernelPackage/ltqcpe_ptm_builtin
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=LTQ PTM support
  DEPENDS:=@!PACKAGE_kmod-ltqcpe_ptm_mod @TARGET_ltqcpe
  KCONFIG:= \
	CONFIG_ATM=y \
	CONFIG_ATM_DRIVERS=y \
	CONFIG_IFX_PTM=y
endef

define KernelPackage/ltqcpe_ptm_builtin/description
 Kernel built-in PTM support
endef

$(eval $(call KernelPackage,ltqcpe_ptm_builtin))

define KernelPackage/ltqcpe_ptm_mod
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=PTM support
  DEPENDS:=@!PACKAGE_kmod-fs-nfs-root @TARGET_ltqcpe
  KCONFIG:= \
	CONFIG_ATM=y \
	CONFIG_ATM_DRIVERS=y \
	CONFIG_IFX_PTM
  FILES:=$(LINUX_DIR)/drivers/net/ifxmips_ptm/ifxmips_ptm.$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/ltqcpe_ptm_mod/description
 Kernel module for PTM support
endef

$(eval $(call KernelPackage,ltqcpe_ptm_mod))


define KernelPackage/ltqcpe_ppa
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=LTQ PPA Support
 DEPENDS:=@TARGET_ltqcpe
 KCONFIG:= \
	CONFIG_IFX_PPA=y \
	CONFIG_IFX_PPA_API=y \
	CONFIG_IFX_PPA_API_DIRECTPATH=y \
	CONFIG_IFX_PPA_API_DIRECTPATH_BRIDGING=y \
	CONFIG_IFX_PPA_API_PROC=y \
	CONFIG_IFX_PPA_DATAPATH=y
endef

define KernelPackage/ltqcpe_ppa/description
 Kernel support for PPA acceleration
endef


define KernelPackage/ltqcpe_ppa_mod
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=LTQ PPA Support
 DEPENDS:=@TARGET_ltqcpe
 KCONFIG:= \
	CONFIG_IFX_PPA=y \
	CONFIG_IFX_PPA_API \
	CONFIG_IFX_PPA_API_DIRECTPATH=y \
	CONFIG_IFX_PPA_API_DIRECTPATH_BRIDGING=y \
	CONFIG_IFX_PPA_API_PROC \
	CONFIG_IFX_PPA_DATAPATH
 FILES:= \
	$(LINUX_DIR)/drivers/net/ifxmips_ppa/ppa_api/ifx_ppa_api.$(LINUX_KMOD_SUFFIX) \
	$(LINUX_DIR)/drivers/net/ifxmips_ppa/ppa_api/ifx_ppa_api_proc.$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/ltqcpe_ppa_mod/description
 Kernel support for PPA acceleration
endef

define KernelPackage/ltqcpe_ppa_a2_builtin
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=LTQ PPA built-in A2 mode
  DEPENDS:=@TARGET_ltqcpe_platform_amazon_se +kmod-atm_stack
  KCONFIG:= \
	CONFIG_IFX_ETOP_ETHERNET=n \
	CONFIG_IFX_A2_SE=y \
	CONFIG_IFX_EPHY_MODE=y \
	CONFIG_IFX_ATM_TASKLET=y \
	CONFIG_IFX_ETH_NAPI=y
endef

define KernelPackage/ltqcpe_ppa_a2_builtin/description
  PPA A2 Mode - built-in
endef

$(eval $(call KernelPackage,ltqcpe_ppa_a2_builtin))

define KernelPackage/ltqcpe_ppa_a2_mod
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=LTQ PPA module A2 mode
  DEPENDS:=@!PACKAGE_kmod-fs-nfs-root @TARGET_ltqcpe_platform_amazon_se +kmod-atm_stack
  KCONFIG:= \
	CONFIG_IFX_A2_SE \
	CONFIG_IFX_EPHY_MODE=y \
	CONFIG_IFX_ATM_TASKLET=y \
	CONFIG_IFX_ETH_NAPI=y
  FILES:=$(LINUX_DIR)/drivers/net/ifxmips_a2_se/ifxmips_a2_se.$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/ltqcpe_ppa_a2_mod/description
  PPA A2 Mode - mod
endef

$(eval $(call KernelPackage,ltqcpe_ppa_a2_mod))

define KernelPackage/ltqcpe_ppa_a5_builtin
  $(call KernelPackage/ltqcpe_ppa)
  TITLE+=(A5 mode - built-in)
  DEPENDS+=@!PACKAGE_kmod-ltqcpe_ppa_a5_mod @TARGET_ltqcpe_platform_ar9||TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_ar10||TARGET_ltqcpe_platform_vrx318 +kmod-atm_stack
  KCONFIG += \
	CONFIG_IFX_EXTRA_CFG=y \
	CONFIG_IFX_PPA_A5=y
endef

define KernelPackage/ltqcpe_ppa_a5_builtin/description
  $(call KernelPackage/ltqcpe_ppa/description)
  PPA A5 Mode - built-in
endef

$(eval $(call KernelPackage,ltqcpe_ppa_a5_builtin))


define KernelPackage/ltqcpe_ppa_a5_mod
  $(call KernelPackage/ltqcpe_ppa_mod)
  TITLE+=(A5 mode - pluggable module)
  DEPENDS+=@!PACKAGE_kmod-fs-nfs-root @TARGET_ltqcpe_platform_ar9||TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_ar10||TARGET_ltqcpe_platform_vrx318 +kmod-atm_stack
  KCONFIG += \
	CONFIG_IFX_EXTRA_CFG=y \
	CONFIG_IFX_PPA_A5=y
  FILES += \
	$(LINUX_DIR)/drivers/net/ifxmips_ppa/platform/$(SUBTARGET_SUFFIX)/a5/ifxmips_ppa_datapath_$(SUBTARGET_SUFFIX)_a5.$(LINUX_KMOD_SUFFIX) \
	$(LINUX_DIR)/drivers/net/ifxmips_ppa/platform/$(SUBTARGET_SUFFIX)/a5/ifxmips_ppa_hal_$(SUBTARGET_SUFFIX)_a5.$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/ltqcpe_ppa_a5_mod/description
  $(call KernelPackage/ltqcpe_ppa/description)
  PPA A5 Mode - pluggable module
endef

$(eval $(call KernelPackage,ltqcpe_ppa_a5_mod))


define KernelPackage/ltqcpe_ppa_d5_builtin
  $(call KernelPackage/ltqcpe_ppa)
  TITLE+=(D5 mode - built-in)
  DEPENDS+=@!PACKAGE_kmod-ltqcpe_ppa_d5_mod @TARGET_ltqcpe_platform_ar9||TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_ar10
  KCONFIG += \
	CONFIG_IFX_EXTRA_CFG=y \
	CONFIG_IFX_PPA_D5=y
endef

define KernelPackage/ltqcpe_ppa_d5_builtin/description
  $(call KernelPackage/ltqcpe_ppa/description)
  PPA D5 Mode - built-in
endef

$(eval $(call KernelPackage,ltqcpe_ppa_d5_builtin))

define KernelPackage/ltqcpe_ppa_d5_mod
  $(call KernelPackage/ltqcpe_ppa_mod)
  TITLE+=(D5 mode - pluggable module)
  DEPENDS+=@!PACKAGE_kmod-fs-nfs-root @TARGET_ltqcpe_platform_ar9||TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_ar10
  KCONFIG += \
	CONFIG_IFX_EXTRA_CFG=y \
	CONFIG_IFX_PPA_D5=y
  FILES += \
	$(LINUX_DIR)/drivers/net/ifxmips_ppa/platform/$(SUBTARGET_SUFFIX)/d5/ifxmips_ppa_datapath_$(SUBTARGET_SUFFIX)_d5.$(LINUX_KMOD_SUFFIX) \
	$(LINUX_DIR)/drivers/net/ifxmips_ppa/platform/$(SUBTARGET_SUFFIX)/d5/ifxmips_ppa_hal_$(SUBTARGET_SUFFIX)_d5.$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/ltqcpe_ppa_d5_mod/description
  $(call KernelPackage/ltqcpe_ppa/description)
  PPA D5 Mode - pluggable module
endef

$(eval $(call KernelPackage,ltqcpe_ppa_d5_mod))


define KernelPackage/ltqcpe_ppa_e5_builtin
  $(call KernelPackage/ltqcpe_ppa)
  TITLE+=(E5 mode - built-in)
  DEPENDS+=@!PACKAGE_kmod-ltqcpe_ppa_e5_mod @TARGET_ltqcpe_platform_ar9||@TARGET_ltqcpe_platform_vr9||@TARGET_ltqcpe_platform_ar10 +kmod-atm_stack
  KCONFIG += \
	CONFIG_IFX_EXTRA_CFG=y \
	CONFIG_IFX_PPA_E5=y
endef

define KernelPackage/ltqcpe_ppa_e5_builtin/description
  $(call KernelPackage/ltqcpe_ppa/description)
  PPA E5 Mode - built-in
endef

$(eval $(call KernelPackage,ltqcpe_ppa_e5_builtin))


define KernelPackage/ltqcpe_ppa_e5_mod
  $(call KernelPackage/ltqcpe_ppa_mod)
  TITLE+=(E5 mode - pluggable module)
  DEPENDS+=@!PACKAGE_kmod-fs-nfs-root @TARGET_ltqcpe_platform_ar9||@TARGET_ltqcpe_platform_vr9||@TARGET_ltqcpe_platform_ar10 +kmod-atm_stack
  KCONFIG += \
	CONFIG_IFX_EXTRA_CFG=y \
	CONFIG_IFX_PPA_E5=y
  FILES += \
	$(LINUX_DIR)/drivers/net/ifxmips_ppa/platform/$(SUBTARGET_SUFFIX)/e5/ifxmips_ppa_datapath_$(SUBTARGET_SUFFIX)_e5.$(LINUX_KMOD_SUFFIX) \
	$(LINUX_DIR)/drivers/net/ifxmips_ppa/platform/$(SUBTARGET_SUFFIX)/e5/ifxmips_ppa_hal_$(SUBTARGET_SUFFIX)_e5.$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/ltqcpe_ppa_e5_mod/description
  $(call KernelPackage/ltqcpe_ppa/description)
  PPA E5 Mode - pluggable module
endef

$(eval $(call KernelPackage,ltqcpe_ppa_e5_mod))


define KernelPackage/ltqcpe_ppa_d4_builtin
  $(call KernelPackage/ltqcpe_ppa)
  TITLE+=(D4 mode - built-in)
  DEPENDS+=@!PACKAGE_kmod-ltqcpe_ppa_d4_mod @TARGET_ltqcpe_platform_danube @!PACKAGE_kmod-ltqcpe_ppa_a4_builtin
  KCONFIG += \
	CONFIG_IFX_PPA_D4=y
endef

define KernelPackage/ltqcpe_ppa_d4_builtin/description
  $(call KernelPackage/ltqcpe_ppa/description)
  PPA D4 Mode - built-in
endef

$(eval $(call KernelPackage,ltqcpe_ppa_d4_builtin))


define KernelPackage/ltqcpe_ppa_d4_mod
  $(call KernelPackage/ltqcpe_ppa_mod)
  TITLE+=(D4 mode - pluggable module)
  DEPENDS+=@!PACKAGE_kmod-fs-nfs-root @TARGET_ltqcpe_platform_danube @!PACKAGE_kmod-ltqcpe_ppa_a4
  KCONFIG += \
	CONFIG_IFX_PPA_D4=y
  FILES += \
	$(LINUX_DIR)/drivers/net/ifxmips_ppa/platform/$(SUBTARGET_SUFFIX)/d4/ifxmips_ppa_datapath_$(SUBTARGET_SUFFIX)_d4.$(LINUX_KMOD_SUFFIX) \
	$(LINUX_DIR)/drivers/net/ifxmips_ppa/platform/$(SUBTARGET_SUFFIX)/d4/ifxmips_ppa_hal_$(SUBTARGET_SUFFIX)_d4.$(LINUX_KMOD_SUFFIX) \
	$(LINUX_DIR)/drivers/net/ifxmips_ppa/platform/$(SUBTARGET_SUFFIX)/d4/ifxmips_ppa_lite_datapath_danube_d4.$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/ltqcpe_ppa_d4_mod/description
  $(call KernelPackage/ltqcpe_ppa/description)
  PPA D4 Mode - pluggable module
endef

$(eval $(call KernelPackage,ltqcpe_ppa_d4_mod))


define KernelPackage/ltqcpe_ppa_a4_builtin
  $(call KernelPackage/ltqcpe_ppa)
  TITLE+=(A4 mode - built-in)
  DEPENDS+=@!PACKAGE_kmod-ltqcpe_ppa_a4_mod @TARGET_ltqcpe_platform_danube||TARGET_ltqcpe_platform_amazon_se +kmod-atm_stack
  KCONFIG += \
	CONFIG_IFX_PPA_A4=y
endef

define KernelPackage/ltqcpe_ppa_a4_builtin/description
  $(call KernelPackage/ltqcpe_ppa/description)
  PPA A4 Mode - built-in
endef

$(eval $(call KernelPackage,ltqcpe_ppa_a4_builtin))


define KernelPackage/ltqcpe_ppa_a4_mod
  $(call KernelPackage/ltqcpe_ppa_mod)
  TITLE+=(A4 mode - pluggable module)
  DEPENDS+=@!PACKAGE_kmod-fs-nfs-root @TARGET_ltqcpe_platform_danube||TARGET_ltqcpe_platform_amazon_se +kmod-atm_stack
  KCONFIG += \
	CONFIG_IFX_PPA_A4=y
  FILES += \
	$(LINUX_DIR)/drivers/net/ifxmips_ppa/platform/$(SUBTARGET_SUFFIX)/a4/ifxmips_ppa_datapath_$(SUBTARGET_SUFFIX)_a4.$(LINUX_KMOD_SUFFIX) \
	$(LINUX_DIR)/drivers/net/ifxmips_ppa/platform/$(SUBTARGET_SUFFIX)/a4/ifxmips_ppa_hal_$(SUBTARGET_SUFFIX)_a4.$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/ltqcpe_ppa_a4_mod/description
  $(call KernelPackage/ltqcpe_ppa/description)
  PPA A4 Mode - pluggable module
endef

$(eval $(call KernelPackage,ltqcpe_ppa_a4_mod))

define KernelPackage/ltqcpe_ppa_e4_builtin
  $(call KernelPackage/ltqcpe_ppa)
  TITLE+=(E4 mode - built-in)
  DEPENDS+=@!PACKAGE_kmod-ltqcpe_ppa_e4_mod @TARGET_ltqcpe_platform_amazon_se +kmod-atm_stack
endef

define KernelPackage/ltqcpe_ppa_e4_builtin/description
  $(call KernelPackage/ltqcpe_ppa/description)
  PPA E4 Mode - built-in
endef

$(eval $(call KernelPackage,ltqcpe_ppa_e4_builtin))


define KernelPackage/ltqcpe_ppa_e4_mod
  $(call KernelPackage/ltqcpe_ppa_mod)
  TITLE+=(E4 mode - pluggable module)
  DEPENDS+=@!PACKAGE_kmod-fs-nfs-root @TARGET_ltqcpe_platform_amazon_se +kmod-atm_stack
  FILES += \
	$(LINUX_DIR)/drivers/net/ifxmips_ppa/platform/$(SUBTARGET_SUFFIX)/e4/ifxmips_ppa_datapath_$(SUBTARGET_SUFFIX)_e4.$(LINUX_KMOD_SUFFIX) \
	$(LINUX_DIR)/drivers/net/ifxmips_ppa/platform/$(SUBTARGET_SUFFIX)/e4/ifxmips_ppa_hal_$(SUBTARGET_SUFFIX)_e4.$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/ltqcpe_ppa_e4_mod/description
  $(call KernelPackage/ltqcpe_ppa/description)
  PPA E4 Mode - pluggable module
endef

$(eval $(call KernelPackage,ltqcpe_ppa_e4_mod))

define KernelPackage/ltqcpe_sata_ahci
SUBMENU:=$(LTQCPE_MENU)
TITLE:=Lantiq SATA AHCI support
DEPENDS:=@TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_ar10
KCONFIG:= \
    CONFIG_ATA=y \
    CONFIG_SATA_AHCI=m \
    CONFIG_PCI=y
 
FILES:=$(LINUX_DIR)/drivers/ata/ahci.$(LINUX_KMOD_SUFFIX)
endef
 
define KernelPackage/ltqcpe_sata_ahci/description
Kernel module for Lantiq SATA AHCI package
endef
 
$(eval $(call KernelPackage,ltqcpe_sata_ahci))

define KernelPackage/ltqcpe_usb3
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=Lantiq USB 3.0 support
 DEPENDS:=$(call merge,@TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_ar10 \
	||TARGET_ltqcpe_vr9||TARGET_ltqcpe_ar10)
 KCONFIG:= \
	CONFIG_USB_XHCI_HCD=m \
	CONFIG_USB_SUPPORT=y \
	CONFIG_USB_ARCH_HAS_HCD=y \
	CONFIG_USB=y \
	CONFIG_PCI=y

 FILES:=$(LINUX_DIR)/drivers/usb/host/xhci.$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/ltqcpe_usb3/description
 Kernel module for Lantiq USB 3.0 XHCI package
endef

$(eval $(call KernelPackage,ltqcpe_usb3))


define KernelPackage/ltqcpe_usb_host_base
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=Lantiq USB Host support
 DEPENDS:=@TARGET_ltqcpe @+FEATURE_IFX_USB_HOST
 KCONFIG:= \
	CONFIG_USB_HOST_IFX=m \
	CONFIG_USB_SUPPORT=y \
	CONFIG_USB_ARCH_HAS_HCD=y \
	CONFIG_USB_ARCH_HAS_OHCI=y \
	CONFIG_USB_ARCH_HAS_EHCI=y \
	CONFIG_USB=y \
	CONFIG_PCI=y \
	CONFIG_USB_HOST_IFX_UNALIGNED_ADJ=y \
	CONFIG_USB_HOST_IFX_COC=y \
	CONFIG_USB_LTQ_BULK_TESTER=m

 FILES:=$(LINUX_DIR)/drivers/usb/host/ifxusb_host.$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/ltqcpe_usb_host_base/description
 Kernel module for Lantiq USB Host base package
endef

$(eval $(call KernelPackage,ltqcpe_usb_host_base))

define KernelPackage/ltqcpe_usb_host
 $(call KernelPackage/ltqcpe_usb_host_base)
 TITLE+= on USB port
 DEPENDS:=@TARGET_ltqcpe_platform_danube||TARGET_ltqcpe_platform_amazon_se \
	||TARGET_ltqcpe_danube||TARGET_ltqcpe_amazon_se
endef

define KernelPackage/ltqcpe_usb_host/description
 Kernel module for Lantiq USB Host support on USB port
endef

$(eval $(call KernelPackage,ltqcpe_usb_host))

define KernelPackage/ltqcpe_usb_host_dual
 $(call KernelPackage/ltqcpe_usb_host_base)
 TITLE+= on Both USB ports
 DEPENDS:=$(call merge,@TARGET_ltqcpe_platform_ar9||TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_ar10 \
	||TARGET_ltqcpe_ar9||TARGET_ltqcpe_vr9||TARGET_ltqcpe_ar10)
 KCONFIG += \
	CONFIG_USB_HOST_IFX_B=y
endef

define KernelPackage/ltqcpe_usb_host_dual/description
 Kernel module for Lantiq USB Host support on both USB ports
endef

#Simon@2014/07/29, Set Config Variable USB_SUPPORT when ltqcpe_usb_host_dual is selected.
define KernelPackage/ltqcpe_usb_host_dual/config
    select USB_SUPPORT
endef

$(eval $(call KernelPackage,ltqcpe_usb_host_dual))

define KernelPackage/ltqcpe_usb_host_port1
 $(call KernelPackage/ltqcpe_usb_host_base)
 TITLE+= only on Port 1
 DEPENDS:=$(call merge,@TARGET_ltqcpe_platform_ar9||TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_ar10 \
	||TARGET_ltqcpe_ar9||TARGET_ltqcpe_vr9||TARGET_ltqcpe_ar10)
 KCONFIG += \
	CONFIG_USB_HOST_IFX_1=y
endef

define KernelPackage/ltqcpe_usb_host_port1/description
 Kernel module for Lantiq USB Host support on Port 1
endef

define KernelPackage/ltqcpe_usb_host_port1/config
    select USB_SUPPORT
endef

$(eval $(call KernelPackage,ltqcpe_usb_host_port1))

define KernelPackage/ltqcpe_usb_host_port2
 $(call KernelPackage/ltqcpe_usb_host_base)
 TITLE+= only on Port 2
 DEPENDS:=$(call merge,@TARGET_ltqcpe_platform_ar9||TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_ar10 \
	||TARGET_ltqcpe_ar9||TARGET_ltqcpe_vr9||TARGET_ltqcpe_ar10)
 KCONFIG += \
	CONFIG_USB_HOST_IFX_2=y
endef

define KernelPackage/ltqcpe_usb_host_port2/description
 Kernel module for Lantiq USB Host support on Port 2
endef

define KernelPackage/ltqcpe_usb_host_port2/config
    select USB_SUPPORT
endef

$(eval $(call KernelPackage,ltqcpe_usb_host_port2))

define KernelPackage/ltqcpe_usb_device_base
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=Lantiq USB Device support
 DEPENDS:=@TARGET_ltqcpe @+FEATURE_IFX_USB_DEVICE
 KCONFIG:= \
	CONFIG_USB_GADGET=m \
	CONFIG_USB_SUPPORT=y \
	CONFIG_USB_GADGET_SELECTED=y \
	CONFIG_USB_GADGET_IFX=m \
	CONFIG_USB_GADGET_DUALSPEED=y \
	CONFIG_USB_GADGET_IFX_COC=y \
	CONFIG_USB_ETH=m \
	CONFIG_USB_ETH_RNDIS=y
 FILES:= \
	$(LINUX_DIR)/drivers/usb/gadget/ifxusb_gadget.$(LINUX_KMOD_SUFFIX) \
	$(LINUX_DIR)/drivers/usb/gadget/g_ether.$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/ltqcpe_usb_device_base/description
 Kernel module for Lantiq USB Device support
endef

define KernelPackage/ltqcpe_usb_device
 $(call KernelPackage/ltqcpe_usb_device_base)
 TITLE+= on USB port
 DEPENDS:=$(call merge,@TARGET_ltqcpe_platform_danube||TARGET_ltqcpe_platform_amazon_se \
	||TARGET_ltqcpe_danube||TARGET_ltqcpe_amazon_se)
 KCONFIG += \
	CONFIG_USB_GADGET_IFX_0=y
endef

define KernelPackage/ltqcpe_usb_device/description
 Kernel module for Lantiq USB device support on USB port if device has only one port
endef

$(eval $(call KernelPackage,ltqcpe_usb_device))

define KernelPackage/ltqcpe_usb_device_port1
 $(call KernelPackage/ltqcpe_usb_device_base)
 DEPENDS:=$(call merge,@TARGET_ltqcpe_platform_ar9||TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_ar10 \
	||TARGET_ltqcpe_ar9||TARGET_ltqcpe_vr9||TARGET_ltqcpe_ar10)
 TITLE+= only on Port 1
 KCONFIG += \
	CONFIG_USB_GADGET_IFX_1=y
endef

define KernelPackage/ltqcpe_usb_device_port1/description
 Kernel module for Lantiq USB device support on Port 1
endef

$(eval $(call KernelPackage,ltqcpe_usb_device_port1))

define KernelPackage/ltqcpe_usb_device_port2
 $(call KernelPackage/ltqcpe_usb_device_base)
 TITLE+= only on Port 2
 DEPENDS:=$(call merge,@TARGET_ltqcpe_platform_ar9||TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_ar10 \
	||TARGET_ltqcpe_ar9||TARGET_ltqcpe_vr9||TARGET_ltqcpe_ar10)
 KCONFIG += \
	CONFIG_USB_GADGET_IFX_2=y
endef

define KernelPackage/ltqcpe_usb_device_port2/description
 Kernel module for Lantiq USB device support on Port 2
endef

$(eval $(call KernelPackage,ltqcpe_usb_device_port2))

define KernelPackage/ltqcpe_usb_class_drivers
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=Lantiq USB Class Drivers Support
 DEPENDS:=@TARGET_ltqcpe
 KCONFIG:= \
	CONFIG_USB_SUPPORT=y \
	CONFIG_USB_PRINTER=y \
	CONFIG_USB_PEGASUS=y \
	CONFIG_USB_STORAGE=y \
	CONFIG_USB_STORAGE_DATAFAB=y \
	CONFIG_USB_STORAGE_FREECOM=y \
	CONFIG_USB_STORAGE_DPCM=y \
	CONFIG_USB_STORAGE_USBAT=y \
	CONFIG_USB_STORAGE_SDDR09=y \
	CONFIG_USB_STORAGE_SDDR55=y \
	CONFIG_USB_STORAGE_JUMPSHOT=y \
	CONFIG_USB_STORAGE_ALAUDA=y \
	CONFIG_SCSI=y \
	CONFIG_BLK_DEV_SD=y
endef

define KernelPackage/ltqcpe_usb_class_drivers/description
 Kernel package for Lantiq USB Class Drivers Support
endef

$(eval $(call KernelPackage,ltqcpe_usb_class_drivers))

define KernelPackage/usb_wwan_serial_modem
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=USB 3G / LTE WWAN serial modem support
  DEPENDS:=@TARGET_ltqcpe +kmod-crc-ccitt
  KCONFIG:= \
	CONFIG_USB_SERIAL \
	CONFIG_USB_SERIAL_OPTION \
	CONFIG_PPP_ASYNC

  FILES:= \
	$(LINUX_DIR)/drivers/usb/serial/usbserial.$(LINUX_KMOD_SUFFIX) \
	$(LINUX_DIR)/drivers/usb/serial/option.$(LINUX_KMOD_SUFFIX) \
	$(LINUX_DIR)/drivers/net/ppp_async.$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/usb_wwan_serial_modem/description
  USB 3G / LTE WWAN serial modem support
endef

$(eval $(call KernelPackage,usb_wwan_serial_modem))

#define KernelPackage/ltqcpe_mei_core
#  SUBMENU:=$(LTQCPE_MENU)
#  TITLE:=DSL support
#  DEPENDS:=@TARGET_ltqcpe
#  KCONFIG:= CONFIG_IFXMIPS_DSL_CPE_MEI=y
#endef

#define KernelPackage/ltqcpe_mei_core/description
# Kernel module for DSL support
#endef

#$(eval $(call KernelPackage,ltqcpe_mei_core))


define KernelPackage/ltqcpe_pcie
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=LTQ VRX PCIe Support
 DEPENDS:=@TARGET_ltqcpe_platform_vr9
 KCONFIG:= \
	CONFIG_HW_HAS_PCI=y \
	CONFIG_PCI=y \
	CONFIG_PCIEPORTBUS=y \
	CONFIG_IFX_PCIE=y \
	CONFIG_IFX_PCIE_PHY_36MHZ_MODE=y \
	CONFIG_IFX_PCIE_HW_SWAP=y \
	CONFIG_MMU=y
endef

define KernelPackage/ltqcpe_pcie/description
 Kernel support for IFX XRX288 or XRX3XX PCIe bus
endef

$(eval $(call KernelPackage,ltqcpe_pcie))

define KernelPackage/ltqcpe_pcie_ar10
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=IFX XRX3XX PCIe Support
 DEPENDS:=@TARGET_ltqcpe_platform_ar10
 KCONFIG:= \
	CONFIG_HW_HAS_PCI=y \
	CONFIG_PCI=y \
	CONFIG_PCIEPORTBUS=y \
	CONFIG_IFX_PCIE=y \
	CONFIG_IFX_PCIE_PHY_36MHZ_MODE=y \
	CONFIG_IFX_PCIE_1ST_CORE=y \
	CONFIG_IFX_PCIE_2ND_CORE=y \
	CONFIG_PCI_LEGACY=y \
	CONFIG_MMU=y
endef

define KernelPackage/ltqcpe_pcie/description
 Kernel support for IFX XRX3XX PCIe bus
endef

$(eval $(call KernelPackage,ltqcpe_pcie_ar10))

define KernelPackage/ltqcpe_pcie_hn
 SUBMENU:=$(LTQCPE_MENU)
  TITLE:=LQ HN PCIe Support
  DEPENDS:=@TARGET_ltqcpe_platform_hn1
  KCONFIG:= \
	CONFIG_HW_HAS_PCI=y \
	CONFIG_PCI=y \
	CONFIG_PCIEPORTBUS=y \
	CONFIG_IFX_PCIE=y \
	CONFIG_IFX_PCIE_PHY_25MHZ_MODE=y \
	CONFIG_IFX_PCIE_HW_SWAP=n \
	CONFIG_MMU=y
endef

define KernelPackage/ltqcpe_pcie_hn/description
 Kernel support for LQ HN PCIe bus
endef

$(eval $(call KernelPackage,ltqcpe_pcie_hn))

define KernelPackage/ltqcpe_pcie_enable_ssc
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=VRX or ARX3XX PCIe spread spectrum Support
 DEPENDS:=@TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_ar10
 KCONFIG:= \
	CONFIG_IFX_PCIE_PHY_36MHZ_MODE=n \
	CONFIG_IFX_PCIE_PHY_36MHZ_SSC_MODE=y
endef

define KernelPackage/ltqcpe_pcie_enable_ssc/description
 Kernel support for VRX or ARX3XX PCIe spread spectrum Support
endef

$(eval $(call KernelPackage,ltqcpe_pcie_enable_ssc))


define KernelPackage/ltqcpe_pcie_grx
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=LTQ VRX PCIe Support for GRX variants
 DEPENDS:=@TARGET_ltqcpe_platform_vr9
 KCONFIG:= \
	CONFIG_HW_HAS_PCI=y \
	CONFIG_PCI=y \
	CONFIG_PCIEPORTBUS=y \
	CONFIG_IFX_PCIE=y \
	CONFIG_IFX_PCIE_PHY_25MHZ_MODE=y \
	CONFIG_IFX_PCIE_HW_SWAP=y \
	CONFIG_MMU=y
endef

define KernelPackage/ltqcpe_pcie_grx/description
 Kernel support for LTQ XRX288 PCIe bus for grx variants
endef

$(eval $(call KernelPackage,ltqcpe_pcie_grx))

FS_MENU:=Filesystems

define KernelPackage/fs-nfs-root
  SUBMENU:=$(FS_MENU)
  TITLE:=NFS root support
  KCONFIG:= \
	CONFIG_NFS_FS=y \
	CONFIG_IP_PNP=y \
	CONFIG_IP_PNP_DHCP=y \
	CONFIG_IP_PNP_BOOTP=y \
	CONFIG_IP_PNP_RARP=y \
	CONFIG_LOCKD=y \
	CONFIG_ROOT_NFS=y \
	CONFIG_RPCSEC_GSS_KRB5=y \
	CONFIG_SUNRPC=y \
	CONFIG_SUNRPC_GSS=y

endef

define KernelPackage/fs-nfs-root/description
 Enables kernel settings for NFS root support
endef

$(eval $(call KernelPackage,fs-nfs-root))


define KernelPackage/ltqcpe_iproxyd
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=IGMP Proxy & Snooping support
  DEPENDS:=@TARGET_ltqcpe @!PACKAGE_kmod-lqcpe_igmpd
  KCONFIG:= \
	CONFIG_IP_MULTICAST=y \
	CONFIG_IP_MROUTE=y \
	CONFIG_IFX_IGMP_PROXY=m
	
 FILES:= \
	$(LINUX_DIR)/net/ipv4/iproxyd/iproxyd.$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/ltqcpe_iproxyd/description
 Kernel module for IGMP Proxy & Snooping support
endef

$(eval $(call KernelPackage,ltqcpe_iproxyd))

define KernelPackage/lqcpe_igmpd
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=IGMPv3 Proxy & Snooping support
  DEPENDS:=@TARGET_ltqcpe
  KCONFIG:= \
	CONFIG_IP_MULTICAST=y \
	CONFIG_IP_MROUTE=y
endef

define KernelPackage/lqcpe_igmpd/description
  User daemon for IGMPv3 Proxy & Snooping support
endef

$(eval $(call KernelPackage,lqcpe_igmpd))

define KernelPackage/ltqcpe_mmc
   SUBMENU:=$(LTQCPE_MENU)
   TITLE:=MMC/SD Card Support
   DEPENDS:=@TARGET_ltqcpe_platform_amazon_se
   KCONFIG:= \
	CONFIG_MMC=y \
	CONFIG_MMC_BLOCK=y \
	CONFIG_MMC_DEBUG=n \
	CONFIG_MMC_UNSAFE_RESUME=n \
	CONFIG_MMC_BLOCK_BOUNCE=y \
	CONFIG_MMC_SDHCI=n \
	CONFIG_MMC_TIFM_SD=n \
	CONFIG_MMC_WBSD=n \
	CONFIG_SDIO_UART=n \
	CONFIG_IFX_MMC=y \
	CONFIG_IFX_MMC_BLOCK_DEV=n \
	CONFIG_IFX_MMC_WLAN=y
endef

define KernelPackage/mmc/description
 Kernel support for MMC/SD cards
endef

$(eval $(call KernelPackage,ltqcpe_mmc))

define KernelPackage/ipv6-kernel
  SUBMENU:=Netfilter Extensions
  TITLE:=Extra IPv6 kernel options
  KCONFIG:= \
	CONFIG_IPV6=y \
	CONFIG_IPV6_ROUTER_PREF=y \
	CONFIG_IPV6_ROUTE_INFO=y \
	CONFIG_NF_CONNTRACK_IPV6=y \
	CONFIG_IP6_NF_QUEUE=n
endef

define KernelPackage/ipv6-kernel/description
  Enable extra IPv6 kernel options
endef

$(eval $(call KernelPackage,ipv6-kernel))

define KernelPackage/ipv6-firewall
  SUBMENU:=Netfilter Extensions
  TITLE:=IPv6 firewall options
  DEPENDS:= +kmod-ipv6-kernel
  KCONFIG:= \
	CONFIG_IP6_NF_IPTABLES=m \
	CONFIG_IP6_NF_MATCH_AH=n \
	CONFIG_IP6_NF_MATCH_EUI64=m \
	CONFIG_IP6_NF_MATCH_FRAG=m \
	CONFIG_IP6_NF_MATCH_OPTS=m \
	CONFIG_IP6_NF_MATCH_HL=n \
	CONFIG_IP6_NF_MATCH_IPV6HEADER=m \
	CONFIG_IP6_NF_MATCH_MH=m \
	CONFIG_IP6_NF_MATCH_RT=m \
	CONFIG_IP6_NF_TARGET_HL=n \
	CONFIG_IP6_NF_TARGET_LOG=m \
	CONFIG_IP6_NF_FILTER=m \
	CONFIG_IP6_NF_TARGET_REJECT=m \
	CONFIG_IP6_NF_MANGLE=m \
	CONFIG_IP6_NF_RAW=n \
	CONFIG_IP6_NF_SECURITY=n
endef

define KernelPackage/ipv6-firewall/description
  Enable  IPv6 firewall specific options
endef

$(eval $(call KernelPackage,ipv6-firewall))


define KernelPackage/nf_alg
 SUBMENU:=Netfilter Extensions
 TITLE:=ALG support for SIP, H323
 KCONFIG:= \
	CONFIG_NF_CONNTRACK_H323=y \
	CONFIG_NF_CONNTRACK_SIP=y
endef

define KernelPackage/nf_alg/description
  ALG support for SIP, H323
endef

$(eval $(call KernelPackage,nf_alg))

define KernelPackage/wan-vlan
 SUBMENU:=Netfilter Extensions
 TITLE:=Enable WAN VLAN support in kernel
 KCONFIG:= \
	CONFIG_WAN_VLAN_SUPPORT=y
endef

define KernelPackage/wan-vlan/description
 Enable WAN VLAN support in kernel also enable WAN VLAN support in br2684
endef

$(eval $(call KernelPackage,wan-vlan))

define KernelPackage/mroute6
 SUBMENU:=Netfilter Extensions
 TITLE:=Enable mroute for IPv6 in kernel
 KCONFIG:= \
	CONFIG_IPV6_MROUTE=y \
	CONFIG_IPV6_PIMSM_V2=y
endef

define KernelPackage/mroute6/description
 Enable mroute for IPv6 in kernel
endef

$(eval $(call KernelPackage,mroute6))

define KernelPackage/jffs2
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=JFFS2 FS Support
 DEPENDS:=@TARGET_ltqcpe
 KCONFIG:= \
        CONFIG_JFFS2_FS=y
endef

define KernelPackage/jffs2/description
  Kernel Built-in support for JFFS2 File System
endef

$(eval $(call KernelPackage,jffs2))

define KernelPackage/ubi_mtd
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=UBI Support
 DEPENDS:=@TARGET_ltqcpe
 KCONFIG:= \
	CONFIG_MTD_UBI=y \
	CONFIG_MTD_UBI_BEB_RESERVE=1 \
	CONFIG_MTD_UBI_GLUEBI=y \
	CONFIG_MTD_UBI_GLUEBI_ROOTFS_DEV=y \
	CONFIG_MTD_UBI_DEBUG=n
endef
# CONFIG_MTD_UBI_WL_THRESHOLD is moved to platformX/configs

define KernelPackage/ubi_mtd/description
  Kernel Built-in support for ubi and gluebi mtd
endef

$(eval $(call KernelPackage,ubi_mtd))

define KernelPackage/ubifs
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=UBI Filesystem Support
  DEPENDS:=@TARGET_ltqcpe +kmod-ubi_mtd
  KCONFIG:= \
	CONFIG_UBIFS_FS=y \
	CONFIG_UBIFS_FS_XATTR=n \
	CONFIG_UBIFS_FS_ADVANCED_COMPR=y \
	CONFIG_UBIFS_FS_LZO=y \
	CONFIG_UBIFS_FS_ZLIB=n \
	CONFIG_UBIFS_FS_DEBUG=n
endef

define KernelPackage/ubifs/description
  Kernel Built-in support for ubi filesystem
endef

$(eval $(call KernelPackage,ubifs))

define KernelPackage/ubifs_crypto
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=UBI Filesystem Encryption Support
  DEPENDS:=@CONFIG_UBIFS_FS_CRYPTO
  KCONFIG:= \
        CONFIG_UBIFS_FS_CRYPTO=y
endef

define KernelPackage/ubifs_crypto/description
  Kernel Built-in support for ubi filesystem encryption
endef

$(eval $(call KernelPackage,ubifs_crypto))

define KernelPackage/esp_conntrack_mod
  SUBMENU:=Netfilter Extensions
  TITLE:=ESP Connection Tracking Module
  KCONFIG:= \
	CONFIG_NF_CT_PROTO_ESP=m

  FILES:=$(LINUX_DIR)/net/netfilter/nf_conntrack_proto_esp.$(LINUX_KMOD_SUFFIX) 
endef

define KernelPackage/esp_conntrack_mod/description
  ESP Connection Tracking Module
endef

$(eval $(call KernelPackage,esp_conntrack_mod))

define KernelPackage/ltqcpe_gw188_support
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Kernel support for GW188 model
  KCONFIG:= \
	CONFIG_IFX_GW188=y \
	CONFIG_IFX_PCI_CLOCK_DELAY_NANO_SECONDS=3 \
	CONFIG_IFX_DUAL_MINI_PCI=y
endef

define KernelPackage/ltqcpe_gw188_support/description
  Kernel support option for GW188 model
endef

$(eval $(call KernelPackage,ltqcpe_gw188_support))

define KernelPackage/ltqcpe_rt288_support
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Kernel support for RT288 model
  DEPENDS:=@TARGET_ltqcpe_platform_vr9_RT288
  KCONFIG:= \
	CONFIG_LTQ_RT288=y \
	CONFIG_IFX_DUAL_MINI_PCI=y
endef

define KernelPackage/ltqcpe_rt288_support/description
  Kernel support option(s) for RT288 model
endef

$(eval $(call KernelPackage,ltqcpe_rt288_support))

define KernelPackage/ltqcpe_vb300_support
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Kernel support for VB300 model
  KCONFIG:= \
	CONFIG_LTQ_VB300=y \
	CONFIG_EXTERNAL_T3G=n \
	CONFIG_INTERNAL_AR9=y
endef

define KernelPackage/ltqcpe_vb300_support/description
  Kernel support option for VB300 model
endef

$(eval $(call KernelPackage,ltqcpe_vb300_support))

define KernelPackage/ltqcpe_AR10_Eval_support
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Kernel support for AR10 Eval board
  DEPENDS:=@TARGET_ltqcpe_platform_ar10||TARGET_ltqcpe_ar10
  KCONFIG:= \
	CONFIG_AR10_EVAL_BOARD=y
endef

define KernelPackage/ltqcpe_AR10_Eval_support/description
  Kernel support option for AR10 Eval board
endef

$(eval $(call KernelPackage,ltqcpe_AR10_Eval_support))

define KernelPackage/ltqcpe_AR10_F1_V1_1_support
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Kernel support for AR10 F1 V1_1_board
  DEPENDS:=@TARGET_ltqcpe_platform_ar10||TARGET_ltqcpe_ar10
  KCONFIG:= \
	CONFIG_AR10_FAMILY_BOARD_1_1=y
endef

define KernelPackage/ltqcpe_AR10_F1_V1_1_support/description
  Kernel support option for AR10 F1 V1.1 board
endef

$(eval $(call KernelPackage,ltqcpe_AR10_F1_V1_1_support))

define KernelPackage/ltqcpe_AR10_F1_V1_2_support
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Kernel support for AR10 F1 V1_2_board
  DEPENDS:=$(call merge,@TARGET_ltqcpe_platform_ar10||TARGET_ltqcpe_platform_vrx318 \
	||TARGET_ltqcpe_ar10||TARGET_ltqcpe_vrx318)
  KCONFIG:= \
	CONFIG_AR10_FAMILY_BOARD_1_2=y
endef

define KernelPackage/ltqcpe_AR10_F1_V1_2_support/description
  Kernel support option for AR10 F1 V1.2 board
endef

$(eval $(call KernelPackage,ltqcpe_AR10_F1_V1_2_support))

define KernelPackage/ltqcpe_MGL7016AW_22_ZZ_R0B_support
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Kernel support for MGL7016AW-22-ZZ_R0B board
  DEPENDS:=$(call merge,@TARGET_ltqcpe_platform_ar10_MGL7016AW_22_ZZ_R0B)
  KCONFIG:= \
	CONFIG_TARGET_MGL7016AW_22_ZZ_R0B=y
endef

define KernelPackage/ltqcpe_MGL7016AW_22_ZZ_R0B_support/description
  Kernel support option for MGL7016AW-22-ZZ_R0 board
endef

$(eval $(call KernelPackage,ltqcpe_MGL7016AW_22_ZZ_R0B_support))

define KernelPackage/ltqcpe_AR10_F2_support
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Kernel support for AR10 F2 board
  KCONFIG:= \
	CONFIG_AR10_FAMILY_BOARD_2=y
endef

define KernelPackage/ltqcpe_AR10_F2_support/description
  Kernel support option for AR10 F2 board
endef

$(eval $(call KernelPackage,ltqcpe_AR10_F2_support))


define KernelPackage/ltqcpe_easy_388_bond_extension
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Support for EASY388 BOND board
  KCONFIG:= \
	CONFIG_EASY_388_BOND_EXTENSION=y
endef

define KernelPackage/ltqcpe_easy_388_bond_extension/description
  Kernel support option for EASY_388_BOND_EXTENSION
endef

$(eval $(call KernelPackage,ltqcpe_easy_388_bond_extension))

## linghong_tan 2014-01-27. GRX390 patch
define KernelPackage/ltqcpe_easy_3x0_bond_extension
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=EASY300/330 BOND board
  KCONFIG:= \
        CONFIG_EASY_3X0_BOND_EXTENSION=y
endef

define KernelPackage/ltqcpe_easy_3x0_bond_extension/description
  Kernel support option for EASY_3X0_BOND_EXTENSION
endef

$(eval $(call KernelPackage,ltqcpe_easy_3x0_bond_extension))

define KernelPackage/ltqcpe_grx_390
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Support for GRX390 Platform
  KCONFIG:= \
	CONFIG_AR10_GRX390=y \
	CONFIG_GRX390_MM_ENHANCE=y

endef

define KernelPackage/ltqcpe_grx_390/description
  Kernel support option for GRX390_Platform
endef

$(eval $(call KernelPackage,ltqcpe_grx_390))

define KernelPackage/ltqcpe_vrx220_support
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Support for VRX220 Platform
  DEPENDS:=@TARGET_ltqcpe_platform_vr9_VRX220
  KCONFIG:= \
        CONFIG_LTQ_VRX220=y

endef

define KernelPackage/ltqcpe_vrx220_support/description
  Kernel support option for VRX220_Platform
endef

$(eval $(call KernelPackage,ltqcpe_vrx220_support))



define KernelPackage/ltqcpe_Emulator_Support
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Kernel support for Emulator
  KCONFIG:= \
	CONFIG_USE_EMULATOR=y
endef

define KernelPackage/ltqcpe_Emulator_Support/description
  Kernel support option for Emulator
endef

$(eval $(call KernelPackage,ltqcpe_Emulator_Support))

define KernelPackage/ltqcpe_Palladium_Support
  SUBMENU:=$(LTQCPE_MENU)
  DEPENDS:=kmod-ltqcpe_Emulator_Support
  TITLE:=Kernel support for Palladium Emulator
  KCONFIG:= \
	CONFIG_USE_PALLADIUM=y \
	# CONFIG_MTD is not set \
	# CONFIG_SCSI is not set
endef

define KernelPackage/ltqcpe_Palladium_Support/description
  Kernel support option for Palladium Emulator
endef

$(eval $(call KernelPackage,ltqcpe_Palladium_Support))

define KernelPackage/ltqcpe_BSD_Accounting_Support
  SUBMENU:=$(LTQCPE_MENU)
  DEPENDS:=kmod-ltqcpe_Emulator_Support
  TITLE:=Kernel support for BSD Style process accounting
  KCONFIG:= \
	CONFIG_BSD_PROCESS_ACCT=y
endef

define KernelPackage/ltqcpe_BSD_Accounting_Support/description
  Kernel support option for BSD Style process accounting
endef

$(eval $(call KernelPackage,ltqcpe_BSD_Accounting_Support))

define KernelPackage/ltqcpe_Futex_Support
  SUBMENU:=$(LTQCPE_MENU)
  DEPENDS:=kmod-ltqcpe_Emulator_Support
  TITLE:=Kernel support for Futex support
  KCONFIG:= \
	CONFIG_FUTEX=y
endef

define KernelPackage/ltqcpe_Futex_Support/description
  Kernel support option for Futex Support
endef

$(eval $(call KernelPackage,ltqcpe_Futex_Support))

define KernelPackage/ltqcpe_Epoll_Support
  SUBMENU:=$(LTQCPE_MENU)
  DEPENDS:=kmod-ltqcpe_Emulator_Support
  TITLE:=Kernel support for Epoll support
  KCONFIG:= \
	CONFIG_EPOLL=y
endef

define KernelPackage/ltqcpe_Epoll_Support/description
  Kernel support option for Epoll Support
endef

$(eval $(call KernelPackage,ltqcpe_Epoll_Support))

define KernelPackage/ltqcpe_Shmem_Support
  SUBMENU:=$(LTQCPE_MENU)
  DEPENDS:=kmod-ltqcpe_Emulator_Support
  TITLE:=Kernel support for Shared memory
  KCONFIG:= \
	CONFIG_SHMEM=y
endef

define KernelPackage/ltqcpe_Shmem_Support/description
  Kernel support option for Shared memory
endef

$(eval $(call KernelPackage,ltqcpe_Shmem_Support))
## linghong_tan 2014-01-27. GRX390 patch
define KernelPackage/ltqcpe_vrx318_on_pcie
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Kernel support for PCIe on VRX318
  KCONFIG:= \
	CONFIG_VRX318_PCIE_EP=y \
	CONFIG_HW_HAS_PCI=y \
	CONFIG_PCI=y \
	CONFIG_PCIEPORTBUS=y \
	CONFIG_IFX_PCIE=y \
	CONFIG_IFX_PCIE_PHY_36MHZ_MODE=y \
	CONFIG_IFX_PCIE_1ST_CORE=y \
	CONFIG_IFX_PCIE_2ND_CORE=y \
	CONFIG_IFX_PCIE_3RD_CORE=y \
	CONFIG_PCI_LEGACY=y \
	CONFIG_IFX_PCIE_INBOUND_NO_HW_SWAP=n \
	CONFIG_IFX_PCIE1_INBOUND_NO_HW_SWAP=n \
	CONFIG_IFX_PCIE2_INBOUND_NO_HW_SWAP=n \
	CONFIG_IFX_PCIE_RST_EP_ACTIVE_HIGH=n \
	CONFIG_IFX_PCIE1_RST_EP_ACTIVE_HIGH=n \
	CONFIG_IFX_PCIE2_RST_EP_ACTIVE_HIGH=n \
	CONFIG_IFX_PCIE_HW_SWAP=n \
	CONFIG_IFX_PCIE1_HW_SWAP=n \
	CONFIG_IFX_PCIE2_HW_SWAP=n \
	CONFIG_PCI_QUIRKS=y \
	CONFIG_MMU=y
endef

define KernelPackage/ltqcpe_vrx318_on_pcie/description
  Kernel support option for PCIe on VRX318
endef
$(eval $(call KernelPackage,ltqcpe_vrx318_on_pcie))


define KernelPackage/ltqcpe_vrx318_on_pcie0
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=VRX318 Connected on PCIe0
  DEPENDS+=@PACKAGE_kmod-ltqcpe_vrx318_on_pcie
  KCONFIG += \
	CONFIG_IFX_PCIE_INBOUND_NO_HW_SWAP=y \
	CONFIG_IFX_PCIE_RST_EP_ACTIVE_HIGH=n
endef

define KernelPackage/ltqcpe_vrx318_on_pcie0/description
  VRX318 On PCIe0
endef

$(eval $(call KernelPackage,ltqcpe_vrx318_on_pcie0))


define KernelPackage/lantiq_mini_jumbo_frame
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Mini Jumbo Frames Support
  KCONFIG += \
                CONFIG=y \
                CONFIG_LTQ_MINI_JUMBO_FRAME_SUPPORT=y
endef

define KernelPackage/lantiq_mini_jumbo_frame/description
        Support for mini jumbo frames (also called as baby jumbo frames).
endef

$(eval $(call KernelPackage,lantiq_mini_jumbo_frame))


# Terry 20140213, KPN need to enable this without PACKAGE_kmod-ltqcpe_wlan_atheros
# DEPENDS+=@PACKAGE_kmod-ltqcpe_vrx318_on_pcie @PACKAGE_kmod-ltqcpe_wlan_atheros
define KernelPackage/ltqcpe_vrx318_on_pcie0_hw_swap_disable
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Inbound Hardware Swap disabled for PCIe0
  DEPENDS+=@PACKAGE_kmod-ltqcpe_vrx318_on_pcie @PACKAGE_kmod-ltqcpe_wlan_atheros
  KCONFIG += \
	CONFIG_IFX_PCIE_INBOUND_NO_HW_SWAP=n
endef

define KernelPackage/ltqcpe_vrx318_on_pcie0_hw_swap_disable/description
  VRX318 Inbound Hardware Swap disabled for PCIe0
endef

$(eval $(call KernelPackage,ltqcpe_vrx318_on_pcie0_hw_swap_disable))

define KernelPackage/ltqcpe_vrx318_on_pcie1
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=VRX318 Connected on PCIe1
  DEPENDS+=@PACKAGE_kmod-ltqcpe_vrx318_on_pcie
  KCONFIG += \
	CONFIG_IFX_PCIE1_INBOUND_NO_HW_SWAP=y \
	CONFIG_IFX_PCIE1_RST_EP_ACTIVE_HIGH=n
endef

define KernelPackage/ltqcpe_vrx318_on_pcie1/description
  VRX318 On PCIe1
endef

$(eval $(call KernelPackage,ltqcpe_vrx318_on_pcie1))


define KernelPackage/ltqcpe_vrx318_on_pcie2
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=VRX318 Connected on PCIe2
  DEPENDS+=@PACKAGE_kmod-ltqcpe_vrx318_on_pcie
  KCONFIG += \
	CONFIG_IFX_PCIE2_INBOUND_NO_HW_SWAP=y \
	CONFIG_IFX_PCIE2_RST_EP_ACTIVE_HIGH=n
endef

define KernelPackage/ltqcpe_vrx318_on_pcie2/description
  VRX318 On PCIe2
endef

$(eval $(call KernelPackage,ltqcpe_vrx318_on_pcie2))



define KernelPackage/ltqcpe_vrx318_pcie_ep_test
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Kernel support for PCIe Test driver on VRX318
  KCONFIG:= \
	CONFIG_VRX318_PCIE_EP_TEST=m
endef

define KernelPackage/ltqcpe_vrx318_pcie_ep_test/description
  Kernel support PCIe test driver on VRX318
endef
$(eval $(call KernelPackage,ltqcpe_vrx318_pcie_ep_test))

define KernelPackage/ltqcpe_arpd_support
  SUBMENU:=$(LTQCPE_MENU)
  DEPENDS:=kmod-ltqcpe_Emulator_Support
  TITLE:=Kernel support for ARPD
  KCONFIG:= \
	CONFIG_ARPD=y \
	CONFIG_SYN_COOKIES=y
endef

define KernelPackage/ltqcpe_arpd_support/description
  Kernel support for ARPD
endef

$(eval $(call KernelPackage,ltqcpe_arpd_support))

define KernelPackage/vrx318_atm_stack
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=ATM stack
  DEPENDS:=kmod-ltqcpe_Emulator_Support
  KCONFIG:= \
	CONFIG_ATM=y \
	CONFIG_ATM_BR2684=y
endef

define KernelPackage/vrx318_atm_stack/description
 Kernel built-in support for ATM stack for emulation
endef

$(eval $(call KernelPackage,vrx318_atm_stack))

define KernelPackage/ipsec_ike_mod
  SUBMENU:=Netfilter Extensions
  TITLE:=IPSec IKE ALG Module
  KCONFIG:= \
	CONFIG_NF_CONNTRACK_IPSEC_IKE=m

  FILES:=$(LINUX_DIR)/net/netfilter/nf_conntrack_ipsec_ike.$(LINUX_KMOD_SUFFIX) 
endef

define KernelPackage/ipsec_ike_mod/description
  IPSec IKE ALG  Module
endef

$(eval $(call KernelPackage,ipsec_ike_mod))

define KernelPackage/ltqcpe_ipsec
  SUBMENU:=Netfilter Extensions
  TITLE:=IPSec Support
  KCONFIG:= \
	CONFIG_CRYPTO_DEV_PWR_SAVE_MODE=n \
	CONFIG_XFRM_USER=y \
	CONFIG_XFRM_SUB_POLICY=y \
	CONFIG_XFRM_IPCOMP=y \
	CONFIG_NET_KEY=y \
	CONFIG_INET_AH=y \
	CONFIG_INET_ESP=y \
	CONFIG_INET_IPCOMP=y \
	CONFIG_INET_XFRM_TUNNEL=y \
	CONFIG_INET_TUNNEL=y \
	CONFIG_INET_XFRM_MODE_TRANSPORT=y \
	CONFIG_INET_XFRM_MODE_TUNNEL=y \
	CONFIG_CRYPTO_AEAD=y \
	CONFIG_CRYPTO_AUTHENC=y \
	CONFIG_CRYPTO_HMAC=y \
	CONFIG_CRYPTO_DEFLATE=y \
	CONFIG_NETFILTER_XT_MATCH_POLICY=y \
	CONFIG_NET_IPIP=y \
	CONFIG_CRYPTO_RNG=y \
	CONFIG_CRYPTO_GF128MUL=y \
	CONFIG_CRYPTO_GCM=y \
	CONFIG_CRYPTO_SEQIV=y \
	CONFIG_CRYPTO_CTR=y \
	CONFIG_CRYPTO_ECB=y \
	CONFIG_CRYPTO_XCBC=y \
	CONFIG_CRYPTO_GHASH=y \
	CONFIG_CRYPTO_MD5=y \
	CONFIG_CRYPTO_BLOWFISH=y
endef

define KernelPackage/ltqcpe_ipsec/description
  IPSec Support
endef

$(eval $(call KernelPackage,ltqcpe_ipsec))


define KernelPackage/ltqcpe_ebtables
  SUBMENU:=$(LTQCPE_MENU)
  DEPENDS:=@!PACKAGE_kmod-ebtables
  TITLE:=Bridge firewalling modules
  KCONFIG:= \
	  CONFIG_BRIDGE_NF_EBTABLES=y \
 	  CONFIG_BRIDGE_EBT_BROUTE=n \
	  CONFIG_BRIDGE_EBT_T_FILTER=y \
	  CONFIG_BRIDGE_EBT_802_3=n \
	  CONFIG_BRIDGE_EBT_AMONG=n \
	  CONFIG_BRIDGE_EBT_LIMIT=n \
	  CONFIG_BRIDGE_EBT_MARK=y \
	  CONFIG_BRIDGE_EBT_PKTTYPE=y \
	  CONFIG_BRIDGE_EBT_STP=n \
	  CONFIG_BRIDGE_EBT_VLAN=n \
	  CONFIG_BRIDGE_EBT_MARK_T=y \
	  CONFIG_BRIDGE_EBT_REDIRECT=y
endef

define KernelPackage/ltqcpe_ebtables/description
  ebtables is a general, extensible frame/packet identification
  framework. It provides you to do Ethernet
  filtering/NAT/brouting on the Ethernet bridge.
endef

$(eval $(call KernelPackage,ltqcpe_ebtables))

define KernelPackage/ltqcpe_ebtables-ipv4
  SUBMENU:=$(LTQCPE_MENU)
  DEPENDS:=kmod-ltqcpe_ebtables
  TITLE:=ebtables: IPv4 support
  KCONFIG:= \
	  CONFIG_BRIDGE_EBT_ARP=n \
	  CONFIG_BRIDGE_EBT_IP=y \
	  CONFIG_BRIDGE_EBT_ARPREPLY=n \
	  CONFIG_BRIDGE_EBT_DNAT=n
endef

define KernelPackage/ltqcpe_ebtables-ipv4/description
 This option adds the IPv4 support to ebtables, which allows basic
 IPv4 header field filtering, ARP filtering as well as SNAT, DNAT targets.
endef

$(eval $(call KernelPackage,ltqcpe_ebtables-ipv4))


define KernelPackage/ltqcpe_ebtables-ipv6
  SUBMENU:=$(LTQCPE_MENU)
  DEPENDS:=kmod-ltqcpe_ebtables
  TITLE:=ebtables: IPv6 support
  KCONFIG:= \
	  CONFIG_BRIDGE_EBT_IP6=n
endef

define KernelPackage/ltqcpe_ebtables-ipv6/description
 This option adds the IPv6 support to ebtables, which allows basic
 IPv6 header field filtering and target support.
endef

$(eval $(call KernelPackage,ltqcpe_ebtables-ipv6))


define KernelPackage/ltqcpe_hardware_deu
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Hardware DEU support
  KCONFIG:= \
	CONFIG_CRYPTO_DEV_DEU=y
endef

define KernelPackage/ltqcpe_hardware_deu/description
  IPSec IKE ALG  Module
endef

$(eval $(call KernelPackage,ltqcpe_hardware_deu))

define KernelPackage/ltq_optimization
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Kernel LTQ Optimizations
  KCONFIG:= \
	CONFIG_LTQ_OPTIMIZATION=y \
	CONFIG_NF_CONNTRACK_IRC=n \
	CONFIG_NF_NAT_IRC=n \
	CONFIG_NETFILTER_XT_TARGET_NOTRACK=n \
	CONFIG_NETFILTER_XT_MATCH_COMMENT=n	
endef

define KernelPackage/ltq_optimization/description
  Kernel LTQ Optimizations
endef

$(eval $(call KernelPackage,ltq_optimization))

define KernelPackage/ltqhn1_hn
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=Lantiq HN driver module
 DEPENDS:=@TARGET_ltqcpe_platform_hn1
 KCONFIG:= \
	CONFIG_LTQ_HN_HNDRV=y
endef


define KernelPackage/ltqhn1_hn/description
  HN driver module
endef

$(eval $(call KernelPackage,ltqhn1_hn))

define KernelPackage/ltqhn1_mb
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=Lantiq mailbox driver module
 DEPENDS:=@TARGET_ltqcpe_platform_hn1
 KCONFIG:= \
	CONFIG_LTQ_HN_MBMOD=y
endef

define KernelPackage/ltqhn1_mb/description
  Mailbox driver module
endef

$(eval $(call KernelPackage,ltqhn1_mb))

define KernelPackage/ltqhn1_vpelauncher
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=Lantiq HN VPE launcher module
 DEPENDS:=@TARGET_ltqcpe_platform_hn1
 KCONFIG:= \
	CONFIG_LTQ_HN_VPELAUNCHER=m
FILES:=$(LINUX_DIR)/drivers/char/ltq_hn_vpe.$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/ltqhn1_vpelauncher/description
  HN VPE launcher module
endef

$(eval $(call KernelPackage,ltqhn1_vpelauncher))

define KernelPackage/ltqhn1_spcmac
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=Lantiq HN special mac snooping support
 DEPENDS:=@TARGET_ltqcpe_platform_hn1
 KCONFIG:= \
	CONFIG_LTQ_HN_MAC_SNOOPING=y
endef

define KernelPackage/ltqhn1_spcmac/description
 Kernel support for HN special mac snooping
endef

$(eval $(call KernelPackage,ltqhn1_spcmac))

define KernelPackage/ltqcpe_hn100w_support
 SUBMENU:=$(LTQCPE_MENU)
 DEPENDS:=@TARGET_ltqcpe_platform_hn1_HN100W
 TITLE:=Kernel support for HN100W model
 KCONFIG:= \
 	CONFIG_HN1_REF_BOARD=y \
 	CONFIG_HN1_NOT_USE_TANTOS=y \
 	CONFIG_HN1_USE_TANTOS=n
endef

define KernelPackage/ltqcpe_hn100w_support/description
 Kernel support option for HN100W model
endef

$(eval $(call KernelPackage,ltqcpe_hn100w_support))

define KernelPackage/ltq_switchport_isolation
 SUBMENU:=$(LTQCPE_MENU)
 DEPENDS:=@TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_ar10
 TITLE:=Lantiq Switch Port Isolation
 KCONFIG:= \
           CONFIG_LTQ_SWITCH_PORT_ISOLATION=n
endef

define KernelPackage/ltq_switchport_isolation/description
 Kernel support for switch port isolation
endef

define KernelPackage/ltq_vlan_switchport_isolation
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=Lantiq VLAN Based Switch Port Isolation
 KCONFIG:= \
           CONFIG_LTQ_VLAN_SWITCH_PORT_ISOLATION=y
endef

define KernelPackage/ltq_vlan_switchport_isolation/description
 Kernel support for vlan based switch port isolation
endef

$(eval $(call KernelPackage,ltq_vlan_switchport_isolation))

define KernelPackage/ltq_portbinding
 SUBMENU:=$(LTQCPE_MENU)
 DEPENDS:=@PACKAGE_kmod-ltq_switchport_isolation||PACKAGE_kmod-ltq_vlan_switchport_isolation
 TITLE:=Lantiq PortBinding 
 KCONFIG:= \
       CONFIG_IP_ADVANCED_ROUTER=y \
       CONFIG_IP_MULTIPLE_TABLES=y \
       CONFIG_BRIDGE_NETFILTER=y \
       CONFIG_NETFILTER_XT_MATCH_PHYSDEV=y
endef

define KernelPackage/ltq_portbinding//description
 kernel support for PortBinding 
endef

$(eval $(call KernelPackage,ltq_portbinding))

define KernelPackage/ftrace
  SUBMENU:=Other modules
  TITLE:=Ftrace support
  KCONFIG:= \
	CONFIG_FTRACE=y \
	CONFIG_TRACING=y \
	CONFIG_IRQSOFF_TRACER=y \
	CONFIG_PREEMPT_TRACER=y \
	CONFIG_SCHED_TRACER=y \
	CONFIG_ENABLE_DEFAULT_TRACERS=y \
	CONFIG_FTRACE_SYSCALLS=y \
	CONFIG_BOOT_TRACER=y \
	CONFIG_BRANCH_TRACER=y \
	CONFIG_POWER_TRACER=y \
	CONFIG_HW_BRANCH_TRACER=y \
	CONFIG_KMEMTRACE=y \
	CONFIG_WORKQUEUE_TRACER=y \
	CONFIG_BLK_DEV_IO_TRACE=y \
	CONFIG_DYNAMIC_FTRACE=y \
	CONFIG_FUNCTION_PROFILER=y \
	CONFIG_MMIOTRACE=y \
	CONFIG_MMIOTRACE_TEST=y
endef

define KernelPackage/ftrace/description
  Kernel support for FTRACE
endef

$(eval $(call KernelPackage,ftrace))

$(eval $(call KernelPackage,ltqcpe_hn100w_support))

define KernelPackage/ltqcpe_ppe_e5_offchip_bonding
 SUBMENU:=$(LTQCPE_MENU)
 TITLE:=Kernel Support for PPE E5 OFFChip Bonding
 DEPENDS+=@PACKAGE_kmod-ltqcpe_ppa_e5_mod||PACKAGE_kmod-ltqcpe_ppa_e5_builtin @TARGET_ltqcpe_platform_vr9
 KCONFIG += \
        CONFIG_IFX_PPE_E5_OFFCHIP_BONDING=y \
        CONFIG_IFX_PCI_BOOT=y
endef

$(eval $(call KernelPackage,ltqcpe_ppe_e5_offchip_bonding))

define KernelPackage/aprp
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=APRP configuration
  DEPENDS:=$(call merge,@TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_ar10 \
	||TARGET_ltqcpe_vr9||TARGET_ltqcpe_ar10)
  KCONFIG:= \
	CONFIG_MTSCHED=y \
	CONFIG_MIPS_VPE_LOADER=y \
	CONFIG_IFX_VPE_EXT=y \
	CONFIG_SOFT_WATCHDOG_VPE=y \
	CONFIG_MIPS_VPE_LOADER_TOM=y
endef

define KernelPackage/aprp/description
  Enable APRP related kernel configuration
endef

$(eval $(call KernelPackage,aprp))

define KernelPackage/smvp
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=SMVP configuration
  DEPENDS:=@!PACKAGE_kmod-aprp $(call merge,@TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_ar10 \
	||TARGET_ltqcpe_ar9||TARGET_ltqcpe_ar10)
  KCONFIG:= \
	CONFIG_MTSCHED=y \
	CONFIG_MIPS_MT_SMP=y \
	CONFIG_SCHED_SMT=y \
	CONFIG_SYS_SUPPORTS_SCHED_SMT=y \
	CONFIG_LTQ_MIPS_MT_SMP_IRQAFF=y
endef

define KernelPackage/smvp/description
  Enable SMVP related kernel configuration
endef

$(eval $(call KernelPackage,smvp))

define KernelPackage/debug
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Enable debug options
  KCONFIG:= \
	CONFIG_DEBUG_KERNEL=y \
	CONFIG_DETECT_SOFTLOCKUP=y \
	CONFIG_BOOTPARAM_SOFTLOCKUP_PANIC_VALUE=0 \
	CONFIG_DETECT_HUNG_TASK=y \
	CONFIG_BOOTPARAM_HUNG_TASK_PANIC_VALUE=0 \
	CONFIG_SCHED_DEBUG=y \
	CONFIG_DEBUG_INFO=y \
	CONFIG_IKCONFIG=y \
	CONFIG_IKCONFIG_PROC=y \
	CONFIG_SWAP=y

endef

define KernelPackage/debug/description
  Enable kernel debug options
endef

$(eval $(call KernelPackage,debug))

define KernelPackage/fast-ethernet
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Enable Fast Ethernet Mode
  DEPENDS:=@TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_ar10 @UBOOT_CONFIG_VR9_SW_PORT2_MII||UBOOT_CONFIG_AR10_SW_PORT2_MII @TARGET_ltqcpe_platform_ar10_MGL7016AW_22_ZZ_6L||TARGET_ltqcpe_platform_ar10_MGL7016AW_22_ZZ_R0A||TARGET_ltqcpe_platform_ar10_MGL7016AW_22_ZZ_R0B
  KCONFIG:= \
	CONFIG_FE_MODE=y
endef

define KernelPackage/fast-ethernet/description
  Enable Fast Ethernet Mode for the switch ports
endef

$(eval $(call KernelPackage,fast-ethernet))

define KernelPackage/gigafast-mixed-ethernet
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Enable Giga/Fast Mixed Ethernet Mode
  DEPENDS:=@TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_ar10 @UBOOT_CONFIG_VR9_SW_PORT2_MII||UBOOT_CONFIG_AR10_SW_PORT2_MII @TARGET_ltqcpe_platform_ar10_MGL7016AW22_VR
  KCONFIG:= \
	CONFIG_GEFE_MIXED_MODE=y
endef

define KernelPackage/gigafast-mixed-ethernet/description
  Enable Giga/Fast Mixed Ethernet Mode for the switch ports
endef

$(eval $(call KernelPackage,gigafast-mixed-ethernet))

define KernelPackage/ltqcpe_hfsplus
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Enable extra hfsplus options
  DEPENDS:=@PACKAGE_kmod-fs-hfsplus
  KCONFIG:= \
	CONFIG_PARTITION_ADVANCED=y \
	CONFIG_EFI_PARTITION=y \
	CONFIG_MAC_PARTITION=y
endef

define KernelPackage/ltqcpe_hfsplus/description
  Enable GUID & MAC partition related kernel configuration
endef

$(eval $(call KernelPackage,ltqcpe_hfsplus))

define KernelPackage/ltqcpe_mlcnand_cs1
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Enable Chip Select1 for MLC Nand
  DEPENDS:=@TARGET_ltqcpe_platform_ar10||TARGET_ltqcpe_ar10
  KCONFIG:= \
	CONFIG_NAND_CS1=y
endef

define KernelPackage/ltqcpe_mlcnand_cs1/description
  Enable Chip Select1 for MLC Nand in Kernel Configuration
endef

$(eval $(call KernelPackage,ltqcpe_mlcnand_cs1))

define KernelPackage/debug-smp
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Enable Spinlock debug options for SMP model
  KCONFIG:= \
	CONFIG_DEBUG_KERNEL=y \
        CONFIG_SCHEDSTATS=y \
	CONFIG_DEBUG_SPINLOCK=y \
	CONFIG_DEBUG_MUTEXES=y \
	CONFIG_DEBUG_LOCK_ALLOC=y \
	CONFIG_PROVE_LOCKING=y \
	CONFIG_LOCKDEP=y \
	CONFIG_LOCK_STAT=y \
	CONFIG_DEBUG_LOCKDEP=y \
	CONFIG_TRACE_IRQFLAGS=y \
	CONFIG_DEBUG_SPINLOCK_SLEEP=y \
	CONFIG_DETECT_HUNG_TASK=y \
	CONFIG_SCHED_DEBUG=y
endef

define KernelPackage/debug-smp/description
  Enable Spinlock debug options for SMP model 
endef

$(eval $(call KernelPackage,debug-smp))

define KernelPackage/ltqcpe_vbg400_chipit_support
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Kernel support for VBG400 FPGA model
  KCONFIG:= \
	CONFIG_VLAN_8021.Q \
	CONFIG_DUMMY=m \
	CONFIG_DEBUG_KERNEL=y \
	CONFIG_DETECT_SOFTLOCKUP=y \
	CONFIG_BOOTPARAM_SOFTLOCKUP_PANIC_VALUE=0 \
	CONFIG_DEBUG_INFO=y \
	CONFIG_NET_IPIP=n \
	CONFIG_LTQ_VB400=y \
	CONFIG_EXTERNAL_T3G=n \
	CONFIG_INTERNAL_VBG400=y \
    CONFIG_VBG400_MAC_GMAC_INDEX=0 \
    CONFIG_VBG400_MAC_INTERFACE_MODE=0 \
 	CONFIG_VBG400_CHIPIT=y
endef

define KernelPackage/ltqcpe_vbg400_chipit_support/description
  Kernel support option for VBG400 FPGA model
endef

$(eval $(call KernelPackage,ltqcpe_vbg400_chipit_support))

define KernelPackage/ltqcpe_vbg400_evb_support
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Kernel support for VBG400 evaluation board
  KCONFIG:= \
	CONFIG_VLAN_8021.Q \
	CONFIG_DUMMY=m \
	CONFIG_DEBUG_KERNEL=y \
	CONFIG_DETECT_SOFTLOCKUP=y \
	CONFIG_BOOTPARAM_SOFTLOCKUP_PANIC_VALUE=0 \
	CONFIG_DEBUG_INFO=y \
	CONFIG_NET_IPIP=n \
	CONFIG_LTQ_VB400=y \
	CONFIG_EXTERNAL_T3G=n \
	CONFIG_INTERNAL_VBG400=y \
    CONFIG_VBG400_MAC_GMAC_INDEX=0 \
    CONFIG_VBG400_MAC_INTERFACE_MODE=0 \
 	CONFIG_VBG400_EVB=y
endef

define KernelPackage/ltqcpe_vbg400_evb_support/description
  Kernel support option for VBG400 evaluation board
endef

$(eval $(call KernelPackage,ltqcpe_vbg400_evb_support))

define KernelPackage/ltqcpe_ppa_vrx318_module
  $(call KernelPackage/ltqcpe_ppa_mod)
  TITLE+=(VRX318 Module Support)
  DEPENDS+=@!PACKAGE_kmod-ltqcpe_ppa_d5_mod @TARGET_ltqcpe_platform_ar9||TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_ar10
  KCONFIG += \
	CONFIG_IFX_EXTRA_CFG=y \
	CONFIG_IFX_PPA_D5=y	\
	CONFIG_IFX_PPA_A5=n	\
	CONFIG_IFX_PPA_VRX218_E1=y	\
	CONFIG_IFX_PPA_VRX218_A1=y

  FILES += \
	$(LINUX_DIR)/drivers/net/ifxmips_ppa/platform/vrx218/soc/ltqmips_ppe_drv.$(LINUX_KMOD_SUFFIX) \
	$(LINUX_DIR)/drivers/net/ifxmips_ppa/platform/vrx218/smartphy/ltqmips_vrx318_e1.$(LINUX_KMOD_SUFFIX) \
	$(LINUX_DIR)/drivers/net/ifxmips_ppa/platform/vrx218/smartphy/ltqmips_vrx318_a1.$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/ltqcpe_ppa_vrx318_module/description
  $(call KernelPackage/ltqcpe_ppa/description)
  PPA for VRX318 support - A1, E1 Modules
endef

$(eval $(call KernelPackage,ltqcpe_ppa_vrx318_module))

define KernelPackage/ltqcpe_ppa_vrx318_builtin
  $(call KernelPackage/ltqcpe_ppa)
  TITLE+=(VRX318 E1 Built-in)
  DEPENDS+=@!PACKAGE_kmod-ltqcpe_ppa_d5_mod @!PACKAGE_kmod-ltqcpe_ppa_vrx318_module @TARGET_ltqcpe_platform_ar9||TARGET_ltqcpe_platform_vr9||TARGET_ltqcpe_platform_ar10
  KCONFIG += \
	CONFIG_IFX_EXTRA_CFG=y \
	CONFIG_IFX_PPA_D5=y     \
	CONFIG_IFX_PPA_A5=n	\
	CONFIG_IFX_PPA_VRX218_E1=y
endef

define KernelPackage/ltqcpe_ppa_vrx318_builtin/description
  $(call KernelPackage/ltqcpe_ppa/description)
  PPA for VRX318 support - E1 Builtin mode
endef

$(eval $(call KernelPackage,ltqcpe_ppa_vrx318_builtin))

define KernelPackage/ltqcpe_ppa_vrx318_bonding
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=VRX318 Bonding Support
  DEPENDS+=@TARGET_ltqcpe_platform_ar10_vrx318||TARGET_ltqcpe_platform_ar10_GRX390
  KCONFIG += \
	CONFIG_VRX318_PCIE_SW_DSL_BONDING=y
endef

define KernelPackage/ltqcpe_ppa_vrx318_bonding/description
  VRX318 Bonding support
endef

$(eval $(call KernelPackage,ltqcpe_ppa_vrx318_bonding))

define KernelPackage/ltqcpe_vdsl_vectoring_support
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=PTM Vectoring Support
  KCONFIG += \
	CONFIG_PTM_VECTORING=y
endef

define KernelPackage/ltqcpe_vdsl_vectoring_support/description
  PTM Vectoring support
endef

$(eval $(call KernelPackage,ltqcpe_vdsl_vectoring_support))

define KernelPackage/disable_ltqcpe_route_and_bridge_simultaneously
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Disable Route and Bridge Simultaneously
  KCONFIG += \
	CONFIG_LTQ_ROUTE_AND_BRIDGE_SIMULTANEOUSLY=n
endef

define KernelPackage/disable_ltqcpe_route_and_bridge_simultaneously/description
  Disable CONFIG_LTQ_ROUTE_AND_BRIDGE_SIMULTANEOUSLY which requires one more MAC address for routing interface.
endef

$(eval $(call KernelPackage,disable_ltqcpe_route_and_bridge_simultaneously))

#linghong_tan 2014-07-07 UGW5.4 03-01-00-UGW-5.4-SW-CD-New-Platforms-patch
define KernelPackage/ltqcpe_directlink_support
  SUBMENU:=$(LTQCPE_MENU)
  TITLE:=Direct Link Support
  DEPENDS:=@TARGET_ltqcpe_platform_ar10_GRX390
  KCONFIG:= \
        CONFIG_ACCL_11AC=m

  FILES += \
        $(LINUX_DIR)/drivers/net/ifxmips_ppa/dlrx_fw.$(LINUX_KMOD_SUFFIX) \
	$(LINUX_DIR)/drivers/net/ifxmips_ppa/platform/vrx218/soc/directlink/ltqmips_dtlk.$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/ltqcpe_directlink_support/description
  Direct Link Support
endef

$(eval $(call KernelPackage,ltqcpe_directlink_support))