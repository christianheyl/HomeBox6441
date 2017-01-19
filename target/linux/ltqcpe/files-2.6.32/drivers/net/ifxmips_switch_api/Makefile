#
# Makefile for the switch api
#
EXTRA_CFLAGS := -I$(srctree)/include/switch_api

obj-$(CONFIG_IFX_ETHSW_API)	+= drv_switch_api.o
obj-$(CONFIG_GPHY_DRIVER)     += GPHY/

drv_switch_api-objs	:= ltq_linux_ioctl_wrapper.o
drv_switch_api-objs	+= ltq_ethsw_init.o

##################### AR9 ######################
ifeq ($(CONFIG_AR9),y)
  EXTRA_CFLAGS += -DAR9
  drv_switch_api-objs   += ltq_tantos_core.o
  drv_switch_api-objs   += ltq_tantos_core_ll_table.o
  drv_switch_api-objs   += ltq_tantos_core_rml.o
#  drv_switch_api-objs   += ifx_ethsw_rml.o
  drv_switch_api-objs   += ltq_tantos_ral.o
ifeq ($(CONFIG_IFX_ETHSW_API_COC),y)
  EXTRA_CFLAGS += -DIFX_ETHSW_API_COC
  drv_switch_api-objs	+= ltq_ethsw_pm.o
  drv_switch_api-objs	+= ltq_ethsw_pm_plat.o
ifeq ($(CONFIG_IFX_ETHSW_API_COC_PMCU),y)
  EXTRA_CFLAGS += -DIFX_ETHSW_API_COC_PMCU
  drv_switch_api-objs	+= ltq_ethsw_pm_pmcu.o
endif
endif
  drv_switch_api-objs   += ltq_ar9_rml_table.o
  drv_switch_api-objs   += ltq_Tantos_rml_table.o
endif

##################### Danube ######################
ifeq ($(CONFIG_DANUBE),y)
  EXTRA_CFLAGS += -DDANUBE
  drv_switch_api-objs   += ltq_tantos_core.o
  drv_switch_api-objs   += ltq_tantos_core_ll_table.o
  drv_switch_api-objs   += ltq_tantos_core_rml.o
#  drv_switch_api-objs   += ifx_ethsw_rml.o
  drv_switch_api-objs   += ltq_tantos_ral.o
ifeq ($(CONFIG_IFX_ETHSW_API_COC),y)
  EXTRA_CFLAGS += -DIFX_ETHSW_API_COC
  drv_switch_api-objs	+= ltq_ethsw_pm.o
  drv_switch_api-objs	+= ltq_ethsw_pm_plat.o
ifeq ($(CONFIG_IFX_ETHSW_API_COC_PMCU),y)
  EXTRA_CFLAGS += -DIFX_ETHSW_API_COC_PMCU
  drv_switch_api-objs	+= ltq_ethsw_pm_pmcu.o
endif
endif
  drv_switch_api-objs   += ltq_ar9_rml_table.o
  drv_switch_api-objs   += ltq_Tantos_rml_table.o
endif

##################### Amazon-SE ######################
ifeq ($(CONFIG_AMAZON_SE),y)
  EXTRA_CFLAGS += -DAMAZON_SE
ifeq ($(CONFIG_AMAZON_SE_TANTOS0G_GPIO),y)
  EXTRA_CFLAGS += -DAMAZON_SE_TANTOS0G_GPIO
endif
  drv_switch_api-objs   += ltq_tantos_core.o
  drv_switch_api-objs   += ltq_tantos_core_ll_table.o
  drv_switch_api-objs   += ltq_tantos_core_rml.o
#  drv_switch_api-objs   += ifx_ethsw_rml.o
  drv_switch_api-objs   += ltq_tantos_ral.o
ifeq ($(CONFIG_IFX_ETHSW_API_COC),y)
  EXTRA_CFLAGS += -DIFX_ETHSW_API_COC
  drv_switch_api-objs	+= ltq_ethsw_pm.o
  drv_switch_api-objs	+= ltq_ethsw_pm_plat.o
ifeq ($(CONFIG_IFX_ETHSW_API_COC_PMCU),y)
  EXTRA_CFLAGS += -DIFX_ETHSW_API_COC_PMCU
  drv_switch_api-objs	+= ltq_ethsw_pm_pmcu.o
endif
endif
  drv_switch_api-objs   += ltq_ar9_rml_table.o
  drv_switch_api-objs   += ltq_Tantos_rml_table.o
endif

##################### VR9 ######################
ifeq ($(CONFIG_VR9),y)
  EXTRA_CFLAGS += -DVR9
  EXTRA_CFLAGS += -DGSWIP
  drv_switch_api-objs   += ltq_xrx_platform.o
  drv_switch_api-objs   += ltq_flow_core.o
  drv_switch_api-objs   += ltq_flow_core_ll_table.o
  drv_switch_api-objs   += ltq_flow_pce.o
  drv_switch_api-objs   += ltq_flow_ral.o
ifeq ($(CONFIG_IFX_ETHSW_API_COC),y)
  EXTRA_CFLAGS += -DIFX_ETHSW_API_COC
  drv_switch_api-objs	+= ltq_ethsw_pm.o
  drv_switch_api-objs	+= ltq_ethsw_pm_plat.o
ifeq ($(CONFIG_IFX_ETHSW_API_COC_PMCU),y)
  EXTRA_CFLAGS += -DIFX_ETHSW_API_COC_PMCU
  drv_switch_api-objs	+= ltq_ethsw_pm_pmcu.o
endif
endif
endif

##################### AR10 ######################
ifeq ($(CONFIG_AR10),y)
  EXTRA_CFLAGS += -DAR10
  EXTRA_CFLAGS += -DGSWIP
  drv_switch_api-objs   += ltq_xrx_platform.o
  drv_switch_api-objs   += ltq_flow_core.o
  drv_switch_api-objs   += ltq_flow_core_ll_table.o
  drv_switch_api-objs   += ltq_flow_pce.o
  drv_switch_api-objs   += ltq_flow_ral.o
endif


##################### HN1 ######################            
ifeq ($(CONFIG_HN1),y)                                      
ifeq ($(CONFIG_LTQ_SWITCH_VPE1),y)
#  EXTRA_CFLAGS += -I$(TOPDIR)/drivers/char                  
  EXTRA_CFLAGS += -I$(srctree)/drivers/char
  EXTRA_CFLAGS += -DHN1                                     
  drv_switch_api-objs   += ltq_ethsw_vpe1_mailbox.o                     
else
  EXTRA_CFLAGS += -DVR9
  drv_switch_api-objs   += ltq_xrx_platform.o
  drv_switch_api-objs   += ltq_flow_core.o
  drv_switch_api-objs   += ltq_flow_core_ll_table.o
  drv_switch_api-objs   += ltq_flow_pce.o
  drv_switch_api-objs   += ltq_flow_ral.o
endif                 
endif    
