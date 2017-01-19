ifneq ($(CONFIG_IFX_PPA_VRX218_E1),)
obj-$(CONFIG_IFX_PPA_DATAPATH)	+= ltqmips_vrx318_e1.o
endif
ifneq ($(CONFIG_IFX_PPA_VRX218_A1),)
obj-$(CONFIG_IFX_PPA_DATAPATH)	+= ltqmips_vrx318_a1.o
endif

ltqmips_vrx318_e1-objs = vrx218_dfe.o vrx218_common.o vrx218_ppe_ptm_init.o vrx218_ptm_datapath.o vrx218_ptm_main.o vrx218_ptm_qos.o
ltqmips_vrx318_a1-objs = vrx218_dfe.o vrx218_common.o vrx218_ppe_atm_init.o vrx218_atm_datapath.o vrx218_atm_main.o



