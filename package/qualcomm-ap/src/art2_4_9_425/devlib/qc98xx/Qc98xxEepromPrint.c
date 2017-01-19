
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "wlantype.h"
#include "smatch.h"
#include "ErrorPrint.h"
#include "NartError.h"

#include "Qc9KDevice.h"
#include "Qc98xxDevice.h"
#include "Qc9KEeprom.h"
#include "qc98xx_eeprom.h"

#include "Qc98xxEepromStruct.h"
#include "rate_constants.h"
#include "vrate_constants.h"
#include "UserPrint.h"

#define MBUFFER 1024
#define MAX_DATA_LENGTH  512

#define SWAP32(_x) ((u_int32_t)(                       \
                    (((const u_int8_t *)(&_x))[0]) |        \
                    (((const u_int8_t *)(&_x))[1]<< 8) |    \
                    (((const u_int8_t *)(&_x))[2]<<16) |    \
                    (((const u_int8_t *)(&_x))[3]<<24)))
extern void Qc98xx_swap_eeprom(QC98XX_EEPROM *eep);

extern A_UINT32 fwBoardDataAddress;

A_UINT8 alphaThermChans2G[QC98XX_NUM_ALPHATHERM_CHANS_2G] = {
    WHAL_FREQ2FBIN(2412, 1), WHAL_FREQ2FBIN(2437,1), WHAL_FREQ2FBIN(2457,1), WHAL_FREQ2FBIN(2472,1)
};

A_UINT8 alphaThermChans5G[QC98XX_NUM_ALPHATHERM_CHANS_5G] = {
    WHAL_FREQ2FBIN(5180,0), WHAL_FREQ2FBIN(5240,0), WHAL_FREQ2FBIN(5260,0), WHAL_FREQ2FBIN(5320,0),
    WHAL_FREQ2FBIN(5500,0), WHAL_FREQ2FBIN(5600,0), WHAL_FREQ2FBIN(5700,0), WHAL_FREQ2FBIN(5805,0)
};
    
void Qc98xxPrintOffset()
{
    UserPrint("offsetof(QC98XX_EEPROM,baseEepHeader) is %d (%x)\n", offsetof(QC98XX_EEPROM,baseEepHeader), offsetof(QC98XX_EEPROM,baseEepHeader)); //  96B   

    UserPrint("offsetof(QC98XX_EEPROM,biModalHeader) is %d (%x)\n", offsetof(QC98XX_EEPROM,biModalHeader), offsetof(QC98XX_EEPROM,biModalHeader)); // 132B   
    UserPrint("offsetof(QC98XX_EEPROM,freqModalHeader) is %d (%x)\n", offsetof(QC98XX_EEPROM,freqModalHeader), offsetof(QC98XX_EEPROM,freqModalHeader)); // 132B   
    UserPrint("offsetof(QC98XX_EEPROM,chipCalData) is %d (%x)\n", offsetof(QC98XX_EEPROM,chipCalData), offsetof(QC98XX_EEPROM,chipCalData)); // 132B   

    UserPrint("offsetof(QC98XX_EEPROM,calFreqPier2G) is %d (%x)\n", offsetof(QC98XX_EEPROM,calFreqPier2G), offsetof(QC98XX_EEPROM,calFreqPier2G)); //  32B
    UserPrint("offsetof(QC98XX_EEPROM,calPierData2G) is %d (%x)\n", offsetof(QC98XX_EEPROM,calPierData2G), offsetof(QC98XX_EEPROM,calPierData2G)); // 576B   

    UserPrint("offsetof(QC98XX_EEPROM,targetFreqbinCck) is %d (%x)\n", offsetof(QC98XX_EEPROM,targetFreqbinCck), offsetof(QC98XX_EEPROM,targetFreqbinCck)); //   2B
    UserPrint("offsetof(QC98XX_EEPROM,pad1) is %d (%x)\n", offsetof(QC98XX_EEPROM,pad1), offsetof(QC98XX_EEPROM,pad1)); //   2B

    UserPrint("offsetof(QC98XX_EEPROM,targetFreqbin2G) is %d (%x)\n", offsetof(QC98XX_EEPROM,targetFreqbin2G), offsetof(QC98XX_EEPROM,targetFreqbin2G)); //   3B
    UserPrint("offsetof(QC98XX_EEPROM,pad2) is %d (%x)\n", offsetof(QC98XX_EEPROM,pad2), offsetof(QC98XX_EEPROM,pad2)); //   1B

    UserPrint("offsetof(QC98XX_EEPROM,targetFreqbin2GVHT20) is %d (%x)\n", offsetof(QC98XX_EEPROM,targetFreqbin2GVHT20), offsetof(QC98XX_EEPROM,targetFreqbin2GVHT20)); //   3B

    UserPrint("offsetof(QC98XX_EEPROM,targetFreqbin2GVHT40) is %d (%x)\n", offsetof(QC98XX_EEPROM,targetFreqbin2GVHT40), offsetof(QC98XX_EEPROM,targetFreqbin2GVHT40)); //   3B

    UserPrint("offsetof(QC98XX_EEPROM,targetPowerCck) is %d (%x)\n", offsetof(QC98XX_EEPROM,targetPowerCck),offsetof(QC98XX_EEPROM,targetPowerCck)); //   8B
    UserPrint("offsetof(QC98XX_EEPROM,targetPower2G) is %d (%x)\n", offsetof(QC98XX_EEPROM,targetPower2G),offsetof(QC98XX_EEPROM,targetPower2G)); //  12B
    UserPrint("offsetof(QC98XX_EEPROM,targetPower2GVHT20) is %d (%x)\n", offsetof(QC98XX_EEPROM,targetPower2GVHT20),offsetof(QC98XX_EEPROM,targetPower2GVHT20)); //  42B
    UserPrint("offsetof(QC98XX_EEPROM,targetPower2GVHT40) is %d (%x)\n", offsetof(QC98XX_EEPROM,targetPower2GVHT40),offsetof(QC98XX_EEPROM,targetPower2GVHT40)); //  42B

    UserPrint("offsetof(QC98XX_EEPROM,ctlIndex2G) is %d (%x)\n", offsetof(QC98XX_EEPROM,ctlIndex2G),offsetof(QC98XX_EEPROM,ctlIndex2G)); //  12B
    UserPrint("offsetof(QC98XX_EEPROM,ctlFreqbin2G) is %d (%x)\n", offsetof(QC98XX_EEPROM,ctlFreqbin2G),offsetof(QC98XX_EEPROM,ctlFreqbin2G)); //  48B
    UserPrint("offsetof(QC98XX_EEPROM,ctlData2G) is %d (%x)\n", offsetof(QC98XX_EEPROM,ctlData2G),offsetof(QC98XX_EEPROM,ctlData2G)); //  48B  
    //UserPrint("offsetof(QC98XX_EEPROM,ctlData2GVHT20) is %d (%x)\n", offsetof(QC98XX_EEPROM,ctlData2GVHT20),offsetof(QC98XX_EEPROM,ctlData2GVHT20)); //  48B  
    //UserPrint("offsetof(QC98XX_EEPROM,ctlData2GVHT40) is %d (%x)\n", offsetof(QC98XX_EEPROM,ctlData2GVHT40),offsetof(QC98XX_EEPROM,ctlData2GVHT40)); //  48B  

    UserPrint("offsetof(QC98XX_EEPROM,tempComp2G) is %d (%x)\n", offsetof(QC98XX_EEPROM,tempComp2G),offsetof(QC98XX_EEPROM,tempComp2G)); //  40B   

    UserPrint("offsetof(QC98XX_EEPROM,calFreqPier5G) is %d (%x)\n", offsetof(QC98XX_EEPROM,calFreqPier5G),offsetof(QC98XX_EEPROM,calFreqPier5G)); //  64B
    UserPrint("offsetof(QC98XX_EEPROM,calPierData5G) is %d (%x)\n", offsetof(QC98XX_EEPROM,calPierData5G),offsetof(QC98XX_EEPROM,calPierData5G)); //1088B + 64B=1152B  

    UserPrint("offsetof(QC98XX_EEPROM,targetFreqbin5G) is %d (%x)\n", offsetof(QC98XX_EEPROM,targetFreqbin5G),offsetof(QC98XX_EEPROM,targetFreqbin5G)); //   8B
    UserPrint("offsetof(QC98XX_EEPROM,targetFreqbin5GVHT20) is %d (%x)\n", offsetof(QC98XX_EEPROM,targetFreqbin5GVHT20),offsetof(QC98XX_EEPROM,targetFreqbin5GVHT20)); //   8B
    UserPrint("offsetof(QC98XX_EEPROM,targetFreqbin5GVHT40) is %d (%x)\n", offsetof(QC98XX_EEPROM,targetFreqbin5GVHT40),offsetof(QC98XX_EEPROM,targetFreqbin5GVHT40)); //   8B
    UserPrint("offsetof(QC98XX_EEPROM,targetFreqbin5GVHT80) is %d (%x)\n", offsetof(QC98XX_EEPROM,targetFreqbin5GVHT80),offsetof(QC98XX_EEPROM,targetFreqbin5GVHT80)); //   8B
    UserPrint("offsetof(QC98XX_EEPROM,targetPower5G) is %d (%x)\n", offsetof(QC98XX_EEPROM,targetPower5G),offsetof(QC98XX_EEPROM,targetPower5G)); //  32B
    UserPrint("offsetof(QC98XX_EEPROM,targetPower5GVHT20) is %d (%x)\n", offsetof(QC98XX_EEPROM,targetPower5GVHT20),offsetof(QC98XX_EEPROM,targetPower5GVHT20)); // 112B
    UserPrint("offsetof(QC98XX_EEPROM,targetPower5GVHT40) is %d (%x)\n", offsetof(QC98XX_EEPROM,targetPower5GVHT40),offsetof(QC98XX_EEPROM,targetPower5GVHT40)); // 112B
    UserPrint("offsetof(QC98XX_EEPROM,targetPower5GVHT80) is %d (%x)\n", offsetof(QC98XX_EEPROM,targetPower5GVHT80),offsetof(QC98XX_EEPROM,targetPower5GVHT80)); // 112B

    UserPrint("offsetof(QC98XX_EEPROM,ctlIndex5G) is %d (%x)\n", offsetof(QC98XX_EEPROM,ctlIndex5G),offsetof(QC98XX_EEPROM,ctlIndex5G)); //   9B
    //UserPrint("offsetof(QC98XX_EEPROM,ctl5GFuture) is %d (%x)\n", offsetof(QC98XX_EEPROM,ctl5GFuture),offsetof(QC98XX_EEPROM,ctl5GFuture)); //   3B  
    UserPrint("offsetof(QC98XX_EEPROM,ctlFreqbin5G) is %d (%x)\n", offsetof(QC98XX_EEPROM,ctlFreqbin5G),offsetof(QC98XX_EEPROM,ctlFreqbin5G)); //  72B
    UserPrint("offsetof(QC98XX_EEPROM,ctlData5G) is %d (%x)\n", offsetof(QC98XX_EEPROM,ctlData5G),offsetof(QC98XX_EEPROM,ctlData5G)); //  72B
    //UserPrint("offsetof(QC98XX_EEPROM,ctlData5GVHT20) is %d (%x)\n", offsetof(QC98XX_EEPROM,ctlData5GVHT20),offsetof(QC98XX_EEPROM,ctlData5GVHT20)); //  72B
    //UserPrint("offsetof(QC98XX_EEPROM,ctlData5GVHT40) is %d (%x)\n", offsetof(QC98XX_EEPROM,ctlData5GVHT40),offsetof(QC98XX_EEPROM,ctlData5GVHT40)); //  72B
    //UserPrint("offsetof(QC98XX_EEPROM,ctlData5GVHT80) is %d (%x)\n", offsetof(QC98XX_EEPROM,ctlData5GVHT80),offsetof(QC98XX_EEPROM,ctlData5GVHT80)); //  72B

    UserPrint("offsetof(QC98XX_EEPROM,tempComp5G) is %d (%x)\n", offsetof(QC98XX_EEPROM,tempComp5G),offsetof(QC98XX_EEPROM,tempComp5G)); //  40B  

    UserPrint("offsetof(QC98XX_EEPROM,configAddr) is %d (%x)\n", offsetof(QC98XX_EEPROM,configAddr),offsetof(QC98XX_EEPROM,configAddr)); //  64B                       
}

void PrintQc98xxBaseHeader(int client, const BASE_EEP_HEADER *pBaseEepHeader, const QC98XX_EEPROM *pEeprom)
{
    char  buffer[MBUFFER];

	SformatOutput(buffer, MBUFFER-1, " | RegDomain 0              0x%04X   |  RegDomain 1             0x%04X   |",
        pBaseEepHeader->regDmn[0],
        pBaseEepHeader->regDmn[1]
        );
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1, " | TX Mask                  0x%X      |  RX Mask                 0x%X      |",
        (pBaseEepHeader->txrxMask&0xF0)>>4,
        pBaseEepHeader->txrxMask&0x0F
        );
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," | OpFlags: 5GHz            %d        |  2GHz                    %d        |",
             (pBaseEepHeader->opCapBrdFlags.opFlags & WHAL_OPFLAGS_11A) || 0,
             (pBaseEepHeader->opCapBrdFlags.opFlags & WHAL_OPFLAGS_11G) || 0
             );
    ErrorPrint(NartOther, buffer);
    SformatOutput(buffer, MBUFFER-1," | 5G HT20                  %d        |  2G HT20                 %d        |",
              (pBaseEepHeader->opCapBrdFlags.opFlags & WHAL_OPFLAGS_5G_HT20) || 0,
              (pBaseEepHeader->opCapBrdFlags.opFlags & WHAL_OPFLAGS_2G_HT20) || 0
                );
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," | 5G HT40                  %d        |  2G HT40                 %d        |",
              (pBaseEepHeader->opCapBrdFlags.opFlags & WHAL_OPFLAGS_5G_HT40) || 0,
              (pBaseEepHeader->opCapBrdFlags.opFlags & WHAL_OPFLAGS_2G_HT40) || 0
                );
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," | 5G VHT20                 %d        |  2G VHT20                %d        |",
              (pBaseEepHeader->opCapBrdFlags.opFlags2 & WHAL_OPFLAGS2_5G_VHT20) || 0,
              (pBaseEepHeader->opCapBrdFlags.opFlags2 & WHAL_OPFLAGS2_2G_VHT20) || 0
                );
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," | 5G VHT40                 %d        |  2G VHT40                %d        |",
              (pBaseEepHeader->opCapBrdFlags.opFlags2 & WHAL_OPFLAGS2_5G_VHT40) || 0,
              (pBaseEepHeader->opCapBrdFlags.opFlags2 & WHAL_OPFLAGS2_2G_VHT40) || 0
                );
    ErrorPrint(NartOther, buffer);
    SformatOutput(buffer, MBUFFER-1," | 5G VHT80                 %d        |                                   |",
              (pBaseEepHeader->opCapBrdFlags.opFlags2 & WHAL_OPFLAGS2_5G_VHT80) || 0
                );
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," | Big Endian               %d        |  Wake On Wireless        %d        |",
             (pBaseEepHeader->opCapBrdFlags.miscConfiguration & WHAL_MISCCONFIG_EEPMISC_BIG_ENDIAN) || 0, (pBaseEepHeader->opCapBrdFlags.miscConfiguration & WHAL_MISCCONFIG_EEPMISC_WOW) || 0);
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1, " | RF Silent                0x%X      |  Bluetooth               0x%X      |",
    	pBaseEepHeader->rfSilent,
    	pBaseEepHeader->opCapBrdFlags.blueToothOptions
    	);
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1, " | GPIO wlan Disable        NA       |  GPIO wlan LED           0x%02x     |",
        pBaseEepHeader->wlanLedGpio);
    ErrorPrint(NartOther, buffer);
    if (pBaseEepHeader->eeprom_version == QC98XX_EEP_VER1)
    {
	    SformatOutput(buffer, MBUFFER-1, " | txrxGain                 0x%02x     |  pwrTableOffset          %d        |",
    	    pBaseEepHeader->txrxgain, pBaseEepHeader->pwrTableOffset);
    }
    else //v2
    {
	    SformatOutput(buffer, MBUFFER-1, " | txrxGain        see Modal Section |  pwrTableOffset          %d        |",
    	    pBaseEepHeader->txrxgain, pBaseEepHeader->pwrTableOffset);
    }
    ErrorPrint(NartOther, buffer);
    SformatOutput(buffer, MBUFFER-1, " | spurBaseA                %d        |  spurBaseB               %d        |",
        pBaseEepHeader->spurBaseA, pBaseEepHeader->spurBaseB);
    ErrorPrint(NartOther, buffer);
    SformatOutput(buffer, MBUFFER-1, " | spurRssiThresh           %d        |  spurRssiThreshCck       %d        |",
        pBaseEepHeader->spurRssiThresh,pBaseEepHeader->spurRssiThreshCck);
    ErrorPrint(NartOther,buffer);    
    SformatOutput(buffer, MBUFFER-1, " | spurMitFlag            0x%08x |  internal regulator      0x%02x     |",
         pBaseEepHeader->spurMitFlag,pBaseEepHeader->swreg);
    ErrorPrint(NartOther,buffer);    

    SformatOutput(buffer, MBUFFER-1, " | boardFlags             0x%08x |  featureEnable         0x%08x |",
		pBaseEepHeader->opCapBrdFlags.boardFlags, pBaseEepHeader->opCapBrdFlags.featureEnable);
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1, " | flag1                  0x%08x |                                   |",
		pBaseEepHeader->opCapBrdFlags.flag1);
    ErrorPrint(NartOther, buffer);
}

void PrintQc98xx_2GHzHeader(int client, const BIMODAL_EEP_HEADER *pBiModalHeader, const QC98XX_EEPROM *pEeprom)
{
    char  buffer[MBUFFER];
	int i, j;
	const FREQ_MODAL_EEP_HEADER *pFreqHeader = &pEeprom->freqModalHeader;

	SformatOutput(buffer, MBUFFER-1," |                                                                       |");
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1," |===========================2GHz Modal Header===========================|");
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," |                                                                       |");
    ErrorPrint(NartOther,buffer);

    SformatOutput(buffer, MBUFFER-1," | Ant Chain0  0x%04X    |  Ant Chain1  0x%04X   |  Ant Chain2  0x%04X   |",
        pBiModalHeader->antCtrlChain[0],
        pBiModalHeader->antCtrlChain[1],
        pBiModalHeader->antCtrlChain[2]
        );
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," | xatten1DB Ch0  0x%02x   | xatten1DB Ch1  0x%02x   | xatten1DB Ch2  0x%02x   |",
        pFreqHeader->xatten1DB[0].value2G,
        pFreqHeader->xatten1DB[1].value2G,
        pFreqHeader->xatten1DB[2].value2G
        );
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," | xatten1Margin Ch0 0x%02x| xatten1Margin Ch1 0x%02x| xatten1Margin Ch2 0x%02x|",
        pFreqHeader->xatten1Margin[0].value2G,
        pFreqHeader->xatten1Margin[1].value2G,
        pFreqHeader->xatten1Margin[2].value2G
        );
    ErrorPrint(NartOther, buffer);

	SformatOutput(buffer, MBUFFER-1," | Volt Slope Ch0  %3d   | Volt Slope Ch1  %3d   | Volt Slope Ch2  %3d   |",
        pBiModalHeader->voltSlope[0],
        pBiModalHeader->voltSlope[1],
        pBiModalHeader->voltSlope[2]
        );
    ErrorPrint(NartOther, buffer);

	if (pEeprom->baseEepHeader.eeprom_version >= QC98XX_EEP_VER3)
	{
		SformatOutput(buffer, MBUFFER-1, " | minCcaPwr  Ch0  %3d   | minCcaPwr  Ch1  %3d   | minCcaPwr  Ch2 %3d    |",
			pBiModalHeader->minCcaPwr[0], pBiModalHeader->minCcaPwr[1], pBiModalHeader->minCcaPwr[2]);
		ErrorPrint (NartOther, buffer);
	}

    SformatOutput(buffer, MBUFFER-1," | Antenna Common        0x%08X  |  Antenna Common2       0x%08X |",
        pBiModalHeader->antCtrlCommon,
        pBiModalHeader->antCtrlCommon2
        );
    ErrorPrint(NartOther, buffer);

	for (i=0; i<WHAL_NUM_CHAINS; i++)
    {
        SformatOutput(buffer, MBUFFER-1, " | Alpha Thermal Slope CH%d                                               |", i);
        ErrorPrint(NartOther, buffer);  
        for (j=0; j<QC98XX_NUM_ALPHATHERM_CHANS_2G; j=j+2)
        {
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
            ErrorPrint(NartOther, buffer);   
        }
    }
        
    for (i=0; i<QC98XX_EEPROM_MODAL_SPURS; i++)
    {
        SformatOutput(buffer, MBUFFER-1," | spurChan[%d]             0x%02x      |                                   |",
            i, pBiModalHeader->spurChans[i].spurChan
            );
        ErrorPrint(NartOther, buffer);
        SformatOutput(buffer, MBUFFER-1," | spurA_PrimSecChoose[%d]  0x%02x      |  spurB_PrimSecChoose[%d] 0x%02x      |",
            i, pBiModalHeader->spurChans[i].spurA_PrimSecChoose, i, pBiModalHeader->spurChans[i].spurB_PrimSecChoose
            );
        ErrorPrint(NartOther, buffer);
    }
        
    //SformatOutput(buffer, MBUFFER-1," | NoiseFloor Thres0      %3d        |  NoiseFloorThres1      %3d        |",
    //    pBiModalHeader->noiseFloorThreshCh[0],
    //    pBiModalHeader->noiseFloorThreshCh[1]
    //    );
    //ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," | xpaBiasLvl              0x%02x      |  xpaBiasBypass        %3d         |",
        (pBiModalHeader->xpaBiasLvl&0xF), ((pBiModalHeader->xpaBiasLvl>>4)&0x1)
        );
    ErrorPrint(NartOther, buffer);
    SformatOutput(buffer, MBUFFER-1," | rxFilterCap             0x%02x      |  rxGainCap              0x%02x      |",
        pBiModalHeader->rxFilterCap, 
		pBiModalHeader->rxGainCap
        );
    ErrorPrint(NartOther, buffer);
    if (pEeprom->baseEepHeader.eeprom_version != QC98XX_EEP_VER1)
    {
        SformatOutput(buffer, MBUFFER-1," | txGain                  0x%02x      |  rxGain                 0x%02x      |",
            (pBiModalHeader->txrxgain >> 4) & 0xf, 
		    pBiModalHeader->rxGainCap & 0xf
            );
        ErrorPrint(NartOther, buffer);
    }

    if (pEeprom->baseEepHeader.eeprom_version != QC98XX_EEP_VER1)
    {
        SformatOutput(buffer, MBUFFER-1," | noiseFlrThr              %3d      |  antennaGainCh        %3d         |",
            pBiModalHeader->noiseFlrThr, pBiModalHeader->antennaGainCh);
        ErrorPrint(NartOther, buffer);
    }

    SformatOutput(buffer, MBUFFER-1," |                                   |                                   |");
    ErrorPrint(NartOther,buffer);
}

/*void PrintQc98xx_2GHzCalPiers(int client, A_UINT8 *pPiers)
{
    int i=0;
    char  buffer[MBUFFER];
    SformatOutput(buffer, MBUFFER-1,"2G Cal Freqncy Piers:");
    ErrorPrint(NartOther,buffer);
    for(i=0;i<WHAL_NUM_11G_CAL_PIERS;i++){
        SformatOutput(buffer, MBUFFER-1,"[%d]: %d",i,WHAL_FBIN2FREQ(pPiers[i], 1));
        ErrorPrint(NartOther,buffer);
    }
}*/

void PrintQc98xx_2GHzRxCalData(int client, const QC98XX_EEPROM *pEeprom)
{
	A_UINT16 numPiers = WHAL_NUM_11G_CAL_PIERS;
	A_UINT16 pc; //pier count
	char  buffer[MBUFFER];
	const A_UINT8 *pPiers = &pEeprom->calFreqPier2G[0];
	const NF_CAL_DATA_PER_CHIP *pData = &pEeprom->nfCalData2G[0];
	int i;

	SformatOutput(buffer, MBUFFER-1, " |===================2G Rx Calibration Information ======================|");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	ErrorPrint(NartOther,buffer);
	for (i = 0; i < WHAL_NUM_CHAINS; ++i)
	{
		SformatOutput ( buffer, MBUFFER-1, " |                          Chain %d                                      |", i );
		ErrorPrint ( NartOther,buffer);
		SformatOutput ( buffer, MBUFFER-1, " |-----------------------------------------------------------------------|" );
		ErrorPrint ( NartOther,buffer );
		SformatOutput ( buffer, MBUFFER-1, " |   Freq   |    nf(dBr)    | nfpower(dBm) |    temp    |   temp_slope   |" );
		ErrorPrint ( NartOther,buffer );
		SformatOutput ( buffer, MBUFFER-1, " |-----------------------------------------------------------------------|" );
		ErrorPrint ( NartOther,buffer );
		for ( pc = 0; pc < numPiers; pc++ ) {
			SformatOutput ( buffer, MBUFFER-1, " |   %04d   |     %4d      |     %4d     |    %4d    |      %4d      |",
				WHAL_FBIN2FREQ(pPiers[pc], 1 ),
				pData [ pc ].NF_Power_dBr[i], pData [ pc ].NF_Power_dBm[i], pData [ pc ].NF_thermCalVal, pData [ pc ].NF_thermCalSlope
				);
			ErrorPrint ( NartOther,buffer );
		}
		SformatOutput ( buffer, MBUFFER-1, " |-----------------------------------------------------------------------|" );
		ErrorPrint(NartOther,buffer);
	}
	SformatOutput(buffer, MBUFFER-1, " |                                                                       |");

	ErrorPrint ( NartOther, buffer );
}

void PrintQc98xx_2GHzCalData(int client, const QC98XX_EEPROM *pEeprom)
{
    A_UINT16 numPiers = WHAL_NUM_11G_CAL_PIERS;
    A_UINT16 pc; //pier count
    char  buffer[MBUFFER];
    const A_UINT8 *pPiers = &pEeprom->calFreqPier2G[0];
    const CAL_DATA_PER_FREQ_OLPC *pData = &pEeprom->calPierData2G[0];
	int i;

    SformatOutput(buffer, MBUFFER-1, " |=================2G Power Calibration Information =====================|");
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
    ErrorPrint(NartOther,buffer);
	for (i = 0; i < WHAL_NUM_CHAINS; ++i)
	{
		SformatOutput(buffer, MBUFFER-1, " |                          Chain %d                                      |", i);
		ErrorPrint(NartOther,buffer);
		SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
		ErrorPrint(NartOther,buffer);
		SformatOutput(buffer, MBUFFER-1, " | Freq  txgainIdx0 dacGain0  Pwr0  txgainIdx1 dacGain1 Pwr1  Volt  Temp |");
		ErrorPrint(NartOther,buffer);
		SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
		ErrorPrint(NartOther,buffer);
		for(pc = 0; pc < numPiers; pc++) {
			SformatOutput(buffer, MBUFFER-1, " | %04d    %4d      %3d     %3d      %4d      %3d     %3d   %3d   %3d  |",
                WHAL_FBIN2FREQ(pPiers[pc], 1),
				pData[pc].calPerPoint[i].txgainIdx[0], pData[pc].dacGain[0], pData[pc].calPerPoint[i].power_t8[0]/8,
				pData[pc].calPerPoint[i].txgainIdx[1], pData[pc].dacGain[1], pData[pc].calPerPoint[i].power_t8[1]/8,
				pData[pc].voltCalVal, pData[pc].thermCalVal
                );
            ErrorPrint(NartOther,buffer);
		}
		SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
		ErrorPrint(NartOther,buffer);
    }
    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
    ErrorPrint(NartOther,buffer);
}

void PrintQc98xx_2GLegacyTargetPower(int client, const CAL_TARGET_POWER_LEG *pVals, const A_UINT8 *pFreq)
{
    int i,j;
    char buffer[MBUFFER],buffer2[MBUFFER];

    SformatOutput(buffer, MBUFFER-1, " |===========================2G Target Powers============================|");
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1," | OFDM   ");

    for (j = 0; j < WHAL_NUM_11G_LEG_TARGET_POWER_CHANS; j++) 
    {
        SformatOutput(buffer2, MBUFFER-1,"|        %04d        ", WHAL_FBIN2FREQ(*(pFreq+j),1));
        strcat(buffer,buffer2);
    }

    strcat(buffer,"|");
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," |========|====================|====================|====================|");
    ErrorPrint(NartOther, buffer);

    for (j = 0; j < WHAL_NUM_LEGACY_TARGET_POWER_RATES; j++)
    {
        SformatOutput(buffer, MBUFFER-1," | %s   ",sRatePrintLegacy[j]);
        for(i=0;i<WHAL_NUM_11G_LEG_TARGET_POWER_CHANS;i++)
        {
            SformatOutput(buffer2, MBUFFER-1,"|        %2d.%d        ", pVals[i].tPow2x[j]/2, (pVals[i].tPow2x[j] % 2) * 5);
            strcat(buffer,buffer2);
        }

        strcat(buffer,"|");
        ErrorPrint(NartOther, buffer);
    }
    SformatOutput(buffer, MBUFFER-1," |========================================================================");
    ErrorPrint(NartOther, buffer);
}

void PrintQc98xx_2GCCKTargetPower(int client, const CAL_TARGET_POWER_11B *pVals, const A_UINT8 *pFreq)
{
    int i,j;
    char buffer[MBUFFER],buffer2[MBUFFER];

    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1," | CCK    ");

    for (j = 0; j < WHAL_NUM_11B_TARGET_POWER_CHANS; j++)
    {
        SformatOutput(buffer2, MBUFFER-1,"|        %04d        ", WHAL_FBIN2FREQ(*(pFreq+j),1));
        strcat(buffer,buffer2);
    }

    strcat(buffer,"|====================|");
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," |========|====================|====================|====================|");
    ErrorPrint(NartOther, buffer);

    for (j = 0; j < WHAL_NUM_11B_TARGET_POWER_RATES; j++) 
    {
        SformatOutput(buffer, MBUFFER-1," | %s  ",sRatePrintCck[j]);
        for(i=0;i<WHAL_NUM_11B_TARGET_POWER_CHANS;i++)
        {
            SformatOutput(buffer2, MBUFFER-1,"|        %2d.%d        ", pVals[i].tPow2x[j]/2, (pVals[i].tPow2x[j] % 2) * 5);
            strcat(buffer,buffer2);
        }

        strcat(buffer,"|====================|");
        ErrorPrint(NartOther, buffer);
    }
    SformatOutput(buffer, MBUFFER-1," |========================================================================");
    ErrorPrint(NartOther, buffer);
}

void PrintQc98xx_2GHT20TargetPower(int client, const CAL_TARGET_POWER_11G_20 *pVals, const A_UINT8 *pFreq) 
{
    int i,j;
    char buffer[MBUFFER],buffer2[MBUFFER];
	double power;
	int rateIndex;

    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1," | VHT20     ");

    for (j = 0; j < WHAL_NUM_11G_20_TARGET_POWER_CHANS; j++)
    {
        SformatOutput(buffer2, MBUFFER-1,"|        %04d       ", WHAL_FBIN2FREQ(*(pFreq+j),1));
        strcat(buffer,buffer2);
    }

    strcat(buffer,"|");
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," |===========|===================|===================|===================|");
    ErrorPrint(NartOther, buffer);

    for (j = 0; j < RATE_PRINT_VHT_SIZE; j++)
    {
        SformatOutput(buffer, MBUFFER-1," | %s  ",sRatePrintVHT[j]);
        for(i=0;i<WHAL_NUM_11G_20_TARGET_POWER_CHANS;i++)
        {
			rateIndex = vRATE_INDEX_HT20_MCS0 + j; 
			Qc98xxTargetPowerGet(WHAL_FBIN2FREQ(*(pFreq+i),1), rateIndex, &power);
            SformatOutput(buffer2, MBUFFER-1,"|        %2.1f       ", power);
            strcat(buffer,buffer2);
        }

        strcat(buffer,"|");
        ErrorPrint(NartOther, buffer);
    }
    SformatOutput(buffer, MBUFFER-1," |========================================================================");
    ErrorPrint(NartOther, buffer);
}

void PrintQc98xx_2GHT40TargetPower(int client, const CAL_TARGET_POWER_11G_40 *pVals, const A_UINT8 *pFreq) 
{
    int i,j;
    char buffer[MBUFFER],buffer2[MBUFFER];
	double power;
	int rateIndex;

    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1," | VHT40     ");

    for (j = 0; j < WHAL_NUM_11G_40_TARGET_POWER_CHANS; j++)
    {
        SformatOutput(buffer2, MBUFFER-1,"|        %04d       ", WHAL_FBIN2FREQ(*(pFreq+j),1));
        strcat(buffer,buffer2);
    }

    strcat(buffer,"|");
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," |===========|===================|===================|===================|");
    ErrorPrint(NartOther, buffer);

    for (j = 0; j < RATE_PRINT_VHT_SIZE; j++)
    {
        SformatOutput(buffer, MBUFFER-1," | %s  ",sRatePrintVHT[j]);
        for(i=0;i<WHAL_NUM_11G_40_TARGET_POWER_CHANS;i++)
        {
			rateIndex = vRATE_INDEX_HT40_MCS0 + j; 
			Qc98xxTargetPowerGet(WHAL_FBIN2FREQ(*(pFreq+i),1), rateIndex, &power);
            SformatOutput(buffer2, MBUFFER-1,"|        %2.1f       ", power);
            strcat(buffer,buffer2);
        }

        strcat(buffer,"|");
        ErrorPrint(NartOther, buffer);
    }
    SformatOutput(buffer, MBUFFER-1," |========================================================================");
    ErrorPrint(NartOther, buffer);
}

void PrintQc98xx_2GCTLIndex(int client, const A_UINT8 *pCtlIndex){
    int i;
    char buffer[MBUFFER];
    SformatOutput(buffer, MBUFFER-1,"2G CTL Index:");
    ErrorPrint(NartOther,buffer);
    for(i=0;i<WHAL_NUM_CTLS_2G;i++){
		if (pCtlIndex[i] == 0)
		{
			continue;
		}
        SformatOutput(buffer, MBUFFER-1,"[%d] :0x%x",i,pCtlIndex[i]);
        ErrorPrint(NartOther,buffer);
    }
}

void PrintQc98xx_2GCTLData(int client,  const A_UINT8 *ctlIndex, const CAL_CTL_DATA_2G Data[WHAL_NUM_CTLS_2G], const A_UINT8 *pFreq)
{

    int i,j;
    char buffer[MBUFFER],buffer2[MBUFFER];

    SformatOutput(buffer, MBUFFER-1," |                                                                       |");
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," |=======================Test Group Band Edge Power======================|");
    ErrorPrint(NartOther, buffer);
    SformatOutput(buffer, MBUFFER-1," |                                                                       |");
    ErrorPrint(NartOther, buffer);
    for (i = 0; i<WHAL_NUM_CTLS_2G; i++) 
	{
		if (ctlIndex[i] == 0)
		{
		    pFreq+=WHAL_NUM_BAND_EDGES_2G;
			continue;
		}
        SformatOutput(buffer, MBUFFER-1," |                                                                       |");
        ErrorPrint(NartOther, buffer);
        SformatOutput(buffer, MBUFFER-1," | CTL: 0x%02x %s                                           |",
                 ctlIndex[i], sCtlType[ctlIndex[i] & QC98XX_CTL_MODE_M]);
        ErrorPrint(NartOther, buffer);
        SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
        ErrorPrint(NartOther, buffer);
        //WHAL_FBIN2FREQ(*(pFreq++),0),Data[i].ctlEdges[j].tPower,Data[i].ctlEdges[j].flag

            SformatOutput(buffer, MBUFFER-1," | edge  ");

            for (j = 0; j < WHAL_NUM_BAND_EDGES_2G; j++) {
                if (*(pFreq+j) == QC98XX_BCHAN_UNUSED) {
                    SformatOutput(buffer2, MBUFFER-1,"|  --   ");
                } else {
                    SformatOutput(buffer2, MBUFFER-1,"| %04d  ", WHAL_FBIN2FREQ(*(pFreq+j),1));
                }
                strcat(buffer,buffer2);
            }

            strcat(buffer,"|");
            ErrorPrint(NartOther, buffer);

            SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
            ErrorPrint(NartOther, buffer);

            SformatOutput(buffer, MBUFFER-1," | power ");

            for (j = 0; j < WHAL_NUM_BAND_EDGES_2G; j++) {
				if (*(pFreq+j) == QC98XX_BCHAN_UNUSED) {
                    SformatOutput(buffer2, MBUFFER-1,"|  --   ");
                } else {
                    SformatOutput(buffer2, MBUFFER-1,"| %2d.%d  ", Data[i].ctl_edges[j].u.tPower / 2,
                        (Data[i].ctl_edges[j].u.tPower % 2) * 5);
                }
                strcat(buffer,buffer2);
            }

            strcat(buffer,"|");
            ErrorPrint(NartOther, buffer);

            SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
            ErrorPrint(NartOther, buffer);

            SformatOutput(buffer, MBUFFER-1," | flag  ");

            for (j = 0; j < WHAL_NUM_BAND_EDGES_2G; j++) {
                if (*(pFreq+j) == QC98XX_BCHAN_UNUSED) {
                    SformatOutput(buffer2, MBUFFER-1,"|  --   ");

                } else {
                    SformatOutput(buffer2, MBUFFER-1,"|   %1d   ", Data[i].ctl_edges[j].u.flag);
                }
                strcat(buffer,buffer2);
            }

            strcat(buffer,"|");
            ErrorPrint(NartOther, buffer);

            pFreq+=WHAL_NUM_BAND_EDGES_2G;

            SformatOutput(buffer, MBUFFER-1," =========================================================================");
            ErrorPrint(NartOther, buffer);
    }
}

void PrintQc98xx_5GHzHeader(int client, const BIMODAL_EEP_HEADER * pBiModalHeader, const QC98XX_EEPROM *pEeprom)
{
    char  buffer[MBUFFER];
    int i, j;
	const FREQ_MODAL_EEP_HEADER *pFreqHeader = &pEeprom->freqModalHeader;

    SformatOutput(buffer, MBUFFER-1," |                                                                       |");
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1," |===========================5GHz Modal Header===========================|");
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," |                                                                       |");
    ErrorPrint(NartOther,buffer);

    SformatOutput(buffer, MBUFFER-1," | Ant Chain0  0x%04X   |  Ant Chain1  0x%04X   |  Ant Chain2  0x%04X    |",
        pBiModalHeader->antCtrlChain[0],
        pBiModalHeader->antCtrlChain[1],
        pBiModalHeader->antCtrlChain[2]
        );
    ErrorPrint(NartOther, buffer);

	SformatOutput(buffer, MBUFFER-1," | Volt Slope Ch0  %3d  | Volt Slope Ch1  %3d   | Volt Slope Ch2  %3d    |",
        pBiModalHeader->voltSlope[0],
        pBiModalHeader->voltSlope[1],
        pBiModalHeader->voltSlope[2]
        );
    ErrorPrint(NartOther, buffer);

	if (pEeprom->baseEepHeader.eeprom_version >= QC98XX_EEP_VER3)
	{
		SformatOutput(buffer, MBUFFER-1, " | minCcaPwr  Ch0  %3d  | minCcaPwr  Ch1  %3d   | minCcaPwr  Ch2 %3d     |",
			pBiModalHeader->minCcaPwr[0], pBiModalHeader->minCcaPwr[1], pBiModalHeader->minCcaPwr[2]);
		ErrorPrint (NartOther, buffer);
	}

    SformatOutput(buffer, MBUFFER-1," | Antenna Common        0x%08X  |  Antenna Common2       0x%08X |",
        pBiModalHeader->antCtrlCommon,
        pBiModalHeader->antCtrlCommon2
        );
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," | xatten1DB Ch0  (0x%02x,0x%02x,0x%02x)   | xatten1DB Ch1  (0x%02x,0x%02x,0x%02x)   |",
        pFreqHeader->xatten1DB[0].value5GLow, pFreqHeader->xatten1DB[0].value5GMid, pFreqHeader->xatten1DB[0].value5GHigh,
        pFreqHeader->xatten1DB[1].value5GLow, pFreqHeader->xatten1DB[1].value5GMid, pFreqHeader->xatten1DB[1].value5GHigh
        );
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," | xatten1DB Ch2  (0x%02x,0x%02x,0x%02x)   | xatten1Margin Ch0 (0x%02x,0x%02x,0x%02x)|",
        pFreqHeader->xatten1DB[2].value5GLow, pFreqHeader->xatten1DB[2].value5GMid, pFreqHeader->xatten1DB[2].value5GHigh,
        pFreqHeader->xatten1Margin[0].value5GLow, pFreqHeader->xatten1Margin[0].value5GMid, pFreqHeader->xatten1Margin[0].value5GHigh
        );
    ErrorPrint(NartOther, buffer);

	SformatOutput(buffer, MBUFFER-1," | xatten1Margin Ch1 (0x%02x,0x%02x,0x%02x)| xatten1Margin Ch2 (0x%02x,0x%02x,0x%02x)|",
        pFreqHeader->xatten1Margin[1].value5GLow, pFreqHeader->xatten1Margin[1].value5GMid, pFreqHeader->xatten1Margin[1].value5GHigh,
        pFreqHeader->xatten1Margin[2].value5GLow, pFreqHeader->xatten1Margin[2].value5GMid, pFreqHeader->xatten1Margin[2].value5GHigh
        );
    ErrorPrint(NartOther, buffer);

	for (i=0; i<WHAL_NUM_CHAINS; i++)
    {
        SformatOutput(buffer, MBUFFER-1, " | Alpha Thermal Slope CH%d                                               |", i);
        ErrorPrint(NartOther, buffer);  
        for (j=0; j<QC98XX_NUM_ALPHATHERM_CHANS_5G; j=j+2)
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
            ErrorPrint(NartOther, buffer);   
        }
    }
        
    for (i=0; i<QC98XX_EEPROM_MODAL_SPURS; i++)
    {
        SformatOutput(buffer, MBUFFER-1," | spurChan[%d]             0x%02x      |                                   |",
            i, pBiModalHeader->spurChans[i].spurChan
            );
        ErrorPrint(NartOther, buffer);
        SformatOutput(buffer, MBUFFER-1," | spurA_PrimSecChoose[%d]  0x%02x      |  spurA_PrimSecChoose[%d] 0x%02x      |",
            i, pBiModalHeader->spurChans[i].spurA_PrimSecChoose, i, pBiModalHeader->spurChans[i].spurB_PrimSecChoose
            );
        ErrorPrint(NartOther, buffer);
    }
        
    //SformatOutput(buffer, MBUFFER-1," | NoiseFloor Thres0      %3d        |  NoiseFloorThres1      %3d        |",
    //    pBiModalHeader->noiseFloorThreshCh[0],
    //    pBiModalHeader->noiseFloorThreshCh[1]
    //    );
    //ErrorPrint(NartOther, buffer);

   SformatOutput(buffer, MBUFFER-1," | xpaBiasLvl              0x%02x      |  xpaBiasBypass        %3d         |",
        (pBiModalHeader->xpaBiasLvl&0xF), ((pBiModalHeader->xpaBiasLvl>>4)&0x1)
        );
    ErrorPrint(NartOther, buffer);
    SformatOutput(buffer, MBUFFER-1," | rxFilterCap             0x%02x      |  rxGainCap              0x%02x      |",
        pBiModalHeader->rxFilterCap, 
		pBiModalHeader->rxGainCap
        );
    ErrorPrint(NartOther, buffer);
    if (pEeprom->baseEepHeader.eeprom_version != QC98XX_EEP_VER1)
    {
        SformatOutput(buffer, MBUFFER-1," | txGain                  0x%02x      |  rxGain                 0x%02x      |",
            (pBiModalHeader->txrxgain >> 4) & 0xf, 
		    pBiModalHeader->rxGainCap & 0xf
            );
        ErrorPrint(NartOther, buffer);
    }

    if (pEeprom->baseEepHeader.eeprom_version != QC98XX_EEP_VER1)
    {
        SformatOutput(buffer, MBUFFER-1," | noiseFlrThr              %3d      |  antennaGainCh        %3d         |",
            pBiModalHeader->noiseFlrThr, pBiModalHeader->antennaGainCh);
        ErrorPrint(NartOther, buffer);
    }    

    SformatOutput(buffer, MBUFFER-1," |                                   |                                   |");
    ErrorPrint(NartOther,buffer);
}

/*void PrintQc98xx_5GHzCalPiers(int client, A_UINT8 *pPiers)
{
    int i=0;
    char  buffer[MBUFFER];
    SformatOutput(buffer, MBUFFER-1,"5G Cal Freqncy Piers:");
    ErrorPrint(NartOther,buffer);
    for(i=0;i<WHAL_NUM_11A_CAL_PIERS;i++){
        SformatOutput(buffer, MBUFFER-1,"[%d]: %d",i,WHAL_FBIN2FREQ(pPiers[i], 0));
        ErrorPrint(NartOther,buffer);
    }
}*/

void PrintQc98xx_5GHzRxCalData(int client, const QC98XX_EEPROM *pEeprom)
{
	A_UINT16 numPiers = WHAL_NUM_11A_CAL_PIERS;
	A_UINT16 pc; //pier count
	char  buffer [ MBUFFER ];
	const A_UINT8 *pPiers = &pEeprom->calFreqPier5G[0];
	const NF_CAL_DATA_PER_CHIP *pData = &pEeprom->nfCalData5G[0];
	int i;

	SformatOutput(buffer, MBUFFER-1, " |==================5G Rx Calibration Information =======================|");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	ErrorPrint(NartOther,buffer);
	for (i = 0; i < WHAL_NUM_CHAINS; ++i)
	{
		SformatOutput ( buffer, MBUFFER-1, " |                          Chain %d                                      |", i );
		ErrorPrint ( NartOther,buffer);
		SformatOutput ( buffer, MBUFFER-1, " |-----------------------------------------------------------------------|" );
		ErrorPrint ( NartOther,buffer );
		SformatOutput ( buffer, MBUFFER-1, " |   Freq   |    nf(dBr)    | nfpower(dBm) |    temp    |   temp_slope   |" );
		ErrorPrint ( NartOther,buffer );
		SformatOutput ( buffer, MBUFFER-1, " |-----------------------------------------------------------------------|" );
		ErrorPrint ( NartOther,buffer );
		for ( pc = 0; pc < numPiers; pc++ ) {
			SformatOutput ( buffer, MBUFFER-1, " |   %04d   |     %4d      |     %4d     |    %4d    |      %4d      |",
				WHAL_FBIN2FREQ(pPiers[pc], 0 ),
				pData [ pc ].NF_Power_dBr[i], pData [ pc ].NF_Power_dBm[i], pData [ pc ].NF_thermCalVal, pData [ pc ].NF_thermCalSlope
				);
			ErrorPrint ( NartOther,buffer );
		}
		SformatOutput ( buffer, MBUFFER-1, " |-----------------------------------------------------------------------|" );
		ErrorPrint(NartOther,buffer);
	}
	SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	ErrorPrint ( NartOther, buffer );
}


void PrintQc98xx_5GHzCalData(int client, const QC98XX_EEPROM *pEeprom)
{
    A_UINT16 numPiers = WHAL_NUM_11A_CAL_PIERS;
    A_UINT16 pc; //pier count
    char  buffer[MBUFFER];
    const A_UINT8 *pPiers = &pEeprom->calFreqPier5G[0];
    const CAL_DATA_PER_FREQ_OLPC *pData = &pEeprom->calPierData5G[0];
	int i;

    SformatOutput(buffer, MBUFFER-1, " |=================5G Power Calibration Information =====================|");
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
    ErrorPrint(NartOther,buffer);
	for (i = 0; i < WHAL_NUM_CHAINS; ++i)
	{
		SformatOutput(buffer, MBUFFER-1, " |                          Chain %d                                      |", i);
		ErrorPrint(NartOther,buffer);
		SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
		ErrorPrint(NartOther,buffer);
		SformatOutput(buffer, MBUFFER-1, " | Freq  txgainIdx0 dacGain0  Pwr0  txgainIdx1 dacGain1 Pwr1  Volt  Temp |");
		ErrorPrint(NartOther,buffer);
		SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
		ErrorPrint(NartOther,buffer);
		for(pc = 0; pc < numPiers; pc++) {
			SformatOutput(buffer, MBUFFER-1, " | %04d    %4d      %3d     %3d      %4d      %3d     %3d   %3d   %3d  |",
                WHAL_FBIN2FREQ(pPiers[pc], 0),
				pData[pc].calPerPoint[i].txgainIdx[0], pData[pc].dacGain[0], pData[pc].calPerPoint[i].power_t8[0]/8, 
				pData[pc].calPerPoint[i].txgainIdx[1], pData[pc].dacGain[1], pData[pc].calPerPoint[i].power_t8[1]/8, 
				pData[pc].voltCalVal, pData[pc].thermCalVal
                );
            ErrorPrint(NartOther,buffer);
		}
		SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
		ErrorPrint(NartOther,buffer);
    }
    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
    ErrorPrint(NartOther,buffer);
}

void PrintQc98xx_5GLegacyTargetPower(int client, const CAL_TARGET_POWER_LEG *pVals, const A_UINT8 *pFreq)
{
    int i,j;
    char buffer[MBUFFER],buffer2[MBUFFER];

    SformatOutput(buffer, MBUFFER-1, " |===========================5G Target Powers============================|");
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1," |  OFDM     ");

    for (j = 0; j < WHAL_NUM_11A_LEG_TARGET_POWER_CHANS; j++) {
        SformatOutput(buffer2, MBUFFER-1,"|  %04d   ", WHAL_FBIN2FREQ(*(pFreq+j),0));
        strcat(buffer,buffer2);
    }

    strcat(buffer,"|");
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," |===========|=========|=========|=========|=========|=========|=========|");
    ErrorPrint(NartOther, buffer);

    for (j = 0; j < WHAL_NUM_LEGACY_TARGET_POWER_RATES; j++) {
        SformatOutput(buffer, MBUFFER-1," |  %s     ",sRatePrintLegacy[j]);
        for(i=0;i<WHAL_NUM_11A_LEG_TARGET_POWER_CHANS;i++){
            SformatOutput(buffer2, MBUFFER-1,"|  %2d.%d   ", pVals[i].tPow2x[j]/2, (pVals[i].tPow2x[j] % 2) * 5);
            strcat(buffer,buffer2);
        }

        strcat(buffer,"|");
        ErrorPrint(NartOther, buffer);
	}
	SformatOutput(buffer, MBUFFER-1," |========================================================================");
    ErrorPrint(NartOther, buffer);
}

void PrintQc98xx_5GHT20TargetPower(int client, const CAL_TARGET_POWER_11A_20 *pVals, const A_UINT8 *pFreq) 
{
    int i,j;
    char buffer[MBUFFER],buffer2[MBUFFER];
	double power;
	int rateIndex;

    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1," |  VHT20    ");

    for (j = 0; j < WHAL_NUM_11A_20_TARGET_POWER_CHANS; j++)
    {
        SformatOutput(buffer2, MBUFFER-1,"|  %04d   ", WHAL_FBIN2FREQ(*(pFreq+j),0));
        strcat(buffer,buffer2);
    }

    strcat(buffer,"|");
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," |===========|=========|=========|=========|=========|=========|=========|");
    ErrorPrint(NartOther, buffer);

    for (j = 0; j < RATE_PRINT_VHT_SIZE; j++)
    {
        SformatOutput(buffer, MBUFFER-1," |  %s ",sRatePrintVHT[j]);
        for(i=0;i<WHAL_NUM_11A_20_TARGET_POWER_CHANS;i++)
        {
			rateIndex = vRATE_INDEX_HT20_MCS0 + j; 
			Qc98xxTargetPowerGet(WHAL_FBIN2FREQ(*(pFreq+i),0), rateIndex, &power);
            SformatOutput(buffer2, MBUFFER-1,"|  %2.1f   ", power);
            strcat(buffer,buffer2);
        }

        strcat(buffer,"|");
        ErrorPrint(NartOther, buffer);
    }
    SformatOutput(buffer, MBUFFER-1," |========================================================================");
    ErrorPrint(NartOther, buffer);
}


void PrintQc98xx_5GHT40TargetPower(int client, const CAL_TARGET_POWER_11A_40 *pVals, const A_UINT8 *pFreq) 
{
    int i,j;
    char buffer[MBUFFER],buffer2[MBUFFER];
	double power;
	int rateIndex;

    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1," |  VHT40    ");

    for (j = 0; j < WHAL_NUM_11A_40_TARGET_POWER_CHANS; j++)
    {
        SformatOutput(buffer2, MBUFFER-1,"|  %04d   ", WHAL_FBIN2FREQ(*(pFreq+j),0));
        strcat(buffer,buffer2);
    }

    strcat(buffer,"|");
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," |===========|=========|=========|=========|=========|=========|=========|");
    ErrorPrint(NartOther, buffer);

    for (j = 0; j < RATE_PRINT_VHT_SIZE; j++)
    {
        SformatOutput(buffer, MBUFFER-1," |  %s ",sRatePrintVHT[j]);
        for(i=0;i<WHAL_NUM_11A_40_TARGET_POWER_CHANS;i++)
        {
			rateIndex = vRATE_INDEX_HT40_MCS0 + j; 
			Qc98xxTargetPowerGet(WHAL_FBIN2FREQ(*(pFreq+i),0), rateIndex, &power);
            SformatOutput(buffer2, MBUFFER-1,"|  %2.1f   ", power);
            strcat(buffer,buffer2);
        }

        strcat(buffer,"|");
        ErrorPrint(NartOther, buffer);
    }
    SformatOutput(buffer, MBUFFER-1," |========================================================================");
    ErrorPrint(NartOther, buffer);
}

void PrintQc98xx_5GHT80TargetPower(int client, const CAL_TARGET_POWER_11A_80 *pVals, const A_UINT8 *pFreq) 
{
    int i,j;
    char buffer[MBUFFER],buffer2[MBUFFER];
	double power;
	int rateIndex;

    SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1," |  VHT80    ");

    for (j = 0; j < WHAL_NUM_11A_80_TARGET_POWER_CHANS; j++)
    {
        SformatOutput(buffer2, MBUFFER-1,"|  %04d   ", WHAL_FBIN2FREQ(*(pFreq+j),0));
        strcat(buffer,buffer2);
    }

    strcat(buffer,"|");
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," |===========|=========|=========|=========|=========|=========|=========|");
    ErrorPrint(NartOther, buffer);

    for (j = 0; j < RATE_PRINT_VHT_SIZE; j++)
    {
        SformatOutput(buffer, MBUFFER-1," |  %s ",sRatePrintVHT[j]);
        for(i=0;i<WHAL_NUM_11A_80_TARGET_POWER_CHANS;i++)
        {
			rateIndex = vRATE_INDEX_HT80_MCS0 + j; 
			Qc98xxTargetPowerGet(WHAL_FBIN2FREQ(*(pFreq+i),0), rateIndex, &power);
            SformatOutput(buffer2, MBUFFER-1,"|  %2.1f   ", power);
            strcat(buffer,buffer2);
        }

        strcat(buffer,"|");
        ErrorPrint(NartOther, buffer);
    }
    SformatOutput(buffer, MBUFFER-1," |========================================================================");
    ErrorPrint(NartOther, buffer);
}

void PrintQc98xx_5GCTLIndex(int client, const A_UINT8 *pCtlIndex){
    int i;
    char buffer[MBUFFER];
    SformatOutput(buffer, MBUFFER-1,"5G CTL Index:");
    ErrorPrint(NartOther,buffer);
    for(i=0;i<WHAL_NUM_CTLS_5G;i++){
		if (pCtlIndex[i] == 0)
		{
			continue;
		}
        SformatOutput(buffer, MBUFFER-1,"[%d] :0x%x",i,pCtlIndex[i]);
        ErrorPrint(NartOther,buffer);
    }
}

void PrintQc98xx_5GCTLData(int client, const A_UINT8 *ctlIndex, const CAL_CTL_DATA_5G Data[WHAL_NUM_CTLS_5G], const A_UINT8 *pFreq)
{

    int i,j;
    char buffer[MBUFFER],buffer2[MBUFFER];

    SformatOutput(buffer, MBUFFER-1," |                                                                       |");
    ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," |=======================Test Group Band Edge Power======================|");
    ErrorPrint(NartOther, buffer);
    SformatOutput(buffer, MBUFFER-1," |                                                                       |");
    ErrorPrint(NartOther, buffer);
    for (i = 0; i<WHAL_NUM_CTLS_5G; i++)
    {
		if (ctlIndex[i] == 0)
		{
		    pFreq+=WHAL_NUM_BAND_EDGES_5G;
			continue;
		}
        SformatOutput(buffer, MBUFFER-1," |                                                                       |");
        ErrorPrint(NartOther, buffer);
        SformatOutput(buffer, MBUFFER-1," | CTL: 0x%02x %s                                           |",
                 ctlIndex[i], sCtlType[ctlIndex[i] & QC98XX_CTL_MODE_M]);
        ErrorPrint(NartOther, buffer);
        SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
        ErrorPrint(NartOther, buffer);
        //WHAL_FBIN2FREQ(*(pFreq++),0),Data[i].ctlEdges[j].tPower,Data[i].ctlEdges[j].flag

        SformatOutput(buffer, MBUFFER-1," | edge  ");

        for (j = 0; j < WHAL_NUM_BAND_EDGES_5G; j++) {
            if (*(pFreq+j) == QC98XX_BCHAN_UNUSED) {
                SformatOutput(buffer2, MBUFFER-1,"|  --   ");
            } else {
                SformatOutput(buffer2, MBUFFER-1,"| %04d  ", WHAL_FBIN2FREQ(*(pFreq+j),0));
            }
            strcat(buffer,buffer2);
        }

        strcat(buffer,"|");
        ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
        ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," | power ");

        for (j = 0; j < WHAL_NUM_BAND_EDGES_5G; j++) {
            if (*(pFreq+j) == QC98XX_BCHAN_UNUSED) {
                SformatOutput(buffer2, MBUFFER-1,"|  --   ");
            } else {
                SformatOutput(buffer2, MBUFFER-1,"| %2d.%d  ", Data[i].ctl_edges[j].u.tPower / 2,
                   (Data[i].ctl_edges[j].u.tPower % 2) * 5);
            }
            strcat(buffer,buffer2);
        }

        strcat(buffer,"|");
        ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
        ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," | flag  ");

        for (j = 0; j < WHAL_NUM_BAND_EDGES_5G; j++) {
            if (*(pFreq+j) == QC98XX_BCHAN_UNUSED) {
                SformatOutput(buffer2, MBUFFER-1,"|  --   ");
            } else {
                SformatOutput(buffer2, MBUFFER-1,"|   %1d   ", Data[i].ctl_edges[j].u.flag);
            }
            strcat(buffer,buffer2);
        }

        strcat(buffer,"|");
        ErrorPrint(NartOther, buffer);

        pFreq+=WHAL_NUM_BAND_EDGES_5G;

        SformatOutput(buffer, MBUFFER-1," =========================================================================");
        ErrorPrint(NartOther, buffer);
    }
}

void PrintQc98xxStruct(QC98XX_EEPROM *pEeprom, int client)

{
    char buffer[MBUFFER],buffer2[MBUFFER];
    int i;

    SformatOutput(buffer, MBUFFER-1,"start get all");
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1,"                         ----------------------                           ");
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1,  " =======================| QC98XX CAL STRUCTURE |==========================");
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1," |                       -----------------------                         |");
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1," |                                   |                                   |");
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1," | Board Data Version      %2d        |  Template Version       %2d        |",
        pEeprom->baseEepHeader.eeprom_version,
        pEeprom->baseEepHeader.template_version
        );
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1," |-----------------------------------------------------------------------|");
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1," | MacAddress: 0x%02X:%02X:%02X:%02X:%02X:%02X                                       |",
          pEeprom->baseEepHeader.macAddr[0],pEeprom->baseEepHeader.macAddr[1],pEeprom->baseEepHeader.macAddr[2],
          pEeprom->baseEepHeader.macAddr[3],pEeprom->baseEepHeader.macAddr[4],pEeprom->baseEepHeader.macAddr[5]);
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1," | Customer Data in hex                                                  |");
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1," |");
    for(i=0;i<CUSTOMER_DATA_SIZE;i++){
        SformatOutput(buffer2, MBUFFER-1," %02X", pEeprom->baseEepHeader.custData[i] );
        strcat(buffer,buffer2);
    }
    strcat(buffer,"           |");
    ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1," |-----------------------------------------------------------------------|");
    ErrorPrint(NartOther,buffer);
    
    PrintQc98xxBaseHeader(client, &(pEeprom->baseEepHeader), pEeprom);

	if (pEeprom->baseEepHeader.opCapBrdFlags.opFlags & WHAL_OPFLAGS_11G)
	{
		PrintQc98xx_2GHzHeader(client, &(pEeprom->biModalHeader[1]), pEeprom);
		PrintQc98xx_2GHzCalData(client, pEeprom);
        PrintQc98xx_2GHzRxCalData ( client, pEeprom );
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
		PrintQc98xx_5GHzRxCalData ( client, pEeprom );
		PrintQc98xx_5GLegacyTargetPower(client, pEeprom->targetPower5G, pEeprom->targetFreqbin5G);
		PrintQc98xx_5GHT20TargetPower(client, pEeprom->targetPower5GVHT20, pEeprom->targetFreqbin5GVHT20);
		PrintQc98xx_5GHT40TargetPower(client, pEeprom->targetPower5GVHT40, pEeprom->targetFreqbin5GVHT40);
		PrintQc98xx_5GHT80TargetPower(client, pEeprom->targetPower5GVHT80, pEeprom->targetFreqbin5GVHT80);
		PrintQc98xx_5GCTLData(client, pEeprom->ctlIndex5G, pEeprom->ctlData5G, &pEeprom->ctlFreqbin5G[0][0]);
	}
}

void PrintQc98xxStructHost(int client, int useDefault)
{
    QC98XX_EEPROM *pEeprom = (QC98XX_EEPROM *)pQc9kEepromBoardArea;

    if(!useDefault){
        pEeprom = (QC98XX_EEPROM *)pQc9kEepromArea;   // prints the Current EEPROM structure
    }
    PrintQc98xxStruct(pEeprom, client);
}

void PrintQc98xxStructDut(int client, int useDefault)
{
    int i, blockSize;
    A_UINT8 buffer[sizeof(QC98XX_EEPROM)];
    A_UINT8 *pEeprom = pQc9kEepromArea;

    // save host board data area to buffer
    memcpy(buffer, pEeprom, sizeof(QC98XX_EEPROM));
    memset(pEeprom, 0, sizeof(QC98XX_EEPROM));

    for(i=0; i<sizeof(QC98XX_EEPROM); i= i+MAX_DATA_LENGTH)
    {
        if((i+MAX_DATA_LENGTH)>sizeof(QC98XX_EEPROM))
        {
            blockSize = sizeof(QC98XX_EEPROM)-i;
        }
        else
        {
            blockSize = MAX_DATA_LENGTH;
        }
        if (Qc9KMemoryRead(fwBoardDataAddress+i, (A_UINT32 *)(&pEeprom[i]), blockSize) != 0)
        {
            SformatOutput(buffer, MBUFFER-1," Error in reading FW board data");
            ErrorPrint(NartOther,buffer);
            return;
        }
    }
#ifdef AP_BUILD
    for(i=0;i<sizeof(QC98XX_EEPROM);i=i+4)
        * (A_UINT32 *)(&pEeprom[i]) = SWAP32(* (A_UINT32 *)(&pEeprom[i]));
#endif

#ifdef AP_BUILD
    Qc98xx_swap_eeprom((QC98XX_EEPROM *)pEeprom);
#endif
    PrintQc98xxStruct((QC98XX_EEPROM *)pEeprom, client);

    // restore host board data area
    memcpy(pEeprom, buffer, sizeof(QC98XX_EEPROM));
}

void Qc98xxEepromDisplayAll(void)
{
    PrintQc98xxStruct(0,0);
}

