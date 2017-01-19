/* ============================================================
Follow by
IEEE Standard for Information technology¡X
Telecommunications and information exchange between systems¡X
Local and metropolitan area networks¡X
Specific requirements

Part 3: Carrier Sense Multiple Access with
Collision Detection (CSMA/CD) access method
and Physical Layer specifications

SECTION  FIVE:  This  section  includes  Clause 56  through  Clause 74  and  Annex  57A  through
Annex 74A. (Third printing: 22 June 2010.)

Annex 57A
Requirements for support of Slow Protocols


Programmer : Alvin Hsu, alvin_hsu@arcadyan.com.tw
=============================================================== */
#ifndef _EOAM_H
#define _EOAM_H

#ifdef SUPERTASK
#include "config.h"
#else
#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/ctype.h>

#include <linux/kthread.h>

#define TRUE	1
#define FALSE	0
#endif // SUPERTASK

/*-------------------------------------------------------------
  Bit Operation.
--------------------------------------------------------------- */
#define	BIT00 		0x01
#define BIT01 		0x02
#define BIT02 		0x04
#define BIT03 		0x08
#define BIT04 		0x10
#define BIT05 		0x20
#define BIT06 		0x40
#define BIT07 		0x80

#define	WBIT00 		0x0001
#define WBIT01 		0x0002
#define WBIT02 		0x0004
#define WBIT03 		0x0008
#define WBIT04 		0x0010
#define WBIT05 		0x0020
#define WBIT06 		0x0040
#define WBIT07 		0x0080
#define WBIT08 		0x0100
#define WBIT09 		0x0200
#define WBIT10 		0x0400
#define WBIT11 		0x0800
#define WBIT12 		0x1000
#define WBIT13 		0x2000
#define WBIT14 		0x4000
#define WBIT15 		0x8000

#define DWBIT00 	0x00000001
#define DWBIT01 	0x00000002
#define DWBIT02 	0x00000004
#define DWBIT03 	0x00000008
#define DWBIT04 	0x00000010
#define DWBIT05 	0x00000020
#define DWBIT06 	0x00000040
#define DWBIT07 	0x00000080
#define DWBIT08 	0x00000100
#define DWBIT09 	0x00000200
#define DWBIT10 	0x00000400
#define DWBIT11 	0x00000800
#define DWBIT12 	0x00001000
#define DWBIT13 	0x00002000
#define DWBIT14 	0x00004000
#define DWBIT15 	0x00008000
#define DWBIT16 	0x00010000
#define DWBIT17 	0x00020000
#define DWBIT18 	0x00040000
#define DWBIT19 	0x00080000
#define DWBIT20 	0x00100000
#define DWBIT21 	0x00200000
#define DWBIT22 	0x00400000
#define DWBIT23 	0x00800000
#define DWBIT24 	0x01000000
#define DWBIT25 	0x02000000
#define DWBIT26 	0x04000000
#define DWBIT27 	0x08000000
#define DWBIT28 	0x10000000
#define DWBIT29 	0x20000000
#define DWBIT30 	0x40000000
#define DWBIT31 	0x80000000

/* Ethernet Frame Constant */
#define ETHER_MAC_LEN							6				/* Octets in one ethernet addr   */
#define ETHER_FRAME_MIN_LEN				60			/* Min. octets in frame sans FCS */
#define ETHER_FRAME_MAX_LEN				1514		/* Max. octets in frame sans FCS */
#define ETHER_FRAME_DATA_LEN 			1496		/* Max. octets in payload        */
#define ETHER_FRAME_FCS_LEN				4				/* Octets in the FCS             */

#define ETHER_HDR_LEN							14			/* Total octets in ethernet header. */
#define SLOW_HDR_LEN							4				/* Total octets in slow-ptl header. */

/*-------------------------------------------------------------
  Slow Protocol Transmission characteristics.
--------------------------------------------------------------- */
#define EOAMPDU_MAX_SIZE					512    	/* Max. OAMPDU size */
//#define EOAMPDU_COUNT_PER_SECOND  		10     		/* OAMPDU send count per second*/
//#define EMAX_OAMPDU_COUNT_PER_SECOND  	20 			/* Max. OAMPDU send count per second*/

/*-------------------------------------------------------------
  Slow Protocol Definition. & OAMPDU Format.
--------------------------------------------------------------- */
/* Slow Protocol Type */
#define SLOW_PTL_TYPE							0x8809	/* Slow Protocol Type */

#ifndef SUPERTASK
// If we want to get Slow Protocol Type from network packet directly, we must consider endian issue.
#if defined(__BIG_ENDIAN)
#define ET_SLOW_PTL								0x8809
#else
#define ET_SLOW_PTL								0x0988
#endif
#endif // SUPERTASK

#define EOAM_IFNAME_LENGTH 32
#define ETH_EOAM_IFNAME "eth1"
#define PTM_EOAM_IFNAME "ptm0"

/* Slow Protocol Subtypes Field */
typedef enum {
		SUBTYPE_UNUSED = 0,										/* Unused¡XIllegal value 															*/
		SUBTYPE_LACP,													/* Link Aggregation Control Protocol (LACP) 					*/
		SUBTYPE_LAMP,													/* Link Aggregation¡XMarker Protocol 									*/
		SUBTYPE_EOAM,													/* Operations, Administration, and Maintenance (OAM) 	*/
		SUBTYPE_RFU4,													/* Reserved for future use 														*/
		SUBTYPE_RFU5,													/* Reserved for future use 														*/
		SUBTYPE_RFU6,													/* Reserved for future use 														*/
		SUBTYPE_RFU7,													/* Reserved for future use 														*/
		SUBTYPE_RFU8,													/* Reserved for future use 														*/
		SUBTYPE_RFU9,													/* Reserved for future use 														*/
		SUBTYPE_OSSP,													/* Organization Specific Slow Protcol (OSSP) 					*/
}enSLOW_PTL_SUBTYPE;


/*-------------------------------------------------------------
  OAM Sublayer Control
--------------------------------------------------------------- */
/*
#define	EOAM_LAYER_DISABLE				(0==1)
#define	EOAM_LAYER_ENABLE				(1==1)

#define	_EOAM_LAYER_OPTIONAL					// add for make sure OAM support

#ifdef _EOAM_LAYER_OPTIONAL
#define	EOAM_LAYER_SUPPORT				EOAM_LAYER_ENABLE
#else
#define	EOAM_LAYER_SUPPORT				EOAM_LAYER_DISABLE
#endif
*/
typedef enum {
		EOAM_LAYER_DISABLE = 0,		//0
		EOAM_LAYER_PASSIVE_DTE,		//1
		EOAM_LAYER_ACTIVE_DTE,		//2
		EOAM_LAYER_LOOPBACK_DTE,
}enEOAM_LAYER_MODE;


/*-------------------------------------------------------------
  OAM Sublayer Control
--------------------------------------------------------------- */
/* Definition for Local OAM Mode*/
#define EOAM_DISABLE_MODE							0
#define EOAM_PASSIVE_MODE							1
#define EOAM_ACTIVE_MODE							2
#define EOAM_LOOPBACK_MODE						3


/*
	 EOAMAPI Definition
*/
#define EOAM_LOCAL_DTE								1
#define EOAM_REMOTE_DTE								2

/* Local EOAM Support Functions */
#define EOAM_SUPPORT_FUNC_NO					7

#define EOAM_FUNC_LINKFAULT						0
#define EOAM_FUNC_DYINGGASP						1
#define EOAM_FUNC_CRITICALEVENT				2
#define EOAM_FUNC_UNIDIRECTIONAL			3
#define EOAM_FUNC_LOOPBACK						4
#define EOAM_FUNC_LINKEVENT						5
#define EOAM_FUNC_VARIRETRIEVAL				6

/* Remote EOAM Information */
#define EOAM_REMOTE_MODE							0x00
#define EOAM_REMOTE_MAC								0x01
#define EOAM_REMOTE_OUI								0x02
#define EOAM_REMOTE_VERSION						0x03
#define EOAM_REMOTE_MAX_SIZE					0x04
#define EOAM_REMOTE_VENDOR						0x05
#define EOAM_REMOTE_PAR_ACT						0x06
#define EOAM_REMOTE_MUX_ACT						0x07

#define EOAM_REMOTE_FUNC_UNIDIRECTIONAL		0x10
#define EOAM_REMOTE_FUNC_LOOPBACK					0x11
#define EOAM_REMOTE_FUNC_LINKEVENT				0x12
#define EOAM_REMOTE_FUNC_VARIRETRIEVAL		0x13

/* supertask! console debug page no. */
#define CONSOLE_DBG_MAIN_PAGE					0
#define CONSOLE_DBG_STATISTICS				1
#define CONSOLE_DBG_LOOPBACK					2

#define TITLE_FUNCTIONALITY_STR					"Functionality     "
#define TITLE_LOCAL_MODE_STR						"Mode              "
#define TITLE_STATUS_STR								"Status            "
#define TITLE_LOCAL_INFORMATION_STR			"Local Information "
#define TITLE_REMOTE_INFORMATION_STR		"Remote Information"

#define TITLE_LOOOPBACK_START_STR				"Start LB Request"
#define TITLE_LOOOPBACK_CONNECTION_STR	"LoopBack Connection "
#define TITLE_LOOOPBACK_PARAMETER_STR		"LoopBack Parameters "
#define TITLE_LOOOPBACK_INFORMATION_STR	"LoopBack Information"

/* Local Information */
#define TITLE_OPERATIONAL_STATUS_STR		"Operational Status"
#define TITLE_LINK_FAULT_STR						"Link Fault DTE    "

/* Remote Information */
#define TITLE_RMT_MODE_STR							"Mode              "
#define TITLE_RMT_MAC_ADDR_STR					"MAC Address       "
#define TITLE_RMT_OUI_STR								"OUI               "
#define TITLE_RMT_VERSION_STR						"Version           "
#define TITLE_RMT_MAX_OAMPDU_SIZE_STR		"Max OAMPDU Size   "
#define TITLE_RMT_VENDOR_SPEC_INFO_STR 	"Vendor Information"
#define TITLE_RMT_PAR_ACTION_STR 				"Parser Action     "
#define TITLE_RMT_MUX_ACTION_STR 				"Multiplexer Action"

#define FUNC_LINKFAULT_STR							"Link Fault    "
#define FUNC_DYINGGASP_STR							"Dying Gasp    "
#define FUNC_CRITICALEVENT_STR					"Critical Event"
#define FUNC_UNIDIRECTIONAL_STR					"Unidirectional"
#define FUNC_LOOPBACK_STR								"LoopBack      "
#define FUNC_LINKEVENT_STR							"Link Event    "
#define FUNC_VARIRETRIEVAL_STR					"Vari Retrieval"

#define LINK_FAULT_REMOTE_DTE_STR				"Remote DTE    "
#define LINK_FAULT_LOCAL_DTE_STR				"Local DTE		 "
#define LINK_FAULT_LOCAL_REMOTE_STR			"Local & Remote DTE"
#define LINK_FAULT_NONE_STR							"None          "

#define STRING_NONE											"None"
#define STRING_ENABLE										"Enable"
#define STRING_DISABLE									"Disable"

#define STRING_LB_CONNECTION						"Connection"
#define STRING_LB_INPROGRESS						"Inprogress"
#define STRING_LB_DISCONNECTION					"Disconnection"

#define STRING_PASSIVE_MODE							"Passive"
#define STRING_ACTIVE_MODE							"Active"
#define STRING_LOOPBACK_MODE						"LoopBack"
#define STRING_ACTIVE_DTE								"Active DTE"
#define STRING_REMOTE_DTE								"Remote DTE"

#define STRING_LINKFAULT								"LINK_FAULT"
#define STRING_PASSIVE_WAIT							"PASSIVE_WAIT"
#define STRING_ACTIVE_SEND_LOCAL				"ACTIVE_SEND_LOCAL"
#define STRING_SEND_LOCAL_REMOTE				"SEND_LOCAL_REMOTE"
#define STRING_SEND_LOCAL_REMOT_OK			"SEND_LOCAL_REMOTE_OK"
#define STRING_OPERATIONAL							"OPERATIONAL"
#define STRING_UNKNOW_STATUS						"UNKNOWN_STATUS"

#define STRING_PARSER_FWD								"FWD"
#define STRING_PARSER_LB								"LB"
#define STRING_PARSER_DISCARD						"Discard"
#define STRING_PARSER_RESERVED					"Reserved"
#define STRING_MUX_FWD									"FWD"
#define STRING_MUX_DISCARD							"Discard"
#define STRING_MUX_RESERVED							"Reserved"

/*-------------------------------------------------------------
  OAM Sublayer Entities
--------------------------------------------------------------- */
#define EOAMPDU_COUNT_PER_SECOND			1
//				/* This counter is used to limit the number of OAMPDUs transmitted per second */
/* anne:
Time checking in eoam use system jiffies as unit.
However, jiffies increasing rate differ from systems and related with HZ.
second = jiffies/HZ, HZ is defined as CONFIG_HZ in kernel .config.
To avoid inconsistency between different project, better use definition as here.
(5*HZ)jiffies=(5)seconds.
*/
#define EOAM_LOCAL_LOST_LINK_TIME  		5*HZ /* Local lost link time value, maxim time of no receive OAM, in jiffies*/
#define EOAM_PDU_TIME                 1*HZ	    /* Local at least send one OAM in PDU_TIME period, in jiffies */
#define ONE_SECOND_JIFFIES            1*HZ

/* Definition for Local OAM LooBack Status */
#define EOAM_LB_DISABLE								0					/* No LoopBack */
#define EOAM_LB_START									1
#define EOAM_LB_EXIT									2
//#define EOAM_LB_ENABLE								1
#define EOAM_LB_START_INPROGRESS			3
#define EOAM_LB_EXIT_INPROGRESS				4

#define	LOCAL_LINK_STATUS_FAIL				0
#define	LOCAL_LINK_STATUS_RUNNING			1

typedef enum {
		EOAM_STATUS_BEGIN = 0,											// 0
		/* OAM sublayer discovery state */
		EOAM_STATUS_DISC_FAULT,											// 1
		EOAM_STATUS_DISC_PASSIVE_WAIT,							// 2
		EOAM_STATUS_DISC_ACTIVE_SEND_LOCAL,					// 3
		EOAM_STATUS_DISC_SEND_LOCAL_REMOTE,					// 4
		EOAM_STATUS_DISC_SEND_LOCAL_REMOTE_SUCCESS,	// 5
		EOAM_STATUS_DISC_SEND_ANY,									// 6
		/* OAM sublayer transmit state */
		EOAM_STATUS_TX_RESET,												// 7
		EOAM_STATUS_TX_WAIT,												// 8
		EOAM_STATUS_TX_DECPDUCNT,										// 9
		EOAM_STATUS_TX_OAMPDU,											// 10
		/* OAM sublayer multiplexer state */
		EOAM_STATUS_MUX_WAIT,												// 11
		EOAM_STATUS_MUX_CHKPHY,											// 12
		EOAM_STATUS_MUX_TXFRAME,										// 13
		/* OAM sublayer parser state */
		EOAM_STATUS_PARS_WAIT,											// 14
		EOAM_STATUS_PARSE,													// 15
		EOAM_STATUS_PARS_RXPDU,											// 16
		EOAM_STATUS_PARS_RXDATA,										// 17
		EOAM_STATUS_PARS_RXLB,											// 18
}enEOAM_STATUS_OPERA_STATE;


/* TX/RX of EOAMPDUs state */
typedef enum {
		LOCAL_PDU_LF_INFO = 0,											/* Only Information OAMPDUs with the Link Fault critical
                                        		   	   link event set and without Information TLVs are allowed
                                        		   	   to be transmitted; only Information OAMPDUsare allowed
                                                   to be received. 																											*/
		LOCAL_PDU_RX_INFO,													/* No OAMPDUs are allowed to be transmitted; only
                                                   Information OAMPDUsare allowed to be received. 											*/
		LOCAL_PDU_INFO,															/* Only Information OAMPDUs are allowed to be transmitted and received 	*/
		LOCAL_PDU_ANY,															/* Any permissible OAMPDU is allowed to be transmitted and received 		*/
		REMOTE_PDU_LF_INFO = 0,
		REMOTE_PDU_RX_INFO,
		REMOTE_PDU_INFO,
		REMOTE_PDU_ANY,
}enEOAMPDU_STATE;

/* Parser function action */
typedef enum {
		LOCAL_PARSER_FWD = 0,												/* Passes received non-OAMPDUs to superior sublayer.												*/
		LOCAL_PARSER_LB,														/* Passes received non-OAMPDUs to Multiplexer during remote loopback test.	*/
		LOCAL_PARSER_DISCARD,												/* Discards received non-OAMPDUs. 																					*/
		REMOTE_PARSER_FWD = 0,
		REMOTE_PARSER_LB,
		REMOTE_PARSER_DISCARD,
}enEOAM_PARSER_ACTION;

/* Mux function action */
typedef enum {
		LOCAL_MUX_FWD = 0,													/* Passes MAC client frames to subordinate sublayer. 	*/
		LOCAL_MUX_DISCARD,													/* Discards MAC client frames. 												*/
		REMOTE_MUX_FWD = 0,
		REMOTE_MUX_DISCARD,
}enEOAM_MUX_ACTION;


/*-------------------------------------------------------------
  OAMPDU Format Definition.
--------------------------------------------------------------- */
/* OAMPDU Flags Field CTL */
#define EOAMPDU_FLAGS_LINKFAULT			WBIT00			/* The PHY has detected a fault has occurred in the receive direction of the local DTE	*/
#define EOAMPDU_FLAGS_DYINGASP			WBIT01			/* An unrecoverable local failure condition */
#define EOAMPDU_FLAGS_CRITICEVT			WBIT02			/* A critical event has occurred 						*/
#define EOAMPDU_FLAGS_LOCALEVAL			WBIT03			/* Local DTE Discovery process 							*/
#define EOAMPDU_FLAGS_LOCALSTABLE		WBIT04			/* Local DTE Discovery process 							*/
#define EOAMPSU_FLAGS_LOCALSTATUS		(WBIT03+WBIT04)
#define EOAMPDU_FLAGS_REMOTEEVAL		WBIT05			/*  */
#define EOAMPDU_FLAGS_REMOTESTABLE	WBIT06			/*  */
#define EOAMPDU_FLAGS_REMOTESTATUS	(WBIT05+WBIT06)
#define EOAMPDU_FLAGS_RFU						WBIT07			/* Reserved for future use */

/* OAMPDU Flags Field - Local Stable and Local Evaluating form a two-bit encoding */
typedef enum {
		LOCAL_DTE_UNSATISFIED = 0,									/* Local DTE Unsatisfied, Discovery can not complete 	*/
		LOCAL_DTE_DISC_UNCOMPLETED,									/* Local DTE Discovery process has not completed 			*/
		LOCAL_DTE_DISC_COMPLETED,										/* Local DTE Discovery process has completed 					*/
		LOCAL_DTE_RFU,															/* Reserved for future use 														*/
}enEOAMPDU_LOCAL_DTE_STATE;

/* OAMPPDU Code Field */
typedef enum {																	/* Table 57¡V4¡XOAMPDU codes */
		CODE_INFO = 0,															/* Communicates local and remote OAM information 	*/
		CODE_EVENT,																	/* Alerts remote DTE of link event(s). 						*/
		CODE_VARREQ,																/* Requests one or more specific MIB variables 		*/
		CODE_VARRESP,																/* Returns one or more specific MIB variables 		*/
		CODE_LOOPBACK_CTL,													/* Enables/disables OAM remote loopback 					*/
		CODE_RFU,																		/* Reserved for future use 												*/
		CODE_ORGSPEC = 0xFE,												/* Organization Specific Extensions 							*/
		//CODE_RFU = 0xFF,													/* Reserved for future use 												*/
}enEOAMPDU_CODE;

/* LoopBack Control OAMPDU Command */
typedef enum {
		REMOTE_LB_RFU = 0,													/* Reserved for future use 			*/
		REMOTE_LB_ENABLE,														/* Enable OAM Remote Loopback 	*/
		REMOTE_LB_DISABLE,													/* Disable OAM Remote Loopback 	*/
}enEOAMPDU_REMOTE_LBCMD;

/*-------------------------------------------------------------
  OAM TLVs
  1 ) 	Information TLV
  2 )		Remote Information TLV
  3 ) 	Organization Specific Information TLV
--------------------------------------------------------------- */
#define INFO_TLV_LEN								0x10  			/* The information TLV length in octets, fixed at 16 			*/
#define LINK_EVENT_TLV_LEN					0x28				/* The link event TLV length in octets, fixed at 40 			*/
#define VARI_REQ_TLV_LEN						0x03				/* The variable request TLV length in octets, fixed at 3 	*/
#define VARI_RESP_TLV_LEN						0x08				/* The variable response TLV length in octets, fixed at 8 */
#define LOOPBACK_TLV_LEN						0x2A				/* The LoopBack TLV length in octets, fixed at 42 */

#define INFO_TLV_OAM_VERSION				0x01				/* For 802.3ah clause 57 */

#define INFO_TLV_OAMPDU_MAXSIZE			0x05EE			/* 1518 */
#define INFO_TLV_OAMPDU_DEFAULT			0x05DC			/* 1500 	*/
#define INFO_TLV_OAMPDU_MINSIZE			0x0040			/* 64 	*/

#define END_OF_TLV_MARKER 					0x0					/* End of TLV marker */
/* Information TLVs Types */
typedef enum {																	/* Table 57¡V6¡XInformation TLV types */
		INFO_TLV_LOCAL_INFO=1,											/* Local Information 									*/
		INFO_TLV_REMOTE_INFO,												/* Remote Information 								*/
		INFO_TLV_RFU,																/* Reserved for future use 						*/
		INFO_TLV_SPEC_INFO=0xFE,										/* Organization Specific Information 	*/
}enEOAMPDU_INFO_TLV_TYPE;

/* Information TLVs State Field Mask CTL */
#define INFO_TLV_STATE_PARSER				(BIT00+BIT01)	/* Parser Action */
#define	INFO_TLV_STATE_MUX					BIT02					/* Multiplexer Action */
#define INFO_TLV_STATE_RFU					(BIT03+BIT04+BIT05+BIT06+BIT07)

/* Information TLVs State Field - PARSER ACTION */
#if 1
typedef enum {
		INFO_TLV_PARSER_FWD = 0,										/* Device is forwarding non-OAMPDUs to higher sublayer 			*/
		INFO_TLV_PARSER_LB,													/* Device is looping back non-OAMPDUs to the lower sublayer */
		INFO_TLV_PARSER_DISCARD,										/* Device is discarding non-OAMPDUs 												*/
		INFO_TLV_PARSER_RFU,												/* Reserved for future use 																	*/
}enINFO_TLV_STATE_PARSER;
#else
#define INFO_TLV_PARSER_FWD 				0x00		/* Device is forwarding non-OAMPDUs to higher sublayer */
#define INFO_TLV_PARSER_LB					BIT00		/* Device is looping back non-OAMPDUs to the lower sublayer */
#define INFO_TLV_PARSER_DISCARD			BIT01		/* Device is discarding non-OAMPDUs */
#define INFO_TLV_PARSER_RFU					(BIT00+BIT01)
#endif

/* Information TLVs State Field - MULTIPLEXER ACTION */
#if 0
typedef enum {
		INFO_TLV_MUX_FWD = 0,						/* Device is forwarding non-OAMPDUs to the lower sublayer */
		INFO_TLV_MUX_DISCARD,						/* Device is discarding non-OAMPDUs */
}enINFO_TLV_STATE_MUX;
#else
#define INFO_TLV_MUX_FWD 						0x00				/* Device is forwarding non-OAMPDUs to the lower sublayer */
#define INFO_TLV_MUX_DISCARD				BIT02				/* Device is discarding non-OAMPDUs */
#endif

/* Information TLVs OAM Configuration Field CTL */
#define INFO_TLV_CONF_ACTIVE				BIT00				/* OAM Mode 										*/
#define	INFO_TLV_CONF_UNIDIRC				BIT01				/* Unidirectional Support 			*/
#define	INFO_TLV_CONF_OAMRMLB				BIT02				/* OAM Remote Loopback Support 	*/
#define INFO_TLV_CONF_LINK					BIT03				/* Link Events 									*/
#define INFO_TLV_CONF_VARRESP				BIT04				/* Variable Retrieval 					*/
#define INFO_TLV_CONF_RFU						(BIT05+BIT06+BIT07)

/* Information TLVs OAMPDU Configuration Field CTL */
#define INFO_TLV_EOAMPDUCONF_MAX		0x07FF			/* Maximum OAMPDU Size 					*/
#define INFO_TLV_EOAMPDUCONF_RFU		0xF800


/* Information TLVs OUI Field Mask CTL */
#define	INFO_TLV_OUI								0x0FFF			/* 24-bit Organizationally Unique Identifier of the vendor */

/* Information TLVs Vendor Specific Information Field Mask CTL */
#define INFO_TLV_VENSPECINFO				0xFFFF			/* Vendor Specific Information */

/* Link Event TLVs Types */
typedef enum {
		LINK_TLV_ERROR_SYMBOL_PERI=1,								/* Errored Symbol Period Event 					*/
		LINK_TLV_ERROR_FRAME,												/* Errored Frame Event 									*/
		LINK_TLV_ERROR_FRAME_PERI,									/* Errored Frame Period Event 					*/
		LINK_TLV_ERROR_FRAME_SEC,										/* Errored Frame Seconds Summary Event 	*/
		LINK_TLV_RFU,																/* Reserved for future use 							*/
		LINK_TLV_ORG_SPEC = 0xFE,										/* Organization Specific Information 		*/
}enLINK_TLV_TYPE;


/*-------------------------------------------------------------
  OAMPDU structure
--------------------------------------------------------------- */
typedef union eoampdu_flag { 										/* Table 57¡V3¡XFlags field */
		short	s16Flags;

		struct {
			short s16Rfu:9;
			short s16RemoteStableEval:2;
			short	s16LocalStableEval:2;
			short	s16CriticEvent:1;
			short	s16DyingGasp:1;
			short	s16LinkFault:1;
		}stFlagBits;

}unEOAMPDU_FLAG;

typedef struct tlvhdr {
		char	s8Type;																/*  */
		char	s8length;															/*  */
		/* unsigned char tlv_value[]; */
}stTLV_HDR;

typedef union info_tlv_state {									/* Table 57¡V7¡XState field */
		char 	s8InfoTlvState;

		struct state_bits {
			char 	s8Rfu:5;
			char 	s8MuxAction:1;
			char	s8ParAction:2;
		}stStateBits;

}unINFO_TLV_STATE;

typedef union info_tlv_oam_cfg { 								/* Table 57¡V8¡XOAM Configuration field */
		char	s8InfoTlvOamCfg;

		struct  {
			char	s8Rfu:3;														/* resverved, be zero*/
			char	s8VarRetrieval:1;										/* DTE supports sending Variable Response OAMPDUs or not		*/
			char	s8LinkEvents:1;											/* DTE supports interpreting Link Events or not							*/
			char	s8RmtLoopSupport:1;	    						/* DTE is capable of OAM remote loopback mode or not				*/
			char	s8UnidirectSupport:1;								/* DTE is capable of sending OAMPDUs when the receive path is non-operational or not*/
			char	s8OamMode:1;												/* DTE configured in Active or Passive mode									*/
		}stCfgBits;

}unINFO_TLV_OAM_CFG;

typedef union info_tlv_oampdu_cfg {							/* Table 57¡V9¡XOAMPDU Configuration field */
		short	s16InfoTlvOamPduCfg;

		struct  {
			short	s16Rfu:5;														/* resverved, be zero	*/
			short	s16MaxOampduSz:11;									/* the largest OAMPDU, in octets, supported by the DTE*/
		}stCfgBits;

}unINFO_TLV_OAMPDU_CFG;

#if 0
typedef union info_tlv_vender_info {				/* Table 57¡V11¡XVendor Specific Information field */
		int		s32VerderSpecInfo;

		struct {
			int		s32VendorDTEType	:16;
			int		s32VendorSWRev		:16;
		}stVenderBits;

}unINFO_TLV_VENDER_INFO;
#else
typedef struct info_tlv_vender_info {						/* Table 57¡V11¡XVendor Specific Information field */
			short	s16VendorDTEType;
			short	s16VendorSWRev;
}stINFO_TLV_VENDER_INFO;
#endif

typedef struct local_info_tlv {
		char	s8InfoType;														/* Information Type 		*/
		char	s8InfoLeng;														/* Information Length 	*/
		char	s8OAMVer;															/* OAM Version 					*/
		char 	s8Rev[2];															/* Revision 						*/
		char	s8InfoState;													/* State 								*/
		char	s8InfoOamCfg;													/* OAM Configuration 		*/
		char	s8InfoOampduCfg[2];										/* OAMPDU Configuration */
		char	s8LocalDTEOUI[3];											/* OUI 									*/

		stINFO_TLV_VENDER_INFO stInfoTlvVenderInfo;
}stLOCAL_INFO_TLV;

typedef struct org_spec_info_tlv {
		char	s8SpecType;														/*  */
		char	s8SpecLeng;														/*  */
		#if 1
		char	s8LocalOUI[3];												/* OUI */
		#else
		char	s8OUIHigh;														/* OUI */
		short	s16OUILow;
		#endif
		char	s8OrgSpecInfo;
}stORG_SPEC_INFO_TLV;

typedef struct error_symbol_peri_tlv {
		char	s8EventType;													/*  */
		char	s8EventLeng;													/*  */
		short	s16EventTimeStamp;										/*  */
		long	s64ErrorWindow;												/*  */
		long	s64ErrorThreshold; 										/*  */
		long	s64ErrorSymbol;												/*  */
		long	s64ErrorTotal;												/*  */
		long	s64EventTotal;												/*  */
}stERROR_SYMBOL_PERI_TLV;

typedef struct error_frame_event_tlv {
		char	s8EventType;													/*  */
		char	s8EventLeng;													/*  */
		short	s16EventTimeStamp;										/*  */
		short	s16ErrorWindow;												/*  */
		int		s32ErrorThreshold; 										/*  */
		int		s32ErrorFrame;												/*  */
		long	s64ErrorTotal;												/*  */
		int		s32EventTotal;												/*  */
}stERROR_FRAME_EVENT_TLV;

typedef struct error_frame_peri_event_tlv {
		char	s8EventType;													/*  */
		char	s8EventLeng;													/*  */
		short	s16EventTimeStamp;										/*  */
		int		s32ErrorWindow;												/*  */
		int		s32ErrorThreshold; 										/*  */
		int		s32ErrorFrame;												/*  */
		long	s64ErrorTotal;												/*  */
		int		s32EventTotal;												/*  */
}stERROR_FRAME_PERI_EVENT_TLV;

typedef struct error_frame_sec_event_tlv {
		char	s8EventType;													/*  */
		char	s8EventLeng;													/*  */
		short	s16EventTimeStamp;										/*  */
		short	s16ErrorWindow;												/*  */
		short	s16ErrorThreshold; 										/*  */
		short	s16ErrorFrameSec;											/*  */
		int		s32ErrorTotal;												/*  */
		int		s32EventTotal;												/*  */
}stERROR_FRAME_SEC_EVENT_TLV;

typedef struct org_spec_event_tlv{
		char	s8EventType;													/*  */
		char	s8EventLeng;													/*  */
		char	s8OUI[3];															/*  */
		char	s8OrgSpecInfo;
}stORG_SPEC_EVENT_TLV;


/*-------------------------------------------------------------
  OAM Sublayer Control Structure
--------------------------------------------------------------- */
typedef struct eoam_layer_ctl{
		/* OAM Event Flags */
		char	s8LocalLinkStatus;										/* 1: Link fault */
		char	s8LocalDyingGasp;											/* 1: Unrecoverable failure */
		char	s8LocalCriticalEvent;									/* 1: Unspecified critical link evt. */
		/* OAM Functions */
		char	s8LocalUnidirectional;								/* 1: Unidirectional support */
		char	s8LocalEOamLoopBack;									/* 1: Loopback function support */
		char	s8LocalEoamLinkEvent;									/* 1: Link events */
		char	s8LocalVariRetrieval;									/* 1: Sending variable response */
		/* OAM Discovery Status */
		char	s8DiscStatus;													/* Discovery status */
		char	s8LocalPdu;														/* Local PDU status*/
		char	s8LocalStable;												/* 1: Local DTE status */
		char	s8RemoteStable;												/* 1: Remote DTE status */
		char	s8LocalSatisfied;											/* 1: Local DTE staisfied with Remote DTE */
		char	s8RemoteStateValid;										/* 1: Wait for Infomation PDU with Local Information TLV */
		char	s8RemoteDTEDiscovered;								/* 1: Found Remote DTE */
		/* OAM System Parameters */
		char	s8LocalEOamInited;										/* 1: EOAM Inited */
		char	s8LocalEOamMode;											/* 1: PASSIVE DTE, 2: ACTIVE DET */
		char	s8LocalEOamLBInited;									/* */
		char	s8LocalParAction;											/* 00: FWD, 01:LB, 11:DISCARD */
		char	s8LocalMuxAction;											/* 0: FWD */
		/* OAM Frame/PDU Parameters */
		short s16LocalFlags;
		char	s8LocalCode;
		char	s8RMTDTEAddr[ETHER_MAC_LEN];					/* Remote DTE MAC Address */
		short	s16RMTFlags;													/* Remote DTE PDU Flags */
		char	s8LocalDTEOUI[3];											/* Local DTE OUI */

		stLOCAL_INFO_TLV	stLocalInfoTlv;						/* TX: OAMPDU Local Information TLV */
		stLOCAL_INFO_TLV	stRMTLocalInfoTlv;				/* RX: OAMPDU Local Information TLV */
		stLOCAL_INFO_TLV	stRMTRemoteInfoTlv;				/* RX: OAMPDU Remote Information TLV */
		stORG_SPEC_INFO_TLV	stRMTSpecInfoTlv;				/* RX: OAMPDU Spec Information TLV */

		char 	*ps8LoopbackBuff;											/* RX: LoopBack mode, non-OAMPDU */
}stEOAM_LAYER_CTL;

/*-------------------------------------------------------------
  IEEE802.3 Slow Protocols Structure. OAMPDU structure see Figure 57-9.
--------------------------------------------------------------- */
typedef struct ether_ptl_header{
		char	s8DestAddr[ETHER_MAC_LEN];						/*  */
		char	s8SrcAddr[ETHER_MAC_LEN];							/*  */
	 	short	s16LengType;													/*  */
}stETHER_PTL_HDR;

typedef struct slow_ptl_header{
		char	s8SubType;														/*  */
		char	s8Flags[2];
		char	s8Code;																/*  */
}stSLOW_PTL_HDR;

typedef struct slow_ptl_comm_header{
		struct ether_ptl_header stEtherHdr;					/*  */
		struct slow_ptl_header stSlowHdr;						/*  */
}stSLOW_PTL_COMM_HDR;


/*-------------------------------------------------------------
  OAMPDU Macro.
--------------------------------------------------------------- */
#define TLV_SET(tlv, type, length) \
		do { \
			(tlv)->tlv_type = (type); \
			(tlv)->tlv_length = sizeof(*tlv) + (length); \
		} while (/*CONSTCOND*/0)

/*-------------------------------------------------------------
  Debug Message
--------------------------------------------------------------- */

typedef enum {
		EOAMAPI_FAIL									= -1,
		EOAM_FAIL 										= 0,					/* EOAM general fail 									*/
		EOAM_SUCCESS									= 1,					/* EOAM general success 							*/
		/* OAM System Parameters */
		EOAM_ERROR_GET_BUFFER					= 2,					/* EAOM get packet buffer fail 				*/
		EOAM_ERROR_REL_BUFFER					= 3,					/* EOAM release packet buffer fial 		*/
		EOAM_ERROR_INIT								= 4,					/* EOAM init fail 										*/
		EOAM_ERROR_TASK_INIT          = 5,					/* EOAM run task fail 								*/
		EOAM_ERROR_WRONG_MODE         = 6,					/* EOAM isn't under correct mode 			*/
		/* OAM Discovery Status */
		EOAM_ERROR_PDU_RXINFO         = 7,					/* EOAM no permission for sent packet */
		/* OAM Received Packets Parameters */
		EOAM_ERROR_INVAILD_PACKET     = 8,					/* EOAM illeagal packet 							*/
		EOAM_ERROR_IGNORED_PACKET     = 9,
		EOAM_ERROR_DISCARD_PACKET     = 10,
		EOAM_ERROR_LOOPBACK_PACKET    = 11,
		EOAM_ERROR_END_TLV            = 12,					/* EOAM end of TLV 										*/

		EOAM_ERROR_MACADDR,													/* EOAM wrong mac address 						*/
		EOAM_ERROR_SUBTYPE,													/* EOAM wrong subtype 								*/
		/* OAM Functions */
		EOAM_ERROR_UNSUPPORT,												/* EOAM not support function 					*/
		EOAM_ERROR_UNSUPPORT_LB,										/* EOAM not support Loopback function */
		EOAM_ERROR_DISCOVERY_UNFINISHED,
		EOAM_ERROR_LOOPBACK_TERMINATE,

}enEOAM_ERROR_CODE;


#define EOAM_DEBUG		FALSE	//TRUE
#define COLOR_DEBUG_MSG	TRUE
#define EOAM_DEBUG_DUMP	TRUE

#if (EOAM_DEBUG == TRUE)
extern void uart_tx_onoff(int onoff);

#define EOAM_MSG(msg)											do{uart_tx_onoff(1); (msg); uart_tx_onoff(0);}while(0);
//#define EOAM_DBG(format, args...)     	iprintf("[%s:%d]"format,__FILE__, __LINE__,##args);
#define EOAM_DBG(format, args...)					iprintf("[%s:%d]"format,__FUNCTION__, __LINE__,##args);
//#define EOAM_DBG(format, args...)     	iprintf("[%s:%d]",__FUNCTION__, __LINE__);iprintf(format,##args);
#else
extern unsigned char gEOAMEnableDebug;
#define EOAM_MSG(msg)	do{ if(gEOAMEnableDebug) (msg); }while(0);
#ifdef SUPERTASK
#define EOAM_DBG(format, args...) dprintf("[%s:%d]"format,__FUNCTION__, __LINE__,##args);
#else // SUPERTASK
#define EOAM_DBG(format, args...) printk("[%s:%d]"format,__FUNCTION__, __LINE__,##args);
#endif // SUPERTASK
#endif

#if (COLOR_DEBUG_MSG == TRUE)
#define CLR0_RESET							"\33[0m"
#define CLR1_30_BLACK						"\33[1;30m"
#define CLR1_31_RED							"\33[1;31m"
#define CLR1_32_GREEN						"\33[1;32m"
#define CLR1_33_YELLOW					"\33[1;33m"
#define CLR1_34_BLUE						"\33[1;34m"
#define CLR1_35_MAGENTA					"\33[1;35m"
#define CLR1_36_CYAN						"\33[1;36m"
#define CLR1_37_WHITE						"\33[1;37m"
#else
#define CLR0_RESET							""
#define CLR1_30_BLACK						""
#define CLR1_31_RED							""
#define CLR1_32_GREEN						""
#define CLR1_33_YELLOW					""
#define CLR1_34_BLUE						""
#define CLR1_35_MAGENTA					""
#define CLR1_36_CYAN						""
#define CLR1_37_WHITE						""
#endif // USB_COLOR_DEBUG_MSG


/*-------------------------------------------------------------
  Function Declaration
--------------------------------------------------------------- */
#define EOAM_BASIC_FUN

#ifdef EOAM_BASIC_FUN
#define EOAM_INTERFACE
#else
#define EOAM_INTERFACE	extern
#endif

EOAM_INTERFACE char gs8LocalEoamEnable;
//EOAM_INTERFACE char gs8LocalEoamBegin;
//EOAM_INTERFACE char gs8LocalEoamMode;
EOAM_INTERFACE stEOAM_LAYER_CTL gstLocalEOamCtl;

//EOAM_INTERFACE void DumpBuffer_RAW(char *ps8buf, int s32len, char *ps8color);
//EOAM_INTERFACE int DumpBuffer_EOAM(char *ps8buf, int s32len, char *ps8color);
#ifdef SUPERTASK
EOAM_INTERFACE int EOAM_PARSE(int s32Ifno, char *ps8PktBuff, int s32BuffLen);
EOAM_INTERFACE int EOAM_HANDLER(int s32Ifno, char *ps8PktBuff, int s32BuffLen);
#else
EOAM_INTERFACE int EOAM_PARSE(int s32Ifno, struct sk_buff *ps8PktBuff, int s32BuffLen);
EOAM_INTERFACE int EOAM_HANDLER(struct sk_buff *ps8PktBuff, int s32BuffLen);
#endif // SUPERTASK
EOAM_INTERFACE int EOAM_INIT(char s8EOamMode);
#ifdef SUPERTASK
EOAM_INTERFACE void EOAM_DISCOVERY_TASK(char s8EOamMode);
#else
EOAM_INTERFACE void EOAM_DISCOVERY_TASK(void *input_data);
#endif // SUPERTASK

EOAM_INTERFACE const int EOAMAPI_LOOPBACK_ENABLE(char s8Enable);
EOAM_INTERFACE const int EOAMAPI_ENABLE(char s8Enable, char s8EOamMode);
EOAM_INTERFACE const int EOAMAPI_GET_EOAM_STATUS(void);
EOAM_INTERFACE const int EOAMAPI_GET_EOAM_MODE(void);
EOAM_INTERFACE const int EOAMAPI_GET_FUNCTIONALITY(int s32EoamFunc);
EOAM_INTERFACE const short EOAMAPI_GET_EOAM_EVENT(char s8LocalRemoteDTE);
EOAM_INTERFACE const char *EOAMAPI_GET_LB_CONNECT(void);
EOAM_INTERFACE const char *EOAMAPI_GET_OPERATIONAL_STATUS_STR(void);
EOAM_INTERFACE const char *EOAMAPI_GET_LINKFAULT_STR(void);
EOAM_INTERFACE const char *EOAMAPI_GET_REMOTE_MODE_STR(void);
EOAM_INTERFACE const int EOAMAPI_GET_REMOTE_INFO(int s32RemoteFunc);
EOAM_INTERFACE const int EOAMAPI_CONSOLE_EOAM_INFO(char s8PageNo);

EOAM_INTERFACE void eoam_exit(void);
EOAM_INTERFACE void SetEOAMBaseIfName(char *pBaseIfName);
#endif //#ifndef _EOAM_H

