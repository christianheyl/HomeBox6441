/****************************************************************************
                              Copyright (c) 2010
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

 *****************************************************************************
   \file ifx_ethsw_flow_api.c
   \remarks implement SWITCH API declared on ifx_ethsw_flow_ll.h
 *****************************************************************************/
#include <ltq_flow_core.h>
#include <ltq_flow_pce.h>
#include <ltq_ethsw_flow_ll.h>
#include <ltq_xrx_platform.h>
#ifdef IFX_ETHSW_API_COC
	#include <ltq_ethsw_pm.h>
#endif
#define MULTI_PCE_RULES_INDEX	40
#define FLOW_TABLE_DEBUG	0
/********************/
/* Global Variables */
/********************/
IFX_FLOW_switchDev_t *pCoreDev[IFX_FLOW_DEV_MAX];
IFX_uint8_t VERSION_NAME[]= SWITCH_API_MODULE_NAME " for GSWITCH Platform";
IFX_uint8_t VERSION_NUMBER[] = SWITCH_API_DRIVER_VERSION;
IFX_uint8_t MICRO_CODE_VERSION_NAME[]= " GSWIP macro revision ID";
IFX_uint8_t MICRO_CODE_VERSION_NUMBER[] = "0x"MICRO_CODE_VERSION;
static IFX_uint8_t PortRedirectFlag = 0;

/******************************/
/* Local Macros & Definitions */
/******************************/
static PORT_STATE_t port_state_table[] = {
	{ IFX_ETHSW_STP_PORT_STATE_FORWARD, IFX_ETHSW_8021X_PORT_STATE_AUTHORIZED, 1, IFX_FLOW_PORT_STATE_FORWARDING_ENABLE, 1 },
	{ IFX_ETHSW_STP_PORT_STATE_FORWARD, IFX_ETHSW_8021X_PORT_STATE_UNAUTHORIZED, 1, IFX_FLOW_PORT_STATE_LISTENING_ONLY, 1 },
	{ IFX_ETHSW_STP_PORT_STATE_FORWARD, IFX_ETHSW_8021X_PORT_STATE_RX_AUTHORIZED, 1, IFX_FLOW_PORT_STATE_RX_ENABLE_TX_DISABLE, 1 },
	{ IFX_ETHSW_STP_PORT_STATE_FORWARD, IFX_ETHSW_8021X_PORT_STATE_TX_AUTHORIZED, 1, IFX_FLOW_PORT_STATE_RX_DISABLE_TX_ENABLE, 1 },
	{ IFX_ETHSW_STP_PORT_STATE_DISABLE, IFX_ETHSW_8021X_PORT_STATE_AUTHORIZED, 0, IFX_FLOW_PORT_STATE_LISTENING_ONLY, 0 },
	{ IFX_ETHSW_STP_PORT_STATE_DISABLE, IFX_ETHSW_8021X_PORT_STATE_UNAUTHORIZED, 0, IFX_FLOW_PORT_STATE_LISTENING_ONLY, 0 },
	{ IFX_ETHSW_STP_PORT_STATE_DISABLE, IFX_ETHSW_8021X_PORT_STATE_RX_AUTHORIZED, 0, IFX_FLOW_PORT_STATE_LISTENING_ONLY, 0 },
	{ IFX_ETHSW_STP_PORT_STATE_DISABLE, IFX_ETHSW_8021X_PORT_STATE_TX_AUTHORIZED, 0, IFX_FLOW_PORT_STATE_LISTENING_ONLY, 0 },
	{ IFX_ETHSW_STP_PORT_STATE_LEARNING, IFX_ETHSW_8021X_PORT_STATE_AUTHORIZED, 1, IFX_FLOW_PORT_STATE_LEARNING, 1 },
	{ IFX_ETHSW_STP_PORT_STATE_LEARNING, IFX_ETHSW_8021X_PORT_STATE_UNAUTHORIZED, 1, IFX_FLOW_PORT_STATE_LEARNING, 1 },
	{ IFX_ETHSW_STP_PORT_STATE_LEARNING, IFX_ETHSW_8021X_PORT_STATE_RX_AUTHORIZED, 1, IFX_FLOW_PORT_STATE_LEARNING, 1 },
	{ IFX_ETHSW_STP_PORT_STATE_LEARNING, IFX_ETHSW_8021X_PORT_STATE_TX_AUTHORIZED, 1, IFX_FLOW_PORT_STATE_LEARNING, 1 },
	{ IFX_ETHSW_STP_PORT_STATE_BLOCKING, IFX_ETHSW_8021X_PORT_STATE_AUTHORIZED, 1, IFX_FLOW_PORT_STATE_LISTENING_ONLY, 0 },
	{ IFX_ETHSW_STP_PORT_STATE_BLOCKING, IFX_ETHSW_8021X_PORT_STATE_UNAUTHORIZED, 1, IFX_FLOW_PORT_STATE_LISTENING_ONLY, 0 },
	{ IFX_ETHSW_STP_PORT_STATE_BLOCKING, IFX_ETHSW_8021X_PORT_STATE_RX_AUTHORIZED, 1, IFX_FLOW_PORT_STATE_LISTENING_ONLY, 0 },
	{ IFX_ETHSW_STP_PORT_STATE_BLOCKING, IFX_ETHSW_8021X_PORT_STATE_TX_AUTHORIZED, 1, IFX_FLOW_PORT_STATE_LISTENING_ONLY, 0 }
};

static IFX_ETHSW_CapDesc_t CAP_Description[] = {
	{ IFX_ETHSW_CAP_TYPE_PORT,                "Number of physical Ethernet ports"},
	{ IFX_ETHSW_CAP_TYPE_VIRTUAL_PORT,        "Number of virtual Ethernet ports"},
	{ IFX_ETHSW_CAP_TYPE_BUFFER_SIZE,         "Size of internal packet memory [in Bytes]"},
	{ IFX_ETHSW_CAP_TYPE_SEGMENT_SIZE,        "Number of Segment size per device"},
	{ IFX_ETHSW_CAP_TYPE_PRIORITY_QUEUE,      "Number of priority queues per device"},
	{ IFX_ETHSW_CAP_TYPE_METER,               "Number of meter instances"},
	{ IFX_ETHSW_CAP_TYPE_RATE_SHAPER,         "Number of rate shaper instances"},
	{ IFX_ETHSW_CAP_TYPE_VLAN_GROUP,          "Number of VLAN groups that can be configured on the switch hardware"},
	{ IFX_ETHSW_CAP_TYPE_FID,                 "Number of Forwarding database IDs [FIDs]"},
	{ IFX_ETHSW_CAP_TYPE_MAC_TABLE_SIZE,      "Number of MAC table entries"},
	{ IFX_ETHSW_CAP_TYPE_MULTICAST_TABLE_SIZE,"Number of multicast level 3 hardware table entries"},
	{ IFX_ETHSW_CAP_TYPE_PPPOE_SESSION,       "Number of supported PPPoE sessions"},
	{ IFX_ETHSW_CAP_TYPE_LAST,                "Last Capability Index"}
};

/*****************/
/* Function Body */
/*****************/
#if defined (CONFIG_LTQ_MULTICAST ) && CONFIG_LTQ_MULTICAST
#if defined(FLOW_TABLE_DEBUG) && FLOW_TABLE_DEBUG
IFX_return_t IFX_FLOW_MULTICAST_SW_Table_Print(IFX_void_t)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pData;
	IFX_void_t *pDevCtx =IFX_NULL;
	IFX_uint8_t i ;

	IFXOS_PRINT_INT_RAW("|======MULTICAST SW Table =======|\n");
	for (i = 0; i < 64; i++) {
		pData.table = IFX_ETHSW_PCE_MULTICAST_SW_INDEX;
		pData.table_index = i;
		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pData);
		if (pData.valid == 1) {
			/* Print out debug info first */
			IFXOS_PRINT_INT_RAW("\t|Index = %d | SIP_LSB = %03d | DIP_LSB = %03d | PortMAP = %02x\n",	\
				i, (pData.key[1] & 0x3F), (pData.key[0] & 0x3F), pData.val[0]);
		}
	}
	return IFX_SUCCESS;
}   /* -----  end of function IP DA/SA LSB Table  ----- */
#endif /* FLOW_TABLE_DEBUG */
#endif /* CONFIG_LTQ_MULTICAST */
static void IFX_FLOW_PortCfg_Init (IFX_void_t *pDevCtx)
{
	IFX_FLOW_switchDev_t *pEthSWDev = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint32_t i, value;

	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_1_PPORTS_OFFSET,	\
			VR9_ETHSW_CAP_1_PPORTS_SHIFT, VR9_ETHSW_CAP_1_PPORTS_SIZE, &value);
	pEthSWDev->nPortNumber = value;
	for ( i = 0; i < pEthSWDev->nPortNumber; i++ ) {
		memset(&pEthSWDev->PortConfig[i], 0 , sizeof(IFX_FLOW_portConfig_t));
		pEthSWDev->PortConfig[i].nLearningLimit = 0xFF;
		pEthSWDev->PortConfig[i].bPortEnable = 1;
	}
	pEthSWDev->STP_8021x_Config.eForwardPort = IFX_ETHSW_PORT_FORWARD_DEFAULT;
	pEthSWDev->STP_8021x_Config.n8021X_ForwardPortId = pEthSWDev->nCPU_Port/* VRX_PLATFORM_CPU_PORT */;
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_1_VPORTS_OFFSET,	\
			VR9_ETHSW_CAP_1_VPORTS_SHIFT, VR9_ETHSW_CAP_1_VPORTS_SIZE, &value);
	pEthSWDev->nTotalPortNumber = value + pEthSWDev->nPortNumber;
}   /* -----  end of function IFX_FLOW_PortCfg_Init  ----- */
#if defined (CONFIG_LTQ_MULTICAST ) && CONFIG_LTQ_MULTICAST
static void IFX_FLOW_MULTICAST_SW_Table_Init (IFX_void_t *pDevCtx)
{
	IFX_FLOW_switchDev_t *pEthSWDev = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint32_t i,value;

	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_8_MCAST_OFFSET,	\
			VR9_ETHSW_CAP_8_MCAST_SHIFT, VR9_ETHSW_CAP_8_MCAST_SIZE, &value);
	memset(&pEthSWDev->IGMP_Flags, 0 , sizeof(IFX_FLOW_IGMP_t));
	pEthSWDev->IGMP_Flags.nSwTblSize = value;
	for ( i = 0; i < pEthSWDev->IGMP_Flags.nSwTblSize; i++ ) {
		pEthSWDev->IGMP_Flags.multicast_sw_table[i].SrcIp_LSB_Index = 0x7F;
		pEthSWDev->IGMP_Flags.multicast_sw_table[i].DisIp_LSB_Index = 0x7F;
		pEthSWDev->IGMP_Flags.multicast_sw_table[i].SrcIp_MSB_Index = 0x1F;
		pEthSWDev->IGMP_Flags.multicast_sw_table[i].DisIp_MSB_Index = 0x1F;
	}

}   /* -----  end of function IFX_FLOW_MULTICAST_SW_Table_Init  ----- */

/* Multicast Software Table Include/Exclude Add function */
IFX_int32_t ifx_multicast_sw_table_write(IFX_void_t *pDevCtx, IFX_ETHSW_multicastTable_t *pPar)
{
	IFX_FLOW_switchDev_t *pDEVHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_int32_t	i,j, exclude_rule=IFX_FALSE;
	IFX_int32_t	Dip_lsb_tab_index = 0x7F,Dip_msb_tab_index = 0x7F,Sip_lsb_tab_index = 0x7F,Sip_msb_tab_index = 0x7F;
	IFX_int32_t dip_lsb_ind = 0, dip_msb_ind = 0, sip_lsb_ind =0, sip_msb_ind = 0;
	IFX_boolean_t new_entry=IFX_FALSE;
	IFX_void_t *pDev = IFX_NULL;
	IFX_PCE_t *pIPTmHandle = &pDEVHandle->PCE_Handler;
	IFX_FLOW_IGMP_t *pIGMPTbHandle = &pDEVHandle->IGMP_Flags;
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pData;
	IFX_PCE_IP_DASA_LSB_TBL_t	dasa_lsb_tbl;
	IFX_PCE_IP_DASA_MSB_TBL_t	dasa_msb_tbl;
	
	memset(&pData, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
	memset(&dasa_lsb_tbl, 0, sizeof(IFX_PCE_IP_DASA_LSB_TBL_t));
	memset(&dasa_msb_tbl, 0, sizeof(IFX_PCE_IP_DASA_MSB_TBL_t));
	if ((pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_INCLUDE)|| (pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_EXCLUDE)) {
		if (pDEVHandle->IGMP_Flags.bIGMPv3 != IFX_TRUE) {
			IFXOS_PRINT_INT_RAW(" ERROR: %s:%s:%d (The bIGMPv3 need to enable) \n", __FILE__, __FUNCTION__, __LINE__);
			return IFX_ERROR;
		}
	}
	if ( (pPar->eIPVersion != IFX_ETHSW_IP_SELECT_IPV4 ) && (pPar->eIPVersion != IFX_ETHSW_IP_SELECT_IPV6 ) ) {
		IFXOS_PRINT_INT_RAW(" ERROR: %s:%s:%d (IPv4/IPV6 need to enable) \n", __FILE__, __FUNCTION__, __LINE__);
		return IFX_ERROR;
	}
	if (pPar->eIPVersion == IFX_ETHSW_IP_SELECT_IPV4 /* IPv4 */) {
		/* First, search for DIP in the DA/SA table (DIP LSB) */
		for ( i = 0; i < 4 ; i++) {
			dasa_lsb_tbl.ip_lsb[i] = (( pPar->uIP_Gda.nIPv4 >> ( i * 8)) & 0xFF);
		}
		/* DIP LSB Nibble Mask */
		dasa_lsb_tbl.mask = 0xFF00;
	}
	if (pPar->eIPVersion == IFX_ETHSW_IP_SELECT_IPV6 /* IPv6 */) {
		/* First, search for DIP in the DA/SA table (DIP MSB) */
		for (i = 0, j = 7; i < 4; i++, j-=2) {
			dasa_msb_tbl.ip_msb[j-1]= (pPar->uIP_Gda.nIPv6[i] & 0xFF);
			dasa_msb_tbl.ip_msb[j]	= ((pPar->uIP_Gda.nIPv6[i] >> 8) & 0xFF);
		}
		dasa_msb_tbl.mask = 0;/* DIP MSB Nibble Mask */
		Dip_msb_tab_index = find_software_msb_tbl_entry(&pIPTmHandle->pce_sub_tbl, &dasa_msb_tbl);
		if (Dip_msb_tab_index == 0xFF ) {
			Dip_msb_tab_index = ifx_pce_tm_ip_dasa_msb_tbl_write(&pIPTmHandle->pce_sub_tbl,&dasa_msb_tbl);
			dip_msb_ind = 1;
		} 
		if (Dip_msb_tab_index < 0 ) {
			IFXOS_PRINT_INT_RAW(" ERROR: %s:%s:%d (IGMP Table full ) \n", __FILE__, __FUNCTION__, __LINE__);
			return IFX_ERROR;
		}
		/* First, search for DIP in the DA/SA table (DIP LSB) */
		for (i = 0, j =7; i < 4; i++, j-=2) {
			dasa_lsb_tbl.ip_lsb[j-1]= (pPar->uIP_Gda.nIPv6[i+4] & 0xFF);
			dasa_lsb_tbl.ip_lsb[j]	= ((pPar->uIP_Gda.nIPv6[i+4] >> 8) & 0xFF);
		}
		dasa_lsb_tbl.mask = 0;/* DIP LSB Nibble Mask */
	}
	Dip_lsb_tab_index = find_software_tbl_entry(&pIPTmHandle->pce_sub_tbl, &dasa_lsb_tbl);
	if (Dip_lsb_tab_index == 0xFF ) {
		Dip_lsb_tab_index = ifx_pce_tm_ip_dasa_lsb_tbl_write(&pIPTmHandle->pce_sub_tbl, &dasa_lsb_tbl);
		dip_lsb_ind = 1;
	}
	if (Dip_lsb_tab_index < 0 ) {
		IFXOS_PRINT_INT_RAW(" ERROR: %s:%s:%d (IGMP Table full ) \n", __FILE__, __FUNCTION__, __LINE__);
		return IFX_ERROR;
	}	
	if ( ( pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_INCLUDE) || 	\
		( pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_EXCLUDE) ) {
		if (pPar->eIPVersion == IFX_ETHSW_IP_SELECT_IPV4 ) {
			/* Second, search for SIP in the DA/SA table ( SIP LSB) */
			for ( i = 0; i < 4 ; i++) {
				dasa_lsb_tbl.ip_lsb[i] = (( pPar->uIP_Gsa.nIPv4 >> ( i * 8)) & 0xFF);
			}
			/* DIP LSB Nibble Mask */
			dasa_lsb_tbl.mask = 0xFF00;
			if ( pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_EXCLUDE) {
				if(dasa_lsb_tbl.ip_lsb[3] == 0 && dasa_lsb_tbl.ip_lsb[2] == 0 &&	\
	   				dasa_lsb_tbl.ip_lsb[1] == 0 && dasa_lsb_tbl.ip_lsb[0] == 0) {
	   				IFXOS_PRINT_INT_RAW(" ERROR: %s:%s:%d (Exclude Rule Source IP is Wildcard  ) \n", __FILE__, __FUNCTION__, __LINE__);
	   				return IFX_ERROR;
	   			}
	   		}
		}
		if (pPar->eIPVersion == IFX_ETHSW_IP_SELECT_IPV6) {
			int src_zero=0;
				/* First, search for DIP in the DA/SA table (DIP MSB) */
			for (i = 0,j=7; i < 4; i++,j-=2) {
				dasa_msb_tbl.ip_msb[j-1]= (pPar->uIP_Gsa.nIPv6[i] & 0xFF);
				dasa_msb_tbl.ip_msb[j]	= ((pPar->uIP_Gsa.nIPv6[i] >> 8) & 0xFF);
			}
			dasa_msb_tbl.mask = 0;/* DIP MSB Nibble Mask */
			if ( pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_EXCLUDE) {
				if(	dasa_msb_tbl.ip_msb[0] == 0 && dasa_msb_tbl.ip_msb[1] == 0 &&	\
	   				dasa_msb_tbl.ip_msb[2] == 0 && dasa_msb_tbl.ip_msb[3] == 0 && 	\
	   				dasa_msb_tbl.ip_msb[4] == 0 && dasa_msb_tbl.ip_msb[5] == 0 && 	\
	   				dasa_msb_tbl.ip_msb[6] == 0 && dasa_msb_tbl.ip_msb[7] == 0 	) {
	   					src_zero = 1;
	   			}
	   		}
			/* First, search for DIP in the DA/SA table (DIP LSB) */
			for (i = 0,j=7; i < 4; i++,j-=2) {
				dasa_lsb_tbl.ip_lsb[j-1]		= (pPar->uIP_Gsa.nIPv6[i+4] & 0xFF);
				dasa_lsb_tbl.ip_lsb[j]	= ((pPar->uIP_Gsa.nIPv6[i+4] >> 8) & 0xFF);
			}
			dasa_lsb_tbl.mask = 0;/* DIP LSB Nibble Mask */
			if ( pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_EXCLUDE) {
				if(	dasa_lsb_tbl.ip_lsb[0] == 0 && dasa_lsb_tbl.ip_lsb[1] == 0 &&	\
	   				dasa_lsb_tbl.ip_lsb[2] == 0 && dasa_lsb_tbl.ip_lsb[3] == 0 && 	\
	   				dasa_lsb_tbl.ip_lsb[4] == 0 && dasa_lsb_tbl.ip_lsb[5] == 0 && 	\
	   				dasa_lsb_tbl.ip_lsb[6] == 0 && dasa_lsb_tbl.ip_lsb[7] == 0 	) {
	   				if ( src_zero) {
	   					IFXOS_PRINT_INT_RAW(" ERROR: %s:%s:%d (Exclude Rule Source IP is Wildcard  ) \n", __FILE__, __FUNCTION__, __LINE__);
	   					return IFX_ERROR;
	   				}
	   			}
	   		}
			Sip_msb_tab_index = find_software_msb_tbl_entry(&pIPTmHandle->pce_sub_tbl, &dasa_msb_tbl);
			if (Sip_msb_tab_index == 0xFF ) {
				Sip_msb_tab_index = ifx_pce_tm_ip_dasa_msb_tbl_write(&pIPTmHandle->pce_sub_tbl,&dasa_msb_tbl);
				sip_msb_ind = 1;
			}
			if (Sip_msb_tab_index < 0 ) {
				IFXOS_PRINT_INT_RAW(" ERROR: %s:%s:%d (IGMP Table full ) \n", __FILE__, __FUNCTION__, __LINE__);
				return IFX_ERROR;
			}
		}
		Sip_lsb_tab_index = find_software_tbl_entry(&pIPTmHandle->pce_sub_tbl, &dasa_lsb_tbl);
		if (Sip_lsb_tab_index == 0xFF ) {
			Sip_lsb_tab_index = ifx_pce_tm_ip_dasa_lsb_tbl_write(&pIPTmHandle->pce_sub_tbl, &dasa_lsb_tbl);
			sip_lsb_ind = 1;
		}
		if (Sip_lsb_tab_index < 0 ) {
			IFXOS_PRINT_INT_RAW(" ERROR: %s:%s:%d (IGMP Table full ) \n", __FILE__, __FUNCTION__, __LINE__);
			return IFX_ERROR;
		}
	}
	/* update the entry for another port number if already exists*/
	for(i=0; i<pDEVHandle->IGMP_Flags.nSwTblSize; i++) {
		/* Check if port was already exist */
		if ((pIGMPTbHandle->multicast_sw_table[i].DisIp_LSB_Index == Dip_lsb_tab_index) &&	\
			(pIGMPTbHandle->multicast_sw_table[i].DisIp_MSB_Index == Dip_msb_tab_index) &&
			(pIGMPTbHandle->multicast_sw_table[i].SrcIp_LSB_Index == Sip_lsb_tab_index) &&	\
			(pIGMPTbHandle->multicast_sw_table[i].SrcIp_MSB_Index == Sip_msb_tab_index) &&
			(pIGMPTbHandle->multicast_sw_table[i].valid == IFX_TRUE)) {
			if (((pIGMPTbHandle->multicast_sw_table[i].PortMap >> pPar->nPortId) & 0x1 )== 1) {
				return IFX_SUCCESS;
			}
			switch (pIGMPTbHandle->multicast_sw_table[i].eModeMember ) {
				case IFX_ETHSW_IGMP_MEMBER_DONT_CARE:
					pIPTmHandle->pce_sub_tbl.ip_dasa_lsb_tbl_cnt[Dip_lsb_tab_index]++;
					if (pPar->eIPVersion == IFX_ETHSW_IP_SELECT_IPV6 )
						pIPTmHandle->pce_sub_tbl.ip_dasa_msb_tbl_cnt[Dip_msb_tab_index]++;
					/* Add the port */
					pIGMPTbHandle->multicast_sw_table[i].PortMap |= (1 << pPar->nPortId);
					break;
				case IFX_ETHSW_IGMP_MEMBER_EXCLUDE:
					exclude_rule = IFX_TRUE;
				case IFX_ETHSW_IGMP_MEMBER_INCLUDE:
					/* Add the port */
					pIGMPTbHandle->multicast_sw_table[i].PortMap |= (1 << pPar->nPortId);
					pIPTmHandle->pce_sub_tbl.ip_dasa_lsb_tbl_cnt[Dip_lsb_tab_index]++;
					pIPTmHandle->pce_sub_tbl.ip_dasa_lsb_tbl_cnt[Sip_lsb_tab_index]++;
					if (pPar->eIPVersion == IFX_ETHSW_IP_SELECT_IPV6 ) {
						pIPTmHandle->pce_sub_tbl.ip_dasa_msb_tbl_cnt[Dip_msb_tab_index]++;
						pIPTmHandle->pce_sub_tbl.ip_dasa_msb_tbl_cnt[Sip_msb_tab_index]++;
					}
					break;
			} /* end switch */
			/* Now, we write into Multicast SW Table */
			memset(&pData, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
			pData.table = IFX_ETHSW_PCE_MULTICAST_SW_INDEX;
			pData.table_index = i;
			pData.key[1] = (pIGMPTbHandle->multicast_sw_table[i].SrcIp_MSB_Index << 8)	\
				| pIGMPTbHandle->multicast_sw_table[i].SrcIp_LSB_Index;
			pData.key[0] = (pIGMPTbHandle->multicast_sw_table[i].DisIp_MSB_Index << 8)	\
				| pIGMPTbHandle->multicast_sw_table[i].DisIp_LSB_Index;
			if (pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_EXCLUDE)
				pData.val[0] = (0 << pPar->nPortId);
			else
				pData.val[0] = pIGMPTbHandle->multicast_sw_table[i].PortMap;
			pData.valid = pIGMPTbHandle->multicast_sw_table[i].valid;
			ifx_ethsw_xwayflow_pce_table_write(pDev, &pData);
			new_entry = IFX_TRUE;
			if (exclude_rule == IFX_FALSE ) {
				return IFX_SUCCESS;
			}
		}
	}
	
	/* wildcard entry for EXCLUDE rule for  port number if already exists*/
	if ((exclude_rule == IFX_TRUE) && (new_entry == IFX_TRUE) ) {
		for(i=0; i<pDEVHandle->IGMP_Flags.nSwTblSize; i++) {
			/* Check if port was already exist */
			if ((pIGMPTbHandle->multicast_sw_table[i].DisIp_LSB_Index == Dip_lsb_tab_index) &&	\
				(pIGMPTbHandle->multicast_sw_table[i].DisIp_MSB_Index == Dip_msb_tab_index) &&
				(pIGMPTbHandle->multicast_sw_table[i].SrcIp_LSB_Index == 0x7F) &&	\
				(pIGMPTbHandle->multicast_sw_table[i].SrcIp_MSB_Index == 0x7F) &&
				(pIGMPTbHandle->multicast_sw_table[i].valid == IFX_TRUE)) {
				if (((pIGMPTbHandle->multicast_sw_table[i].PortMap >> pPar->nPortId) & 0x1 )== 1) {
					return IFX_SUCCESS;
				} else {
					pIPTmHandle->pce_sub_tbl.ip_dasa_lsb_tbl_cnt[Dip_lsb_tab_index]++;
					if (pPar->eIPVersion == IFX_ETHSW_IP_SELECT_IPV6 )
						pIPTmHandle->pce_sub_tbl.ip_dasa_msb_tbl_cnt[Dip_msb_tab_index]++;
					/* Add the port */
					pIGMPTbHandle->multicast_sw_table[i].PortMap |= (1 << pPar->nPortId);
				}
				pIGMPTbHandle->multicast_sw_table[i].eModeMember = IFX_ETHSW_IGMP_MEMBER_DONT_CARE;
				memset(&pData, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
				pData.table = IFX_ETHSW_PCE_MULTICAST_SW_INDEX;
				pData.table_index = i;
				pData.key[1] = (pIGMPTbHandle->multicast_sw_table[i].SrcIp_MSB_Index << 8)	\
					| pIGMPTbHandle->multicast_sw_table[i].SrcIp_LSB_Index;
				pData.key[0] = (pIGMPTbHandle->multicast_sw_table[i].DisIp_MSB_Index << 8)	\
					| pIGMPTbHandle->multicast_sw_table[i].DisIp_LSB_Index;
				pData.val[0] = pIGMPTbHandle->multicast_sw_table[i].PortMap;
				pData.valid = pIGMPTbHandle->multicast_sw_table[i].valid;
				ifx_ethsw_xwayflow_pce_table_write(pDev, &pData);
				return IFX_SUCCESS;
						
			}
		}
	}
/* Create the new DstIP & SrcIP entry */
	if (new_entry == IFX_FALSE ) {
		if ( (pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_INCLUDE) ||	\
				(pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_EXCLUDE) )	{
			i = 0;
			while ( i < pDEVHandle->IGMP_Flags.nSwTblSize ) {
				/* Find a new empty entry to add */
				if (pIGMPTbHandle->multicast_sw_table[i].valid == IFX_FALSE)
					break;
				i++;
			}
		} else if (pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_DONT_CARE) {
			i = 63;
			while ( i > 0 ) {
				/* Find a new empty entry to add */
				if (pIGMPTbHandle->multicast_sw_table[i].valid == IFX_FALSE)
					break;
				i--;
			}
		}
		if ( i >= 0 && i < pDEVHandle->IGMP_Flags.nSwTblSize) {
			pIGMPTbHandle->multicast_sw_table[i].DisIp_LSB_Index = Dip_lsb_tab_index;
			pIGMPTbHandle->multicast_sw_table[i].DisIp_MSB_Index = Dip_msb_tab_index;
			pIGMPTbHandle->multicast_sw_table[i].PortMap |= (1 << pPar->nPortId);
			if (dip_lsb_ind) {
				pIPTmHandle->pce_sub_tbl.ip_dasa_lsb_tbl_cnt[Dip_lsb_tab_index] = 1;
			} else {
				pIPTmHandle->pce_sub_tbl.ip_dasa_lsb_tbl_cnt[Dip_lsb_tab_index]++;
			}
			if (pPar->eIPVersion == IFX_ETHSW_IP_SELECT_IPV6 ) {
				if (dip_msb_ind) {
					pIPTmHandle->pce_sub_tbl.ip_dasa_msb_tbl_cnt[Dip_msb_tab_index] = 1;
				} else {
					pIPTmHandle->pce_sub_tbl.ip_dasa_msb_tbl_cnt[Dip_msb_tab_index]++;
				}
			}
			pIGMPTbHandle->multicast_sw_table[i].valid = IFX_TRUE;
			pIGMPTbHandle->multicast_sw_table[i].eModeMember = pPar->eModeMember;
			if ( (pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_INCLUDE) ||	\
				(pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_EXCLUDE) ) {
				pIGMPTbHandle->multicast_sw_table[i].SrcIp_LSB_Index = Sip_lsb_tab_index;
				pIGMPTbHandle->multicast_sw_table[i].SrcIp_MSB_Index = Sip_msb_tab_index;
				if (sip_lsb_ind) {
					pIPTmHandle->pce_sub_tbl.ip_dasa_lsb_tbl_cnt[Sip_lsb_tab_index]=1;
				} else {
					pIPTmHandle->pce_sub_tbl.ip_dasa_lsb_tbl_cnt[Sip_lsb_tab_index]++;
				}
				if (pPar->eIPVersion == IFX_ETHSW_IP_SELECT_IPV6 ) {
					if ( sip_msb_ind) {
						pIPTmHandle->pce_sub_tbl.ip_dasa_msb_tbl_cnt[Sip_msb_tab_index]=1;
					} else {
						pIPTmHandle->pce_sub_tbl.ip_dasa_msb_tbl_cnt[Sip_msb_tab_index]++;
					}
				}
			} else if (pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_DONT_CARE) {
				pIGMPTbHandle->multicast_sw_table[i].SrcIp_LSB_Index = 0x7F;
				pIGMPTbHandle->multicast_sw_table[i].SrcIp_MSB_Index = 0x7F;
			}
		}
		memset(&pData, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
		/* Now, we write into Multicast SW Table */
		pData.table = IFX_ETHSW_PCE_MULTICAST_SW_INDEX;
		pData.table_index = i;
		pData.key[1] = (pIGMPTbHandle->multicast_sw_table[i].SrcIp_MSB_Index << 8) | pIGMPTbHandle->multicast_sw_table[i].SrcIp_LSB_Index;
		pData.key[0] = (pIGMPTbHandle->multicast_sw_table[i].DisIp_MSB_Index << 8) | pIGMPTbHandle->multicast_sw_table[i].DisIp_LSB_Index;
		if ( (pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_EXCLUDE) ) {
			pData.val[0] = (0 << pPar->nPortId);
		} else {
			pData.val[0] = pIGMPTbHandle->multicast_sw_table[i].PortMap;
		}
		pData.valid = pIGMPTbHandle->multicast_sw_table[i].valid ;
		ifx_ethsw_xwayflow_pce_table_write(pDev, &pData);
		
		if ( (pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_EXCLUDE) ) {
				for(i=0; i<pDEVHandle->IGMP_Flags.nSwTblSize; i++) {
				/* Check if port was already exist */
				if ((pIGMPTbHandle->multicast_sw_table[i].DisIp_LSB_Index == Dip_lsb_tab_index) &&	\
					(pIGMPTbHandle->multicast_sw_table[i].DisIp_MSB_Index == Dip_msb_tab_index) &&
					(pIGMPTbHandle->multicast_sw_table[i].SrcIp_LSB_Index == 0x7F) &&	\
					(pIGMPTbHandle->multicast_sw_table[i].SrcIp_MSB_Index == 0x7F) &&
					(pIGMPTbHandle->multicast_sw_table[i].valid == IFX_TRUE)) {
					if (((pIGMPTbHandle->multicast_sw_table[i].PortMap >> pPar->nPortId) & 0x1 )== 1) {
		  				return IFX_SUCCESS;
		  			}
		  			pIPTmHandle->pce_sub_tbl.ip_dasa_lsb_tbl_cnt[Dip_lsb_tab_index]++;
					if (pPar->eIPVersion == IFX_ETHSW_IP_SELECT_IPV6 )
						pIPTmHandle->pce_sub_tbl.ip_dasa_msb_tbl_cnt[Dip_msb_tab_index]++;
					pIGMPTbHandle->multicast_sw_table[i].eModeMember = IFX_ETHSW_IGMP_MEMBER_DONT_CARE;
					/* Add the port */
					pIGMPTbHandle->multicast_sw_table[i].PortMap |= (1 << pPar->nPortId);
					memset(&pData, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
					pData.table = IFX_ETHSW_PCE_MULTICAST_SW_INDEX;
					pData.table_index = i;
					pData.key[1] = (pIGMPTbHandle->multicast_sw_table[i].SrcIp_MSB_Index << 8)	\
						| pIGMPTbHandle->multicast_sw_table[i].SrcIp_LSB_Index;
					pData.key[0] = (pIGMPTbHandle->multicast_sw_table[i].DisIp_MSB_Index << 8)	\
						| pIGMPTbHandle->multicast_sw_table[i].DisIp_LSB_Index;
					pData.val[0] = pIGMPTbHandle->multicast_sw_table[i].PortMap;
					pData.valid = pIGMPTbHandle->multicast_sw_table[i].valid ;
					ifx_ethsw_xwayflow_pce_table_write(pDev, &pData);
					return IFX_SUCCESS;	
				}
			}
			i = 63;
			while ( i > 0 ) {
				/* Find a new empty entry to add */
				if (pIGMPTbHandle->multicast_sw_table[i].valid == IFX_FALSE)
					break;
				i--;
			}
		if ( i >= 0 && i < pDEVHandle->IGMP_Flags.nSwTblSize) {
				/* Now, we write into Multicast SW Table */
			pIGMPTbHandle->multicast_sw_table[i].DisIp_LSB_Index = Dip_lsb_tab_index;
			pIGMPTbHandle->multicast_sw_table[i].DisIp_MSB_Index = Dip_msb_tab_index;
			pIGMPTbHandle->multicast_sw_table[i].SrcIp_LSB_Index = 0x7F;
			pIGMPTbHandle->multicast_sw_table[i].SrcIp_MSB_Index = 0x7F;
			pIGMPTbHandle->multicast_sw_table[i].PortMap |= (1 << pPar->nPortId);
			pIGMPTbHandle->multicast_sw_table[i].eModeMember = IFX_ETHSW_IGMP_MEMBER_DONT_CARE;
			pIGMPTbHandle->multicast_sw_table[i].valid = IFX_TRUE;
			pIPTmHandle->pce_sub_tbl.ip_dasa_lsb_tbl_cnt[Dip_lsb_tab_index]++;
			if (pPar->eIPVersion == IFX_ETHSW_IP_SELECT_IPV6 )
				pIPTmHandle->pce_sub_tbl.ip_dasa_msb_tbl_cnt[Dip_msb_tab_index]++;
			memset(&pData, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
			pData.table = IFX_ETHSW_PCE_MULTICAST_SW_INDEX;
			pData.table_index = i;
			pData.key[1] = (pIGMPTbHandle->multicast_sw_table[i].SrcIp_MSB_Index << 8) | pIGMPTbHandle->multicast_sw_table[i].SrcIp_LSB_Index;
			pData.key[0] = (pIGMPTbHandle->multicast_sw_table[i].DisIp_MSB_Index << 8) | pIGMPTbHandle->multicast_sw_table[i].DisIp_LSB_Index;
			pData.val[0] = pIGMPTbHandle->multicast_sw_table[i].PortMap;
			pData.valid = pIGMPTbHandle->multicast_sw_table[i].valid ;
			ifx_ethsw_xwayflow_pce_table_write(pDev, &pData);
			} else {
				IFXOS_PRINT_INT_RAW(" ERROR: %s:%s:%d (IGMP Table full ) \n", __FILE__, __FUNCTION__, __LINE__);
			}
		}
	}
	/* Debug */
	return IFX_SUCCESS;
}

/* Multicast Software Table Include/Exclude Remove function */
IFX_int32_t ifx_multicast_sw_table_remove(IFX_void_t *pDevCtx, IFX_ETHSW_multicastTable_t *pPar)
{
	IFX_FLOW_switchDev_t *pDEVHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint8_t	i, j;
	IFX_boolean_t   MATCH = IFX_FALSE;
	IFX_void_t *pDev = IFX_NULL;
	IFX_PCE_t *pIPTmHandle = &pDEVHandle->PCE_Handler;
	IFX_FLOW_IGMP_t *pIGMPTbHandle = &pDEVHandle->IGMP_Flags;
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pData;
	IFX_PCE_IP_DASA_LSB_TBL_t  dasa_lsb_tbl;
	IFX_PCE_IP_DASA_MSB_TBL_t	dasa_msb_tbl;
	IFX_int32_t   Dip_lsb_tab_index = 0x7F,Dip_msb_tab_index = 0x7F,	\
		Sip_lsb_tab_index = 0x7F,Sip_msb_tab_index = 0x7F;
	memset(&pData, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
	memset(&dasa_lsb_tbl, 0, sizeof(IFX_PCE_IP_DASA_LSB_TBL_t));
	memset(&dasa_msb_tbl, 0, sizeof(IFX_PCE_IP_DASA_MSB_TBL_t));
	
	if ( (pPar->eIPVersion != IFX_ETHSW_IP_SELECT_IPV4 ) && (pPar->eIPVersion != IFX_ETHSW_IP_SELECT_IPV6 ) ) {
		IFXOS_PRINT_INT_RAW(" ERROR: %s:%s:%d (IPv4/IPV6 need to enable!!!) \n", __FILE__, __FUNCTION__, __LINE__);
		return IFX_ERROR;
	}
	if ((pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_INCLUDE) &&	\
		(pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_EXCLUDE) &&	\
		(pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_DONT_CARE)) {
		IFXOS_PRINT_INT_RAW(" ERROR: %s:%s:%d (The Mode Member ERROR !!!) \n", __FILE__, __FUNCTION__, __LINE__);
		return IFX_ERROR;
	}
	if (pPar->eIPVersion == IFX_ETHSW_IP_SELECT_IPV4 /* IPv4 */) {
		/* First, search for DIP in the DA/SA table (DIP LSB) */
		for ( i = 0; i < 4 ; i++) {
			dasa_lsb_tbl.ip_lsb[i] = (( pPar->uIP_Gda.nIPv4 >> ( i * 8)) & 0xFF);
		}
		/* DIP LSB Nibble Mask */
		dasa_lsb_tbl.mask = 0xFF00;
	}
	if (pPar->eIPVersion == IFX_ETHSW_IP_SELECT_IPV6 /* IPv6 */) {
		/* First, search for DIP in the DA/SA table (DIP MSB) */
		for (i = 0, j = 7; i < 4; i++, j-=2) {
			dasa_msb_tbl.ip_msb[j-1]= (pPar->uIP_Gda.nIPv6[i] & 0xFF);
			dasa_msb_tbl.ip_msb[j]	= ((pPar->uIP_Gda.nIPv6[i] >> 8) & 0xFF);
		}
		dasa_msb_tbl.mask = 0;/* DIP MSB Nibble Mask */
		Dip_msb_tab_index = find_software_msb_tbl_entry(&pIPTmHandle->pce_sub_tbl, &dasa_msb_tbl);
		if (Dip_msb_tab_index == 0xFF) {
			IFXOS_PRINT_INT_RAW(" ERROR: %s:%s:%d (IGMP Entry not found  ) \n", __FILE__, __FUNCTION__, __LINE__);
			return IFX_ERROR;
		}
		/* First, search for DIP in the DA/SA table (DIP LSB) */
		for (i = 0, j =7; i < 4; i++, j-=2) {
			dasa_lsb_tbl.ip_lsb[j-1]= (pPar->uIP_Gda.nIPv6[i+4] & 0xFF);
			dasa_lsb_tbl.ip_lsb[j]	= ((pPar->uIP_Gda.nIPv6[i+4] >> 8) & 0xFF);
		}
		dasa_lsb_tbl.mask = 0;/* DIP LSB Nibble Mask */
	}
	Dip_lsb_tab_index = find_software_tbl_entry(&pIPTmHandle->pce_sub_tbl, &dasa_lsb_tbl);
	if (Dip_lsb_tab_index == 0xFF) {
		IFXOS_PRINT_INT_RAW(" ERROR: %s:%s:%d (IGMP Entry not found  ) \n", __FILE__, __FUNCTION__, __LINE__);
		return IFX_ERROR;
	}
	if ((pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_INCLUDE) ||	\
		(pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_EXCLUDE)) {
		if (pPar->eIPVersion == IFX_ETHSW_IP_SELECT_IPV4 /* IPv4 */) {
			/* First, search for DIP in the DA/SA table (DIP LSB) */
			for ( i = 0; i < 4 ; i++) {
				dasa_lsb_tbl.ip_lsb[i] = (( pPar->uIP_Gsa.nIPv4 >> ( i * 8)) & 0xFF);
			}
			/* DIP LSB Nibble Mask */
			dasa_lsb_tbl.mask = 0xFF00;
			if ( pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_EXCLUDE) {
				if(dasa_lsb_tbl.ip_lsb[3] == 0 && dasa_lsb_tbl.ip_lsb[2] == 0 &&	\
	   				dasa_lsb_tbl.ip_lsb[1] == 0 && dasa_lsb_tbl.ip_lsb[0] == 0) {
	   				IFXOS_PRINT_INT_RAW(" ERROR: %s:%s:%d (Exclude Rule Source IP is Wildcard  ) \n", __FILE__, __FUNCTION__, __LINE__);
	   				return IFX_ERROR;
	   			}
	   		}
		}
		if (pPar->eIPVersion == IFX_ETHSW_IP_SELECT_IPV6 /* IPv6 */) {
			int src_zero=0;
			/* First, search for DIP in the DA/SA table (DIP MSB) */
			for (i = 0,j=7; i < 4; i++,j-=2) {
				dasa_msb_tbl.ip_msb[j-1]= (pPar->uIP_Gsa.nIPv6[i] & 0xFF);
				dasa_msb_tbl.ip_msb[j]	= ((pPar->uIP_Gsa.nIPv6[i] >> 8) & 0xFF);
			}
			dasa_msb_tbl.mask = 0;/* DIP MSB Nibble Mask */
			if ( pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_EXCLUDE) {
				if(	dasa_msb_tbl.ip_msb[0] == 0 && dasa_msb_tbl.ip_msb[1] == 0 &&	\
	   				dasa_msb_tbl.ip_msb[2] == 0 && dasa_msb_tbl.ip_msb[3] == 0 && 	\
	   				dasa_msb_tbl.ip_msb[4] == 0 && dasa_msb_tbl.ip_msb[5] == 0 && 	\
	   				dasa_msb_tbl.ip_msb[6] == 0 && dasa_msb_tbl.ip_msb[7] == 0 	) {
	   					src_zero = 1;
	   			}
	   		}
			Sip_msb_tab_index = find_software_msb_tbl_entry(&pIPTmHandle->pce_sub_tbl, &dasa_msb_tbl);
			if (Sip_msb_tab_index == 0xFF) {
				IFXOS_PRINT_INT_RAW(" ERROR: %s:%s:%d (IGMP Entry not found  ) \n", __FILE__, __FUNCTION__, __LINE__);
				return IFX_ERROR;
			}
			/* First, search for DIP in the DA/SA table (DIP LSB) */
			for (i = 0,j=7; i < 4; i++,j-=2) {
				dasa_lsb_tbl.ip_lsb[j-1]		= (pPar->uIP_Gsa.nIPv6[i+4] & 0xFF);
				dasa_lsb_tbl.ip_lsb[j]	= ((pPar->uIP_Gsa.nIPv6[i+4] >> 8) & 0xFF);
			}
			dasa_lsb_tbl.mask = 0;/* DIP LSB Nibble Mask */
			if ( pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_EXCLUDE) {
				if(	dasa_lsb_tbl.ip_lsb[0] == 0 && dasa_lsb_tbl.ip_lsb[1] == 0 &&	\
	   				dasa_lsb_tbl.ip_lsb[2] == 0 && dasa_lsb_tbl.ip_lsb[3] == 0 && 	\
	   				dasa_lsb_tbl.ip_lsb[4] == 0 && dasa_lsb_tbl.ip_lsb[5] == 0 && 	\
	   				dasa_lsb_tbl.ip_lsb[6] == 0 && dasa_lsb_tbl.ip_lsb[7] == 0 	) {
	   				if ( src_zero) {
	   					IFXOS_PRINT_INT_RAW(" ERROR: %s:%s:%d (Exclude Rule Source IP is Wildcard  ) \n", __FILE__, __FUNCTION__, __LINE__);
	   					return IFX_ERROR;
	   				}
	   			}
	   		}
		}
		Sip_lsb_tab_index = find_software_tbl_entry(&pIPTmHandle->pce_sub_tbl, &dasa_lsb_tbl);
		if (Sip_lsb_tab_index == 0xFF) {
			IFXOS_PRINT_INT_RAW(" ERROR: %s:%s:%d (IGMP Entry not found  ) \n", __FILE__, __FUNCTION__, __LINE__);
			return IFX_ERROR;
			
		}
	}
	for ( i = 0; i < pDEVHandle->IGMP_Flags.nSwTblSize; i++ ) {
		if ((pIGMPTbHandle->multicast_sw_table[i].DisIp_LSB_Index == Dip_lsb_tab_index) &&	\
			(pIGMPTbHandle->multicast_sw_table[i].SrcIp_LSB_Index == Sip_lsb_tab_index) &&	\
			(pIGMPTbHandle->multicast_sw_table[i].DisIp_MSB_Index == Dip_msb_tab_index) &&	\
			(pIGMPTbHandle->multicast_sw_table[i].SrcIp_MSB_Index == Sip_msb_tab_index) &&	\
			( pIGMPTbHandle->multicast_sw_table[i].valid == IFX_TRUE)) {
			switch (pIGMPTbHandle->multicast_sw_table[i].eModeMember ) {
				case IFX_ETHSW_IGMP_MEMBER_DONT_CARE:
					if (((pIGMPTbHandle->multicast_sw_table[i].PortMap >> pPar->nPortId) & 0x1 )== 1) {
						pIGMPTbHandle->multicast_sw_table[i].PortMap &= ~(1 << pPar->nPortId);
						if (pIPTmHandle->pce_sub_tbl.ip_dasa_lsb_tbl_cnt[Dip_lsb_tab_index] > 0) {
							ifx_pce_tm_ip_dasa_lsb_tbl_idx_delete(&pIPTmHandle->pce_sub_tbl, Dip_lsb_tab_index);
							if (pIPTmHandle->pce_sub_tbl.ip_dasa_lsb_tbl_cnt[Dip_lsb_tab_index] == 0) {
								/* Delet the sub table */
								ifx_pce_tm_ip_dasa_lsb_tbl_delete(&pIPTmHandle->pce_sub_tbl, Dip_lsb_tab_index);
							}
						}
						if (pIPTmHandle->pce_sub_tbl.ip_dasa_msb_tbl_cnt[Dip_msb_tab_index] > 0) {
							ifx_pce_tm_ip_dasa_msb_tbl_idx_delete(&pIPTmHandle->pce_sub_tbl, Dip_msb_tab_index);
							if (pIPTmHandle->pce_sub_tbl.ip_dasa_msb_tbl_cnt[Dip_msb_tab_index] == 0) {
							/* Delet the sub table */
							if (pPar->eIPVersion == IFX_ETHSW_IP_SELECT_IPV6 /* IPv6 */)
								ifx_pce_tm_ip_dasa_msb_tbl_delete(&pIPTmHandle->pce_sub_tbl, Dip_msb_tab_index);
							}
						}
						/* Check the port map status */
						if (pIGMPTbHandle->multicast_sw_table[i].PortMap == 0) {
							/* Delet the entry from Multicast sw Table */
							pIGMPTbHandle->multicast_sw_table[i].valid = IFX_FALSE;
						}
						
						MATCH = IFX_TRUE;
					}
						break;
				case IFX_ETHSW_IGMP_MEMBER_INCLUDE:
				case IFX_ETHSW_IGMP_MEMBER_EXCLUDE:
					if (((pIGMPTbHandle->multicast_sw_table[i].PortMap >> pPar->nPortId) & 0x1 )== 1) {
						pIGMPTbHandle->multicast_sw_table[i].PortMap &= ~(1 << pPar->nPortId);
						if (pIPTmHandle->pce_sub_tbl.ip_dasa_lsb_tbl_cnt[Dip_lsb_tab_index] > 0) {
							ifx_pce_tm_ip_dasa_lsb_tbl_idx_delete(&pIPTmHandle->pce_sub_tbl, Dip_lsb_tab_index);
							if (pIPTmHandle->pce_sub_tbl.ip_dasa_lsb_tbl_cnt[Dip_lsb_tab_index] == 0) {
								/* Delet the sub table */
								ifx_pce_tm_ip_dasa_lsb_tbl_delete(&pIPTmHandle->pce_sub_tbl, Dip_lsb_tab_index);
							}
						}
						if (pIPTmHandle->pce_sub_tbl.ip_dasa_msb_tbl_cnt[Dip_msb_tab_index] > 0) {
							ifx_pce_tm_ip_dasa_msb_tbl_idx_delete(&pIPTmHandle->pce_sub_tbl, Dip_msb_tab_index);
							if (pIPTmHandle->pce_sub_tbl.ip_dasa_msb_tbl_cnt[Dip_msb_tab_index] == 0) {
							/* Delet the sub table */
								ifx_pce_tm_ip_dasa_msb_tbl_delete(&pIPTmHandle->pce_sub_tbl, Dip_msb_tab_index);
							}
						}
						if (pIPTmHandle->pce_sub_tbl.ip_dasa_lsb_tbl_cnt[Sip_lsb_tab_index] > 0) {
							ifx_pce_tm_ip_dasa_lsb_tbl_idx_delete(&pIPTmHandle->pce_sub_tbl, Sip_lsb_tab_index);
							if (pIPTmHandle->pce_sub_tbl.ip_dasa_lsb_tbl_cnt[Sip_lsb_tab_index] == 0) {
								/* Delet the sub table */
								ifx_pce_tm_ip_dasa_lsb_tbl_delete(&pIPTmHandle->pce_sub_tbl, Sip_lsb_tab_index);
							}
						}
						if (pIPTmHandle->pce_sub_tbl.ip_dasa_msb_tbl_cnt[Sip_msb_tab_index] > 0) {
							ifx_pce_tm_ip_dasa_msb_tbl_idx_delete(&pIPTmHandle->pce_sub_tbl, Sip_msb_tab_index);
							if (pIPTmHandle->pce_sub_tbl.ip_dasa_msb_tbl_cnt[Sip_msb_tab_index] == 0) {
							/* Delet the sub table */
								ifx_pce_tm_ip_dasa_msb_tbl_delete(&pIPTmHandle->pce_sub_tbl, Sip_msb_tab_index);
							}
						}
						/* Check the port map status */
						if (pIGMPTbHandle->multicast_sw_table[i].PortMap == 0) {
							/* Delet the entry from Multicast sw Table */
							pIGMPTbHandle->multicast_sw_table[i].valid = IFX_FALSE;
						}
						MATCH = IFX_TRUE;
						if (pPar->eModeMember == IFX_ETHSW_IGMP_MEMBER_EXCLUDE) {
							for ( j = 0; j < pDEVHandle->IGMP_Flags.nSwTblSize; j++ ) {
								if ((pIGMPTbHandle->multicast_sw_table[j].DisIp_LSB_Index == Dip_lsb_tab_index) &&	\
									(pIGMPTbHandle->multicast_sw_table[j].SrcIp_LSB_Index == 0x7F) &&	\
									(pIGMPTbHandle->multicast_sw_table[j].DisIp_MSB_Index == Dip_msb_tab_index) &&	\
									(pIGMPTbHandle->multicast_sw_table[j].SrcIp_MSB_Index == 0x7F) &&	\
									( pIGMPTbHandle->multicast_sw_table[j].valid == IFX_TRUE)) {
									if (((pIGMPTbHandle->multicast_sw_table[j].PortMap >> pPar->nPortId) & 0x1 )== 1) {
										pIGMPTbHandle->multicast_sw_table[j].PortMap &= ~(1 << pPar->nPortId);
										if (pIPTmHandle->pce_sub_tbl.ip_dasa_lsb_tbl_cnt[Dip_lsb_tab_index] > 0) {
											ifx_pce_tm_ip_dasa_lsb_tbl_idx_delete(&pIPTmHandle->pce_sub_tbl, Dip_lsb_tab_index);
											if (pIPTmHandle->pce_sub_tbl.ip_dasa_lsb_tbl_cnt[Dip_lsb_tab_index] == 0) {
												/* Delet the sub table */
												ifx_pce_tm_ip_dasa_lsb_tbl_delete(&pIPTmHandle->pce_sub_tbl, Dip_lsb_tab_index);
											}
										}
									if (pIPTmHandle->pce_sub_tbl.ip_dasa_msb_tbl_cnt[Dip_msb_tab_index] > 0) {
										ifx_pce_tm_ip_dasa_msb_tbl_idx_delete(&pIPTmHandle->pce_sub_tbl, Dip_msb_tab_index);
										if (pIPTmHandle->pce_sub_tbl.ip_dasa_msb_tbl_cnt[Dip_msb_tab_index] == 0) {
											/* Delet the sub table */
											ifx_pce_tm_ip_dasa_msb_tbl_delete(&pIPTmHandle->pce_sub_tbl, Dip_msb_tab_index);
										}
									}
									/* Check the port map status */
									if (pIGMPTbHandle->multicast_sw_table[j].PortMap == 0) {
										/* Delet the entry from Multicast sw Table */
										pIGMPTbHandle->multicast_sw_table[j].valid = IFX_FALSE;
										pIGMPTbHandle->multicast_sw_table[i].valid = IFX_FALSE;
									}
									memset(&pData, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
									pData.table = IFX_ETHSW_PCE_MULTICAST_SW_INDEX;
									pData.table_index = j;
									pData.key[1]= ((0x7F << 8) | 0x7F);
									pData.key[0]= (pIGMPTbHandle->multicast_sw_table[j].DisIp_MSB_Index << 8) | pIGMPTbHandle->multicast_sw_table[i].DisIp_LSB_Index;
									pData.val[0]= pIGMPTbHandle->multicast_sw_table[j].PortMap;
		 							pData.valid	= pIGMPTbHandle->multicast_sw_table[j].valid;
									ifx_ethsw_xwayflow_pce_table_write(pDev, &pData);
								}
							}
						}
					}
				}
					break;
				}
			if (MATCH == IFX_TRUE ) {
				memset(&pData, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
				pData.table = IFX_ETHSW_PCE_MULTICAST_SW_INDEX;
				pData.table_index = i;
				pData.key[1]= (pIGMPTbHandle->multicast_sw_table[i].SrcIp_MSB_Index << 8) | pIGMPTbHandle->multicast_sw_table[i].SrcIp_LSB_Index;
				pData.key[0]= (pIGMPTbHandle->multicast_sw_table[i].DisIp_MSB_Index << 8) | pIGMPTbHandle->multicast_sw_table[i].DisIp_LSB_Index;
				pData.val[0]= pIGMPTbHandle->multicast_sw_table[i].PortMap;
		 		pData.valid	= pIGMPTbHandle->multicast_sw_table[i].valid;
				ifx_ethsw_xwayflow_pce_table_write(pDev, &pData);
			}
		}
	}	
	if (MATCH == IFX_FALSE)
		IFXOS_PRINT_INT_RAW("The GIP/SIP not found\n");
   return IFX_SUCCESS;
}

#endif /*CONFIG_LTQ_MULTICAST */
#if  ( (defined (CONFIG_LTQ_STP ) && CONFIG_LTQ_STP ) || (defined (CONFIG_LTQ_8021X ) && CONFIG_LTQ_8021X ) )
/*
 *  Function:    Internal function to program the registers when 802.1x and STP API are called.
 *  Description: Referene the matrix table to program the LRNLIM, PSTATE and PEN bit
 *               according to the Software architecture spec design.
 */
static void set_port_state(IFX_void_t *pDevCtx, IFX_uint32_t PortID, IFX_uint32_t ifx_stp_state, IFX_uint32_t ifx_8021_state)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint32_t i;

	for (i = 0; i < sizeof(port_state_table)/sizeof(PORT_STATE_t); i++) {
		PORT_STATE_t *pTable = &port_state_table[i];
		if ((pTable->ifx_stp_state == ifx_stp_state) && (pTable->ifx_8021_state == ifx_8021_state)) {
			pEthSWDevHandle->PortConfig[PortID].bPortEnable = pTable->pen_reg;
			pEthSWDevHandle->PortConfig[PortID].nPortState = pTable->pstate_reg;	
			/* Learning Limit */
			if ( pTable->lrnlim == 0 ) {
				ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PCE_PCTRL_1_LRNLIM_OFFSET + (0xA * PortID)),	\
					VR9_PCE_PCTRL_1_LRNLIM_SHIFT, VR9_PCE_PCTRL_1_LRNLIM_SIZE,0);
			} else {
				ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PCE_PCTRL_1_LRNLIM_OFFSET + (0xA * PortID)),	\
					VR9_PCE_PCTRL_1_LRNLIM_SHIFT, VR9_PCE_PCTRL_1_LRNLIM_SIZE,	\
					pEthSWDevHandle->PortConfig[PortID].nLearningLimit);
			}
			/* Port State */
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PCE_PCTRL_0_PSTATE_OFFSET + (0xA * PortID)),	\
				VR9_PCE_PCTRL_0_PSTATE_SHIFT, VR9_PCE_PCTRL_0_PSTATE_SIZE,	\
				pEthSWDevHandle->PortConfig[PortID].nPortState);
			/* Port Enable */
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_SDMA_PCTRL_PEN_OFFSET + (0xA * PortID)),	\
				VR9_SDMA_PCTRL_PEN_SHIFT, VR9_SDMA_PCTRL_PEN_SIZE,	\
				pEthSWDevHandle->PortConfig[PortID].bPortEnable);
		}
	}
}
#endif /* CONFIG_LTQ_STP / CONFIG_LTQ_8021X */
#if defined (CONFIG_LTQ_QOS ) && CONFIG_LTQ_QOS
/*
 *  Function:    Internal function to calculate the Rate when Shaper and Meter API is called.
 *  Description: Calculate the Rate by input Ibs, Exp and Mant.
 *               The algorithm designed based on software architecture spec.
 */
static IFX_uint32_t RateCalc(IFX_uint32_t pIbsIdx, IFX_uint32_t pExp, IFX_uint32_t pMant)
{
	static const IFX_uint16_t ibs_table[] = {8*8, 32*8, 64*8, 96*8};
	IFX_uint16_t ibs;
	IFX_uint32_t Rate;

	if  ((pIbsIdx == 0) && (pExp == 0) && (pMant == 0))
		return 0;
	ibs = ibs_table[pIbsIdx];
	Rate = ((ibs * 25000) >> pExp) / pMant;
	return Rate;
}

/*
 *  Function:    Internal function to calculate the Token when Shaper and Meter API is called.
 *  Description: Calculate the Token by input Rate, Ibs, Exp and Mant.
 *               The algorithm designed based on software architecture spec.
 */
static IFX_return_t calcToken(IFX_uint32_t Rate, IFX_uint32_t *pIbsIdx, IFX_uint32_t *pExp, IFX_uint32_t *pMant)
{
	static const IFX_uint16_t ibs_table[] = {8*8, 32*8, 64*8, 96*8};
	IFX_uint8_t i;

	for (i = 3; i >= 0; i--) {
		IFX_uint32_t exp;
		IFX_uint16_t ibs = ibs_table[i];
		for (exp = 0; exp < 16; exp++) {
			IFX_uint32_t mant =  ((ibs * 25000) >> exp) / Rate ;
			if (mant < (1 << 10) )  {
				/* target is to get the biggest mantissa value that can be used for the 10-Bit register */
				*pIbsIdx = i;
				*pExp = exp;
				*pMant = mant;
				return IFX_SUCCESS;
			}
		}
	}
}
#endif /*CONFIG_LTQ_QOS */
#if defined (CONFIG_LTQ_VLAN ) && CONFIG_LTQ_VLAN
static void IFX_FLOW_VLAN_Table_Init ( IFX_void_t *pDevCtx)
{
	IFX_FLOW_switchDev_t *pEthSWDev = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint16_t i;

	for ( i = 0; i < IFX_FLOW_VLAN_ENTRY_MAX; i++ ) {
		memset(&pEthSWDev->VLAN_Table[i], 0, sizeof(IFX_FLOW_VLAN_tableEntry_t));
	}
}   /* -----  end of function IFX_FLOW_VLAN_Table_Init  ----- */
#endif /*CONFIG_LTQ_VLAN*/
#if defined(FLOW_TABLE_DEBUG) && FLOW_TABLE_DEBUG
void IFX_FLOW_PCE_TRAFFIC_Table_Print(IFX_void_t)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pData;
	IFX_void_t *pDevCtx =IFX_NULL;
	IFX_uint8_t i;

	IFXOS_PRINT_INT_RAW("|======PCE Traffic FLOW Table =======|\n");
	for (i = 0; i < 64; i++) {
		pData.table = IFX_ETHSW_PCE_TFLOW_INDEX;
		pData.table_index = i;
		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pData);
		if (pData.valid == 1) {
			/* Print out debug info first */
			IFXOS_PRINT_INT_RAW("=================================================================================\n");
			IFXOS_PRINT_INT_RAW("\t|Index = %03x|\n", i);
			IFXOS_PRINT_INT_RAW("\t|PPPoE index = %03x|Packet length index = %03x|\n", (pData.key[7]>>8) & 0xFF, pData.key[7] & 0xFF);
			IFXOS_PRINT_INT_RAW("\t|DSCP value  = %03x|PCP value           = %03x|\n", (pData.key[6]>>8) & 0x7F, pData.key[6] & 0xF);
			IFXOS_PRINT_INT_RAW("\t|Des MAC     = %03x|Src MAC             = %03x|\n", (pData.key[5]>>8) & 0xFF, pData.key[5] & 0xFF);
			IFXOS_PRINT_INT_RAW("\t|Des APP     = %03x|Src APP             = %03x|\n", (pData.key[4]>>8) & 0xFF, pData.key[4] & 0xFF);
			IFXOS_PRINT_INT_RAW("\t|DIP MSB     = %03x|DPI LSB             = %03x|\n", (pData.key[3]>>8) & 0xFF, pData.key[3] & 0xFF);
			IFXOS_PRINT_INT_RAW("\t|SIP MSB     = %03x|SPI LSB             = %03x|\n", (pData.key[2]>>8) & 0x1F, pData.key[2] & 0xFF);
			IFXOS_PRINT_INT_RAW("\t|IP Protocol = %03x|Enter Type          = %03x|\n", (pData.key[1]>>8) & 0xFF, pData.key[1] & 0xFF);
			IFXOS_PRINT_INT_RAW("\t|VLAN index  = %03x|Port ID             = %03x|\n\n",(pData.key[0]>>8) & 0xFF, pData.key[0] & 0xFF);

			IFXOS_PRINT_INT_RAW("\t|Port map act= %03x|VLAN action         = %03x|\n", pData.val[0] & 0x1, pData.val[0] & (1 << 1));
			IFXOS_PRINT_INT_RAW("\t|Traffic act = %03x|Remarking action    = %03x|\n", pData.val[0] & (1 << 2), pData.val[0] & (1 << 3));
			IFXOS_PRINT_INT_RAW("\t|Cross VLAN  = %03x|Cross state action  = %03x|\n", pData.val[0] & (1 << 4), pData.val[0] & (1 << 5));
			IFXOS_PRINT_INT_RAW("\t|Critical act= %03x|Time stamp action   = %03x|\n", pData.val[0] & (1 << 6), pData.val[0] & (1 << 7));
			IFXOS_PRINT_INT_RAW("\t|IRQ Egress  = %03x|Learning action     = %03x|\n", pData.val[0] & (1 << 8), pData.val[0] & (1 << 9));
			IFXOS_PRINT_INT_RAW("\t|Snooping act= %03x|Metering action     = %03x|\n", pData.val[0] & (1 << 10), pData.val[0] & (1 << 11));
			IFXOS_PRINT_INT_RAW("\t|RMON act    = %03x|VLAN default        = %03x|\n", pData.val[0] & (1 << 12), pData.val[0] & (1 << 13));
			IFXOS_PRINT_INT_RAW("\t|TrafficClass= %03x|IRQ Special Tag     = %03x|\n", pData.val[0] & (1 << 14), pData.val[0] & (1 << 15));
			IFXOS_PRINT_INT_RAW("\t|Traffic act = %03x|VLAN action         = %03x|\n\n", pData.val[0] & (1 << 2), pData.val[0] & (1 << 3));
			IFXOS_PRINT_INT_RAW("\t|Port Map    = %03x|\n", pData.val[1]);
			IFXOS_PRINT_INT_RAW("\t|Alt FID     = %03x|Alt VLAN index      = %03x|\n",pData.val[2]>>8, pData.val[2] & 0xFF);
			IFXOS_PRINT_INT_RAW("\t|VALUE[3]    = %08x|\n",pData.val[3]);
			IFXOS_PRINT_INT_RAW("\t|VALUE[4]    = %08x|\n",pData.val[4]);
		}
	}
}   /* -----  end of function Traffic Flow Table  ----- */

void IFX_FLOW_MulticastHW_Table_Print(IFX_void_t)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pData;
	IFX_void_t *pDevCtx =IFX_NULL;
	IFX_uint8_t i, j, empty = 1, port = 0;

	IFXOS_PRINT_INT_RAW("|======Multicast Hardware Data Table =======|\n");
	for (i = 0; i < 64; i++) {
		pData.table = IFX_ETHSW_PCE_MULTICAST_HW_INDEX;
		pData.table_index = i;
		empty = 1;
		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pData);
		for (j = 0; j < 12; j++) {
			if (((pData.val[0] >> j) & 0x1) == 1) {
				port = j;
				/* Print out debug info first */
				IFXOS_PRINT_INT_RAW("|Index = %d | GroupIPv4 = %03d.%03d.%03d.%03d | Port = %d\n",i,	\
					(pData.key[1] >> 8) & 0xFF, (pData.key[1] & 0xFF),	\
					(pData.key[0] >> 8) & 0xFF, (pData.key[0] & 0xFF), port);
			}
		}
	}
}   /* -----  end of function PPPoE Data Table  ----- */

void IFX_FLOW_PPPOE_Table_Print(IFX_void_t)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pData;
	IFX_void_t *pDevCtx =IFX_NULL;
	IFX_uint8_t i ;

	IFXOS_PRINT_INT_RAW("|======PPPoE Data Table =======|\n");
	for (i = 0; i < 16; i++) {
		pData.table = IFX_ETHSW_PCE_PPPOE_INDEX;
		pData.table_index = i;
		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pData);
		if (pData.valid == 1) {
			/* Print out debug info first */
			IFXOS_PRINT_INT_RAW("|Index = %d |PPPoE Session ID = %02x \n", i, pData.key[0]);
		}
	}
}   /* -----  end of function PPPoE Data Table  ----- */

void IFX_FLOW_PROTOCOL_Table_Print(IFX_void_t)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pData;
	IFX_void_t *pDevCtx =IFX_NULL;
	IFX_uint8_t i ;

	IFXOS_PRINT_INT_RAW("|======Protocol Data Table =======|\n");
	for (i = 0; i < 32; i++) {
		pData.table = IFX_ETHSW_PCE_PROTOCOL_INDEX;
		pData.table_index = i;
		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pData);
		if (pData.valid == 1) {
			/* Print out debug info first */
			IFXOS_PRINT_INT_RAW("\t|Index = %d |EnterType = %02x | EnterType or protocol Mask = %02x\n",	\
				i, (pData.key[0]), pData.mask);
		}
	}
}   /* -----  end of function Protocol Data Table  ----- */

void IFX_FLOW_APP_DATA_MSB_LSB_Table_Print(IFX_void_t)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pData;
	IFX_void_t *pDevCtx =IFX_NULL;
	IFX_uint8_t i ;

	IFXOS_PRINT_INT_RAW("|======Application Data Table =======|\n");
	for (i = 0; i < 64; i++) {
		pData.table = IFX_ETHSW_PCE_APPLICATION_INDEX;
		pData.table_index = i;
		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pData);
		if (pData.valid == 1) {
			/* Print out debug info first */
       IFXOS_PRINT_INT_RAW("\t|Index = %d |Application Data = %02x | Mode Mask = %03x | Mode Range = %02x\n",	\
       	i, pData.key[0], pData.mask, pData.mask);
       }
    }
}   /* -----  end of function Application Data Table  ----- */


void IFX_FLOW_PacketLength_Table_Print(IFX_void_t)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pData;
	IFX_void_t *pDevCtx =IFX_NULL;
	IFX_uint8_t i ;

	IFXOS_PRINT_INT_RAW("|======Packet Length Table =======|\n");
	for (i = 0; i < 16; i++) {
		pData.table = IFX_ETHSW_PCE_PACKET_INDEX;
		pData.table_index = i;
		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pData);
		if (pData.valid == 1) {
			/* Print out debug info first */
			IFXOS_PRINT_INT_RAW("\t|Index = %d | Packet Length = %02x | Packet Length Range = %02x\n",	\
				i, (pData.key[0]), pData.mask);
		}
	}
}   /* -----  end of function Packet Length Table  ----- */

void IFX_FLOW_MAC_DASA_LSB_Table_Print(IFX_void_t)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pData;
	IFX_void_t *pDevCtx =IFX_NULL;
	IFX_uint8_t i ;

	IFXOS_PRINT_INT_RAW("|====== MAC DASA Table =======|\n");
	for (i = 0; i < 64; i++) {
		pData.table = IFX_ETHSW_PCE_MAC_DASA_INDEX;
		pData.table_index = i;
		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pData);
		if (pData.valid == 1) {
			/* Print out debug info first */
			IFXOS_PRINT_INT_RAW("\t|MAC DA/SA LSB Table = %02d | MAC = %03x:%03x:%03x:%03x:%03x:%03x| mask = %02x\n",	\
			i, (pData.key[0] & 0xFF), ((pData.key[0] >> 8) & 0xFF), (pData.key[1] & 0xFF),	\
			((pData.key[1] >> 8) & 0xFF),(pData.key[2] & 0xFF), ((pData.key[2] >> 8) & 0xFF), pData.mask);
		}
	}
}   /* -----  end of function MAC DA/SA LSB Table  ----- */

void IFX_FLOW_IP_DASA_LSB_Table_Print(IFX_void_t)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pData;
	IFX_void_t *pDevCtx =IFX_NULL;
	IFX_uint8_t i ;
	IFXOS_PRINT_INT_RAW("|======IP DASA LSB Table =======|\n");
	for (i = 0; i < 64; i++) {
		pData.table = IFX_ETHSW_PCE_IP_DASA_LSB_INDEX;
		pData.table_index = i;
		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pData);
		if (pData.valid == 1) {
			/* Print out debug info first */
			IFXOS_PRINT_INT_RAW("\t|IP DA/SA LSB Table = %02d | DescIP = %03d.%03d.%03d.%03d | mask = %02x\n",	\
			i, ((pData.key[1] >> 8) & 0xFF), (pData.key[1] & 0xFF), ((pData.key[0] >> 8) & 0xFF),	\
			(pData.key[0] & 0xFF), pData.mask);
		}
	}
}   /* -----  end of function IP DA/SA LSB Table  ----- */

void IFX_FLOW_VLAN_ActionMappingTable_Print(IFX_void_t)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pData;
	IFX_void_t *pDevCtx =IFX_NULL;
	IFX_uint8_t i ;

	IFXOS_PRINT_INT_RAW("|======VLAN Action/Mapping Table =======|\n");
	for (i = 0; i < IFX_FLOW_VLAN_ENTRY_MAX; i++) {
		pData.table = IFX_ETHSW_PCE_ACTVLAN_INDEX; // PCE VLAN ACTIVE Table
		pData.table_index = i;
		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pData);
		if (pData.valid == 1) {
			/* Print out debug info first */
			IFXOS_PRINT_INT_RAW("\t|  VLAN Action Table  | Table = %02d  | Byte0 = %d | Byte1 = %d| FID = %d	\
			 | RFLAG = %d\n", i, (pData.key[0] & 0xFF), ((pData.key[0] >> 8) & 0xFF), (pData.val[0] & 0xFF),	\
			 ((pData.key[0] >> 8) & 0x1 ) );
		}
	}
	for (i = 0; i < IFX_FLOW_VLAN_ENTRY_MAX; i++) {
		pData.table = IFX_ETHSW_PCE_VLANMAP_INDEX; // PCE VLAN Mapping Table
		pData.table_index = i;
		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pData);
		/* Print out debug info first */
		IFXOS_PRINT_INT_RAW("\t|  VLAN Mapping Table  | Index = %02d  | Byte0 = %d | Byte1 = %d	\
		| PortMap = %x | TagMap = %x\n", pData.table_index, (pData.val[0] & 0xFF),	\
		((pData.val[0] >> 8) & 0xFF), pData.val[1], pData.val[2]);
	}
}   /* -----  end of function VLAN Action & Mapping Table  ----- */

void IFX_FLOW_VLAN_Table_Print ( IFX_void_t *pDevCtx )
{
	IFX_FLOW_switchDev_t *pEthSWDev = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint8_t i ;

	for ( i = 0; i < IFX_FLOW_VLAN_ENTRY_MAX; i += 1 ) {
		IFXOS_PRINT_INT_RAW("entry[%2d]:\n", i);
		IFXOS_PRINT_INT_RAW("            vid = %d\n", pEthSWDev->VLAN_Table[i].vid);
		IFXOS_PRINT_INT_RAW("            fid = %d\n", pEthSWDev->VLAN_Table[i].fid);
		//    IFXOS_PRINT_INT_RAW("            tf  = %d\n", pEthSWDev->VLAN_Table[i].tf);
		IFXOS_PRINT_INT_RAW("            pm  = %d\n", pEthSWDev->VLAN_Table[i].pm);
		IFXOS_PRINT_INT_RAW("            tm  = %d\n", pEthSWDev->VLAN_Table[i].tm);
	}
}  /* -----  end of function IFX_FLOW_VLAN_Table_Print  ----- */
#endif /* FLOW_TABLE_DEBUG) */
#if defined (CONFIG_LTQ_VLAN ) && CONFIG_LTQ_VLAN
IFX_uint8_t IFX_FLOW_VLAN_Table_Index_Find ( IFX_void_t *pDevCtx, IFX_uint16_t vid )
{
	IFX_FLOW_switchDev_t *pEthSWDev = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint8_t i, index = 0xFF;
	for ( i = 0; i < IFX_FLOW_VLAN_ENTRY_MAX; i++ ) {
		if (vid == pEthSWDev->VLAN_Table[i].vid ) {
			index = i;
			break;
		}
	}
	return index;
}   /* -----  end of function IFX_FLOW_VLAN_Table_Index_Find  ----- */

IFX_uint8_t IFX_FLOW_VLAN_Table_Empty_Entry_find ( IFX_void_t *pDevCtx  )
{
	IFX_FLOW_switchDev_t *pEthSWDev = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint8_t i, index = 0xFF;

	for ( i = 1; i < IFX_FLOW_VLAN_ENTRY_MAX; i += 1 ) {
		if (pEthSWDev->VLAN_Table[i].valid == IFX_FALSE) {
			index = i;
			break;
		}
	}
	if ((index == 0xFF) && (pEthSWDev->VLAN_Table[0].valid == IFX_FALSE))
		return 0;
	return index;
}   /* -----  end of function IFX_FLOW_VLAN_Table_Empty_Entry_find  ----- */

static void IFX_FLOW_VLAN_Table_Entry_Set ( IFX_void_t *pDevCtx, IFX_uint8_t table_index,	\
			IFX_FLOW_VLAN_tableEntry_t *pTable_Entry )
{
	IFX_FLOW_switchDev_t *pEthSWDev = (IFX_FLOW_switchDev_t*)pDevCtx;

	pEthSWDev->VLAN_Table[table_index].valid = pTable_Entry->valid;
	pEthSWDev->VLAN_Table[table_index].reserved = pTable_Entry->reserved;
	pEthSWDev->VLAN_Table[table_index].vid = pTable_Entry->vid;
	pEthSWDev->VLAN_Table[table_index].fid = pTable_Entry->fid;
	//  pEthSWDev->VLAN_Table[table_index].tf = pTable_Entry->tf;
	pEthSWDev->VLAN_Table[table_index].pm = pTable_Entry->pm;
	pEthSWDev->VLAN_Table[table_index].tm = pTable_Entry->tm;
}   /* -----  end of function IFX_FLOW_VLAN_Table_Entry_Set  ----- */

void IFX_FLOW_VLAN_Table_Entry_Get ( IFX_void_t *pDevCtx, IFX_uint8_t table_index,	\
	IFX_FLOW_VLAN_tableEntry_t *pTable_Entry )
{
	IFX_FLOW_switchDev_t *pEthSWDev = (IFX_FLOW_switchDev_t*)pDevCtx;

	pTable_Entry->valid = pEthSWDev->VLAN_Table[table_index].valid;
	pTable_Entry->reserved = pEthSWDev->VLAN_Table[table_index].reserved;
	pTable_Entry->vid = pEthSWDev->VLAN_Table[table_index].vid;
	pTable_Entry->fid = pEthSWDev->VLAN_Table[table_index].fid;
	//  pTable_Entry->tf = pEthSWDev->VLAN_Table[table_index].tf;
	pTable_Entry->pm = pEthSWDev->VLAN_Table[table_index].pm;
	pTable_Entry->tm = pEthSWDev->VLAN_Table[table_index].tm;

}   /* -----  end of function IFX_FLOW_VLAN_Table_Entry_Get  ----- */
#endif /* CONFIG_LTQ_VLAN */
/**
*	This is the switch core layer init function.
*	\param pInit This parameter is a pointer to the switch core context.
*	\return Return value as follows:
*	pDev: if successful */
IFX_void_t *IFX_FLOW_SwitchCoreInit(IFX_FLOW_switchCoreInit_t *pInit )
{
	IFX_FLOW_switchDev_t *pDev;
	IFX_uint32_t reg_val;
	int j;
	pDev = (IFX_FLOW_switchDev_t*) IFXOS_BlockAlloc (sizeof (IFX_FLOW_switchDev_t));
	if (!pDev) {
    	IFXOS_PRINT_INT_RAW("ERROR: %s:%s:%d (memory allocation failed) \n", __FILE__, __FUNCTION__, __LINE__);
    	return pDev;
    }
	memset(pDev, 0, sizeof(IFX_FLOW_switchDev_t));
	pDev->pRAL_Dev = pInit->pDev;
	pCoreDev[pInit->eDev] = pDev;
	pDev->eDev = pInit->eDev;
	//   pDev->nPortNumber = IFX_FLOW_PORT_NUMBER_MAX;
	/* -----  end switch  ----- */
#if defined (CONFIG_LTQ_VLAN ) && CONFIG_LTQ_VLAN
	IFX_FLOW_VLAN_Table_Init(pDev);
#endif /* CONFIG_LTQ_VLAN */
	IFX_FLOW_PortCfg_Init(pDev);
#if defined (CONFIG_LTQ_MULTICAST ) && CONFIG_LTQ_MULTICAST
	IFX_FLOW_MULTICAST_SW_Table_Init(pDev);
#endif /*CONFIG_LTQ_MULTICAST*/
	ifx_pce_table_init(&pDev->PCE_Handler);
	pDev->bResetCalled = IFX_FALSE;
	pDev->bHW_InitCalled = IFX_FALSE;
	pDev->MAC_AgeTimer = DEFAULT_AGING_TIMEOUT;
	IFXOS_PRINT_INT_RAW("Switch API: PCE MicroCode loaded !!\n");
	IFX_VR9_Switch_PCE_Micro_Code_Int(pDev);
#ifdef IFX_ETHSW_API_COC
	pDev->pPMCtx = IFX_ETHSW_PM_powerManegementInit(pDev, IFX_ETHSW_PM_MODULENR_GSWIP);
	if (pDev->pPMCtx == IFX_NULL) {
		IFXOS_PRINT_INT_RAW("ERROR: %s:%s:%d (IFX_ETHSW_PM_powerManegementInit failed) \n", __FILE__, __FUNCTION__, __LINE__);
	}
#endif
	/* Configure the MDIO Clock 97.6 Khz */
	ifx_ethsw_ll_DirectAccessWrite(pDev, (VR9_MDC_CFG_1_FREQ_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_MDC_CFG_1_FREQ_SHIFT, VR9_MDC_CFG_1_FREQ_SIZE, 0xFF);
	/* EEE auto negotiation overides:  clock disable (ANEG_EEE_0.CLK_STOP_CAPABLE)  */
	for (j =0; j < pDev->nPortNumber-1; j++) {
		ifx_ethsw_ll_DirectAccessWrite(pDev, ((VR9_ANEG_EEE_0_CLK_STOP_CAPABLE_OFFSET+j) + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_ANEG_EEE_0_CLK_STOP_CAPABLE_SHIFT, VR9_ANEG_EEE_0_CLK_STOP_CAPABLE_SIZE, 0x3);
	}
	ifx_ethsw_ll_DirectAccessRead(pDev, VR9_ETHSW_CAP_10_MACBT_OFFSET,	\
		VR9_ETHSW_CAP_10_MACBT_SHIFT, VR9_ETHSW_CAP_10_MACBT_SIZE, &reg_val);
	pDev->max_mac_bridge_table_size = reg_val;
	ifx_ethsw_ll_DirectAccessRead(pDev, VR9_ETHSW_CAP_3_METERS_OFFSET,	\
		VR9_ETHSW_CAP_3_METERS_SHIFT, VR9_ETHSW_CAP_3_METERS_SIZE, &reg_val);
	pDev->max_meter_table_size = reg_val;
	ifx_ethsw_ll_DirectAccessRead(pDev, VR9_ETHSW_CAP_3_SHAPERS_OFFSET,	\
		VR9_ETHSW_CAP_3_SHAPERS_SHIFT, VR9_ETHSW_CAP_3_SHAPERS_SIZE, &reg_val);
	pDev->max_shapers_table_size = reg_val;
	
	
	ifx_ethsw_ll_DirectAccessRead(pDev, VR9_ETHSW_VERSION_REV_ID_OFFSET,	\
		VR9_ETHSW_VERSION_REV_ID_SHIFT, 16, &reg_val);
	pDev->GSWIP_Ver = reg_val;
	return pDev;
}

/**
*	This is the switch core layer cleanup function.
*	\return Return value as follows:
*	IFX_SUCCESS: if successful */
void IFX_FLOW_SwitchCoreCleanUP(IFX_void_t )
{
	IFX_uint8_t i;
	IFX_FLOW_switchDev_t *pDev;
	for ( i=0; i<IFX_FLOW_DEV_MAX; i++) {
		pDev = (IFX_FLOW_switchDev_t*) pCoreDev[i];
		if (pDev) {
#ifdef IFX_ETHSW_API_COC
			if (pDev->pPMCtx != IFX_NULL) {
				IFX_ETHSW_PM_powerManegementCleanUp(pDev->pPMCtx);
			}
#endif
			IFXOS_BlockFree(pDev);
			pDev = IFX_NULL;
		}
	}
}

IFX_return_t IFX_FLOW_MAC_TableClear(IFX_void_t *pDevCtx)
{
	/*  flush all entries from the MAC table */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_GCTRL_0_MTFL_OFFSET,	\
		VR9_PCE_GCTRL_0_MTFL_SHIFT, VR9_PCE_GCTRL_0_MTFL_SIZE, 1);
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_MAC_TableEntryAdd(IFX_void_t *pDevCtx, IFX_ETHSW_MAC_tableAdd_t *pPar)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pData;
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;

	if ( (pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber) && ( !(pPar->nPortId & 0x80000000)) )
		return IFX_ERROR;
	pData.table = IFX_ETHSW_PCE_MAC_BRIDGE_INDEX; // PCE MAC Table Address
	pData.key[0] = pPar->nMAC[4] << 8 | pPar->nMAC[5];
	pData.key[1] = pPar->nMAC[2] << 8 | pPar->nMAC[3];
	pData.key[2] = pPar->nMAC[0] << 8 | pPar->nMAC[1];
	pData.key[3] = pPar->nFId;
	pData.valid = 1;
	if(pPar->bStaticEntry) {
		if (pPar->nPortId & 0x80000000 ) { /*Port Map */
			pData.val[0] = ( pPar->nPortId & 0x7FFF);
		} else {
			pData.val[0] = (1 << pPar->nPortId);
		}
		pData.val[1] = 1;
	} else {
		pData.val[0] =  ( ( (pPar->nPortId & 0xF)<< 4 ) | (pPar->nAgeTimer & 0xF) );
	}
	ifx_ethsw_xwayflow_pce_table_key_write(pDevCtx, &pData);

	memset(&pData, 0, sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
	pData.table = IFX_ETHSW_PCE_MAC_BRIDGE_INDEX; // PCE MAC Table Address
	pData.key[0] = pPar->nMAC[4] << 8 | pPar->nMAC[5];
	pData.key[1] = pPar->nMAC[2] << 8 | pPar->nMAC[3];
	pData.key[2] = pPar->nMAC[0] << 8 | pPar->nMAC[1];
	pData.key[3] = pPar->nFId;
	ifx_ethsw_xwayflow_pce_table_key_read(pDevCtx, &pData);
	if (pData.valid != 1) {
		IFXOS_PRINT_INT_RAW("WARNING: (MAC Table full) %s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__);
	}

	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_MAC_TableEntryRead(IFX_void_t *pDevCtx, IFX_ETHSW_MAC_tableRead_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pData;
	if (pPar->bInitial == IFX_TRUE) {
		pEthSWDevHandle->mac_table_index = 0; /*Start from the index 0 */
		pPar->bInitial = IFX_FALSE;
	}
	if (pEthSWDevHandle->mac_table_index >= IFX_HW_MAC_TABLE_MAX) {
		memset(pPar, 0, sizeof(IFX_ETHSW_MAC_tableRead_t));
		pPar->bLast = IFX_TRUE;
		pEthSWDevHandle->mac_table_index = 0;
		return IFX_SUCCESS;
	}
	pData.table = IFX_ETHSW_PCE_MAC_BRIDGE_INDEX;
	do {
		pData.table_index = pEthSWDevHandle->mac_table_index;
		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pData);
		pEthSWDevHandle->mac_table_index++;
		if (pData.valid != 0)
			break;
	}while (pEthSWDevHandle->mac_table_index < IFX_HW_MAC_TABLE_MAX);
	if (pData.valid == 1) {
		pPar->nFId = pData.key[3] & 0x3F;
		pPar->bStaticEntry = (pData.val[1] & 0x1) ;
		if (pPar->bStaticEntry == 1) {
			pPar->nAgeTimer = 0;
			pPar->nPortId = pData.val[0] ;
		} else {
			IFX_uint32_t mant,timer=300;
			/* Aging Counter Mantissa Value */
			ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_AGE_1_MANT_OFFSET,	\
				VR9_PCE_AGE_1_MANT_SHIFT, VR9_PCE_AGE_1_MANT_SIZE, &mant);
			switch(mant) {
				case AGETIMER_1_DAY:
					timer = 86400;
					break;
				case AGETIMER_1_HOUR:
					timer = 3600;
					break;
				case AGETIMER_300_SEC:
					timer = 300;
					break;
				case AGETIMER_10_SEC:
					timer = 10;
					break;
				case AGETIMER_1_SEC:
					timer = 1;
					break;
			}
			pPar->nAgeTimer = pData.val[0] & 0xF;
			pPar->nAgeTimer = (timer * pPar->nAgeTimer)/0xF;
 			pPar->nPortId = (pData.val[0] >> 4) & 0xF;
		}
		pPar->nMAC[0] = pData.key[2] >> 8;;
		pPar->nMAC[1] = pData.key[2] & 0xFF;
		pPar->nMAC[2] = pData.key[1] >> 8;
		pPar->nMAC[3] = pData.key[1] & 0xFF;
		pPar->nMAC[4] = pData.key[0] >> 8;
		pPar->nMAC[5] = pData.key[0] & 0xFF;
		pPar->bInitial = IFX_FALSE;
		pPar->bLast = IFX_FALSE;
	} else {
		memset(pPar, 0, sizeof(IFX_ETHSW_MAC_tableRead_t));
		pPar->bLast = IFX_TRUE;
/*		IFXOS_PRINT_INT_RAW("WARNING:%s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__); */
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_MAC_TableEntryQuery(IFX_void_t *pDevCtx, IFX_ETHSW_MAC_tableQuery_t *pPar)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pData;
	pPar->bFound = IFX_FALSE;
	memset(&pData, 0, sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
	pData.table = IFX_ETHSW_PCE_MAC_BRIDGE_INDEX;
	pData.table = IFX_ETHSW_PCE_MAC_BRIDGE_INDEX; // PCE MAC Table Address
	pData.key[0] = pPar->nMAC[4] << 8 | pPar->nMAC[5];
	pData.key[1] = pPar->nMAC[2] << 8 | pPar->nMAC[3];
	pData.key[2] = pPar->nMAC[0] << 8 | pPar->nMAC[1];
	pData.key[3] = pPar->nFId;
	ifx_ethsw_xwayflow_pce_table_key_read(pDevCtx, &pData);
		if (pData.valid == 1) {
			pPar->bFound = IFX_TRUE;
			pPar->bStaticEntry = (pData.val[1] & 0x1) ;
			if ((pData.val[1] & 0x1) == 1) {
				pPar->nAgeTimer = 0;
				pPar->nPortId = (pData.val[0] ); 
			} else {
				IFX_uint32_t mant,timer=300;
				/* Aging Counter Mantissa Value */
				ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_AGE_1_MANT_OFFSET,	\
					VR9_PCE_AGE_1_MANT_SHIFT, VR9_PCE_AGE_1_MANT_SIZE, &mant);
				switch(mant) {
					case AGETIMER_1_DAY:
						timer = 86400;
						break;
					case AGETIMER_1_HOUR:
						timer = 3600;
						break;
					case AGETIMER_300_SEC:
						timer = 300;
						break;
					case AGETIMER_10_SEC:
						timer = 10;
						break;
					case AGETIMER_1_SEC:
						timer = 1;
						break;
					}
					pPar->nAgeTimer = pData.val[0] & 0xF;
					pPar->nAgeTimer = (timer * pPar->nAgeTimer)/0xF;
					pPar->nPortId = (pData.val[0] >> 4) & 0xF;
			}
		}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_MAC_TableEntryRemove(IFX_void_t *pDevCtx, IFX_ETHSW_MAC_tableRemove_t *pPar)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pData;
	IFX_uint32_t MAC_Table_Entry, value,RemoveFlag = 0 ;

	/* Learning Limit Port Lock */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_GCTRL_0_MTFL_OFFSET,	\
		VR9_PCE_GCTRL_0_MTFL_SHIFT, VR9_PCE_GCTRL_0_MTFL_SIZE, &value);
		/* Do nothing since the Delet all bit is set */
	if (!value) { /*if value is 1 means, flush all entries from the MAC table */
		for ( MAC_Table_Entry = 0; MAC_Table_Entry < IFX_HW_MAC_TABLE_MAX ; MAC_Table_Entry++) {
			memset(&pData, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
			pData.table = IFX_ETHSW_PCE_MAC_BRIDGE_INDEX; // PCE MAC Table Address
			pData.table_index = MAC_Table_Entry;
			ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pData);
			if((pPar->nMAC[0] == (pData.key[2] >> 8)) && (pPar->nMAC[1] == (pData.key[2] & 0xFF))	\
				&& (pPar->nMAC[2] == (pData.key[1] >> 8)) && (pPar->nMAC[3] == (pData.key[1] & 0xFF))	\
				&& (pPar->nMAC[4] == (pData.key[0] >> 8)) && (pPar->nMAC[5] == (pData.key[0] & 0xFF))	\
				&& (pPar->nFId == (pData.key[3] & 0x3F))) {
				memset(&pData, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
				pData.table = IFX_ETHSW_PCE_MAC_BRIDGE_INDEX;
				pData.table_index = MAC_Table_Entry;
				ifx_ethsw_xwayflow_pce_table_write(pDevCtx, &pData);
				RemoveFlag = 1;
			}
		}
	}
	if (RemoveFlag == 0) {
		IFXOS_PRINT_INT_RAW("WARNING: (MAC Address search Not Found) %s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__);
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_PortCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_portCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint8_t portIdx = pPar->nPortId;
	IFX_uint32_t value, Monitor_rx = 0, Monitor_tx = 0, PEN, EN;

	if( pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber)
		return IFX_ERROR;
	/* See if PORT enable or not */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_SDMA_PCTRL_PEN_OFFSET + (0x6 * portIdx)),	\
		VR9_SDMA_PCTRL_PEN_SHIFT, VR9_SDMA_PCTRL_PEN_SIZE, &PEN);
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_FDMA_PCTRL_EN_OFFSET + (0x6 * portIdx)),	\
		VR9_FDMA_PCTRL_EN_SHIFT, VR9_FDMA_PCTRL_EN_SIZE, &EN);
	/* Port Enable feature only support 6 port */
	 if ( portIdx >= pEthSWDevHandle->nPortNumber ) {
		pPar->eEnable = IFX_ENABLE;
	} else {
		if ((PEN == 1) && (EN == 1))
			pPar->eEnable = IFX_ETHSW_PORT_ENABLE_RXTX;
		else if ((PEN == 1) && (EN == 0))
			pPar->eEnable = IFX_ETHSW_PORT_ENABLE_RX;
		else if ((PEN == 0) && (EN == 1))
			pPar->eEnable = IFX_ETHSW_PORT_ENABLE_TX;
		else
			pPar->eEnable = IFX_ETHSW_PORT_DISABLE;
	}
	/* Learning Limit */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PCE_PCTRL_1_LRNLIM_OFFSET + (0xA * portIdx)),	\
		VR9_PCE_PCTRL_1_LRNLIM_SHIFT, VR9_PCE_PCTRL_1_LRNLIM_SIZE, &value);
	pPar->nLearningLimit = value;
	
	/* Learning Limit Port Lock */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PCE_PCTRL_0_PLOCK_OFFSET + (0xA * portIdx)) ,	\
		VR9_PCE_PCTRL_0_PLOCK_SHIFT, VR9_PCE_PCTRL_0_PLOCK_SIZE, &value);
	pPar->bLearningMAC_PortLock = value;
	/* Aging */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_PCTRL_0_AGEDIS_OFFSET,	\
		VR9_PCE_PCTRL_0_AGEDIS_SHIFT, VR9_PCE_PCTRL_0_AGEDIS_SIZE, &value);
	pPar->bAging = value;
	if ( pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2) {
	/* Supported for GSWIP 2.2 and newer and returns with an error for older hardware revisions. */
		/** MAC address table learning on the port specified. */
	 	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (GSWIP_PCE_PCTRL_3_LNDIS_OFFSET + (0xA * portIdx)),	\
			GSWIP_PCE_PCTRL_3_LNDIS_SHIFT, GSWIP_PCE_PCTRL_3_LNDIS_SIZE, &pPar->bLearning);
		/** MAC spoofing detection. */
	 	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (GSWIP_PCE_PCTRL_0_SPFDIS_OFFSET + (0xA * portIdx)),	\
			GSWIP_PCE_PCTRL_0_SPFDIS_SHIFT, GSWIP_PCE_PCTRL_0_SPFDIS_SIZE, &pPar->bMAC_SpoofingDetection);
	}
	/* UnicastUnknownDrop */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_PMAP_3_UUCMAP_OFFSET,	\
		VR9_PCE_PMAP_3_UUCMAP_SHIFT, VR9_PCE_PMAP_3_UUCMAP_SIZE, &value);
	/* UnicastUnknownDrop feature  support  */
	if( (value & ( 1 << portIdx)) == 0)
		pPar->bUnicastUnknownDrop = IFX_ENABLE;
	else
		pPar->bUnicastUnknownDrop = 0;
	/* MulticastUnknownDrop */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_PMAP_2_DMCPMAP_OFFSET,	\
		VR9_PCE_PMAP_2_DMCPMAP_SHIFT, VR9_PCE_PMAP_2_DMCPMAP_SIZE, &value);
	/* MulticastUnknownDrop feature  support  */
	if((value & (1 << portIdx)) == 0) {
		pPar->bMulticastUnknownDrop = 1;
		pPar->bBroadcastDrop = 1;
	} else {
		pPar->bMulticastUnknownDrop = 0;
		pPar->bBroadcastDrop = 0;
	}
	/* Require to check later - 3M */
	pPar->bReservedPacketDrop = 0;
	/* Port Monitor */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PCE_PCTRL_3_RXVMIR_OFFSET + (0xA * portIdx)),	\
		VR9_PCE_PCTRL_3_RXVMIR_SHIFT, VR9_PCE_PCTRL_3_RXVMIR_SIZE, &Monitor_rx);
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PCE_PCTRL_3_TXMIR_OFFSET + (0xA * portIdx)),	\
		VR9_PCE_PCTRL_3_TXMIR_SHIFT, VR9_PCE_PCTRL_3_TXMIR_SIZE, &Monitor_tx);
	if ((Monitor_rx == 1 ) && (Monitor_tx == 1))
		pPar->ePortMonitor = IFX_ETHSW_PORT_MONITOR_RXTX;
	else if ((Monitor_rx == 1 ) && (Monitor_tx == 0))
		pPar->ePortMonitor = IFX_ETHSW_PORT_MONITOR_RX;
	else if ((Monitor_rx == 0 ) && (Monitor_tx == 1))
		pPar->ePortMonitor = IFX_ETHSW_PORT_MONITOR_TX;
	else
		pPar->ePortMonitor = IFX_ETHSW_PORT_MONITOR_NONE;
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MAC_PSTAT_TXPAUEN_OFFSET + (0xC * portIdx)),	\
		VR9_MAC_PSTAT_TXPAUEN_SHIFT, 2 , &value);
	pPar->eFlowCtrl = value;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_PortCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_portCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint8_t portIdx = pPar->nPortId;
	IFX_uint32_t value, EN, PEN, PACT, Monitor_rx, Monitor_tx;

	if( pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber )
		return IFX_ERROR;

	/* Learning Limit Port Lock */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PCE_PCTRL_0_PLOCK_OFFSET + (0xA * portIdx)),	\
		VR9_PCE_PCTRL_0_PLOCK_SHIFT, VR9_PCE_PCTRL_0_PLOCK_SIZE, pPar->bLearningMAC_PortLock);
	/* Learning Limit Action */
	if (pPar->nLearningLimit == 0)
		value = 0;
	else if (pPar->nLearningLimit == 0xFFFF)
		value = 0xFF;
	else
		value = pPar->nLearningLimit;
	pEthSWDevHandle->PortConfig[pPar->nPortId].nLearningLimit = value;
	/* Learning Limit */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PCE_PCTRL_1_LRNLIM_OFFSET + (0xA * portIdx)),	\
		VR9_PCE_PCTRL_1_LRNLIM_SHIFT, VR9_PCE_PCTRL_1_LRNLIM_SIZE, value);
	if ( pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2) {
	/* Supported for GSWIP 2.2 and newer and returns with an error for older hardware revisions. */
		/** MAC address table learning on the port specified */
	 	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (GSWIP_PCE_PCTRL_3_LNDIS_OFFSET + (0xA * portIdx)),	\
			GSWIP_PCE_PCTRL_3_LNDIS_SHIFT, GSWIP_PCE_PCTRL_3_LNDIS_SIZE, pPar->bLearning);
		/** MAC spoofing detection. */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (GSWIP_PCE_PCTRL_0_SPFDIS_OFFSET + (0xA * portIdx)),\
			GSWIP_PCE_PCTRL_0_SPFDIS_SHIFT, GSWIP_PCE_PCTRL_0_SPFDIS_SIZE, pPar->bMAC_SpoofingDetection);
	}
	/* Aging */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PCTRL_0_AGEDIS_OFFSET,	\
		VR9_PCE_PCTRL_0_AGEDIS_SHIFT, VR9_PCE_PCTRL_0_AGEDIS_SIZE, pPar->bAging);

	/* UnicastUnknownDrop Read first */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_PMAP_3_UUCMAP_OFFSET,	\
		VR9_PCE_PMAP_3_UUCMAP_SHIFT, VR9_PCE_PMAP_3_UUCMAP_SIZE, &value);

	if (pPar->bUnicastUnknownDrop == 1) {
		value &= ~( 1 << portIdx);
	} else {
		value |= 1 << portIdx;
	}
	/* UnicastUnknownDrop write back */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PMAP_3_UUCMAP_OFFSET,	\
		VR9_PCE_PMAP_3_UUCMAP_SHIFT, VR9_PCE_PMAP_3_UUCMAP_SIZE, value);

	/* MulticastUnknownDrop */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_PMAP_2_DMCPMAP_OFFSET,	\
		VR9_PCE_PMAP_2_DMCPMAP_SHIFT, VR9_PCE_PMAP_2_DMCPMAP_SIZE, &value);

	if (pPar->bMulticastUnknownDrop == 1) {
		value &= ~( 1 << portIdx);
	} else {
		value |= 1 << portIdx;
	}
	/* MulticastUnknownDrop */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PMAP_2_DMCPMAP_OFFSET,	\
		VR9_PCE_PMAP_2_DMCPMAP_SHIFT, VR9_PCE_PMAP_2_DMCPMAP_SIZE, value);

	/* bReservedPacketDrop */
	/* bBroadcastDrop */

	/* Flow Control */
	if ( portIdx < pEthSWDevHandle->nPortNumber) {
		/* Check MDC_CFG_0.PEN_x */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MDC_CFG_0_PEN_0_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			(VR9_MDC_CFG_0_PEN_0_SHIFT + portIdx), VR9_MDC_CFG_0_PEN_0_SIZE, &PEN);
		/* Check MDIO STAT.PACT */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MDIO_STAT_0_PACT_OFFSET+ IFX_ETHSW_FLOW_TOP_REG_OFFSET + portIdx),	\
			VR9_MDIO_STAT_0_PACT_SHIFT, VR9_MDIO_STAT_0_PACT_SIZE, &PACT);

		if ((PEN == 1) && (PACT == 1)) {
			/* PHY polling statemachine (of the MAC) is activated and
				an external PHY reacts on the MDIO accesses.
			   Therefore update the MDIO register of the attached PHY. */

			IFX_ETHSW_MDIO_data_t mdio_data;
			/* Write directly to MDIO register */
			ifx_ethsw_ll_DirectAccessRead(pDevCtx, ( (VR9_PHY_ADDR_0_ADDR_OFFSET - portIdx) + IFX_ETHSW_FLOW_TOP_REG_OFFSET ),	\
				VR9_PHY_ADDR_0_ADDR_SHIFT, VR9_PHY_ADDR_0_ADDR_SIZE, &value);
			mdio_data.nAddressDev = value;
			mdio_data.nAddressReg = 0x4;
			IFX_FLOW_MDIO_DataRead(pEthSWDevHandle, &mdio_data);
			mdio_data.nData &= ~(0xC00);
			switch(pPar->eFlowCtrl) {
				case IFX_ETHSW_FLOW_OFF:
					break;
				case IFX_ETHSW_FLOW_TX:
					mdio_data.nData |= 0x800;
					break;
				case IFX_ETHSW_FLOW_RXTX:
					mdio_data.nData |= 0x400;
					break;
				case IFX_ETHSW_FLOW_RX:
				case IFX_ETHSW_FLOW_AUTO:
					mdio_data.nData |= 0xC00;
					break;
			}
			IFX_FLOW_MDIO_DataWrite(pEthSWDevHandle, &mdio_data);
			/* Restart Auto negotiation */
			mdio_data.nAddressReg = 0x0;
			IFX_FLOW_MDIO_DataRead(pEthSWDevHandle, &mdio_data);
			mdio_data.nData |= 0x1200;
			IFX_FLOW_MDIO_DataWrite(pEthSWDevHandle, &mdio_data);
		} else {
			/* Either PHY polling statemachine (of the MAC) is disable,
			   or the statemachine did not find any attached PHY. */
			IFX_uint32_t RX = 0, TX = 0;
			switch(pPar->eFlowCtrl) {
				case IFX_ETHSW_FLOW_AUTO:
				case IFX_ETHSW_FLOW_OFF:
					RX=0;TX=0;
					break;
				case IFX_ETHSW_FLOW_RXTX:
					RX=1;TX=1;
					break;
				case IFX_ETHSW_FLOW_RX:
					RX=1;TX=0;
					break;
				case IFX_ETHSW_FLOW_TX:
					RX=0;TX=1;
					break;
			}
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_MAC_CTRL_0_FCON_OFFSET + ( 0xC * portIdx)),	\
				VR9_MAC_CTRL_0_FCON_SHIFT, VR9_MAC_CTRL_0_FCON_SIZE, pPar->eFlowCtrl);
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PHY_ADDR_0_FCONTX_OFFSET - ( 0x1 * portIdx)),	\
				VR9_PHY_ADDR_0_FCONTX_SHIFT, VR9_PHY_ADDR_0_FCONTX_SIZE , TX);
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PHY_ADDR_0_FCONRX_OFFSET - ( 0x1 * portIdx)),	\
				VR9_PHY_ADDR_0_FCONRX_SHIFT, VR9_PHY_ADDR_0_FCONRX_SIZE, RX);
		}
	}
	/* Port Monitor */
	if (pPar->ePortMonitor == IFX_ETHSW_PORT_MONITOR_RXTX) {
		Monitor_rx =1;
		Monitor_tx =1;
	} else if (pPar->ePortMonitor == IFX_ETHSW_PORT_MONITOR_RX) {
		Monitor_rx = 1;
		Monitor_tx = 0;
	} else if (pPar->ePortMonitor == IFX_ETHSW_PORT_MONITOR_TX) {
		Monitor_rx = 0;
		Monitor_tx = 1;
	} else {
		Monitor_rx = 0;
		Monitor_tx = 0;
	}
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PCE_PCTRL_3_RXVMIR_OFFSET + (0xA * portIdx)),	\
		VR9_PCE_PCTRL_3_RXVMIR_SHIFT, VR9_PCE_PCTRL_3_RXVMIR_SIZE, Monitor_rx);
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PCE_PCTRL_3_TXMIR_OFFSET + (0xA * portIdx)),	\
		VR9_PCE_PCTRL_3_TXMIR_SHIFT, VR9_PCE_PCTRL_3_TXMIR_SIZE, Monitor_tx);
	if(pPar->eEnable == IFX_ETHSW_PORT_ENABLE_RXTX) {
		PEN = 1;
		EN = 1;
	} else if (pPar->eEnable == IFX_ETHSW_PORT_ENABLE_RX) {
		PEN = 1;
		EN = 0;
	} else if (pPar->eEnable == IFX_ETHSW_PORT_ENABLE_TX) {
		PEN = 0;
		EN = 1;
	} else {
		PEN = 0;
		EN = 0;
	}
	/* Set SDMA_PCTRL_PEN PORT enable */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_SDMA_PCTRL_PEN_OFFSET + (6 * portIdx)),	\
		VR9_SDMA_PCTRL_PEN_SHIFT, VR9_SDMA_PCTRL_PEN_SIZE, PEN);
	/* Set FDMA_PCTRL_EN PORT enable  */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_FDMA_PCTRL_EN_OFFSET + (0x6 * portIdx)),	\
		VR9_FDMA_PCTRL_EN_SHIFT, VR9_FDMA_PCTRL_EN_SIZE, EN);
	return IFX_SUCCESS;
}
#if defined (CONFIG_LTQ_STP ) && CONFIG_LTQ_STP
IFX_return_t IFX_FLOW_STP_BPDU_RuleGet(IFX_void_t *pDevCtx, IFX_ETHSW_STP_BPDU_Rule_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_FLOW_STP_8021X_t *pStateHandle = &pEthSWDevHandle->STP_8021x_Config;

	pPar->eForwardPort = pStateHandle->eSTPPortState;
	pPar->nForwardPortId = pStateHandle->nSTP_PortID;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_STP_BPDU_RuleSet(IFX_void_t *pDevCtx, IFX_ETHSW_STP_BPDU_Rule_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_FLOW_STP_8021X_t *pStateHandle = &pEthSWDevHandle->STP_8021x_Config;
	IFX_FLOW_PCE_rule_t PCE_rule;

	pStateHandle->eSTPPortState = pPar->eForwardPort;
	pStateHandle->nSTP_PortID   = pPar->nForwardPortId;
	memset(&PCE_rule, 0, sizeof(IFX_FLOW_PCE_rule_t));
	/* Attached the PCE rule for BPDU packet */
	/*fill the required fields, other fields are zeros */
	PCE_rule.pattern.nIndex = 61;
	PCE_rule.pattern.bEnable = IFX_TRUE;
	PCE_rule.pattern.bMAC_DstEnable = IFX_TRUE;
	PCE_rule.pattern.nMAC_Dst[0] = 0x01;
	PCE_rule.pattern.nMAC_Dst[1] = 0x80;
	PCE_rule.pattern.nMAC_Dst[2] = 0xC2;
	PCE_rule.pattern.nMAC_Dst[3] = 0x00;
	PCE_rule.pattern.nMAC_Dst[4] = 0x00;
	PCE_rule.pattern.nMAC_Dst[5] = 0x00;
	PCE_rule.action.eCrossStateAction = IFX_FLOW_PCE_ACTION_CROSS_STATE_CROSS;
	if ((pStateHandle->eSTPPortState < 4) && (pStateHandle->eSTPPortState > 0))
		PCE_rule.action.ePortMapAction = pStateHandle->eSTPPortState + 1;
	else
		IFXOS_PRINT_INT_RAW("The Forward port action incorrect\n");
	PCE_rule.action.nForwardPortMap = (1 << pStateHandle->nSTP_PortID);
	/* We prepare everything and write into PCE Table */
	if (0 != ifx_pce_rule_write(&pEthSWDevHandle->PCE_Handler,&PCE_rule))
		return IFX_ERROR;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_STP_PortCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_STP_portCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	if( pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber)
		return IFX_ERROR;
	pPar->ePortState = pEthSWDevHandle->PortConfig[pPar->nPortId].ifx_stp_state;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_STP_PortCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_STP_portCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	if( pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber)
		return IFX_ERROR;
	pEthSWDevHandle->PortConfig[pPar->nPortId].ifx_stp_state = pPar->ePortState;
	/* Config the Table */
	set_port_state(pDevCtx, pPar->nPortId, pEthSWDevHandle->PortConfig[pPar->nPortId].ifx_stp_state,	\
		pEthSWDevHandle->PortConfig[pPar->nPortId].ifx_8021x_state );
	return IFX_SUCCESS;
}
#endif /* CONFIG_LTQ_STP */
IFX_return_t IFX_FLOW_TrunkingCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_trunkingCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	/* Supported for GSWIP 2.2 and newer and returns with an error for older hardware revisions. */
	if ( pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2) {
		/* Destination IP Mask */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, GSWIP_PCE_TRUNK_CONF_DIP_OFFSET,	\
			GSWIP_PCE_TRUNK_CONF_DIP_SHIFT, GSWIP_PCE_TRUNK_CONF_DIP_SIZE, &pPar->bIP_Dst);
		/* 'Source IP Mask */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, GSWIP_PCE_TRUNK_CONF_SIP_OFFSET,	\
			GSWIP_PCE_TRUNK_CONF_SIP_SHIFT, GSWIP_PCE_TRUNK_CONF_SIP_SIZE, &pPar->bIP_Src);
		/* Destination MAC Mask */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, GSWIP_PCE_TRUNK_CONF_DA_OFFSET,	\
			GSWIP_PCE_TRUNK_CONF_DA_SHIFT, GSWIP_PCE_TRUNK_CONF_DA_SIZE, &pPar->bMAC_Dst);
		/* 'Source MAC Mask */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, GSWIP_PCE_TRUNK_CONF_SA_OFFSET,	\
			GSWIP_PCE_TRUNK_CONF_SA_SHIFT, GSWIP_PCE_TRUNK_CONF_SA_SIZE, &pPar->bMAC_Src);
		return IFX_SUCCESS;
	} else {
		return IFX_ERROR;
	}
}
IFX_return_t IFX_FLOW_TrunkingCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_trunkingCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	/* Supported for GSWIP 2.2 and newer and returns with an error for older hardware revisions. */
	if ( pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2) {
		/* Destination IP Mask */
		if (pPar->bIP_Dst == 1 ) {
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, GSWIP_PCE_TRUNK_CONF_DIP_OFFSET,	\
				GSWIP_PCE_TRUNK_CONF_DIP_SHIFT, GSWIP_PCE_TRUNK_CONF_DIP_SIZE, 1);
		} else {
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, GSWIP_PCE_TRUNK_CONF_DIP_OFFSET,	\
				GSWIP_PCE_TRUNK_CONF_DIP_SHIFT, GSWIP_PCE_TRUNK_CONF_DIP_SIZE, 0);
		}
		/* 'Source IP Mask */
		if (pPar->bIP_Src == 1 ) {
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, GSWIP_PCE_TRUNK_CONF_SIP_OFFSET,	\
				GSWIP_PCE_TRUNK_CONF_SIP_SHIFT, GSWIP_PCE_TRUNK_CONF_SIP_SIZE, 1);
		} else {
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, GSWIP_PCE_TRUNK_CONF_SIP_OFFSET,	\
				GSWIP_PCE_TRUNK_CONF_SIP_SHIFT, GSWIP_PCE_TRUNK_CONF_SIP_SIZE, 0);
		}
		/* Destination MAC Mask */
		if (pPar->bMAC_Dst == 1 ) {
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, GSWIP_PCE_TRUNK_CONF_DA_OFFSET,	\
				GSWIP_PCE_TRUNK_CONF_DA_SHIFT, GSWIP_PCE_TRUNK_CONF_DA_SIZE, 1);
		} else {
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, GSWIP_PCE_TRUNK_CONF_DA_OFFSET,	\
				GSWIP_PCE_TRUNK_CONF_DA_SHIFT, GSWIP_PCE_TRUNK_CONF_DA_SIZE, 0);
		}
		/* 'Source MAC Mask */
		if (pPar->bMAC_Src == 1 ) {
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, GSWIP_PCE_TRUNK_CONF_SA_OFFSET,	\
				GSWIP_PCE_TRUNK_CONF_SA_SHIFT, GSWIP_PCE_TRUNK_CONF_SA_SIZE, 1);
		} else {
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, GSWIP_PCE_TRUNK_CONF_SA_OFFSET,	\
				GSWIP_PCE_TRUNK_CONF_SA_SHIFT, GSWIP_PCE_TRUNK_CONF_SA_SIZE, 0);
		}
		return IFX_SUCCESS;
	} else {
		return IFX_ERROR;
	}
}

IFX_return_t IFX_FLOW_TrunkingPortCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_trunkingPortCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint32_t value;
	if( pPar->nPortId >= (pEthSWDevHandle->nTotalPortNumber) )
		return IFX_ERROR;
	/* Supported for GSWIP 2.2 and newer and returns with an error for older hardware revisions. */
	if ( pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2) {
	 	/** Ports are aggregated. the 'nPortId' and the 'nAggrPortId' ports form an aggregated link. */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (GSWIP_PCE_PTRUNK_EN_OFFSET +(pPar->nPortId * 0x2)),	\
			GSWIP_PCE_PTRUNK_EN_SHIFT, GSWIP_PCE_PTRUNK_EN_SIZE, &value);
		 pPar->bAggregateEnable = value;
		 IFXOS_PRINT_INT_RAW("%s:%s:%d, address:0x%0x, value:%d \n", __FILE__, __FUNCTION__, __LINE__,	\
		(GSWIP_PCE_PTRUNK_EN_OFFSET +(pPar->nPortId * 0x2)),pPar->bAggregateEnable );
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (GSWIP_PCE_PTRUNK_PARTER_OFFSET +(pPar->nPortId * 0x2)),	\
			GSWIP_PCE_PTRUNK_PARTER_SHIFT, GSWIP_PCE_PTRUNK_PARTER_SIZE, &value );
		pPar->nAggrPortId = value;
		IFXOS_PRINT_INT_RAW("%s:%s:%d, address:0x%0x, value:%d \n", __FILE__, __FUNCTION__, __LINE__,	\
		(GSWIP_PCE_PTRUNK_PARTER_OFFSET +(pPar->nPortId * 0x2)),pPar->nAggrPortId );
		return IFX_SUCCESS;
	} else {
		return IFX_ERROR;
	}
}

IFX_return_t IFX_FLOW_TrunkingPortCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_trunkingPortCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	if( pPar->nPortId >= (pEthSWDevHandle->nTotalPortNumber) )
		return IFX_ERROR;
	/* Supported for GSWIP 2.2 and newer and returns with an error for older hardware revisions. */
	if ( pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2) {
		 /** Ports are aggregated. the 'nPortId' and the 'nAggrPortId' ports form an aggregated link. */
		if ( pPar->bAggregateEnable == 1 ) {
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (GSWIP_PCE_PTRUNK_EN_OFFSET +(pPar->nPortId * 0x2)),	\
				GSWIP_PCE_PTRUNK_EN_SHIFT, GSWIP_PCE_PTRUNK_EN_SIZE, 1);
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (GSWIP_PCE_PTRUNK_PARTER_OFFSET +(pPar->nPortId * 0x2)),	\
				GSWIP_PCE_PTRUNK_PARTER_SHIFT, GSWIP_PCE_PTRUNK_PARTER_SIZE, (pPar->nAggrPortId & 0xF));
		}
		return IFX_SUCCESS;
	} else {
		return IFX_ERROR;
	}
}

IFX_return_t IFX_FLOW_QoS_WredPortCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_WRED_PortCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	if( pPar->nPortId >= (pEthSWDevHandle->nPortNumber) )
		return IFX_ERROR;
	/* Supported for GSWIP 2.2 and newer and returns with an error for older hardware revisions. */
	if ( pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2) {		
		/** WRED Red Threshold Min [number of segments]. */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (GSWIP_BM_PWRED_RTH_0_MINTH_OFFSET +(pPar->nPortId * 0x6)),	\
			GSWIP_BM_PWRED_RTH_0_MINTH_SHIFT, GSWIP_BM_PWRED_RTH_0_MINTH_SIZE, &pPar->nRed_Min);
		 /** WRED Red Threshold Max [number of segments]. */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (GSWIP_BM_PWRED_RTH_1_MAXTH_OFFSET +(pPar->nPortId * 0x6)),	\
			GSWIP_BM_PWRED_RTH_1_MAXTH_SHIFT, GSWIP_BM_PWRED_RTH_1_MAXTH_SIZE, &pPar->nRed_Max);
		/* WRED Yellow Threshold - Minimum */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (GSWIP_BM_PWRED_YTH_0_MINTH_OFFSET +(pPar->nPortId * 0x6)),	\
			GSWIP_BM_PWRED_YTH_0_MINTH_SHIFT, GSWIP_BM_PWRED_YTH_0_MINTH_SIZE, &pPar->nYellow_Min);
		/* WRED Yellow Threshold - Maximum */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (GSWIP_BM_PWRED_YTH_1_MAXTH_OFFSET + (pPar->nPortId * 0x6)),	\
			GSWIP_BM_PWRED_YTH_1_MAXTH_SHIFT, GSWIP_BM_PWRED_YTH_1_MAXTH_SIZE, &pPar->nYellow_Max);
		/* WRED Green Threshold - Minimum */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (GSWIP_BM_PWRED_GTH_0_MINTH_OFFSET + (pPar->nPortId * 0x6)),	\
			GSWIP_BM_PWRED_GTH_0_MINTH_SHIFT, GSWIP_BM_PWRED_GTH_0_MINTH_SIZE, &pPar->nGreen_Min);
		/* WRED Green Threshold - Maximum */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (GSWIP_BM_PWRED_GTH_1_MAXTH_OFFSET +(pPar->nPortId * 0x6)),	\
			GSWIP_BM_PWRED_GTH_1_MAXTH_SHIFT, GSWIP_BM_PWRED_GTH_1_MAXTH_SIZE, &pPar->nGreen_Max);
		return IFX_SUCCESS;
	} else {
		return IFX_ERROR;
	}
}


IFX_return_t IFX_FLOW_QoS_WredPortCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_WRED_PortCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	if( pPar->nPortId >= (pEthSWDevHandle->nPortNumber) )
		return IFX_ERROR;
	/* Supported for GSWIP 2.2 and newer and returns with an error for older hardware revisions. */
	if ( pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2) {	
		/** WRED Red Threshold Min [number of segments]. */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (GSWIP_BM_PWRED_RTH_0_MINTH_OFFSET +(pPar->nPortId * 0x6)),	\
			GSWIP_BM_PWRED_RTH_0_MINTH_SHIFT, GSWIP_BM_PWRED_RTH_0_MINTH_SIZE, pPar->nRed_Min);
		 /** WRED Red Threshold Max [number of segments]. */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (GSWIP_BM_PWRED_RTH_1_MAXTH_OFFSET +(pPar->nPortId * 0x6)),	\
			GSWIP_BM_PWRED_RTH_1_MAXTH_SHIFT, GSWIP_BM_PWRED_RTH_1_MAXTH_SIZE, pPar->nRed_Max);
		/* WRED Yellow Threshold - Minimum */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (GSWIP_BM_PWRED_YTH_0_MINTH_OFFSET +(pPar->nPortId * 0x6)),	\
			GSWIP_BM_PWRED_YTH_0_MINTH_SHIFT, GSWIP_BM_PWRED_YTH_0_MINTH_SIZE, pPar->nYellow_Min);
		/* WRED Yellow Threshold - Maximum */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (GSWIP_BM_PWRED_YTH_1_MAXTH_OFFSET + (pPar->nPortId * 0x6)),	\
			GSWIP_BM_PWRED_YTH_1_MAXTH_SHIFT, GSWIP_BM_PWRED_YTH_1_MAXTH_SIZE, pPar->nYellow_Max);
		/* WRED Green Threshold - Minimum */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (GSWIP_BM_PWRED_GTH_0_MINTH_OFFSET + (pPar->nPortId * 0x6)),	\
			GSWIP_BM_PWRED_GTH_0_MINTH_SHIFT, GSWIP_BM_PWRED_GTH_0_MINTH_SIZE, pPar->nGreen_Min);
		/* WRED Green Threshold - Maximum */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (GSWIP_BM_PWRED_GTH_1_MAXTH_OFFSET +(pPar->nPortId * 0x6)),	\
			GSWIP_BM_PWRED_GTH_1_MAXTH_SHIFT, GSWIP_BM_PWRED_GTH_1_MAXTH_SIZE, pPar->nGreen_Max);
		return IFX_SUCCESS;
	} else {
		return IFX_ERROR;
	}
}

IFX_return_t IFX_FLOW_QoS_FlowctrlCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_FlowCtrlCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	/* Supported for GSWIP 2.2 and newer and returns with an error for older hardware revisions. */
	if ( pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2) {	
			/** Global Buffer Non Conforming Flow Control Threshold Minimum [number of segments]. */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_SDMA_FCTHR1_THR1_OFFSET ),	\
			VR9_SDMA_FCTHR1_THR1_SHIFT, VR9_SDMA_FCTHR1_THR1_SIZE, &pPar->nFlowCtrlNonConform_Min);
		/** Global Buffer Non Conforming Flow Control Threshold Maximum [number of segments]. */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_SDMA_FCTHR2_THR2_OFFSET ),	\
			VR9_SDMA_FCTHR2_THR2_SHIFT, VR9_SDMA_FCTHR2_THR2_SIZE, &pPar->nFlowCtrlNonConform_Max);
		/** Global Buffer Conforming Flow Control Threshold Minimum [number of segments]. */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_SDMA_FCTHR3_THR3_OFFSET ),	\
			VR9_SDMA_FCTHR3_THR3_SHIFT, VR9_SDMA_FCTHR3_THR3_SIZE, &pPar->nFlowCtrlConform_Min);
		/** Global Buffer Conforming Flow Control Threshold Maximum [number of segments]. */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_SDMA_FCTHR4_THR4_OFFSET ),	\
			VR9_SDMA_FCTHR4_THR4_SHIFT, VR9_SDMA_FCTHR4_THR4_SIZE, &pPar->nFlowCtrlConform_Max);
		return IFX_SUCCESS;
	} else {
		return IFX_ERROR;
	}
}

IFX_return_t IFX_FLOW_QoS_FlowctrlCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_FlowCtrlCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	/* Supported for GSWIP 2.2 and newer and returns with an error for older hardware revisions. */
	if ( pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2) {	
		/** Global Buffer Non Conforming Flow Control Threshold Minimum [number of segments]. */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_SDMA_FCTHR1_THR1_OFFSET ),	\
			VR9_SDMA_FCTHR1_THR1_SHIFT, VR9_SDMA_FCTHR1_THR1_SIZE, pPar->nFlowCtrlNonConform_Min);
		/** Global Buffer Non Conforming Flow Control Threshold Maximum [number of segments]. */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_SDMA_FCTHR2_THR2_OFFSET ),	\
			VR9_SDMA_FCTHR2_THR2_SHIFT, VR9_SDMA_FCTHR2_THR2_SIZE, pPar->nFlowCtrlNonConform_Max);
		/** Global Buffer Conforming Flow Control Threshold Minimum [number of segments]. */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_SDMA_FCTHR3_THR3_OFFSET ),	\
			VR9_SDMA_FCTHR3_THR3_SHIFT, VR9_SDMA_FCTHR3_THR3_SIZE, pPar->nFlowCtrlConform_Min);
		/** Global Buffer Conforming Flow Control Threshold Maximum [number of segments]. */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_SDMA_FCTHR4_THR4_OFFSET ),	\
			VR9_SDMA_FCTHR4_THR4_SHIFT, VR9_SDMA_FCTHR4_THR4_SIZE, pPar->nFlowCtrlConform_Max);
		return IFX_SUCCESS;
	} else {
		return IFX_ERROR;
	}
}

IFX_return_t IFX_FLOW_QoS_FlowctrlPortCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_FlowCtrlPortCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	if( pPar->nPortId >= (pEthSWDevHandle->nPortNumber) )
		return IFX_ERROR;
	/* Supported for GSWIP 2.2 and newer and returns with an error for older hardware revisions. */
	if ( pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2) {	
		/** Ingress Port occupied Buffer Flow Control Threshold Minimum [number of segments]. */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (GSWIP_SDMA_PFCTHR8_THR8_OFFSET +(pPar->nPortId * 0x4)),	\
			GSWIP_SDMA_PFCTHR8_THR8_SHIFT, GSWIP_SDMA_PFCTHR8_THR8_SIZE, &pPar->nFlowCtrl_Min);	
		/** Ingress Port occupied Buffer Flow Control Threshold Maximum [number of segments]. */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (GSWIP_SDMA_PFCTHR9_THR9_OFFSET +(pPar->nPortId * 0x4)),	\
			GSWIP_SDMA_PFCTHR9_THR9_SHIFT, GSWIP_SDMA_PFCTHR9_THR9_SIZE, &pPar->nFlowCtrl_Max);
		return IFX_SUCCESS;
	} else {
		return IFX_ERROR;
	}
}

IFX_return_t IFX_FLOW_QoS_FlowctrlPortCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_FlowCtrlPortCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	if( pPar->nPortId >= (pEthSWDevHandle->nPortNumber) )
		return IFX_ERROR;
	/* Supported for GSWIP 2.2 and newer and returns with an error for older hardware revisions. */
	if ( pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2) {	
		/** Ingress Port occupied Buffer Flow Control Threshold Minimum [number of segments]. */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (GSWIP_SDMA_PFCTHR8_THR8_OFFSET +(pPar->nPortId * 0x4)),	\
			GSWIP_SDMA_PFCTHR8_THR8_SHIFT, GSWIP_SDMA_PFCTHR8_THR8_SIZE, pPar->nFlowCtrl_Min);
		/** Ingress Port occupied Buffer Flow Control Threshold Maximum [number of segments]. */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (GSWIP_SDMA_PFCTHR9_THR9_OFFSET +(pPar->nPortId * 0x4)),	\
			GSWIP_SDMA_PFCTHR9_THR9_SHIFT, GSWIP_SDMA_PFCTHR9_THR9_SIZE, pPar->nFlowCtrl_Max);
		return IFX_SUCCESS;
	} else {
		return IFX_ERROR;
	}
}

IFX_return_t IFX_FLOW_QoS_QueueBufferReserveCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_QueueBufferReserveCfg_t *pPar)
{
	IFX_uint32_t  value, addr;
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_1_QUEUE_OFFSET,	\
		VR9_ETHSW_CAP_1_QUEUE_SHIFT, VR9_ETHSW_CAP_1_QUEUE_SIZE, &value);
	if ( pPar->nQueueId >= value )
		return IFX_ERROR;
	/* Supported for GSWIP 2.2 and newer and returns with an error for older hardware revisions. */
	if ( pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2) {	
			/* Colourcode=0  and fixed offset=0 */
			/* Set the BM RAM ADDR  */
//		addr = (((pPar->nQueueId << 3 ) & 0xF8) | 1);
		addr = (((pPar->nQueueId << 3 ) & 0xF8));
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_ADDR_ADDR_OFFSET,	\
		VR9_BM_RAM_ADDR_ADDR_SHIFT, VR9_BM_RAM_ADDR_ADDR_SIZE, addr);
		/* Specify the PQMCTXT = 0x9 */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_ADDR_OFFSET,	\
			VR9_BM_RAM_CTRL_ADDR_SHIFT, VR9_BM_RAM_CTRL_ADDR_SIZE, 0x9);
		/* Table Access Operation Mode Read */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_OPMOD_OFFSET,	\
			VR9_BM_RAM_CTRL_OPMOD_SHIFT, VR9_BM_RAM_CTRL_OPMOD_SIZE, 0x0);
		value = 1;
		/* Active */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_BAS_OFFSET,	\
			VR9_BM_RAM_CTRL_BAS_SHIFT, VR9_BM_RAM_CTRL_BAS_SIZE, value);
		do {
			ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_RAM_CTRL_BAS_OFFSET,	\
				VR9_BM_RAM_CTRL_BAS_SHIFT, VR9_BM_RAM_CTRL_BAS_SIZE, &value);
		} while(value);
	
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_RAM_VAL_0_VAL0_OFFSET,	\
			VR9_BM_RAM_VAL_0_VAL0_SHIFT, VR9_BM_RAM_VAL_0_VAL0_SIZE, &pPar->nBufferReserved);
		return IFX_SUCCESS;
	} else {
		return IFX_ERROR;
	}
}

IFX_return_t IFX_FLOW_QoS_QueueBufferReserveCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_QueueBufferReserveCfg_t *pPar)
{
	IFX_uint32_t  value, addr;
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_1_QUEUE_OFFSET,	\
		VR9_ETHSW_CAP_1_QUEUE_SHIFT, VR9_ETHSW_CAP_1_QUEUE_SIZE, &value);

	if ( pPar->nQueueId >= value )
		return IFX_ERROR;
	/* Supported for GSWIP 2.2 and newer and returns with an error for older hardware revisions. */
	if ( pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2) {	
		/* Colourcode=0  and fixed offset=0 */
		/* Set the BM RAM ADDR  */
//		addr = (((pPar->nQueueId << 3 ) & 0xF8) | 1);
		addr = (((pPar->nQueueId << 3 ) & 0xF8));
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_ADDR_ADDR_OFFSET,	\
			VR9_BM_RAM_ADDR_ADDR_SHIFT, VR9_BM_RAM_ADDR_ADDR_SIZE, addr);
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_VAL_0_VAL0_OFFSET,	\
			VR9_BM_RAM_VAL_0_VAL0_SHIFT, VR9_BM_RAM_VAL_0_VAL0_SIZE, pPar->nBufferReserved);
		/* Specify the PQMCTXT = 0x9 */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_ADDR_OFFSET,	\
			VR9_BM_RAM_CTRL_ADDR_SHIFT, VR9_BM_RAM_CTRL_ADDR_SIZE, 0x9);
		/* Table Access Operation Mode Write */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_OPMOD_OFFSET,	\
			VR9_BM_RAM_CTRL_OPMOD_SHIFT, VR9_BM_RAM_CTRL_OPMOD_SIZE, 0x1);
/*		IFXOS_PRINT_INT_RAW("Queue:%d,RAM_ADDR_ADDR = 0x%x, value:0x%0x, VAL0_ADDR:0x%0x,value:0x%0x\n",\
		pPar->nQueueId, VR9_BM_RAM_ADDR_ADDR_OFFSET, addr,VR9_BM_RAM_VAL_0_VAL0_OFFSET,	\
		pPar->nBufferReserved );*/
		value = 1;
		/* Active */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_BAS_OFFSET,	\
			VR9_BM_RAM_CTRL_BAS_SHIFT, VR9_BM_RAM_CTRL_BAS_SIZE, value);
		do {
			ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_RAM_CTRL_BAS_OFFSET,	\
				VR9_BM_RAM_CTRL_BAS_SHIFT, VR9_BM_RAM_CTRL_BAS_SIZE, &value);
		} while(value);
		return IFX_SUCCESS;
	} else {
		return IFX_ERROR;
	}
}

IFX_return_t IFX_FLOW_TimestampTimerSet(IFX_void_t *pDevCtx, IFX_FLOW_TIMESTAMP_Timer_t *pPar)
{
	IFX_uint32_t value;
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	/* Supported for GSWIP 2.2 and newer and returns with an error for older hardware revisions. */
	if ( pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2) {	
		/** Second. Absolute second timer count. */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (GSWIP_TIMER_SEC_LSB_SECLSB_OFFSET ),	\
			GSWIP_TIMER_SEC_LSB_SECLSB_SHIFT, GSWIP_TIMER_SEC_LSB_SECLSB_SIZE, (pPar->nSec & 0xFFFF));
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (GSWIP_TIMER_SEC_MSB_SECMSB_OFFSET ),	\
			GSWIP_TIMER_SEC_MSB_SECMSB_SHIFT, GSWIP_TIMER_SEC_MSB_SECMSB_SIZE, ((pPar->nSec >> 16) & 0xFFFF));
		/** Nano Second. Absolute nano second timer count. */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (GSWIP_TIMER_NS_LSB_NSLSB_OFFSET ),	\
			GSWIP_TIMER_NS_LSB_NSLSB_SHIFT, GSWIP_TIMER_NS_LSB_NSLSB_SIZE, (pPar->nNanoSec & 0xFFFF));
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (GSWIP_TIMER_NS_MSB_NSMSB_OFFSET ),	\
			GSWIP_TIMER_NS_MSB_NSMSB_SHIFT, GSWIP_TIMER_NS_MSB_NSMSB_SIZE, ((pPar->nNanoSec >> 16) & 0xFFFF));
   	/** Fractional Nano Second. Absolute fractional nano second timer count.
       This counter specifis a 2^32 fractional 'nNanoSec'. */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (GSWIP_TIMER_FS_LSB_FSLSB_OFFSET ),	\
			GSWIP_TIMER_FS_LSB_FSLSB_SHIFT, GSWIP_TIMER_FS_LSB_FSLSB_SIZE, (pPar->nFractionalNanoSec & 0xFFFF));
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (GSWIP_TIMER_FS_MSB_FSMSB_OFFSET ),	\
			GSWIP_TIMER_FS_MSB_FSMSB_SHIFT, GSWIP_TIMER_FS_MSB_FSMSB_SIZE, ((pPar->nFractionalNanoSec >> 16) & 0xFFFF));
		value = 1;
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (GSWIP_TIMER_CTRL_WR_OFFSET ),	\
			GSWIP_TIMER_CTRL_WR_SHIFT, GSWIP_TIMER_CTRL_WR_SIZE, value);
		do {
			ifx_ethsw_ll_DirectAccessRead(pDevCtx, (GSWIP_TIMER_CTRL_WR_OFFSET ),	\
				GSWIP_TIMER_CTRL_WR_SHIFT, GSWIP_TIMER_CTRL_WR_SIZE, &value);
		} while (value == 1 );
		return IFX_SUCCESS;
	} else {
		return IFX_ERROR;
	}
}
IFX_return_t IFX_FLOW_TimestampTimerGet(IFX_void_t *pDevCtx, IFX_FLOW_TIMESTAMP_Timer_t *pPar)
{
	IFX_uint32_t value;
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	value = 1;
	/* Supported for GSWIP 2.2 and newer and returns with an error for older hardware revisions. */
	if ( pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2) {	
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (GSWIP_TIMER_CTRL_RD_OFFSET ),	\
			GSWIP_TIMER_CTRL_RD_SHIFT, GSWIP_TIMER_CTRL_RD_SIZE, value);
		do {
			ifx_ethsw_ll_DirectAccessRead(pDevCtx, (GSWIP_TIMER_CTRL_RD_OFFSET ),	\
				GSWIP_TIMER_CTRL_RD_SHIFT, GSWIP_TIMER_CTRL_RD_SIZE, &value);
		} while (value == 1 );
		/** Second. Absolute second timer count. */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (GSWIP_TIMER_SEC_LSB_SECLSB_OFFSET ),	\
			GSWIP_TIMER_SEC_LSB_SECLSB_SHIFT, GSWIP_TIMER_SEC_LSB_SECLSB_SIZE, &value);
		pPar->nSec = value & 0xFFFF;
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (GSWIP_TIMER_SEC_MSB_SECMSB_OFFSET ),	\
			GSWIP_TIMER_SEC_MSB_SECMSB_SHIFT, GSWIP_TIMER_SEC_MSB_SECMSB_SIZE, &value);
		pPar->nSec |= (value & 0xFFFF << 16);
		/** Nano Second. Absolute nano second timer count. */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (GSWIP_TIMER_NS_LSB_NSLSB_OFFSET ),	\
			GSWIP_TIMER_NS_LSB_NSLSB_SHIFT, GSWIP_TIMER_NS_LSB_NSLSB_SIZE, &value);
		pPar->nNanoSec = value & 0xFFFF;
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (GSWIP_TIMER_NS_MSB_NSMSB_OFFSET ),	\
			GSWIP_TIMER_NS_MSB_NSMSB_SHIFT, GSWIP_TIMER_NS_MSB_NSMSB_SIZE, &value);
		pPar->nNanoSec |= (value & 0xFFFF << 16);
		/** Fractional Nano Second. Absolute fractional nano second timer count.
		This counter specifis a 2^32 fractional 'nNanoSec'. */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (GSWIP_TIMER_FS_LSB_FSLSB_OFFSET ),	\
			GSWIP_TIMER_FS_LSB_FSLSB_SHIFT, GSWIP_TIMER_FS_LSB_FSLSB_SIZE, &value);
		pPar->nFractionalNanoSec = value & 0xFFFF;
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (GSWIP_TIMER_FS_MSB_FSMSB_OFFSET ),	\
			GSWIP_TIMER_FS_MSB_FSMSB_SHIFT, GSWIP_TIMER_FS_MSB_FSMSB_SIZE, &value);
		pPar->nFractionalNanoSec |= (value & 0xFFFF << 16);
		return IFX_SUCCESS;
	} else {
		return IFX_ERROR;
	}
}
IFX_return_t IFX_FLOW_TimestampPortRead(IFX_void_t *pDevCtx, IFX_FLOW_TIMESTAMP_PortRead_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint32_t tstamp0, tstamp1;
	if( pPar->nPortId >= (pEthSWDevHandle->nPortNumber) )
		return IFX_ERROR;
	/* Supported for GSWIP 2.2 and newer and returns with an error for older hardware revisions. */
	if ( pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2) {	
		/** Second. Absolute second timer count. */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_FDMA_TSTAMP0_TSTL_OFFSET + (pPar->nPortId * 0x6) ),	\
			VR9_FDMA_TSTAMP0_TSTL_SHIFT, VR9_FDMA_TSTAMP0_TSTL_SIZE, &tstamp0);
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_FDMA_TSTAMP1_TSTH_OFFSET + (pPar->nPortId * 0x6) ),	\
			VR9_FDMA_TSTAMP1_TSTH_SHIFT, VR9_FDMA_TSTAMP1_TSTH_SIZE, &tstamp1);
		pPar->nEgressSec =  ( (tstamp0 |  (tstamp1  << 16) ) ) >> 30;
		pPar->nEgressNanoSec =  ( (tstamp0 |  (tstamp1  << 16) ) ) & 0x7FFFFFFF;
		/** Nano Second. Absolute nano second timer count. */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_SDMA_TSTAMP0_TSTL_OFFSET + + (pPar->nPortId * 0x6) ),	\
			VR9_SDMA_TSTAMP0_TSTL_SHIFT, VR9_SDMA_TSTAMP0_TSTL_SIZE, &tstamp0);
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_SDMA_TSTAMP1_TSTH_OFFSET + + (pPar->nPortId * 0x6) ),	\
			VR9_SDMA_TSTAMP1_TSTH_SHIFT, VR9_SDMA_TSTAMP1_TSTH_SIZE, &tstamp1);
		pPar->nIngressSec =  ( (tstamp0 |  (tstamp1  << 16) ) ) >> 30;
		pPar->nIngressNanoSec =  ( (tstamp0 |  (tstamp1  << 16) ) ) & 0x7FFFFFFF;
		return IFX_SUCCESS;
	} else {
		return IFX_ERROR;
	}
}
#if defined (CONFIG_LTQ_VLAN ) && CONFIG_LTQ_VLAN
IFX_return_t IFX_FLOW_VLAN_IdCreate(IFX_void_t *pDevCtx, IFX_ETHSW_VLAN_IdCreate_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint8_t table_index;
	IFX_FLOW_VLAN_tableEntry_t VLAN_Table_Entry;
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pcetable;
	/**  validation */
	if (IFX_FLOW_VLAN_Table_Index_Find(pEthSWDevHandle, pPar->nVId) != 0xFF ) {
		IFXOS_PRINT_INT_RAW("This vid exists\n");
		return IFX_ERROR;
	}
	table_index = IFX_FLOW_VLAN_Table_Empty_Entry_find(pEthSWDevHandle);
	if ( table_index == 0xFF ) {
		IFXOS_PRINT_INT_RAW("There is no table entry avariable\n");
		return IFX_ERROR;
	}
	// write to mirrored table
	memset(&VLAN_Table_Entry, 0, sizeof(IFX_FLOW_VLAN_tableEntry_t));
	memset(&pcetable, 0, sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
	VLAN_Table_Entry.valid = IFX_TRUE;
	VLAN_Table_Entry.vid = pPar->nVId;
	VLAN_Table_Entry.fid = pPar->nFId;
	IFX_FLOW_VLAN_Table_Entry_Set(pEthSWDevHandle, table_index, &VLAN_Table_Entry);
	/* write to ACTIVE VLAN Table */
	/* We fill all the PCE table requirement data */
	memset(&pcetable, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
	pcetable.table_index = table_index; //index of the VLAN ID configuration
	pcetable.table = IFX_ETHSW_PCE_ACTVLAN_INDEX;
	pcetable.key[0] = pPar->nVId;
	pcetable.val[0] = pPar->nFId; //Replacement Flag=0
	pcetable.valid = IFX_TRUE;
	ifx_ethsw_xwayflow_pce_table_write(pDevCtx, &pcetable);
	/* write to VLAN MAPPING Table */
	pcetable.table = IFX_ETHSW_PCE_VLANMAP_INDEX;
	pcetable.val[0] = pPar->nVId;
	pcetable.val[1] = 0; //reset Port Map and Tag Map values
	pcetable.val[2] = 0; //reset Port Map and Tag Map values
	ifx_ethsw_xwayflow_pce_table_write(pDevCtx, &pcetable);
#if defined(FLOW_TABLE_DEBUG) && FLOW_TABLE_DEBUG
	IFX_FLOW_VLAN_ActionMappingTable_Print();
#endif
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_VLAN_IdDelete(IFX_void_t *pDevCtx, IFX_ETHSW_VLAN_IdDelete_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pcetable;
	IFX_uint8_t table_index;
	IFX_FLOW_VLAN_tableEntry_t VLAN_Table_Entry;
	IFX_PCE_t *pVlanTmHandle = &pEthSWDevHandle->PCE_Handler;

	memset(&VLAN_Table_Entry, 0, sizeof(IFX_FLOW_VLAN_tableEntry_t));
	table_index = IFX_FLOW_VLAN_Table_Index_Find(pEthSWDevHandle, pPar->nVId);
	if (table_index == 0xFF) {
		IFXOS_PRINT_INT_RAW("ERROR: (VID does not exists) %s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__);
		return IFX_ERROR;
	}
	if ( get_ifx_pce_tm_vlan_act_tbl_index(&pVlanTmHandle->pce_sub_tbl,table_index)!= IFX_SUCCESS) {
		IFXOS_PRINT_INT_RAW("ERROR: (VID: 0x%0x used by flow table) %s:%s:%d \n",pPar->nVId, __FILE__, __FUNCTION__, __LINE__);
		return IFX_ERROR;
	}
	/*Remove the vlan entry from the Software index table */
	IFX_FLOW_VLAN_Table_Entry_Set(pEthSWDevHandle, table_index, &VLAN_Table_Entry);
	memset(&pcetable, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
	pcetable.table_index = table_index; /*index of the VLAN ID configuration */
	pcetable.table = IFX_ETHSW_PCE_ACTVLAN_INDEX;
	pcetable.valid = IFX_FALSE;
	ifx_ethsw_xwayflow_pce_table_write(pDevCtx, &pcetable);
	pcetable.table = IFX_ETHSW_PCE_VLANMAP_INDEX;
	ifx_ethsw_xwayflow_pce_table_write(pDevCtx, &pcetable);
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_VLAN_IdGet(IFX_void_t *pDevCtx, IFX_ETHSW_VLAN_IdGet_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint8_t table_index;
	IFX_FLOW_VLAN_tableEntry_t VLAN_Table_Entry;

	table_index = IFX_FLOW_VLAN_Table_Index_Find(pEthSWDevHandle, pPar->nVId);
	if (table_index == 0xFF) {
		IFXOS_PRINT_INT_RAW("ERROR: (VID does not exists) %s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__);
		return IFX_ERROR;
	}
	IFX_FLOW_VLAN_Table_Entry_Get ( pEthSWDevHandle, table_index, &VLAN_Table_Entry );
	pPar->nFId = VLAN_Table_Entry.fid;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_VLAN_PortCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_VLAN_portCfg_t *pPar)
{
	IFX_uint32_t value;
	IFX_int_t table_index;
	IFX_boolean_t bUVR, bVIMR, bVEMR;
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_FLOW_VLAN_tableEntry_t VLAN_Table_Entry;

	if (pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber)
		return IFX_ERROR;
   /* nPortId:port identification (0...6)
    * nPVid: read from PCE_DEFPVID,
    * retrieve the corresponding VLAN ID from the Active VLAN Table */
    ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_DEFPVID_PVID_OFFSET + (10 * pPar->nPortId),	\
    		VR9_PCE_DEFPVID_PVID_SHIFT, VR9_PCE_DEFPVID_PVID_SIZE, &value);

    table_index = value;
    IFX_FLOW_VLAN_Table_Entry_Get ( pEthSWDevHandle, table_index, &VLAN_Table_Entry );
    pPar->nPortVId = VLAN_Table_Entry.vid;
    /* bVlanUnknownDrop:
    * read from PCE_VCTRL.VIMR.UVR  */
    ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_VCTRL_UVR_OFFSET + (10 * pPar->nPortId),	\
    		VR9_PCE_VCTRL_UVR_SHIFT, VR9_PCE_VCTRL_UVR_SIZE, &value);
    bUVR = value;
    if ( bUVR == 1 )
    	pPar->bVLAN_UnknownDrop = IFX_TRUE;
    else
    	pPar->bVLAN_UnknownDrop = IFX_FALSE;
	/* bVlanReAssign:
    * read from PCE_VCTRL.VSR */
    ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_VCTRL_VSR_OFFSET + (10 * pPar->nPortId),	\
    		VR9_PCE_VCTRL_VSR_SHIFT, VR9_PCE_VCTRL_VSR_SIZE, &value);
    pPar->bVLAN_ReAssign = value;
    /* bVlanMemberViolationIngress:
    * read from PCE_VCTRL.VIMR    */
    ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_VCTRL_VIMR_OFFSET + (10 * pPar->nPortId),	\
    		VR9_PCE_VCTRL_VIMR_SHIFT, VR9_PCE_VCTRL_VIMR_SIZE, &value);
    bVIMR = value;
    /* bVlanMemberViolationEgress:
    * read from PCE_VCTRL.VEMR */
    ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_VCTRL_VEMR_OFFSET + (10 * pPar->nPortId),	\
    		VR9_PCE_VCTRL_VEMR_SHIFT, VR9_PCE_VCTRL_VEMR_SIZE, &value);
    bVEMR = value;
    if (bVIMR == IFX_FALSE && bVEMR == IFX_FALSE)
    	pPar->eVLAN_MemberViolation = IFX_ETHSW_VLAN_MEMBER_VIOLATION_NO;
    if (bVIMR == IFX_TRUE && bVEMR == IFX_FALSE)
    	pPar->eVLAN_MemberViolation = IFX_ETHSW_VLAN_MEMBER_VIOLATION_INGRESS;
    if (bVIMR == IFX_FALSE && bVEMR == IFX_TRUE)
    	pPar->eVLAN_MemberViolation = IFX_ETHSW_VLAN_MEMBER_VIOLATION_EGRESS;
    if (bVIMR == IFX_TRUE && bVEMR == IFX_TRUE)
    	pPar->eVLAN_MemberViolation = IFX_ETHSW_VLAN_MEMBER_VIOLATION_BOTH;
    /* eAdmitMode:
    * read from PCE_VCTRL.VINR  */
    ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_VCTRL_VINR_OFFSET + (10 * pPar->nPortId),	\
    		VR9_PCE_VCTRL_VINR_SHIFT, VR9_PCE_VCTRL_VINR_SIZE, &value);
    switch ( value ) {
    	case 0:
    		pPar->eAdmitMode = IFX_ETHSW_VLAN_ADMIT_ALL;
    		break;
    	case 1:
    		pPar->eAdmitMode = IFX_ETHSW_VLAN_ADMIT_TAGGED;
    		break;
    	case 2:
    		pPar->eAdmitMode = IFX_ETHSW_VLAN_ADMIT_UNTAGGED;
    		break;
    	default:
    		break;
    } /* -----  end switch  ----- */
    /* bTLS:
    * read from PCE_PCTRL.TVM  */
    ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_PCTRL_0_TVM_OFFSET + (10 * pPar->nPortId),	\
    		VR9_PCE_PCTRL_0_TVM_SHIFT, VR9_PCE_PCTRL_0_TVM_SIZE, &value);
    pPar->bTVM = value;
    return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_VLAN_PortCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_VLAN_portCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint32_t value;
	IFX_uint8_t table_index;

	if (pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber)
		return IFX_ERROR;
	/**  nPortId: port identification (0...6)
	* nPVid: write the VLAN index to PCE_DEFPVID */
	table_index = IFX_FLOW_VLAN_Table_Index_Find(pEthSWDevHandle, pPar->nPortVId);
	if (table_index == 0xFF) {
		IFXOS_PRINT_INT_RAW("ERROR: (VID does not exists) %s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__);
		return IFX_ERROR;
	}
	value = table_index;
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_DEFPVID_PVID_OFFSET + (10 * pPar->nPortId),	\
			VR9_PCE_DEFPVID_PVID_SHIFT, VR9_PCE_DEFPVID_PVID_SIZE, value);
	/** bVlanUnknownDrop:
	* write to PCE_VCTRL.VIMR/.UVR **/
	value = 0;
	if ( pPar->bVLAN_UnknownDrop == IFX_TRUE ) {
		value = 1;
	}
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_VCTRL_UVR_OFFSET + (10 * pPar->nPortId),	\
			VR9_PCE_VCTRL_UVR_SHIFT, VR9_PCE_VCTRL_UVR_SIZE, value);
	/** bVlanReAssign:
	* write to PCE_VCTRL.VSR */
	value = pPar->bVLAN_ReAssign;
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_VCTRL_VSR_OFFSET + (10 * pPar->nPortId),	\
			VR9_PCE_VCTRL_VSR_SHIFT, VR9_PCE_VCTRL_VSR_SIZE, value);
	/** bVlanMemberViolationIngress:
	* write to PCE_VCTRL.VIMR */
	switch ( pPar->eVLAN_MemberViolation ) {
		case IFX_ETHSW_VLAN_MEMBER_VIOLATION_NO:
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_VCTRL_VIMR_OFFSET + (10 * pPar->nPortId),	\
						VR9_PCE_VCTRL_VIMR_SHIFT, VR9_PCE_VCTRL_VIMR_SIZE, 0);
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_VCTRL_VEMR_OFFSET + (10 * pPar->nPortId),	\
						VR9_PCE_VCTRL_VEMR_SHIFT, VR9_PCE_VCTRL_VEMR_SIZE, 0);
			break;
		case IFX_ETHSW_VLAN_MEMBER_VIOLATION_INGRESS:
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_VCTRL_VIMR_OFFSET + (10 * pPar->nPortId),	\
						VR9_PCE_VCTRL_VIMR_SHIFT, VR9_PCE_VCTRL_VIMR_SIZE, 1);
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_VCTRL_VEMR_OFFSET + (10 * pPar->nPortId),	\
						VR9_PCE_VCTRL_VEMR_SHIFT, VR9_PCE_VCTRL_VEMR_SIZE, 0);
			break;
		case IFX_ETHSW_VLAN_MEMBER_VIOLATION_EGRESS:
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_VCTRL_VIMR_OFFSET + (10 * pPar->nPortId),	\
					VR9_PCE_VCTRL_VIMR_SHIFT, VR9_PCE_VCTRL_VIMR_SIZE, 0);
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_VCTRL_VEMR_OFFSET + (10 * pPar->nPortId),	\
					VR9_PCE_VCTRL_VEMR_SHIFT, VR9_PCE_VCTRL_VEMR_SIZE, 1);
			break;
		case IFX_ETHSW_VLAN_MEMBER_VIOLATION_BOTH:
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_VCTRL_VIMR_OFFSET + (10 * pPar->nPortId),	\
						VR9_PCE_VCTRL_VIMR_SHIFT, VR9_PCE_VCTRL_VIMR_SIZE, 1);
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_VCTRL_VEMR_OFFSET + (10 * pPar->nPortId),	\
					VR9_PCE_VCTRL_VEMR_SHIFT, VR9_PCE_VCTRL_VEMR_SIZE, 1);
			break;
		default:
			IFXOS_PRINT_INT_RAW("WARNING: (eVLAN_MemberViolation) %s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__);
	} /* -----  end switch  ----- */
	/** eAdmitMode:
	* write to PCE_VCTRL.VINR */
	switch ( pPar->eAdmitMode ) {
		case IFX_ETHSW_VLAN_ADMIT_ALL:
			value = 0;
			break;
		case IFX_ETHSW_VLAN_ADMIT_TAGGED:
			value = 1;
			break;
		case IFX_ETHSW_VLAN_ADMIT_UNTAGGED:
			value = 2;
			break;
		default:
			value = 0;
			IFXOS_PRINT_INT_RAW("WARNING: (eAdmitMode) %s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__);
	} /* -----  end switch  ----- */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_VCTRL_VINR_OFFSET + (10 * pPar->nPortId),	\
				VR9_PCE_VCTRL_VINR_SHIFT, VR9_PCE_VCTRL_VINR_SIZE, value);
	/** bTLS:
	* write to PCE_PCTRL.TVM */
	value = 0;
	if ( pPar->bTVM == IFX_TRUE) {
		value = 1;
	}
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PCTRL_0_TVM_OFFSET + (10 * pPar->nPortId),	\
				VR9_PCE_PCTRL_0_TVM_SHIFT, VR9_PCE_PCTRL_0_TVM_SIZE, value);
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_VLAN_PortMemberAdd(IFX_void_t *pDevCtx, IFX_ETHSW_VLAN_portMemberAdd_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pcetable;
	IFX_uint8_t table_index;
	IFX_FLOW_VLAN_tableEntry_t VLAN_Table_Entry;

	if ( (pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber) && ( !(pPar->nPortId & 0x80000000)) )
		return IFX_ERROR;
	memset(&VLAN_Table_Entry, 0, sizeof(IFX_FLOW_VLAN_tableEntry_t));
	table_index = IFX_FLOW_VLAN_Table_Index_Find(pEthSWDevHandle, pPar->nVId);
	if (table_index == 0xFF) {
		IFXOS_PRINT_INT_RAW("ERROR: (VID does not exists) %s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__);
		return IFX_ERROR;
	}
	IFX_FLOW_VLAN_Table_Entry_Get(pEthSWDevHandle, table_index, &VLAN_Table_Entry);
	if (VLAN_Table_Entry.reserved == IFX_TRUE) {
		IFXOS_PRINT_INT_RAW("ERROR: (VID was already reserved) %s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__);
		return IFX_ERROR;
	}
	/*  Support  portmap information.
	*  To differentiate between port index and portmap, the MSB (highest data bit) should be 1. */
	if (pPar->nPortId & 0x80000000) { /*Port Map */
		VLAN_Table_Entry.pm |= ( (pPar->nPortId) & 0x7FFF ) ;
		if (pPar->bVLAN_TagEgress)
			VLAN_Table_Entry.tm |= ( (pPar->nPortId) & 0x7FFF ) ;
		else
			VLAN_Table_Entry.tm &= ~((pPar->nPortId) & 0x7FFF);
	} else {
		VLAN_Table_Entry.pm |= 1 << pPar->nPortId; /*  single port index */
		if (pPar->bVLAN_TagEgress)
			VLAN_Table_Entry.tm |= 1 << pPar->nPortId;
		else
			VLAN_Table_Entry.tm &= ~(1 << pPar->nPortId);
	}
	IFX_FLOW_VLAN_Table_Entry_Set(pEthSWDevHandle, table_index, &VLAN_Table_Entry);
	/* Read VID from the table */
	pcetable.table = IFX_ETHSW_PCE_VLANMAP_INDEX;
	pcetable.table_index = table_index;
	ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pcetable);
	pcetable.table = IFX_ETHSW_PCE_VLANMAP_INDEX;
	pcetable.table_index = table_index;
	pcetable.val[1] = VLAN_Table_Entry.pm;
	pcetable.val[2] = VLAN_Table_Entry.tm;
	ifx_ethsw_xwayflow_pce_table_write(pDevCtx, &pcetable);
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_VLAN_PortMemberRead(IFX_void_t *pDevCtx, IFX_ETHSW_VLAN_portMemberRead_t *pPar)
{

	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pcetable;

	if (pPar->bInitial == IFX_TRUE) {
		pEthSWDevHandle->vlan_table_index = 0; /*Start from the index 0 */
		pcetable.table = IFX_ETHSW_PCE_VLANMAP_INDEX;
		pcetable.table_index = pEthSWDevHandle->vlan_table_index; /* vlan_index retrieved from the Active VLAN table */
		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pcetable);
		pPar->nVId				= (pcetable.val[0] & 0xFFF);
#if 0
		pPar->nPortId			= (pcetable.val[1]);		/* | 0x80000000); Port Map */
		pPar->nTagId			= (pcetable.val[2]);	/* | 0x80000000); Port Map */
#else
		pPar->nPortId			= (pcetable.val[1]| 0x80000000); /* Port Map */
		pPar->nTagId			= (pcetable.val[2]| 0x80000000); /*Port Map */
#endif
		pPar->bInitial = IFX_FALSE;
		pPar->bLast = IFX_FALSE;
	}

	if ( (pPar->bLast != IFX_TRUE)  ) {
		if (pEthSWDevHandle->vlan_table_index < IFX_HW_VLAN_MAP_TABLE_MAX) {
			pEthSWDevHandle->vlan_table_index++;
			pcetable.table = IFX_ETHSW_PCE_VLANMAP_INDEX;
			pcetable.table_index = pEthSWDevHandle->vlan_table_index; /* vlan_index retrieved from the Active VLAN table */
			ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pcetable);
			pPar->nVId				= (pcetable.val[0] & 0xFFF);
#if 0
			pPar->nPortId			= (pcetable.val[1]);	/* | 0x80000000); Port Map */
			pPar->nTagId			= (pcetable.val[2]);	/* | 0x80000000);Port Map */ 
#else
			pPar->nPortId			= (pcetable.val[1] | 0x80000000);	/* Port Map */
			pPar->nTagId			= (pcetable.val[2] | 0x80000000);	/* Port Map */ 
#endif
		} else {
			pPar->bLast = IFX_TRUE;
			pEthSWDevHandle->vlan_table_index = 0;
		}
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_VLAN_PortMemberRemove(IFX_void_t *pDevCtx, IFX_ETHSW_VLAN_portMemberRemove_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pcetable;
	IFX_FLOW_VLAN_tableEntry_t VLAN_Table_Entry;
	IFX_uint8_t table_index;
	if ( (pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber) && ( !(pPar->nPortId & 0x80000000)) )
		return IFX_ERROR;
	memset(&VLAN_Table_Entry, 0, sizeof(IFX_FLOW_VLAN_tableEntry_t));
	table_index = IFX_FLOW_VLAN_Table_Index_Find(pEthSWDevHandle, pPar->nVId);
	if (table_index == 0xFF) {
		IFXOS_PRINT_INT_RAW("This vid doesn't exists\n");
		return IFX_ERROR;
	}
	IFX_FLOW_VLAN_Table_Entry_Get(pEthSWDevHandle, table_index, &VLAN_Table_Entry);
	/*  Support  portmap information.
	*  To differentiate between port index and portmap, the MSB (highest data bit) should be 1.
	*/
	if (pPar->nPortId & 0x80000000) {
		VLAN_Table_Entry.pm  &= ~( (pPar->nPortId) & 0x7FFF ) ;
	} else {
		VLAN_Table_Entry.pm &= ~(1 << pPar->nPortId);
	}
	VLAN_Table_Entry.tm &= ~(1 << pPar->nPortId);
	IFX_FLOW_VLAN_Table_Entry_Set(pEthSWDevHandle, table_index, &VLAN_Table_Entry);
	pcetable.table = IFX_ETHSW_PCE_VLANMAP_INDEX;
	pcetable.table_index = table_index; //vlan_index retrieved from the Active VLAN table
	ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pcetable);
	if (pPar->nPortId & 0x80000000) {
		pcetable.val[1] &= ~((pPar->nPortId) & 0x7FFF);
		pcetable.val[2] &= ~((pPar->nPortId) & 0x7FFF);
	} else {
		pcetable.val[1] &= ~(1 << pPar->nPortId);
		pcetable.val[2] &= ~(1 << pPar->nPortId);
	}
	ifx_ethsw_xwayflow_pce_table_write(pDevCtx, &pcetable);
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_VLAN_ReservedAdd(IFX_void_t *pDevCtx, IFX_ETHSW_VLAN_reserved_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pcetable;
	IFX_uint8_t table_index;
	IFX_FLOW_VLAN_tableEntry_t VLAN_Table_Entry;

	table_index = IFX_FLOW_VLAN_Table_Index_Find(pEthSWDevHandle, pPar->nVId);

	if (table_index == 0xFF) {
		IFXOS_PRINT_INT_RAW("This vid doesn't exists, Please create VID first\n");
		return IFX_ERROR;
	}
	IFX_FLOW_VLAN_Table_Entry_Get(pEthSWDevHandle, table_index, &VLAN_Table_Entry);
	if ( VLAN_Table_Entry.pm != 0) {
		IFXOS_PRINT_INT_RAW("This VID was already add into the member, can not reserve\n");
		return IFX_ERROR;
	}
	if ( VLAN_Table_Entry.tm != 0) {
		IFXOS_PRINT_INT_RAW("This VID was already add into the member, can not reserve\n");
		return IFX_ERROR;
	}
	VLAN_Table_Entry.reserved = IFX_TRUE;
	IFX_FLOW_VLAN_Table_Entry_Set(pEthSWDevHandle, table_index, &VLAN_Table_Entry);
	memset(&pcetable, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
	pcetable.table_index = table_index; //index of the VLAN ID configuration
	pcetable.table = IFX_ETHSW_PCE_ACTVLAN_INDEX;
	pcetable.val[0] |= (1 << 8);
	ifx_ethsw_xwayflow_pce_table_write(pDevCtx, &pcetable);
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_VLAN_ReservedRemove(IFX_void_t *pDevCtx, IFX_ETHSW_VLAN_reserved_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pcetable;
	IFX_uint8_t table_index;
	IFX_FLOW_VLAN_tableEntry_t VLAN_Table_Entry;

	table_index = IFX_FLOW_VLAN_Table_Index_Find(pEthSWDevHandle, pPar->nVId);
   if (table_index == 0xFF) {
   		IFXOS_PRINT_INT_RAW("This vid doesn't exists, Please create VID first\n");
   		return IFX_ERROR;
   	}
	IFX_FLOW_VLAN_Table_Entry_Get(pEthSWDevHandle, table_index, &VLAN_Table_Entry);
	if ( VLAN_Table_Entry.pm != 0) {
		IFXOS_PRINT_INT_RAW("This VID was already add into the member, can not remove the reserve\n");
		return IFX_ERROR;
	}
	if ( VLAN_Table_Entry.tm != 0) {
		IFXOS_PRINT_INT_RAW("This VID was already add into the member, can not remove the reserve\n");
		return IFX_ERROR;
	}
	if (VLAN_Table_Entry.reserved == IFX_FALSE) {
		IFXOS_PRINT_INT_RAW("This VID was not reserve, please reserve it first\n");
		return IFX_ERROR;
	} else {
		VLAN_Table_Entry.reserved = IFX_FALSE;
		IFX_FLOW_VLAN_Table_Entry_Set(pEthSWDevHandle, table_index, &VLAN_Table_Entry);
	}
	IFX_FLOW_VLAN_Table_Entry_Set(pEthSWDevHandle, table_index, &VLAN_Table_Entry);
	memset(&pcetable, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
	pcetable.table_index = table_index; //index of the VLAN ID configuration
	pcetable.table = IFX_ETHSW_PCE_ACTVLAN_INDEX;
	pcetable.val[0] &=~ (1 << 8);
	ifx_ethsw_xwayflow_pce_table_write(pDevCtx, &pcetable);
	return IFX_SUCCESS;
}
#endif /*CONFIG_LTQ_VLAN */
#if defined (CONFIG_LTQ_QOS ) && CONFIG_LTQ_QOS
IFX_return_t IFX_FLOW_QoS_MeterCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_meterCfg_t *pPar)
{
	IFX_uint32_t nMeterId = pPar->nMeterId;
	IFX_uint32_t  value, exp, mant, ibs;

	if( nMeterId > 7 )
		return IFX_ERROR;

	/* Enable/Disable the meter shaper */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_TCM_CTRL_TCMEN_OFFSET + (nMeterId * 7),	\
		VR9_PCE_TCM_CTRL_TCMEN_SHIFT, VR9_PCE_TCM_CTRL_TCMEN_SIZE, &value);
	pPar->bEnable = value;
	/* Committed Burst Size */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_TCM_CBS_CBS_OFFSET + (nMeterId * 7),	\
		VR9_PCE_TCM_CBS_CBS_SHIFT, VR9_PCE_TCM_CBS_CBS_SIZE, &value);
	pPar->nCbs = (value * 64);
	/* Excess Burst Size (EBS [bytes]) */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_TCM_EBS_EBS_OFFSET + (nMeterId * 7),	\
		VR9_PCE_TCM_EBS_EBS_SHIFT, VR9_PCE_TCM_EBS_EBS_SIZE, &value);
	pPar->nEbs = (value * 64);
	/* Rate Counter Exponent */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_TCM_CIR_EXP_EXP_OFFSET + (nMeterId * 7),	\
		VR9_PCE_TCM_CIR_EXP_EXP_SHIFT, VR9_PCE_TCM_CIR_EXP_EXP_SIZE, &exp);
	/* Rate Counter Mantissa */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_TCM_CIR_MANT_MANT_OFFSET + (nMeterId * 7),
                              VR9_PCE_TCM_CIR_MANT_MANT_SHIFT, VR9_PCE_TCM_CIR_MANT_MANT_SIZE, &mant);
   /* Rate Counter iBS */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_TCM_IBS_IBS_OFFSET + (nMeterId * 7),	\
		VR9_PCE_TCM_IBS_IBS_SHIFT, VR9_PCE_TCM_IBS_IBS_SIZE, &ibs);
	/* calc the Rate */
	pPar->nRate = RateCalc(ibs, exp, mant);
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_MeterCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_meterCfg_t *pPar)
{
	IFX_uint32_t nMeterId = pPar->nMeterId;
	IFX_uint32_t nCbs = 0, nEbs = 0;
	IFX_uint32_t  exp = 0, mant = 0, rate = 0, ibs = 0;

	if( nMeterId > 7 )
		return IFX_ERROR;
	/* Committed Burst Size */
	if (pPar->nCbs > 0xFFC0)
		nCbs = 0x3FF;
	else
		nCbs = ((pPar->nCbs + 63) / 64);
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_TCM_CBS_CBS_OFFSET + (nMeterId * 7),	\
		VR9_PCE_TCM_CBS_CBS_SHIFT, VR9_PCE_TCM_CBS_CBS_SIZE, nCbs);
	/* Excess Burst Size (EBS [bytes]) */
	if (pPar->nEbs > 0xFFC0)
		nEbs = 0x3FF;
	else
		nEbs = ((pPar->nEbs + 63) / 64);
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_TCM_EBS_EBS_OFFSET + (nMeterId * 7),	\
		VR9_PCE_TCM_EBS_EBS_SHIFT, VR9_PCE_TCM_EBS_EBS_SIZE, nEbs);
	/* Calc the Rate and convert to MANT and EXP*/
	rate = pPar->nRate;
	if (rate)
		calcToken(rate, &ibs, &exp, &mant);
	/* Rate Counter Exponent */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_TCM_CIR_EXP_EXP_OFFSET + (nMeterId * 7),	\
		VR9_PCE_TCM_CIR_EXP_EXP_SHIFT, VR9_PCE_TCM_CIR_EXP_EXP_SIZE, exp);
	/* Rate Counter Mantissa */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_TCM_CIR_MANT_MANT_OFFSET + (nMeterId * 7),	\
		VR9_PCE_TCM_CIR_MANT_MANT_SHIFT, VR9_PCE_TCM_CIR_MANT_MANT_SIZE, mant);
	/* Rate Counter iBS */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_TCM_IBS_IBS_OFFSET + (nMeterId * 7),	\
		VR9_PCE_TCM_IBS_IBS_SHIFT, VR9_PCE_TCM_IBS_IBS_SIZE, ibs);
	/* Enable/Disable the meter shaper */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_TCM_CTRL_TCMEN_OFFSET + (nMeterId * 7),	\
		VR9_PCE_TCM_CTRL_TCMEN_SHIFT, VR9_PCE_TCM_CTRL_TCMEN_SIZE, pPar->bEnable);
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_MeterPortAssign(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_meterPort_t *pPar)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pcetable;
	IFX_uint32_t  EgressPortId, IngressPortId, value1, value2;
	IFX_boolean_t EntryFound_Tbl = IFX_FALSE;
	IFX_boolean_t noEmptyEntry_Tb1 = IFX_FALSE;
	memset(&pcetable, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
	value1 = 0;
	while ( value1 < 8 ) {
		pcetable.table = IFX_ETHSW_PCE_METER_INS_0_INDEX;
		pcetable.table_index = value1; //index of the Meter Ins 0 configuration
		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pcetable);

		if (pcetable.valid == 1) {
			IngressPortId= pcetable.key[0] & 0xF;
			EgressPortId= (pcetable.key[0] >> 8) & 0xF;
			if ((EgressPortId == pPar->nPortEgressId) && (IngressPortId == pPar->nPortIngressId)) {
				EntryFound_Tbl = IFX_TRUE;
				value2 = 0;
				while ( value2 < 8 ) {
					pcetable.table = IFX_ETHSW_PCE_METER_INS_1_INDEX;
					pcetable.table_index = value2; //index of the Meter Ins 1 configuration
					ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pcetable);
					if (pcetable.valid == 1) {
						IngressPortId= pcetable.key[0] & 0xF;
						EgressPortId= (pcetable.key[0] >> 8) & 0xF;
						if ((EgressPortId == pPar->nPortEgressId) && (IngressPortId == pPar->nPortIngressId))
							return IFX_ERROR;
					}
					value2++;
				}
			}
		}
		value1++;
	}
	/*  Not in the original table, write a new one */
	if ( EntryFound_Tbl == IFX_FALSE ) {
		value1 = 0;
		memset(&pcetable, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
		/* Search for whole Table 1*/
		while ( value1 < 8 ) {
			pcetable.table = IFX_ETHSW_PCE_METER_INS_0_INDEX;
			pcetable.table_index = value1; //index of the Meter Ins 0 configuration
			ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pcetable);
			/* We found the empty one */
			if (pcetable.valid == 0) {
				switch (pPar->eDir) {
					case IFX_ETHSW_DIRECTION_BOTH:
						pcetable.key[0] = (((pPar->nPortEgressId & 0xF ) << 8) |	\
							( pPar->nPortIngressId & 0xF));
						pcetable.mask = 0;
					break;
					case IFX_ETHSW_DIRECTION_EGRESS:
						pcetable.key[0] = (((pPar->nPortEgressId & 0xF) << 8) | 0xF);
						pcetable.mask = 1;
						break;
					case IFX_ETHSW_DIRECTION_INGRESS:
						pcetable.key[0] = (0xF00 | (pPar->nPortIngressId & 0xF) );
						pcetable.mask = 4;
						break;
					default:
						pcetable.key[0] = 0;
						pcetable.mask = 5;
				}
				pcetable.val[0] = pPar->nMeterId & 0x3F;
				pcetable.valid = 1;
				ifx_ethsw_xwayflow_pce_table_write(pDevCtx, &pcetable);
				return IFX_SUCCESS;
			}
			value1++;
		}
		if (value1 >= 8)
			noEmptyEntry_Tb1 = IFX_TRUE;
	}

	/* The Table 1 is full, We go search table 2 */
	if ( (noEmptyEntry_Tb1 == IFX_TRUE) || (EntryFound_Tbl == IFX_TRUE)) {
		value2 = 0;
		memset(&pcetable, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
		while ( value2 < 8 ) {
			pcetable.table = IFX_ETHSW_PCE_METER_INS_1_INDEX;
			pcetable.table_index = value2; //index of the Meter Ins 0 configuration
			ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pcetable);
			/* We found the empty one */
			if (pcetable.valid == 0) {
				switch (pPar->eDir) {
					case IFX_ETHSW_DIRECTION_BOTH:
						pcetable.key[0] = (((pPar->nPortEgressId & 0xF ) << 8) |	\
							( pPar->nPortIngressId & 0xF));
						pcetable.mask = 0;
					break;
					case IFX_ETHSW_DIRECTION_EGRESS:
						pcetable.key[0] = (((pPar->nPortEgressId & 0xF) << 8) | 0xF);
						pcetable.mask = 1;
						break;
					case IFX_ETHSW_DIRECTION_INGRESS:
						pcetable.key[0] = (0xF00 | (pPar->nPortIngressId & 0xF) );
						pcetable.mask = 4;
						break;
					default:
						pcetable.key[0] = 0;
						pcetable.mask = 5;
				}
				pcetable.val[0] = pPar->nMeterId & 0x3F;
				pcetable.valid = 1;
				ifx_ethsw_xwayflow_pce_table_write(pDevCtx, &pcetable);
				noEmptyEntry_Tb1 = IFX_FALSE;
				return IFX_SUCCESS;
			}
			value2++;
		}
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_MeterPortDeassign(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_meterPort_t *pPar)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pcetable;
	IFX_uint32_t  EgressPortId, IngressPortId, MeterId;
	IFX_boolean_t EntryFound_Tbl = IFX_FALSE;
	IFX_uint32_t i, j;

	for ( i = 0; i < 2;i++) {
		memset(&pcetable, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
		if ( i == 0)
			pcetable.table = IFX_ETHSW_PCE_METER_INS_0_INDEX;
		else
			pcetable.table = IFX_ETHSW_PCE_METER_INS_1_INDEX;
		for ( j = 0; j < 8; j++) {
			pcetable.table_index = j; //index of the Meter Ins 1 configuration
			ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pcetable);
			if (pcetable.valid == 1) {
				IngressPortId= pcetable.key[0] & 0xF;
				EgressPortId= (pcetable.key[0] >> 8) & 0xF;
				MeterId = pcetable.val[0] & 0x1F;
				if ((EgressPortId == pPar->nPortEgressId) &&	\
					(IngressPortId == pPar->nPortIngressId) && (MeterId == pPar->nMeterId)) {
					if ( i == 0)
						pcetable.table = IFX_ETHSW_PCE_METER_INS_0_INDEX;
					else
						pcetable.table = IFX_ETHSW_PCE_METER_INS_1_INDEX;
					pcetable.key[0] = 0;
					pcetable.val[0] = 0;
					pcetable.valid = 0;
					ifx_ethsw_xwayflow_pce_table_write(pDevCtx, &pcetable);
					EntryFound_Tbl = IFX_TRUE;
					IFXOS_PRINT_INT_RAW("Found the entry, delet it\n");
				}

			}
		}
	}
	if (EntryFound_Tbl == IFX_FALSE)
		IFXOS_PRINT_INT_RAW("The Entry not found\n");
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_MeterPortGet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_meterPortGet_t *pPar)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pcetable;
	IFX_uint32_t value = 0;
	IFX_FLOW_switchDev_t *pEthSWDev = (IFX_FLOW_switchDev_t*)pDevCtx;
	memset(&pcetable, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_3_METERS_OFFSET,	\
				VR9_ETHSW_CAP_3_METERS_SHIFT, VR9_ETHSW_CAP_3_METERS_SIZE, &value);
/*	IFXOS_PRINT_INT_RAW(" No of  Meters:%d,pEthSWDev->meter_cnt=%d \n", value,pEthSWDev->meter_cnt); */
	if (pPar->bInitial == IFX_TRUE) {
		pEthSWDev->meter_cnt = 0;
		pPar->bInitial = IFX_FALSE;
	} else {
		if ( pEthSWDev->meter_cnt > (value * 2))
			pPar->bLast = IFX_TRUE;
	}
	if( pEthSWDev->meter_cnt > value)
		pcetable.table = IFX_ETHSW_PCE_METER_INS_1_INDEX;
	else
		pcetable.table = IFX_ETHSW_PCE_METER_INS_0_INDEX;
	pcetable.table_index = pEthSWDev->meter_cnt;
	ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pcetable);
	if ( pcetable.valid) {
		pPar->nMeterId = (pcetable.val[0] & 0x3F);
		pPar->nPortEgressId = ((pcetable.key[0] >> 8) & 0xF);
		pPar->nPortIngressId = (pcetable.key[0] & 0xF);
		if ( (pcetable.mask & 0x5) == 0)
				pPar->eDir = IFX_ETHSW_DIRECTION_BOTH;
		else if ( (pcetable.mask & 0x5) == 1)
			pPar->eDir = IFX_ETHSW_DIRECTION_EGRESS;
		else if ( (pcetable.mask & 0x5) == 4)
			pPar->eDir = IFX_ETHSW_DIRECTION_INGRESS;
		else
			pPar->eDir = IFX_ETHSW_DIRECTION_NONE;
	}
	pEthSWDev->meter_cnt++;
/*	IFXOS_PRINT_INT_RAW(" aftterNo of  Meters:%d,pEthSWDev->meter_cnt=%d \n", value,pEthSWDev->meter_cnt); */
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_DSCP_ClassGet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_DSCP_ClassCfg_t *pPar)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pcetable;
	IFX_uint32_t  value;

	for ( value =0; value <= 63; value++) {
		pcetable.table = IFX_ETHSW_PCE_DSCP_INDEX;
		pcetable.table_index = value; //index of the DSCP configuration
		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pcetable);
		pPar->nTrafficClass[value] = pcetable.val[0] & 0xF;
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_DSCP_ClassSet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_DSCP_ClassCfg_t *pPar)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pcetable;
	IFX_uint32_t  value;

	for( value =0; value <= 63; value++) {
		pcetable.table = IFX_ETHSW_PCE_DSCP_INDEX;
		pcetable.table_index = value; //index of the DSCP configuration
		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pcetable);
		pcetable.val[0] &= ~( 0xF  );
		pcetable.val[0] |= (pPar->nTrafficClass[value] & 0xF );
		ifx_ethsw_xwayflow_pce_table_write(pDevCtx, &pcetable);
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_ClassDSCP_Get(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_ClassDSCP_Cfg_t *pPar)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pcetable;
	IFX_uint32_t  value;

	for ( value =0; value < 16; value++) {
		pcetable.table = IFX_ETHSW_PCE_REMARKING_INDEX;
		pcetable.table_index = value; //index of the DSCP configuration
		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pcetable);
		pPar->nDSCP[value] = pcetable.val[0] & 0x3F;
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_ClassDSCP_Set(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_ClassDSCP_Cfg_t *pPar)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pcetable;
	IFX_uint32_t  value;
	IFX_uint32_t  nDSCP, nPCP;

	for( value =0; value < 16; value++) {
		pcetable.table = IFX_ETHSW_PCE_REMARKING_INDEX;
		pcetable.table_index = value; //index of the DSCP configuration
		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pcetable);
		nPCP = (pcetable.val[0] >> 8 ) & 0x7;
		nDSCP = pPar->nDSCP[value] & 0x3F;
		pcetable.val[0] = (nPCP << 8) | nDSCP;
		ifx_ethsw_xwayflow_pce_table_write(pDevCtx, &pcetable);
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_DSCP_DropPrecedenceCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_DSCP_DropPrecedenceCfg_t *pPar)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pcetable;
	IFX_uint32_t  value;

	for ( value =0; value <= 63; value++) {
		memset(&pcetable, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
		pcetable.table = IFX_ETHSW_PCE_DSCP_INDEX;
		pcetable.table_index = value; //index of the DSCP configuration
		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pcetable);
		pPar->nDSCP_DropPrecedence[value] = (pcetable.val[0] >> 4) & 0x3;
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_DSCP_DropPrecedenceCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_DSCP_DropPrecedenceCfg_t *pPar)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pcetable;
	IFX_uint32_t  value;

	for( value =0; value <= 63; value++) {
		memset(&pcetable, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
		pcetable.table = IFX_ETHSW_PCE_DSCP_INDEX;
		pcetable.table_index = value; //index of the DSCP configuration
		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pcetable);
		pcetable.val[0] &= ~( 0x3 << 4 );
		pcetable.val[0] |= ( (pPar->nDSCP_DropPrecedence[value] & 0x3) << 4);
		ifx_ethsw_xwayflow_pce_table_write(pDevCtx, &pcetable);
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_PortRemarkingCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_portRemarkingCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint32_t  value, value_CLPEN=0, value_DPEN=0, value_DSCPMOD=0;

	if( pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber)
		return IFX_ERROR;

	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_PCTRL_0_CLPEN_OFFSET + (10 * pPar->nPortId),	\
		VR9_PCE_PCTRL_0_CLPEN_SHIFT, VR9_PCE_PCTRL_0_CLPEN_SIZE, &value_CLPEN);
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_PCTRL_0_DPEN_OFFSET + (10 * pPar->nPortId),	\
		VR9_PCE_PCTRL_0_DPEN_SHIFT, VR9_PCE_PCTRL_0_DPEN_SIZE, &value_DPEN);
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_PCTRL_2_DSCPMOD_OFFSET + (10 * pPar->nPortId),	\
		VR9_PCE_PCTRL_2_DSCPMOD_SHIFT, VR9_PCE_PCTRL_2_DSCPMOD_SIZE, &value_DSCPMOD);

	if (( value_CLPEN == 0 ) && ( value_DPEN == 0 ) && (value_DSCPMOD == 0))
		pPar->eDSCP_IngressRemarkingEnable = IFX_ETHSW_DSCP_REMARK_DISABLE;
	else if (( value_CLPEN == 1 ) && ( value_DPEN == 0 ) && ( value_DSCPMOD == 1 ))
		pPar->eDSCP_IngressRemarkingEnable = IFX_ETHSW_DSCP_REMARK_TC6;
	else if (( value_CLPEN == 1 ) && ( value_DPEN == 1 ) && ( value_DSCPMOD == 1 ))
		pPar->eDSCP_IngressRemarkingEnable = IFX_ETHSW_DSCP_REMARK_TC3;
	else if (( value_CLPEN == 0 ) && ( value_DPEN == 1 ) && ( value_DSCPMOD == 1 ))
		pPar->eDSCP_IngressRemarkingEnable = IFX_ETHSW_DSCP_REMARK_DP3;
	else if (( value_CLPEN == 1 ) && ( value_DPEN == 1 ) && ( value_DSCPMOD == 1 ))
		pPar->eDSCP_IngressRemarkingEnable = IFX_ETHSW_DSCP_REMARK_DP3_TC3;

	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_PCTRL_0_PCPEN_OFFSET + (10 * pPar->nPortId),\
		 VR9_PCE_PCTRL_0_PCPEN_SHIFT, VR9_PCE_PCTRL_0_PCPEN_SIZE, &value);
	pPar->bPCP_IngressRemarkingEnable = value;
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_FDMA_PCTRL_DSCPRM_OFFSET + (6 * pPar->nPortId),	\
		VR9_FDMA_PCTRL_DSCPRM_SHIFT, VR9_FDMA_PCTRL_DSCPRM_SIZE, &value);
	pPar->bDSCP_EgressRemarkingEnable = value;
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_FDMA_PCTRL_VLANMOD_OFFSET + (6 * pPar->nPortId),	\
		VR9_FDMA_PCTRL_VLANMOD_SHIFT, VR9_FDMA_PCTRL_VLANMOD_SIZE, &value);
	if ( value == 3)
		pPar->bPCP_EgressRemarkingEnable = IFX_ENABLE;
	else
		pPar->bPCP_EgressRemarkingEnable = IFX_DISABLE;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_PortRemarkingCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_portRemarkingCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint32_t  value, value_CLPEN=0, value_DPEN=0, value_DSCPMOD=0;

	if( pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber)
		return IFX_ERROR;

	switch (pPar->eDSCP_IngressRemarkingEnable) {
		case IFX_ETHSW_DSCP_REMARK_DISABLE:
			value_CLPEN = 0;
			value_DPEN = 0;
			value_DSCPMOD = 0;
			break;
		case IFX_ETHSW_DSCP_REMARK_TC6:
         	value_CLPEN = 1;
         	value_DPEN = 0;
         	value_DSCPMOD = 1;
      		break;
		case IFX_ETHSW_DSCP_REMARK_TC3:
        	 value_CLPEN = 1;
        	 value_DPEN = 1;
        	 value_DSCPMOD = 1;
     	 	break;
		case IFX_ETHSW_DSCP_REMARK_DP3:
			value_CLPEN = 0;
			value_DPEN = 1;
			value_DSCPMOD = 1;
			break;
		case IFX_ETHSW_DSCP_REMARK_DP3_TC3:
			value_CLPEN = 1;
			value_DPEN = 1;
			value_DSCPMOD = 1;
			break;
	}

	if (pPar->eDSCP_IngressRemarkingEnable == IFX_FALSE ) {
		value_CLPEN = 0;
		value_DPEN = 0;
		value_DSCPMOD = 0;
	}

	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PCTRL_0_CLPEN_OFFSET + (10 * pPar->nPortId),	\
		VR9_PCE_PCTRL_0_CLPEN_SHIFT, VR9_PCE_PCTRL_0_CLPEN_SIZE, value_CLPEN);
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PCTRL_0_DPEN_OFFSET + (10 * pPar->nPortId),	\
		VR9_PCE_PCTRL_0_DPEN_SHIFT, VR9_PCE_PCTRL_0_DPEN_SIZE, value_DPEN);
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PCTRL_2_DSCPMOD_OFFSET + (10 * pPar->nPortId),	\
		VR9_PCE_PCTRL_2_DSCPMOD_SHIFT, VR9_PCE_PCTRL_2_DSCPMOD_SIZE, value_DSCPMOD);
	if (pPar->bDSCP_EgressRemarkingEnable > 0 )
		value = pPar->bDSCP_EgressRemarkingEnable;
	else
		value = 0;
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_FDMA_PCTRL_DSCPRM_OFFSET + (6 * pPar->nPortId),	\
		VR9_FDMA_PCTRL_DSCPRM_SHIFT, VR9_FDMA_PCTRL_DSCPRM_SIZE, value);

	if (pPar->bPCP_IngressRemarkingEnable > 0 )
		value = pPar->bPCP_IngressRemarkingEnable;
	else
		value = 0;
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PCTRL_0_PCPEN_OFFSET + (10 * pPar->nPortId),	\
		VR9_PCE_PCTRL_0_PCPEN_SHIFT, VR9_PCE_PCTRL_0_PCPEN_SIZE, value);

	if (pPar->bPCP_EgressRemarkingEnable > 0)
		value = 3;
	else
		value = 0;

	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_FDMA_PCTRL_VLANMOD_OFFSET + (6 * pPar->nPortId),	\
		VR9_FDMA_PCTRL_VLANMOD_SHIFT, VR9_FDMA_PCTRL_VLANMOD_SIZE, value);
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_ClassPCP_Get(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_ClassPCP_Cfg_t *pPar)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pcetable;
	IFX_uint32_t  value;

	for ( value =0; value < 16; value++) {
		pcetable.table = IFX_ETHSW_PCE_REMARKING_INDEX;
		pcetable.table_index = value; //index of the DSCP configuration
		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pcetable);
		pPar->nPCP[value] = (pcetable.val[0] >> 8 ) & 0x7;
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_ClassPCP_Set(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_ClassPCP_Cfg_t *pPar)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pcetable;
	IFX_uint32_t  nDSCP, nPCP, value;

	for( value =0; value < 16; value++) {
		pcetable.table = IFX_ETHSW_PCE_REMARKING_INDEX;
		pcetable.table_index = value; //index of the DSCP configuration
 		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pcetable);
		nDSCP = pcetable.val[0] & 0x3F;
		nPCP = pPar->nPCP[value] & 0x7;
		pcetable.val[0] = (nPCP << 8) | nDSCP;
		ifx_ethsw_xwayflow_pce_table_write(pDevCtx, &pcetable);
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_PCP_ClassGet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_PCP_ClassCfg_t *pPar)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pcetable;
	IFX_uint32_t  value;

	for ( value =0; value < 8; value++) {
		pcetable.table = IFX_ETHSW_PCE_PCP_INDEX;
		pcetable.table_index = value; //index of the PCP configuration
		//IFXOS_PRINT_INT_RAW("table_index = %d\n",value);
		ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pcetable);
		pPar->nTrafficClass[value] = pcetable.val[0] & 0xF;
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_PCP_ClassSet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_PCP_ClassCfg_t *pPar)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pcetable;
	IFX_uint32_t  value;

	for ( value = 0; value < 8; value++) {
		memset(&pcetable, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
		pcetable.table = IFX_ETHSW_PCE_PCP_INDEX;
		pcetable.table_index = value; //index of the PCP configuration
		pcetable.val[0] = pPar->nTrafficClass[value] & 0xF;
		ifx_ethsw_xwayflow_pce_table_write(pDevCtx, &pcetable);
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_PortCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_portCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint32_t  value, value_DSCP, value_PCP;

	if (pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber)
		return IFX_ERROR;

	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_PCTRL_2_DSCP_OFFSET + (10 * pPar->nPortId),	\
		VR9_PCE_PCTRL_2_DSCP_SHIFT, VR9_PCE_PCTRL_2_DSCP_SIZE, &value_DSCP);
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_PCTRL_2_PCP_OFFSET + (10 * pPar->nPortId),	\
		VR9_PCE_PCTRL_2_PCP_SHIFT, VR9_PCE_PCTRL_2_PCP_SIZE, &value_PCP);

	if ((value_DSCP == 1) && (value_PCP == 1))
		pPar->eClassMode = 4;
	else if ((value_DSCP == 2) && (value_PCP == 1))
		pPar->eClassMode = 3;
	else if ((value_DSCP == 0) && (value_PCP == 1))
		pPar->eClassMode = 2;
	else if ((value_DSCP == 2) && (value_PCP == 0))
		pPar->eClassMode = 1;
	else if ((value_DSCP == 0) && (value_PCP == 0))
		pPar->eClassMode = 0;
	else
		pPar->eClassMode = 0;

	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_PCTRL_2_PCLASS_OFFSET + (10 * pPar->nPortId),	\
		VR9_PCE_PCTRL_2_PCLASS_SHIFT, VR9_PCE_PCTRL_2_PCLASS_SIZE, &value);
	pPar->nTrafficClass = value;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_PortCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_portCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;

	if (pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber)
		return IFX_ERROR;

	switch (pPar->eClassMode) {
		case IFX_ETHSW_QOS_CLASS_SELECT_NO:
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PCTRL_2_DSCP_OFFSET + (10 * pPar->nPortId),	\
				VR9_PCE_PCTRL_2_DSCP_SHIFT, VR9_PCE_PCTRL_2_DSCP_SIZE, 0);
         	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PCTRL_2_PCP_OFFSET + (10 * pPar->nPortId),	\
         		VR9_PCE_PCTRL_2_PCP_SHIFT, VR9_PCE_PCTRL_2_PCP_SIZE, 0);
		break;
 		case IFX_ETHSW_QOS_CLASS_SELECT_DSCP:
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PCTRL_2_DSCP_OFFSET + (10 * pPar->nPortId),	\
				VR9_PCE_PCTRL_2_DSCP_SHIFT, VR9_PCE_PCTRL_2_DSCP_SIZE, 2);
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PCTRL_2_PCP_OFFSET + (10 * pPar->nPortId),	\
				VR9_PCE_PCTRL_2_PCP_SHIFT, VR9_PCE_PCTRL_2_PCP_SIZE, 0);
		break;
		case IFX_ETHSW_QOS_CLASS_SELECT_PCP:
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PCTRL_2_DSCP_OFFSET + (10 * pPar->nPortId),	\
				VR9_PCE_PCTRL_2_DSCP_SHIFT, VR9_PCE_PCTRL_2_DSCP_SIZE, 0);
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PCTRL_2_PCP_OFFSET + (10 * pPar->nPortId),	\
				VR9_PCE_PCTRL_2_PCP_SHIFT, VR9_PCE_PCTRL_2_PCP_SIZE, 1);
		break;
		case IFX_ETHSW_QOS_CLASS_SELECT_DSCP_PCP:
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PCTRL_2_DSCP_OFFSET + (10 * pPar->nPortId),	\
				VR9_PCE_PCTRL_2_DSCP_SHIFT, VR9_PCE_PCTRL_2_DSCP_SIZE, 2);
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PCTRL_2_PCP_OFFSET + (10 * pPar->nPortId),	\
				VR9_PCE_PCTRL_2_PCP_SHIFT, VR9_PCE_PCTRL_2_PCP_SIZE, 1);
		break;
		case IFX_ETHSW_QOS_CLASS_SELECT_PCP_DSCP:
 			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PCTRL_2_DSCP_OFFSET + (10 * pPar->nPortId),	\
 				VR9_PCE_PCTRL_2_DSCP_SHIFT, VR9_PCE_PCTRL_2_DSCP_SIZE, 1);
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PCTRL_2_PCP_OFFSET + (10 * pPar->nPortId),	\
				VR9_PCE_PCTRL_2_PCP_SHIFT, VR9_PCE_PCTRL_2_PCP_SIZE, 1);
		break;
	}

	if (pPar->nTrafficClass > 0xF)
		return IFX_ERROR;
	else
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PCTRL_2_PCLASS_OFFSET + (10 * pPar->nPortId),	\
			VR9_PCE_PCTRL_2_PCLASS_SHIFT, VR9_PCE_PCTRL_2_PCLASS_SIZE, pPar->nTrafficClass);

	return IFX_SUCCESS;
}
#if 0
static IFX_return_t IFX_FLOW_QoS_Buffer_QueuePortGet(IFX_void_t *pDevCtx,IFX_uint8_t EgressPortId , IFX_uint8_t QueueId)
{
	IFX_uint32_t   value;
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_VAL_0_VAL0_OFFSET,	\
		VR9_BM_RAM_VAL_0_VAL0_SHIFT, VR9_BM_RAM_VAL_0_VAL0_SIZE, EgressPortId);
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_ADDR_ADDR_OFFSET,	\
		VR9_BM_RAM_ADDR_ADDR_SHIFT, VR9_BM_RAM_ADDR_ADDR_SIZE, QueueId);
	/* Specify the Table Address */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_ADDR_OFFSET,	\
		VR9_BM_RAM_CTRL_ADDR_SHIFT, VR9_BM_RAM_CTRL_ADDR_SIZE, 0xE);
	/* Assign Read operation */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_OPMOD_OFFSET,	\
		VR9_BM_RAM_CTRL_OPMOD_SHIFT, VR9_BM_RAM_CTRL_OPMOD_SIZE, 0);
	value = 1;
	/* Active */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_BAS_OFFSET,	\
		 VR9_BM_RAM_CTRL_BAS_SHIFT, VR9_BM_RAM_CTRL_BAS_SIZE, value);
	do {
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_RAM_CTRL_BAS_OFFSET,	\
			VR9_BM_RAM_CTRL_BAS_SHIFT, VR9_BM_RAM_CTRL_BAS_SIZE, &value);
	} while(value);
	
	return IFX_SUCCESS;
}
#endif
IFX_return_t IFX_FLOW_QoS_QueuePortGet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_queuePort_t *pPar)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pcetable;

	memset(&pcetable, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
	/* Set the PCE Queue Mapping Table */
	pcetable.table = IFX_ETHSW_PCE_QUEUE_MAP_INDEX;
	pcetable.table_index = (((pPar->nPortId << 4) & 0x70) | (pPar->nTrafficClassId & 0xF));
	ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pcetable);
	pPar->nQueueId = (pcetable.val[0] & 0x1F);
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_QueuePortSet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_queuePort_t *pPar)
{
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pcetable;
	IFX_uint32_t  EgressPortId = 0, value;

	memset(&pcetable, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
	/* Set the PCE Queue Mapping Table */
	pcetable.table = IFX_ETHSW_PCE_QUEUE_MAP_INDEX;
	pcetable.table_index = ((pPar->nPortId << 4) & 0x70 ) | (pPar->nTrafficClassId & 0xF);
	pcetable.val[0] = (pPar->nQueueId & 0x1F);
	ifx_ethsw_xwayflow_pce_table_write(pDevCtx, &pcetable);
	/* Assign the Egress Port Id and Enable the Queue */
	EgressPortId = (pPar->nPortId & 0x7);
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_VAL_0_VAL0_OFFSET,	\
		VR9_BM_RAM_VAL_0_VAL0_SHIFT, VR9_BM_RAM_VAL_0_VAL0_SIZE, EgressPortId);
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_ADDR_ADDR_OFFSET,	\
		VR9_BM_RAM_ADDR_ADDR_SHIFT, VR9_BM_RAM_ADDR_ADDR_SIZE, pPar->nQueueId);
	/* Specify the Table Address */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_ADDR_OFFSET,	\
		VR9_BM_RAM_CTRL_ADDR_SHIFT, VR9_BM_RAM_CTRL_ADDR_SIZE, 0xE);
	/* Assign Write operation */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_OPMOD_OFFSET,	\
		VR9_BM_RAM_CTRL_OPMOD_SHIFT, VR9_BM_RAM_CTRL_OPMOD_SIZE, 1);
	value = 1;
	/* Active */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_BAS_OFFSET,	\
		 VR9_BM_RAM_CTRL_BAS_SHIFT, VR9_BM_RAM_CTRL_BAS_SIZE, value);
	do {
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_RAM_CTRL_BAS_OFFSET,	\
			VR9_BM_RAM_CTRL_BAS_SHIFT, VR9_BM_RAM_CTRL_BAS_SIZE, &value);
	} while(value);

	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_SchedulerCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_schedulerCfg_t *pPar)
{
	IFX_int32_t nQueueId = pPar->nQueueId;
	IFX_uint32_t  value;

	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_1_QUEUE_OFFSET,	\
		VR9_ETHSW_CAP_1_QUEUE_SHIFT, VR9_ETHSW_CAP_1_QUEUE_SIZE, &value);
	if ( nQueueId >= value )
		return IFX_ERROR;
	/* Set the BM RAM ADDR  */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_ADDR_ADDR_OFFSET,	\
		VR9_BM_RAM_ADDR_ADDR_SHIFT, VR9_BM_RAM_ADDR_ADDR_SIZE, nQueueId);
	/* Specify the WFQ = 0x8 */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_ADDR_OFFSET,	\
		VR9_BM_RAM_CTRL_ADDR_SHIFT, VR9_BM_RAM_CTRL_ADDR_SIZE, 0x8);
	/* Table Access Operation Mode Read */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_OPMOD_OFFSET,	\
			VR9_BM_RAM_CTRL_OPMOD_SHIFT, VR9_BM_RAM_CTRL_OPMOD_SIZE, 0x0);
	value = 1;
	/* Active */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_BAS_OFFSET,	\
		VR9_BM_RAM_CTRL_BAS_SHIFT, VR9_BM_RAM_CTRL_BAS_SIZE, value);
	do {
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_RAM_CTRL_BAS_OFFSET,	\
			VR9_BM_RAM_CTRL_BAS_SHIFT, VR9_BM_RAM_CTRL_BAS_SIZE, &value);
	} while(value);
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_RAM_VAL_0_VAL0_OFFSET,	\
		VR9_BM_RAM_VAL_0_VAL0_SHIFT, VR9_BM_RAM_VAL_0_VAL0_SIZE, &value);
	pPar->nWeight = value;
	if( value == 0xFFFF || value == 0x1800)
 		pPar->eType = IFX_ETHSW_QOS_SCHEDULER_STRICT;
	else
		pPar->eType = IFX_ETHSW_QOS_SCHEDULER_WFQ;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_SchedulerCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_schedulerCfg_t *pPar)
{
	IFX_int32_t nQueueId = pPar->nQueueId;
	IFX_uint32_t  value;

	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_1_QUEUE_OFFSET,	\
		VR9_ETHSW_CAP_1_QUEUE_SHIFT, VR9_ETHSW_CAP_1_QUEUE_SIZE, &value);

	if ( nQueueId >= value )
		return IFX_ERROR;
	/* Set the BM RAM ADDR  */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_ADDR_ADDR_OFFSET,	\
		VR9_BM_RAM_ADDR_ADDR_SHIFT, VR9_BM_RAM_ADDR_ADDR_SIZE, nQueueId);
	/* Specify the WFQ = 0x8 */
 	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_ADDR_OFFSET,	\
 		VR9_BM_RAM_CTRL_ADDR_SHIFT, VR9_BM_RAM_CTRL_ADDR_SIZE, 0x8);
	if( pPar->eType == IFX_ETHSW_QOS_SCHEDULER_STRICT )
		value = 0xFFFF;
	else
		value = pPar->nWeight;
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_VAL_0_VAL0_OFFSET,	\
		VR9_BM_RAM_VAL_0_VAL0_SHIFT, VR9_BM_RAM_VAL_0_VAL0_SIZE, value);
	/* Table Access Operation Mode Write */
 	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_OPMOD_OFFSET,	\
 		VR9_BM_RAM_CTRL_OPMOD_SHIFT, VR9_BM_RAM_CTRL_OPMOD_SIZE, 0x1);
	value = 1;
	/* Active */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_BAS_OFFSET,	\
		VR9_BM_RAM_CTRL_BAS_SHIFT, VR9_BM_RAM_CTRL_BAS_SIZE, value);
	do {
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_RAM_CTRL_BAS_OFFSET,	\
			VR9_BM_RAM_CTRL_BAS_SHIFT, VR9_BM_RAM_CTRL_BAS_SIZE, &value);
	} while(value);

	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_ShaperCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_ShaperCfg_t *pPar)
{
	IFX_int32_t nRateShaperId = pPar->nRateShaperId;
	IFX_uint32_t  value, exp, mant, ibs;
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_3_SHAPERS_OFFSET,	\
		VR9_ETHSW_CAP_3_SHAPERS_SHIFT, VR9_ETHSW_CAP_3_SHAPERS_SIZE, &value);
	if ( nRateShaperId >= value)
		return IFX_ERROR;
	/* Enable/Disable the rate shaper  */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_RS_CTRL_RSEN_OFFSET + (nRateShaperId * 0x5),	\
		VR9_RS_CTRL_RSEN_SHIFT, VR9_RS_CTRL_RSEN_SIZE, &value);
	pPar->bEnable = value;
	/* Committed Burst Size (CBS [bytes]) */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_RS_CBS_CBS_OFFSET + (nRateShaperId * 0x5),	\
		VR9_RS_CBS_CBS_SHIFT, VR9_RS_CBS_CBS_SIZE, &value);
	pPar->nCbs = (value * 64);
	/** Rate [Mbit/s] */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_RS_CIR_EXP_EXP_OFFSET + (nRateShaperId * 0x5),	\
		VR9_RS_CIR_EXP_EXP_SHIFT, VR9_RS_CIR_EXP_EXP_SIZE, &exp);
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_RS_CIR_MANT_MANT_OFFSET + (nRateShaperId * 0x5),	\
		VR9_RS_CIR_MANT_MANT_SHIFT, VR9_RS_CIR_MANT_MANT_SIZE, &mant);
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_RS_IBS_IBS_OFFSET + (nRateShaperId * 0x5),	\
		VR9_RS_IBS_IBS_SHIFT, VR9_RS_IBS_IBS_SIZE, &ibs);
	/* calc the Rate */
	pPar->nRate = RateCalc(ibs, exp, mant);
	if ( pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2) {
	/* Supported for GSWIP 2.2 and newer and returns with an error for older hardware revisions. */
	/** 802.1Qav credit based shaper mode. This specific shaper algorithm mode is used by 
	the audio/video bridging (AVB) network (according to 802.1Qav). By default, an token
	     based shaper algorithm is used. */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, GSWIP_RS_CTRL_RSMOD_OFFSET + (nRateShaperId * 0x5),	\
			GSWIP_RS_CTRL_RSMOD_SHIFT, GSWIP_RS_CTRL_RSMOD_SIZE, &pPar->bAVB);
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_ShaperCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_ShaperCfg_t *pPar)
{
	IFX_int32_t nRateShaperId = pPar->nRateShaperId;
	IFX_uint32_t  value, exp = 0, mant = 0, rate = 0, ibs = 0;
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_3_SHAPERS_OFFSET,	\
		VR9_ETHSW_CAP_3_SHAPERS_SHIFT, VR9_ETHSW_CAP_3_SHAPERS_SIZE, &value);
	if ( nRateShaperId >= value)
		return IFX_ERROR;

	/* Committed Burst Size */
	if (pPar->nCbs > 0xFFC0)
		value = 0x3FF;
	else
		value = ((pPar->nCbs + 63) / 64);
	/* Committed Burst Size (CBS [bytes]) */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_RS_CBS_CBS_OFFSET + (nRateShaperId * 0x5),	\
		VR9_RS_CBS_CBS_SHIFT, VR9_RS_CBS_CBS_SIZE, value);
	/* Rate [kbit/s] */
	/* Calc the Rate */
	rate = pPar->nRate;
	if (rate)
		calcToken(rate, &ibs, &exp, &mant);
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_RS_CIR_EXP_EXP_OFFSET + (nRateShaperId * 0x5),	\
		 VR9_RS_CIR_EXP_EXP_SHIFT, VR9_RS_CIR_EXP_EXP_SIZE, exp);
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_RS_CIR_MANT_MANT_OFFSET + (nRateShaperId * 0x5),	\
		VR9_RS_CIR_MANT_MANT_SHIFT, VR9_RS_CIR_MANT_MANT_SIZE, mant);
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_RS_IBS_IBS_OFFSET + (nRateShaperId * 0x5),	\
		VR9_RS_IBS_IBS_SHIFT, VR9_RS_IBS_IBS_SIZE, ibs);
	/* Enable/Disable the rate shaper  */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_RS_CTRL_RSEN_OFFSET + (nRateShaperId * 0x5),	\
		VR9_RS_CTRL_RSEN_SHIFT, VR9_RS_CTRL_RSEN_SIZE, pPar->bEnable);
	if ( pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2) {
	/* Supported for GSWIP 2.2 and newer and returns with an error for older hardware revisions. */
	/** 802.1Qav credit based shaper mode. This specific shaper algorithm mode is used by 
	the audio/video bridging (AVB) network (according to 802.1Qav). By default, an token
	     based shaper algorithm is used. */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, GSWIP_RS_CTRL_RSMOD_OFFSET + (nRateShaperId * 0x5),	\
			GSWIP_RS_CTRL_RSMOD_SHIFT, GSWIP_RS_CTRL_RSMOD_SIZE, pPar->bAVB);
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_ShaperQueueAssign(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_ShaperQueue_t *pPar)
{
	IFX_uint8_t nRateShaperId = pPar->nRateShaperId;
	IFX_uint8_t nQueueId = pPar->nQueueId;
	IFX_uint32_t value1_RS, value1_ShaperId, value2_RS, value2_ShaperId;

	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_1_QUEUE_OFFSET,	\
		VR9_ETHSW_CAP_1_QUEUE_SHIFT, VR9_ETHSW_CAP_1_QUEUE_SIZE, &value1_RS);

	if ( nQueueId >= value1_RS )
		return IFX_ERROR;

	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_3_SHAPERS_OFFSET,	\
		VR9_ETHSW_CAP_3_SHAPERS_SHIFT, VR9_ETHSW_CAP_3_SHAPERS_SIZE, &value1_RS);
	if ( nRateShaperId >= value1_RS)
		return IFX_ERROR;

	/* Check Rate Shaper 1 Enable  */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PQM_RS_EN1_OFFSET + (nQueueId * 2),	\
		VR9_PQM_RS_EN1_SHIFT, VR9_PQM_RS_EN1_SIZE, &value1_RS);
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PQM_RS_RS1_OFFSET + (nRateShaperId * 2),	\
		VR9_PQM_RS_RS1_SHIFT, VR9_PQM_RS_RS1_SIZE, &value1_ShaperId);
	/* Check Rate Shaper 2 Enable  */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PQM_RS_EN2_OFFSET + (nQueueId * 2),	\
		VR9_PQM_RS_EN2_SHIFT, VR9_PQM_RS_EN2_SIZE, &value2_RS);
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PQM_RS_RS2_OFFSET + (nRateShaperId * 2),	\
		VR9_PQM_RS_RS2_SHIFT, VR9_PQM_RS_RS2_SIZE, &value2_ShaperId);
	if (( value1_RS == 1) && ( value1_ShaperId == nRateShaperId))
		return IFX_SUCCESS;
	else if (( value2_RS == 1) && ( value2_ShaperId == nRateShaperId))
		return IFX_SUCCESS;
	else if ( value1_RS == 0) {
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PQM_RS_RS1_OFFSET + (nQueueId * 2),	\
			VR9_PQM_RS_RS1_SHIFT, VR9_PQM_RS_RS1_SIZE, nRateShaperId);
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PQM_RS_EN1_OFFSET + (nQueueId * 2),	\
			VR9_PQM_RS_EN1_SHIFT, VR9_PQM_RS_EN1_SIZE, 0x1);
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_RS_CTRL_RSEN_OFFSET + (nRateShaperId * 0x5),	\
			VR9_RS_CTRL_RSEN_SHIFT, VR9_RS_CTRL_RSEN_SIZE, 0x1);
	} else if ( value2_RS == 0) {
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PQM_RS_RS2_OFFSET + (nQueueId * 2),	\
			VR9_PQM_RS_RS2_SHIFT, VR9_PQM_RS_RS2_SIZE, nRateShaperId);
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PQM_RS_EN2_OFFSET + (nQueueId * 2),	\
			VR9_PQM_RS_EN2_SHIFT, VR9_PQM_RS_EN2_SIZE, 0x1);
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_RS_CTRL_RSEN_OFFSET + (nRateShaperId * 0x5),	\
			VR9_RS_CTRL_RSEN_SHIFT, VR9_RS_CTRL_RSEN_SIZE, 0x1);
	} else {
		return IFX_ERROR;
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_ShaperQueueDeassign(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_ShaperQueue_t *pPar)
{
	IFX_uint8_t nRateShaperId = pPar->nRateShaperId;
	IFX_uint8_t nQueueId = pPar->nQueueId;
	IFX_uint32_t value1, value2;
	IFX_uint32_t value3, value4;

	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_1_QUEUE_OFFSET,	\
		VR9_ETHSW_CAP_1_QUEUE_SHIFT, VR9_ETHSW_CAP_1_QUEUE_SIZE, &value1);
	if ( nQueueId >= value1 )
		return IFX_ERROR;
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_3_SHAPERS_OFFSET,	\
		VR9_ETHSW_CAP_3_SHAPERS_SHIFT, VR9_ETHSW_CAP_3_SHAPERS_SIZE, &value1);
	if ( nRateShaperId >= value1)
		return IFX_ERROR;
	/* Rate Shaper 1 Read  */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PQM_RS_EN1_OFFSET + (nQueueId * 2),	\
		VR9_PQM_RS_EN1_SHIFT, VR9_PQM_RS_EN1_SIZE, &value1);
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PQM_RS_RS1_OFFSET + (nQueueId * 2),	\
		VR9_PQM_RS_RS1_SHIFT, VR9_PQM_RS_RS1_SIZE, &value2);
	/* Rate Shaper 2 Read  */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PQM_RS_EN2_OFFSET + (nQueueId * 2),	\
		VR9_PQM_RS_EN2_SHIFT, VR9_PQM_RS_EN2_SIZE, &value3);
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PQM_RS_RS2_OFFSET + (nQueueId * 2),	\
		VR9_PQM_RS_RS2_SHIFT, VR9_PQM_RS_RS2_SIZE, &value4);
	if ((value1 == 1) && (value2 == nRateShaperId)) {
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PQM_RS_EN1_OFFSET + (nQueueId * 2),	\
			VR9_PQM_RS_EN1_SHIFT, VR9_PQM_RS_EN1_SIZE, 0);
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PQM_RS_RS1_OFFSET + (nQueueId * 2),	\
			VR9_PQM_RS_RS1_SHIFT, VR9_PQM_RS_RS1_SIZE, 0);
		return IFX_SUCCESS;
	} else if ((value3 == 1) && (value4 == nRateShaperId)) {
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PQM_RS_EN2_OFFSET + (nQueueId * 2),	\
			VR9_PQM_RS_EN2_SHIFT, VR9_PQM_RS_EN2_SIZE, 0);
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PQM_RS_RS2_OFFSET + (nQueueId * 2),	\
			VR9_PQM_RS_RS2_SHIFT, VR9_PQM_RS_RS2_SIZE, 0);
		return IFX_SUCCESS;
	} else {
		return IFX_ERROR;
	}
	if ((value1 == 0) && (value3 == 0)) {
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_RS_CTRL_RSEN_OFFSET + (nRateShaperId * 0x5),	\
			VR9_RS_CTRL_RSEN_SHIFT, VR9_RS_CTRL_RSEN_SIZE, 0);
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_ShaperQueueGet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_ShaperQueueGet_t *pPar)
{
	IFX_uint32_t value1_RS,value2_RS, value;
	IFX_uint8_t nQueueId = pPar->nQueueId;

	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_1_QUEUE_OFFSET,	\
		VR9_ETHSW_CAP_1_QUEUE_SHIFT, VR9_ETHSW_CAP_1_QUEUE_SIZE, &value1_RS);
	if ( nQueueId >= value1_RS )
		return IFX_ERROR;
	pPar->bAssigned = IFX_FALSE;
	pPar->nRateShaperId = 0;
	/* Check Rate Shaper 1 Enable  */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PQM_RS_EN1_OFFSET + (nQueueId * 2),	\
		VR9_PQM_RS_EN1_SHIFT, VR9_PQM_RS_EN1_SIZE, &value1_RS);
	if (value1_RS == 1 ) {
		pPar->bAssigned = IFX_TRUE;
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PQM_RS_RS1_OFFSET + (nQueueId * 2),	\
		VR9_PQM_RS_RS1_SHIFT, VR9_PQM_RS_RS1_SIZE, &value);
		pPar->nRateShaperId = value;
	}
	/* Check Rate Shaper 2 Enable  */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PQM_RS_EN2_OFFSET + (nQueueId * 2),	\
		VR9_PQM_RS_EN2_SHIFT, VR9_PQM_RS_EN2_SIZE, &value2_RS);
	if (value1_RS == 2 ) {
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PQM_RS_RS2_OFFSET + (nQueueId * 2),	\
		VR9_PQM_RS_RS2_SHIFT, VR9_PQM_RS_RS2_SIZE, &value);
		pPar->nRateShaperId = value;
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_StormCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_stormCfg_t *pPar)
{
	IFX_uint32_t SCONBC, SCONMC, SCONUC, MeterID;

	if ((pPar->bBroadcast == IFX_FALSE) && (pPar->bMulticast == IFX_FALSE)	\
			&& (pPar->bUnknownUnicast == IFX_FALSE)) {
		/*  Storm Control Mode  */
      	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_GCTRL_0_SCONMOD_OFFSET,	\
      		VR9_PCE_GCTRL_0_SCONMOD_SHIFT, VR9_PCE_GCTRL_0_SCONMOD_SIZE, 0);
		/* Meter instances used for broadcast traffic  */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_GCTRL_0_SCONBC_OFFSET,	\
			VR9_PCE_GCTRL_0_SCONBC_SHIFT, VR9_PCE_GCTRL_0_SCONBC_SIZE, 0);
		/* Meter instances used for multicast traffic  */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_GCTRL_0_SCONMC_OFFSET,	\
			VR9_PCE_GCTRL_0_SCONMC_SHIFT, VR9_PCE_GCTRL_0_SCONMC_SIZE, 0);
		/* Meter instances used for unknown unicast traffic  */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_GCTRL_0_SCONUC_OFFSET,	\
			VR9_PCE_GCTRL_0_SCONUC_SHIFT, VR9_PCE_GCTRL_0_SCONUC_SIZE, 0);
	}
	/*  Meter ID */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_GCTRL_0_SCONMET_OFFSET,	\
		VR9_PCE_GCTRL_0_SCONMET_SHIFT, VR9_PCE_GCTRL_0_SCONMET_SIZE, &MeterID);
	/* Meter instances used for broadcast traffic  */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_GCTRL_0_SCONBC_OFFSET,	\
		VR9_PCE_GCTRL_0_SCONBC_SHIFT, VR9_PCE_GCTRL_0_SCONBC_SIZE, &SCONBC);
	/* Meter instances used for multicast traffic  */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_GCTRL_0_SCONMC_OFFSET,	\
		VR9_PCE_GCTRL_0_SCONMC_SHIFT, VR9_PCE_GCTRL_0_SCONMC_SIZE, &SCONMC);
	/* Meter instances used for unknown unicast traffic  */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_GCTRL_0_SCONUC_OFFSET,	\
		VR9_PCE_GCTRL_0_SCONUC_SHIFT, VR9_PCE_GCTRL_0_SCONUC_SIZE, &SCONUC);

	if ((SCONBC == 1) || (SCONMC == 1) ||(SCONMC == 1)) {
		if (pPar->nMeterId == (MeterID + 1)) {
			/*  Storm Control Mode  */
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_GCTRL_0_SCONMOD_OFFSET,	\
				VR9_PCE_GCTRL_0_SCONMOD_SHIFT, VR9_PCE_GCTRL_0_SCONMOD_SIZE, 3);
		} else if (pPar->nMeterId != MeterID)
			return IFX_ERROR;
	} else {
		/*  Storm Control Mode  */
      	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_GCTRL_0_SCONMOD_OFFSET,	\
      		VR9_PCE_GCTRL_0_SCONMOD_SHIFT, VR9_PCE_GCTRL_0_SCONMOD_SIZE, 1);
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_GCTRL_0_SCONMET_OFFSET,	\
			VR9_PCE_GCTRL_0_SCONMET_SHIFT, VR9_PCE_GCTRL_0_SCONMET_SIZE, pPar->nMeterId);
	}

	/* Meter instances used for broadcast traffic  */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_GCTRL_0_SCONBC_OFFSET,	\
		VR9_PCE_GCTRL_0_SCONBC_SHIFT, VR9_PCE_GCTRL_0_SCONBC_SIZE, pPar->bBroadcast);
	/* Meter instances used for multicast traffic  */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_GCTRL_0_SCONMC_OFFSET,	\
		VR9_PCE_GCTRL_0_SCONMC_SHIFT, VR9_PCE_GCTRL_0_SCONMC_SIZE, pPar->bMulticast);
	/* Meter instances used for unknown unicast traffic  */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_GCTRL_0_SCONUC_OFFSET,	\
		VR9_PCE_GCTRL_0_SCONUC_SHIFT, VR9_PCE_GCTRL_0_SCONUC_SIZE, pPar->bUnknownUnicast);
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_StormCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_stormCfg_t *pPar)
{
	IFX_uint32_t value;

	/*  Storm Control Mode  */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_GCTRL_0_SCONMOD_OFFSET,	\
		VR9_PCE_GCTRL_0_SCONMOD_SHIFT, VR9_PCE_GCTRL_0_SCONMOD_SIZE, &value);

	if (value == 0) {
		pPar->nMeterId = IFX_FALSE;
		pPar->bBroadcast = IFX_FALSE;
		pPar->bMulticast = IFX_FALSE;
		pPar->bUnknownUnicast =  IFX_FALSE;
	}  else {
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_GCTRL_0_SCONMET_OFFSET,	\
			VR9_PCE_GCTRL_0_SCONMET_SHIFT, VR9_PCE_GCTRL_0_SCONMET_SIZE, &value);
		pPar->nMeterId = value;
		/* Meter instances used for broadcast traffic  */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_GCTRL_0_SCONBC_OFFSET,	\
			VR9_PCE_GCTRL_0_SCONBC_SHIFT, VR9_PCE_GCTRL_0_SCONBC_SIZE, &value );
		pPar->bBroadcast = value;
		/* Meter instances used for multicast traffic  */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_GCTRL_0_SCONMC_OFFSET,	\
			VR9_PCE_GCTRL_0_SCONMC_SHIFT, VR9_PCE_GCTRL_0_SCONMC_SIZE, &value );
		pPar->bMulticast = value;
		/* Meter instances used for unknown unicast traffic  */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_GCTRL_0_SCONUC_OFFSET,	\
			VR9_PCE_GCTRL_0_SCONUC_SHIFT, VR9_PCE_GCTRL_0_SCONUC_SIZE, &value);
		pPar->bUnknownUnicast = value;
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_WredCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_WRED_Cfg_t *pPar)
{
	/* Description: 'Drop Probability Profile' */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_QUEUE_GCTRL_DPROB_OFFSET,	\
		VR9_BM_QUEUE_GCTRL_DPROB_SHIFT, VR9_BM_QUEUE_GCTRL_DPROB_SIZE, &pPar->eProfile);
	/* WRED Red Threshold - Minimum */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_WRED_RTH_0_MINTH_OFFSET,	\
		VR9_BM_WRED_RTH_0_MINTH_SHIFT, VR9_BM_WRED_RTH_0_MINTH_SIZE, &pPar->nRed_Min);
	/* WRED Red Threshold - Maximum */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_WRED_RTH_1_MAXTH_OFFSET,	\
		VR9_BM_WRED_RTH_1_MAXTH_SHIFT, VR9_BM_WRED_RTH_1_MAXTH_SIZE, &pPar->nRed_Max);
	/* WRED Yellow Threshold - Minimum */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_WRED_YTH_0_MINTH_OFFSET,	\
		VR9_BM_WRED_YTH_0_MINTH_SHIFT, VR9_BM_WRED_YTH_0_MINTH_SIZE, &pPar->nYellow_Min);
	/* WRED Yellow Threshold - Maximum */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_WRED_YTH_1_MAXTH_OFFSET,	\
		VR9_BM_WRED_YTH_1_MAXTH_SHIFT, VR9_BM_WRED_YTH_1_MAXTH_SIZE, &pPar->nYellow_Max);
	/* WRED Green Threshold - Minimum */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_WRED_GTH_0_MINTH_OFFSET,	\
		VR9_BM_WRED_GTH_0_MINTH_SHIFT, VR9_BM_WRED_GTH_0_MINTH_SIZE, &pPar->nGreen_Min);
	/* WRED Green Threshold - Maximum */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_WRED_GTH_1_MAXTH_OFFSET,	\
		VR9_BM_WRED_GTH_1_MAXTH_SHIFT, VR9_BM_WRED_GTH_1_MAXTH_SIZE, &pPar->nGreen_Max);
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_WredCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_WRED_Cfg_t *pPar)
{
	/* Set the global threshold */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_QUEUE_GCTRL_GL_MOD_OFFSET,	\
		VR9_BM_QUEUE_GCTRL_GL_MOD_SHIFT,VR9_BM_QUEUE_GCTRL_GL_MOD_SIZE, 0x1);
	/* Description: 'Drop Probability Profile' */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_QUEUE_GCTRL_DPROB_OFFSET,	\
		VR9_BM_QUEUE_GCTRL_DPROB_SHIFT, VR9_BM_QUEUE_GCTRL_DPROB_SIZE, pPar->eProfile);
	/* WRED Red Threshold - Minimum */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_WRED_RTH_0_MINTH_OFFSET,	\
		VR9_BM_WRED_RTH_0_MINTH_SHIFT, VR9_BM_WRED_RTH_0_MINTH_SIZE, pPar->nRed_Min);
	/* WRED Red Threshold - Maximum */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_WRED_RTH_1_MAXTH_OFFSET,	\
		VR9_BM_WRED_RTH_1_MAXTH_SHIFT, VR9_BM_WRED_RTH_1_MAXTH_SIZE, pPar->nRed_Max);
	/* WRED Yellow Threshold - Minimum */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_WRED_YTH_0_MINTH_OFFSET,	\
		VR9_BM_WRED_YTH_0_MINTH_SHIFT, VR9_BM_WRED_YTH_0_MINTH_SIZE, pPar->nYellow_Min);
	/* WRED Yellow Threshold - Maximum */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_WRED_YTH_1_MAXTH_OFFSET,	\
		VR9_BM_WRED_YTH_1_MAXTH_SHIFT, VR9_BM_WRED_YTH_1_MAXTH_SIZE, pPar->nYellow_Max);
	/* WRED Green Threshold - Minimum */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_WRED_GTH_0_MINTH_OFFSET,	\
		VR9_BM_WRED_GTH_0_MINTH_SHIFT, VR9_BM_WRED_GTH_0_MINTH_SIZE, pPar->nGreen_Min);
	/* WRED Green Threshold - Maximum */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_WRED_GTH_1_MAXTH_OFFSET,	\
		VR9_BM_WRED_GTH_1_MAXTH_SHIFT, VR9_BM_WRED_GTH_1_MAXTH_SIZE, pPar->nGreen_Max);
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_WredQueueCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_WRED_QueueCfg_t *pPar)
{
	IFX_uint32_t nQueueId = pPar->nQueueId;
	IFX_uint32_t  value, addr;
	IFX_uint32_t  data0, data1;
	IFX_uint8_t ColorCode;

	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_1_QUEUE_OFFSET,	\
		VR9_ETHSW_CAP_1_QUEUE_SHIFT, VR9_ETHSW_CAP_1_QUEUE_SIZE, &value);

	if ( nQueueId >= value )
		return IFX_ERROR;
	/* For different color 0(not drop) 1(Green) 2(Yellow) 3(Red) */
	for (ColorCode = 0; ColorCode < 4; ColorCode++) {
		/* Set the BM RAM ADDR  */
		addr = ((nQueueId << 3 ) | ColorCode);
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_ADDR_ADDR_OFFSET,	\
			VR9_BM_RAM_ADDR_ADDR_SHIFT, VR9_BM_RAM_ADDR_ADDR_SIZE, addr);
		/* Specify the PQMCTXT = 0x9 */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_ADDR_OFFSET,	\
			VR9_BM_RAM_CTRL_ADDR_SHIFT, VR9_BM_RAM_CTRL_ADDR_SIZE, 0x9);
		/* Table Access Operation Mode Read */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_OPMOD_OFFSET,	\
			VR9_BM_RAM_CTRL_OPMOD_SHIFT, VR9_BM_RAM_CTRL_OPMOD_SIZE, 0x0);
		value = 1;
		/* Active */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_BAS_OFFSET,	\
			VR9_BM_RAM_CTRL_BAS_SHIFT, VR9_BM_RAM_CTRL_BAS_SIZE, value);
		do {
			ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_RAM_CTRL_BAS_OFFSET,	\
				VR9_BM_RAM_CTRL_BAS_SHIFT, VR9_BM_RAM_CTRL_BAS_SIZE, &value);
		} while(value);
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_RAM_VAL_0_VAL0_OFFSET,	\
			VR9_BM_RAM_VAL_0_VAL0_SHIFT, VR9_BM_RAM_VAL_0_VAL0_SIZE, &data0);
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_RAM_VAL_1_VAL1_OFFSET,	\
			VR9_BM_RAM_VAL_1_VAL1_SHIFT, VR9_BM_RAM_VAL_1_VAL1_SIZE, &data1);
		switch (ColorCode) {
			case 3:
				pPar->nRed_Max = data1;
				pPar->nRed_Min = data0;
			break;
			case 2:
				pPar->nYellow_Max = data1;
				pPar->nYellow_Min = data0;
			break;
			case 1:
				pPar->nGreen_Max = data1;
				pPar->nGreen_Min = data0;
			break;
			case 0:
#if 0
				pPar->nRed_Max = 0;
				pPar->nRed_Min = 0;
				pPar->nYellow_Max = 0;
				pPar->nYellow_Min = 0;
				pPar->nGreen_Max = 0;
				pPar->nGreen_Min = 0;
#endif
			break;
		}
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_QoS_WredQueueCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_QoS_WRED_QueueCfg_t *pPar)
{
	IFX_uint32_t nQueueId = pPar->nQueueId;
	IFX_uint32_t  value, addr;
	IFX_uint32_t  data0 = 0, data1 = 0;
	IFX_uint8_t ColorCode;

	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_1_QUEUE_OFFSET,	\
		VR9_ETHSW_CAP_1_QUEUE_SHIFT, VR9_ETHSW_CAP_1_QUEUE_SIZE, &value);

	if ( nQueueId >= value )
		return IFX_ERROR;
	/* For different color 0(not drop) 1(Green) 2(Yellow) 3(Red) */
	for (ColorCode = 0; ColorCode < 4; ColorCode++) {
		/* Set the BM RAM ADDR  */
		addr = (((nQueueId << 3 ) & 0xF8) | ColorCode);
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_ADDR_ADDR_OFFSET,	\
			VR9_BM_RAM_ADDR_ADDR_SHIFT, VR9_BM_RAM_ADDR_ADDR_SIZE, addr);
		/* Specify the PQMCTXT = 0x9 */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_ADDR_OFFSET,	\
			VR9_BM_RAM_CTRL_ADDR_SHIFT, VR9_BM_RAM_CTRL_ADDR_SIZE, 0x9);
		/* Table Access Operation Mode Write */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_OPMOD_OFFSET,	\
			VR9_BM_RAM_CTRL_OPMOD_SHIFT, VR9_BM_RAM_CTRL_OPMOD_SIZE, 0x1);

		switch (ColorCode) {
			case 3:
				data1 = pPar->nRed_Max;
				data0 = pPar->nRed_Min;
				break;
			case 2:
				data1 = pPar->nYellow_Max;
				data0 = pPar->nYellow_Min;
				break;
			case 1:
				data1 = pPar->nGreen_Max;
				data0 = pPar->nGreen_Min;
				break;
			case 0:
				data0 = 0;
				data1 = 0;
				break;
		}
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_VAL_0_VAL0_OFFSET,	\
			VR9_BM_RAM_VAL_0_VAL0_SHIFT, VR9_BM_RAM_VAL_0_VAL0_SIZE, data0);

		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_VAL_1_VAL1_OFFSET,	\
			VR9_BM_RAM_VAL_1_VAL1_SHIFT, VR9_BM_RAM_VAL_1_VAL1_SIZE, data1);
		value = 1;
		/* Active */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_BAS_OFFSET,	\
			VR9_BM_RAM_CTRL_BAS_SHIFT, VR9_BM_RAM_CTRL_BAS_SIZE, value);
		do {
			ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_RAM_CTRL_BAS_OFFSET,	\
				VR9_BM_RAM_CTRL_BAS_SHIFT, VR9_BM_RAM_CTRL_BAS_SIZE, &value);
		} while(value);
#if 0
		/* Specify the PQMCTXT = 0x9 */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_ADDR_OFFSET,	\
			VR9_BM_RAM_CTRL_ADDR_SHIFT, VR9_BM_RAM_CTRL_ADDR_SIZE, 0);
		/* Table Access Operation Mode Write */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_OPMOD_OFFSET,	\
			VR9_BM_RAM_CTRL_OPMOD_SHIFT, VR9_BM_RAM_CTRL_OPMOD_SIZE, 0);
		/* Active */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_BAS_OFFSET,	\
			VR9_BM_RAM_CTRL_BAS_SHIFT, VR9_BM_RAM_CTRL_BAS_SIZE, 0);
#endif
	}
	/* Set the local threshold */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_QUEUE_GCTRL_GL_MOD_OFFSET,	\
		VR9_BM_QUEUE_GCTRL_GL_MOD_SHIFT,VR9_BM_QUEUE_GCTRL_GL_MOD_SIZE, 0);
   return IFX_SUCCESS;
}
#endif  /* CONFIG_LTQ_QOS */
#if defined (CONFIG_LTQ_MULTICAST ) && CONFIG_LTQ_MULTICAST
IFX_return_t IFX_FLOW_MulticastRouterPortAdd(IFX_void_t *pDevCtx, IFX_ETHSW_multicastRouter_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_FLOW_IGMP_t      *pIGMPTbHandle   = &pEthSWDevHandle->IGMP_Flags;
	IFX_uint32_t value;
	IFX_uint8_t portIdx = pPar->nPortId;
	
	if (pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber)
		return IFX_ERROR;

	/* Read the Default Router Port Map - DRPM */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_IGMP_DRPM_DRPM_OFFSET,	\
		VR9_PCE_IGMP_DRPM_DRPM_SHIFT, VR9_PCE_IGMP_DRPM_DRPM_SIZE, &value);

	if(((value >> portIdx) & 0x1 ) == 1) {
		IFXOS_PRINT_INT_RAW("Error: the prot was already in the member\n");
	} else {
		value = (value | (1 << portIdx));
		/* Write the Default Router Port Map - DRPM  */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_IGMP_DRPM_DRPM_OFFSET,	\
			VR9_PCE_IGMP_DRPM_DRPM_SHIFT, VR9_PCE_IGMP_DRPM_DRPM_SIZE, value);
	}
	if (pIGMPTbHandle->eIGMP_Mode == IFX_ETHSW_MULTICAST_SNOOP_MODE_FORWARD) {
		IFX_FLOW_PCE_rule_t PCE_rule;
		int i;
		for (i = 0; i <2; i++) {
			memset(&PCE_rule, 0 , sizeof(IFX_FLOW_PCE_rule_t));
			PCE_rule.pattern.bEnable = IFX_TRUE;	
			PCE_rule.pattern.bProtocolEnable = IFX_TRUE;
			switch(i) {
				case 0:	
				/*	Management port remaining IGMP packets (forwarding them to Router Ports) */
					PCE_rule.pattern.nIndex = MULTI_PCE_RULES_INDEX;
					PCE_rule.pattern.nProtocol		= 0x2; /* for IPv4 */
					PCE_rule.pattern.bAppMaskRangeMSB_Select = IFX_TRUE;
					PCE_rule.pattern.bAppDataMSB_Enable	= IFX_TRUE;
					PCE_rule.pattern.nAppDataMSB		= 0x1200;
					PCE_rule.pattern.nAppMaskRangeMSB	= 0x1DFF;
					break;
				case 1:
					/* Management Port ICMPv6 Multicast Listerner Report & Leave (Avoiding Loopback abd Discard) */
					PCE_rule.pattern.nIndex = MULTI_PCE_RULES_INDEX+3;
					PCE_rule.pattern.bAppDataMSB_Enable	= IFX_TRUE;
					PCE_rule.pattern.bAppMaskRangeMSB_Select = IFX_TRUE;
					PCE_rule.pattern.nAppDataMSB		= 0x8300;
					PCE_rule.pattern.nAppMaskRangeMSB	= 0x1FF;
					PCE_rule.pattern.nProtocol			= 0x3A;  /*for IPv6*/
					PCE_rule.action.ePortMapAction = IFX_FLOW_PCE_ACTION_PORTMAP_ALTERNATIVE;
					PCE_rule.action.nForwardPortMap = value ;
					break;
			}
			/* Router portmap */
			PCE_rule.action.ePortMapAction	= IFX_FLOW_PCE_ACTION_PORTMAP_ALTERNATIVE;
			PCE_rule.action.nForwardPortMap = value ;
			if (pIGMPTbHandle->nClassOfService != 0 ) {
				PCE_rule.action.eTrafficClassAction = 1;
				PCE_rule.action.nTrafficClassAlternate = pEthSWDevHandle->IGMP_Flags.nClassOfService;
			}
			/*  Set eForwardPort */
			PCE_rule.pattern.bPortIdEnable	= IFX_TRUE;
			if ( pIGMPTbHandle->eForwardPort == IFX_ETHSW_PORT_FORWARD_PORT ) {
				PCE_rule.pattern.nPortId		= pIGMPTbHandle->nForwardPortId;
			} else if ( pIGMPTbHandle->eForwardPort == IFX_ETHSW_PORT_FORWARD_CPU ) {
				PCE_rule.pattern.nPortId		= (pEthSWDevHandle->nCPU_Port);
			} 
			if (pIGMPTbHandle->bCrossVLAN)
				PCE_rule.action.eVLAN_CrossAction = IFX_FLOW_PCE_ACTION_CROSS_VLAN_CROSS;
			else
				PCE_rule.action.eVLAN_CrossAction = IFX_FLOW_PCE_ACTION_CROSS_VLAN_DISABLE;
			/* We prepare everything and write into PCE Table */
			if (0 != ifx_pce_rule_write(&pEthSWDevHandle->PCE_Handler,&PCE_rule))
				return IFX_ERROR;
		}
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_MulticastRouterPortRead(IFX_void_t *pDevCtx, IFX_ETHSW_multicastRouterRead_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDev = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint32_t value_1, value_2;

	if (pPar->bInitial == IFX_TRUE) {
		/* Read the Default Router Port Map - DRPM*/
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_IGMP_DRPM_DRPM_OFFSET,	\
			VR9_PCE_IGMP_DRPM_DRPM_SHIFT, VR9_PCE_IGMP_DRPM_DRPM_SIZE, &value_1);
		/* Read the Default Router Port Map - IGPM */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_IGMP_STAT_IGPM_OFFSET,	\
			VR9_PCE_IGMP_STAT_IGPM_SHIFT, VR9_PCE_IGMP_STAT_IGPM_SIZE, &value_2);
		pEthSWDev->IGMP_Flags.eRouterPort = (value_1 | value_2);
		pPar->bInitial = IFX_FALSE;
		pEthSWDev->multi_routerport_counter = 0;
	}
	if (pPar->bLast == IFX_FALSE) {
		/* Need to clarify the different between DRPM & IGPM */
		while (((pEthSWDev->IGMP_Flags.eRouterPort >> pEthSWDev->multi_routerport_counter) & 0x1) == 0) {
			pEthSWDev->multi_routerport_counter++;
			if (pEthSWDev->multi_routerport_counter > (pEthSWDev->nTotalPortNumber-1)) {
				pPar->bLast = IFX_TRUE;
				return IFX_SUCCESS;
			}
		}
		pPar->nPortId = pEthSWDev->multi_routerport_counter;
		
		if (pEthSWDev->multi_routerport_counter < pEthSWDev->nTotalPortNumber )
			pEthSWDev->multi_routerport_counter++;
		else
			pPar->bLast = IFX_TRUE;
	}
    return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_MulticastRouterPortRemove(IFX_void_t *pDevCtx, IFX_ETHSW_multicastRouter_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_FLOW_IGMP_t      *pIGMPTbHandle   = &pEthSWDevHandle->IGMP_Flags;
	IFX_uint32_t value_1, value_2;
	IFX_uint8_t portIdx = pPar->nPortId;

	if (pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber)
		return IFX_ERROR;
	/* Read the Default Router Port Map - DRPM */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_IGMP_DRPM_DRPM_OFFSET,	\
		VR9_PCE_IGMP_DRPM_DRPM_SHIFT, VR9_PCE_IGMP_DRPM_DRPM_SIZE, &value_1);
	/* Read the Default Router Port Map - IGPM */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_IGMP_STAT_IGPM_OFFSET,	\
		VR9_PCE_IGMP_STAT_IGPM_SHIFT, VR9_PCE_IGMP_STAT_IGPM_SIZE, &value_2);
	if(((value_1 >> portIdx) & 0x1 ) == 0) {
		IFXOS_PRINT_INT_RAW("Error: the port was not in the member\n");
		return IFX_SUCCESS;
	} else {
	value_1 = (value_1 & ~(1 << portIdx));
	/* Write the Default Router Port Map - DRPM*/
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_IGMP_DRPM_DRPM_OFFSET,	\
		VR9_PCE_IGMP_DRPM_DRPM_SHIFT, VR9_PCE_IGMP_DRPM_DRPM_SIZE, value_1);
	}
	if (pIGMPTbHandle->eIGMP_Mode == IFX_ETHSW_MULTICAST_SNOOP_MODE_FORWARD) {
		IFX_FLOW_PCE_rule_t PCE_rule;
			int i;
		if (value_1) {
			for (i = 0; i <2; i++) {
				memset(&PCE_rule, 0 , sizeof(IFX_FLOW_PCE_rule_t));
				PCE_rule.pattern.bEnable = IFX_TRUE;	
				PCE_rule.pattern.bProtocolEnable = IFX_TRUE;
				switch(i) {
					case 0:	
					/*	Management port remaining IGMP packets (forwarding them to Router Ports) */
						PCE_rule.pattern.nIndex = MULTI_PCE_RULES_INDEX;
						PCE_rule.pattern.nProtocol		= 0x2; /* for IPv4 */
						PCE_rule.pattern.bAppMaskRangeMSB_Select = IFX_TRUE;
						PCE_rule.pattern.bAppDataMSB_Enable	= IFX_TRUE;
						PCE_rule.pattern.nAppDataMSB		= 0x1200;
						PCE_rule.pattern.nAppMaskRangeMSB	= 0x1DFF;
						break;
					case 1:
						/* Management Port ICMPv6 Multicast Listerner Report & Leave (Avoiding Loopback abd Discard) */
						PCE_rule.pattern.nIndex = MULTI_PCE_RULES_INDEX+3;
						PCE_rule.pattern.bAppDataMSB_Enable	= IFX_TRUE;
						PCE_rule.pattern.bAppMaskRangeMSB_Select = IFX_TRUE;
						PCE_rule.pattern.nAppDataMSB		= 0x8300;
						PCE_rule.pattern.nAppMaskRangeMSB	= 0x1FF;
						PCE_rule.pattern.nProtocol			= 0x3A;  /*for IPv6*/
						break;
				}
				/* Router portmap */
				PCE_rule.action.ePortMapAction	= IFX_FLOW_PCE_ACTION_PORTMAP_ALTERNATIVE;
				PCE_rule.action.nForwardPortMap = value_1 ;
				if (pIGMPTbHandle->nClassOfService != 0 ) {
					PCE_rule.action.eTrafficClassAction = 1;
					PCE_rule.action.nTrafficClassAlternate = pEthSWDevHandle->IGMP_Flags.nClassOfService;
				}
				/*  Set eForwardPort */
				PCE_rule.pattern.bPortIdEnable	= IFX_TRUE;
				if ( pIGMPTbHandle->eForwardPort == IFX_ETHSW_PORT_FORWARD_PORT ) {
					PCE_rule.pattern.nPortId		= pIGMPTbHandle->nForwardPortId;
				} else if ( pIGMPTbHandle->eForwardPort == IFX_ETHSW_PORT_FORWARD_CPU ) {
					PCE_rule.pattern.nPortId		= (pEthSWDevHandle->nCPU_Port);
				} 
				if (pIGMPTbHandle->bCrossVLAN)
					PCE_rule.action.eVLAN_CrossAction = IFX_FLOW_PCE_ACTION_CROSS_VLAN_CROSS;
				else
					PCE_rule.action.eVLAN_CrossAction = IFX_FLOW_PCE_ACTION_CROSS_VLAN_DISABLE;
				/* We prepare everything and write into PCE Table */
				if (0 != ifx_pce_rule_write(&pEthSWDevHandle->PCE_Handler,&PCE_rule))
					return IFX_ERROR;
			}
		} else {
			for (i = 0; i <2; i++) {
				memset(&PCE_rule, 0 , sizeof(IFX_FLOW_PCE_rule_t));
				switch(i) {
					case 0:
						PCE_rule.pattern.nIndex = MULTI_PCE_RULES_INDEX;
						break;
					case 1:
						/* Management Port ICMPv6 Multicast Listerner Report & Leave (Avoiding Loopback abd Discard) */
						PCE_rule.pattern.nIndex = MULTI_PCE_RULES_INDEX+3;
						break;
					}
				if (0 != ifx_pce_pattern_delete(&pEthSWDevHandle->PCE_Handler, PCE_rule.pattern.nIndex))
				return IFX_ERROR;
			}
		}
	}
 	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_MulticastSnoopCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_multicastSnoopCfg_t *pPar)
{
	IFX_uint32_t value=0;
	IFX_uint32_t data_1, data_2;
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;

	/* eIGMP_Mode read */
	pPar->eIGMP_Mode = pEthSWDevHandle->IGMP_Flags.eIGMP_Mode;
	// Get bIGMPv3
    pPar->bIGMPv3 = pEthSWDevHandle->IGMP_Flags.bIGMPv3;
    // Get bCrossVLAN
	pPar->bCrossVLAN = pEthSWDevHandle->IGMP_Flags.bCrossVLAN;
    // Get eForwardPort
    pPar->eForwardPort = pEthSWDevHandle->IGMP_Flags.eForwardPort;
    pPar->nForwardPortId = pEthSWDevHandle->IGMP_Flags.nForwardPortId;
    // Set nClassOfService
    pPar->nClassOfService = pEthSWDevHandle->IGMP_Flags.nClassOfService;
    // Get nRobust
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_IGMP_CTRL_ROB_OFFSET,	\
		VR9_PCE_IGMP_CTRL_ROB_SHIFT, VR9_PCE_IGMP_CTRL_ROB_SIZE, &value );
	pPar->nRobust = value;
	// Get nQueryInterval
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_IGMP_CTRL_DMRT_OFFSET,	\
		VR9_PCE_IGMP_CTRL_DMRT_SHIFT, VR9_PCE_IGMP_CTRL_DMRT_SIZE, &value );
	pPar->nQueryInterval = value;
	// Get eSuppressionAggregation
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_IGMP_CTRL_REPSUP_OFFSET,	\
		VR9_PCE_IGMP_CTRL_REPSUP_SHIFT, VR9_PCE_IGMP_CTRL_REPSUP_SIZE, &data_1);
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_IGMP_CTRL_JASUP_OFFSET,	\
		VR9_PCE_IGMP_CTRL_JASUP_SHIFT, VR9_PCE_IGMP_CTRL_JASUP_SIZE, &data_2 );

	if ( data_1 == 0 && data_2 == 0 ) {
		pPar->eSuppressionAggregation = IFX_ETHSW_MULTICAST_TRANSPARENT;
    } else if (data_1 == 1 && data_2 == 0) {
		pPar->eSuppressionAggregation = IFX_ETHSW_MULTICAST_REPORT;
    } else if (data_1 == 1 && data_2 == 1) {
		pPar->eSuppressionAggregation = IFX_ETHSW_MULTICAST_REPORT_JOIN;
	} else {
		pPar->eSuppressionAggregation = IFX_ETHSW_MULTICAST_TRANSPARENT;
    }
    // Get bFastLeave
    ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_IGMP_CTRL_FLEAVE_OFFSET,
                        VR9_PCE_IGMP_CTRL_FLEAVE_SHIFT, VR9_PCE_IGMP_CTRL_FLEAVE_SIZE, &value);
	if (value == 1)
		pPar->bFastLeave = 1;
	else
		pPar->bFastLeave = 0;
	// Get bLearningRouter
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_IGMP_CTRL_SRPEN_OFFSET,	\
		VR9_PCE_IGMP_CTRL_SRPEN_SHIFT, VR9_PCE_IGMP_CTRL_SRPEN_SIZE, &value);
	pPar->bLearningRouter = value;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_MulticastSnoopCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_multicastSnoopCfg_t *pPar)
{
	IFX_uint32_t i, data_1 = 0, data_2 = 0, pce_table_index= MULTI_PCE_RULES_INDEX ;
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_FLOW_PCE_rule_t PCE_rule;

	/* Choose IGMP Mode */
	switch (pPar->eIGMP_Mode) {
		case IFX_ETHSW_MULTICAST_SNOOP_MODE_DISABLED:
			/* Snooping of Router Port Disable */
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_IGMP_CTRL_SRPEN_OFFSET,	\
				VR9_PCE_IGMP_CTRL_SRPEN_SHIFT, VR9_PCE_IGMP_CTRL_SRPEN_SIZE, 0);
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_GCTRL_0_IGMP_OFFSET,	\
				VR9_PCE_GCTRL_0_IGMP_SHIFT, VR9_PCE_GCTRL_0_IGMP_SIZE, 0);
			for (i = 0; i <= pEthSWDevHandle->nTotalPortNumber; i++) {
				ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PCTRL_0_MCST_OFFSET + (0xA * i),	\
					VR9_PCE_PCTRL_0_MCST_SHIFT, VR9_PCE_PCTRL_0_MCST_SIZE, 0);
			}
			break;
		case IFX_ETHSW_MULTICAST_SNOOP_MODE_AUTOLEARNING:
			/* Snooping of Router Port Enable */
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_GCTRL_0_IGMP_OFFSET,	\
				VR9_PCE_GCTRL_0_IGMP_SHIFT, VR9_PCE_GCTRL_0_IGMP_SIZE, 0);
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_IGMP_CTRL_SRPEN_OFFSET,	\
				VR9_PCE_IGMP_CTRL_SRPEN_SHIFT, VR9_PCE_IGMP_CTRL_SRPEN_SIZE, 1);
			for (i = 0; i <= pEthSWDevHandle->nTotalPortNumber ; i++) {
				ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PCTRL_0_MCST_OFFSET + (0xA * i),	\
					VR9_PCE_PCTRL_0_MCST_SHIFT, VR9_PCE_PCTRL_0_MCST_SIZE, 1);
			}
			break;
		case IFX_ETHSW_MULTICAST_SNOOP_MODE_FORWARD:
			/* Snooping of Router Port Forward */
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_IGMP_CTRL_SRPEN_OFFSET,	\
				VR9_PCE_IGMP_CTRL_SRPEN_SHIFT, VR9_PCE_IGMP_CTRL_SRPEN_SIZE, 0);
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_GCTRL_0_IGMP_OFFSET,	\
				VR9_PCE_GCTRL_0_IGMP_SHIFT, VR9_PCE_GCTRL_0_IGMP_SIZE, 1);
			for (i = 0; i <= pEthSWDevHandle->nTotalPortNumber ; i++) {
				ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PCTRL_0_MCST_OFFSET + (0xA * i),	\
					VR9_PCE_PCTRL_0_MCST_SHIFT, VR9_PCE_PCTRL_0_MCST_SIZE, 1);
			}
			break;
		default:
			IFXOS_PRINT_INT_RAW("This Mode doesn't exists\n");
			return IFX_ERROR;
	} /* -----  end switch  ----- */
	/* Set the Flag for eIGMP_Mode flag*/
	pEthSWDevHandle->IGMP_Flags.eIGMP_Mode = pPar->eIGMP_Mode;
	/* Set bIGMPv3 flag*/
	pEthSWDevHandle->IGMP_Flags.bIGMPv3 =  pPar->bIGMPv3;
	/* Set bCrossVLAN flag*/
	pEthSWDevHandle->IGMP_Flags.bCrossVLAN = pPar->bCrossVLAN;
	/* Set eForwardPort flag */
	pEthSWDevHandle->IGMP_Flags.eForwardPort = pPar->eForwardPort;
	/* Set nForwardPortId */
	if ( pPar->eForwardPort == IFX_ETHSW_PORT_FORWARD_CPU) {
		pEthSWDevHandle->IGMP_Flags.nForwardPortId = (1<< pEthSWDevHandle->nCPU_Port);
	} else {
		pEthSWDevHandle->IGMP_Flags.nForwardPortId = pPar->nForwardPortId;
	}
	pEthSWDevHandle->IGMP_Flags.nClassOfService = pPar->nClassOfService;
/*** Prepare and write into PCE Traffic Table ***
* We write the rule from the end of the Table
* For Hardware snooping with 8 rule and software snooping with another 5 rule.*/
	/* If IGMP mode set to AutoLearning then the following Rule have to add it */
	if (pPar->eIGMP_Mode == IFX_ETHSW_MULTICAST_SNOOP_MODE_AUTOLEARNING) {
		for (i = pce_table_index; i <= (pce_table_index + 7); i++) {
			memset(&PCE_rule, 0 , sizeof(IFX_FLOW_PCE_rule_t));
			PCE_rule.pattern.nIndex = i;
			PCE_rule.pattern.bEnable = IFX_TRUE;
			PCE_rule.pattern.bAppDataMSB_Enable = IFX_TRUE;
			if ((i == pce_table_index + 0) || (i == pce_table_index + 1) || (i == pce_table_index + 2))
				PCE_rule.pattern.nAppDataMSB = 0x1100;
			else if (i == pce_table_index + 3)
				PCE_rule.pattern.nAppDataMSB = 0x1200;
			else if (i == pce_table_index + 4)
				PCE_rule.pattern.nAppDataMSB = 0x1600;
			else if (i == pce_table_index + 5)
				PCE_rule.pattern.nAppDataMSB = 0x1700;
			else if (i == pce_table_index + 6)
				PCE_rule.pattern.nAppDataMSB = 0x3100;
			else if (i == pce_table_index + 7)
				PCE_rule.pattern.nAppDataMSB = 0x3000;

			PCE_rule.pattern.bAppMaskRangeMSB_Select = 0;
			PCE_rule.pattern.nAppMaskRangeMSB = 0x3;
			if ((i == pce_table_index + 0)|| (i == pce_table_index + 1) || (i == pce_table_index + 6)	\
				|| (i == pce_table_index + 7))
				PCE_rule.pattern.eDstIP_Select = IFX_TRUE;
			if ((i == pce_table_index + 0) || (i == pce_table_index + 1))
				PCE_rule.pattern.nDstIP.nIPv4 = 0xE0000001;
			else if (i == pce_table_index + 6)
				PCE_rule.pattern.nDstIP.nIPv4 = 0xE0000002;
			else if (i == pce_table_index + 7)
				PCE_rule.pattern.nDstIP.nIPv4 = 0xE00000A6;
			PCE_rule.pattern.nDstIP_Mask = 0xFF00;
			if (i == pce_table_index + 1)
				PCE_rule.pattern.eSrcIP_Select = IFX_TRUE;
			else
				PCE_rule.pattern.eSrcIP_Select = 0;
			if (i == pce_table_index + 1)
				PCE_rule.pattern.nSrcIP_Mask = 0xFF00;
			else
				PCE_rule.pattern.nSrcIP_Mask = 0xFFFF;
			PCE_rule.pattern.bProtocolEnable = IFX_TRUE;
			PCE_rule.pattern.nProtocol = 0x2;
			if (pEthSWDevHandle->IGMP_Flags.nClassOfService == 0 ) {
				PCE_rule.action.eTrafficClassAction = 0;
				PCE_rule.action.nTrafficClassAlternate = 0;
			} else {
				PCE_rule.action.eTrafficClassAction = 1;
				PCE_rule.action.nTrafficClassAlternate = pEthSWDevHandle->IGMP_Flags.nClassOfService;
			}
			if (i == pce_table_index + 0)
				PCE_rule.action.eSnoopingTypeAction =IFX_FLOW_PCE_ACTION_IGMP_SNOOP_QUERY;
			else if (i == pce_table_index + 1)
				PCE_rule.action.eSnoopingTypeAction = IFX_FLOW_PCE_ACTION_IGMP_SNOOP_QUERY_NO_ROUTER;
			else if (i == pce_table_index + 2)
				PCE_rule.action.eSnoopingTypeAction = IFX_FLOW_PCE_ACTION_IGMP_SNOOP_QUERY_GROUP;
			else if (i == pce_table_index + 3)
				PCE_rule.action.eSnoopingTypeAction = IFX_FLOW_PCE_ACTION_IGMP_SNOOP_REPORT;
			else if (i == pce_table_index + 4)
				PCE_rule.action.eSnoopingTypeAction = IFX_FLOW_PCE_ACTION_IGMP_SNOOP_REPORT;
			else if (i == pce_table_index + 5)
				PCE_rule.action.eSnoopingTypeAction = IFX_FLOW_PCE_ACTION_IGMP_SNOOP_LEAVE;
			else if (i == pce_table_index + 6)
				PCE_rule.action.eSnoopingTypeAction = IFX_FLOW_PCE_ACTION_IGMP_SNOOP_AD;
			else if (i == pce_table_index + 7)
				PCE_rule.action.eSnoopingTypeAction = IFX_FLOW_PCE_ACTION_IGMP_SNOOP_AD;
/*			if ((i == pce_table_index + 3) || (i == pce_table_index + 4) || (i == pce_table_index + 5)) */
			PCE_rule.action.ePortMapAction = IFX_FLOW_PCE_ACTION_PORTMAP_MULTICAST_ROUTER;
			if (pPar->bCrossVLAN)
				PCE_rule.action.eVLAN_CrossAction = IFX_FLOW_PCE_ACTION_CROSS_VLAN_CROSS;
			else
				PCE_rule.action.eVLAN_CrossAction = IFX_FLOW_PCE_ACTION_CROSS_VLAN_DISABLE;
			/* We prepare everything and write into PCE Table */
			if (0 != ifx_pce_rule_write(&pEthSWDevHandle->PCE_Handler,&PCE_rule)) {
				IFXOS_PRINT_INT_RAW("ERROR: : %s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__);
				return IFX_ERROR;
			}
		}
	}
#if defined(FLOW_TABLE_DEBUG) && FLOW_TABLE_DEBUG
	IFX_FLOW_PROTOCOL_Table_Print();
	IFX_FLOW_IP_DASA_LSB_Table_Print();
	IFX_FLOW_APP_DATA_MSB_LSB_Table_Print();
	// IFX_FLOW_PCE_TRAFFIC_Table_Print();
#endif
	/* If IGMP mode set to forwarding then the following Rule have to add it */
	if (pPar->eIGMP_Mode == IFX_ETHSW_MULTICAST_SNOOP_MODE_FORWARD) {
		for (i = pce_table_index; i <= (pce_table_index + 7); i++) {
			memset(&PCE_rule, 0 , sizeof(IFX_FLOW_PCE_rule_t));
			PCE_rule.pattern.nIndex = i;
			PCE_rule.pattern.bEnable = IFX_TRUE;	
			PCE_rule.pattern.bProtocolEnable = IFX_TRUE;
			switch(i - pce_table_index) {
//				case 0:
					/*Rule added by Router port ADD function based on router port for IPv4*/
//					break;
				case 1:
					/*	Avoid IGMP Packets Redirection when seen on Management Port */
					PCE_rule.pattern.nProtocol		= 0x2; /* for IPv4 */
					PCE_rule.pattern.bPortIdEnable	= IFX_TRUE;
					/* Action Enabled, no redirection (default portmap) */
					PCE_rule.action.ePortMapAction	= IFX_FLOW_PCE_ACTION_PORTMAP_REGULAR; 
					break;
				case 2:
					/* IGMPv1/2/3 IPv4 */
					PCE_rule.pattern.nProtocol		= 0x2; /* for IPv4 */
					PCE_rule.action.ePortMapAction = IFX_FLOW_PCE_ACTION_PORTMAP_ALTERNATIVE;
					break;
//				case 3:
					/*Rules added by Router port ADD function based on router port for IPv6 */		
//					break;
				case 4:
					/*	Managemnt Port Remaining ICMPv6/MLD packets(Avoiding Loopback and Disacard) */
					PCE_rule.pattern.bPortIdEnable	= IFX_TRUE;
					PCE_rule.pattern.nPortId		= pPar->nForwardPortId;
					PCE_rule.pattern.nProtocol		= 0x3A;  /*for IPv6*/
					PCE_rule.pattern.bPortIdEnable	= IFX_TRUE;
					PCE_rule.action.ePortMapAction	= IFX_FLOW_PCE_ACTION_PORTMAP_REGULAR;
					break;
				case 5:
					/* ICMPv6 Multicast Listener Query/Report/Done(Leave) */
					PCE_rule.pattern.bAppDataMSB_Enable	= IFX_TRUE;
					PCE_rule.pattern.bAppMaskRangeMSB_Select = IFX_TRUE;
					PCE_rule.pattern.nAppDataMSB		= 0x8200;
					PCE_rule.pattern.nAppMaskRangeMSB	= 0x2FF;
					PCE_rule.pattern.nProtocol			= 0x3A;  /*for IPv6*/
					PCE_rule.action.ePortMapAction = IFX_FLOW_PCE_ACTION_PORTMAP_ALTERNATIVE;
					break;
				case 6:
					/* ICMPv6 Multicast Listener Report */
					PCE_rule.pattern.bAppDataMSB_Enable	= IFX_TRUE;
					PCE_rule.pattern.nAppDataMSB		= 0x8F00;
					PCE_rule.pattern.nAppMaskRangeMSB	= 0x3;
					PCE_rule.pattern.nProtocol			= 0x3A;  /*for IPv6*/
					PCE_rule.action.ePortMapAction = IFX_FLOW_PCE_ACTION_PORTMAP_ALTERNATIVE;
					break;
				case 7:
					/* ICMPv6 Multicast Router Advertisement/Solicitation/Termination */
					PCE_rule.pattern.bAppDataMSB_Enable	= IFX_TRUE;
					PCE_rule.pattern.bAppMaskRangeMSB_Select = IFX_TRUE;
					PCE_rule.pattern.nAppDataMSB		= 0x9700; 
					PCE_rule.pattern.nAppMaskRangeMSB	= 0x2FF;
					PCE_rule.pattern.nProtocol			= 0x3A;  /*for IPv6*/
					PCE_rule.action.ePortMapAction = IFX_FLOW_PCE_ACTION_PORTMAP_ALTERNATIVE;
					break;
				default:
					continue;
			}
			if (pEthSWDevHandle->IGMP_Flags.nClassOfService != 0 ) {
				PCE_rule.action.eTrafficClassAction = 1;
				PCE_rule.action.nTrafficClassAlternate = pEthSWDevHandle->IGMP_Flags.nClassOfService;
			}
			/*  Set eForwardPort */
			if ( pPar->eForwardPort == IFX_ETHSW_PORT_FORWARD_PORT ) {
				PCE_rule.action.nForwardPortMap = (1 << pPar->nForwardPortId);
				PCE_rule.pattern.nPortId		= pPar->nForwardPortId;
			} else if ( pPar->eForwardPort == IFX_ETHSW_PORT_FORWARD_CPU ) {
				PCE_rule.action.nForwardPortMap = (1<< pEthSWDevHandle->nCPU_Port);
				PCE_rule.pattern.nPortId		= pEthSWDevHandle->nCPU_Port;
			} 
			if (pPar->bCrossVLAN)
				PCE_rule.action.eVLAN_CrossAction = IFX_FLOW_PCE_ACTION_CROSS_VLAN_CROSS;
			else
				PCE_rule.action.eVLAN_CrossAction = IFX_FLOW_PCE_ACTION_CROSS_VLAN_DISABLE;
			/* We prepare everything and write into PCE Table */
			if (0 != ifx_pce_rule_write(&pEthSWDevHandle->PCE_Handler,&PCE_rule))
				return IFX_ERROR;
		}
#if defined(FLOW_TABLE_DEBUG) && FLOW_TABLE_DEBUG
		IFX_FLOW_PROTOCOL_Table_Print();
		IFX_FLOW_PCE_TRAFFIC_Table_Print();
#endif
	}
	if(pPar->eIGMP_Mode == IFX_ETHSW_MULTICAST_SNOOP_MODE_DISABLED) {
		pce_table_index = MULTI_PCE_RULES_INDEX;
		for (i = pce_table_index; i <= (pce_table_index + 7); i++) {
			PCE_rule.pattern.nIndex = i;
			PCE_rule.pattern.bEnable = IFX_FALSE;
			/* We prepare everything and write into PCE Table */
			if (0 != ifx_pce_pattern_delete(&pEthSWDevHandle->PCE_Handler, i))
				return IFX_ERROR;
		}
	}
	// Set nRobust
	if ( pPar->nRobust < 4) {
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_IGMP_CTRL_ROB_OFFSET,	\
			VR9_PCE_IGMP_CTRL_ROB_SHIFT, VR9_PCE_IGMP_CTRL_ROB_SIZE, pPar->nRobust );
	} else {
		IFXOS_PRINT_INT_RAW("The Robust time would only support 0..3\n");
		return IFX_ERROR;
	}
	// Set nQueryInterval
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_IGMP_CTRL_DMRTEN_OFFSET,	\
		VR9_PCE_IGMP_CTRL_DMRTEN_SHIFT, VR9_PCE_IGMP_CTRL_DMRTEN_SIZE, 1);
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_IGMP_CTRL_DMRT_OFFSET,	\
		VR9_PCE_IGMP_CTRL_DMRT_SHIFT, VR9_PCE_IGMP_CTRL_DMRT_SIZE, pPar->nQueryInterval );

	// Set eSuppressionAggregation
	if (pPar->eIGMP_Mode == IFX_ETHSW_MULTICAST_SNOOP_MODE_AUTOLEARNING) {
		switch (pPar->eSuppressionAggregation) {
			case IFX_ETHSW_MULTICAST_REPORT_JOIN:
				data_2 = 1;
				data_1 = 1;
				break;
			case IFX_ETHSW_MULTICAST_REPORT:
				data_2 = 0;
				data_1 = 1;
				break;
			case IFX_ETHSW_MULTICAST_TRANSPARENT:
				data_2 = 0;
				data_1 = 0;
				break;
		}       /* -----  end switch  ----- */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_IGMP_CTRL_REPSUP_OFFSET,	\
			VR9_PCE_IGMP_CTRL_REPSUP_SHIFT, VR9_PCE_IGMP_CTRL_REPSUP_SIZE, data_1);
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_IGMP_CTRL_JASUP_OFFSET,	\
			VR9_PCE_IGMP_CTRL_JASUP_SHIFT, VR9_PCE_IGMP_CTRL_JASUP_SIZE, data_2 );
	}
	// Set bFastLeave, supported only if SNOOPING MODE AUTOLEARNING
	if (pPar->eIGMP_Mode == IFX_ETHSW_MULTICAST_SNOOP_MODE_AUTOLEARNING) {
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_IGMP_CTRL_SRPEN_OFFSET,	\
			VR9_PCE_IGMP_CTRL_SRPEN_SHIFT, VR9_PCE_IGMP_CTRL_SRPEN_SIZE, pPar->bLearningRouter);
		if (pPar->bFastLeave == 1)
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_IGMP_CTRL_FLEAVE_OFFSET,	\
				VR9_PCE_IGMP_CTRL_FLEAVE_SHIFT, VR9_PCE_IGMP_CTRL_FLEAVE_SIZE, 1);
		else
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_IGMP_CTRL_FLEAVE_OFFSET,	\
				VR9_PCE_IGMP_CTRL_FLEAVE_SHIFT, VR9_PCE_IGMP_CTRL_FLEAVE_SIZE, 0);
	} else {
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_IGMP_CTRL_FLEAVE_OFFSET,	\
			VR9_PCE_IGMP_CTRL_FLEAVE_SHIFT, VR9_PCE_IGMP_CTRL_FLEAVE_SIZE, 0);
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_MulticastTableEntryAdd(IFX_void_t *pDevCtx, IFX_ETHSW_multicastTable_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_FLOW_IGMP_t      *pIGMPTbHandle   = &pEthSWDevHandle->IGMP_Flags;
	IFX_uint8_t portIdx = pPar->nPortId;
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pData;

	if (pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber)
		return IFX_ERROR;
	memset(&pData, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
	if (pIGMPTbHandle->eIGMP_Mode == IFX_ETHSW_MULTICAST_SNOOP_MODE_AUTOLEARNING) {
		IFX_uint32_t index = 0, i, available= 0;
		if ((pEthSWDevHandle->IGMP_Flags.bIGMPv3 == 1) || (pPar->eIPVersion == IFX_ETHSW_IP_SELECT_IPV6))
			return IFX_ERROR;
		/* Read Out all of the HW Table */
		for (i=0; i < IFX_FLOW_MULTICAST_HW_TBL_SIZE; i++) {
			pData.table = IFX_ETHSW_PCE_MULTICAST_HW_INDEX; //Multicast Table Address
			pData.table_index = i;
			ifx_ethsw_xwayflow_pce_table_read(pEthSWDevHandle, &pData);
			if (pData.valid) {
				if ( ( pData.key[0] == (pPar->uIP_Gda.nIPv4 & 0xFFFF) )	\
					&&  (pData.key[1] == ((pPar->uIP_Gda.nIPv4 >> 16 ) & 0xFFFF) ) ) {
					index = i;
					available = 1;
					break;
				}
			}
		}
		pData.table = IFX_ETHSW_PCE_MULTICAST_HW_INDEX;
		if (available == 0 ) {
			index = IFX_FLOW_MULTICAST_HW_TBL_SIZE; /*Assume table is full */
			for (i=0; i < IFX_FLOW_MULTICAST_HW_TBL_SIZE; i++) {
				pData.table_index = i;
				ifx_ethsw_xwayflow_pce_table_read(pEthSWDevHandle, &pData);
				if (pData.valid == 0) {
					index = i;  /* Free index */
					break;
				}
			}
		}
		if ( index < IFX_FLOW_MULTICAST_HW_TBL_SIZE ) {
			pData.table			= IFX_ETHSW_PCE_MULTICAST_HW_INDEX;
			pData.table_index	= index;
			pData.key[1]		= ((pPar->uIP_Gda.nIPv4 >> 16 ) & 0xFFFF);
			pData.key[0]		= (pPar->uIP_Gda.nIPv4 & 0xFFFF);
			pData.val[0]		|= (1 << portIdx);
			pData.val[4]		|= (1 << 14);
			pData.valid			= 1;
#if defined(FLOW_TABLE_DEBUG) && FLOW_TABLE_DEBUG
		/* Print out debug info first */
		IFXOS_PRINT_INT_RAW("|DEBUG: GroupIPv4 = %03d.%03d.%03d.%03d \n",	\
			(pData.key[1] >> 8) & 0xFF, (pData.key[1] & 0xFF),	\
			(pData.key[0] >> 8) & 0xFF, (pData.key[0] & 0xFF));
#endif
			ifx_ethsw_xwayflow_pce_table_write(pEthSWDevHandle, &pData);
		} else {
			IFXOS_PRINT_INT_RAW("Error: (IGMP HW Table is full) %s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__);
			return IFX_ERROR;
		}

	} else if (pIGMPTbHandle->eIGMP_Mode == IFX_ETHSW_MULTICAST_SNOOP_MODE_FORWARD) {
		/* Program the Multicast SW Table */
		ifx_multicast_sw_table_write(pEthSWDevHandle, pPar);
	} else {
		/* Disable All Multicast SW Table */
		IFXOS_PRINT_INT_RAW("Please Select the IGMP Mode through Multicast Snooping Configuration API\n");
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_MulticastTableEntryRead(IFX_void_t *pDevCtx, IFX_ETHSW_multicastTableRead_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_FLOW_IGMP_t      *pIGMPTbHandle   = &pEthSWDevHandle->IGMP_Flags;
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pData;

	if (pIGMPTbHandle->eIGMP_Mode == IFX_ETHSW_MULTICAST_SNOOP_MODE_DISABLED) {
		IFXOS_PRINT_INT_RAW("Error: (IGMP snoop is not enabled) %s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__);
		return IFX_ERROR;
	}
	if (pIGMPTbHandle->eIGMP_Mode == IFX_ETHSW_MULTICAST_SNOOP_MODE_AUTOLEARNING) {
		if (pPar->bInitial == IFX_TRUE) {
			pEthSWDevHandle->multi_hw_table_index = 0; /*Start from the index 0 */
			pPar->bInitial = IFX_FALSE;
		}
		if (pEthSWDevHandle->multi_hw_table_index >= IFX_FLOW_MULTICAST_HW_TBL_SIZE) {
			memset(pPar, 0, sizeof(IFX_ETHSW_multicastTableRead_t));
			pPar->bLast = IFX_TRUE;
			pEthSWDevHandle->multi_hw_table_index = 0;
			return IFX_SUCCESS;
		}
		
		do {
			memset(&pData, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
			pData.table = IFX_ETHSW_PCE_MULTICAST_HW_INDEX;
			pData.table_index = pEthSWDevHandle->multi_hw_table_index;
			ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pData);
			pEthSWDevHandle->multi_hw_table_index++;
			if (pData.valid != 0)
				break;
		} while (pEthSWDevHandle->multi_hw_table_index < IFX_FLOW_MULTICAST_HW_TBL_SIZE);
		if (pData.valid != 0) {
			pPar->nPortId 		= pData.val[0] | 0x80000000;
			pPar->uIP_Gda.nIPv4 = ( (pData.key[1] << 16) | pData.key[0] );
			pPar->uIP_Gsa.nIPv4 = 0;
			pPar->eModeMember 	= IFX_ETHSW_IGMP_MEMBER_DONT_CARE;
			pPar->eIPVersion 	= IFX_ETHSW_IP_SELECT_IPV4;
			pPar->bInitial = IFX_FALSE;
			pPar->bLast = IFX_FALSE;
		} else {
			memset(pPar, 0, sizeof(IFX_ETHSW_multicastTableRead_t));
			pPar->bLast = IFX_TRUE;
			pEthSWDevHandle->multi_hw_table_index = 0;
		}
	}
	/*Snooping in Forward mode */
	if (pIGMPTbHandle->eIGMP_Mode == IFX_ETHSW_MULTICAST_SNOOP_MODE_FORWARD) {
		IFX_uint32_t Dest_lsb_index, Src_lsb_index,Dest_msb_index, Src_msb_index;
		if (pPar->bInitial == IFX_TRUE) {
			pEthSWDevHandle->multi_sw_table_index = 0; /*Start from the index 0 */
			pPar->bInitial = IFX_FALSE;
		}
		if (pEthSWDevHandle->multi_sw_table_index >= IFX_FLOW_MULTICAST_SW_TBL_SIZE) {
			memset(pPar, 0, sizeof(IFX_ETHSW_multicastTableRead_t));
			pPar->bLast = IFX_TRUE;
			pEthSWDevHandle->multi_sw_table_index = 0;
			return IFX_SUCCESS;
		}
		
		do {
			memset(&pData, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
			pData.table = IFX_ETHSW_PCE_MULTICAST_SW_INDEX;
			pData.table_index = pEthSWDevHandle->multi_sw_table_index;
			ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pData);
			pEthSWDevHandle->multi_sw_table_index++;
			if (pData.valid != 0)
				break;
		}while (pEthSWDevHandle->multi_sw_table_index < IFX_FLOW_MULTICAST_SW_TBL_SIZE);
		if (pData.valid == 1) {
			IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pData_IP_Table;
			pPar->nPortId 	= pData.val[0] | 0x80000000;
			Dest_lsb_index	= pData.key[0] & 0xFF;
			Dest_msb_index	= (pData.key[0] >> 8 ) & 0xFF;
			Src_lsb_index	= pData.key[1] & 0xFF;
			Src_msb_index	= (pData.key[1] >> 8 ) & 0xFF;
			if (Dest_lsb_index <= 0x3F ) {
				memset(&pData_IP_Table, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
				pData_IP_Table.table = IFX_ETHSW_PCE_IP_DASA_LSB_INDEX;
				/* Search the DIP */
				pData_IP_Table.table_index = Dest_lsb_index;
				ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pData_IP_Table);
				if (pData_IP_Table.valid == 1) {
					if ( pData_IP_Table.mask  == 0xFF00 ) {
						pPar->uIP_Gda.nIPv4 = (pData_IP_Table.key[1] << 16) | (pData_IP_Table.key[0] & 0xFFFF);
						pPar->eIPVersion 	= IFX_ETHSW_IP_SELECT_IPV4;
					} else if ( pData_IP_Table.mask  == 0x0 ) {
						pPar->uIP_Gda.nIPv6[4] = (pData_IP_Table.key[3] & 0xFFFF);
						pPar->uIP_Gda.nIPv6[5] = (pData_IP_Table.key[2] & 0xFFFF);
						pPar->uIP_Gda.nIPv6[6] = (pData_IP_Table.key[1] & 0xFFFF);
						pPar->uIP_Gda.nIPv6[7] = (pData_IP_Table.key[0] & 0xFFFF);
						pPar->eIPVersion 	= IFX_ETHSW_IP_SELECT_IPV6;
					}
				}
			}
			if (Src_lsb_index <= 0x3F ) {
				memset(&pData_IP_Table, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
				pData_IP_Table.table = IFX_ETHSW_PCE_IP_DASA_LSB_INDEX;
				/* Search the SIP */
				pData_IP_Table.table_index = Src_lsb_index;
				ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pData_IP_Table);
				if (pData_IP_Table.valid == 1) {
					if ( pData_IP_Table.mask  == 0xFF00 ) {
						pPar->uIP_Gsa.nIPv4 = (pData_IP_Table.key[1] << 16) | (pData_IP_Table.key[0] & 0xFFFF);
						pPar->eIPVersion 	= IFX_ETHSW_IP_SELECT_IPV4;
					} else if ( pData_IP_Table.mask  == 0x0 ) {
						pPar->uIP_Gsa.nIPv6[4] = (pData_IP_Table.key[3] & 0xFFFF);
						pPar->uIP_Gsa.nIPv6[5] = (pData_IP_Table.key[2] & 0xFFFF);
						pPar->uIP_Gsa.nIPv6[6] = (pData_IP_Table.key[1] & 0xFFFF);
						pPar->uIP_Gsa.nIPv6[7] = (pData_IP_Table.key[0] & 0xFFFF);
					}
				}
			}
			if (Dest_msb_index <= 0xF ) {
				memset(&pData_IP_Table, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
				pData_IP_Table.table = IFX_ETHSW_PCE_IP_DASA_MSB_INDEX;
				/* Search the DIP */
				pData_IP_Table.table_index = Dest_msb_index;
				ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pData_IP_Table);
				if (pData_IP_Table.valid == 1) {
					if ( pData_IP_Table.mask  == 0 ) {
						pPar->uIP_Gda.nIPv6[0] = (pData_IP_Table.key[3] & 0xFFFF);
						pPar->uIP_Gda.nIPv6[1] = (pData_IP_Table.key[2] & 0xFFFF);
						pPar->uIP_Gda.nIPv6[2] = (pData_IP_Table.key[1] & 0xFFFF);
						pPar->uIP_Gda.nIPv6[3] = (pData_IP_Table.key[0] & 0xFFFF);
					}
				}
			}
			if (Src_msb_index <= 0xF ) {
				memset(&pData_IP_Table, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
				pData_IP_Table.table = IFX_ETHSW_PCE_IP_DASA_MSB_INDEX;
				/* Search the DIP */
				pData_IP_Table.table_index = Src_msb_index;
				ifx_ethsw_xwayflow_pce_table_read(pDevCtx, &pData_IP_Table);
				if (pData_IP_Table.valid == 1) {
					if ( pData_IP_Table.mask  == 0 ) {
						pPar->uIP_Gsa.nIPv6[0] = (pData_IP_Table.key[3] & 0xFFFF);
						pPar->uIP_Gsa.nIPv6[1] = (pData_IP_Table.key[2] & 0xFFFF);
						pPar->uIP_Gsa.nIPv6[2] = (pData_IP_Table.key[1] & 0xFFFF);
						pPar->uIP_Gsa.nIPv6[3] = (pData_IP_Table.key[0] & 0xFFFF);
					}
				}
			}
			pPar->eModeMember = pIGMPTbHandle->multicast_sw_table[pEthSWDevHandle->multi_sw_table_index-1].eModeMember;
			pPar->bInitial = IFX_FALSE;
			pPar->bLast = IFX_FALSE;
		} else {

			memset(pPar, 0, sizeof(IFX_ETHSW_multicastTableRead_t));
			pPar->bLast = IFX_TRUE;
			pEthSWDevHandle->multi_sw_table_index = 0;
		}
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_MulticastTableEntryRemove(IFX_void_t *pDevCtx, IFX_ETHSW_multicastTable_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_FLOW_IGMP_t      *pIGMPTbHandle   = &pEthSWDevHandle->IGMP_Flags;
	IFX_uint8_t portIdx = pPar->nPortId;
	IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pData;
	IFX_uint32_t i;
	IFX_boolean_t DeletFlag = IFX_FALSE;
	IFX_uint32_t port = 0;

	if (pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber)
		return IFX_ERROR;
	memset(&pData, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
	if (pIGMPTbHandle->eIGMP_Mode == IFX_ETHSW_MULTICAST_SNOOP_MODE_AUTOLEARNING) {
		if (pEthSWDevHandle->IGMP_Flags.bIGMPv3 == 1)
			return IFX_ERROR;
		/* Read Out all of the HW Table */
		for (i=0; i < IFX_FLOW_MULTICAST_HW_TBL_SIZE; i++) {
			memset(&pData, 0 , sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));
			pData.table = IFX_ETHSW_PCE_MULTICAST_HW_INDEX; //Multicast Table Address
			pData.table_index = i;
			ifx_ethsw_xwayflow_pce_table_read(pEthSWDevHandle, &pData);
			/* Fill into Structure */
			if (((pData.val[0] >> portIdx) & 0x1 )== 1) {
				if (pPar->uIP_Gda.nIPv4 == ((pData.key[1] << 16) | (pData.key[0] & 0xFFFF))) {
					port = (pData.val[0] & ( ~(1 << portIdx)));
					if (port == 0) {
						pData.val[0] = 0;
						pData.key[1] = 0;
						pData.val[4] = 0;
					} else {
						pData.val[0] &= ~(1 << portIdx);
					}
					DeletFlag = IFX_TRUE;
					ifx_ethsw_xwayflow_pce_table_write(pDevCtx, &pData);
				}
			}
		}
		if (DeletFlag == IFX_FALSE)
			IFXOS_PRINT_INT_RAW("The input did not found \n");
	} else if (pIGMPTbHandle->eIGMP_Mode == IFX_ETHSW_MULTICAST_SNOOP_MODE_FORWARD) {
		/* Program the Multicast SW Table */
		ifx_multicast_sw_table_remove(pEthSWDevHandle, pPar);
	}
	return IFX_SUCCESS;
}
#endif /*CONFIG_LTQ_MULTICAST*/
IFX_return_t IFX_FLOW_CapGet(IFX_void_t *pDevCtx, IFX_ETHSW_cap_t *pPar)
{
	IFX_uint32_t value, data1, data2;

	if (pPar->nCapType >= IFX_ETHSW_CAP_TYPE_LAST)
		return IFX_ERROR;
	else
		strcpy(pPar->cDesc, CAP_Description[pPar->nCapType].Desci);

	/* As request, attached the code in the next version*/
	switch (pPar->nCapType) {
		case IFX_ETHSW_CAP_TYPE_PORT:
			ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_1_PPORTS_OFFSET,	\
				VR9_ETHSW_CAP_1_PPORTS_SHIFT, VR9_ETHSW_CAP_1_PPORTS_SIZE, &value);
			pPar->nCap = value;
			break;
		case IFX_ETHSW_CAP_TYPE_VIRTUAL_PORT:
			ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_1_VPORTS_OFFSET,	\
				VR9_ETHSW_CAP_1_VPORTS_SHIFT, VR9_ETHSW_CAP_1_VPORTS_SIZE, &value);
			pPar->nCap = value;
			break;
		case IFX_ETHSW_CAP_TYPE_BUFFER_SIZE:
			ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_11_BSIZEL_OFFSET,	\
				VR9_ETHSW_CAP_11_BSIZEL_SHIFT, VR9_ETHSW_CAP_11_BSIZEL_SIZE, &data1);
			ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_12_BSIZEH_OFFSET,	\
				VR9_ETHSW_CAP_12_BSIZEH_SHIFT, VR9_ETHSW_CAP_12_BSIZEH_SIZE, &data2);
			pPar->nCap = (data2 << 16 | data1);
			break;
		case IFX_ETHSW_CAP_TYPE_SEGMENT_SIZE:
			/* This is Hard coded */
			pPar->nCap = VRX_PLATFORM_CAP_SEGNENT;
			break;
		case IFX_ETHSW_CAP_TYPE_PRIORITY_QUEUE:
			ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_1_QUEUE_OFFSET,	\
				VR9_ETHSW_CAP_1_QUEUE_SHIFT, VR9_ETHSW_CAP_1_QUEUE_SIZE, &value);
			pPar->nCap = value;
			break;
		case IFX_ETHSW_CAP_TYPE_METER:
			ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_3_METERS_OFFSET,	\
				VR9_ETHSW_CAP_3_METERS_SHIFT, VR9_ETHSW_CAP_3_METERS_SIZE, &value);
			pPar->nCap = value;
			break;
      case IFX_ETHSW_CAP_TYPE_RATE_SHAPER:
			ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_3_SHAPERS_OFFSET,	\
				VR9_ETHSW_CAP_3_SHAPERS_SHIFT, VR9_ETHSW_CAP_3_SHAPERS_SIZE, &value);
			pPar->nCap = value;
			break;
		case IFX_ETHSW_CAP_TYPE_VLAN_GROUP:
			ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_4_VLAN_OFFSET,	\
				VR9_ETHSW_CAP_4_VLAN_SHIFT, VR9_ETHSW_CAP_4_VLAN_SIZE, &value);
			pPar->nCap = value;
			break;
		case IFX_ETHSW_CAP_TYPE_FID:
			/* This is Hard coded */
			pPar->nCap = VRX_PLATFORM_CAP_FID;
			break;
		case IFX_ETHSW_CAP_TYPE_MAC_TABLE_SIZE:
			ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_10_MACBT_OFFSET,	\
				VR9_ETHSW_CAP_10_MACBT_SHIFT, VR9_ETHSW_CAP_10_MACBT_SIZE, &value);
			pPar->nCap = value;
			break;
		case IFX_ETHSW_CAP_TYPE_MULTICAST_TABLE_SIZE:
			ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_8_MCAST_OFFSET,
				VR9_ETHSW_CAP_8_MCAST_SHIFT, VR9_ETHSW_CAP_8_MCAST_SIZE, &value);
			pPar->nCap = value;
			break;
		case IFX_ETHSW_CAP_TYPE_PPPOE_SESSION:
			ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_ETHSW_CAP_4_PPPOE_OFFSET,	\
				VR9_ETHSW_CAP_4_PPPOE_SHIFT, VR9_ETHSW_CAP_4_PPPOE_SIZE, &value);
			pPar->nCap = value;
			break;
		case IFX_ETHSW_CAP_TYPE_LAST:
			pPar->nCap = 12;
			break;
		default:
			pPar->nCap = 0;
			break;
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_CfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_cfg_t *pPar)
{
	IFX_uint32_t value, data2;
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	/* Aging Counter Mantissa Value */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_AGE_1_MANT_OFFSET,
                                 VR9_PCE_AGE_1_MANT_SHIFT, VR9_PCE_AGE_1_MANT_SIZE, &data2);

   if ( data2 == AGETIMER_1_DAY)
      pPar->eMAC_TableAgeTimer = IFX_ETHSW_AGETIMER_1_DAY;
   else if (data2 == AGETIMER_1_HOUR)
      pPar->eMAC_TableAgeTimer = IFX_ETHSW_AGETIMER_1_HOUR;
   else if (data2 == AGETIMER_300_SEC)
      pPar->eMAC_TableAgeTimer = IFX_ETHSW_AGETIMER_300_SEC;
   else if (data2 == AGETIMER_10_SEC)
      pPar->eMAC_TableAgeTimer = IFX_ETHSW_AGETIMER_10_SEC;
   else if (data2 == AGETIMER_1_SEC)
      pPar->eMAC_TableAgeTimer = IFX_ETHSW_AGETIMER_1_SEC;
   else
      pPar->eMAC_TableAgeTimer = 0;

	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_MAC_FLEN_LEN_OFFSET,	\
		VR9_MAC_FLEN_LEN_SHIFT, VR9_MAC_FLEN_LEN_SIZE, &value);
	pPar->nMaxPacketLen = value;
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_MAC_PFAD_CFG_SAMOD_OFFSET,	\
		VR9_MAC_PFAD_CFG_SAMOD_SHIFT, VR9_MAC_PFAD_CFG_SAMOD_SIZE, &value);
	pPar->bPauseMAC_ModeSrc = value;
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_GCTRL_0_VLAN_OFFSET,	\
		VR9_PCE_GCTRL_0_VLAN_SHIFT, VR9_PCE_GCTRL_0_VLAN_SIZE, &value);
	pPar->bVLAN_Aware = value;
	/* MAC Address Learning Limitation Mode */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_GCTRL_0_PLIMMOD_OFFSET ,	\
		VR9_PCE_GCTRL_0_PLIMMOD_SHIFT, VR9_PCE_GCTRL_0_PLIMMOD_SIZE, &pPar->bLearningLimitAction);
	if ( pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2) {
	/* Supported for GSWIP 2.2 and newer and returns with an error for older hardware revisions. */
		/*Accept or discard MAC spoofing and port MAC locking violation packets */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_GCTRL_0_PLCKMOD_OFFSET ,	\
			VR9_PCE_GCTRL_0_PLCKMOD_SHIFT, VR9_PCE_GCTRL_0_PLCKMOD_SIZE, &pPar->bMAC_SpoofingAction);
	}
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_MAC_PFSA_0_PFAD_OFFSET,	\
		VR9_MAC_PFSA_0_PFAD_SHIFT, VR9_MAC_PFSA_0_PFAD_SIZE, &value);

	pPar->nPauseMAC_Src[5] = value & 0xFF;
	pPar->nPauseMAC_Src[4] = (value >> 8) & 0xFF;
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_MAC_PFSA_1_PFAD_OFFSET,	\
		VR9_MAC_PFSA_1_PFAD_SHIFT, VR9_MAC_PFSA_1_PFAD_SIZE, &value);
	pPar->nPauseMAC_Src[3] = value & 0xFF;
	pPar->nPauseMAC_Src[2] = (value >> 8) & 0xFF;
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_MAC_PFSA_2_PFAD_OFFSET,	\
		VR9_MAC_PFSA_2_PFAD_SHIFT, VR9_MAC_PFSA_2_PFAD_SIZE, &value);
	pPar->nPauseMAC_Src[1] = value & 0xFF;
	pPar->nPauseMAC_Src[0] = (value >> 8) & 0xFF;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_CfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_cfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint32_t MANT = 0, EXP = 0, value, i;

	switch (pPar->eMAC_TableAgeTimer) {
		case IFX_ETHSW_AGETIMER_1_SEC:
			MANT = AGETIMER_1_SEC; EXP = 0x2;
			pEthSWDevHandle->MAC_AgeTimer = 1;
			break;
		case IFX_ETHSW_AGETIMER_10_SEC:
			MANT = AGETIMER_10_SEC; EXP = 0x2;
			pEthSWDevHandle->MAC_AgeTimer = 10;
			break;
		case IFX_ETHSW_AGETIMER_300_SEC:
			MANT = AGETIMER_300_SEC; EXP = 0x2;
			pEthSWDevHandle->MAC_AgeTimer = 300;
			break;
		case IFX_ETHSW_AGETIMER_1_HOUR:
			MANT = AGETIMER_1_HOUR; EXP = 0x6;
			pEthSWDevHandle->MAC_AgeTimer = 3600;
			break;
		case IFX_ETHSW_AGETIMER_1_DAY:
			MANT = AGETIMER_1_DAY; EXP = 0xA;
			pEthSWDevHandle->MAC_AgeTimer = 86400;
			break;
		default:
			MANT = AGETIMER_300_SEC; EXP = 0x2;
			pEthSWDevHandle->MAC_AgeTimer = 300;
	}

	/* Aging Counter Exponent Value */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_AGE_0_EXP_OFFSET,	\
		VR9_PCE_AGE_0_EXP_SHIFT, VR9_PCE_AGE_0_EXP_SIZE, EXP);
	/* Aging Counter Mantissa Value */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_AGE_1_MANT_OFFSET,	\
		VR9_PCE_AGE_1_MANT_SHIFT, VR9_PCE_AGE_1_MANT_SIZE, MANT);
	/* Maximum Ethernet packet length */
	if ( pPar->nMaxPacketLen < 0xFFFF )
		value = pPar->nMaxPacketLen;
	else
		value = IFX_ETHSW_MAX_PACKET_LENGTH;

	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_MAC_FLEN_LEN_OFFSET,	\
		VR9_MAC_FLEN_LEN_SHIFT, VR9_MAC_FLEN_LEN_SIZE, value);
	if (pPar->nMaxPacketLen > 0x5EE) {
		for (i = 0; i < 6; i++) {
			ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_MAC_CTRL_2_MLEN_OFFSET + (i * 0xC)),	\
				VR9_MAC_CTRL_2_MLEN_SHIFT, VR9_MAC_CTRL_2_MLEN_SIZE, 1);
		}
	}
	/* MAC Address Learning Limitation Mode */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_GCTRL_0_PLIMMOD_OFFSET ,	\
		VR9_PCE_GCTRL_0_PLIMMOD_SHIFT, VR9_PCE_GCTRL_0_PLIMMOD_SIZE, pPar->bLearningLimitAction);
	if ( pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2) {
	/* Supported for GSWIP 2.2 and newer and returns with an error for older hardware revisions. */
		/*Accept or discard MAC spoofing and port MAC locking violation packets */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_GCTRL_0_PLCKMOD_OFFSET ,	\
			VR9_PCE_GCTRL_0_PLCKMOD_SHIFT, VR9_PCE_GCTRL_0_PLCKMOD_SIZE, pPar->bMAC_SpoofingAction);
	}
	/* VLAN-aware Switching           */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_GCTRL_0_VLAN_OFFSET,	\
		VR9_PCE_GCTRL_0_VLAN_SHIFT, VR9_PCE_GCTRL_0_VLAN_SIZE, pPar->bVLAN_Aware);

	/* MAC Source Address Mode */
	if (pPar->bPauseMAC_ModeSrc == IFX_TRUE) {
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_MAC_PFAD_CFG_SAMOD_OFFSET,	\
			VR9_MAC_PFAD_CFG_SAMOD_SHIFT, VR9_MAC_PFAD_CFG_SAMOD_SIZE, pPar->bPauseMAC_ModeSrc);

		value = pPar->nPauseMAC_Src[4] << 8 | pPar->nPauseMAC_Src[5];

		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_MAC_PFSA_0_PFAD_OFFSET,	\
			VR9_MAC_PFSA_0_PFAD_SHIFT, VR9_MAC_PFSA_0_PFAD_SIZE, value);

		value = pPar->nPauseMAC_Src[2] << 8 | pPar->nPauseMAC_Src[3];

		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_MAC_PFSA_1_PFAD_OFFSET,	\
			VR9_MAC_PFSA_1_PFAD_SHIFT, VR9_MAC_PFSA_1_PFAD_SIZE, value);

		value = pPar->nPauseMAC_Src[0] << 8 | pPar->nPauseMAC_Src[1];

		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_MAC_PFSA_2_PFAD_OFFSET,	\
			VR9_MAC_PFSA_2_PFAD_SHIFT, VR9_MAC_PFSA_2_PFAD_SIZE, value);
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_HW_Init(IFX_void_t *pDevCtx, IFX_ETHSW_HW_Init_t *pPar)
{
	IFX_FLOW_switchDev_t *pDev = (IFX_FLOW_switchDev_t*)pDevCtx;
	int j;
	IFX_uint32_t reg_val;
/*
	if (pDev->bResetCalled == IFX_FALSE) {
		platform_device_reset_trigger();
		pDev->bResetCalled = IFX_TRUE;
	}
*/
	platform_device_init(pDev);
	pDev->bHW_InitCalled = IFX_TRUE;
	/* Software Table Init */
#if defined (CONFIG_LTQ_VLAN ) && CONFIG_LTQ_VLAN
	IFX_FLOW_VLAN_Table_Init(pDev);
#endif /*CONFIG_LTQ_VLAN */
	IFX_FLOW_PortCfg_Init(pDev);
#if defined (CONFIG_LTQ_MULTICAST ) && CONFIG_LTQ_MULTICAST
	IFX_FLOW_MULTICAST_SW_Table_Init(pDev);
#endif /*CONFIG_LTQ_MULTICAST*/
	ifx_pce_table_init(&pDev->PCE_Handler);
	/* HW Init */
	IFX_VR9_Switch_PCE_Micro_Code_Int(pDev);
	/* Configure the MDIO Clock 97.6 Khz */
	ifx_ethsw_ll_DirectAccessWrite(pDev, (VR9_MDC_CFG_1_FREQ_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_MDC_CFG_1_FREQ_SHIFT, VR9_MDC_CFG_1_FREQ_SIZE, 0xFF);
	for (j =0; j < pDev->nPortNumber-1; j++) {
		ifx_ethsw_ll_DirectAccessWrite(pDev, ((VR9_ANEG_EEE_0_CLK_STOP_CAPABLE_OFFSET+j) + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_ANEG_EEE_0_CLK_STOP_CAPABLE_SHIFT, VR9_ANEG_EEE_0_CLK_STOP_CAPABLE_SIZE, 0x3);
	}
	ifx_ethsw_ll_DirectAccessRead(pDev, VR9_ETHSW_CAP_10_MACBT_OFFSET,	\
		VR9_ETHSW_CAP_10_MACBT_SHIFT, VR9_ETHSW_CAP_10_MACBT_SIZE, &reg_val);
	 pDev->max_mac_bridge_table_size = reg_val;
	 ifx_ethsw_ll_DirectAccessRead(pDev, VR9_ETHSW_CAP_3_METERS_OFFSET,	\
				VR9_ETHSW_CAP_3_METERS_SHIFT, VR9_ETHSW_CAP_3_METERS_SIZE, &reg_val);
	 pDev->max_meter_table_size = reg_val;
	 ifx_ethsw_ll_DirectAccessRead(pDev, VR9_ETHSW_CAP_3_SHAPERS_OFFSET,	\
		VR9_ETHSW_CAP_3_SHAPERS_SHIFT, VR9_ETHSW_CAP_3_SHAPERS_SIZE, &reg_val);
	pDev->max_shapers_table_size = reg_val;
	
	
	ifx_ethsw_ll_DirectAccessRead(pDev, VR9_ETHSW_VERSION_REV_ID_OFFSET,	\
		VR9_ETHSW_VERSION_REV_ID_SHIFT, 16, &reg_val);
	pDev->GSWIP_Ver = reg_val;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_MDIO_CfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_MDIO_cfg_t *pPar)
{
	IFX_uint32_t value;
	/* Get MDC_CFG_1.FREQ value */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MDC_CFG_1_FREQ_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_MDC_CFG_1_FREQ_SHIFT, VR9_MDC_CFG_1_FREQ_SIZE, &value);
	pPar->nMDIO_Speed = value & 0xFF;
	/* Get MDC_CFG_1.MCEN value */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MDC_CFG_1_MCEN_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_MDC_CFG_1_MCEN_SHIFT, VR9_MDC_CFG_1_MCEN_SIZE, &value);
	pPar->bMDIO_Enable = value;
	if(value == 1)
		pPar->bMDIO_Enable = IFX_ENABLE;
	else
		pPar->bMDIO_Enable = IFX_DISABLE;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_MDIO_CfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_MDIO_cfg_t *pPar)
{
	IFX_uint32_t value;
	/* Set MDC_CFG_1.FREQ value */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_MDC_CFG_1_FREQ_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_MDC_CFG_1_FREQ_SHIFT, VR9_MDC_CFG_1_FREQ_SIZE, pPar->nMDIO_Speed);
	if (pPar->bMDIO_Enable)
		value = 0x3F;
	else
		value = 0;
	/* Set Auto-Polling of connected PHYs - For all ports */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_MDC_CFG_0_PEN_0_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_MDC_CFG_0_PEN_0_SHIFT, 6, value);
	/* Set MDC_CFG_1.MCEN value */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_MDC_CFG_1_MCEN_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_MDC_CFG_1_MCEN_SHIFT, VR9_MDC_CFG_1_MCEN_SIZE, pPar->bMDIO_Enable);
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_MDIO_DataRead(IFX_void_t *pDevCtx, IFX_ETHSW_MDIO_data_t *pPar)
{
	IFX_uint32_t value;
	value = 1;
	while (value == 1) {
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MDIO_CTRL_MBUSY_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_MDIO_CTRL_MBUSY_SHIFT, VR9_MDIO_CTRL_MBUSY_SIZE, &value);
	}
	value = ((0x2 << 10 ) | ((pPar->nAddressDev & 0x1F) << 5) | (pPar->nAddressReg & 0x1F));
	/* Special write command, becouse we need to write "MDIO Control Register" once at a time */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_MDIO_CTRL_MBUSY_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET), 0, 16, value);
	value = 1;
	while (value == 1) {
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MDIO_CTRL_MBUSY_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_MDIO_CTRL_MBUSY_SHIFT, VR9_MDIO_CTRL_MBUSY_SIZE, &value);
	}
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MDIO_READ_RDATA_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_MDIO_READ_RDATA_SHIFT, VR9_MDIO_READ_RDATA_SIZE, &value);
	pPar->nData = value & 0xFFFF;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_MDIO_DataWrite(IFX_void_t *pDevCtx, IFX_ETHSW_MDIO_data_t *pPar)
{
	IFX_uint32_t value;
	value = 1;
	while (value == 1) {
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MDIO_CTRL_MBUSY_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_MDIO_CTRL_MBUSY_SHIFT, VR9_MDIO_CTRL_MBUSY_SIZE, &value);
	}
	value = pPar->nData & 0xFFFF;
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_MDIO_WRITE_WDATA_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_MDIO_WRITE_WDATA_SHIFT, VR9_MDIO_WRITE_WDATA_SIZE, value);
	value = ((0x1 << 10 ) | ((pPar->nAddressDev & 0x1F) << 5) | (pPar->nAddressReg & 0x1F));
	/* Special write command, becouse we need to write "MDIO Control Register" once at a time */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_MDIO_CTRL_MBUSY_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET), 0, 16, value);
	value = 1;
	while (value == 1) {
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MDIO_CTRL_MBUSY_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_MDIO_CTRL_MBUSY_SHIFT, VR9_MDIO_CTRL_MBUSY_SIZE, &value);
	}
	return IFX_SUCCESS;
}
void inline ltq_mdelay( IFX_int32_t delay){
	IFX_int32_t i;
	for ( i=delay; i > 0; i--)
		udelay(1000);
}

static int force_to_configure_phy_settings(IFX_void_t *pDevCtx,IFX_uint8_t portIdx, IFX_uint8_t link_status )
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint32_t mdio_stat_reg, phy_addr_reg=0;
	if( portIdx >= (pEthSWDevHandle->nPortNumber-1) )
		return IFX_ERROR;
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, ( (VR9_PHY_ADDR_0_ADDR_OFFSET - portIdx) + IFX_ETHSW_FLOW_TOP_REG_OFFSET ),	\
		VR9_PHY_ADDR_0_ADDR_SHIFT, 16, &phy_addr_reg);
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MDIO_STAT_0_TXPAUEN_OFFSET+ IFX_ETHSW_FLOW_TOP_REG_OFFSET + portIdx),	\
		VR9_MDIO_STAT_0_TXPAUEN_SHIFT, 16, &mdio_stat_reg);
	if ( link_status ) {
		/* PHY active Status */
		if ( (mdio_stat_reg >> 6) & 0x1 ) {
			IFX_uint32_t temp=0;
			/* Link Status */
			if ( (mdio_stat_reg >> 5) & 0x1 ) {
				phy_addr_reg &= ~(0xFFE0);
				phy_addr_reg |= (1 << 13); /* Link up */
				temp = ( (mdio_stat_reg >> 3) & 0x3); /*Speed */
				phy_addr_reg |= (temp << 11); /*Speed */
				if( (mdio_stat_reg >> 2) & 0x1) /*duplex */ {
					phy_addr_reg |= ( 0x1 << 9); /*duplex */
				} else {
					phy_addr_reg |= ( 0x3 << 9); 
				}
				if( (mdio_stat_reg >> 1) & 0x1) /*Receive Pause Enable Status */ {
					phy_addr_reg |= ( 0x1 << 5); /*Receive Pause Enable Status */
				} else {
					phy_addr_reg |= ( 0x3 << 5); 
				}
				if( (mdio_stat_reg >> 0) & 0x1) /*Transmit Pause Enable Status */ {
					phy_addr_reg |= ( 0x1 << 7); /*Transmit Pause Enable Status */
				} else {
					phy_addr_reg |= ( 0x3 << 7); 
				}
				ifx_ethsw_ll_DirectAccessWrite(pDevCtx, ((VR9_PHY_ADDR_0_ADDR_OFFSET - portIdx) + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
					VR9_PHY_ADDR_0_ADDR_SHIFT, 16, phy_addr_reg);
			}
		}
	} else {
		phy_addr_reg &= ~(0xFFE0);
		phy_addr_reg |= (0x3 << 11 );
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, ((VR9_PHY_ADDR_0_ADDR_OFFSET - portIdx) + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_PHY_ADDR_0_ADDR_SHIFT, 16, phy_addr_reg);
	}
	return IFX_TRUE;
}
IFX_return_t IFX_FLOW_MmdDataRead(IFX_void_t *pDevCtx, IFX_ETHSW_MMD_data_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_ETHSW_MDIO_data_t mmd_data;
	IFX_uint32_t found = 0, phy_addr, mdc_reg,dev, portIdx;
	for ( portIdx = 0; portIdx < (pEthSWDevHandle->nPortNumber-1); portIdx++ ) {
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, ((VR9_PHY_ADDR_0_ADDR_OFFSET - portIdx) + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PHY_ADDR_0_ADDR_SHIFT, VR9_PHY_ADDR_0_ADDR_SIZE, &phy_addr);
		if ( phy_addr == pPar->nAddressDev ) {
			found = 1;
			break;
		}
	}
	if (found) {
		force_to_configure_phy_settings(pDevCtx,portIdx, 1);
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MDC_CFG_0_PEN_0_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
				VR9_MDC_CFG_0_PEN_0_SHIFT, 6, &mdc_reg);
		mdc_reg &= ~(1 << portIdx );
		dev = ( (pPar->nAddressReg >> 16 ) & 0x1F);
/*		IFXOS_PRINT_INT_RAW("%s:%s:%d, dev:0x%x, portidx:%d\n", __FILE__, __FUNCTION__, __LINE__,dev, portIdx); */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_MDC_CFG_0_PEN_0_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_MDC_CFG_0_PEN_0_SHIFT, 6, mdc_reg);
		ltq_mdelay(20);
		mmd_data.nAddressDev = pPar->nAddressDev;
		mmd_data.nAddressReg = 0xd;
		mmd_data.nData = dev;
		IFX_FLOW_MDIO_DataWrite(pEthSWDevHandle, &mmd_data);
		
		mmd_data.nAddressDev = pPar->nAddressDev;
		mmd_data.nAddressReg = 0xe;
		mmd_data.nData = pPar->nAddressReg & 0xFFFF;
		IFX_FLOW_MDIO_DataWrite(pEthSWDevHandle, &mmd_data);
		
		mmd_data.nAddressDev = pPar->nAddressDev;
		mmd_data.nAddressReg = 0xd;
		mmd_data.nData = ( (0x4000) | dev  );
		IFX_FLOW_MDIO_DataWrite(pEthSWDevHandle, &mmd_data);
		
		mmd_data.nAddressDev = pPar->nAddressDev;
		mmd_data.nAddressReg = 0xe;
		IFX_FLOW_MDIO_DataRead(pEthSWDevHandle, &mmd_data);
		pPar->nData = mmd_data.nData;
		
		mdc_reg |= (1 << portIdx );
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_MDC_CFG_0_PEN_0_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_MDC_CFG_0_PEN_0_SHIFT, 6, mdc_reg);
		ltq_mdelay(100);
		force_to_configure_phy_settings(pDevCtx, portIdx, 0);
		
	} else {
		IFXOS_PRINT_INT_RAW("Error: (Wrong PHY Address )  %s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
	}
    return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_MmdDataWrite(IFX_void_t *pDevCtx, IFX_ETHSW_MMD_data_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_ETHSW_MDIO_data_t mmd_data;
	IFX_uint32_t found = 0, phy_addr, mdc_reg,dev, portIdx;

	for ( portIdx = 0; portIdx < (pEthSWDevHandle->nPortNumber-1); portIdx++ ) {
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, ((VR9_PHY_ADDR_0_ADDR_OFFSET - portIdx) + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PHY_ADDR_0_ADDR_SHIFT, VR9_PHY_ADDR_0_ADDR_SIZE, &phy_addr);
		if ( phy_addr == pPar->nAddressDev ) {
			found = 1;
			break;
		}
	}
	if (found) {
		force_to_configure_phy_settings(pDevCtx,portIdx, 1);
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MDC_CFG_0_PEN_0_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
				VR9_MDC_CFG_0_PEN_0_SHIFT, 6, &mdc_reg);
		mdc_reg &= ~(1 << portIdx );
		dev = ( (pPar->nAddressReg >> 16 ) & 0x1F);
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_MDC_CFG_0_PEN_0_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_MDC_CFG_0_PEN_0_SHIFT, 6, mdc_reg);
		ltq_mdelay(20);
		mmd_data.nAddressDev = pPar->nAddressDev;
		mmd_data.nAddressReg = 0xd;
		mmd_data.nData = dev;
		IFX_FLOW_MDIO_DataWrite(pEthSWDevHandle, &mmd_data);
		
		mmd_data.nAddressDev = pPar->nAddressDev;
		mmd_data.nAddressReg = 0xe;
		mmd_data.nData = pPar->nAddressReg & 0xFFFF;
		IFX_FLOW_MDIO_DataWrite(pEthSWDevHandle, &mmd_data);
		
		mmd_data.nAddressDev = pPar->nAddressDev;
		mmd_data.nAddressReg = 0xd;
		mmd_data.nData = ( (0x4000) | dev  );
		IFX_FLOW_MDIO_DataWrite(pEthSWDevHandle, &mmd_data);
		
		mmd_data.nAddressDev = pPar->nAddressDev;
		mmd_data.nAddressReg = 0xe;
		mmd_data.nData = pPar->nData;
		IFX_FLOW_MDIO_DataWrite(pEthSWDevHandle, &mmd_data);
		
		mdc_reg |= (1 << portIdx );
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_MDC_CFG_0_PEN_0_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_MDC_CFG_0_PEN_0_SHIFT, 6, mdc_reg);
		ltq_mdelay(100);
		force_to_configure_phy_settings(pDevCtx, portIdx, 0);

	} else {
		IFXOS_PRINT_INT_RAW("Error: (Wrong PHY Address )  %s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
	}
    return IFX_SUCCESS;
}

static IFX_return_t IFX_FLOW_MMD_MDIO_DataWrite(IFX_void_t *pDevCtx, IFX_ETHSW_MDIO_data_t *pPar,IFX_uint8_t portIdx,IFX_uint8_t dev)
{
	IFX_ETHSW_MDIO_data_t mmd_data;
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint32_t mdc_reg;
	
	if( portIdx >= (pEthSWDevHandle->nPortNumber-1) )
		return IFX_ERROR;
	force_to_configure_phy_settings(pDevCtx,portIdx, 1);
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MDC_CFG_0_PEN_0_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_MDC_CFG_0_PEN_0_SHIFT, 6, &mdc_reg);

	mdc_reg &= ~(1 << portIdx );
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_MDC_CFG_0_PEN_0_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_MDC_CFG_0_PEN_0_SHIFT, 6, mdc_reg);
	ltq_mdelay(20);

	mmd_data.nAddressDev = pPar->nAddressDev;
	mmd_data.nAddressReg = 0xd;
	mmd_data.nData = dev;
	IFX_FLOW_MDIO_DataWrite(pEthSWDevHandle, &mmd_data);
	
	mmd_data.nAddressDev = pPar->nAddressDev;
	mmd_data.nAddressReg = 0xe;
	mmd_data.nData = pPar->nAddressReg;
	IFX_FLOW_MDIO_DataWrite(pEthSWDevHandle, &mmd_data);
	
	mmd_data.nAddressDev = pPar->nAddressDev;
	mmd_data.nAddressReg = 0xd;
	mmd_data.nData = ( (0x4000) | (dev & 0x1F) );
	IFX_FLOW_MDIO_DataWrite(pEthSWDevHandle, &mmd_data);
	
	mmd_data.nAddressDev = pPar->nAddressDev;
	mmd_data.nAddressReg = 0xe;
	mmd_data.nData = pPar->nData;
	IFX_FLOW_MDIO_DataWrite(pEthSWDevHandle, &mmd_data);
	
	mdc_reg |= (1 << portIdx );
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_MDC_CFG_0_PEN_0_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_MDC_CFG_0_PEN_0_SHIFT, 6, mdc_reg);
	ltq_mdelay(100);
	force_to_configure_phy_settings(pDevCtx,portIdx, 0);
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_MonitorPortCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_monitorPortCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint8_t portIdx = pPar->nPortId;
	IFX_uint32_t value;

	if( pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber)
		return IFX_ERROR;
	/* Get PCE Port Map 1 */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_PMAP_1_MPMAP_OFFSET ,	\
		VR9_PCE_PMAP_1_MPMAP_SHIFT, VR9_PCE_PMAP_1_MPMAP_SIZE, &value);
	if ((( value & (1 << portIdx)) >> portIdx) == 1)
		pPar->bMonitorPort = IFX_TRUE;
	else
		pPar->bMonitorPort = IFX_FALSE;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_MonitorPortCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_monitorPortCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint8_t portIdx = pPar->nPortId;
	IFX_uint32_t value;

	if( pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber)
		return IFX_ERROR;
	/* Get PCE Port Map 1 */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_PMAP_1_MPMAP_OFFSET ,	\
		VR9_PCE_PMAP_1_MPMAP_SHIFT, VR9_PCE_PMAP_1_MPMAP_SIZE, &value);
	if (pPar->bMonitorPort == 1)
		value |= (pPar->bMonitorPort << portIdx);
	else
		value = (value & ~(1 << portIdx));
	/* Set PCE Port Map 1 */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PMAP_1_MPMAP_OFFSET ,	\
		VR9_PCE_PMAP_1_MPMAP_SHIFT, VR9_PCE_PMAP_1_MPMAP_SIZE, value);
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_PortLinkCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_portLinkCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint32_t value;
	IFX_uint8_t portIdx = pPar->nPortId;

	if( pPar->nPortId >= (pEthSWDevHandle->nPortNumber-1) )
		return IFX_ERROR;
	/* Duplex */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MAC_PSTAT_FDUP_OFFSET + (0xC * portIdx)),	\
		VR9_MAC_PSTAT_FDUP_SHIFT, VR9_MAC_PSTAT_FDUP_SIZE , &value);
	if (value) {
		pPar->eDuplex = IFX_ETHSW_DUPLEX_FULL;
	} else {
		pPar->eDuplex = IFX_ETHSW_DUPLEX_HALF;
	}
   /* Speed */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MAC_PSTAT_GBIT_OFFSET + (0xC * portIdx)),	\
		VR9_MAC_PSTAT_GBIT_SHIFT, VR9_MAC_PSTAT_GBIT_SIZE , &value);
	if (value) {
		pPar->eSpeed = IFX_ETHSW_PORT_SPEED_1000;
	} else {
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MAC_PSTAT_MBIT_OFFSET + (0xC * portIdx)),	\
		VR9_MAC_PSTAT_MBIT_SHIFT, VR9_MAC_PSTAT_MBIT_SIZE , &value);
		if (value) {
			pPar->eSpeed = IFX_ETHSW_PORT_SPEED_100;
		} else {
			pPar->eSpeed = IFX_ETHSW_PORT_SPEED_10;
		}
	}
	/* Low-power Idle Mode  configuration                                */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MAC_CTRL_4_LPIEN_OFFSET + ( 0xC * portIdx)),	\
			VR9_MAC_CTRL_4_LPIEN_SHIFT, VR9_MAC_CTRL_4_LPIEN_SIZE, &pPar->bLPI);
	/* State */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MAC_PSTAT_LSTAT_OFFSET + (0xC * portIdx)),	\
		VR9_MAC_PSTAT_LSTAT_SHIFT, VR9_MAC_PSTAT_LSTAT_SIZE , &value);
	if (value) {
		pPar->eLink = IFX_ETHSW_PORT_LINK_UP;
	} else {
		pPar->eLink = IFX_ETHSW_PORT_LINK_DOWN;
	}
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MII_CFG_0_MIIMODE_OFFSET + (0x2 * portIdx) + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_MII_CFG_0_MIIMODE_SHIFT, VR9_MII_CFG_0_MIIMODE_SIZE, &value);

	switch (value) {
		case 0:
			pPar->eMII_Mode = IFX_ETHSW_PORT_HW_MII;
			pPar->eMII_Type = IFX_ETHSW_PORT_PHY;
			break;
		case 1:
			pPar->eMII_Mode = IFX_ETHSW_PORT_HW_MII ;
			pPar->eMII_Type = IFX_ETHSW_PORT_MAC;
			break;
		case 2:
			pPar->eMII_Mode = IFX_ETHSW_PORT_HW_RMII;
			pPar->eMII_Type = IFX_ETHSW_PORT_PHY;
			break;
		case 3:
			pPar->eMII_Mode = IFX_ETHSW_PORT_HW_RMII;
			pPar->eMII_Type = IFX_ETHSW_PORT_MAC;
			break;
		case 4:
			pPar->eMII_Mode = IFX_ETHSW_PORT_HW_RGMII;
			pPar->eMII_Type = IFX_ETHSW_PORT_MAC;
			break;
	}
	pPar->eClkMode = IFX_ETHSW_PORT_CLK_NA;
	if (pPar->eMII_Mode == IFX_ETHSW_PORT_HW_RMII ) {
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MII_CFG_0_RMII_OFFSET + (0x2 * portIdx) + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_MII_CFG_0_RMII_SHIFT, VR9_MII_CFG_0_RMII_SIZE, &value);
		if (value == 1)
			pPar->eClkMode = IFX_ETHSW_PORT_CLK_MASTER;
		else
			pPar->eClkMode = IFX_ETHSW_PORT_CLK_SLAVE;
	}
	return IFX_SUCCESS;
}	

IFX_return_t IFX_FLOW_PortLinkCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_portLinkCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint32_t data = DUPLEX_AUTO,phy_addr=0,phy_ctrl=0, duplex=DUPLEX_AUTO;
	IFX_uint8_t portIdx = pPar->nPortId;
	IFX_ETHSW_MDIO_data_t mdio_data;

	if( pPar->nPortId > (pEthSWDevHandle->nPortNumber-1) )
		return IFX_ERROR;

	/* ---------------------------------------------------------------- */
	/* Duplex Mode configuration                                        */
	/* ---------------------------------------------------------------- */
	if (pPar->bDuplexForce == IFX_TRUE) {
		if( pPar->eDuplex == IFX_ETHSW_DUPLEX_FULL) {
			data = DUPLEX_EN;
			duplex = DUPLEX_EN;
		} else {
			data = DUPLEX_DIS;
			duplex = DUPLEX_DIS;
		}
	} else {
		data = DUPLEX_AUTO;
	}
/*	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PHY_ADDR_0_FDUP_OFFSET - (0x1 * portIdx) + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PHY_ADDR_0_FDUP_SHIFT, VR9_PHY_ADDR_0_FDUP_SIZE, data); */
	/* ---------------------------------------------------------------- */

	/* ---------------------------------------------------------------- */
	/* Speed configuration                                              */
	/* ---------------------------------------------------------------- */
	data = 3; /*default value*/
	if (pPar->bSpeedForce == IFX_TRUE ) {
		switch(pPar->eSpeed) {
			case IFX_ETHSW_PORT_SPEED_10:
				data = 0;
				if (duplex == DUPLEX_DIS )
					phy_ctrl = PHY_AN_ADV_10HDX;
				else
					phy_ctrl = PHY_AN_ADV_10FDX;
				break;
			case IFX_ETHSW_PORT_SPEED_100:
				data = 1;
				if (duplex == DUPLEX_DIS )
					phy_ctrl = PHY_AN_ADV_100HDX;
				else
					phy_ctrl = PHY_AN_ADV_100FDX;
				break;
			case IFX_ETHSW_PORT_SPEED_200:
				IFXOS_PRINT_INT_RAW("Error: (GSWITCH not supported) %s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__);
				return IFX_ERROR;
//				break;
			case IFX_ETHSW_PORT_SPEED_1000:
				data = 2;
				if (duplex == DUPLEX_DIS )
					phy_ctrl = PHY_AN_ADV_1000HDX;
				else
					phy_ctrl = PHY_AN_ADV_1000FDX;
				break;
		}
	}
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, ((VR9_PHY_ADDR_0_ADDR_OFFSET - portIdx) + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PHY_ADDR_0_ADDR_SHIFT, VR9_PHY_ADDR_0_ADDR_SIZE, &phy_addr);
	mdio_data.nAddressDev = phy_addr;
	IFX_FLOW_MDIO_DataRead(pEthSWDevHandle, &mdio_data);
	if ( (data == 0) || (data == 1) ) {
		mdio_data.nAddressReg = 4;
		IFX_FLOW_MDIO_DataRead(pEthSWDevHandle, &mdio_data);
		mdio_data.nData &= ~(PHY_AN_ADV_10HDX | PHY_AN_ADV_10FDX |PHY_AN_ADV_100HDX | PHY_AN_ADV_100FDX );
		mdio_data.nData |= phy_ctrl;
		IFX_FLOW_MDIO_DataWrite(pEthSWDevHandle, &mdio_data);
		mdio_data.nAddressReg = 9;
		IFX_FLOW_MDIO_DataRead(pEthSWDevHandle, &mdio_data);
		mdio_data.nData &= ~(PHY_AN_ADV_1000HDX | PHY_AN_ADV_1000FDX);
		IFX_FLOW_MDIO_DataWrite(pEthSWDevHandle, &mdio_data);
	}
	if (data == 2 ) {
		mdio_data.nAddressReg = 9;
		IFX_FLOW_MDIO_DataRead(pEthSWDevHandle, &mdio_data);
		mdio_data.nData &= ~(PHY_AN_ADV_1000HDX | PHY_AN_ADV_1000FDX);
		mdio_data.nData |= phy_ctrl;
		IFX_FLOW_MDIO_DataWrite(pEthSWDevHandle, &mdio_data);
		mdio_data.nAddressReg = 4;
		IFX_FLOW_MDIO_DataRead(pEthSWDevHandle, &mdio_data);
		mdio_data.nData &= ~(PHY_AN_ADV_10HDX | PHY_AN_ADV_10FDX |PHY_AN_ADV_100HDX | PHY_AN_ADV_100FDX );
		IFX_FLOW_MDIO_DataWrite(pEthSWDevHandle, &mdio_data);
	}
	if (data == 3) {
		mdio_data.nAddressReg = 4;
		IFX_FLOW_MDIO_DataRead(pEthSWDevHandle, &mdio_data);
		if (duplex == DUPLEX_DIS) {
			mdio_data.nData &=~(PHY_AN_ADV_10HDX | PHY_AN_ADV_10FDX |PHY_AN_ADV_100HDX | PHY_AN_ADV_100FDX );
			mdio_data.nData |=(PHY_AN_ADV_10HDX | PHY_AN_ADV_100HDX );
		} else {
			mdio_data.nData |=(PHY_AN_ADV_10HDX | PHY_AN_ADV_10FDX |PHY_AN_ADV_100HDX | PHY_AN_ADV_100FDX );
		}
		IFX_FLOW_MDIO_DataWrite(pEthSWDevHandle, &mdio_data);
		
		mdio_data.nAddressReg = 9;
		IFX_FLOW_MDIO_DataRead(pEthSWDevHandle, &mdio_data);
		if (duplex == DUPLEX_DIS) {
			mdio_data.nData &=~(PHY_AN_ADV_1000HDX | PHY_AN_ADV_1000FDX  );
			mdio_data.nData |=(PHY_AN_ADV_1000HDX);
		} else {
			mdio_data.nData |=(PHY_AN_ADV_1000HDX | PHY_AN_ADV_1000FDX );
		}
		IFX_FLOW_MDIO_DataWrite(pEthSWDevHandle, &mdio_data);
	}

/*	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PHY_ADDR_0_SPEED_OFFSET - (0x1 * portIdx) + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PHY_ADDR_0_SPEED_SHIFT, VR9_PHY_ADDR_0_SPEED_SIZE, data); */
	/* ---------------------------------------------------------------- */
	/* Low-power Idle Mode  configuration                                */
	/* ---------------------------------------------------------------- */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_MAC_CTRL_4_LPIEN_OFFSET + ( 0xC * portIdx)),	\
		VR9_MAC_CTRL_4_LPIEN_SHIFT, VR9_MAC_CTRL_4_LPIEN_SIZE, pPar->bLPI);
	/* LPI Wait Time for 1G -- 50us */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_MAC_CTRL_4_GWAIT_OFFSET + ( 0xC * portIdx)),	\
		VR9_MAC_CTRL_4_GWAIT_SHIFT, VR9_MAC_CTRL_4_GWAIT_SIZE, 0x32);
	/* LPI Wait Time for 100M -- 21us */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_MAC_CTRL_4_WAIT_OFFSET + ( 0xC * portIdx)),	\
		VR9_MAC_CTRL_4_WAIT_SHIFT, VR9_MAC_CTRL_4_WAIT_SIZE, 0x15);
		/* LPI request controlled by data available for  port */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_FDMA_CTRL_LPI_MODE_OFFSET ),	\
		VR9_FDMA_CTRL_LPI_MODE_SHIFT, VR9_FDMA_CTRL_LPI_MODE_SIZE, 0x4);
	mdio_data.nAddressDev = phy_addr;
	mdio_data.nAddressReg = 0x3C;
	if ( pPar->bLPI == IFX_TRUE) {
		mdio_data.nData = 0x6;
		IFX_FLOW_MMD_MDIO_DataWrite(pEthSWDevHandle, &mdio_data,portIdx,0x07);
	} else {
		mdio_data.nData = 0x0;
		IFX_FLOW_MMD_MDIO_DataWrite(pEthSWDevHandle, &mdio_data,portIdx,0x07);
	}
	/* ---------------------------------------------------------------- */
	
	/* ---------------------------------------------------------------- */
	/* Link Status Control configuration                                */
	/* ---------------------------------------------------------------- */
	/* Specify PHY Address */
/*	ifx_ethsw_ll_DirectAccessRead(pDevCtx, ((VR9_PHY_ADDR_0_ADDR_OFFSET - portIdx) + IFX_ETHSW_FLOW_TOP_REG_OFFSET ),	\
		VR9_PHY_ADDR_0_ADDR_SHIFT, VR9_PHY_ADDR_0_ADDR_SIZE, &phy_addr); */
	mdio_data.nAddressDev = phy_addr;
	mdio_data.nAddressReg = 0;
	IFX_FLOW_MDIO_DataRead(pEthSWDevHandle, &mdio_data);
	mdio_data.nData = 0x1200;
	data = 0;
	if(pPar->bLinkForce == IFX_TRUE ) {
		if (pPar->eLink == IFX_ETHSW_PORT_LINK_UP ) {
			data = 1;
		} else {
			data = 2;
			mdio_data.nData =0x800;
		}
#ifdef IFX_ETHSW_API_COC
		IFX_ETHSW_PM_linkForceSet(pEthSWDevHandle->pPMCtx, portIdx, IFX_TRUE);
#endif
	}
	IFX_FLOW_MDIO_DataWrite(pEthSWDevHandle, &mdio_data);
	/* MAC */
/*	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PHY_ADDR_0_LNKST_OFFSET - (0x1 * portIdx) + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PHY_ADDR_0_LNKST_SHIFT, VR9_PHY_ADDR_0_LNKST_SIZE, data); */
	/* ---------------------------------------------------------------- */

	/* ---------------------------------------------------------------- */
	/* RGMII/GMII/MII configuration                                     */
	/* ---------------------------------------------------------------- */
	data = 4 /*IFX_ETHSW_PORT_HW_RGMII */; /*default mode */
	switch(pPar->eMII_Mode) {
		case IFX_ETHSW_PORT_HW_MII:
			data = 1;
			if (pPar->eMII_Type == IFX_ETHSW_PORT_PHY )
				data = 0;
			break;
		case IFX_ETHSW_PORT_HW_RMII:
			data = 3;
			if (pPar->eMII_Type == IFX_ETHSW_PORT_PHY )
				data = 2;
			break;
		case IFX_ETHSW_PORT_HW_RGMII:
			data = 4;
			break;
		case IFX_ETHSW_PORT_HW_GMII:
/*			data = 1; */
			break;
	}
	if (! (pPar->eMII_Mode == IFX_ETHSW_PORT_HW_GMII) )
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_MII_CFG_0_MIIMODE_OFFSET + (0x2 * portIdx) +	\
			IFX_ETHSW_FLOW_TOP_REG_OFFSET),	VR9_MII_CFG_0_MIIMODE_SHIFT, VR9_MII_CFG_0_MIIMODE_SIZE, data);
	data = 0;
	if (pPar->eMII_Mode == IFX_ETHSW_PORT_HW_RMII ) {
		if (pPar->eClkMode == IFX_ETHSW_PORT_CLK_MASTER) {
			data = 1;
		}
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_MII_CFG_0_RMII_OFFSET+ (0x2 * portIdx) + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_MII_CFG_0_RMII_SHIFT, VR9_MII_CFG_0_RMII_SIZE, data);
	}
	/* ---------------------------------------------------------------- */

	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_PortPHY_AddrGet(IFX_void_t *pDevCtx, IFX_ETHSW_portPHY_Addr_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint8_t portIdx = pPar->nPortId;
	IFX_uint32_t value;

	if( pPar->nPortId >= ( pEthSWDevHandle->nPortNumber - 1) )
		return IFX_ERROR;

	ifx_ethsw_ll_DirectAccessRead(pDevCtx, ((VR9_PHY_ADDR_0_ADDR_OFFSET - portIdx) + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PHY_ADDR_0_ADDR_SHIFT, VR9_PHY_ADDR_0_ADDR_SIZE, &value);
	pPar->nAddressDev = value;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_PortPHY_Query(IFX_void_t *pDevCtx, IFX_ETHSW_portPHY_Query_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint8_t portIdx = pPar->nPortId;
	IFX_uint32_t value;

	if( pPar->nPortId >= ( pEthSWDevHandle->nPortNumber - 1 ) )
		return IFX_ERROR;

	/* Read out the MAC_PSTAT.PACT */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MAC_PSTAT_PACT_OFFSET + (0xC * portIdx)),	\
		VR9_MAC_PSTAT_PACT_SHIFT, VR9_MAC_PSTAT_PACT_SIZE , &value);
	pPar->bPHY_Present = value;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_PortRGMII_ClkCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_portRGMII_ClkCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint32_t value;

	if( pPar->nPortId >= (pEthSWDevHandle->nPortNumber-1) )
		return IFX_ERROR;

	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCDU_0_RXDLY_OFFSET + (0x2 * pPar->nPortId) + IFX_ETHSW_FLOW_TOP_REG_OFFSET,	\
		VR9_PCDU_0_RXDLY_SHIFT, VR9_PCDU_0_RXDLY_SIZE, &value);
	IFXOS_PRINT_INT_RAW("nDelayRx = %d\n",value);
	pPar->nDelayRx = value;
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCDU_0_TXDLY_OFFSET + (0x2 * pPar->nPortId) + IFX_ETHSW_FLOW_TOP_REG_OFFSET,	\
		VR9_PCDU_0_TXDLY_SHIFT, VR9_PCDU_0_TXDLY_SIZE, &value);
	IFXOS_PRINT_INT_RAW("nDelayTx = %d\n",value);
	pPar->nDelayTx = value;

	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_PortRGMII_ClkCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_portRGMII_ClkCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;

	if( pPar->nPortId >= (pEthSWDevHandle->nPortNumber - 1) )
		return IFX_ERROR;

	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PCDU_0_RXDLY_OFFSET + (0x2 * pPar->nPortId) + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PCDU_0_RXDLY_SHIFT, VR9_PCDU_0_RXDLY_SIZE, pPar->nDelayRx);

	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PCDU_0_TXDLY_OFFSET + (0x2 * pPar->nPortId) +IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PCDU_0_TXDLY_SHIFT, VR9_PCDU_0_TXDLY_SIZE, pPar->nDelayTx);

	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_PortRedirectGet(IFX_void_t *pDevCtx, IFX_ETHSW_portRedirectCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint8_t portIdx = pPar->nPortId;
	IFX_uint32_t value;

	if( pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber )
		return IFX_ERROR;
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PCE_PCTRL_3_EDIR_OFFSET + (0xA * portIdx)),	\
		VR9_PCE_PCTRL_3_EDIR_SHIFT, VR9_PCE_PCTRL_3_EDIR_SIZE, &value);
	pPar->bRedirectEgress = value;
	if ( PortRedirectFlag > 0 )
		pPar->bRedirectIngress = IFX_TRUE;
	else
		pPar->bRedirectIngress = IFX_FALSE;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_PortRedirectSet(IFX_void_t *pDevCtx, IFX_ETHSW_portRedirectCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_FLOW_PCE_rule_t PCE_rule;
	IFX_uint8_t portIdx = pPar->nPortId;
	IFX_uint32_t RedirectPort = 0, value = 0, i;

	if( pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber )
		return IFX_ERROR;
	/* The Redirect port need to set as monitor port */
	/* Get PCE Port Map 1 */
	//   ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_PCE_PMAP_1_MPMAP_OFFSET ,
	//		VR9_PCE_PMAP_1_MPMAP_SHIFT, VR9_PCE_PMAP_1_MPMAP_SIZE, &value);
	value |= (1 << pEthSWDevHandle->nManagementPortNumber);
	RedirectPort = value;
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_PCE_PMAP_1_MPMAP_OFFSET ,	\
		VR9_PCE_PMAP_1_MPMAP_SHIFT, VR9_PCE_PMAP_1_MPMAP_SIZE, value);
	value = pPar->bRedirectEgress;
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PCE_PCTRL_3_EDIR_OFFSET + (0xA * portIdx)),	\
		VR9_PCE_PCTRL_3_EDIR_SHIFT, VR9_PCE_PCTRL_3_EDIR_SIZE, value);

	if ( pPar->bRedirectIngress == IFX_TRUE)
		PortRedirectFlag |= (1 << pPar->nPortId);
	else
		PortRedirectFlag &= ~(1 << pPar->nPortId) ;

	for (i = 0; i < pEthSWDevHandle->nPortNumber; i++) {
		if ((( PortRedirectFlag >> i ) & 0x1 )== 1) {
			memset(&PCE_rule, 0 , sizeof(IFX_FLOW_PCE_rule_t));
			PCE_rule.pattern.nIndex = (30 + i);
			PCE_rule.pattern.bEnable = IFX_TRUE;
			PCE_rule.pattern.bPortIdEnable = IFX_TRUE;
			PCE_rule.pattern.nPortId = i;
			if (pPar->bRedirectIngress == IFX_TRUE)
				PCE_rule.action.ePortMapAction = IFX_FLOW_PCE_ACTION_PORTMAP_ALTERNATIVE;
			PCE_rule.action.nForwardPortMap = RedirectPort;
			/* We prepare everything and write into PCE Table */
			if (0 != ifx_pce_rule_write(&pEthSWDevHandle->PCE_Handler,&PCE_rule))
				return IFX_ERROR;
		}  else {
			if (0 != ifx_pce_pattern_delete(&pEthSWDevHandle->PCE_Handler, 30+i))
				return IFX_ERROR;
		}
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_RMON_Clear(IFX_void_t *pDevCtx, IFX_ETHSW_RMON_clear_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint8_t portIdx = pPar->nPortId;

	if( pPar->nPortId >= pEthSWDevHandle->nPortNumber )
		return IFX_ERROR;

	/* Reset all RMON counter */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RMON_CTRL_RAM1_RES_OFFSET + (portIdx * 2),	\
		VR9_BM_RMON_CTRL_RAM1_RES_SHIFT, VR9_BM_RMON_CTRL_RAM1_RES_SIZE, 0x1);
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RMON_CTRL_RAM2_RES_OFFSET + (portIdx * 2),	\
		VR9_BM_RMON_CTRL_RAM2_RES_SHIFT, VR9_BM_RMON_CTRL_RAM2_RES_SIZE, 0x1);
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_RMON_Get(IFX_void_t *pDevCtx, IFX_ETHSW_RMON_cnt_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint8_t portIdx = pPar->nPortId, i;
	IFX_uint32_t  data, data0, data1, bcast_cnt, value;
	IFX_uint32_t  r_frame = 0, r_unicast = 0, r_multicast = 0;
	IFX_uint32_t  t_frame = 0, t_unicast = 0, t_multicast = 0;
	IFX_uint32_t  R_GoodByteCount_L = 0,  R_BadByteCount_L = 0, T_GoodByteCount_L = 0;
	IFX_uint64_t  R_GoodByteCount_H = 0,  R_BadByteCount_H = 0, T_GoodByteCount_H = 0;
	if (pEthSWDevHandle) {
		if( pPar->nPortId >= pEthSWDevHandle->nPortNumber )
			return IFX_ERROR;
	}
//	if( pPar->nPortId >= pEthSWDevHandle->nPortNumber )
	//	return IFX_ERROR;

	/* Enable the RMON Counter  */
/*	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_PCFG_CNTEN_OFFSET + (portIdx * 2),	\
		VR9_BM_PCFG_CNTEN_SHIFT, VR9_BM_PCFG_CNTEN_SIZE, 1); */
	memset(pPar, 0 , sizeof(IFX_ETHSW_RMON_cnt_t));
	pPar->nPortId = portIdx;
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, GSWIP_BM_RMON_CTRL_BCAST_CNT_OFFSET + (portIdx * 2),	\
		GSWIP_BM_RMON_CTRL_BCAST_CNT_SHIFT, GSWIP_BM_RMON_CTRL_BCAST_CNT_SIZE, &bcast_cnt);
	
	for (i = 0; i < IFX_ETHSW_RMON_COUNTER_OFFSET; i++) {
		/* Set the BM RAM ADDR  */
		/* Receive Frame Count  */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_ADDR_ADDR_OFFSET,	\
			VR9_BM_RAM_ADDR_ADDR_SHIFT, VR9_BM_RAM_ADDR_ADDR_SIZE, i);
		/* Specify the port ID */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_ADDR_OFFSET,	\
			VR9_BM_RAM_CTRL_ADDR_SHIFT, VR9_BM_RAM_CTRL_ADDR_SIZE, portIdx);
		/* Table Access Operation Mode Read */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_OPMOD_OFFSET,	\
			VR9_BM_RAM_CTRL_OPMOD_SHIFT, VR9_BM_RAM_CTRL_OPMOD_SIZE, 0x0);
		value = 1;
		/* Active */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_BAS_OFFSET,	\
			VR9_BM_RAM_CTRL_BAS_SHIFT, VR9_BM_RAM_CTRL_BAS_SIZE, value);
		do {
			ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_RAM_CTRL_BAS_OFFSET,	\
				VR9_BM_RAM_CTRL_BAS_SHIFT, VR9_BM_RAM_CTRL_BAS_SIZE, &value);
		} while(value);
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_RAM_VAL_0_VAL0_OFFSET,	\
			VR9_BM_RAM_VAL_0_VAL0_SHIFT, VR9_BM_RAM_VAL_0_VAL0_SIZE, &data0);
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_RAM_VAL_1_VAL1_OFFSET,	\
			VR9_BM_RAM_VAL_1_VAL1_SHIFT, VR9_BM_RAM_VAL_1_VAL1_SIZE, &data1);
		//    IFXOS_PRINT_INT_RAW("The Data we get is = %x\n",(data1 << 16 | data0));
		data = (data1 << 16 | data0);
		switch (i) {
			case 0x1F:
				/* Receive Frme Count */
				if (bcast_cnt == 1 /*pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2*/  ) {
					pPar->nRxBroadcastPkts = data;
				} else {
					pPar->nRxGoodPkts = data;
				}
				r_frame = data;
				break;
			case 0x23:
				/* Receive Unicast Frame Count */
				pPar->nRxUnicastPkts = data;
				r_unicast = data;
				break;
			case 0x22:
				/* Receive Multicast Frame Count1 */
				pPar->nRxMulticastPkts = data;
				r_multicast = data;
				break;
			case 0x21:
				/* Receive CRC Errors Count */
				pPar->nRxFCSErrorPkts = data;
				break;
			case 0x1D:
				/* Receive Undersize Good Count */
				pPar->nRxUnderSizeGoodPkts = data;
				break;
			case 0x1B:
				/* Receive Oversize Good Count */
				pPar->nRxOversizeGoodPkts = data;
				break;
			case 0x1E:
				/* Receive Undersize Bad Count */
				pPar->nRxUnderSizeErrorPkts = data;
				break;
			case 0x20:
				/* Receive Pause Good Count */
				pPar->nRxGoodPausePkts = data;
				break;
			case 0x1C:
				/* Receive Oversize Bad Count */
				pPar->nRxOversizeErrorPkts = data;
				break;
			case 0x1A:
				/* Receive Alignment Errors Count */
				pPar->nRxAlignErrorPkts = data;
				break;
			case 0x12:
				/* Receive Size 64 Frame Count1 */
				pPar->nRx64BytePkts = data;
				break;
			case 0x13:
				/* Receive Size 65-127 Frame Count */
				pPar->nRx127BytePkts = data;
				break;
			case 0x14:
				/* Receive Size 128-255 Frame Count */
				pPar->nRx255BytePkts = data;
				break;
			case 0x15:
				/* Receive Size 256-511 Frame Count */
				pPar->nRx511BytePkts = data;
	            break;
			case 0x16:
				/* Receive Size 512-1023 Frame Count */
				pPar->nRx1023BytePkts = data;
				break;
			case 0x17:
				/* Receive Size Greater 1023 Frame Count */
				pPar->nRxMaxBytePkts = data;
				break;
			case 0x18:
				/* Receive Discard (Tail-Drop) Frame Count */
				pPar->nRxDroppedPkts = data;
				break;
			case 0x19:
				/* Receive Drop (Filter) Frame Count */
				pPar->nRxFilteredPkts = data;
				break;
			case 0x24:
				/* Receive Good Byte Count (Low) */
				R_GoodByteCount_L = data;
				break;
			case 0x25:
				/* Receive Good Byte Count (High) */
				R_GoodByteCount_H = data;
				break;
			case 0x26:
				/* Receive Bad Byte Count (Low) */
				R_BadByteCount_L = data;
				break;
			case 0x27:
	            /* Receive Bad Byte Count (High) */
	            R_BadByteCount_H = data;
	            break;
		/* =================== */
			case 0x0C:
				/* Transmit Frame Count */
				if (bcast_cnt == 1 /*pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2*/  ) {
					pPar->nTxBroadcastPkts = data;
				} else {
					pPar->nTxGoodPkts = data;
				}
				t_frame = data;
				break;
			case 0x06:
				/* Transmit Unicast Frame Count */
				pPar->nTxUnicastPkts = data;
				t_unicast = data;
				break;
			case 0x07:
				/* Transmit Multicast Frame Count1 */
				pPar->nTxMulticastPkts = data;
				t_multicast = data;
				break;
			case 0x00:
				/* Transmit Size 64 Frame Count */
				pPar->nTx64BytePkts = data;
				break;
			case 0x01:
				/* Transmit Size 65-127 Frame Count */
				pPar->nTx127BytePkts = data;
				break;
			case 0x02:
				/* Transmit Size 128-255 Frame Count */
				pPar->nTx255BytePkts = data;
				break;
			case 0x03:
				/* Transmit Size 256-511 Frame Count */
				pPar->nTx511BytePkts = data;
			break;
			case 0x04:
				/* Transmit Size 512-1023 Frame Count */
				pPar->nTx1023BytePkts = data;
				break;
			case 0x05:
				/* Transmit Size Greater 1024 Frame Count */
				pPar->nTxMaxBytePkts = data;
				break;
			case 0x08:
				/* Transmit Single Collision Count. */
				pPar->nTxSingleCollCount = data;
				break;
			case 0x09:
				/* Transmit Multiple Collision Count */
				pPar->nTxMultCollCount = data;
			break;
			case 0x0A:
				/* Transmit Late Collision Count */
				pPar->nTxLateCollCount = data;
				break;
			case 0x0B:
				/* Transmit Excessive Collision.*/
				pPar->nTxExcessCollCount = data;
				break;
			case 0x0D:
				/* Transmit Pause Frame Count */
				pPar->nTxPauseCount = data;
				break;
			case 0x10:
				/* Transmit Drop Frame Count */
				pPar->nTxDroppedPkts = data;
				break;
			case 0x0E:
				/* Transmit Good Byte Count (Low) */
				T_GoodByteCount_L = data;
				break;
			case 0x0F:
				/* Transmit Good Byte Count (High) */
				T_GoodByteCount_H = data;
				break;
			case 0x11:
				/* Transmit Dropped Packet Cound, based on Congestion Management.*/
				pPar->nTxAcmDroppedPkts = data;
				break;
		}
	}
	if ( bcast_cnt == 1 /*pEthSWDevHandle->GSWIP_Ver == LTQ_GSWIP_2_2*/  ) {
		pPar->nRxGoodPkts = r_frame + r_unicast + r_multicast;
		pPar->nTxGoodPkts = t_frame + t_unicast + t_multicast;
	} else {
		/* Receive Broadcase Frme Count */
		pPar->nRxBroadcastPkts = r_frame - r_unicast - r_multicast;
		/* Transmit Broadcase Frme Count */
		pPar->nTxBroadcastPkts = t_frame - t_unicast - t_multicast;
	}
	/* Receive Good Byte Count */
	pPar->nRxGoodBytes = (R_GoodByteCount_H << 32) | R_GoodByteCount_L;
	/* Receive Bad Byte Count */
	pPar->nRxBadBytes = (R_BadByteCount_H << 32) | R_BadByteCount_L;
	/* Transmit Good Byte Count */
	pPar->nTxGoodBytes = (T_GoodByteCount_H << 32) | T_GoodByteCount_L;
	return IFX_SUCCESS;
}
#if defined (CONFIG_LTQ_8021X ) && CONFIG_LTQ_8021X
IFX_return_t IFX_FLOW_8021X_EAPOL_RuleGet(IFX_void_t *pDevCtx, IFX_ETHSW_8021X_EAPOL_Rule_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_FLOW_STP_8021X_t *pStateHandle = &pEthSWDevHandle->STP_8021x_Config;

	pPar->eForwardPort = pStateHandle->eForwardPort;
	pPar->nForwardPortId = pStateHandle->n8021X_ForwardPortId;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_8021X_EAPOL_RuleSet(IFX_void_t *pDevCtx, IFX_ETHSW_8021X_EAPOL_Rule_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_FLOW_STP_8021X_t *pStateHandle = &pEthSWDevHandle->STP_8021x_Config;
	IFX_FLOW_PCE_rule_t PCE_rule;

	pStateHandle->eForwardPort = pPar->eForwardPort;
	pStateHandle->n8021X_ForwardPortId   = pPar->nForwardPortId;
	memset(&PCE_rule, 0 , sizeof(IFX_FLOW_PCE_rule_t));
	PCE_rule.pattern.nIndex = 60;
	PCE_rule.pattern.bEnable = IFX_TRUE;
	PCE_rule.pattern.bMAC_DstEnable = IFX_TRUE;
	PCE_rule.pattern.nMAC_Dst[0] = 0x01;
	PCE_rule.pattern.nMAC_Dst[1] = 0x80;
	PCE_rule.pattern.nMAC_Dst[2] = 0xC2;
	PCE_rule.pattern.nMAC_Dst[3] = 0x00;
	PCE_rule.pattern.nMAC_Dst[4] = 0x00;
	PCE_rule.pattern.nMAC_Dst[5] = 0x03;
	PCE_rule.pattern.nMAC_Src[5] = 0;
    PCE_rule.pattern.bEtherTypeEnable = IFX_TRUE;
    PCE_rule.pattern.nEtherType = 0x888E;
    PCE_rule.action.eCrossStateAction = IFX_FLOW_PCE_ACTION_CROSS_STATE_CROSS;
    if ((pStateHandle->eForwardPort < 4) && (pStateHandle->eForwardPort > 0))
    	PCE_rule.action.ePortMapAction = pStateHandle->eForwardPort + 1;
    else
    	IFXOS_PRINT_INT_RAW("The Forward port action incorrect\n");
    PCE_rule.action.nForwardPortMap = (1 << pPar->nForwardPortId);
    /* We prepare everything and write into PCE Table */
    if (0 != ifx_pce_rule_write(&pEthSWDevHandle->PCE_Handler,&PCE_rule))
    	return IFX_ERROR;
#if defined(FLOW_TABLE_DEBUG) && FLOW_TABLE_DEBUG
	IFX_FLOW_MAC_DASA_LSB_Table_Print();
#endif
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_8021X_PortCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_8021X_portCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_FLOW_portConfig_t *pPortHandle ;
	if( pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber)
		return IFX_ERROR;
/*	IFX_FLOW_portConfig_t *pPortHandle = &pEthSWDevHandle->PortConfig[pPar->nPortId]; */
	pPortHandle = &pEthSWDevHandle->PortConfig[pPar->nPortId];

	pPar->eState = pPortHandle->ifx_8021x_state;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_8021X_PortCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_8021X_portCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_FLOW_portConfig_t *pPortHandle;

	if( pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber)
		return IFX_ERROR;
	pPortHandle = &pEthSWDevHandle->PortConfig[pPar->nPortId];
/*	IFX_FLOW_portConfig_t *pPortHandle = &pEthSWDevHandle->PortConfig[pPar->nPortId]; */

	pPortHandle->ifx_8021x_state = pPar->eState;
	/* Config the Table */
	set_port_state(pDevCtx, pPar->nPortId, pPortHandle->ifx_stp_state, pPortHandle->ifx_8021x_state);
	return IFX_SUCCESS;
}
#endif /*CONFIG_LTQ_8021X  */
IFX_return_t IFX_FLOW_CPU_PortCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_CPU_PortCfg_t *pPar)
{
   IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint8_t portIdx = pPar->nPortId;
	IFX_uint32_t value;

   if( pPar->nPortId >= pEthSWDevHandle->nPortNumber )
      return IFX_ERROR;
	if (portIdx ==  pEthSWDevHandle->nCPU_Port /* VRX_PLATFORM_CPU_PORT */)
		pPar->bCPU_PortValid = IFX_ENABLE;
	else
		pPar->bCPU_PortValid = IFX_DISABLE;
	/* Special Tag Egress*/
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_FDMA_PCTRL_STEN_OFFSET + ( 0x6 * portIdx)),	\
		VR9_FDMA_PCTRL_STEN_SHIFT, VR9_FDMA_PCTRL_STEN_SIZE, &value);
	pPar->bSpecialTagEgress = value;
	/* Special Tag Igress*/
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PCE_PCTRL_0_IGSTEN_OFFSET + ( 0xa * portIdx)),	\
		VR9_PCE_PCTRL_0_IGSTEN_SHIFT, VR9_PCE_PCTRL_0_IGSTEN_SIZE, &value);
	pPar->bSpecialTagIngress = value;
	/* FCS Check */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_SDMA_PCTRL_FCSIGN_OFFSET + ( 0x6 * portIdx)),	\
		VR9_SDMA_PCTRL_FCSIGN_SHIFT, VR9_SDMA_PCTRL_FCSIGN_SIZE, &value);
	pPar->bFcsCheck = value;
	/* FCS Generate */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_MAC_CTRL_0_FCS_OFFSET + ( 0xC * portIdx)),	\
		VR9_MAC_CTRL_0_FCS_SHIFT, VR9_MAC_CTRL_0_FCS_SIZE, &value);
	pPar->bFcsGenerate = value;
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_FDMA_PCTRL_ST_TYPE_OFFSET + ( 0x6 * portIdx)),	\
		VR9_FDMA_PCTRL_ST_TYPE_SHIFT, VR9_FDMA_PCTRL_ST_TYPE_SIZE, &value);
	 pPar->bSpecialTagEthType = value  ;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_CPU_PortCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_CPU_PortCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint8_t portIdx = pPar->nPortId;
	IFX_uint32_t RST, AS, AST, RXSH;

   if( pPar->nPortId >= pEthSWDevHandle->nPortNumber )
      return IFX_ERROR;
	if (portIdx == pEthSWDevHandle->nCPU_Port /* VRX_PLATFORM_CPU_PORT */)
		pPar->bCPU_PortValid = IFX_ENABLE;
	else
 		pPar->bCPU_PortValid = IFX_DISABLE;
	pEthSWDevHandle->nManagementPortNumber = portIdx;
	/* Special Tag Egress*/
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_FDMA_PCTRL_STEN_OFFSET + ( 0x6 * portIdx)),	\
		VR9_FDMA_PCTRL_STEN_SHIFT, VR9_FDMA_PCTRL_STEN_SIZE, pPar->bSpecialTagEgress);
	/* VRX CPU port */
	if ( portIdx == pEthSWDevHandle->nCPU_Port /* VRX_PLATFORM_CPU_PORT */ ) {
		if (pPar->bSpecialTagEgress == IFX_FALSE) {
			RST = 1;
			AS = 0;
		} else {
			RST = 0;
			AS = 1;
		}
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PMAC_HD_CTL_RST_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_PMAC_HD_CTL_RST_SHIFT, VR9_PMAC_HD_CTL_RST_SIZE, RST);

		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PMAC_HD_CTL_AS_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_PMAC_HD_CTL_AS_SHIFT, VR9_PMAC_HD_CTL_AS_SIZE, AS);
	}
	/* Special Tag Igress*/
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PCE_PCTRL_0_IGSTEN_OFFSET + ( 0xa * portIdx)),	\
		VR9_PCE_PCTRL_0_IGSTEN_SHIFT, VR9_PCE_PCTRL_0_IGSTEN_SIZE, pPar->bSpecialTagIngress);
	if (pPar->bSpecialTagIngress == IFX_FALSE) {
		AST = 0;
		RXSH = 0;
	} else {
		AST = 1;
		RXSH = 1;
	}
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PMAC_HD_CTL_AST_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PMAC_HD_CTL_AST_SHIFT, VR9_PMAC_HD_CTL_AST_SIZE, AST);
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PMAC_HD_CTL_RXSH_OFFSET+ IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PMAC_HD_CTL_RXSH_SHIFT, VR9_PMAC_HD_CTL_RXSH_SIZE, RXSH);
	/* FCS Check */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_SDMA_PCTRL_FCSIGN_OFFSET + ( 0x6 * portIdx)),	\
		VR9_SDMA_PCTRL_FCSIGN_SHIFT, VR9_SDMA_PCTRL_FCSIGN_SIZE, pPar->bFcsCheck);
	/* FCS Generate */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_MAC_CTRL_0_FCS_OFFSET + ( 0xC * portIdx)),	\
		VR9_MAC_CTRL_0_FCS_SHIFT, VR9_MAC_CTRL_0_FCS_SIZE, pPar->bFcsGenerate);
	if ( pPar->bSpecialTagEthType == IFX_ETHSW_CPU_ETHTYPE_FLOWID ) {
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_FDMA_PCTRL_ST_TYPE_OFFSET + ( 0x6 * portIdx)),	\
			VR9_FDMA_PCTRL_ST_TYPE_SHIFT, VR9_FDMA_PCTRL_ST_TYPE_SIZE, 1);
	} else {
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_FDMA_PCTRL_ST_TYPE_OFFSET + ( 0x6 * portIdx)),	\
			VR9_FDMA_PCTRL_ST_TYPE_SHIFT, VR9_FDMA_PCTRL_ST_TYPE_SIZE, 0);
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_CPU_PortExtendCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_CPU_PortExtendCfg_t *pPar)
{
	IFX_uint32_t  value, value_add, value_vlan;

	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PMAC_HD_CTL_ADD_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PMAC_HD_CTL_ADD_SHIFT, VR9_PMAC_HD_CTL_ADD_SIZE, &value_add);

	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PMAC_HD_CTL_TAG_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PMAC_HD_CTL_TAG_SHIFT, VR9_PMAC_HD_CTL_TAG_SIZE, &value_vlan);
	if (value_add == 0 && value_vlan == 0)
		pPar->eHeaderAdd = 0;
	else if ( value_add == 1 && value_vlan == 0)
		pPar->eHeaderAdd = 1;
	else if ( value_add == 1 && value_vlan == 1)
		pPar->eHeaderAdd = 2;
	else
		pPar->eHeaderAdd = 0;

	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PMAC_HD_CTL_RL2_OFFSET +IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PMAC_HD_CTL_RL2_SHIFT, VR9_PMAC_HD_CTL_RL2_SIZE, &value);
	pPar->bHeaderRemove = value;
	memset(&pPar->sHeader, 0 , sizeof(IFX_ETHSW_CPU_Header_t));
	if ( value_add == 1 ) {
 		/* Output the Src MAC */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PMAC_SA3_SA_15_0_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_PMAC_SA3_SA_15_0_SHIFT, VR9_PMAC_SA3_SA_15_0_SIZE, &value);
		pPar->sHeader.nMAC_Src[0] = value & 0xFF;
		pPar->sHeader.nMAC_Src[1] = ((value >> 8 ) & 0xFF);
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PMAC_SA2_SA_31_16_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_PMAC_SA2_SA_31_16_SHIFT, VR9_PMAC_SA2_SA_31_16_SIZE, &value);
		pPar->sHeader.nMAC_Src[2] = value & 0xFF;
		pPar->sHeader.nMAC_Src[3] = ((value >> 8 ) & 0xFF);
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PMAC_SA1_SA_47_32_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_PMAC_SA1_SA_47_32_SHIFT, VR9_PMAC_SA1_SA_47_32_SIZE, &value);
		pPar->sHeader.nMAC_Src[4] = value & 0xFF;
		pPar->sHeader.nMAC_Src[5] = ((value >> 8 ) & 0xFF);
		/* Output the Dst MAC */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PMAC_DA3_DA_15_0_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_PMAC_DA3_DA_15_0_SHIFT, VR9_PMAC_DA3_DA_15_0_SIZE, &value);
		pPar->sHeader.nMAC_Dst[0] = value & 0xFF;;
		pPar->sHeader.nMAC_Dst[1] = ((value >> 8 ) & 0xFF);
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PMAC_DA2_DA_31_16_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_PMAC_DA2_DA_31_16_SHIFT, VR9_PMAC_DA2_DA_31_16_SIZE, &value);
		pPar->sHeader.nMAC_Dst[2] = value & 0xFF;;
		pPar->sHeader.nMAC_Dst[3] = ((value >> 8 ) & 0xFF);
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PMAC_DA1_SA_47_32_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_PMAC_DA1_SA_47_32_SHIFT, VR9_PMAC_DA1_SA_47_32_SIZE, &value);
		pPar->sHeader.nMAC_Dst[4] = value & 0xFF;;
		pPar->sHeader.nMAC_Dst[5] = ((value >> 8 ) & 0xFF);
		/* Input the Ethernet Type */
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PMAC_TL_TYPE_LEN_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_PMAC_TL_TYPE_LEN_SHIFT, VR9_PMAC_TL_TYPE_LEN_SIZE, &value);
		pPar->sHeader.nEthertype = value;
	}
   if ( value_vlan == 1) {
      ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PMAC_VLAN_PRI_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
      	VR9_PMAC_VLAN_PRI_SHIFT, VR9_PMAC_VLAN_PRI_SIZE, &value);
      pPar->sHeader.nVLAN_Prio = value;
      ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PMAC_VLAN_CFI_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
      	VR9_PMAC_VLAN_CFI_SHIFT, VR9_PMAC_VLAN_CFI_SIZE, &value);
      pPar->sHeader.nVLAN_CFI = value;
      ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PMAC_VLAN_VLAN_ID_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
      	VR9_PMAC_VLAN_VLAN_ID_SHIFT, VR9_PMAC_VLAN_VLAN_ID_SIZE, &value);
      pPar->sHeader.nVLAN_ID = value;
   }
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PMAC_HD_CTL_FC_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PMAC_HD_CTL_FC_SHIFT, VR9_PMAC_HD_CTL_FC_SIZE, &value);
	pPar->ePauseCtrl = value;
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PMAC_HD_CTL_RC_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PMAC_HD_CTL_RC_SHIFT, VR9_PMAC_HD_CTL_RC_SIZE, &value);
	pPar->bFcsRemove = value;
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PMAC_EWAN_EWAN_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PMAC_EWAN_EWAN_SHIFT, VR9_PMAC_EWAN_EWAN_SIZE, &value);
	pPar->nWAN_Ports = value;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_CPU_PortExtendCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_CPU_PortExtendCfg_t *pPar)
{
	IFX_uint32_t  value_add =0, value_vlan=0;
	IFX_uint32_t  nMAC;

	switch (pPar->eHeaderAdd) {
		case IFX_ETHSW_CPU_HEADER_NO:
			value_add = 0;
			value_vlan = 0;
			break;
		case IFX_ETHSW_CPU_HEADER_MAC:
			value_add = 1;
			value_vlan = 0;
			break;
		case IFX_ETHSW_CPU_HEADER_VLAN:
			value_add = 1;
			value_vlan = 1;
			break;
	}
	if ((pPar->bHeaderRemove == IFX_ENABLE) && (pPar->eHeaderAdd != IFX_ETHSW_CPU_HEADER_NO)) {
		IFXOS_PRINT_INT_RAW("The Header Can't be remove because the Header Add parameter is not 0");
		return IFX_ERROR;
	}  else {
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PMAC_HD_CTL_RL2_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_PMAC_HD_CTL_RL2_SHIFT, VR9_PMAC_HD_CTL_RL2_SIZE, pPar->bHeaderRemove);
	}
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PMAC_HD_CTL_ADD_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PMAC_HD_CTL_ADD_SHIFT, VR9_PMAC_HD_CTL_ADD_SIZE, value_add);
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PMAC_HD_CTL_TAG_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PMAC_HD_CTL_TAG_SHIFT, VR9_PMAC_HD_CTL_TAG_SIZE, value_vlan);
	if (pPar->eHeaderAdd == IFX_ETHSW_CPU_HEADER_MAC) {
		/* Input the Src MAC */
		nMAC = pPar->sHeader.nMAC_Src[0] | pPar->sHeader.nMAC_Src[1] << 8;
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PMAC_SA3_SA_15_0_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_PMAC_SA3_SA_15_0_SHIFT, VR9_PMAC_SA3_SA_15_0_SIZE, nMAC);
		nMAC = pPar->sHeader.nMAC_Src[2] | pPar->sHeader.nMAC_Src[3] << 8;
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PMAC_SA2_SA_31_16_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_PMAC_SA2_SA_31_16_SHIFT, VR9_PMAC_SA2_SA_31_16_SIZE, nMAC);
		nMAC = pPar->sHeader.nMAC_Src[4] | pPar->sHeader.nMAC_Src[5] << 8;
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PMAC_SA1_SA_47_32_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_PMAC_SA1_SA_47_32_SHIFT, VR9_PMAC_SA1_SA_47_32_SIZE, nMAC);
		/* Input the Dst MAC */
		nMAC = pPar->sHeader.nMAC_Dst[0] | pPar->sHeader.nMAC_Dst[1] << 8;
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PMAC_DA3_DA_15_0_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_PMAC_DA3_DA_15_0_SHIFT, VR9_PMAC_DA3_DA_15_0_SIZE, nMAC);
		nMAC = pPar->sHeader.nMAC_Dst[2] | pPar->sHeader.nMAC_Dst[3] << 8;
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PMAC_DA2_DA_31_16_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_PMAC_DA2_DA_31_16_SHIFT, VR9_PMAC_DA2_DA_31_16_SIZE, nMAC);
		nMAC = pPar->sHeader.nMAC_Dst[4] | pPar->sHeader.nMAC_Dst[5] << 8;
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PMAC_DA1_SA_47_32_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_PMAC_DA1_SA_47_32_SHIFT, VR9_PMAC_DA1_SA_47_32_SIZE, nMAC);
		/* Input the Ethernet Type */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PMAC_TL_TYPE_LEN_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_PMAC_TL_TYPE_LEN_SHIFT, VR9_PMAC_TL_TYPE_LEN_SIZE, pPar->sHeader.nEthertype);
	}
	if (pPar->eHeaderAdd == IFX_ETHSW_CPU_HEADER_VLAN) {
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PMAC_VLAN_PRI_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
			VR9_PMAC_VLAN_PRI_SHIFT, VR9_PMAC_VLAN_PRI_SIZE, pPar->sHeader.nVLAN_Prio);
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PMAC_VLAN_CFI_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PMAC_VLAN_CFI_SHIFT, VR9_PMAC_VLAN_CFI_SIZE, pPar->sHeader.nVLAN_CFI);
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PMAC_VLAN_VLAN_ID_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PMAC_VLAN_VLAN_ID_SHIFT, VR9_PMAC_VLAN_VLAN_ID_SIZE, pPar->sHeader.nVLAN_ID);
	}
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PMAC_HD_CTL_FC_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PMAC_HD_CTL_FC_SHIFT, VR9_PMAC_HD_CTL_FC_SIZE, pPar->ePauseCtrl);
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PMAC_HD_CTL_RC_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PMAC_HD_CTL_RC_SHIFT, VR9_PMAC_HD_CTL_RC_SIZE, pPar->bFcsRemove);
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PMAC_EWAN_EWAN_OFFSET + IFX_ETHSW_FLOW_TOP_REG_OFFSET),	\
		VR9_PMAC_EWAN_EWAN_SHIFT, VR9_PMAC_EWAN_EWAN_SIZE, pPar->nWAN_Ports);
	return IFX_SUCCESS;
}
#if defined (CONFIG_LTQ_WOL ) && CONFIG_LTQ_WOL
IFX_return_t IFX_FLOW_WoL_CfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_WoL_Cfg_t *pPar)
{
	IFX_uint32_t value;

	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_WOL_GLB_CTRL_PASSEN_OFFSET,	\
	VR9_WOL_GLB_CTRL_PASSEN_SHIFT, VR9_WOL_GLB_CTRL_PASSEN_SIZE, &value);
	pPar->bWolPasswordEnable = value;
	/* ToDo: Require future fill in the DstMac & Password */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_WOL_DA_2_DA2_OFFSET,VR9_WOL_DA_2_DA2_SHIFT,	\
		VR9_WOL_DA_2_DA2_SIZE, &value );
	pPar->nWolMAC[0] = (value >> 8 & 0xFF);
	pPar->nWolMAC[1] = (value & 0xFF);
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_WOL_DA_1_DA1_OFFSET,VR9_WOL_DA_1_DA1_SHIFT,	\
		VR9_WOL_DA_1_DA1_SIZE, &value );
	pPar->nWolMAC[2] = (value >> 8 & 0xFF);
	pPar->nWolMAC[3] = (value & 0xFF);
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_WOL_DA_0_DA0_OFFSET,VR9_WOL_DA_0_DA0_SHIFT,	\
		VR9_WOL_DA_0_DA0_SIZE, &value );
	pPar->nWolMAC[4] = (value >> 8 & 0xFF);
	pPar->nWolMAC[5] = (value & 0xFF);

	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_WOL_PW_2_PW2_OFFSET,VR9_WOL_PW_2_PW2_SHIFT,	\
		VR9_WOL_PW_2_PW2_SIZE, &value );
	pPar->nWolPassword[0] = (value >> 8 & 0xFF);
	pPar->nWolPassword[1] = (value & 0xFF);
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_WOL_PW_1_PW1_OFFSET,VR9_WOL_PW_1_PW1_SHIFT,	\
		VR9_WOL_PW_1_PW1_SIZE, &value );
	pPar->nWolPassword[2] = (value >> 8 & 0xFF);
	pPar->nWolPassword[3] = (value & 0xFF);
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_WOL_PW_0_PW0_OFFSET,VR9_WOL_PW_0_PW0_SHIFT,	\
		VR9_WOL_PW_0_PW0_SIZE, &value );
	pPar->nWolPassword[4] = (value >> 8 & 0xFF);
	pPar->nWolPassword[5] = (value & 0xFF);

	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_WoL_CfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_WoL_Cfg_t *pPar)
{
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_WOL_GLB_CTRL_PASSEN_OFFSET,	\
		VR9_WOL_GLB_CTRL_PASSEN_SHIFT, VR9_WOL_GLB_CTRL_PASSEN_SIZE, pPar->bWolPasswordEnable);
	/* ToDo: Require future fill in the DstMac & Password */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_WOL_DA_2_DA2_OFFSET, VR9_WOL_DA_2_DA2_SHIFT,	\
		VR9_WOL_DA_2_DA2_SIZE, ( ( (pPar->nWolMAC[0] & 0xFF) << 8 )| (pPar->nWolMAC[1] & 0xFF) ) );
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_WOL_DA_1_DA1_OFFSET,VR9_WOL_DA_1_DA1_SHIFT,	\
		VR9_WOL_DA_1_DA1_SIZE, ( ( (pPar->nWolMAC[2] &0xFF) << 8)| (pPar->nWolMAC[3] & 0xFF)) );
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_WOL_DA_0_DA0_OFFSET,VR9_WOL_DA_0_DA0_SHIFT,	\
		VR9_WOL_DA_0_DA0_SIZE, ( ((pPar->nWolMAC[4] & 0xFF) << 8) | (pPar->nWolMAC[5] & 0xFF)) );

	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_WOL_PW_2_PW2_OFFSET,VR9_WOL_PW_2_PW2_SHIFT,	\
		VR9_WOL_PW_2_PW2_SIZE, ( ((pPar->nWolPassword[0] & 0xFF) << 8)| (pPar->nWolPassword[1] & 0xFF)) );
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_WOL_PW_1_PW1_OFFSET,VR9_WOL_PW_1_PW1_SHIFT,	\
		VR9_WOL_PW_1_PW1_SIZE, (  ( (pPar->nWolPassword[2] & 0xFF) << 8) | (pPar->nWolPassword[3] & 0xFF)) );
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_WOL_PW_0_PW0_OFFSET,VR9_WOL_PW_0_PW0_SHIFT,	\
		VR9_WOL_PW_0_PW0_SIZE, ( ( (pPar->nWolPassword[4] & 0xFF)  << 8 )| (pPar->nWolPassword[5] & 0xFF)));
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_WoL_PortCfgGet(IFX_void_t *pDevCtx, IFX_ETHSW_WoL_PortCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	if( pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber)
		return IFX_ERROR;
	/* Read the WOL_CTRL.PORT */
	ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_WOL_CTRL_PORT_OFFSET + (0xA * pPar->nPortId)),	\
		VR9_WOL_CTRL_PORT_SHIFT, VR9_WOL_CTRL_PORT_SIZE, &pPar->bWakeOnLAN_Enable);
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_WoL_PortCfgSet(IFX_void_t *pDevCtx, IFX_ETHSW_WoL_PortCfg_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	if( pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber )
		return IFX_ERROR;
	/* Write to WOL_CTRL.PORT */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_WOL_CTRL_PORT_OFFSET + (0xA * pPar->nPortId)),	\
		VR9_WOL_CTRL_PORT_SHIFT, VR9_WOL_CTRL_PORT_SIZE, pPar->bWakeOnLAN_Enable);
	return IFX_SUCCESS;
}
#endif /* CONFIG_LTQ_WOL */
IFX_return_t IFX_FLOW_RegisterGet(IFX_void_t *pDevCtx, IFX_FLOW_register_t *pPar)
{
	IFX_uint32_t regValue, regAddr = pPar->nRegAddr;
	regValue = VR9_REG32_ACCESS(VR9_BASE_ADDRESS + regAddr * 4);
	pPar->nData = regValue;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_RegisterSet(IFX_void_t *pDevCtx, IFX_FLOW_register_t *pPar)
{
	IFX_uint32_t regValue = pPar->nData, regAddr = pPar->nRegAddr  ;
	VR9_REG32_ACCESS(VR9_BASE_ADDRESS + regAddr * 4) = regValue;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_IrqGet(IFX_void_t *pDevCtx, IFX_FLOW_irq_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	if( pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber)
		return IFX_ERROR;
	/* ToDo: Require future clarify for how to display */
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_IrqMaskGet(IFX_void_t *pDevCtx, IFX_FLOW_irq_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint8_t portIdx = pPar->nPortId;
	IFX_uint32_t value;

	if( pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber)
 		return IFX_ERROR;
	/* Set PCE.PIER */
	if (pPar->eIrqSrc == IFX_FLOW_IRQ_WOL) {
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PCE_PIER_WOL_OFFSET + (0xA * portIdx)),	\
			VR9_PCE_PIER_WOL_SHIFT, VR9_PCE_PIER_WOL_SIZE, &value);
	IFXOS_PRINT_INT_RAW("The Wake-on-LAN Interrupt is %d\n",value);
	} else if(pPar->eIrqSrc == IFX_FLOW_IRQ_LIMIT_ALERT) {
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PCE_PIER_LOCK_OFFSET + (0xA * portIdx)),	\
			VR9_PCE_PIER_LOCK_SHIFT, VR9_PCE_PIER_LOCK_SIZE, &value);
		IFXOS_PRINT_INT_RAW("The Port Limit Alert Interrupt is %d\n",value);
	} else if(pPar->eIrqSrc == IFX_FLOW_IRQ_LOCK_ALERT) {
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, (VR9_PCE_PIER_LIM_OFFSET + (0xA * portIdx)),	\
			VR9_PCE_PIER_LIM_SHIFT, VR9_PCE_PIER_LIM_SIZE, &value);
		IFXOS_PRINT_INT_RAW("The Port Lock Alert Interrupt is %d\n",value);
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_IrqMaskSet(IFX_void_t *pDevCtx, IFX_FLOW_irq_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint8_t portIdx = pPar->nPortId;

	if( pPar->nPortId >= pEthSWDevHandle->nTotalPortNumber)
		return IFX_ERROR;
	/* Set PCE.PIER */
	if (pPar->eIrqSrc == IFX_FLOW_IRQ_WOL) {
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PCE_PIER_WOL_OFFSET + (0xA * portIdx)),	\
			VR9_PCE_PIER_WOL_SHIFT, VR9_PCE_PIER_WOL_SIZE, 1);
		IFXOS_PRINT_INT_RAW("Enable Wake-on-LAN Interrupt \n");
	} else if(pPar->eIrqSrc == IFX_FLOW_IRQ_LIMIT_ALERT) {
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx,(VR9_PCE_PIER_LOCK_OFFSET + (0xA * portIdx)),	\
			VR9_PCE_PIER_LOCK_SHIFT, VR9_PCE_PIER_LOCK_SIZE, 1);
		IFXOS_PRINT_INT_RAW("Enable Port Limit Alert Interrupt \n");
	} else if(pPar->eIrqSrc == IFX_FLOW_IRQ_LOCK_ALERT) {
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_PCE_PIER_LIM_OFFSET + (0xA * portIdx)),	\
			VR9_PCE_PIER_LIM_SHIFT, VR9_PCE_PIER_LIM_SIZE, 1);
		IFXOS_PRINT_INT_RAW("Enable Port Lock Alert Interrupt \n");
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_IrqStatusClear(IFX_void_t *pDevCtx, IFX_FLOW_irq_t *pPar)
{
	/* ToDo: Request future clarify */
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_PceRuleRead(IFX_void_t *pDevCtx, IFX_FLOW_PCE_rule_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;

	if (0 != ifx_pce_rule_read(&pEthSWDevHandle->PCE_Handler, pPar))
		return IFX_ERROR;
  return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_PceRuleWrite(IFX_void_t *pDevCtx, IFX_FLOW_PCE_rule_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;

	if (0 != ifx_pce_rule_write(&pEthSWDevHandle->PCE_Handler, pPar))
 		return IFX_ERROR;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_PceRuleDelete(IFX_void_t *pDevCtx, IFX_FLOW_PCE_ruleDelete_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint32_t value;

	value = pPar->nIndex;
	if (0 != ifx_pce_pattern_delete(&pEthSWDevHandle->PCE_Handler, value))
		return IFX_ERROR;
	if (0 != ifx_pce_action_delete(&pEthSWDevHandle->PCE_Handler, value))
		return IFX_ERROR;
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_RMON_ExtendGet(IFX_void_t *pDevCtx, IFX_FLOW_RMON_extendGet_t *pPar)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint8_t portIdx = pPar->nPortId;
	IFX_uint32_t  value;
	IFX_uint8_t   i;
	IFX_uint32_t  data0, data1;

	if( pPar->nPortId >= pEthSWDevHandle->nPortNumber )
		return IFX_ERROR;
	memset(pPar, 0 , sizeof(IFX_FLOW_RMON_extendGet_t));
	for (i = 0; i < IFX_FLOW_RMON_EXTEND_NUM; i++) {
		/* Set the BM RAM ADDR  */
		/* Receive Frame Count  */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_ADDR_ADDR_OFFSET,	\
			VR9_BM_RAM_ADDR_ADDR_SHIFT, VR9_BM_RAM_ADDR_ADDR_SIZE, (i+RMON_EXTEND_TRAFFIC_FLOW_COUNT_1));
		/* Specify the port ID */
 		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_ADDR_OFFSET,	\
 			VR9_BM_RAM_CTRL_ADDR_SHIFT, VR9_BM_RAM_CTRL_ADDR_SIZE, portIdx);
 		/* Table Access Operation Mode Read */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_OPMOD_OFFSET,	\
			VR9_BM_RAM_CTRL_OPMOD_SHIFT, VR9_BM_RAM_CTRL_OPMOD_SIZE, 0x0);
		value = 1;
		/* Active */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_RAM_CTRL_BAS_OFFSET,	\
			VR9_BM_RAM_CTRL_BAS_SHIFT, VR9_BM_RAM_CTRL_BAS_SIZE, value);
		do {
			ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_RAM_CTRL_BAS_OFFSET,	\
				VR9_BM_RAM_CTRL_BAS_SHIFT, VR9_BM_RAM_CTRL_BAS_SIZE, &value);
		} while(value);
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_RAM_VAL_0_VAL0_OFFSET,	\
			VR9_BM_RAM_VAL_0_VAL0_SHIFT, VR9_BM_RAM_VAL_0_VAL0_SIZE, &data0);
		ifx_ethsw_ll_DirectAccessRead(pDevCtx, VR9_BM_RAM_VAL_1_VAL1_OFFSET,	\
			VR9_BM_RAM_VAL_1_VAL1_SHIFT, VR9_BM_RAM_VAL_1_VAL1_SIZE, &data1);
		pPar->nTrafficFlowCnt[i] = (data1 << 16 | data0);
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_Reset(IFX_void_t *pDevCtx, IFX_FLOW_reset_t *pPar)
{
	IFX_FLOW_switchDev_t *pDev = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint32_t  value;
	pDev->bResetCalled = IFX_TRUE;
	pDev->bHW_InitCalled = IFX_FALSE;
/* Reset the Switch via Switch IP register*/
	value = 1;
	ifx_ethsw_ll_DirectAccessWrite(pDev, VR9_ETHSW_SWRES_R0_OFFSET,	\
		VR9_ETHSW_SWRES_R0_SHIFT, VR9_ETHSW_SWRES_R0_SIZE, value);
	do {
		ifx_ethsw_ll_DirectAccessRead(pDev, VR9_ETHSW_SWRES_R0_OFFSET,	\
			VR9_ETHSW_SWRES_R0_SHIFT, VR9_ETHSW_SWRES_R0_SIZE, &value);
		udelay(1000);
	} while(value);
	/* Reset the Switch via RCU reset bit*/
//	platform_device_reset_trigger();
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_VersionGet(IFX_void_t *pDevCtx, IFX_ETHSW_version_t *pPar)
{
	if (pPar->nId == 0) {
		memcpy(pPar->cName, VERSION_NAME, sizeof(VERSION_NAME));
		memcpy(pPar->cVersion, VERSION_NUMBER, sizeof(VERSION_NUMBER));
	} else if (pPar->nId == 1) {
		memcpy(pPar->cName, MICRO_CODE_VERSION_NAME, sizeof(MICRO_CODE_VERSION_NAME));
		memcpy(pPar->cVersion, MICRO_CODE_VERSION_NUMBER, sizeof(MICRO_CODE_VERSION_NUMBER));
	} else {
		memcpy(pPar->cName, "", 0);
		memcpy(pPar->cVersion, "", 0);
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_Enable(IFX_void_t *pDevCtx)
{
	IFX_FLOW_switchDev_t *pDev = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint8_t j;

	if (pDev->bHW_InitCalled == IFX_FALSE)
		platform_device_init(pDev);
	/* Enable all physical port - FDMA_PCTRL.EN & SDMA_PCTRL.PEN */
	for (j =0; j < pDev->nPortNumber; j++) {
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_FDMA_PCTRL_EN_OFFSET + (j * 0x6)),	\
			VR9_FDMA_PCTRL_EN_SHIFT, VR9_FDMA_PCTRL_EN_SIZE, 1);
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_SDMA_PCTRL_PEN_OFFSET + (j * 0x6)),	\
			VR9_SDMA_PCTRL_PEN_SHIFT, VR9_SDMA_PCTRL_PEN_SIZE, 1);
		/* Enable the RMON Counter  */
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, VR9_BM_PCFG_CNTEN_OFFSET + (j * 2),	\
			VR9_BM_PCFG_CNTEN_SHIFT, VR9_BM_PCFG_CNTEN_SIZE, 1);
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_FLOW_Disable(IFX_void_t *pDevCtx)
{
	IFX_FLOW_switchDev_t *pDev = (IFX_FLOW_switchDev_t*)pDevCtx;
	IFX_uint8_t j;

	/* Disable all physical port - FDMA_PCTRL.EN & SDMA_PCTRL.PEN */
	for (j =0; j < pDev->nPortNumber; j++) {
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_FDMA_PCTRL_EN_OFFSET + (j * 0x6)),	\
			VR9_FDMA_PCTRL_EN_SHIFT, VR9_FDMA_PCTRL_EN_SIZE, 0);
		ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_SDMA_PCTRL_PEN_OFFSET + (j * 0x6)),	\
		VR9_SDMA_PCTRL_PEN_SHIFT, VR9_SDMA_PCTRL_PEN_SIZE, 0);
	}
	return IFX_SUCCESS;
}
#ifdef IFX_ETHSW_API_COC
/**
   This is the switch core layer function to set PHY PD bit (Phy register 0:bit11)
   \param pDevCtx  This parameter is a pointer to the switch device context.
   \param nPHYAD  This parameter is phy address.
   \return Return value as follows:
   - IFX_SUCCESS: if successful
*/
IFX_return_t IFX_FLOW_PHY_PDN_Set(IFX_void_t *pDevCtx, IFX_uint8_t nPHYAD)
{
	IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
    IFX_ETHSW_MDIO_data_t mdio_data;
    mdio_data.nAddressDev = nPHYAD;
    mdio_data.nAddressReg = 0;
	IFX_FLOW_MDIO_DataRead(pEthSWDevHandle, &mdio_data);
	/* GSWIP_SET_BITS(x, msb, lsb, value) */
	 /*  Forces the PHY device into Power Down */
    mdio_data.nData = GSWIP_SET_BITS(mdio_data.nData, 11, 11, 1);
    IFX_FLOW_MDIO_DataWrite(pEthSWDevHandle, &mdio_data);
    return IFX_SUCCESS;
}

/**
   This is the switch core layer function to clear PHY PDN bit
   \param pDevCtx  This parameter is a pointer to the switch device context.
   \param nPHYAD  This parameter is phy address.
   \return Return value as follows:
   - IFX_SUCCESS: if successful
*/
IFX_return_t IFX_FLOW_PHY_PDN_Clear(IFX_void_t *pDevCtx, IFX_uint8_t nPHYAD)
{
    IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
    IFX_ETHSW_MDIO_data_t mdio_data;
    mdio_data.nAddressDev = nPHYAD;
    mdio_data.nAddressReg = 0;
    IFX_FLOW_MDIO_DataRead(pEthSWDevHandle, &mdio_data);
	/* GSWIP_SET_BITS(x, msb, lsb, value) */
    mdio_data.nData = GSWIP_SET_BITS(mdio_data.nData, 11, 11, 0);
    IFX_FLOW_MDIO_DataWrite(pEthSWDevHandle, &mdio_data);
    return IFX_SUCCESS;
}
/**
   This is the switch core layer function to get medium detect status
   \param pDevCtx  This parameter is a pointer to the switch device context.
   \param nPortID  This parameter is port id.
   \return Return value as follows:
   - IFX_TRUE: if exists
   - IFX_FALSE: if non-exists
*/
IFX_boolean_t IFX_FLOW_PHY_mediumDetectStatusGet(IFX_void_t *pDevCtx, IFX_uint8_t nPortID)
{
	IFX_ETHSW_MDIO_data_t mdio_data;
    IFX_uint32_t value;
    IFX_boolean_t bLinkStatus;
    IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
    mdio_data.nAddressDev = nPortID;
    mdio_data.nAddressReg = 0x16;
    IFX_FLOW_MDIO_DataRead(pEthSWDevHandle, &mdio_data);
    value = GSWIP_GET_BITS(mdio_data.nData,10,10);
    if (value)
        bLinkStatus = IFX_TRUE;
    else
        bLinkStatus = IFX_FALSE;
    return bLinkStatus;
}

IFX_boolean_t IFX_FLOW_PHY_Link_Status_Get(IFX_void_t *pDevCtx, IFX_uint8_t nPortID)
{
	IFX_ETHSW_MDIO_data_t mdio_data;
    IFX_uint32_t value;
    IFX_FLOW_switchDev_t *pEthSWDevHandle = (IFX_FLOW_switchDev_t*)pDevCtx;
    mdio_data.nAddressDev = nPortID;
    mdio_data.nAddressReg = 0x1;
    IFX_FLOW_MDIO_DataRead(pEthSWDevHandle, &mdio_data);
    if (mdio_data.nData != 0xFFFF ) {
	    value = GSWIP_GET_BITS(mdio_data.nData,2,2);
	    if (value)
	    	return (1);
	    else
	    	 return (0);
	} else
        return (0);
}
#if 0
IFX_boolean_t IFX_FLOW_PHY_Link_LPI_Set(IFX_void_t *pDevCtx, IFX_uint8_t nPortID) {

/* ---------------------------------------------------------------- */
	/* Low-power Idle Mode  configuration                                */
	/* ---------------------------------------------------------------- */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_MAC_CTRL_4_LPIEN_OFFSET + ( 0xC * portIdx)),	\
		VR9_MAC_CTRL_4_LPIEN_SHIFT, VR9_MAC_CTRL_4_LPIEN_SIZE, pPar->bLPI);
		/* LPI request controlled by data available for  port */
	ifx_ethsw_ll_DirectAccessWrite(pDevCtx, (VR9_FDMA_CTRL_LPI_MODE_OFFSET ),	\
		VR9_FDMA_CTRL_LPI_MODE_SHIFT, VR9_FDMA_CTRL_LPI_MODE_SIZE, 0x4);
}
#endif
#endif /* IFX_ETHSW_API_COC */
