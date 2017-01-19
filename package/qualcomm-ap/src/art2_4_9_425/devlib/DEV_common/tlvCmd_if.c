/* tlvCmd_if.c -  contains the ART wrapper functions */

/* Copyright (c) 2000 Atheros Communications, Inc., All Rights Reserved */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include <malloc.h>
#include "wlantype.h"
#include "wlanproto.h"
#define SUPPORT_11N
#include "wlan_defs.h"
#include "athreg.h"
#include "manlib.h"

#include "Device.h"
#include "DevSetConfig.h"
#include "dk_cmds.h"
#include "dk_common.h"

#include "UserPrint.h"
#include "DevConfigDiff.h"
#include "DevNonEepromParameter.h"

#include "art_utf_common.h"
#include "genTxBinCmdTlv.h"
#include "parseRxBinCmdTlv.h"
#include "cmdOpcodes.h"
#include "tlvCmd_if.h"
#include "os_if.h"
#include "tlv_rxParmDef.h"
#include "tlv_txParmDef.h"
#include "testcmd.h"
#include "cmdTxParms.h"
#include "rate_constants.h"
#include "vrate_constants.h"
#include "LinkList.h"

#include "sw_version.h"

#ifdef __LINUX_POWERPC_ARCH__
#include "instance.h"
#endif

#ifdef QDART_BUILD
#include "qmslCmd.h"
#endif 

#ifdef QDART_BUILD
void DbgPrint(char * fmt,...)
{
    va_list marker;
    static char szBuf[1024]; 

    va_start(marker, fmt);
    vsprintf(szBuf, fmt, marker);
    va_end(marker);

    OutputDebugString(szBuf);
}
#endif

#if (defined(LINUX) && !defined(HAVE_STRNICMP))
#define _strnicmp   strncasecmp
#endif
#define MAX_MEM_CMD_BLOCK_SIZE 3000

extern MLD_CONFIG configSetup;

#ifdef _WINDOWS
extern A_BOOL BmiOpeation();
#endif

//  Remote error number and error string
A_INT32 remoteMdkErrNo = 0;
A_CHAR remoteMdkErrStr[SIZE_ERROR_BUFFER];

A_UINT32 fwBoardDataAddress;

// holds the cmd replies sent over channel
static CMD_REPLY cmdReply;

A_UINT32 rateMaskRowDef[RATE_MASK_ROW_MAX] = {0x00000100, 0x00000000}; // 6M only

static A_BOOL cmdInitCalled = FALSE;

typedef struct _TlvParamDefault
{
	unsigned char *key;
	unsigned char data[6];
} TLV_PARAM_DEFAULT;

static TLV_PARAM_DEFAULT TlvTxParamDefaultTbl[] =
{
	{TLV_TXPARM_KEY_CHANNEL, TLV_TXPARM_DEFAULT_CHANNEL},
	{TLV_TXPARM_KEY_TXMODE, TLV_TXPARM_DEFAULT_TXMODE},
#if 0
	{TLV_TXPARM_KEY_RATEMASK0, TLV_TXPARM_DEFAULT_RATEMASK0},
	{TLV_TXPARM_KEY_RATEMASK1, TLV_TXPARM_DEFAULT_RATEMASK1},
    {TLV_TXPARM_KEY_PWRGAINSTART0, TLV_TXPARM_DEFAULT_PWRGAINSTART0},
    {TLV_TXPARM_KEY_PWRGAINSTART1, TLV_TXPARM_DEFAULT_PWRGAINSTART1},
    {TLV_TXPARM_KEY_PWRGAINSTART2, TLV_TXPARM_DEFAULT_PWRGAINSTART2},
    {TLV_TXPARM_KEY_PWRGAINSTART3, TLV_TXPARM_DEFAULT_PWRGAINSTART3},
    {TLV_TXPARM_KEY_PWRGAINSTART4, TLV_TXPARM_DEFAULT_PWRGAINSTART4},
    {TLV_TXPARM_KEY_PWRGAINSTART5, TLV_TXPARM_DEFAULT_PWRGAINSTART5},
    {TLV_TXPARM_KEY_PWRGAINSTART6, TLV_TXPARM_DEFAULT_PWRGAINSTART6},
    {TLV_TXPARM_KEY_PWRGAINSTART7, TLV_TXPARM_DEFAULT_PWRGAINSTART7},
    {TLV_TXPARM_KEY_PWRGAINSTART8, TLV_TXPARM_DEFAULT_PWRGAINSTART8},
    {TLV_TXPARM_KEY_PWRGAINSTART9, TLV_TXPARM_DEFAULT_PWRGAINSTART9},
    {TLV_TXPARM_KEY_PWRGAINSTART10, TLV_TXPARM_DEFAULT_PWRGAINSTART10},
    {TLV_TXPARM_KEY_PWRGAINSTART11, TLV_TXPARM_DEFAULT_PWRGAINSTART11},
    {TLV_TXPARM_KEY_PWRGAINSTART12, TLV_TXPARM_DEFAULT_PWRGAINSTART12},
    {TLV_TXPARM_KEY_PWRGAINSTART13, TLV_TXPARM_DEFAULT_PWRGAINSTART13},
	{TLV_TXPARM_KEY_PWRGAINSTART14, TLV_TXPARM_DEFAULT_PWRGAINSTART14},
    {TLV_TXPARM_KEY_PWRGAINSTART15, TLV_TXPARM_DEFAULT_PWRGAINSTART15},
    {TLV_TXPARM_KEY_PWRGAINEND0, TLV_TXPARM_DEFAULT_PWRGAINEND0},
    {TLV_TXPARM_KEY_PWRGAINEND1, TLV_TXPARM_DEFAULT_PWRGAINEND1},
    {TLV_TXPARM_KEY_PWRGAINEND2, TLV_TXPARM_DEFAULT_PWRGAINEND2},
    {TLV_TXPARM_KEY_PWRGAINEND3, TLV_TXPARM_DEFAULT_PWRGAINEND3},
    {TLV_TXPARM_KEY_PWRGAINEND4, TLV_TXPARM_DEFAULT_PWRGAINEND4},
    {TLV_TXPARM_KEY_PWRGAINEND5, TLV_TXPARM_DEFAULT_PWRGAINEND5},
    {TLV_TXPARM_KEY_PWRGAINEND6, TLV_TXPARM_DEFAULT_PWRGAINEND6},
    {TLV_TXPARM_KEY_PWRGAINEND7, TLV_TXPARM_DEFAULT_PWRGAINEND7},
    {TLV_TXPARM_KEY_PWRGAINEND8, TLV_TXPARM_DEFAULT_PWRGAINEND8},
    {TLV_TXPARM_KEY_PWRGAINEND9, TLV_TXPARM_DEFAULT_PWRGAINEND9},
    {TLV_TXPARM_KEY_PWRGAINEND10, TLV_TXPARM_DEFAULT_PWRGAINEND10},
    {TLV_TXPARM_KEY_PWRGAINEND11, TLV_TXPARM_DEFAULT_PWRGAINEND11},
    {TLV_TXPARM_KEY_PWRGAINEND12, TLV_TXPARM_DEFAULT_PWRGAINEND12},
    {TLV_TXPARM_KEY_PWRGAINEND13, TLV_TXPARM_DEFAULT_PWRGAINEND13},
    {TLV_TXPARM_KEY_PWRGAINEND14, TLV_TXPARM_DEFAULT_PWRGAINEND14},
    {TLV_TXPARM_KEY_PWRGAINEND15, TLV_TXPARM_DEFAULT_PWRGAINEND15},
    {TLV_TXPARM_KEY_PWRGAINSTEP0, TLV_TXPARM_DEFAULT_PWRGAINSTEP0},
    {TLV_TXPARM_KEY_PWRGAINSTEP1, TLV_TXPARM_DEFAULT_PWRGAINSTEP1},
    {TLV_TXPARM_KEY_PWRGAINSTEP2, TLV_TXPARM_DEFAULT_PWRGAINSTEP2},
    {TLV_TXPARM_KEY_PWRGAINSTEP3, TLV_TXPARM_DEFAULT_PWRGAINSTEP3},
    {TLV_TXPARM_KEY_PWRGAINSTEP4, TLV_TXPARM_DEFAULT_PWRGAINSTEP4},
    {TLV_TXPARM_KEY_PWRGAINSTEP5, TLV_TXPARM_DEFAULT_PWRGAINSTEP5},
    {TLV_TXPARM_KEY_PWRGAINSTEP6, TLV_TXPARM_DEFAULT_PWRGAINSTEP6},
    {TLV_TXPARM_KEY_PWRGAINSTEP7, TLV_TXPARM_DEFAULT_PWRGAINSTEP7},
    {TLV_TXPARM_KEY_PWRGAINSTEP8, TLV_TXPARM_DEFAULT_PWRGAINSTEP8},
    {TLV_TXPARM_KEY_PWRGAINSTEP9, TLV_TXPARM_DEFAULT_PWRGAINSTEP9},
    {TLV_TXPARM_KEY_PWRGAINSTEP10, TLV_TXPARM_DEFAULT_PWRGAINSTEP10},
    {TLV_TXPARM_KEY_PWRGAINSTEP11, TLV_TXPARM_DEFAULT_PWRGAINSTEP11},
    {TLV_TXPARM_KEY_PWRGAINSTEP12, TLV_TXPARM_DEFAULT_PWRGAINSTEP12},
    {TLV_TXPARM_KEY_PWRGAINSTEP13, TLV_TXPARM_DEFAULT_PWRGAINSTEP13},
    {TLV_TXPARM_KEY_PWRGAINSTEP14, TLV_TXPARM_DEFAULT_PWRGAINSTEP14},
    {TLV_TXPARM_KEY_PWRGAINSTEP15, TLV_TXPARM_DEFAULT_PWRGAINSTEP15},
#endif //0
    {TLV_TXPARM_KEY_ANTENNA, TLV_TXPARM_DEFAULT_ANTENNA},
    {TLV_TXPARM_KEY_ENANI, TLV_TXPARM_DEFAULT_ENANI},
    {TLV_TXPARM_KEY_SCRAMBLEROFF, TLV_TXPARM_DEFAULT_SCRAMBLEROFF},
    {TLV_TXPARM_KEY_AIFSN, TLV_TXPARM_DEFAULT_AIFSN},
    {TLV_TXPARM_KEY_PKTSZ, TLV_TXPARM_DEFAULT_PKTSZ},
    {TLV_TXPARM_KEY_TXPATTERN, TLV_TXPARM_DEFAULT_TXPATTERN},
    {TLV_TXPARM_KEY_SHORTGUARD, TLV_TXPARM_DEFAULT_SHORTGUARD},
    {TLV_TXPARM_KEY_NUMPACKETS, TLV_TXPARM_DEFAULT_NUMPACKETS},
    {TLV_TXPARM_KEY_WLANMODE, TLV_TXPARM_DEFAULT_WLANMODE},
    {TLV_TXPARM_KEY_TXCHAIN0, TLV_TXPARM_DEFAULT_TXCHAIN0},
    {TLV_TXPARM_KEY_TXCHAIN1, TLV_TXPARM_DEFAULT_TXCHAIN1},
	{TLV_TXPARM_KEY_TXCHAIN2, TLV_TXPARM_DEFAULT_TXCHAIN2},
    {TLV_TXPARM_KEY_TXCHAIN3, TLV_TXPARM_DEFAULT_TXCHAIN3},
    {TLV_TXPARM_KEY_TPCM, TLV_TXPARM_DEFAULT_TPCM},
    {TLV_TXPARM_KEY_FLAGS, TLV_TXPARM_DEFAULT_FLAGS},
    {TLV_TXPARM_KEY_AGG, TLV_TXPARM_DEFAULT_AGG},
    {TLV_TXPARM_KEY_BROADCAST, TLV_TXPARM_DEFAULT_BROADCAST},
    {TLV_TXPARM_KEY_BANDWIDTH, TLV_TXPARM_DEFAULT_BANDWIDTH},
    {TLV_TXPARM_KEY_BSSID, TLV_TXPARM_DEFAULT_BSSID},
    {TLV_TXPARM_KEY_TXSTATION, TLV_TXPARM_DEFAULT_TXSTATION},
    {TLV_TXPARM_KEY_RXSTATION, TLV_TXPARM_DEFAULT_RXSTATION},
    {TLV_TXPARM_KEY_RESERVED, TLV_TXPARM_DEFAULT_RESERVED},
    {TLV_TXPARM_KEY_DUTYCYCLE, TLV_TXPARM_DEFAULT_DUTYCYCLE},
    {TLV_TXPARM_KEY_NPATTERN, TLV_TXPARM_DEFAULT_NPATTERN},
    {TLV_TXPARM_KEY_RESERVED1, TLV_TXPARM_DEFAULT_RESERVED1},
    {TLV_TXPARM_KEY_DATAPATTERN, TLV_TXPARM_DEFAULT_DATAPATTERN},
    {TLV_TXPARM_KEY_RATEBITINDEX0, TLV_TXPARM_DEFAULT_RATEBITINDEX0},
    {TLV_TXPARM_KEY_RATEBITINDEX1, TLV_TXPARM_DEFAULT_RATEBITINDEX1},
    {TLV_TXPARM_KEY_RATEBITINDEX2, TLV_TXPARM_DEFAULT_RATEBITINDEX2},
    {TLV_TXPARM_KEY_RATEBITINDEX3, TLV_TXPARM_DEFAULT_RATEBITINDEX3},
    {TLV_TXPARM_KEY_RATEBITINDEX4, TLV_TXPARM_DEFAULT_RATEBITINDEX4},
    {TLV_TXPARM_KEY_RATEBITINDEX5, TLV_TXPARM_DEFAULT_RATEBITINDEX5},
    {TLV_TXPARM_KEY_RATEBITINDEX6, TLV_TXPARM_DEFAULT_RATEBITINDEX6},
    {TLV_TXPARM_KEY_RATEBITINDEX7, TLV_TXPARM_DEFAULT_RATEBITINDEX7},
    {TLV_TXPARM_KEY_TXPOWER0, TLV_TXPARM_DEFAULT_TXPOWER0},
	{TLV_TXPARM_KEY_TXPOWER1, TLV_TXPARM_DEFAULT_TXPOWER1},
    {TLV_TXPARM_KEY_TXPOWER2, TLV_TXPARM_DEFAULT_TXPOWER2},
    {TLV_TXPARM_KEY_TXPOWER3, TLV_TXPARM_DEFAULT_TXPOWER3},
    {TLV_TXPARM_KEY_TXPOWER4, TLV_TXPARM_DEFAULT_TXPOWER4},
	{TLV_TXPARM_KEY_TXPOWER5, TLV_TXPARM_DEFAULT_TXPOWER5},
    {TLV_TXPARM_KEY_TXPOWER6, TLV_TXPARM_DEFAULT_TXPOWER6},
    {TLV_TXPARM_KEY_TXPOWER7, TLV_TXPARM_DEFAULT_TXPOWER7},
    {TLV_TXPARM_KEY_PKTLEN0, TLV_TXPARM_DEFAULT_PKTLEN0},
    {TLV_TXPARM_KEY_PKTLEN1, TLV_TXPARM_DEFAULT_PKTLEN1},
    {TLV_TXPARM_KEY_PKTLEN2, TLV_TXPARM_DEFAULT_PKTLEN2},
    {TLV_TXPARM_KEY_PKTLEN3, TLV_TXPARM_DEFAULT_PKTLEN3},
    {TLV_TXPARM_KEY_PKTLEN4, TLV_TXPARM_DEFAULT_PKTLEN4},
    {TLV_TXPARM_KEY_PKTLEN5, TLV_TXPARM_DEFAULT_PKTLEN5},
    {TLV_TXPARM_KEY_PKTLEN6, TLV_TXPARM_DEFAULT_PKTLEN6},
    {TLV_TXPARM_KEY_PKTLEN7, TLV_TXPARM_DEFAULT_PKTLEN7},
#if 0
    {TLV_TXPARM_KEY_AGGPERRATE0, TLV_TXPARM_DEFAULT_AGGPERRATE0},
    {TLV_TXPARM_KEY_AGGPERRATE1, TLV_TXPARM_DEFAULT_AGGPERRATE1},
    {TLV_TXPARM_KEY_AGGPERRATE2, TLV_TXPARM_DEFAULT_AGGPERRATE2},
    {TLV_TXPARM_KEY_AGGPERRATE3, TLV_TXPARM_DEFAULT_AGGPERRATE3},
    {TLV_TXPARM_KEY_AGGPERRATE4, TLV_TXPARM_DEFAULT_AGGPERRATE4},
    {TLV_TXPARM_KEY_AGGPERRATE5, TLV_TXPARM_DEFAULT_AGGPERRATE5},
    {TLV_TXPARM_KEY_AGGPERRATE6, TLV_TXPARM_DEFAULT_AGGPERRATE6},
    {TLV_TXPARM_KEY_AGGPERRATE7, TLV_TXPARM_DEFAULT_AGGPERRATE7},
#endif //0
    {TLV_TXPARM_KEY_IR, TLV_TXPARM_DEFAULT_IR},
#if 0
    {TLV_TXPARM_KEY_GAINSTART, TLV_TXPARM_DEFAULT_GAINSTART},
    {TLV_TXPARM_KEY_GAINSTEP, TLV_TXPARM_DEFAULT_GAINSTEP},
    {TLV_TXPARM_KEY_GAINEND, TLV_TXPARM_DEFAULT_GAINEND},
    {TLV_TXPARM_KEY_RESERVED2, TLV_TXPARM_DEFAULT_RESERVED2},
    {TLV_TXPARM_KEY_RATEMASK11AC0, TLV_TXPARM_DEFAULT_RATEMASK11AC0},
    {TLV_TXPARM_KEY_RATEMASK11AC1, TLV_TXPARM_DEFAULT_RATEMASK11AC1},
    {TLV_TXPARM_KEY_RATEMASK11AC2, TLV_TXPARM_DEFAULT_RATEMASK11AC2},
    {TLV_TXPARM_KEY_RATEMASK11AC3, TLV_TXPARM_DEFAULT_RATEMASK11AC3},
    {TLV_TXPARM_KEY_PWRGAIN11AC0, TLV_TXPARM_DEFAULT_PWRGAIN11AC0},
    {TLV_TXPARM_KEY_PWRGAIN11AC1, TLV_TXPARM_DEFAULT_PWRGAIN11AC1},
    {TLV_TXPARM_KEY_PWRGAIN11AC2, TLV_TXPARM_DEFAULT_PWRGAIN11AC2},
    {TLV_TXPARM_KEY_PWRGAIN11AC3, TLV_TXPARM_DEFAULT_PWRGAIN11AC3},
    {TLV_TXPARM_KEY_PWRGAIN11AC4, TLV_TXPARM_DEFAULT_PWRGAIN11AC4},
    {TLV_TXPARM_KEY_PWRGAIN11AC5, TLV_TXPARM_DEFAULT_PWRGAIN11AC5},
    {TLV_TXPARM_KEY_PWRGAIN11AC6, TLV_TXPARM_DEFAULT_PWRGAIN11AC6},
    {TLV_TXPARM_KEY_PWRGAIN11AC7, TLV_TXPARM_DEFAULT_PWRGAIN11AC7},
    {TLV_TXPARM_KEY_PWRGAIN11AC8, TLV_TXPARM_DEFAULT_PWRGAIN11AC8},
    {TLV_TXPARM_KEY_PWRGAIN11AC9, TLV_TXPARM_DEFAULT_PWRGAIN11AC9},
    {TLV_TXPARM_KEY_PWRGAIN11AC10, TLV_TXPARM_DEFAULT_PWRGAIN11AC10},
    {TLV_TXPARM_KEY_PWRGAIN11AC11, TLV_TXPARM_DEFAULT_PWRGAIN11AC11},
    {TLV_TXPARM_KEY_PWRGAIN11AC12, TLV_TXPARM_DEFAULT_PWRGAIN11AC12},
    {TLV_TXPARM_KEY_PWRGAIN11AC13, TLV_TXPARM_DEFAULT_PWRGAIN11AC13},
    {TLV_TXPARM_KEY_PWRGAIN11AC14, TLV_TXPARM_DEFAULT_PWRGAIN11AC14},
    {TLV_TXPARM_KEY_PWRGAIN11AC15, TLV_TXPARM_DEFAULT_PWRGAIN11AC15},
    {TLV_TXPARM_KEY_PWRGAIN11AC16, TLV_TXPARM_DEFAULT_PWRGAIN11AC16},
    {TLV_TXPARM_KEY_PWRGAIN11AC17, TLV_TXPARM_DEFAULT_PWRGAIN11AC17},
    {TLV_TXPARM_KEY_PWRGAIN11AC18, TLV_TXPARM_DEFAULT_PWRGAIN11AC18},
    {TLV_TXPARM_KEY_PWRGAIN11AC19, TLV_TXPARM_DEFAULT_PWRGAIN11AC19},
    {TLV_TXPARM_KEY_PWRGAIN11AC20, TLV_TXPARM_DEFAULT_PWRGAIN11AC20},
    {TLV_TXPARM_KEY_PWRGAIN11AC21, TLV_TXPARM_DEFAULT_PWRGAIN11AC21},
    {TLV_TXPARM_KEY_PWRGAIN11AC22, TLV_TXPARM_DEFAULT_PWRGAIN11AC22},
    {TLV_TXPARM_KEY_PWRGAIN11AC23, TLV_TXPARM_DEFAULT_PWRGAIN11AC23},
    {TLV_TXPARM_KEY_PWRGAIN11AC24, TLV_TXPARM_DEFAULT_PWRGAIN11AC24},
    {TLV_TXPARM_KEY_PWRGAIN11AC25, TLV_TXPARM_DEFAULT_PWRGAIN11AC25},
    {TLV_TXPARM_KEY_PWRGAIN11AC26, TLV_TXPARM_DEFAULT_PWRGAIN11AC26},
    {TLV_TXPARM_KEY_PWRGAIN11AC27, TLV_TXPARM_DEFAULT_PWRGAIN11AC27},
    {TLV_TXPARM_KEY_PWRGAIN11AC28, TLV_TXPARM_DEFAULT_PWRGAIN11AC28},
    {TLV_TXPARM_KEY_PWRGAIN11AC29, TLV_TXPARM_DEFAULT_PWRGAIN11AC29},
#endif //0
	{TLV_TXPARM_KEY_GAINIDX, TLV_TXPARM_DEFAULT_GAINIDX},
	{TLV_TXPARM_KEY_DACGAIN, TLV_TXPARM_DEFAULT_DACGAIN},
};

static TLV_PARAM_DEFAULT TlvRxParamDefaultTbl[] =
{
	{TLV_RXPARM_KEY_CHANNEL, TLV_RXPARM_DEFAULT_CHANNEL},
	{TLV_RXPARM_KEY_RXMODE, TLV_RXPARM_DEFAULT_RXMODE},
	{TLV_RXPARM_KEY_ENANI, TLV_RXPARM_DEFAULT_ENANI},
    {TLV_RXPARM_KEY_ANTENNA, TLV_RXPARM_DEFAULT_ANTENNA},
    {TLV_RXPARM_KEY_WLANMODE, TLV_RXPARM_DEFAULT_WLANMODE},
    {TLV_RXPARM_KEY_RXCHAIN, TLV_RXPARM_DEFAULT_RXCHAIN},
    {TLV_RXPARM_KEY_EXPECTEDPACKETS, TLV_RXPARM_DEFAULT_EXPECTEDPACKETS},
    {TLV_RXPARM_KEY_ACK, TLV_RXPARM_DEFAULT_ACK},
    {TLV_RXPARM_KEY_BROARDCAST, TLV_RXPARM_DEFAULT_BROARDCAST},
    {TLV_RXPARM_KEY_BANDWIDTH, TLV_RXPARM_DEFAULT_BANDWIDTH},
    {TLV_RXPARM_KEY_LPL, TLV_RXPARM_DEFAULT_LPL},
    {TLV_RXPARM_KEY_ANTSWITCH1, TLV_RXPARM_DEFAULT_ANTSWITCH1},
    {TLV_RXPARM_KEY_ANTSWITCH2, TLV_RXPARM_DEFAULT_ANTSWITCH2},
    {TLV_RXPARM_KEY_ADDR, TLV_RXPARM_DEFAULT_ADDR},
    {TLV_RXPARM_KEY_BSSID, TLV_RXPARM_DEFAULT_BSSID},
    {TLV_RXPARM_KEY_BTADDR, TLV_RXPARM_DEFAULT_BTADDR},
    {TLV_RXPARM_KEY_RESERVED, TLV_RXPARM_DEFAULT_RESERVED},
    {TLV_RXPARM_KEY_REGDMN0, TLV_RXPARM_DEFAULT_REGDMN0},
    {TLV_RXPARM_KEY_REGDMN1, TLV_RXPARM_DEFAULT_REGDMN1},
    {TLV_RXPARM_KEY_OTPWRITEFLAG, TLV_RXPARM_DEFAULT_OTPWRITEFLAG},
    {TLV_RXPARM_KEY_FLAGS, TLV_RXPARM_DEFAULT_FLAGS},
    {TLV_RXPARM_KEY_RATEMASK0, TLV_RXPARM_DEFAULT_RATEMASK0},
    {TLV_RXPARM_KEY_RATEMASK1, TLV_RXPARM_DEFAULT_RATEMASK1},
    {TLV_RXPARM_KEY_RATEMASK2, TLV_RXPARM_DEFAULT_RATEMASK2},
    {TLV_RXPARM_KEY_RATEMASK3, TLV_RXPARM_DEFAULT_RATEMASK3},
    {TLV_RXPARM_KEY_RATEMASK4, TLV_RXPARM_DEFAULT_RATEMASK4},
    {TLV_RXPARM_KEY_RATEMASK5, TLV_RXPARM_DEFAULT_RATEMASK5},
};



int getTlvParams (A_UINT8 *key, A_UINT8 *getBuf, A_UINT32 maxLen)
{
	A_UINT8 *data;
	A_UINT32 len;

	if (getParams(key, &data, &len) == FALSE)
	{
		UserPrint ("getTlvParams - Error in getting %s\n", key);
		return 1;
	}
	else if (len > maxLen)
	{
		UserPrint ("getTlvParams - returned length is too big\n");
		return 1;
	}
	else
	{
		memcpy(getBuf, data, len);
	}
	return 0;
}

int getTlvDataParams (A_UINT8 *key, A_UINT8 *getBuf, A_UINT32 maxLen, A_UINT32 dataLen)
{
	A_UINT8 *data;
	A_UINT32 len;

	if (dataLen > maxLen)
	{
		UserPrint ("getTlvDataParams - buffer size is too small\n");
		return 1;
	}

	if (getParams(key, &data, &len) == FALSE)
	{
		UserPrint ("getTlvParams - Error in getting %s\n", key);
		return 1;
	}
	else if (len > maxLen)
	{
		UserPrint ("getTlvParams - returned length is too big\n");
		return 1;
	}
	else if (len < dataLen)
	{
		UserPrint ("getTlvParams - returned length is too small\n");
		return 1;
	}
	else
	{
		memcpy(getBuf, data, dataLen);
	}
	return 0;
}

void cmdReplyGeneric ()
{
    int error = 0;

    error = getTlvParams((A_UINT8*)"status", (A_UINT8*)&cmdReply.status, sizeof(cmdReply.status));
    // make the reply id to 0xffffffff
    cmdReply.replyCmdId = 0xffffffff;
}

void cmdReplyGenericNart ()
{
    //A_UINT32 i;
    A_UINT32 dataLen;
    int error = 0;

    error = getTlvParams((A_UINT8*)"status", (A_UINT8*)&cmdReply.status, sizeof(cmdReply.status));
    if (error)
    {
       return;
    }

    error = getTlvParams((A_UINT8*)"commandId", (A_UINT8*)&cmdReply.replyCmdId, sizeof(cmdReply.replyCmdId));
    if (error)
    {
        return;
    }

    error = getTlvParams((A_UINT8*)"length", (A_UINT8*)&dataLen, sizeof(dataLen));
    if (error)
    {
        return;
    }

 	if (dataLen)
 	{
    	error = getTlvDataParams((A_UINT8*)"data", (A_UINT8*)&cmdReply.cmdBytes, sizeof(cmdReply.cmdBytes), dataLen);
    	if (error)
    	{
        	return;
    	}
 	}


	cmdReply.replyCmdLen = dataLen + sizeof(A_UINT32) * 2; 	//length of: cmdId+status+data

#if 0
    printf("Reply: dataLen = %u, cmdId = %u, status = %u\n", dataLen, cmdReply.replyCmdId, cmdReply.status);
    for(i = 0; i < dataLen; i++)
    {
        if(i%16==0) printf("\n");
        printf("%02x ", cmdReply.cmdBytes[i]);
    }
    printf("\n");
#endif
    return;
}

void cmdReplyTxStatus()
{
    A_UINT32 numOfReports;
    TX_STATS_STRUCT_UTF *pTxStats = (TX_STATS_STRUCT_UTF *)&cmdReply.cmdBytes[4];
    int error;

    cmdReply.replyCmdId = M_TX_DATA_STATUS_CMD_ID;
    cmdReply.status = CMD_OK;
    numOfReports = 0;

    error = getTlvParams((A_UINT8*)"numOfReports", (A_UINT8*)&numOfReports, sizeof(numOfReports));
    if (error) return;

    // reply length = 4-byte cmdID + 4-byte status + 4-byte numOfReports + status reports
    cmdReply.replyCmdLen = numOfReports * sizeof(TX_STATS_STRUCT_UTF) + sizeof(A_UINT32)*3;
    memcpy (cmdReply.cmdBytes, &numOfReports, sizeof(A_UINT32));

    if (numOfReports)
    {
	    while (numOfReports)
	    {
    		error += getTlvParams((A_UINT8*)"totalPackets", (A_UINT8*)&pTxStats->totalPackets, sizeof(pTxStats->totalPackets));
    		error += getTlvParams((A_UINT8*)"goodPackets",(A_UINT8*)&pTxStats->goodPackets, sizeof(pTxStats->goodPackets));
    		error += getTlvParams((A_UINT8*)"underruns",(A_UINT8*)&pTxStats->underruns, sizeof(pTxStats->underruns));
    		error += getTlvParams((A_UINT8*)"otherError",(A_UINT8*)&pTxStats->otherError, sizeof(pTxStats->otherError));
			error += getTlvParams((A_UINT8*)"excessRetries",(A_UINT8*)&pTxStats->excessiveRetries, sizeof(pTxStats->excessiveRetries));
   			error += getTlvParams((A_UINT8*)"rateBit",(A_UINT8*)&pTxStats->rateBit, sizeof(pTxStats->rateBit));

    		error += getTlvParams((A_UINT8*)"shortRetry",(A_UINT8*)&pTxStats->shortRetry, sizeof(pTxStats->shortRetry));
    		error += getTlvParams((A_UINT8*)"longRetry",(A_UINT8*)&pTxStats->longRetry, sizeof(pTxStats->longRetry));

    		error += getTlvParams((A_UINT8*)"startTime",(A_UINT8*)&pTxStats->startTime, sizeof(pTxStats->startTime));
    		error += getTlvParams((A_UINT8*)"endTime",(A_UINT8*)&pTxStats->endTime, sizeof(pTxStats->endTime));

    		error += getTlvParams((A_UINT8*)"byteCount",(A_UINT8*)&pTxStats->byteCount, sizeof(pTxStats->byteCount));
    		error += getTlvParams((A_UINT8*)"dontCount",(A_UINT8*)&pTxStats->dontCount, sizeof(pTxStats->dontCount));

    		error += getTlvParams((A_UINT8*)"rssi",(A_UINT8*)&pTxStats->rssi, sizeof(pTxStats->rssi));

    		error += getTlvParams((A_UINT8*)"rssic0",(A_UINT8*)&pTxStats->rssic[0], sizeof(pTxStats->rssic[0]));
    		error += getTlvParams((A_UINT8*)"rssic1",(A_UINT8*)&pTxStats->rssic[1], sizeof(pTxStats->rssic[1]));
    		error += getTlvParams((A_UINT8*)"rssic2",(A_UINT8*)&pTxStats->rssic[2], sizeof(pTxStats->rssic[2]));

    		error += getTlvParams((A_UINT8*)"rssie0",(A_UINT8*)&pTxStats->rssie[0], sizeof(pTxStats->rssie[0]));
    		error += getTlvParams((A_UINT8*)"rssie1",(A_UINT8*)&pTxStats->rssie[1], sizeof(pTxStats->rssie[1]));
    		error += getTlvParams((A_UINT8*)"rssie2",(A_UINT8*)&pTxStats->rssie[2], sizeof(pTxStats->rssie[2]));

    		error += getTlvParams((A_UINT8*)"thermCal",(A_UINT8*)&pTxStats->thermCal, sizeof(pTxStats->thermCal));

			if (error)
			{
    			cmdReply.status = COMMS_ERROR_TLV_GET_PARAM_FAIL;
    			return;
			}
			pTxStats++;
			numOfReports--;
	    }
    }
	return;
}

static void cmdReplyRxStatus()
{
    A_UINT32 numOfReports;
    RX_STATS_STRUCT_UTF *pRxStats = (RX_STATS_STRUCT_UTF *)&cmdReply.cmdBytes[4];
    int error;

    cmdReply.replyCmdId = M_RX_DATA_STATUS_CMD_ID;
    cmdReply.status = CMD_OK;
    numOfReports = 0;

    error = getTlvParams((A_UINT8*)"numOfReports", (A_UINT8*)&numOfReports, sizeof(numOfReports));
    if (error) return;

    // reply length = 4-byte cmdID + 4-byte status + 4-byte numOfReports + status reports
    cmdReply.replyCmdLen = numOfReports * sizeof(RX_STATS_STRUCT_UTF) + sizeof(A_UINT32)*3;
    memcpy (cmdReply.cmdBytes, &numOfReports, sizeof(A_UINT32));

    if (numOfReports)
    {
	    while (numOfReports)
	    {
    		error += getTlvParams((A_UINT8*)"totalPackets", (A_UINT8*)&pRxStats->totalPackets, sizeof(pRxStats->totalPackets));
    		error += getTlvParams((A_UINT8*)"goodPackets",(A_UINT8*)&pRxStats->goodPackets, sizeof(pRxStats->goodPackets));
    		error += getTlvParams((A_UINT8*)"otherError",(A_UINT8*)&pRxStats->otherError, sizeof(pRxStats->otherError));
    		error += getTlvParams((A_UINT8*)"crcPackets",(A_UINT8*)&pRxStats->crcPackets, sizeof(pRxStats->crcPackets));
			error += getTlvParams((A_UINT8*)"decrypErrors",(A_UINT8*)&pRxStats->decrypErrors, sizeof(pRxStats->decrypErrors));
   			error += getTlvParams((A_UINT8*)"rateBit",(A_UINT8*)&pRxStats->rateBit, sizeof(pRxStats->rateBit));

    		error += getTlvParams((A_UINT8*)"startTime",(A_UINT8*)&pRxStats->startTime, sizeof(pRxStats->startTime));
    		error += getTlvParams((A_UINT8*)"endTime",(A_UINT8*)&pRxStats->endTime, sizeof(pRxStats->endTime));

    		error += getTlvParams((A_UINT8*)"byteCount",(A_UINT8*)&pRxStats->byteCount, sizeof(pRxStats->byteCount));
    		error += getTlvParams((A_UINT8*)"dontCount",(A_UINT8*)&pRxStats->dontCount, sizeof(pRxStats->dontCount));

    		error += getTlvParams((A_UINT8*)"rssi",(A_UINT8*)&pRxStats->rssi, sizeof(pRxStats->rssi));

    		error += getTlvParams((A_UINT8*)"rssic0",(A_UINT8*)&pRxStats->rssic[0], sizeof(pRxStats->rssic[0]));
    		error += getTlvParams((A_UINT8*)"rssic1",(A_UINT8*)&pRxStats->rssic[1], sizeof(pRxStats->rssic[1]));
    		error += getTlvParams((A_UINT8*)"rssic2",(A_UINT8*)&pRxStats->rssic[2], sizeof(pRxStats->rssic[2]));

    		error += getTlvParams((A_UINT8*)"rssie0",(A_UINT8*)&pRxStats->rssie[0], sizeof(pRxStats->rssie[0]));
    		error += getTlvParams((A_UINT8*)"rssie1",(A_UINT8*)&pRxStats->rssie[1], sizeof(pRxStats->rssie[1]));
    		error += getTlvParams((A_UINT8*)"rssie2",(A_UINT8*)&pRxStats->rssie[2], sizeof(pRxStats->rssie[2]));

    		error += getTlvParams((A_UINT8*)"evm0",(A_UINT8*)&pRxStats->evm[0], sizeof(pRxStats->evm[0]));
    		error += getTlvParams((A_UINT8*)"evm1",(A_UINT8*)&pRxStats->evm[1], sizeof(pRxStats->evm[1]));
    		error += getTlvParams((A_UINT8*)"evm2",(A_UINT8*)&pRxStats->evm[2], sizeof(pRxStats->evm[2]));

			error += getTlvParams((A_UINT8*)"badrssic0",(A_UINT8*)&pRxStats->badrssic[0], sizeof(pRxStats->badrssic[0]));
    		error += getTlvParams((A_UINT8*)"badrssic1",(A_UINT8*)&pRxStats->badrssic[1], sizeof(pRxStats->badrssic[1]));
    		error += getTlvParams((A_UINT8*)"badrssic2",(A_UINT8*)&pRxStats->badrssic[2], sizeof(pRxStats->badrssic[2]));

    		error += getTlvParams((A_UINT8*)"badrssie0",(A_UINT8*)&pRxStats->badrssie[0], sizeof(pRxStats->badrssie[0]));
    		error += getTlvParams((A_UINT8*)"badrssie1",(A_UINT8*)&pRxStats->badrssie[1], sizeof(pRxStats->badrssie[1]));
    		error += getTlvParams((A_UINT8*)"badrssie2",(A_UINT8*)&pRxStats->badrssie[2], sizeof(pRxStats->badrssie[2]));

    		error += getTlvParams((A_UINT8*)"badevm0",(A_UINT8*)&pRxStats->badevm[0], sizeof(pRxStats->badevm[0]));
    		error += getTlvParams((A_UINT8*)"badevm1",(A_UINT8*)&pRxStats->badevm[1], sizeof(pRxStats->badevm[1]));
    		error += getTlvParams((A_UINT8*)"badevm2",(A_UINT8*)&pRxStats->badevm[2], sizeof(pRxStats->badevm[2]));

			if (error)
			{
    			cmdReply.status = COMMS_ERROR_TLV_GET_PARAM_FAIL;
    			return;
			}
			pRxStats++;
			numOfReports--;
	    }
    }
	return;
}

/**************************************************************************
* receiveCmdReturn - Callback function Get the return info from a command sent
*
* Read from the pipe and get the info returned from a command.	######Need to
* make this timeout if don't receive reply, but don't know how to do that
* yet
*
*
*/
void receiveCmdReturn (void *buf)
{
    A_UINT32 length;
    A_UINT8 *reply = (A_UINT8*)buf;
    A_UINT8 responseOpCode = _OP_GENERIC_NART_RSP;
    A_BOOL ret = FALSE;

    memset(&cmdReply, 0, sizeof(cmdReply));

    length = *(A_UINT32 *)&(reply[0]);

#if 0
    {
        int i;
        A_UINT8 *cmdS=&reply[4];
        printf("TLV length got %u\n",length);
        for (i=0; i < length; i++)
        {
            if (i % 16 == 0) printf("\n");
            printf("0x%02x ", cmdS[i]);
        }
        printf ("\n");
    }
#endif

    ret = initResponse(&reply[4], length,&responseOpCode);

    if ( ret == FALSE )
    {
        printf("Error in initResponse()\n");
        return;
    }

    if ( responseOpCode == _OP_GENERIC_NART_RSP )
    {
	    cmdReplyGenericNart();
    }
    else if ( responseOpCode == _OP_TX_STATUS )
    {
	    cmdReplyTxStatus();
    }
    else if ( responseOpCode == _OP_RX_STATUS )
    {
	    cmdReplyRxStatus();
    }
    // This is for _OP_TX and _OP_RX response
    else if ( responseOpCode == _OP_GENERIC_RSP )
    {
	    cmdReplyGeneric();
    }
    else
    {
        printf ("cmdReplyFunc - Invalid response opcode %d\n", responseOpCode);
    }
}

/**************************************************************************
* artSendCmd - Send a command to a dk client
*
* Construct a command to send to the mdk client.
* Wait for a return struct to verify that the command was successfully
* excuted.
*
* In :
* pCmdStruct:	pointer to structure containing cmd info
* cmdSize:	size of the command structure
* Out:
* returnCmdStruct: pointer to structure returned by cmd
* RETURNS: TRUE if command was successful, FALSE if it was not
*/
A_BOOL artSendCmd (A_UINT8 *pCmdStruct, A_UINT32 cmdSize, A_UINT32 cmdId, void **returnCmdStruct)
{
    A_BOOL			bGoodWrite = FALSE;
    A_UINT16		errorNo;
    char buf[2048 + 8];
#ifdef QDART_BUILD
	unsigned char Qreply[2048+8];
	cmdInitCalled = TRUE;
#endif

    //printf("artSendCmd - cmdID = %u\n", cmdId);
    memset(buf, 0, sizeof(buf));

    if (cmdInitCalled == FALSE)
    {
#ifdef __LINUX_POWERPC_ARCH__
	if (pcie==2)
    		errorNo = cmd_init("wifi1",receiveCmdReturn);
	else
    		errorNo = cmd_init("wifi0",receiveCmdReturn);
#else
    	errorNo = cmd_init("wifi0",receiveCmdReturn);
#endif
#define NART_DKDOWNLOAD
#if (defined _WINDOWS) && (defined NART_DKDOWNLOAD)
		if (cmd_Art2GetInitStatus() == 0)
		{
			if (BmiOpeation() == FALSE)
			{
				UserPrint("Error in BMI loading!!!\n");
				return FALSE;
			}
		}
#endif
    	cmdInitCalled = TRUE;
    }

#ifdef QDART_BUILD
    memcpy(&buf[0],pCmdStruct,cmdSize);
    if (!cmd_send_Qdart(buf,cmdSize,1, Qreply))
		return FALSE;
	receiveCmdReturn(Qreply);
#else
    memcpy(&buf[8],pCmdStruct,cmdSize);
    cmd_send(buf,cmdSize,1);
#endif

#ifdef PEREGRINE_FPGA
    MyDelay(100);
#endif
    // disconnect and close don't expect ANY return (not even the ID ack)
    // so we check for that here
    if ( (DISCONNECT_PIPE_CMD_ID == cmdId) || (CLOSE_PIPE_CMD_ID == cmdId) )
	{
		//milliSleep(1000);
		return TRUE;
	}

    // check to see if the command ID's match.	if they don't, error
	if ((cmdReply.replyCmdId != 0xffffffff) && (cmdId != cmdReply.replyCmdId) &&
		(!((cmdId == M_TX_DATA_STOP_CMD_ID && cmdReply.replyCmdId == M_TX_DATA_STATUS_CMD_ID) ||
			(cmdId == M_RX_DATA_STOP_CMD_ID && cmdReply.replyCmdId == M_RX_DATA_STATUS_CMD_ID))))
	{
       	UserPrint("Error: client reply to command has mismatched ID!\n");
       	UserPrint("     : sent cmdID: %d, returned: %d, size %d\n", cmdId, cmdReply.replyCmdId, cmdReply.replyCmdLen);
       	return FALSE;
    }

    remoteMdkErrNo = 0;
    errorNo = (A_UINT16) (cmdReply.status & COMMS_ERR_MASK) >> COMMS_ERR_SHIFT;
    if (errorNo == COMMS_ERR_MDK_ERROR)
    {
        remoteMdkErrNo = (cmdReply.status & COMMS_ERR_INFO_MASK) >> COMMS_ERR_INFO_SHIFT;
        strncpy(remoteMdkErrStr,(const char *)cmdReply.cmdBytes,SIZE_ERROR_BUFFER);
        UserPrint("Error: COMMS error MDK error for command %d\n", cmdId);
        return TRUE;
    }

    // check for a bad status in the command reply
    if (errorNo != CMD_OK)
	{
        UserPrint("Error: Bad return status (%d) in client command %d response!\n", errorNo, cmdId);
        return FALSE;
    }

    if (!returnCmdStruct)
	{
		//commands have _OP_GENERIC_RSP for response. Only status is returned in this response so no need to check for the length
		if (cmdReply.replyCmdId == 0xffffffff)
		{
			return TRUE;
		}
        // see if the length of the reply makes sense
        if ( cmdReply.replyCmdLen != (sizeof(cmdReply.replyCmdId) + sizeof(cmdReply.status)) )
		{
            UserPrint("Error: Invalid # of bytes in client command %d response!\n", cmdId);
            return FALSE;
        }
        return TRUE;
    }

    // reply must be OK, return pointer to additional reply info
    *returnCmdStruct = cmdReply.cmdBytes;
    return TRUE;
}

A_BOOL artSendCmd2( A_UINT8 *pCmdStruct, A_UINT32 cmdSize, unsigned char* responseCharBuf, unsigned int *responseSize )
{
#ifdef _WINDOWS
    A_BOOL			bGoodWrite = FALSE;
    A_UINT16		errorNo;
    char buf[2048 + 8];

    memset(buf, 0, sizeof(buf));

    if (cmdInitCalled == FALSE)
    {
    	errorNo = cmd_init("wifi0",receiveCmdReturn);
#define NART_DKDOWNLOAD
#if (defined _WINDOWS) && (defined NART_DKDOWNLOAD)
		printf( "performing cmd_Art2GetInitStatus() \n" );
		if (cmd_Art2GetInitStatus() == 0)
		{
			printf( "performing BmiOpeation()\n" );
			if (BmiOpeation() == FALSE)
			{
				UserPrint("Error in BMI loading!!!\n");
				return FALSE;
			}
		}
#endif
    	cmdInitCalled = TRUE;
    }

    memcpy(&buf[8],pCmdStruct,cmdSize);

	cmd_send2( buf, cmdSize, responseCharBuf, responseSize );

    remoteMdkErrNo = 0;
    errorNo = (A_UINT16) (cmdReply.status & COMMS_ERR_MASK) >> COMMS_ERR_SHIFT;
    if (errorNo == COMMS_ERR_MDK_ERROR)
    {
        remoteMdkErrNo = (cmdReply.status & COMMS_ERR_INFO_MASK) >> COMMS_ERR_INFO_SHIFT;
        strncpy(remoteMdkErrStr,(const char *)cmdReply.cmdBytes,SIZE_ERROR_BUFFER);
		UserPrint("Error: COMMS error MDK error for command DONT_CARE\n" );
        return TRUE;
    }

    // check for a bad status in the command reply
    if (errorNo != CMD_OK)
	{
		UserPrint("Error: Bad return status (%d) in client command DONT_CARE response!\n", errorNo);
        return FALSE;
    }

    return TRUE;
#else
	// Other OS not supported at this time
	return FALSE;
#endif
}


int art_initF2()
{
    A_UINT32 *pRegValue;
    A_UINT32 swVersion;
    A_UINT32 whichDevice;
    A_UINT32 cmdId;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;

    whichDevice = 1;

    cmdId = INIT_F2_CMD_ID;
    createCommand(_OP_GENERIC_NART_CMD);
    addParameterToCommand((A_UINT8 *)"commandId",(A_UINT8 *)&cmdId);
    addParameterToCommand((A_UINT8 *)"param1",(A_UINT8*)&whichDevice);
    commandComplete(&rCmdStream, &cmdStreamLen );
    ////UserPrint("..stream len %u\n",cmdStreamLen);

    if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, (void **)&pRegValue))
    {
        UserPrint("Error: Unable to send command INIT_F2_CMD_ID to client! Handle not created.\n");
        return -1;
    }
    //devNum = *pRegValue & 0x0fffffff;
    //thin_client = (A_BOOL)((*pRegValue) >> 28);
    swVersion = *(pRegValue+3);
    configSetup.SwVersion = swVersion;
    UserPrint("SW version %d.%d.%d build %d\n",
                (swVersion & VER_MAJOR_BIT_MASK) >> VER_MAJOR_BIT_OFFSET,
                (swVersion & VER_MINOR_BIT_MASK) >> VER_MINOR_BIT_OFFSET,
                (swVersion & VER_PATCH_BIT_MASK) >> VER_PATCH_BIT_OFFSET,
                 (swVersion & VER_BUILD_NUM_BIT_MASK) >> VER_BUILD_NUM_BIT_OFFSET);
    // Check SW version
    if (configSetup.checkSwVer && (swVersion != SOC_SW_VERSION))
    {
        // If not local build then error
        if (((swVersion & VER_BUILD_NUM_BIT_MASK) != 9999) && ((SOC_SW_VERSION & VER_BUILD_NUM_BIT_MASK) != 9999))
        {
            UserPrint("Error: This program supports SW version %d.%d.%d build %d. Unexpected results might happen\n",
                  __VER_MAJOR_, __VER_MINOR_, __VER_PATCH_, __BUILD_NUMBER_);
            return -1;
        }
    }
    fwBoardDataAddress = *(pRegValue+4);
    return A_OK;
}

void art_teardownDevice ()
{
    A_UINT32 cmdId;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;

	cmdId = M_CLOSE_DEVICE_CMD_ID;
    createCommand(_OP_GENERIC_NART_CMD);
    addParameterToCommand((A_UINT8 *)"commandId",(A_UINT8 *)&cmdId);
    commandComplete(&rCmdStream, &cmdStreamLen );
    ////UserPrint("..stream len %u\n",cmdStreamLen);

    if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, NULL))
    {
        UserPrint("Error: problem sending CLOSE_DEVICE cmd to client!\n");
    }

	cmdId = DISCONNECT_PIPE_CMD_ID;
    createCommand(_OP_GENERIC_NART_CMD);
    addParameterToCommand((A_UINT8 *)"commandId",(A_UINT8 *)&cmdId);
    commandComplete(&rCmdStream, &cmdStreamLen );
    ////UserPrint("..stream len %u\n",cmdStreamLen);

    if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, NULL))
	{
        UserPrint("Error: problem sending DISCONNECT cmd to client in teardownDevice()!\n");
    }
    //pArtPrimarySock = NULL;
	if (cmd_end())
	{
		UserPrint("Error: problem closing driver\n");
	}
	cmdInitCalled = FALSE;
}

int art_whalResetDevice(A_UCHAR *mac, A_UCHAR *bss, A_UINT32 freq, A_INT32 bandwidth, A_UINT16 rxchain, A_UINT16 txchain)
{
    A_UINT32 cmdId, wlanMode;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;

	// Push config differences if any
    ConfigDiffExecute();
    // Push cal data to DUT if any
    CalInfoExecute();

    //if a quarter channel has been requested, then add the flag onto turbo
    //do it here, so can contain the changes to one place
    //if(configSetup.quarterChannel) {
    //    freq = freq * 10 + 25;
    //}
	// Setup the command
    //memcpy(Cmd.mac, mac, 6);
    //memcpy(Cmd.bss, bss, 6);
    //Cmd.freq = freq;
    //Cmd.turbo = turbo;
    //Cmd.rxchain = rxchain;
    //Cmd.txchain = txchain;
#if 0
    if (rateMaskRow == NULL)
    {
        rateMaskRow = rateMaskRowDef;
    }
    // 11G
    if (freq < 4000)
    {
        wlanMode = MODE_11G;
        if ((rateMaskRow[0] & 0xffff0000) || (rateMaskRow[1] != 0))
        {
            wlanMode = ht40 ? MODE_11NG_HT40 : MODE_11NG_HT20;
        }
    }
    else //11A
    {
        wlanMode = MODE_11A;
        if ((rateMaskRow[0] & 0xffff0000) || (rateMaskRow[1] != 0))
        {
            wlanMode = ht40 ? MODE_11NA_HT40 : MODE_11NA_HT20;
        }
    }
#endif //0
	if (bandwidth == BW_VHT80_0) 
	{
		wlanMode = TCMD_WLAN_MODE_VHT80_0;
	}
	else if (bandwidth == BW_VHT80_1)
	{
		wlanMode = TCMD_WLAN_MODE_VHT80_1;
	}
    else if (bandwidth == BW_VHT80_2)
	{
		wlanMode = TCMD_WLAN_MODE_VHT80_2;
	}
	else if (bandwidth == BW_VHT80_3)
	{
		wlanMode = TCMD_WLAN_MODE_VHT80_3;
	}
	else if (bandwidth == BW_HT40_PLUS)
	{
		wlanMode = TCMD_WLAN_MODE_HT40PLUS;
	}
	else if (bandwidth == BW_HT40_MINUS)
	{
		wlanMode = TCMD_WLAN_MODE_HT40MINUS;
	}
	else
	{
		wlanMode = TCMD_WLAN_MODE_HT20;
	}

    // create cmd to send to client
    cmdId = M_RESET_DEVICE_CMD_ID;
    createCommand(_OP_GENERIC_NART_CMD);
    addParameterToCommand((A_UINT8 *)"commandId",(A_UINT8 *)&cmdId);
    addParameterToCommand((A_UINT8 *)"param1",(A_UINT8 *)&freq);
    addParameterToCommand((A_UINT8 *)"param2",(A_UINT8 *)&wlanMode);
    //addParameterToCommand((A_UINT8 *)"param3",(A_UINT8 *)&turbo);

    commandComplete(&rCmdStream, &cmdStreamLen );
    ////UserPrint("..stream len %u\n",cmdStreamLen);

    if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, NULL))
    {
        UserPrint("Error: Unable to successfully send RESET_DEVICE_CMD command to client, exiting!!\n");
        return A_ERROR;
    }
    return A_OK;
}

A_UINT32 art_regRead (A_UINT32 regOffset)
{
    A_UINT32 *pRegValue;
    A_UINT32 cmdId;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;

    /* create cmd structure and send command */
    cmdId = REG_READ_CMD_ID;
    createCommand(_OP_GENERIC_NART_CMD);
    addParameterToCommand((A_UINT8 *)"commandId",(A_UINT8 *)&cmdId);
    addParameterToCommand((A_UINT8 *)"param1",(A_UINT8 *)&regOffset);
    commandComplete(&rCmdStream, &cmdStreamLen );
    ////UserPrint("..stream len %u\n",cmdStreamLen);

    if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, (void **)&pRegValue))
    {
        UserPrint("Error: Unable to successfully send REG_READ command\n");
        return 0xdeadbeef;
    }
    return(*pRegValue);
}

int art_regWrite (A_UINT32 regOffset, A_UINT32 regValue)
{
    A_UINT32 cmdId;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;

    /* create cmd structure and send command */
    cmdId = REG_WRITE_CMD_ID;
    createCommand(_OP_GENERIC_NART_CMD);
    addParameterToCommand((A_UINT8 *)"commandId",(A_UINT8 *)&cmdId);
    addParameterToCommand((A_UINT8 *)"param1",(A_UINT8 *)&regOffset);
    addParameterToCommand((A_UINT8 *)"param2",(A_UINT8 *)&regValue);
    commandComplete(&rCmdStream, &cmdStreamLen );
    //UserPrint("..stream len %u\n",cmdStreamLen);

    if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, NULL))
    {
        UserPrint("Error: Unable to successfully send REG_WRITE command\n");
        return A_ERROR;
    }
    return A_OK;
}

A_UINT32 art_mem32Read (A_UINT32 regAddr)
{
    A_UINT32 *pReadValues;
    A_UINT32 cmdId, size;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;

    /* create cmd structure and send command */
    cmdId = MEM_READ_CMD_ID;
	size = 32;
    createCommand(_OP_GENERIC_NART_CMD);
    addParameterToCommand((A_UINT8 *)"commandId", (A_UINT8 *)&cmdId);
    addParameterToCommand((A_UINT8 *)"param1", (A_UINT8 *)&regAddr);
    addParameterToCommand((A_UINT8 *)"param2", (A_UINT8 *)&size);
    commandComplete(&rCmdStream, &cmdStreamLen );
    //UserPrint("..stream len %u\n", cmdStreamLen);

    if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, (void **)&pReadValues))
    {
        UserPrint("Error: Unable to successfully send MEM_READ command\n");
        return 0xdeadbeef;
    }
    return(*pReadValues);
}

A_UINT32 art_mem32Write (A_UINT32 regAddr, A_UINT32 regValue)
{
    A_UINT32 cmdId, size;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;

    /* create cmd structure and send command */
    cmdId = MEM_WRITE_CMD_ID;
	size = 32;
    createCommand(_OP_GENERIC_NART_CMD);
    addParameterToCommand((A_UINT8 *)"commandId", (A_UINT8 *)&cmdId);
    addParameterToCommand((A_UINT8 *)"param1", (A_UINT8 *)&regAddr);
    addParameterToCommand((A_UINT8 *)"param2", (A_UINT8 *)&regValue);
    addParameterToCommand((A_UINT8 *)"param3", (A_UINT8 *)&size);
    commandComplete(&rCmdStream, &cmdStreamLen );
    //UserPrint("..stream len %u\n", cmdStreamLen);

    if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, NULL))
    {
        UserPrint("Error: Unable to successfully send MEM_WRITE command\n");
        return A_ERROR;
    }
    return A_OK;
}

static int mem_read_block_2048 (A_UINT32 physAddr, A_UINT32 length, A_UCHAR  *buf)
{
    A_UINT32 *pReadValues;
    A_UINT32 cmdId;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;

    /* check to see if the size will make us bigger than the send buffer */
    if (length > MAX_BLOCK_BYTES)
	{
         UserPrint("Error: block size too large, can only write %x bytes\n", MAX_BLOCK_BYTES);
         return(0);
    }

    /* create cmd structure and send command */
    cmdId = MEM_READ_BLOCK_CMD_ID;
    createCommand(_OP_GENERIC_NART_CMD);
    addParameterToCommand((A_UINT8 *)"commandId", (A_UINT8 *)&cmdId);
    addParameterToCommand((A_UINT8 *)"param1", (A_UINT8 *)&physAddr);
    addParameterToCommand((A_UINT8 *)"param2", (A_UINT8 *)&length);
    commandComplete(&rCmdStream, &cmdStreamLen );
    //UserPrint("..stream len %u\n", cmdStreamLen);

    if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, (void **)&pReadValues))
    {
        UserPrint("Error: Unable to send command to client! Handle not created.\n");
        return A_ERROR;
    }
    memcpy(buf, pReadValues, length);
    return A_OK;
}

int art_memRead (A_UINT32 physAddr, A_UCHAR  *bytesRead, A_UINT32 length)
{
    A_UINT32 ii, startAddr_ii, len_ii, arrayStart_ii;
    A_UINT32 retAddr=0;

    // Split the writes into blocks of 2048 bytes only if the memory is already allocated
    ii = length;
    startAddr_ii= physAddr;
    arrayStart_ii=0;

    while( ii > 0)
    {
		len_ii = (ii > MAX_MEM_CMD_BLOCK_SIZE) ? MAX_MEM_CMD_BLOCK_SIZE : ii;
		if ((mem_read_block_2048(startAddr_ii, len_ii, ((A_UCHAR *)bytesRead+arrayStart_ii)))==A_ERROR)
        {
            return A_ERROR;
        }
        startAddr_ii += len_ii;
        ii -= len_ii;
        arrayStart_ii += len_ii;
    }
    return A_OK;
}

int art_memWrite (A_UINT32 physAddr, A_UCHAR  *buf, A_UINT32 length)
{
    A_UINT32 cmdId;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;

    /* check to see if the size will make us bigger than the send buffer */
    if (length > MAX_BLOCK_BYTES)
	{
         UserPrint("Error: block size too large, can only write %x bytes\n", MAX_BLOCK_BYTES);
         return(0);
    }

    /* create cmd structure and send command */
    cmdId = MEM_WRITE_BLOCK_CMD_ID;
    createCommand(_OP_GENERIC_NART_CMD);
    addParameterToCommand((A_UINT8 *)"commandId", (A_UINT8 *)&cmdId);
    addParameterToCommand((A_UINT8 *)"param1", (A_UINT8 *)&length);
    addParameterToCommand((A_UINT8 *)"param2", (A_UINT8 *)&physAddr);
    addParameterToCommandWithLen((A_UINT8 *)"data", buf, (A_UINT16)length);
    commandComplete(&rCmdStream, &cmdStreamLen );
    //UserPrint("..stream len %u\n",cmdStreamLen);

    if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, NULL))
    {
        UserPrint("Error: Unable to send command to client! Handle not created.\n");
        return A_ERROR;
    }
    return A_OK;
}

A_UINT32 art_cfgRead (A_UINT32 regOffset)
{
    A_UINT32 *pRegValue;
    A_UINT32 cmdId, size;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;

	/* create cmd structure and send command */
    cmdId = CFG_READ_CMD_ID;
	size = 32;
    createCommand(_OP_GENERIC_NART_CMD);
    addParameterToCommand((A_UINT8 *)"commandId", (A_UINT8 *)&cmdId);
    addParameterToCommand((A_UINT8 *)"param1", (A_UINT8 *)&regOffset);
    addParameterToCommand((A_UINT8 *)"param2", (A_UINT8 *)&size);
    commandComplete(&rCmdStream, &cmdStreamLen );
    //UserPrint("..stream len %u\n", cmdStreamLen);

    if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, (void **)&pRegValue))
    {
        UserPrint("Error: Unable to successfully send CFG_READ command\n");
        return 0xdeadbeef;
    }
    return(*pRegValue);
}

int art_cfgWrite (A_UINT32 regOffset, A_UINT32 regValue)
{
    A_UINT32 cmdId, size;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;

    /* create cmd structure and send command */
    cmdId = CFG_WRITE_CMD_ID;
	size = 32;
    createCommand(_OP_GENERIC_NART_CMD);
    addParameterToCommand((A_UINT8 *)"commandId", (A_UINT8 *)&cmdId);
    addParameterToCommand((A_UINT8 *)"param1", (A_UINT8 *)&regOffset);
    addParameterToCommand((A_UINT8 *)"param2", (A_UINT8 *)&regValue);
    addParameterToCommand((A_UINT8 *)"param3", (A_UINT8 *)&size);
    commandComplete(&rCmdStream, &cmdStreamLen );
    //UserPrint("..stream len %u\n", cmdStreamLen);

    if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, NULL))
    {
            UserPrint("Error: Unable to successfully send CFG_WRITE command\n");
            return A_ERROR;
    }
    return A_OK;
}


int art_eepromWriteItems(unsigned int numOfItems, unsigned char *pBuffer, unsigned int length)
{
    A_UINT32 cmdId, cmdSize;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;
	A_UINT8 *pBuf;

	if (numOfItems == 0 || pBuffer == NULL || length == 0)
	{
		return A_OK;
	}

	// create cmd to send to client
    cmdId = M_EEPROM_WRITE_ITEMS_CMD_ID;
	cmdSize = length + sizeof(numOfItems);
	pBuf = (A_UINT8 *)malloc(cmdSize);
	memcpy (pBuf, pBuffer, length);
    createCommand(_OP_GENERIC_NART_CMD);
    addParameterToCommand((A_UINT8 *)"commandId", (A_UINT8 *)&cmdId);
	addParameterToCommand((A_UINT8 *)"param1", (A_UINT8 *)&numOfItems);
    addParameterToCommandWithLen((A_UINT8 *)"data", pBuf,length);
	free (pBuf);
    commandComplete(&rCmdStream, &cmdStreamLen );
    //UserPrint("..stream len %u\n",cmdStreamLen);

    if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, NULL))
    {
        UserPrint("Error: Unable to successfully send M_EEPROM_WRITE_ITEMS command to client!\n");
        return A_ERROR;
    }
    return A_OK;
}

int art_stickyWrite(int numOfRegs, unsigned char *pBuffer, unsigned int length)
{
    A_UINT32 cmdId, cmdSize;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;
    A_UINT8 *pBuf;

	if (numOfRegs == 0 || pBuffer == NULL || length == 0)
	{
		return A_OK;
	}

	// create cmd to send to client
    cmdId = M_STICKY_WRITE_CMD_ID;
	cmdSize = length + sizeof(numOfRegs);
	pBuf = (A_UINT8 *)malloc(cmdSize);
	memcpy (pBuf, &numOfRegs, sizeof(numOfRegs));
	memcpy (pBuf+sizeof(numOfRegs), pBuffer, length);

	createCommand(_OP_GENERIC_NART_CMD);
    addParameterToCommand((A_UINT8 *)"commandId", (A_UINT8 *)&cmdId);
    addParameterToCommand((A_UINT8 *)"param1", (A_UINT8 *)&cmdSize);
    addParameterToCommand((A_UINT8 *)"param2", (A_UINT8 *)&numOfRegs);
    addParameterToCommandWithLen((A_UINT8 *)"data", pBuf, (A_UINT16)cmdSize);
	free (pBuf);
    commandComplete(&rCmdStream, &cmdStreamLen );
    //UserPrint("..stream len %u\n",cmdStreamLen);
    if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, NULL))
    {
        UserPrint("Error: Unable to successfully send M_STICKY_WRITE command to client!\n");
        return A_ERROR;
    }
    return A_OK;
}

int art_stickyClear(int numOfRegs, unsigned char *pBuffer, unsigned int length)
{
    A_UINT32 cmdId, cmdSize;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;
    A_UINT8 *pBuf;

	if (numOfRegs == 0)
	{
		return A_OK;
	}

	// create cmd to send to client
    cmdId = M_STICKY_CLEAR_CMD_ID;
	cmdSize = length + sizeof(numOfRegs);
	pBuf = (A_UINT8 *)malloc(cmdSize);
	memcpy (pBuf, &numOfRegs, sizeof(numOfRegs));
	memcpy (pBuf+sizeof(numOfRegs), pBuffer, length);

	createCommand(_OP_GENERIC_NART_CMD);
    addParameterToCommand((A_UINT8 *)"commandId", (A_UINT8 *)&cmdId);
    addParameterToCommand((A_UINT8 *)"param1", (A_UINT8 *)&cmdSize);
    addParameterToCommand((A_UINT8 *)"param2", (A_UINT8 *)&numOfRegs);
    addParameterToCommandWithLen((A_UINT8 *)"data", pBuf, (A_UINT16)cmdSize);
    commandComplete(&rCmdStream, &cmdStreamLen );
    //UserPrint("..stream len %u\n",cmdStreamLen);
	free (pBuf);

    if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, NULL))
    {
        UserPrint("Error: Unable to successfully send M_STICKY_CLEAR command to client!\n");
        return A_ERROR;
    }
    return A_OK;
}

int art_otpWrite (A_UCHAR *buf, A_UINT32 length)
{
    A_UINT32 cmdId;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;

    /* check to see if the size will make us bigger than the send buffer */
    if (length > MAX_BLOCK_BYTES) {
        UserPrint("Error: block size too large, can only write %x bytes\n", MAX_BLOCK_BYTES);
        return A_OK;
    }

    /* create cmd structure and send command */
    cmdId = OTP_WRITE_CMD_ID;
	createCommand(_OP_GENERIC_NART_CMD);
    addParameterToCommand((A_UINT8 *)"commandId", (A_UINT8 *)&cmdId);
    addParameterToCommand((A_UINT8 *)"param1", (A_UINT8 *)&length);
    addParameterToCommandWithLen((A_UINT8 *)"data", buf, (A_UINT16)length);

	commandComplete(&rCmdStream, &cmdStreamLen );
    //UserPrint("..stream len %u\n",cmdStreamLen);

    if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, NULL))
	{
        UserPrint("Error: Unable to successfully send OTP_WRITE_CMD command\n");
        return A_ERROR;
    }
    return A_OK;
}

int art_otpRead (A_UCHAR *buf, A_UINT32 *length)
{
    A_UCHAR  *pReadValues;
    A_UINT32 cmdId, cmdSize;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;

    /* create cmd structure and send command */
    cmdId = OTP_READ_CMD_ID;
    cmdSize = OTPSTREAM_MAXSZ_APP;
	createCommand(_OP_GENERIC_NART_CMD);
    addParameterToCommand((A_UINT8 *)"commandId", (A_UINT8 *)&cmdId);
    addParameterToCommand((A_UINT8 *)"param1", (A_UINT8 *)&cmdSize);

	commandComplete(&rCmdStream, &cmdStreamLen );
    //UserPrint("..stream len %u\n",cmdStreamLen);

    if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, (void **)&pReadValues))
	{
        UserPrint("Error: Unable to successfully send OTP_READ_CMD command\n");
        return A_ERROR;
    }
    *length = *((A_UINT32 *)pReadValues);
    if (*length)
	{
        memcpy(buf, (A_UCHAR *)(pReadValues + 4), *length);
    }
    return A_OK;
}

int art_otpReset (enum otpstream_op_app resetCmd)
{
    A_UINT32 *pRet;
    A_UINT32 cmdId, value;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;

    /* create cmd structure and send command */
    cmdId = OTP_RESET_CMD_ID;
    value = (A_UINT32)resetCmd;
	createCommand(_OP_GENERIC_NART_CMD);
    addParameterToCommand((A_UINT8 *)"commandId", (A_UINT8 *)&cmdId);
    addParameterToCommand((A_UINT8 *)"param1", (A_UINT8 *)&value);

	commandComplete(&rCmdStream, &cmdStreamLen );
    //UserPrint("..stream len %u\n",cmdStreamLen);

    if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, (void **)&pRet))
	{
        UserPrint("Error: Unable to successfully send OTP_RESET_CMD command %d\n", resetCmd);
        return A_ERROR;
    }
    return A_OK;
}

int art_efuseRead (A_UCHAR  *buf, A_UINT32 *length, A_UINT32 startPos)
{
    A_UINT32 size, expectedSize, readLen, count;
    A_UCHAR  *pReadValues;
    A_UINT32 cmdId;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;

    if (startPos >= EFUSE_MAX_NUM_BYTES)
    {
        UserPrint ("art_efuseRead - invalid address (0x%x); should be less than 0x%x\n", startPos, EFUSE_MAX_NUM_BYTES);
        return A_ERROR;
    }

    count = *length;
    if (startPos+count > EFUSE_MAX_NUM_BYTES)
    {
        count = EFUSE_MAX_NUM_BYTES - startPos;
        UserPrint ("art_efuseRead - length (%d) exceeds EFUSE_MAX_NUM_BYTES (%d). Adjust length to %d\n", *length, EFUSE_MAX_NUM_BYTES, count);
    }

    readLen = 0;
    cmdId = EFUSE_READ_CMD_ID;
    while (count)
    {
        expectedSize = (count > 0xff) ? 0xff : count;

        /* create cmd structure and send command */
		createCommand(_OP_GENERIC_NART_CMD);
		addParameterToCommand((A_UINT8 *)"commandId", (A_UINT8 *)&cmdId);
		addParameterToCommand((A_UINT8 *)"param1", (A_UINT8 *)&expectedSize);
		addParameterToCommand((A_UINT8 *)"param2", (A_UINT8 *)&startPos);

		commandComplete(&rCmdStream, &cmdStreamLen );
		//UserPrint("..stream len %u\n",cmdStreamLen);

		if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, (void **)&pReadValues))
        {
            UserPrint("Error: Unable to successfully send EFUSE_READ_CMD command for %d bytes\n", expectedSize);
            return A_ERROR;
        }
        size = *((A_UCHAR *)pReadValues);
        if (size != expectedSize)
        {
            UserPrint ("art_efuseRead - size = %d; expectedSize = %d\n\t\t%d bytes have been read\n", size, expectedSize, readLen);
            count = size; //to terminate while loop
        }
        memcpy (buf+readLen, (A_UCHAR *)(pReadValues +1), size);
        count -= size;
        startPos += size;
        readLen += size;
    }
    *length = readLen;
    return A_OK;
}

int art_efuseWrite (A_UCHAR *buf, A_UINT32 length, A_UINT32 startPos)
{
    A_UINT32  writtenLen, size, maxSize;
    A_UINT32 cmdId;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;

    if (startPos >= EFUSE_MAX_NUM_BYTES)
    {
        UserPrint ("art_efuseWrite - invalid address (0x%x); should be less than 0x%x\n", startPos, EFUSE_MAX_NUM_BYTES);
        return A_ERROR;
    }
    if (length+startPos > EFUSE_MAX_NUM_BYTES)
    {
        UserPrint ("art_efuseWrite - length (%d) exceeds EFUSE_MAX_NUM_BYTES (%d). Adjust length to %d\n", length, EFUSE_MAX_NUM_BYTES, (length = EFUSE_MAX_NUM_BYTES - startPos));
    }
    writtenLen = 0;
    maxSize = 16;

    cmdId = EFUSE_WRITE_CMD_ID;
    while (length)
    {
        size = (length > maxSize) ? maxSize : length;

        /* create cmd structure and send command */
		createCommand(_OP_GENERIC_NART_CMD);
		addParameterToCommand((A_UINT8 *)"commandId", (A_UINT8 *)&cmdId);
		addParameterToCommand((A_UINT8 *)"param1", (A_UINT8 *)&size);
		addParameterToCommand((A_UINT8 *)"param2", (A_UINT8 *)&startPos);
		addParameterToCommandWithLen((A_UINT8 *)"data", buf+writtenLen, (A_UINT16)size);

		commandComplete(&rCmdStream, &cmdStreamLen );
		//UserPrint("..stream len %u\n",cmdStreamLen);

		if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, NULL))
        {
            UserPrint("Error: Unable to successfully send EFUSE_WRITE_CMD command for %d bytes\n", size);
            return A_ERROR;
        }
        writtenLen += size;
        startPos += size;
        length -= size;
    }
    return A_OK;
}


//
// Tell UTF to load OTP
//
int art_otpLoad (A_UINT32 value)
{
    A_UINT32 cmdId;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;

	// create cmd to send to client
    cmdId = OTP_LOAD_CMD_ID;
	createCommand(_OP_GENERIC_NART_CMD);
	addParameterToCommand((A_UINT8 *)"commandId", (A_UINT8 *)&cmdId);
	addParameterToCommand((A_UINT8 *)"param1", (A_UINT8 *)&value);

	commandComplete(&rCmdStream, &cmdStreamLen );
	//UserPrint("..stream len %u\n",cmdStreamLen);

	if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, NULL))
    {
        UserPrint("Error: Unable to successfully send OTP_LOAD command to client!\n");
        return A_ERROR;
    }
    return A_OK;
}

void addParameterIfNeeded (TLV_PARAM_DEFAULT *pTlvParamDefaultTbl, A_UINT32 tblSize, A_UINT8 *key, A_UINT8 *pData, A_UINT32 size)
{
	A_UINT8 data[40];
	A_UINT32 i, j;

	for (i = 0; i < tblSize; ++i)
	{
		if (strcmp(key, pTlvParamDefaultTbl[i].key) == 0)
		{
			// add only if the value is different from default
			for (j = 0;j < size; ++j)
			{
				if (pData[j] != pTlvParamDefaultTbl[i].data[j])
				{
					// copy to local in case type of pData defined in TX_DATA_START_PARAMS is different from the one defined in cmdTxParms
					memset (data, 0, sizeof(data));
					memcpy (data, pData, size);
					addParameterToCommand(key, data);
#ifdef QDART_BUILD
					DbgPrint("TLV - addParameterToCommand(%s) {", key);
					for (j = 0; j < size; ++j)
					{
						DbgPrint("0x%02x,", data[j]);
					}
					DbgPrint("}\n");
#else
					
					UserPrint("TLV - addParameterToCommand(%s) {", key);
					for (j = 0; j < size; ++j)
					{
						UserPrint("0x%02x,", data[j]);
					}
					
					UserPrint("}\n");
					
#endif					
					break;
				}
			}
			break;
		}
	}
}

void addParameterSequenceIfNeeded (TLV_PARAM_DEFAULT *pTlvParamDefaultTbl, A_UINT32 tblSize, A_UINT8 *firstKey, A_UINT8 *pData, A_UINT32 size, A_UINT32 sequenceSize)
{
	A_UINT8 key[30], indexStr[4], data[4];
	A_UINT32 i, j, k, n, keyLen;

	if (size > 4)
	{
		UserPrint("addTxParameterSequenceIfNeeded - WARNING parameter size > 4\n");
		size = 4;
	}
	//search the firstKey from the table
	for (i = 0; i < tblSize; ++i)
	{
		if (strcmp (firstKey, pTlvParamDefaultTbl[i].key) == 0)
		{
			//found
			keyLen =  strlen(firstKey);
			// chop the key index
			memcpy (key, firstKey, keyLen-1);
			for (j = i, k = 0; k < (int)sequenceSize; ++j, ++k)
			{
				// get data
				memset (data, 0, sizeof(data));
				memcpy (data, pData+(k*size), size);
				if (memcmp (data, pTlvParamDefaultTbl[j].data, size) != 0)
				{
					key[keyLen-1] = '\0';
					// concatenate the index
					sprintf (indexStr, "%d", k);
					strcat (key, indexStr);
					// add only if the value is different from default
					addParameterToCommand(key, data);
#ifdef QDART_BUILD
					DbgPrint("TLV - addParameterToCommand(%s) {", key);
					for (n = 0; n < size; ++n)
					{
						DbgPrint("0x%02x,", data[n]);
					}
					DbgPrint("}\n");
#else
					
					UserPrint("TLV - addParameterToCommand(%s) {", key);
					for (n = 0; n < size; ++n)
					{
						UserPrint("0x%02x,", data[n]);
					}
					UserPrint("}\n");
					
#endif
				}
			}
			break;
		}
	}
}

void addTxParameterIfNeeded (A_UINT8 *key, A_UINT8 *pData, A_UINT32 size)
{
	addParameterIfNeeded (TlvTxParamDefaultTbl, sizeof(TlvTxParamDefaultTbl)/sizeof(TLV_PARAM_DEFAULT), key, pData, size);
}

void addTxParameterSequenceIfNeeded (A_UINT8 *firstKey, A_UINT8 *pData, A_UINT32 size, A_UINT32 sequenceSize)
{
	addParameterSequenceIfNeeded (TlvTxParamDefaultTbl, sizeof(TlvTxParamDefaultTbl)/sizeof(TLV_PARAM_DEFAULT), firstKey, pData, size, sequenceSize);
}

void addTxParameters (TX_DATA_START_PARAMS *Params)
{
	//A_UINT32 txChain[MAX_TX_CHAIN];
	//int i;

	addTxParameterIfNeeded(TLV_TXPARM_KEY_CHANNEL, (A_UINT8 *)&Params->freq, sizeof(Params->freq));
	addTxParameterIfNeeded(TLV_TXPARM_KEY_ANTENNA, (A_UINT8 *)&Params->antenna, sizeof(Params->antenna));
	addTxParameterIfNeeded(TLV_TXPARM_KEY_ENANI, (A_UINT8 *)&Params->enANI, sizeof(Params->enANI));
	addTxParameterIfNeeded(TLV_TXPARM_KEY_SCRAMBLEROFF, (A_UINT8 *)&Params->scramblerOff, sizeof(Params->scramblerOff));
	addTxParameterIfNeeded(TLV_TXPARM_KEY_SHORTGUARD, (A_UINT8 *)&Params->shortGuard, sizeof(Params->shortGuard));
	addTxParameterIfNeeded(TLV_TXPARM_KEY_NUMPACKETS, (A_UINT8 *)&Params->numPackets, sizeof(Params->numPackets));
	addTxParameterIfNeeded(TLV_TXPARM_KEY_BROADCAST, (A_UINT8 *)&Params->broadcast, sizeof(Params->broadcast));
	addTxParameterIfNeeded(TLV_TXPARM_KEY_BSSID, (A_UINT8 *)&Params->bssid, sizeof(Params->bssid));
	addTxParameterIfNeeded(TLV_TXPARM_KEY_TXSTATION, (A_UINT8 *)&Params->txStation, sizeof(Params->txStation));
	addTxParameterIfNeeded(TLV_TXPARM_KEY_RXSTATION, (A_UINT8 *)&Params->rxStation, sizeof(Params->rxStation));
	addTxParameterIfNeeded(TLV_TXPARM_KEY_TPCM, (A_UINT8 *)&Params->tpcm, sizeof(Params->tpcm));
	addTxParameterIfNeeded(TLV_TXPARM_KEY_FLAGS, (A_UINT8 *)&Params->miscFlags, sizeof(Params->miscFlags));
	addTxParameterIfNeeded(TLV_TXPARM_KEY_BANDWIDTH, (A_UINT8 *)&Params->bandwidth, sizeof(Params->bandwidth));
	addTxParameterIfNeeded(TLV_TXPARM_KEY_WLANMODE, (A_UINT8 *)&Params->wlanMode, sizeof(Params->wlanMode));
	addTxParameterIfNeeded(TLV_TXPARM_KEY_TXMODE, (A_UINT8 *)&Params->mode, sizeof(Params->mode));
	addTxParameterIfNeeded(TLV_TXPARM_KEY_TXPATTERN, (A_UINT8 *)&Params->txPattern, sizeof(Params->txPattern));
	addTxParameterIfNeeded(TLV_TXPARM_KEY_DUTYCYCLE, (A_UINT8 *)&Params->dutycycle, sizeof(Params->dutycycle));
	if (Params->txPattern == USER_DEFINED_PATTERN)
	{
		addTxParameterIfNeeded(TLV_TXPARM_KEY_NPATTERN, (A_UINT8 *)&Params->nPattern, sizeof(Params->nPattern));
		addTxParameterIfNeeded(TLV_TXPARM_KEY_DATAPATTERN, (A_UINT8 *)&Params->dataPattern, sizeof(Params->dataPattern));
	}
	addTxParameterIfNeeded(TLV_TXPARM_KEY_AIFSN, (A_UINT8 *)&Params->aifsn, sizeof(Params->aifsn));
	addTxParameterSequenceIfNeeded(TLV_TXPARM_KEY_RATEBITINDEX0, (A_UINT8 *)&Params->rateMaskBitPosition,
									sizeof(Params->rateMaskBitPosition)/RATE_POWER_MAX_INDEX, RATE_POWER_MAX_INDEX);
	addTxParameterSequenceIfNeeded(TLV_TXPARM_KEY_PKTLEN0, (A_UINT8 *)&Params->pktLength,
									sizeof(Params->pktLength)/RATE_POWER_MAX_INDEX, RATE_POWER_MAX_INDEX);

	//memset (txChain, 0, sizeof(txChain));
	//for (i = 0; i < MAX_TX_CHAIN; ++i)
	//{
	//	if ((Params->txChain >> i) & 1)
	//	{
	//		txChain[i] = 1;
	//	}
	//}
	//addTxParameterSequenceIfNeeded(TLV_TXPARM_KEY_TXCHAIN0, (A_UINT8 *)txChain, sizeof(txChain)/MAX_TX_CHAIN, MAX_TX_CHAIN);
	addTxParameterIfNeeded(TLV_TXPARM_KEY_TXCHAIN0, (A_UINT8 *)&Params->txChain, sizeof(Params->txChain));
	addTxParameterIfNeeded(TLV_TXPARM_KEY_AGG, (A_UINT8 *)&Params->agg, sizeof(Params->agg));
	addTxParameterIfNeeded(TLV_TXPARM_KEY_IR, (A_UINT8 *)&Params->ir, sizeof(Params->ir));
	if (Params->tpcm == TPC_FORCED_GAINIDX)
	{
		if (Params->gainIdx == 0xff)
		{
			Params->gainIdx = 15;
		}
		addTxParameterIfNeeded(TLV_TXPARM_KEY_GAINIDX, (A_UINT8 *)&Params->gainIdx, sizeof(Params->gainIdx));
		addTxParameterIfNeeded(TLV_TXPARM_KEY_DACGAIN, (A_UINT8 *)&Params->dacGain, sizeof(Params->dacGain));
	}
	else
	{
		addTxParameterSequenceIfNeeded(TLV_TXPARM_KEY_TXPOWER0, (A_UINT8 *)&Params->txPower,
									sizeof(Params->txPower)/RATE_POWER_MAX_INDEX, RATE_POWER_MAX_INDEX);
	}
}

int art_tlvSend2( void* tlvStr, int tlvStrLen, unsigned char *respdata, unsigned int *nrespdata )
{
#ifdef _WINDOWS
	ConfigDiffExecute();

	if (!artSendCmd2(tlvStr, tlvStrLen, respdata, nrespdata ))
    {
        UserPrint("Error: Unable to successfully send TLV command to client!\n");
		return A_ERROR;
    }

	printf( "art_tlvSend2() return AOK\n" );
	return A_OK;
#else
	// Other OS not supported at this time
	return A_ERROR;
#endif
}

int art_txDataStart (TX_DATA_START_PARAMS *Params)
{
    A_UINT32 cmdId;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;
#ifdef TLV_USE_TX_DATA_START_STRUCTURE
    A_UINT32 cmdSize;
#endif
	// Push config differences if any
    ConfigDiffExecute();

    cmdId = M_TX_DATA_START_CMD_ID;
#ifdef TLV_USE_TX_DATA_START_STRUCTURE
	cmdSize = sizeof(TX_DATA_START_PARAMS);
	createCommand(_OP_GENERIC_NART_CMD);
	addParameterToCommand((A_UINT8 *)"commandId", (A_UINT8 *)&cmdId);
	addParameterToCommand((A_UINT8 *)"param1", (A_UINT8 *)&cmdSize);
	addParameterToCommand((A_UINT8 *)"data", (A_UINT8 *)Params);
#else
	createCommand(_OP_TX);
	addTxParameters(Params);
#endif //TLV_USE_TX_DATA_START_STRUCTURE

	commandComplete(&rCmdStream, &cmdStreamLen );
	//UserPrint("..stream len %u\n",cmdStreamLen);

	if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, NULL))
    {
        UserPrint("Error: Unable to successfully send TX_DATA_START_CMD command to client!\n");
		return A_ERROR;
    }
	return A_OK;
}

int art_txDataStop(void **txStatus, int calibrate)
{
    A_UINT32 *pReadValues;
    A_UINT32 cmdId, value;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;

    // create cmd to send to client
    cmdId = M_TX_DATA_STOP_CMD_ID;

	if (calibrate)
	{
		value = 0;
	}
	else
	{
		value = 1;
	}

	createCommand(_OP_GENERIC_NART_CMD);
	addParameterToCommand((A_UINT8 *)"commandId", (A_UINT8 *)&cmdId);
	addParameterToCommand((A_UINT8 *)"param1", (A_UINT8 *)&value);

	commandComplete(&rCmdStream, &cmdStreamLen );
	//UserPrint("..stream len %u\n",cmdStreamLen);

	if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, (void **)&pReadValues))
    {
        UserPrint("Error: Unable to successfully send M_TX_DATA_STOP command to client!\n");
        return A_ERROR;
    }
    *txStatus = (*pReadValues) ? (void *)pReadValues : NULL;
    return A_OK;
}

int art_txStatusReport(void **txStatus, int stop)
{
    A_UINT32  *pReadValues;
    A_UINT32 cmdId, value;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;

    // create cmd to send to client
    cmdId = M_TX_DATA_STATUS_CMD_ID;
    value= stop;
	createCommand(_OP_GENERIC_NART_CMD);
	addParameterToCommand((A_UINT8 *)"commandId", (A_UINT8 *)&cmdId);
	addParameterToCommand((A_UINT8 *)"param1", (A_UINT8 *)&value);

	commandComplete(&rCmdStream, &cmdStreamLen );
	//UserPrint("..stream len %u\n",cmdStreamLen);

	if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, (void **)&pReadValues))
    {
        UserPrint("Error: Unable to successfully send M_TX_DATA_STATUS command to client!\n");
        return A_ERROR;
    }
    *txStatus = (*pReadValues) ? (void *)pReadValues : NULL;



    return A_OK;
}

void addRxParameterIfNeeded (A_UINT8 *key, A_UINT8 *pData, A_UINT32 size)
{
	addParameterIfNeeded (TlvRxParamDefaultTbl, sizeof(TlvRxParamDefaultTbl)/sizeof(TLV_PARAM_DEFAULT), key, pData, size);
}

void addRxParameterSequenceIfNeeded (A_UINT8 *firstKey, A_UINT8 *pData, A_UINT32 size, A_UINT32 sequenceSize)
{
	addParameterSequenceIfNeeded (TlvRxParamDefaultTbl, sizeof(TlvRxParamDefaultTbl)/sizeof(TLV_PARAM_DEFAULT), firstKey, pData, size, sequenceSize);
}

void addRxParameters (RX_DATA_START_PARAMS *Params)
{
	A_UINT32  value;
	addRxParameterIfNeeded(TLV_RXPARM_KEY_CHANNEL, (A_UINT8 *)&Params->freq, sizeof(Params->freq));
	addRxParameterIfNeeded(TLV_RXPARM_KEY_ANTENNA, (A_UINT8 *)&Params->antenna, sizeof(Params->antenna));
	addRxParameterIfNeeded(TLV_RXPARM_KEY_ENANI, (A_UINT8 *)&Params->enANI, sizeof(Params->enANI));

	value = (Params->promiscuous) ? TCMD_CONT_RX_PROMIS : TCMD_CONT_RX_FILTER;
	addRxParameterIfNeeded(TLV_RXPARM_KEY_RXMODE, (A_UINT8 *)&value, sizeof(value));

	addRxParameterIfNeeded(TLV_RXPARM_KEY_WLANMODE, (A_UINT8 *)&Params->wlanMode, sizeof(Params->wlanMode));
	addRxParameterIfNeeded(TLV_RXPARM_KEY_RXCHAIN, (A_UINT8 *)&Params->rxChain, sizeof(Params->rxChain));
	addRxParameterIfNeeded(TLV_RXPARM_KEY_BROARDCAST, (A_UINT8 *)&Params->broadcast, sizeof(Params->broadcast));
	addRxParameterIfNeeded(TLV_RXPARM_KEY_BANDWIDTH, (A_UINT8 *)&Params->bandwidth, sizeof(Params->bandwidth));
	addRxParameterIfNeeded(TLV_RXPARM_KEY_BSSID, (A_UINT8 *)&Params->bssid, sizeof(Params->bssid));
	addRxParameterIfNeeded(TLV_RXPARM_KEY_ADDR, (A_UINT8 *)&Params->rxStation, sizeof(Params->rxStation));
	addRxParameterIfNeeded(TLV_RXPARM_KEY_EXPECTEDPACKETS, (A_UINT8 *)&Params->numPackets, sizeof(Params->numPackets));

	value = RX_STATUS_PER_RATE_MASK | PROCESS_RATE_IN_ORDER_MASK;
	addRxParameterIfNeeded(TLV_RXPARM_KEY_FLAGS, (A_UINT8 *)&value, sizeof(value));

	addRxParameterSequenceIfNeeded(TLV_RXPARM_KEY_RATEMASK0, (A_UINT8 *)&Params->rateMask,
									sizeof(Params->rateMask)/RATE_MASK_ROW_MAX, RATE_MASK_ROW_MAX);
}

int art_rxDataStart (RX_DATA_START_PARAMS *Params)
{
    A_UINT32 cmdId;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;
#ifdef TLV_USE_TX_DATA_START_STRUCTURE
    A_UINT32 cmdSize;
#endif

	// Push config differences if any
    ConfigDiffExecute();

    cmdId = M_RX_DATA_START_CMD_ID;
#ifdef TLV_USE_RX_DATA_START_STRUCTURE
	cmdSize = sizeof(RX_DATA_START_PARAMS);
	createCommand(_OP_GENERIC_NART_CMD);
	addParameterToCommand((A_UINT8 *)"commandId", (A_UINT8 *)&cmdId);
	addParameterToCommand((A_UINT8 *)"param1", (A_UINT8 *)&cmdSize);
	addParameterToCommand((A_UINT8 *)"data", (A_UINT8 *)Params);
#else
	createCommand(_OP_RX);
	addRxParameters(Params);
#endif //TLV_USE_RX_DATA_START_STRUCTURE

	commandComplete(&rCmdStream, &cmdStreamLen );
	//UserPrint("..stream len %u\n",cmdStreamLen);

	if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, NULL))
    {
        UserPrint("Error: Unable to successfully send RX_DATA_START_CMD command to client!\n");
		return A_ERROR;
    }
    return A_OK;
}

int art_rxDataStop(void **rxStatus)
{
    A_UINT32 *pReadValues;
    A_UINT32 cmdId, value;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;

    // create cmd to send to client
    cmdId = M_RX_DATA_STOP_CMD_ID;
    value = 1;
	createCommand(_OP_GENERIC_NART_CMD);
	addParameterToCommand((A_UINT8 *)"commandId", (A_UINT8 *)&cmdId);
	addParameterToCommand((A_UINT8 *)"param1", (A_UINT8 *)&value);

	commandComplete(&rCmdStream, &cmdStreamLen );
	//UserPrint("..stream len %u\n",cmdStreamLen);

	if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, (void **)&pReadValues))
    {
        UserPrint("Error: Unable to successfully send M_RX_DATA_STOP command to client!\n");
        return A_ERROR;
    }
    *rxStatus = (*pReadValues) ? (void *)pReadValues : NULL;
    return A_OK;
}

int art_rxStatusReport(void **rxStatus, int stop)
{
    A_UINT32  *pReadValues;
    A_UINT32 cmdId, value;
    A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;

    // create cmd to send to client
    cmdId = M_RX_DATA_STATUS_CMD_ID;
    value = stop;
	createCommand(_OP_GENERIC_NART_CMD);
	addParameterToCommand((A_UINT8 *)"commandId", (A_UINT8 *)&cmdId);
	addParameterToCommand((A_UINT8 *)"param1", (A_UINT8 *)&value);

	commandComplete(&rCmdStream, &cmdStreamLen );
	//UserPrint("..stream len %u\n",cmdStreamLen);

	if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, (void **)&pReadValues))
    {
        UserPrint("Error: Unable to successfully send M_TX_DATA_STATUS command to client!\n");
        return A_ERROR;
    }
    *rxStatus = (*pReadValues) ? (void *)pReadValues : NULL;
    return A_OK;
}

int art_sleepMode (int mode)
{
	A_BOOL ret;
    A_UINT32 cmdId, value;
	A_UINT8 *rCmdStream = NULL;
    A_UINT32 cmdStreamLen=0;

    ret = createCommand(_OP_PM);
	if (ret == FALSE)
	{
		UserPrint("Error in creating command _OP_PM\n");
		return A_ERROR;
	}
    ret = addParameterToCommand((A_UINT8 *)"mode",(A_UINT8 *)&value);
    if (ret == FALSE)
	{
		UserPrint("Error in adding param mode to command _OP_PM\n");
	}
	commandComplete(&rCmdStream, &cmdStreamLen );
	//UserPrint("..stream len %u\n",cmdStreamLen);

	cmdId = SLEEP_CMD_ID;
	if (!artSendCmd(rCmdStream, cmdStreamLen, cmdId, NULL))
    {
        UserPrint("Error: Unable to successfully send SLEEP_CMD_ID command to client!\n");
        return A_ERROR;
    }
    return A_OK;
}

int art_getDeviceHandle (unsigned int *handle)
{
#ifdef _WINDOWS
	*handle = (unsigned int)cmd_getDevHandle();
#else
	*handle = 0;
#endif
	return A_OK;
}

A_UINT32 art_readPciConfigSpace(A_UINT32 offset)
{
    return cmd_Art2ReadPciConfigSpace(offset);
}

int art_writePciConfigSpace(A_UINT32 offset, A_UINT32 value)
{
    return cmd_Art2WritePciConfigSpace(offset, value);
}
