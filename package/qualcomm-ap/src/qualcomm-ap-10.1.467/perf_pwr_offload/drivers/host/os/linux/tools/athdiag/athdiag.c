/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/if.h>
#include <linux/wireless.h>
#include <linux/version.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include <a_osapi.h>
#include <athdefs.h>
#include <athtypes_linux.h>

#include "hw/apb_athr_wlan_map.h"
#include "hw/rtc_soc_reg.h"
#include "hw/efuse_reg.h"
#include "qc98xx_eeprom.h"
#include "Qc98xxEepromStruct.h"
#include "rate_constants.h"
#include "vrate_constants.h"
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

/*
 * This is a user-level agent which provides diagnostic read/write
 * access to Target space.  This may be used
 *   to collect information for analysis
 *   to read/write Target registers
 *   etc.
 */

#define DIAG_READ_TARGET      1
#define DIAG_WRITE_TARGET     2
#define DIAG_READ_WORD        3
#define DIAG_WRITE_WORD       4

#define ADDRESS_FLAG                    0x001
#define LENGTH_FLAG                     0x002
#define PARAM_FLAG                      0x004
#define FILE_FLAG                       0x008
#define UNUSED0x010                     0x010
#define AND_OP_FLAG                     0x020
#define BITWISE_OP_FLAG                 0x040
#define QUIET_FLAG                      0x080
#define OTP_FLAG                        0x100
#define HEX_FLAG                     	0x200 //dump file mode,x: hex mode; other binary mode.
#define UNUSED0x400                     0x400
#define DEVICE_FLAG                     0x800
/* Limit malloc size when reading/writing file */
#define MAX_BUF                         (8*1024)
FILE *pFile;
unsigned int flag;
const char *progname;
const char commands[] = 
"commands and options:\n\
--get --address=<target word address>\n\
--set --address=<target word address> --[value|param]=<value>\n\
                                      --or=<OR-ing value>\n\
                                      --and=<AND-ing value>\n\
--read --address=<target address> --length=<bytes> --file=<filename>\n\
--write --address=<target address> --file=<filename>\n\
                                   --[value|param]=<value>\n\
--otp --read --address=<otp offset> --length=<bytes> --file=<filename>\n\
--otp --write --address=<otp offset> --file=<filename>\n\
--quiet\n\
--device=<device name> (if not default)\n\
The options can also be given in the abbreviated form --option=x or -o x.\n\
The options can be given in any order.\n\
If you need to read cal data add --cal ahead of --read command";

#define A_ROUND_UP(x, y)             ((((x) + ((y) - 1)) / (y)) * (y))

#define quiet() (flag & QUIET_FLAG)
#define nqprintf(args...) if (!quiet()) {printf(args);}
#define min(x,y) ((x) < (y) ? (x) : (y))
#define rev(ui) ((ui >> 24) |((ui<<8) & 0x00FF0000) | ((ui>>8) & 0x0000FF00) | (ui << 24))
#define rev2(a) ((a>>8)|((a&0xff)<<8))
void ReadTargetRange(int dev, A_UINT32 address, A_UINT8 *buffer, A_UINT32 length);
void ReadTargetWord(int dev, A_UINT32 address, A_UINT32 *buffer);
void WriteTargetRange(int dev, A_UINT32 address, A_UINT8 *buffer, A_UINT32 length);
void WriteTargetWord(int dev, A_UINT32 address, A_UINT32 value);
int ValidWriteOTP(int dev, A_UINT32 address, A_UINT8 *buffer, A_UINT32 length);

INLINE void *
MALLOC(int nbytes)
{
    void *p= malloc(nbytes);

    if (!p)
    {
        fprintf(stderr, "err -Cannot allocate memory\n");
    }

    return p;
}

void
usage(void)
{
    fprintf(stderr, "usage:\n%s ", progname);
    fprintf(stderr, "%s\n", commands);
    exit(-1);
}
A_UINT8 alphaThermChans2G[QC98XX_NUM_ALPHATHERM_CHANS_2G] = {
    WHAL_FREQ2FBIN(2412, 1), WHAL_FREQ2FBIN(2437,1), WHAL_FREQ2FBIN(2457,1), WHAL_FREQ2FBIN(2472,1)
};

A_UINT8 alphaThermChans5G[QC98XX_NUM_ALPHATHERM_CHANS_5G] = {
    WHAL_FREQ2FBIN(5200,0), WHAL_FREQ2FBIN(5300,0), WHAL_FREQ2FBIN(5520,0), WHAL_FREQ2FBIN(5580,0),
    WHAL_FREQ2FBIN(5640,0), WHAL_FREQ2FBIN(5700,0), WHAL_FREQ2FBIN(5765,0), WHAL_FREQ2FBIN(5805,0)
};

void
ReadTargetRange(int dev, A_UINT32 address, A_UINT8 *buffer, A_UINT32 length)
{
    int nbyte;
    unsigned int remaining;

    (void)lseek(dev, address, SEEK_SET);

    remaining = length;
    while (remaining) {
        nbyte = read(dev, buffer, (size_t)remaining);
        if (nbyte <= 0) {
            fprintf(stderr, "err %s failed (nbyte=%d, address=0x%x remaining=%d).\n",
                __FUNCTION__, nbyte, address, remaining);
            exit(1);
        }

        remaining -= nbyte;
        buffer += nbyte;
        address += nbyte;
    }
}

void
ReadTargetWord(int dev, A_UINT32 address, A_UINT32 *buffer)
{
    ReadTargetRange(dev, address, (A_UINT8 *)buffer, sizeof(*buffer));
}

void
ReadTargetOTP(int dev, A_UINT32 offset, A_UINT8 *buffer, A_UINT32 length)
{
    A_UINT32 status_mask;
    A_UINT32 otp_status;
    int i;

    /* Enable OTP reads */
    WriteTargetWord(dev, RTC_SOC_BASE_ADDRESS+OTP_OFFSET, OTP_VDD12_EN_SET(1));
    status_mask = OTP_STATUS_VDD12_EN_READY_SET(1);
    do {
        ReadTargetWord(dev, RTC_SOC_BASE_ADDRESS+OTP_STATUS_OFFSET, &otp_status);
    } while ((otp_status & OTP_STATUS_VDD12_EN_READY_MASK) != status_mask);

    /* Conservatively set OTP read timing */
    WriteTargetWord(dev, EFUSE_BASE_ADDRESS+RD_STROBE_PW_REG_OFFSET, 6);

    /* Read data from OTP */
    for (i=0; i<length; i++, offset++) {
        A_UINT32 efuse_word;

        ReadTargetWord(dev, EFUSE_BASE_ADDRESS+EFUSE_INTF0_OFFSET+(offset<<2), &efuse_word);
        buffer[i] = (A_UINT8)efuse_word;
    }

    /* Disable OTP */
    WriteTargetWord(dev, RTC_SOC_BASE_ADDRESS+OTP_OFFSET, 0);
}

void
WriteTargetRange(int dev, A_UINT32 address, A_UINT8 *buffer, A_UINT32 length)
{
    int nbyte;
    unsigned int remaining;

    (void)lseek(dev, address, SEEK_SET);

    remaining = length;
    while (remaining) {
        nbyte = write(dev, buffer, (size_t)remaining);
        if (nbyte <= 0) {
            fprintf(stderr, "err %s failed (nbyte=%d, address=0x%x remaining=%d).\n",
                __FUNCTION__, nbyte, address, remaining);
        }

        remaining -= nbyte;
        buffer += nbyte;
        address += nbyte;
    }
}

void
WriteTargetWord(int dev, A_UINT32 address, A_UINT32 value)
{
    A_UINT32 param = value;

    WriteTargetRange(dev, address, (A_UINT8 *)&param, sizeof(param));
}

#define BAD_OTP_WRITE(have, want) ((((have) ^ (want)) & (have)) != 0)

/*
 * Check if the current contents of OTP and the desired
 * contents specified by buffer/length are compatible.
 * If we're trying to CLEAR an OTP bit, then this request
 * is invalid.
 * returns: 0-->INvalid; 1-->valid
 */
int
ValidWriteOTP(int dev, A_UINT32 offset, A_UINT8 *buffer, A_UINT32 length)
{
    int i;
    A_UINT8 *otp_contents;

    otp_contents = MALLOC(length);
    ReadTargetOTP(dev, offset, otp_contents, length);

    for (i=0; i<length; i++) {
        if (BAD_OTP_WRITE(otp_contents[i], buffer[i])) {
            fprintf(stderr, "Abort. Cannot change offset %d from 0x%02x to 0x%02x\n",
                offset+i, otp_contents[i], buffer[i]);
            return 0;
        }
    }

    return 1;
}
void PrintQc98xxBaseHeader(int client, const BASE_EEP_HEADER *pBaseEepHeader, const QC98XX_EEPROM *pEeprom)
{
#define MBUFFER 1024
    char  buffer[MBUFFER];

    SformatOutput(buffer, MBUFFER-1, "| RegDomain 1              0x%04X   |  RegDomain 2             0x%04X   |",
        pBaseEepHeader->regDmn[0],
        pBaseEepHeader->regDmn[1]
        );
  	fprintf(pFile, "\n %s\n", buffer);
    SformatOutput(buffer, MBUFFER-1, "| TX Mask                  0x%X      |  RX Mask                 0x%X      |",
        (pBaseEepHeader->txrxMask&0xF0)>>4,
        pBaseEepHeader->txrxMask&0x0F
        );
	fprintf(pFile, "\n %s\n", buffer);
    SformatOutput(buffer, MBUFFER-1,"| OpFlags: 5GHz            %d        |  2GHz                    %d        |",
             (pBaseEepHeader->opCapBrdFlags.opFlags & WHAL_OPFLAGS_11A) || 0,
             (pBaseEepHeader->opCapBrdFlags.opFlags & WHAL_OPFLAGS_11G) || 0
             );
  	fprintf(pFile, "\n %s\n", buffer);
    SformatOutput(buffer, MBUFFER-1,"| 5G HT20                  %d        |  2G HT20                 %d        |",
              (pBaseEepHeader->opCapBrdFlags.opFlags & WHAL_OPFLAGS_5G_HT20) || 0,
              (pBaseEepHeader->opCapBrdFlags.opFlags & WHAL_OPFLAGS_2G_HT20) || 0
                );
  	fprintf(pFile, "\n %s\n", buffer);
    SformatOutput(buffer, MBUFFER-1,"| 5G HT40                  %d        |  2G HT40                 %d        |",
              (pBaseEepHeader->opCapBrdFlags.opFlags & WHAL_OPFLAGS_5G_HT40) || 0,
              (pBaseEepHeader->opCapBrdFlags.opFlags & WHAL_OPFLAGS_2G_HT40) || 0
                );
  	fprintf(pFile, "\n %s\n", buffer);
    SformatOutput(buffer, MBUFFER-1,"| 5G VHT20                 %d        |  2G VHT20                %d        |",
              (pBaseEepHeader->opCapBrdFlags.opFlags2 & WHAL_OPFLAGS2_5G_VHT20) || 0,
              (pBaseEepHeader->opCapBrdFlags.opFlags2 & WHAL_OPFLAGS2_2G_VHT20) || 0
                );
  	fprintf(pFile, "\n %s\n", buffer);
    SformatOutput(buffer, MBUFFER-1,"| 5G VHT40                 %d        |  2G VHT40                %d        |",
              (pBaseEepHeader->opCapBrdFlags.opFlags2 & WHAL_OPFLAGS2_5G_VHT40) || 0,
              (pBaseEepHeader->opCapBrdFlags.opFlags2 & WHAL_OPFLAGS2_2G_VHT40) || 0
                );
  	fprintf(pFile, "\n %s\n", buffer);
    SformatOutput(buffer, MBUFFER-1,"| 5G VHT80                 %d        |                                   |",
              (pBaseEepHeader->opCapBrdFlags.opFlags2 & WHAL_OPFLAGS2_5G_VHT80) || 0
                );
  	fprintf(pFile, "\n %s\n", buffer);
    SformatOutput(buffer, MBUFFER-1,"| Big Endian               %d        |  Wake On Wireless        %d        |",
             (pBaseEepHeader->opCapBrdFlags.miscConfiguration & WHAL_MISCCONFIG_EEPMISC_BIG_ENDIAN) || 0, (pBaseEepHeader->opCapBrdFlags.miscConfiguration & WHAL_MISCCONFIG_EEPMISC_WOW) || 0);

  	fprintf(pFile, "\n %s\n", buffer);
   	SformatOutput(buffer, MBUFFER-1, "| RF Silent                0x%X      |  Bluetooth               0x%X      |",
        pBaseEepHeader->rfSilent,
        pBaseEepHeader->opCapBrdFlags.blueToothOptions
        );
  	fprintf(pFile, "\n %s\n", buffer);
    SformatOutput(buffer, MBUFFER-1, "| GPIO wlan Disable        NA       |  GPIO wlan LED           0x%02x     |",
        pBaseEepHeader->wlanLedGpio);
  	fprintf(pFile, "\n %s\n", buffer);
    if (pBaseEepHeader->eeprom_version == QC98XX_EEP_VER1)
    {
        SformatOutput(buffer, MBUFFER-1, "| txrxGain                 0x%02x     |  pwrTableOffset          %d        |",
            pBaseEepHeader->txrxgain, pBaseEepHeader->pwrTableOffset);
	  	fprintf(pFile, "\n %s\n", buffer);
    }
    else //v2
    {
        SformatOutput(buffer, MBUFFER-1, "| txrxGain          see Modal Section  |  pwrTableOffset          %d        |",
            pBaseEepHeader->txrxgain, pBaseEepHeader->pwrTableOffset);
	  	fprintf(pFile, "\n %s\n", buffer);
    }
    SformatOutput(buffer, MBUFFER-1, "| spurBaseA                %d        |  spurBaseB               %d        |",
        pBaseEepHeader->spurBaseA, pBaseEepHeader->spurBaseB);
  	fprintf(pFile, "\n %s\n", buffer);
    SformatOutput(buffer, MBUFFER-1, "| spurRssiThresh           %d        |  spurRssiThreshCck       %d        |",
        pBaseEepHeader->spurRssiThresh,pBaseEepHeader->spurRssiThreshCck);
  	fprintf(pFile, "\n %s\n", buffer);
    SformatOutput(buffer, MBUFFER-1, "| spurMitFlag            0x%08x |  internal regulator      0x%02x     |",
         pBaseEepHeader->spurMitFlag,pBaseEepHeader->swreg);
  	fprintf(pFile, "\n %s\n", buffer);
	uint32_t ui=rev(pBaseEepHeader->opCapBrdFlags.boardFlags);
    SformatOutput(buffer, MBUFFER-1, "| boardFlags             0x%08x |  featureEnable         0x%08x |",
        ui, pBaseEepHeader->opCapBrdFlags.featureEnable);
  	fprintf(pFile, "\n %s\n", buffer);
}


void PrintQc98xx_2GHzHeader(int client, const BIMODAL_EEP_HEADER *pBiModalHeader, const QC98XX_EEPROM *pEeprom)
{
    char  buffer[MBUFFER];
    int i, j;
    const FREQ_MODAL_EEP_HEADER *pFreqHeader = &pEeprom->freqModalHeader;

    SformatOutput(buffer, MBUFFER-1," |                                                                       |");
	fprintf(pFile, "%s", buffer);

    SformatOutput(buffer, MBUFFER-1," |===========================2GHz Modal Header===========================|");
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," |                                                                       |");
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," | Antenna Common        0x%08X  |  Antenna Common2       0x%08X |",
        rev(pBiModalHeader->antCtrlCommon),
        rev(pBiModalHeader->antCtrlCommon2)
        );
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," | Ant Chain0  0x%04X    |  Ant Chain1  0x%04X   |  Ant Chain2  0x%04X   |",
        pBiModalHeader->antCtrlChain[0],
        pBiModalHeader->antCtrlChain[1],
        pBiModalHeader->antCtrlChain[2]
        );
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," | xatten1DB Ch0  0x%02x   | xatten1DB Ch1  0x%02x   | xatten1DB Ch2  0x%02x   |",
        pFreqHeader->xatten1DB[0].value2G,
        pFreqHeader->xatten1DB[1].value2G,
        pFreqHeader->xatten1DB[2].value2G
        );
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," | xatten1Margin Ch0 0x%02x| xatten1Margin Ch1 0x%02x| xatten1Margin Ch2 0x%02x|",
        pFreqHeader->xatten1Margin[0].value2G,
        pFreqHeader->xatten1Margin[1].value2G,
        pFreqHeader->xatten1Margin[2].value2G
        );
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," | Volt Slope Ch0  %3d   | Volt Slope Ch1  %3d   | Volt Slope Ch2  %3d   |",
        pBiModalHeader->voltSlope[0],
        pBiModalHeader->voltSlope[1],
        pBiModalHeader->voltSlope[2]
        );
	fprintf(pFile, "%s\n", buffer);

	for (i=0; i<WHAL_NUM_CHAINS; i++) {
        SformatOutput(buffer, MBUFFER-1, " | Alpha Thermal Slope CH%d                                               |", i);
		fprintf(pFile, "%s\n", buffer);

        for (j=0; j<QC98XX_NUM_ALPHATHERM_CHANS_2G; j=j+2) {
            SformatOutput(buffer, MBUFFER-1, " | %04d : %02d, %02d, %02d, %02d             |  %04d : %02d, %02d, %02d, %02d            |",
                    WHAL_FBIN2FREQ(alphaThermChans2G[j], 1),
                    pEeprom->tempComp2G.alphaThermTbl[i][j][0],
                    pEeprom->tempComp2G.alphaThermTbl[i][j][1],
                    pEeprom->tempComp2G.alphaThermTbl[i][j][2],
                    pEeprom->tempComp2G.alphaThermTbl[i][j][3],
                    WHAL_FBIN2FREQ(alphaThermChans2G[j+1], 1),
                    pEeprom->tempComp2G.alphaThermTbl[i][j+1][0],
                    pEeprom->tempComp2G.alphaThermTbl[i][j+1][1],
                    pEeprom->tempComp2G.alphaThermTbl[i][j+1][2],
                    pEeprom->tempComp2G.alphaThermTbl[i][j+1][3]
                    );
			fprintf(pFile, "%s\n", buffer);

        }
    }

    for (i=0; i<QC98XX_EEPROM_MODAL_SPURS; i++) {
        SformatOutput(buffer, MBUFFER-1," | spurChan[%d]             0x%02x      |                                   |",
            i, pBiModalHeader->spurChans[i].spurChan
            );
		fprintf(pFile, "%s\n", buffer);

        SformatOutput(buffer, MBUFFER-1," | spurA_PrimSecChoose[%d]  0x%02x      |  spurB_PrimSecChoose[%d] 0x%02x      |",
            i, pBiModalHeader->spurChans[i].spurA_PrimSecChoose, i, pBiModalHeader->spurChans[i].spurB_PrimSecChoose
            );
		fprintf(pFile, "%s\n", buffer);

    }


	 SformatOutput(buffer, MBUFFER-1," | xpaBiasLvl              0x%02x      |  antennaGainCh        %3d         |",
        pBiModalHeader->xpaBiasLvl,
        pBiModalHeader->antennaGainCh
        );
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," | rxFilterCap             0x%02x      |  rxGainCap              0x%02x      |",
        pBiModalHeader->rxFilterCap,
        pBiModalHeader->rxGainCap
        );
	fprintf(pFile, "%s\n", buffer);

    if (pEeprom->baseEepHeader.eeprom_version == QC98XX_EEP_VER1)
    {
        SformatOutput(buffer, MBUFFER-1," | txGain                  0x%02x      |  rxGain                 0x%02x      |",
            (pBiModalHeader->txrxgain >> 4) & 0xf,
            pBiModalHeader->rxGainCap & 0xf
            );
		fprintf(pFile, "%s\n", buffer);

    }
    SformatOutput(buffer, MBUFFER-1," |                                   |                                   |");
	fprintf(pFile, "%s\n", buffer);

}


void PrintQc98xx_2GHzCalData(int client, const QC98XX_EEPROM *pEeprom)
{
    A_UINT16 numPiers = WHAL_NUM_11G_CAL_PIERS;
    A_UINT16 pc; //pier count
    char  buffer[MBUFFER];
    A_UINT16 a,b;
    const A_UINT8 *pPiers = &pEeprom->calFreqPier2G[0];
    const CAL_DATA_PER_FREQ_OLPC *pData = &pEeprom->calPierData2G[0];
    int i;

    SformatOutput(buffer, MBUFFER-1, " |=================2G Power Calibration Information =====================|");
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	fprintf(pFile, "%s\n", buffer);

    for (i = 0; i < WHAL_NUM_CHAINS; ++i) {
        SformatOutput(buffer, MBUFFER-1, " |                          Chain %d                                      |", i);
		fprintf(pFile, "%s\n", buffer);

        SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
		fprintf(pFile, "%s\n", buffer);

        SformatOutput(buffer, MBUFFER-1, " | Freq  txgainIdx0 dacGain0  Pwr0  txgainIdx1 dacGain1 Pwr1  Volt  Temp |");
		fprintf(pFile, "%s\n", buffer);

        SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
		fprintf(pFile, "%s\n", buffer);

        for(pc = 0; pc < numPiers; pc++) {
        a = (pData[pc].calPerPoint[i].power_t8[0]);
        b = (pData[pc].calPerPoint[i].power_t8[1]);
        a = (a>>8)|((a&0xff)<<8);
        b = (b>>8)|((b&0xff)<<8);
            SformatOutput(buffer, MBUFFER-1, " | %04d    %4d      %3d     %3d      %4d      %3d     %3d   %3d   %3d  |",
                WHAL_FBIN2FREQ(pPiers[pc], 1),
                pData[pc].calPerPoint[i].txgainIdx[0], pData[pc].dacGain[0], a/8,
                pData[pc].calPerPoint[i].txgainIdx[1], pData[pc].dacGain[1], b/8,
                pData[pc].voltCalVal, pData[pc].thermCalVal
                );
		fprintf(pFile, "%s\n", buffer);

        }
        SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
		fprintf(pFile, "%s\n", buffer);

    }
    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	fprintf(pFile, "%s\n", buffer);

}

void PrintQc98xx_2GLegacyTargetPower(int client, const CAL_TARGET_POWER_LEG *pVals, const A_UINT8 *pFreq)
{
    int i,j;
    char buffer[MBUFFER],buffer2[MBUFFER];

    SformatOutput(buffer, MBUFFER-1, " |===========================2G Target Powers============================|");
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," | OFDM   ");
	fprintf(pFile, "%s", buffer);


    for (j = 0; j < WHAL_NUM_11G_LEG_TARGET_POWER_CHANS; j++) {
        SformatOutput(buffer2, MBUFFER-1,"|        %04d        ", WHAL_FBIN2FREQ(*(pFreq+j),1));
		fprintf(pFile, "%s", buffer2);

        strcat(buffer, buffer2);
    }

    strcat(buffer, "|");

    SformatOutput(buffer, MBUFFER-1," |========|====================|====================|====================|");
	fprintf(pFile, "\n %s\n", buffer);


    for (j = 0; j < WHAL_NUM_LEGACY_TARGET_POWER_RATES; j++) {
        SformatOutput(buffer, MBUFFER-1," | %s   ",sRatePrintLegacy[j]);
		fprintf(pFile, "%s", buffer);

        for(i=0; i<WHAL_NUM_11G_LEG_TARGET_POWER_CHANS; i++) {
            SformatOutput(buffer2, MBUFFER-1,"|        %2d.%d        ", pVals[i].tPow2x[j]/2, (pVals[i].tPow2x[j] % 2) * 5);
			fprintf(pFile, "%s", buffer2);

            strcat(buffer, buffer2);
        }
        fprintf(pFile, "\n");
        strcat(buffer, "|");
    }
    SformatOutput(buffer, MBUFFER-1," |========================================================================");
	fprintf(pFile, "%s\n", buffer);

}

void PrintQc98xx_2GCCKTargetPower(int client, const CAL_TARGET_POWER_11B *pVals, const A_UINT8 *pFreq)
{
    int i,j;
    char buffer[MBUFFER],buffer2[MBUFFER];

    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," | CCK    ");
	fprintf(pFile, "%s", buffer);


    for (j = 0; j < WHAL_NUM_11B_TARGET_POWER_CHANS; j++) {
        SformatOutput(buffer2, MBUFFER-1,"|        %04d        ", WHAL_FBIN2FREQ(*(pFreq+j),1));
		fprintf(pFile, "%s", buffer2);

        strcat(buffer, buffer2);
    }

    strcat(buffer, "|====================|");

    SformatOutput(buffer, MBUFFER-1," |========|====================|====================|====================|");
	fprintf(pFile, "\n %s\n", buffer);


    for (j = 0; j < WHAL_NUM_11B_TARGET_POWER_RATES; j++) {
        SformatOutput(buffer, MBUFFER-1," | %s  ",sRatePrintCck[j]);
		fprintf(pFile, "%s", buffer);

        for(i=0; i<WHAL_NUM_11B_TARGET_POWER_CHANS; i++) {
            SformatOutput(buffer2, MBUFFER-1,"|        %2d.%d        ", pVals[i].tPow2x[j]/2, (pVals[i].tPow2x[j] % 2) * 5);
			fprintf(pFile, "%s", buffer2);

            strcat(buffer, buffer2);
        }

        strcat(buffer, "|====================|");
        fprintf(pFile, "\n");
    }
    SformatOutput(buffer, MBUFFER-1," |========================================================================");
	fprintf(pFile, "%s\n", buffer);

}

void PrintQc98xx_2GHT20TargetPower(int client, const CAL_TARGET_POWER_11G_20 *pVals, const A_UINT8 *pFreq)
{
    int i,j;
    char buffer[MBUFFER],buffer2[MBUFFER];
    double power;
    int rateIndex;

    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," | VHT20     ");
	fprintf(pFile, "%s", buffer);


    for (j = 0; j < WHAL_NUM_11G_20_TARGET_POWER_CHANS; j++) {
        SformatOutput(buffer2, MBUFFER-1,"|        %04d       ", WHAL_FBIN2FREQ(*(pFreq+j),1));
		fprintf(pFile, "%s", buffer2);

        strcat(buffer, buffer2);
    }

    strcat(buffer, "|");

    SformatOutput(buffer, MBUFFER-1," |===========|===================|===================|===================|");
	fprintf(pFile, "\n %s\n", buffer);

    for (j = 0; j < RATE_PRINT_VHT_SIZE; j++) {
        SformatOutput(buffer, MBUFFER-1," | %s  ",sRatePrintVHT[j]);
		fprintf(pFile, "%s", buffer);

        for(i=0; i<WHAL_NUM_11G_20_TARGET_POWER_CHANS; i++) {
            rateIndex = vRATE_INDEX_HT20_MCS0 + j;
            Qc98xxTargetPowerGet(WHAL_FBIN2FREQ(*(pFreq+i),1), rateIndex, &power);
            SformatOutput(buffer2, MBUFFER-1,"|        %2.1f       ", power);
			fprintf(pFile, "%s", buffer2);

            strcat(buffer, buffer2);
        }

        strcat(buffer, "|");
        fprintf(pFile, "\n");
    }
    SformatOutput(buffer, MBUFFER-1," |========================================================================");
	fprintf(pFile, "%s\n", buffer);

}


void PrintQc98xx_2GHT40TargetPower(int client, const CAL_TARGET_POWER_11G_40 *pVals, const A_UINT8 *pFreq)
{
    int i,j;
    char buffer[MBUFFER],buffer2[MBUFFER];
    double power;
    int rateIndex;

    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," | VHT40     ");
	fprintf(pFile, "%s", buffer);


    for (j = 0; j < WHAL_NUM_11G_40_TARGET_POWER_CHANS; j++) {
        SformatOutput(buffer2, MBUFFER-1,"|        %04d       ", WHAL_FBIN2FREQ(*(pFreq+j),1));
		fprintf(pFile, "%s", buffer2);

        strcat(buffer, buffer2);
    }

    strcat(buffer, "|");

    SformatOutput(buffer, MBUFFER-1," |===========|===================|===================|===================|");
	fprintf(pFile, "\n %s\n", buffer);


    for (j = 0; j < RATE_PRINT_VHT_SIZE; j++) {
        SformatOutput(buffer, MBUFFER-1," | %s  ",sRatePrintVHT[j]);
		fprintf(pFile, "%s", buffer);

        for(i=0; i < WHAL_NUM_11G_40_TARGET_POWER_CHANS; i++) {
            rateIndex = vRATE_INDEX_HT40_MCS0 + j;
            Qc98xxTargetPowerGet(WHAL_FBIN2FREQ(*(pFreq+i),1), rateIndex, &power);
            SformatOutput(buffer2, MBUFFER-1,"|        %2.1f       ", power);
			fprintf(pFile, "%s", buffer2);

            strcat(buffer, buffer2);
        }

        strcat(buffer, "|");
        fprintf(pFile, "\n");
    }
    SformatOutput(buffer, MBUFFER-1," |========================================================================");
	fprintf(pFile, "%s\n", buffer);

}



void PrintQc98xx_2GCTLIndex(int client, const A_UINT8 *pCtlIndex){
    int i;
    char buffer[MBUFFER];
    SformatOutput(buffer, MBUFFER-1, "2G CTL Index:");
	fprintf(pFile, "%s", buffer);

    for(i=0; i < WHAL_NUM_CTLS_2G; i++) {
        if (pCtlIndex[i] == 0)
        {
            continue;
        }
        SformatOutput(buffer, MBUFFER-1, "[%d] :0x%x", i, pCtlIndex[i]);
		fprintf(pFile, "%s", buffer);

    }
}

void PrintQc98xx_2GCTLData(int client,  const A_UINT8 *ctlIndex, const CAL_CTL_DATA_2G Data[WHAL_NUM_CTLS_2G], const A_UINT8 *pFreq)
{

    int i,j;
    char buffer[MBUFFER], buffer2[MBUFFER];

    SformatOutput(buffer, MBUFFER-1," |                                                                       |");
	fprintf(pFile, "%s\n", buffer);


    SformatOutput(buffer, MBUFFER-1, " |=======================Test Group Band Edge Power======================|");
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	fprintf(pFile, "%s\n", buffer);

    for (i = 0; i < WHAL_NUM_CTLS_2G; i++)
    {
        if (ctlIndex[i] == 0)
        {
            pFreq+=WHAL_NUM_BAND_EDGES_2G;
            continue;
        }
        SformatOutput(buffer, MBUFFER-1," |                                                                       |");
		fprintf(pFile, "%s\n", buffer);

        SformatOutput(buffer, MBUFFER-1," | CTL: 0x%02x %s                                           |",
                 ctlIndex[i], sCtlType[ctlIndex[i] & QC98XX_CTL_MODE_M]);
		fprintf(pFile, "%s\n", buffer);

        SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
		fprintf(pFile, "%s\n", buffer);

        //WHAL_FBIN2FREQ(*(pFreq++),0),Data[i].ctlEdges[j].tPower,Data[i].ctlEdges[j].flag

		SformatOutput(buffer, MBUFFER-1," | edge  ");
		fprintf(pFile, "%s", buffer);

        for (j = 0; j < WHAL_NUM_BAND_EDGES_2G; j++) {
        	if (*(pFreq+j) == QC98XX_BCHAN_UNUSED) {
            	SformatOutput(buffer2, MBUFFER-1,"|  --   ");fprintf(pFile,"%s",buffer2);
                } else {
                	SformatOutput(buffer2, MBUFFER-1,"| %04d  ", WHAL_FBIN2FREQ(*(pFreq+j),1));
                    fprintf(pFile, "%s", buffer2);
                }
                strcat(buffer, buffer2);
            }

		strcat(buffer, "|");
        fprintf(pFile, "\n");

        SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
		fprintf(pFile, "%s\n", buffer);

		SformatOutput(buffer, MBUFFER-1," | power ");
		fprintf(pFile, "%s", buffer);


        for (j = 0; j < WHAL_NUM_BAND_EDGES_2G; j++) {
        	if (*(pFreq+j) == QC98XX_BCHAN_UNUSED) {
            	SformatOutput(buffer2, MBUFFER-1,"|  --   ");
				fprintf(pFile, "%s", buffer2);
				} else {
                    SformatOutput(buffer2, MBUFFER-1,"| %2d.%d  ", Data[i].ctl_edges[j].u.tPower / 2,
                        (Data[i].ctl_edges[j].u.tPower % 2) * 5);
						fprintf(pFile, "%s", buffer2);

                }
                strcat(buffer, buffer2);
            }

		strcat(buffer, "|");

        SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
		fprintf(pFile, "\n%s\n", buffer);


        SformatOutput(buffer, MBUFFER-1, " | flag  ");
		fprintf(pFile, "%s", buffer);


         for (j = 0; j < WHAL_NUM_BAND_EDGES_2G; j++) {
         	if (*(pFreq+j) == QC98XX_BCHAN_UNUSED) {
            	SformatOutput(buffer2, MBUFFER-1, "|  --   ");
				fprintf(pFile, "%s", buffer2);
				} else {
                    SformatOutput(buffer2, MBUFFER-1,"|   %1d   ", Data[i].ctl_edges[j].u.flag);
					fprintf(pFile, "%s", buffer2);

                }
                strcat(buffer, buffer2);
            }

		strcat(buffer, "|");

        pFreq+=WHAL_NUM_BAND_EDGES_2G;

        SformatOutput(buffer, MBUFFER-1," =========================================================================");
		fprintf(pFile, "\n%s\n", buffer);

    }
}

void PrintQc98xx_5GHzHeader(int client, const BIMODAL_EEP_HEADER * pBiModalHeader, const QC98XX_EEPROM *pEeprom)
{
    char  buffer[MBUFFER];
    int i, j;
    const FREQ_MODAL_EEP_HEADER *pFreqHeader = &pEeprom->freqModalHeader;
    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," |===========================5GHz Modal Header===========================|");
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," |                                                                       |");
	fprintf(pFile, "%s\n", buffer);


	SformatOutput(buffer, MBUFFER-1," | Antenna Common        0x%08X  |  Antenna Common2       0x%08X |",
        rev(pBiModalHeader->antCtrlCommon),
        rev(pBiModalHeader->antCtrlCommon2)
        );
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," | Ant Chain0  0x%04X   |  Ant Chain1  0x%04X   |  Ant Chain2  0x%04X    |",
        rev2(pBiModalHeader->antCtrlChain[0]),
        rev2(pBiModalHeader->antCtrlChain[1]),
        rev2(pBiModalHeader->antCtrlChain[2])
        );
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," | xatten1DB Ch0  (0x%02x,0x%02x,0x%02x)   | xatten1DB Ch1  (0x%02x,0x%02x,0x%02x)   |",
        pFreqHeader->xatten1DB[0].value5GLow, pFreqHeader->xatten1DB[0].value5GMid, pFreqHeader->xatten1DB[0].value5GHigh,
        pFreqHeader->xatten1DB[1].value5GLow, pFreqHeader->xatten1DB[1].value5GMid, pFreqHeader->xatten1DB[1].value5GHigh
        );
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," | xatten1DB Ch2  (0x%02x,0x%02x,0x%02x)   | xatten1Margin Ch0 (0x%02x,0x%02x,0x%02x)|",
        pFreqHeader->xatten1DB[2].value5GLow, pFreqHeader->xatten1DB[2].value5GMid, pFreqHeader->xatten1DB[2].value5GHigh,
        pFreqHeader->xatten1Margin[0].value5GLow, pFreqHeader->xatten1Margin[0].value5GMid, pFreqHeader->xatten1Margin[0].value5GHigh
        );
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," | xatten1Margin Ch1 (0x%02x,0x%02x,0x%02x)| xatten1Margin Ch2 (0x%02x,0x%02x,0x%02x)|",
        pFreqHeader->xatten1Margin[1].value5GLow, pFreqHeader->xatten1Margin[1].value5GMid, pFreqHeader->xatten1Margin[1].value5GHigh,
        pFreqHeader->xatten1Margin[2].value5GLow, pFreqHeader->xatten1Margin[2].value5GMid, pFreqHeader->xatten1Margin[2].value5GHigh
        );
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," | Volt Slope Ch0  %3d  | Volt Slope Ch1  %3d   | Volt Slope Ch2  %3d    |",
        pBiModalHeader->voltSlope[0],
        pBiModalHeader->voltSlope[1],
        pBiModalHeader->voltSlope[2]
        );
	fprintf(pFile, "%s\n", buffer);

	for (i=0; i < WHAL_NUM_CHAINS; i++) {
        SformatOutput(buffer, MBUFFER-1, " | Alpha Thermal Slope CH%d                                               |", i);
		fprintf(pFile, "%s\n", buffer);

        for (j=0; j < QC98XX_NUM_ALPHATHERM_CHANS_2G; j=j+2)
        {
            SformatOutput(buffer, MBUFFER-1, " | %04d : %02d, %02d, %02d, %02d             |  %04d : %02d, %02d, %02d, %02d            |",
                    WHAL_FBIN2FREQ(alphaThermChans5G[j], 0),
                    pEeprom->tempComp5G.alphaThermTbl[i][j][0],
                    pEeprom->tempComp5G.alphaThermTbl[i][j][1],
                    pEeprom->tempComp5G.alphaThermTbl[i][j][2],
                    pEeprom->tempComp5G.alphaThermTbl[i][j][3],
                    WHAL_FBIN2FREQ(alphaThermChans5G[j+1], 0),
                    pEeprom->tempComp5G.alphaThermTbl[i][j+1][0],
                    pEeprom->tempComp5G.alphaThermTbl[i][j+1][1],
                    pEeprom->tempComp5G.alphaThermTbl[i][j+1][2],
                    pEeprom->tempComp5G.alphaThermTbl[i][j+1][3]
                    );
			fprintf(pFile, "%s\n", buffer);

        }
    }
    for (i=0; i < QC98XX_EEPROM_MODAL_SPURS; i++)
    {
        SformatOutput(buffer, MBUFFER-1," | spurChan[%d]             0x%02x      |                                   |",
            i, pBiModalHeader->spurChans[i].spurChan
            );
		fprintf(pFile, "%s\n", buffer);

        SformatOutput(buffer, MBUFFER-1," | spurA_PrimSecChoose[%d]  0x%02x      |  spurA_PrimSecChoose[%d] 0x%02x      |",
            i, pBiModalHeader->spurChans[i].spurA_PrimSecChoose, i, pBiModalHeader->spurChans[i].spurB_PrimSecChoose
            );
		fprintf(pFile, "%s\n", buffer);

    }

	SformatOutput(buffer, MBUFFER-1," | xpaBiasLvl              0x%02x      |  antennaGainCh         %3d        |",
        pBiModalHeader->xpaBiasLvl,
        pBiModalHeader->antennaGainCh
        );
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," | rxFilterCap             0x%02x      |  rxGainCap              0x%02x      |",
        pBiModalHeader->rxFilterCap,
        pBiModalHeader->rxGainCap
        );
	fprintf(pFile, "%s\n", buffer);

    if (pEeprom->baseEepHeader.eeprom_version == QC98XX_EEP_VER1)
    {
        SformatOutput(buffer, MBUFFER-1," | txGain                  0x%02x      |  rxGain                 0x%02x      |",
            (pBiModalHeader->txrxgain >> 4) & 0xf,
            pBiModalHeader->rxGainCap & 0xf
            );
		fprintf(pFile, "%s\n", buffer);

    }

    SformatOutput(buffer, MBUFFER-1," |                                   |                                   |");
	fprintf(pFile, "%s\n", buffer);

}

void PrintQc98xx_5GHzCalData(int client, const QC98XX_EEPROM *pEeprom)
{
    A_UINT16 numPiers = WHAL_NUM_11A_CAL_PIERS;
    A_UINT16 pc; //pier count
	A_UINT16 a,b;
    char  buffer[MBUFFER];
    const A_UINT8 *pPiers = &pEeprom->calFreqPier5G[0];
    const CAL_DATA_PER_FREQ_OLPC *pData = &pEeprom->calPierData5G[0];
    int i;

    SformatOutput(buffer, MBUFFER-1, " |=================5G Power Calibration Information =====================|");
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	fprintf(pFile, "%s\n", buffer);

    for (i = 0; i < WHAL_NUM_CHAINS; ++i)
    {
        SformatOutput(buffer, MBUFFER-1, " |                          Chain %d                                      |", i);
		fprintf(pFile, "%s\n", buffer);

        SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
		fprintf(pFile, "%s\n", buffer);

        SformatOutput(buffer, MBUFFER-1, " | Freq  txgainIdx0 dacGain0  Pwr0  txgainIdx1 dacGain1 Pwr1  Volt  Temp |");
		fprintf(pFile, "%s\n", buffer);

        SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
		fprintf(pFile, "%s\n", buffer);

        for(pc = 0; pc < numPiers; pc++) {
	   	   	a = (pData[pc].calPerPoint[i].power_t8[0]);
		   	b = (pData[pc].calPerPoint[i].power_t8[1]);	
	    	a = (a>>8)|((a&0xff)<<8);
			b = (b>>8)|((b&0xff)<<8);
		
			SformatOutput(buffer, MBUFFER-1, " | %04d    %4d      %3d     %3d      %4d      %3d     %3d   %3d   %3d  |",
                WHAL_FBIN2FREQ(pPiers[pc], 0),
                pData[pc].calPerPoint[i].txgainIdx[0], pData[pc].dacGain[0], a/8,
                pData[pc].calPerPoint[i].txgainIdx[1], pData[pc].dacGain[1], b/8,
                pData[pc].voltCalVal, pData[pc].thermCalVal
                );
			fprintf(pFile, "%s\n", buffer);

        }
        SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
		fprintf(pFile, "%s\n", buffer);

    }
    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	fprintf(pFile, "%s\n", buffer);

}



void PrintQc98xx_5GLegacyTargetPower(int client, const CAL_TARGET_POWER_LEG *pVals, const A_UINT8 *pFreq)
{
    int i,j;
    char buffer[MBUFFER],buffer2[MBUFFER];

    SformatOutput(buffer, MBUFFER-1, " |===========================5G Target Powers============================|");
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," |  OFDM     ");
	fprintf(pFile, "%s", buffer);


    for (j = 0; j < WHAL_NUM_11A_LEG_TARGET_POWER_CHANS; j++) {
        SformatOutput(buffer2, MBUFFER-1, "|  %04d   ", WHAL_FBIN2FREQ(*(pFreq+j),0));
		fprintf(pFile, "%s", buffer2);

        strcat(buffer, buffer2);
    }

    strcat(buffer, "|");

    SformatOutput(buffer, MBUFFER-1,"|===========|=========|=========|=========|=========|=========|=========|");
	fprintf(pFile, "\n %s\n", buffer);


    for (j = 0; j < WHAL_NUM_LEGACY_TARGET_POWER_RATES; j++) {
        SformatOutput(buffer, MBUFFER-1," |  %s     ",sRatePrintLegacy[j]);
		fprintf(pFile, "%s", buffer);

        for(i=0; i < WHAL_NUM_11A_LEG_TARGET_POWER_CHANS; i++) {
            SformatOutput(buffer2, MBUFFER-1,"|  %2d.%d   ", pVals[i].tPow2x[j]/2, (pVals[i].tPow2x[j] % 2) * 5);
			fprintf(pFile, "%s", buffer2);

            strcat(buffer, buffer2);
        }
	fprintf(pFile, "\n");
    strcat(buffer, "|");
    }
    SformatOutput(buffer, MBUFFER-1," |========================================================================");
	fprintf(pFile, "%s\n", buffer);

}

void PrintQc98xx_5GHT20TargetPower(int client, const CAL_TARGET_POWER_11A_20 *pVals, const A_UINT8 *pFreq)
{
    int i,j;
    char buffer[MBUFFER],buffer2[MBUFFER];
    double power;
    int rateIndex;

    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," |  VHT20    ");
	fprintf(pFile, "%s", buffer);


    for (j = 0; j < WHAL_NUM_11A_20_TARGET_POWER_CHANS; j++)
    {
        SformatOutput(buffer2, MBUFFER-1,"|  %04d   ", WHAL_FBIN2FREQ(*(pFreq+j),0));
		fprintf(pFile, "%s", buffer2);

        strcat(buffer, buffer2);
    }

    strcat(buffer, "|");

    SformatOutput(buffer, MBUFFER-1,"|===========|=========|=========|=========|=========|=========|=========|");
	fprintf(pFile, "\n %s\n", buffer);


    for (j = 0; j < RATE_PRINT_VHT_SIZE; j++) {
        SformatOutput(buffer, MBUFFER-1," |  %s ",sRatePrintVHT[j]);
		fprintf(pFile, "%s", buffer);

        for(i=0; i < WHAL_NUM_11A_20_TARGET_POWER_CHANS; i++) {
            rateIndex = vRATE_INDEX_HT20_MCS0 + j;
            Qc98xxTargetPowerGet(WHAL_FBIN2FREQ(*(pFreq+i),0), rateIndex, &power);
            SformatOutput(buffer2, MBUFFER-1,"|  %2.1f   ", power);
			fprintf(pFile, "%s", buffer2);

            strcat(buffer, buffer2);
        }
    strcat(buffer, "|");
	fprintf(pFile, "\n");
    }
    SformatOutput(buffer, MBUFFER-1," |========================================================================");
	fprintf(pFile, "%s\n", buffer);

}



void PrintQc98xx_5GHT40TargetPower(int client, const CAL_TARGET_POWER_11A_40 *pVals, const A_UINT8 *pFreq)
{
    int i,j;
    char buffer[MBUFFER], buffer2[MBUFFER];
    double power;
    int rateIndex;

    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," |  VHT40    ");
	fprintf(pFile, "%s", buffer);


    for (j = 0; j < WHAL_NUM_11A_40_TARGET_POWER_CHANS; j++) {
        SformatOutput(buffer2, MBUFFER-1,"|  %04d   ", WHAL_FBIN2FREQ(*(pFreq+j),0));
		fprintf(pFile, "%s", buffer2);

        strcat(buffer, buffer2);
    }

    strcat(buffer, "|");

    SformatOutput(buffer, MBUFFER-1,"|===========|=========|=========|=========|=========|=========|=========|");
	fprintf(pFile, "\n %s\n", buffer);


    for (j = 0; j < RATE_PRINT_VHT_SIZE; j++) {
        SformatOutput(buffer, MBUFFER-1," |  %s ", sRatePrintVHT[j]);
		fprintf(pFile, "%s", buffer);

        for(i=0; i < WHAL_NUM_11A_40_TARGET_POWER_CHANS; i++) {
            rateIndex = vRATE_INDEX_HT40_MCS0 + j;
            Qc98xxTargetPowerGet(WHAL_FBIN2FREQ(*(pFreq+i),0), rateIndex, &power);
            SformatOutput(buffer2, MBUFFER-1,"|  %2.1f   ", power);
			fprintf(pFile, "%s", buffer2);

            strcat(buffer, buffer2);
        }
    strcat(buffer, "|");
    fprintf(pFile, "\n");
	}
    SformatOutput(buffer, MBUFFER-1," |========================================================================");
	fprintf(pFile, "%s\n", buffer);

}


void PrintQc98xx_5GHT80TargetPower(int client, const CAL_TARGET_POWER_11A_80 *pVals, const A_UINT8 *pFreq)
{
    int i,j;
    char buffer[MBUFFER], buffer2[MBUFFER];
    double power;
    int rateIndex;
    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," |  VHT80    ");
	fprintf(pFile, "%s", buffer);


    for (j = 0; j < WHAL_NUM_11A_80_TARGET_POWER_CHANS; j++) {
        SformatOutput(buffer2, MBUFFER-1,"|  %04d   ", WHAL_FBIN2FREQ(*(pFreq+j),0));
		fprintf(pFile, "%s", buffer2);

        strcat(buffer, buffer2);
    }

    strcat(buffer, "|");

    SformatOutput(buffer, MBUFFER-1,"|===========|=========|=========|=========|=========|=========|=========|");
	fprintf(pFile, "\n %s\n", buffer);


    for (j = 0; j < RATE_PRINT_VHT_SIZE; j++) {
        SformatOutput(buffer, MBUFFER-1," |  %s ", sRatePrintVHT[j]);
		fprintf(pFile, "%s", buffer);

        for(i=0; i < WHAL_NUM_11A_80_TARGET_POWER_CHANS; i++) {
            rateIndex = vRATE_INDEX_HT80_MCS0 + j;
            Qc98xxTargetPowerGet(WHAL_FBIN2FREQ(*(pFreq+i),0), rateIndex, &power);
            SformatOutput(buffer2, MBUFFER-1,"|  %2.1f   ", power);
			fprintf(pFile, "%s", buffer2);

            strcat(buffer, buffer2);
        }
        strcat(buffer, "|");
		fprintf(pFile, "\n");
    }
    SformatOutput(buffer, MBUFFER-1," |========================================================================");
	fprintf(pFile, "%s\n", buffer);
}



void PrintQc98xx_5GCTLIndex(int client, const A_UINT8 *pCtlIndex){
    int i;
    char buffer[MBUFFER];
    SformatOutput(buffer, MBUFFER-1, "5G CTL Index:");
    for(i=0; i < WHAL_NUM_CTLS_5G; i++) {
        if (pCtlIndex[i] == 0)
        {
            continue;
        }
        SformatOutput(buffer, MBUFFER-1,"[%d] :0x%x", i, pCtlIndex[i]);
    }
}



void PrintQc98xx_5GCTLData(int client, const A_UINT8 *ctlIndex, const CAL_CTL_DATA_5G Data[WHAL_NUM_CTLS_5G], const A_UINT8 *pFreq)
{

    int i,j;
    char buffer[MBUFFER],buffer2[MBUFFER];

    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	fprintf(pFile, "%s\n", buffer);


    SformatOutput(buffer, MBUFFER-1," |=======================Test Group Band Edge Power======================|");
	fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," |                                                                       |");
	fprintf(pFile, "%s\n", buffer);

    for (i = 0; i < WHAL_NUM_CTLS_5G; i++)
    {
        if (ctlIndex[i] == 0)
        {
            pFreq+=WHAL_NUM_BAND_EDGES_5G;
            continue;
        }
        SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
		fprintf(pFile, "%s\n", buffer);
        SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
		fprintf(pFile, "%s\n", buffer);
		SformatOutput(buffer, MBUFFER-1, " | edge  ");
		fprintf(pFile, "%s", buffer);

        for (j = 0; j < WHAL_NUM_BAND_EDGES_5G; j++) {
            if (*(pFreq+j) == QC98XX_BCHAN_UNUSED) {
                SformatOutput(buffer2, MBUFFER-1,"|  --   ");
				fprintf(pFile, "%s", buffer2);

            } else {
                SformatOutput(buffer2, MBUFFER-1,"| %04d  ", WHAL_FBIN2FREQ(*(pFreq+j),0));
				fprintf(pFile, "%s", buffer2);

            }
            strcat(buffer, buffer2);
        }

	strcat(buffer, "|");
	fprintf(pFile, "\n");

    SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
	fprintf(pFile, "%s\n", buffer);


    SformatOutput(buffer, MBUFFER-1," | power ");
	fprintf(pFile, "%s", buffer);


    for (j = 0; j < WHAL_NUM_BAND_EDGES_5G; j++) {
    	if (*(pFreq+j) == QC98XX_BCHAN_UNUSED) {
        	SformatOutput(buffer2, MBUFFER-1,"|  --   ");
			fprintf(pFile, "%s", buffer2);

            } else {
                SformatOutput(buffer2, MBUFFER-1,"| %2d.%d  ", Data[i].ctl_edges[j].u.tPower / 2,
                   (Data[i].ctl_edges[j].u.tPower % 2) * 5);
				fprintf(pFile, "%s", buffer2);

            }
            strcat(buffer, buffer2);
        }

        strcat(buffer, "|");
        SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
		fprintf(pFile, "\n%s\n", buffer);


        SformatOutput(buffer, MBUFFER-1," | flag  ");
		fprintf(pFile, "%s", buffer);


        for (j = 0; j < WHAL_NUM_BAND_EDGES_5G; j++) {
            if (*(pFreq+j) == QC98XX_BCHAN_UNUSED) {
                SformatOutput(buffer2, MBUFFER-1,"|  --   ");
				fprintf(pFile, "%s", buffer2);

            } else {
                SformatOutput(buffer2, MBUFFER-1,"|   %1d   ", Data[i].ctl_edges[j].u.flag);
				fprintf(pFile, "%s", buffer2);

            }
            strcat(buffer, buffer2);
        }

        strcat(buffer, "|");

        pFreq+=WHAL_NUM_BAND_EDGES_5G;

        SformatOutput(buffer, MBUFFER-1," =========================================================================");
		fprintf(pFile, "\n%s\n", buffer);

    }
}

void PrintQc98xxStruct(QC98XX_EEPROM *pEeprom, int client)

{
	A_UINT16 a,b;
#define MBUFFER 1024
    char buffer[MBUFFER],buffer2[MBUFFER];
    int i;

    SformatOutput(buffer, MBUFFER-1,"start get all");
	fprintf(pFile, "%s\n", buffer);
    SformatOutput(buffer, MBUFFER-1,"                         ----------------------                           ");
	fprintf(pFile, "%s\n", buffer);
    SformatOutput(buffer, MBUFFER-1,  " =======================| QC98XX CAL STRUCTURE |==========================");
	fprintf(pFile, "%s\n", buffer);
    SformatOutput(buffer, MBUFFER-1," |                       -----------------------                         |");
	fprintf(pFile, "%s\n", buffer);
    SformatOutput(buffer, MBUFFER-1," |                                   |                                   |");
	fprintf(pFile, "%s\n", buffer);
    SformatOutput(buffer, MBUFFER-1," | Board Data Version      %2d        |  Template Version       %2d        |",
        pEeprom->baseEepHeader.eeprom_version,
        pEeprom->baseEepHeader.template_version
        );

    fprintf(pFile, "%s\n", buffer);

    SformatOutput(buffer, MBUFFER-1," |-----------------------------------------------------------------------|");
	fprintf(pFile, "%s\n", buffer);
    SformatOutput(buffer, MBUFFER-1," | MacAddress: 0x%02X:%02X:%02X:%02X:%02X:%02X                                       |",
          pEeprom->baseEepHeader.macAddr[0],pEeprom->baseEepHeader.macAddr[1],pEeprom->baseEepHeader.macAddr[2],
          pEeprom->baseEepHeader.macAddr[3],pEeprom->baseEepHeader.macAddr[4],pEeprom->baseEepHeader.macAddr[5]);
	fprintf(pFile, "%s\n", buffer);
    SformatOutput(buffer, MBUFFER-1," | Customer Data in hex                                                  |");
	fprintf(pFile, "%s\n", buffer);
    SformatOutput(buffer, MBUFFER-1, " |");
	fprintf(pFile, "%s", buffer);
    for(i=0; i < CUSTOMER_DATA_SIZE; i++) {
        SformatOutput(buffer2, MBUFFER-1," %02X", pEeprom->baseEepHeader.custData[i] );
        strcat(buffer, buffer2);
    }
    strcat(buffer, "         |");
	fprintf(pFile, "%s\n", buffer);
    SformatOutput(buffer, MBUFFER-1," |-----------------------------------------------------------------------|");
	fprintf(pFile, "%s", buffer);
	PrintQc98xxBaseHeader(0, &(pEeprom->baseEepHeader), pEeprom);
	if (pEeprom->baseEepHeader.opCapBrdFlags.opFlags & WHAL_OPFLAGS_11G)
    {
        PrintQc98xx_2GHzHeader(client, &(pEeprom->biModalHeader[1]), pEeprom);
        PrintQc98xx_2GHzCalData(client, pEeprom);
        PrintQc98xx_2GLegacyTargetPower(client, pEeprom->targetPower2G, pEeprom->targetFreqbin2G);
        PrintQc98xx_2GCCKTargetPower(client, pEeprom->targetPowerCck, pEeprom->targetFreqbinCck);
        PrintQc98xx_2GHT20TargetPower(client, pEeprom->targetPower2GVHT20, pEeprom->targetFreqbin2GVHT20);
        PrintQc98xx_2GHT40TargetPower(client, pEeprom->targetPower2GVHT40, pEeprom->targetFreqbin2GVHT40);
        PrintQc98xx_2GCTLData(client, pEeprom->ctlIndex2G, pEeprom->ctlData2G, &pEeprom->ctlFreqbin2G[0][0]);
    }
    if (pEeprom->baseEepHeader.opCapBrdFlags.opFlags & WHAL_OPFLAGS_11A)
    {
        PrintQc98xx_5GHzHeader(client, &(pEeprom->biModalHeader[0]), pEeprom);
        PrintQc98xx_5GHzCalData(client, pEeprom);
        PrintQc98xx_5GLegacyTargetPower(client, pEeprom->targetPower5G, pEeprom->targetFreqbin5G);
        PrintQc98xx_5GHT20TargetPower(client, pEeprom->targetPower5GVHT20, pEeprom->targetFreqbin5GVHT20);
        PrintQc98xx_5GHT40TargetPower(client, pEeprom->targetPower5GVHT40, pEeprom->targetFreqbin5GVHT40);
        PrintQc98xx_5GHT80TargetPower(client, pEeprom->targetPower5GVHT80, pEeprom->targetFreqbin5GVHT80);
        PrintQc98xx_5GCTLData(client, pEeprom->ctlIndex5G, pEeprom->ctlData5G, &pEeprom->ctlFreqbin5G[0][0]);
    }

}
/*
 * This is NOT the ideal way to write OTP since it does not handle
 * media errors.  It's much better to use the otpstream_* API.
 * This capability is here to help salvage parts that have previously
 * had OTP written.
 */
void
WriteTargetOTP(int dev, A_UINT32 offset, A_UINT8 *buffer, A_UINT32 length)
{
    A_UINT32 status_mask;
    A_UINT32 otp_status;
    int i;

    /* Enable OTP read/write power */
    WriteTargetWord(dev, RTC_SOC_BASE_ADDRESS+OTP_OFFSET, OTP_VDD12_EN_SET(1) | OTP_LDO25_EN_SET(1));
    status_mask = OTP_STATUS_VDD12_EN_READY_SET(1) | OTP_STATUS_LDO25_EN_READY_SET(1);
    do {
        ReadTargetWord(dev, RTC_SOC_BASE_ADDRESS+OTP_STATUS_OFFSET, &otp_status);
    } while ((otp_status & (OTP_STATUS_VDD12_EN_READY_MASK|OTP_STATUS_LDO25_EN_READY_MASK)) != status_mask);

    /* Conservatively set OTP read/write timing for 110MHz core clock */
    WriteTargetWord(dev, EFUSE_BASE_ADDRESS+VDDQ_SETTLE_TIME_REG_OFFSET, 2200);
    WriteTargetWord(dev, EFUSE_BASE_ADDRESS+PG_STROBE_PW_REG_OFFSET, 605);
    WriteTargetWord(dev, EFUSE_BASE_ADDRESS+RD_STROBE_PW_REG_OFFSET, 6);

    /* Enable eFuse for write */
    WriteTargetWord(dev, EFUSE_BASE_ADDRESS+EFUSE_WR_ENABLE_REG_OFFSET, EFUSE_WR_ENABLE_REG_V_SET(1));
    WriteTargetWord(dev, EFUSE_BASE_ADDRESS+BITMASK_WR_REG_OFFSET, 0x00);

    /* Write data to OTP */
    for (i=0; i<length; i++, offset++) {
        A_UINT32 efuse_word;
        A_UINT32 readback;
        int attempt;

#define EFUSE_WRITE_COUNT 3
        efuse_word = (A_UINT32)buffer[i];
        for (attempt=1; attempt<=EFUSE_WRITE_COUNT; attempt++) {
            WriteTargetWord(dev, EFUSE_BASE_ADDRESS+EFUSE_INTF0_OFFSET+(offset<<2), efuse_word);
        }

        /* verify */
        ReadTargetWord(dev, EFUSE_BASE_ADDRESS+EFUSE_INTF0_OFFSET+(offset<<2), &readback);
        if (efuse_word != readback) {
            fprintf(stderr, "OTP write failed. Offset=%d, Value=0x%x, Readback=0x%x\n",
                        offset, efuse_word, readback);
            break;
        }
    }

    /* Disable OTP */
    WriteTargetWord(dev, RTC_SOC_BASE_ADDRESS+OTP_OFFSET, 0);
}

unsigned int
parse_address(char *optarg)
{
    unsigned int address;

    /* may want to add support for symbolic addresses here */

    address = strtoul(optarg, NULL, 0);

    return address;
}

int
main (int argc, char **argv) {
    int c, fd, dev;
    FILE * dump_fd;
    unsigned int address, length;
    A_UINT32 param;
    char filename[PATH_MAX];
    char devicename[PATH_MAX];
    unsigned int cmd;
    A_UINT8 *buffer;
	int i, CAL_FLAG = 0;
    unsigned int bitwise_mask;    
    progname = argv[0];
 
    if (argc == 1) usage();
    flag = 0;
    memset(filename, '\0', sizeof(filename));
    memset(devicename, '\0', sizeof(devicename));

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"address", 1, NULL, 'a'},
            {"and", 1, NULL, 'n'},
            {"device", 1, NULL, 'D'},
            {"get", 0, NULL, 'g'},
            {"file", 1, NULL, 'f'},
            {"length", 1, NULL, 'l'},
            {"or", 1, NULL, 'o'},
            {"otp", 0, NULL, 'O'},
            {"param", 1, NULL, 'p'},
            {"quiet", 0, NULL, 'q'},
            {"read", 0, NULL, 'r'},
            {"set", 0, NULL, 's'},
            {"value", 1, NULL, 'p'},
            {"write", 0, NULL, 'w'},
            {"hex", 0, NULL, 'x'},
			{"cal", 0, NULL, 'c'},
            {0, 0, 0, 0}
        };

        c = getopt_long (argc, argv, "xrwgsqOf:l:a:p:c:n:o:D:",
                         long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'r':
            cmd = DIAG_READ_TARGET;
            break;

        case 'w':
            cmd = DIAG_WRITE_TARGET;
            break;

        case 'g':
            cmd = DIAG_READ_WORD;
            break;

        case 's':
            cmd = DIAG_WRITE_WORD;
            break;

        case 'f':
            memset(filename, '\0', sizeof(filename));
            strncpy(filename, optarg, sizeof(filename));
            flag |= FILE_FLAG;
            break;

        case 'l':
            length = parse_address(optarg);
            flag |= LENGTH_FLAG;
            break;

        case 'a':
            address = parse_address(optarg);
            flag |= ADDRESS_FLAG;
            break;

        case 'p':
            param = strtoul(optarg, NULL, 0);
            flag |= PARAM_FLAG;
            break;

        case 'n':
            flag |= PARAM_FLAG | AND_OP_FLAG | BITWISE_OP_FLAG;
            bitwise_mask = strtoul(optarg, NULL, 0);
            break;
            
        case 'o':                
            flag |= PARAM_FLAG | BITWISE_OP_FLAG;
            bitwise_mask = strtoul(optarg, NULL, 0);
            break;

        case 'O':
            flag |= OTP_FLAG;
            break;

        case 'q':
            flag |= QUIET_FLAG;
            break;
            
        case 'D':
            strncpy(devicename, optarg, sizeof(devicename)-9);
            strcat(devicename, "/athdiag");
            flag |= DEVICE_FLAG;
            break;
			
        case 'x':
            flag |= HEX_FLAG;
            break;
		
		case 'c':
			CAL_FLAG = 1;
			break;

        default:
            fprintf(stderr, "Cannot understand '%s'\n", argv[option_index]);
            usage();
        }
    }

    for (;;) {
        /* DIAG uses a sysfs special file which may be auto-detected */
        if (!(flag & DEVICE_FLAG)) {
    	    FILE *find_dev;
    	    size_t nbytes;
    
            /*
             * Convenience: if no device was specified on the command
             * line, try to figure it out.  Typically there's only a
             * single device anyway.
             */
    	    find_dev = popen("find /sys/devices -name athdiag | head -1", "r");
            if (find_dev) {
    	        nbytes=fread(devicename, 1, sizeof(devicename), find_dev);
    	        pclose(find_dev);
    
    	        if (nbytes > 15) {
                    /* auto-detect possibly successful */
    	            devicename[nbytes-1]='\0'; /* replace \n with 0 */
    	        } else {
                    strcpy(devicename, "unknown_DIAG_device");
                }
            }
        }
    
        dev = open(devicename, O_RDWR);
        if (dev >= 0) {
            break; /* successfully opened diag special file */
        } else {
            fprintf(stderr, "err %s failed (%d) to open DIAG file (%s)\n",
                __FUNCTION__, errno, devicename);
            exit(1);
        }
    }
    switch(cmd)
    {
    case DIAG_READ_TARGET:
        if ((flag & (ADDRESS_FLAG | LENGTH_FLAG | FILE_FLAG )) == 
                (ADDRESS_FLAG | LENGTH_FLAG | FILE_FLAG))
        {
            if ((dump_fd = fopen(filename, "wb+")) < 0)
            {
                fprintf(stderr, "err %s cannot create/open output file (%s)\n",
                        __FUNCTION__, filename);
                exit(1);
            }

            buffer = (A_UINT8 *)MALLOC(MAX_BUF);

            nqprintf(
                    "DIAG Read Target (address: 0x%x, length: %d, filename: %s)\n",
                    address, length, filename);


            {
                unsigned int remaining = length;

				unsigned int i = 0;
				unsigned int cc;

                if(flag & HEX_FLAG)
                {
                    if (flag & OTP_FLAG) {
                        fprintf(dump_fd,"target otp dump area[0x%08x - 0x%08x]",address,address+length);
                    } else {
                        fprintf(dump_fd,"target mem dump area[0x%08x - 0x%08x]",address,address+length);
                    }
                }
                while (remaining)
                {
                    length = (remaining > MAX_BUF) ? MAX_BUF : remaining;
                    if (flag & OTP_FLAG) {
                        ReadTargetOTP(dev, address, buffer, length);
                    } else {
                        ReadTargetRange(dev, address, buffer, length);
                    }

                    if(flag & HEX_FLAG)
                    {
                        for(i=0;i<length;i+=4)
                        {
                            if(i%16 == 0)
                                fprintf(dump_fd,"\n0x%08x:\t",address+i);
                            fprintf(dump_fd,"0x%08x\t",*(A_UINT32*)(buffer+i));
                        }
                    }
                    else
                    {
                        fwrite(buffer,1 , length, dump_fd);
                    }
                    
                    /*  swapping the values as the HIF layer swaps the bytes on a 4-byte boundary
                        which causes reverse in the bytes, example if a[4], then a[3] is read as 
                        a[0] only u32 remain same*/

                   if (CAL_FLAG) {
                        pFile = fopen("Driver_Cal_structure.txt", "w");
                        for(i = 0; i < length; i = i + 4) {
                            cc = buffer[i];
                            buffer[i] = buffer[i+3];
                            buffer[i+3] = cc;
                            cc = buffer[i+1];
                            buffer[i+1] = buffer[i+2];
                            buffer[i+2] = cc;
                    	}
	                    pQc9kEepromArea = buffer;
    	                PrintQc98xxStruct((QC98XX_EEPROM *)buffer, 0);
        	            fclose(pFile);
                    }

					remaining -= length;
                    address += length;
                }
            }
            fclose(dump_fd);
            free(buffer);
        } else {
            usage();
        }
        break;

    case DIAG_WRITE_TARGET:
        if (!(flag & ADDRESS_FLAG))
        {
            usage(); /* no address specified */
        }
        if (!(flag & (FILE_FLAG | PARAM_FLAG)))
        {
            usage(); /* no data specified */
        }
        if ((flag & FILE_FLAG) && (flag & PARAM_FLAG))
        {
            usage(); /* too much data specified */
        }

        if (flag & FILE_FLAG)
        {
            struct stat filestat;
            unsigned int file_length;

            if ((fd = open(filename, O_RDONLY)) < 0)
            {
                fprintf(stderr, "err %s Could not open file (%s)\n", __FUNCTION__, filename);
                exit(1);
            }
            memset(&filestat, '\0', sizeof(struct stat));
            buffer = (A_UINT8 *)MALLOC(MAX_BUF);
            fstat(fd, &filestat);
            file_length = filestat.st_size;
            if (file_length == 0) {
                fprintf(stderr, "err %s Zero length input file (%s)\n", __FUNCTION__, filename);
                exit(1);
            }

            if (flag & LENGTH_FLAG) {
                if (length > file_length) {
                    fprintf(stderr, "err %s file %s: length (%d) too short (%d)\n", __FUNCTION__,
                        filename, file_length, length);
                    exit(1);
                }
            } else {
                length = file_length;
            }

            nqprintf(
                 "DIAG Write Target (address: 0x%x, filename: %s, length: %d)\n",
                  address, filename, length);

        }
        else
        { /* PARAM_FLAG */
            nqprintf(
                 "DIAG Write Word (address: 0x%x, value: 0x%x)\n",
                  address, param);
            length = sizeof(param);
            buffer = (A_UINT8 *)&param;
            fd = -1;
        }

        /*
         * Write length bytes of data to memory/OTP.
         * Data is either present in buffer OR
         * needs to be read from fd in MAX_BUF chunks.
         *
         * Within the kernel, the implementation of
         * DIAG_WRITE_TARGET further limits the size
         * of each transfer over the interconnect.
         */ 
        {
            unsigned int remaining;
            unsigned int otp_check_address = address;

            if (flag & OTP_FLAG) { /* Validate OTP write before committing anything */
                remaining = length;
                while (remaining)
                {
                    int nbyte;

                    length = (remaining > MAX_BUF) ? MAX_BUF : remaining;
                    if (fd > 0)
                    {
                        nbyte = read(fd, buffer, length);
                        if (nbyte != length) {
                            fprintf(stderr, "err %s read from file failed (%d)\n", __FUNCTION__, nbyte);
                            exit(1);
                        }
                    }

                    if ((flag & OTP_FLAG) && !ValidWriteOTP(dev, otp_check_address, buffer, length))
                    {
                            exit(1);
                    }

                    remaining -= length;
                    otp_check_address += length;
                }
                (void)lseek(fd, 0, SEEK_SET);
            }

            remaining = length;
            while (remaining)
            {
                int nbyte;

                length = (remaining > MAX_BUF) ? MAX_BUF : remaining;
                if (fd > 0)
                {
                    nbyte = read(fd, buffer, length);
                    if (nbyte != length) {
                        fprintf(stderr, "err %s read from file failed (%d)\n", __FUNCTION__, nbyte);
                        exit(1);
                    }
                }

                if (flag & OTP_FLAG) {
                    WriteTargetOTP(dev, address, buffer, length);
                } else {
                    WriteTargetRange(dev, address, buffer, length);
                }

                remaining -= length;
                address += length;
            }
        }

        if (flag & FILE_FLAG) {
            free(buffer);
            close(fd);
        }

        break;

    case DIAG_READ_WORD:
        if ((flag & (ADDRESS_FLAG)) == (ADDRESS_FLAG))
        {
            nqprintf("DIAG Read Word (address: 0x%x)\n", address);
            ReadTargetWord(dev, address, &param);

            if (quiet()) {
                printf("0x%x\n", param);
            } else {
                printf("Value in target at 0x%x: 0x%x (%d)\n", address, param, param);
            }
        }
        else usage();
        break;

    case DIAG_WRITE_WORD:
        if ((flag & (ADDRESS_FLAG | PARAM_FLAG)) == (ADDRESS_FLAG | PARAM_FLAG))
        {
            A_UINT32 origvalue = 0;
            
            if (flag & BITWISE_OP_FLAG) {
                /* first read */    
                ReadTargetWord(dev, address, &origvalue);
                param = origvalue;
                
                /* now modify */
                if (flag & AND_OP_FLAG) {
                    param &= bitwise_mask;        
                } else {
                    param |= bitwise_mask;
                }               
            
                /* fall through to write out the parameter */
            }
            
            if (flag & BITWISE_OP_FLAG) {
                if (quiet()) {
                    printf("0x%x\n", origvalue);
                } else {
                    printf("DIAG Bit-Wise (%s) modify Word (address: 0x%x, orig:0x%x, new: 0x%x,  mask:0x%X)\n", 
                       (flag & AND_OP_FLAG) ? "AND" : "OR", address, origvalue, param, bitwise_mask );   
                }
            } else{ 
                nqprintf("DIAG Write Word (address: 0x%x, param: 0x%x)\n", address, param);
            }
            
            WriteTargetWord(dev, address, param);
        }
        else usage();
        break;

    default:
        usage();
    }

    exit (0);
}
