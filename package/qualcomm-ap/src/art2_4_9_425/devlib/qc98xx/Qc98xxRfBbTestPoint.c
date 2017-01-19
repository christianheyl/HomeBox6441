/*
 *  Copyright ?2005 Atheros Communications, Inc.,  All Rights Reserved.
 *
 */


#include <stdio.h>
#include <stdlib.h>

#include "wlantype.h"
#include "wlanproto.h"
#include "athreg.h"
#include "manlib.h"     /* The Manufacturing Library */
#include "mlibif.h"
#include "DevSetConfig.h"
//#include "hw/bb_reg_map.h"
#include "hw/mac_pcu_reg.h"
#include "dk_cmds.h"
#include "tlvCmd_if.h"
#include "Qc98xxDevice.h"
#include "TimeMillisecond.h"
#include "UserPrint.h"
#include "MyDelay.h"
#include "Field.h"
#include "rate_constants.h"
#include "vrate_constants.h"
#include "LinkList.h"

#define RX_GAIN_TABLE_MAX_ENTRY  (128)
#define RXGAIN_TABLE_STRING      "bb_oc_gain_tab_"

#define RXGAIN_TABLE_RX1DB_BIQUAD_LSB				0
#define RXGAIN_TABLE_RX1DB_BIQUAD_MASK              0x7
#define RXGAIN_TABLE_RX6DB1_BIQUAD_LSB				5
#define RXGAIN_TABLE_RX6DB1_BIQUAD_MASK             0x3
#define RXGAIN_TABLE_RX6DB2_BIQUAD_LSB              3
#define RXGAIN_TABLE_RX6DB2_BIQUAD_MASK             0x3
//#define RXGAIN_TABLE_SCFIR_GAIN_LSB                   7
//#define RXGAIN_TABLE_SCFIR_GAIN_MASK                0x1
#define RXGAIN_TABLE_MXR_GAIN_LSB                   8
#define RXGAIN_TABLE_MXR_GAIN_MASK                  0xf
#define RXGAIN_TABLE_VGA_GAIN_LSB                   5
#define RXGAIN_TABLE_VGA_GAIN_MASK                  0x7
#define RXGAIN_TABLE_LNA_GAIN_LSB                   8
#define RXGAIN_TABLE_LNA_GAIN_MASK                  0xf

#define RX_GAIN_MIN_QC98XX_11a      0
#define RX_GAIN_MIN_QC98XX_11bg     0

#define SKIP_SWITCH_TABLE                     0x100

A_UINT32 gSwitchTable = SKIP_SWITCH_TABLE;


static A_UCHAR  bssID[6]     = {0x50, 0x55, 0x5A, 0x50, 0x00, 0x00};
static A_UCHAR  rxStation[6] = {0x10, 0x11, 0x12, 0x13, 0x00, 0x00};	// DUT

//-----------------------------------------------------------------------------

A_BOOL qc98xxSetRxGain(A_INT32 frequency, A_INT32 bandwidth, A_UINT32 chainNum, A_INT32 mbGainIndex, A_INT32 rfGainIndex)
{
    unsigned int mode;
    unsigned int gainTbl;
    unsigned int mbGain;
    unsigned int mbGainEntry;
    unsigned int mbGainMax;
    A_CHAR   mbGainName[50];
    unsigned int rfGain;
    unsigned int rfGainEntry;
    unsigned int rfGainMax;
    A_CHAR   rfGainName[50];
    unsigned int rfGainBaseAddr;
    unsigned int rx1dbBiquad;
    unsigned int rx6dbBiquad1;
    unsigned int rx6dbBiquad2;
    unsigned int mxrGain    ;
    unsigned int vgaGain    ;
    unsigned int lnaGain    ;
    unsigned int reg;
	unsigned int bw;
	unsigned int filterfc;

    mode = (frequency < 4000 ) ? MODE_11G : MODE_11A;

    FieldReadNoMask("BB_rx_gain_bounds_1.rx_max_mb_gain", &reg);

    // Get max number of mb gain entries
    FieldGet("BB_rx_gain_bounds_1.rx_max_mb_gain", &mbGainMax, reg);

    // Get max of rf gain 
    FieldGet("BB_rx_gain_bounds_1.rx_max_rf_gain", &rfGainMax, reg);

    // Adjust based on max entries of each;
    if ((unsigned int)mbGainIndex <= mbGainMax)
    {
        mbGainEntry = mbGainIndex;
    }
    else
    {
        mbGainEntry = mbGainMax;
        UserPrint ("WARNING - The mb-gain exceeds max gain. Adjust to max (%d)\n", mbGainMax);
    }
    if ((unsigned int)rfGainIndex <= rfGainMax)
    {
        rfGainEntry = rfGainIndex;
    }
    else
    {
        rfGainEntry = rfGainMax;
        UserPrint ("WARNING - The rf-gain exceeds max gain. Adjust to max (%d)\n", rfGainMax);
    }

    // Get base address of rf gain table
    FieldRead("BB_rx_gain_bounds_2.rf_gain_base_addr", &rfGainBaseAddr);
	
    // check gain table 
	if (mode == MODE_11A)
	{
		FieldGet("BB_rx_gain_bounds_1.rx_ocgain_sel_5G", &gainTbl, reg);
	}
	else //MOD
	{
		FieldGet("BB_rx_gain_bounds_1.rx_ocgain_sel_2G", &gainTbl, reg);
	}

    // Get field names. 2 16-bit entries per address word
	if (gainTbl == 0)
	{
		sprintf(mbGainName, "BB_rx_ocgain[%d].gain_entry", mbGainEntry >> 1); //mb gain starts at ocgain address 0
		sprintf(rfGainName, "BB_rx_ocgain[%d].gain_entry", (rfGainEntry >> 1)+rfGainBaseAddr); //rf gain starts at ocgain address rf_gain_base_addr
	}
	else
	{
		sprintf(mbGainName, "BB_rx_ocgain2[%d].gain_entry2", mbGainEntry >> 1); //mb gain starts at ocgain address 0
		sprintf(rfGainName, "BB_rx_ocgain2[%d].gain_entry2", (rfGainEntry >> 1)+rfGainBaseAddr); //rf gain starts at ocgain address rf_gain_base_addr
	}
    FieldRead(mbGainName, &mbGain);
    FieldRead(rfGainName, &rfGain);

	//determine bw_st setting
	bw = (bandwidth == BW_HT20) ? 5 : ((bandwidth == BW_HT40_PLUS || bandwidth == BW_HT40_MINUS) ? 4 :
		 (((bandwidth == BW_VHT80_0) || (bandwidth == BW_VHT80_1) || (bandwidth == BW_VHT80_2) || (bandwidth == BW_VHT80_3)) ? 3 : 2));

    if (mbGainEntry & 1) //odd entry
    {
        mbGain >>= 16;
    }
    if (rfGainEntry & 1) //odd entry
    {
        rfGain >>= 16;
    }

    /* get field name parameters from ini file */
    rx1dbBiquad  = (mbGain >> RXGAIN_TABLE_RX1DB_BIQUAD_LSB) & RXGAIN_TABLE_RX1DB_BIQUAD_MASK;
    rx6dbBiquad1 = (mbGain >> RXGAIN_TABLE_RX6DB1_BIQUAD_LSB) & RXGAIN_TABLE_RX6DB1_BIQUAD_MASK;
    rx6dbBiquad2 = (mbGain >> RXGAIN_TABLE_RX6DB2_BIQUAD_LSB) & RXGAIN_TABLE_RX6DB2_BIQUAD_MASK;
    mxrGain      = (mbGain >> RXGAIN_TABLE_MXR_GAIN_LSB) & RXGAIN_TABLE_MXR_GAIN_MASK;
    
    lnaGain      = (rfGain >> RXGAIN_TABLE_LNA_GAIN_LSB) & RXGAIN_TABLE_LNA_GAIN_MASK;
    vgaGain      = (rfGain >> RXGAIN_TABLE_VGA_GAIN_LSB) & RXGAIN_TABLE_VGA_GAIN_MASK;

	FieldRead("bb_b0_BB2.FILTERFC", &filterfc);

    //printf("TRANG - rx1dbBiquad = %x\n", rx1dbBiquad);
    //printf("TRANG - rx6dbBiquad = %x\n", rx6dbBiquad);
    //printf("TRANG - scfirGain = %x\n", scfirGain);
    //printf("TRANG - mxrGain = %x\n", mxrGain);
    //printf("TRANG - vgaGain = %x\n", vgaGain);
    //printf("TRANG - lnaGain = %x\n", lnaGain);

	// force XLNA and XPA biases off
	FieldReadNoMask("top_wlan_PLLCLKMODAWLAN.pwd_xlnabias", &reg);
	FieldSet("top_wlan_PLLCLKMODAWLAN.pwd_xlnabias", 1, &reg); 
	FieldSet("top_wlan_PLLCLKMODAWLAN.xlnaon_ovr", 1, &reg); 
	FieldSet("top_wlan_PLLCLKMODAWLAN.xlnaon",    0, &reg); 
	FieldSet("top_wlan_PLLCLKMODAWLAN.xlna5on_0", 0, &reg); 
	FieldSet("top_wlan_PLLCLKMODAWLAN.xlna2on_0", 0, &reg); 
	FieldWriteNoMask("top_wlan_PLLCLKMODAWLAN.xlna2on_0", reg);

	FieldReadNoMask("top_wlan_TOPWLAN1.xpashort2gnd", &reg);
	FieldSet("top_wlan_TOPWLAN1.xpashort2gnd", 0, &reg); 
	FieldSet("top_wlan_TOPWLAN1.local_xpaon", 1, &reg); 
	FieldSet("top_wlan_TOPWLAN1.xpa2on", 0, &reg); 
	FieldSet("top_wlan_TOPWLAN1.xpa5on", 0, &reg); 
	FieldWriteNoMask("top_wlan_TOPWLAN1.xpa5on", reg);

//#ifdef _DEBUG
	FieldReadNoMask("top_wlan_PLLCLKMODAWLAN.pwd_xlnabias", &reg);
	UserPrint("top_wlan_PLLCLKMODAWLAN = 0x%08x\n", reg);
//#endif //_DEBUG

    if (chainNum == 0)
    {
		// force analog RX mode
		FieldReadNoMask("rxtx_b0_RXTX2.txon_ovr", &reg);
        FieldSet("rxtx_b0_RXTX2.txon_ovr",    1, &reg);
        FieldSet("rxtx_b0_RXTX2.txon",        0, &reg);
        FieldSet("rxtx_b0_RXTX2.rxon_ovr",    1, &reg);
        FieldSet("rxtx_b0_RXTX2.rxon",        1, &reg);
        FieldSet("rxtx_b0_RXTX2.synthon_ovr", 1, &reg);
        FieldSet("rxtx_b0_RXTX2.synthon",     1, &reg);  
        FieldSet("rxtx_b0_RXTX2.paon_ovr",    1, &reg);
        FieldSet("rxtx_b0_RXTX2.paon",        0, &reg);
		FieldSet("rxtx_b0_RXTX2.BW_ST",		 bw, &reg);
		FieldSet("rxtx_b0_RXTX2.BW_ST_ovr",	  1, &reg);

		FieldSet("rxtx_b0_RXTX2.bmode_ovr",   1, &reg);
        if (mode == MODE_11A)
		{
            FieldSet("rxtx_b0_RXTX2.bmode", 0, &reg);
		}
		else
		{
			FieldSet("rxtx_b0_RXTX2.bmode", 1, &reg);
		}
		FieldWriteNoMask("rxtx_b0_RXTX2.bmode", reg);

        FieldWrite("bb_b0_BB2.FILTERFC", filterfc);
        FieldWrite("bb_b0_BB2.FILTERFC_OVR" , 1);

		// force analog RX gain
		FieldReadNoMask("rxtx_b0_RXTX1.lnagain", &reg);
        FieldSet("rxtx_b0_RXTX1.lnagain", lnaGain, &reg);
        FieldSet("rxtx_b0_RXTX1.vgagain", vgaGain, &reg);
        FieldSet("rxtx_b0_RXTX1.mxrgain", mxrGain, &reg);
        FieldSet("rxtx_b0_RXTX1.manrxgain", 1, &reg);
		FieldWriteNoMask("rxtx_b0_RXTX1.manrxgain", reg);

		FieldReadNoMask("bb_b0_BB8.rx1db_biquad", &reg);
        FieldSet("bb_b0_BB8.rx1db_biquad", rx1dbBiquad, &reg);
        FieldSet("bb_b0_BB8.rx6db_biquad1", rx6dbBiquad1, &reg);
        FieldSet("bb_b0_BB8.rx6db_biquad2", rx6dbBiquad2, &reg);

		// force ADC on
		FieldSet("bb_b0_BB8.local_dacpwd", 0x1, &reg);
		FieldSet("bb_b0_BB8.dacpwd",       0x1, &reg);
		FieldSet("bb_b0_BB8.local_adcpwd", 0x1, &reg);
		FieldSet("bb_b0_BB8.adcpwd",       0x0, &reg);
		FieldWriteNoMask("bb_b0_BB8.adcpwd", reg);
    }
    else if (chainNum == 1)
    {
		// force analog RX mode
		FieldReadNoMask("rxtx_b1_RXTX2.txon_ovr", &reg);
        FieldSet("rxtx_b1_RXTX2.txon_ovr",    1, &reg);
        FieldSet("rxtx_b1_RXTX2.txon",        0, &reg);
        FieldSet("rxtx_b1_RXTX2.rxon_ovr",    1, &reg);
        FieldSet("rxtx_b1_RXTX2.rxon",        1, &reg);
        FieldSet("rxtx_b1_RXTX2.synthon_ovr", 1, &reg);
        FieldSet("rxtx_b1_RXTX2.synthon",     1, &reg);  
        FieldSet("rxtx_b1_RXTX2.paon_ovr",    1, &reg);
        FieldSet("rxtx_b1_RXTX2.paon",        0, &reg);
		FieldSet("rxtx_b1_RXTX2.BW_ST",		 bw, &reg);
		FieldSet("rxtx_b1_RXTX2.BW_ST_ovr",	  1, &reg);

		FieldSet("rxtx_b1_RXTX2.bmode_ovr",   1, &reg);
        if (mode == MODE_11A)
		{
            FieldSet("rxtx_b1_RXTX2.bmode", 0, &reg);
		}
		else
		{
			FieldSet("rxtx_b1_RXTX2.bmode", 1, &reg);
		}
		FieldWriteNoMask("rxtx_b1_RXTX2.bmode", reg);

        FieldWrite("bb_b1_BB2.FILTERFC", filterfc);
        FieldWrite("bb_b1_BB2.FILTERFC_OVR" , 1);

		// force analog RX gain
		FieldReadNoMask("rxtx_b1_RXTX1.lnagain", &reg);
        FieldSet("rxtx_b1_RXTX1.lnagain", lnaGain, &reg);
        FieldSet("rxtx_b1_RXTX1.vgagain", vgaGain, &reg);
        FieldSet("rxtx_b1_RXTX1.mxrgain", mxrGain, &reg);
        FieldSet("rxtx_b1_RXTX1.manrxgain", 1, &reg);
		FieldWriteNoMask("rxtx_b1_RXTX1.manrxgain", reg);

		FieldReadNoMask("bb_b1_BB8.rx1db_biquad", &reg);
        FieldSet("bb_b1_BB8.rx1db_biquad", rx1dbBiquad, &reg);
        FieldSet("bb_b1_BB8.rx6db_biquad1", rx6dbBiquad1, &reg);
        FieldSet("bb_b1_BB8.rx6db_biquad2", rx6dbBiquad2, &reg);

		// force ADC on
		FieldSet("bb_b1_BB8.local_dacpwd", 0x1, &reg);
		FieldSet("bb_b1_BB8.dacpwd",       0x1, &reg);
		FieldSet("bb_b1_BB8.local_adcpwd", 0x1, &reg);
		FieldSet("bb_b1_BB8.adcpwd",       0x0, &reg);
		FieldWriteNoMask("bb_b1_BB8.adcpwd", reg);
    }
	else //chainNum == 2
    {
		// force analog RX mode
		FieldReadNoMask("rxtx_b2_RXTX2.txon_ovr", &reg);
        FieldSet("rxtx_b2_RXTX2.txon_ovr",    1, &reg);
        FieldSet("rxtx_b2_RXTX2.txon",        0, &reg);
        FieldSet("rxtx_b2_RXTX2.rxon_ovr",    1, &reg);
        FieldSet("rxtx_b2_RXTX2.rxon",        1, &reg);
        FieldSet("rxtx_b2_RXTX2.synthon_ovr", 1, &reg);
        FieldSet("rxtx_b2_RXTX2.synthon",     1, &reg);  
        FieldSet("rxtx_b2_RXTX2.paon_ovr",    1, &reg);
        FieldSet("rxtx_b2_RXTX2.paon",        0, &reg);
		FieldSet("rxtx_b2_RXTX2.BW_ST",		 bw, &reg);
		FieldSet("rxtx_b2_RXTX2.BW_ST_ovr",	  1, &reg);

		FieldSet("rxtx_b2_RXTX2.bmode_ovr",   1, &reg);
        if (mode == MODE_11A)
		{
            FieldSet("rxtx_b2_RXTX2.bmode", 0, &reg);
		}
		else
		{
			FieldSet("rxtx_b2_RXTX2.bmode", 1, &reg);
		}
		FieldWriteNoMask("rxtx_b2_RXTX2.bmode", reg);

        FieldWrite("bb_b2_BB2.FILTERFC", filterfc);
        FieldWrite("bb_b2_BB2.FILTERFC_OVR" , 1);

		// force analog RX gain
		FieldReadNoMask("rxtx_b2_RXTX1.lnagain", &reg);
        FieldSet("rxtx_b2_RXTX1.lnagain", lnaGain, &reg);
        FieldSet("rxtx_b2_RXTX1.vgagain", vgaGain, &reg);
        FieldSet("rxtx_b2_RXTX1.mxrgain", mxrGain, &reg);
        FieldSet("rxtx_b2_RXTX1.manrxgain", 1, &reg);
		FieldWriteNoMask("rxtx_b2_RXTX1.manrxgain", reg);

		FieldReadNoMask("bb_b2_BB8.rx1db_biquad", &reg);
        FieldSet("bb_b2_BB8.rx1db_biquad", rx1dbBiquad, &reg);
        FieldSet("bb_b2_BB8.rx6db_biquad1", rx6dbBiquad1, &reg);
        FieldSet("bb_b2_BB8.rx6db_biquad2", rx6dbBiquad2, &reg);

		// force ADC on
		FieldSet("bb_b2_BB8.local_dacpwd", 0x1, &reg);
		FieldSet("bb_b2_BB8.dacpwd",       0x1, &reg);
		FieldSet("bb_b2_BB8.local_adcpwd", 0x1, &reg);
		FieldSet("bb_b2_BB8.adcpwd",       0x0, &reg);
		FieldWriteNoMask("bb_b2_BB8.adcpwd", reg);
    }
    //printf("TRANG - rxtx_b1_RXTX2 = 0x%08x\n", art_regRead(0, 0x1c104));
    return TRUE;
}
   
//-----------------------------------------------------------------------------
/* Port this from the EEP_12 file */

void qc98xxBbSignalBringOut(A_BOOL bAnaEn, unsigned char chainnum, int antennapair)
{
	unsigned int reg;
    if (!bAnaEn)
    {
        return;
    }
    // We cannot bring both chains at the same time
    // bring out chain 0
    if (chainnum == 0)
    {
		// select test mode and enable analog BB outputs
		FieldReadNoMask("rxtx_b0_RXTX5.testIQ_rsel", &reg);

		FieldSet("rxtx_b0_RXTX5.testIQ_rsel", 0x1, &reg);
		FieldSet("rxtx_b0_RXTX5.testIQ_bufen", 0x1, &reg);
    
		// Select either TESTI on ANTA and ANTB, or TESTQ on ANTC and ANTD, or both based on antennapair
		//antennapair = 1 -> TESTI
		//            = 2 -> TESTQ
		//            = 3 -> both TESTI & TESTQ
		FieldSet("rxtx_b0_RXTX5.testI_on", 0, &reg);
		FieldSet("rxtx_b0_RXTX5.testQ_on", 0, &reg);
		if (antennapair & 1)
		{
			FieldSet("rxtx_b0_RXTX5.testI_on", 0x1, &reg);
		}
		if (antennapair & 2)
		{
			FieldSet("rxtx_b0_RXTX5.testQ_on", 0x1, &reg);
		}
		FieldWriteNoMask("rxtx_b0_RXTX5.testQ_on", reg);
		FieldWrite("bb_b0_BB2.SEL_TEST", 0x24);
//#ifdef _DEBUG
		FieldReadNoMask("rxtx_b0_RXTX2.bmode", &reg);
		UserPrint("rxtx_b0_RXTX2 = 0x%08x\n", reg);
		FieldReadNoMask("rxtx_b0_RXTX1.mantxgain", &reg);
		UserPrint("rxtx_b0_RXTX1 = 0x%08x\n", reg);
		FieldReadNoMask("bb_b0_BB8.RX1DB_BIQUAD", &reg);
		UserPrint("bb_b0_BB8 = 0x%08x\n", reg);
		FieldReadNoMask("rxtx_b0_RXTX5.testI_on", &reg);
		UserPrint("rxtx_b0_RXTX5 = 0x%08x\n", reg);
		FieldReadNoMask("bb_b0_BB2.FILTERFC", &reg);
		UserPrint("bb_b0_BB2 = 0x%08x", reg);
//#endif //_DEBUG
    }
    // bring out chain 1
    else if (chainnum == 1)
    {
		FieldReadNoMask("rxtx_b1_RXTX5.testIQ_rsel", &reg);

		FieldSet("rxtx_b1_RXTX5.testIQ_rsel", 0x1, &reg);
		FieldSet("rxtx_b1_RXTX5.testIQ_bufen", 0x1, &reg);
    
		FieldSet("rxtx_b1_RXTX5.testI_on", 0, &reg);
		FieldSet("rxtx_b1_RXTX5.testQ_on", 0, &reg);
		if (antennapair & 1)
		{
			FieldSet("rxtx_b1_RXTX5.testI_on", 0x1, &reg);
		}
		if (antennapair & 2)
		{
			FieldSet("rxtx_b1_RXTX5.testQ_on", 0x1, &reg);
		}
		FieldWriteNoMask("rxtx_b1_RXTX5.testQ_on", reg);
		FieldWrite("bb_b1_BB2.SEL_TEST", 0x24);
//#ifdef _DEBUG
		FieldReadNoMask("rxtx_b1_RXTX2.bmode", &reg);
		UserPrint("rxtx_b1_RXTX2 = 0x%08x\n", reg);
		FieldReadNoMask("rxtx_b1_RXTX1.mantxgain", &reg);
		UserPrint("rxtx_b1_RXTX1 = 0x%08x\n", reg);
		FieldReadNoMask("bb_b1_BB8.RX1DB_BIQUAD", &reg);
		UserPrint("bb_b1_BB8 = 0x%08x\n", reg);
		FieldReadNoMask("rxtx_b1_RXTX5.testI_on", &reg);
		UserPrint("rxtx_b1_RXTX5 = 0x%08x\n", reg);
		FieldReadNoMask("bb_b1_BB2.FILTERFC", &reg);
		UserPrint("bb_b1_BB2 = 0x%08x\n", reg);
//#endif //_DEBUG
    }
	else if (chainnum == 2)
    {
		FieldReadNoMask("rxtx_b2_RXTX5.testIQ_rsel", &reg);

		FieldSet("rxtx_b2_RXTX5.testIQ_rsel", 0x1, &reg);
		FieldSet("rxtx_b2_RXTX5.testIQ_bufen", 0x1, &reg);
    
		FieldSet("rxtx_b2_RXTX5.testI_on", 0, &reg);
		FieldSet("rxtx_b2_RXTX5.testQ_on", 0, &reg);
		if (antennapair & 1)
		{
			FieldSet("rxtx_b2_RXTX5.testI_on", 0x1, &reg);
		}
		if (antennapair & 2)
		{
			FieldSet("rxtx_b2_RXTX5.testQ_on", 0x1, &reg);
		}
		FieldWriteNoMask("rxtx_b2_RXTX5.testQ_on", reg);
		FieldWrite("bb_b2_BB2.SEL_TEST", 0x24);
//#ifdef _DEBUG
		FieldReadNoMask("rxtx_b2_RXTX2.bmode", &reg);
		UserPrint("rxtx_b2_RXTX2 = 0x%08x\n", reg);
		FieldReadNoMask("rxtx_b2_RXTX1.mantxgain", &reg);
		UserPrint("rxtx_b2_RXTX1 = 0x%08x\n", reg);
		FieldReadNoMask("bb_b2_BB8.RX1DB_BIQUAD", &reg);
		UserPrint("bb_b2_BB8 = 0x%08x\n", reg);
		FieldReadNoMask("rxtx_b2_RXTX5.testI_on", &reg);
		UserPrint("rxtx_b2_RXTX5 = 0x%08x\n", reg);
		FieldReadNoMask("bb_b2_BB2.FILTERFC", &reg);
		UserPrint("bb_b2_BB2 = 0x%08x", reg);
//#endif //_DEBUG
    }
}

//-----------------------------------------------------------------------------

int Qc98xxRfBbTestPoint(int frequency, int ht40, int bandwidth, int antennapair, unsigned char chainnum,
                        int mbgain, int rfgain, int coex, int sharedrx, int switchtable, unsigned char AnaOutEn)
{
    // TRANG - For now do reset first so the registers will get the right mode values
    art_whalResetDevice(rxStation, bssID, frequency, bandwidth, MAX_CHAIN_MASK, MAX_CHAIN_MASK);

    if (!qc98xxSetRxGain(frequency, bandwidth, chainnum, mbgain, rfgain))
    {
        UserPrint("Unable to set gain!\n");
        return 1;
    }

    if ((switchtable != gSwitchTable) && (switchtable >= 0) && (switchtable <= 9))
    {
		UserPrint("switch table is not supported!\n");
    }

    qc98xxBbSignalBringOut(TRUE, chainnum, antennapair);

    return 0;
}



