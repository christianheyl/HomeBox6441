
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "AquilaNewmaMapping.h"

#include "wlantype.h"
#include "NewArt.h"
#include "smatch.h"
#include "ErrorPrint.h"
#include "NartError.h"

#include "ah.h"
#include "ah_internal.h"
#include "ar9300eep.h"
#include "ah_devid.h"
#include "ar9300reg.h"
#include "ar9300.h"

#include "mEepStruct9300.h"
#include "Ar9300EepromStructSet.h"

#include "ParameterConfigDef.h"
#include "ar9300Eeprom_newItems.h"

#define MBUFFER 1024

extern struct ath_hal *AH;

void print9300BaseHeader(int client, const OSPREY_BASE_EEP_HEADER    *pBaseEepHeader)
{
	char  buffer[MBUFFER];

	SformatOutput(buffer, MBUFFER-1, " | RegDomain 1              0x%04X   |  RegDomain 2             0x%04X   |",
		pBaseEepHeader->reg_dmn[0],
		pBaseEepHeader->reg_dmn[1]
		);
	ErrorPrint(NartOther, buffer);

	SformatOutput(buffer, MBUFFER-1, " | TX Mask                  0x%X      |  RX Mask                 0x%X      |",
		(pBaseEepHeader->txrx_mask&0xF0)>>4,
		pBaseEepHeader->txrx_mask&0x0F
		);
	ErrorPrint(NartOther, buffer);
	SformatOutput(buffer, MBUFFER-1," | OpFlags: 5GHz            %d        |  2GHz                    %d        |",
             (pBaseEepHeader->op_cap_flags.op_flags & OSPREY_OPFLAGS_11A) || 0,
			 (pBaseEepHeader->op_cap_flags.op_flags & OSPREY_OPFLAGS_11G) || 0
			 );
 	ErrorPrint(NartOther, buffer);
	SformatOutput(buffer, MBUFFER-1," | Disable 5G HT20          %d        |  Disable 2G HT20         %d        |",
		      (pBaseEepHeader->op_cap_flags.op_flags & OSPREY_OPFLAGS_5G_HT20) || 0,
			  (pBaseEepHeader->op_cap_flags.op_flags & OSPREY_OPFLAGS_2G_HT20) || 0
				);
    ErrorPrint(NartOther, buffer);

	SformatOutput(buffer, MBUFFER-1," | Disable 5G HT40          %d        |  Disable 2G HT40         %d        |",
		      (pBaseEepHeader->op_cap_flags.op_flags & OSPREY_OPFLAGS_5G_HT40) || 0,
			  (pBaseEepHeader->op_cap_flags.op_flags & OSPREY_OPFLAGS_2G_HT40) || 0
				);
    ErrorPrint(NartOther, buffer);


    SformatOutput(buffer, MBUFFER-1," | Big Endian               %d        |  Wake On Wireless        %d        |",
			 (pBaseEepHeader->op_cap_flags.eepMisc & OSPREY_EEPMISC_BIG_ENDIAN) || 0, (pBaseEepHeader->op_cap_flags.eepMisc & OSPREY_EEPMISC_WOW) || 0);
	ErrorPrint(NartOther, buffer);

	SformatOutput(buffer, MBUFFER-1, " | RF Silent                0x%X      |  Bluetooth               0x%X      |",
		pBaseEepHeader->rf_silent,
		pBaseEepHeader->blue_tooth_options
		);
	ErrorPrint(NartOther, buffer);

	SformatOutput(buffer, MBUFFER-1, " | Device Cap               %d        |  DeviceType              %s  |",
		pBaseEepHeader->device_cap,
		sDeviceType[pBaseEepHeader->device_type]
		);
	ErrorPrint(NartOther, buffer);
		SformatOutput(buffer, MBUFFER-1 ," | Tuning Caps(0,1):        (%02x,%02x)  |  Enbl Compensation       0x%02x     |",
		pBaseEepHeader->params_for_tuning_caps[0],
		pBaseEepHeader->params_for_tuning_caps[1],
		pBaseEepHeader->feature_enable
		);
	ErrorPrint(NartOther, buffer);
	SformatOutput(buffer, MBUFFER-1, " | GPIO eepWrite Enable     %d        |  GPIO rxBand Selection   0x%02x     |",
		pBaseEepHeader->eeprom_write_enable_gpio, pBaseEepHeader->rx_band_select_gpio);
	ErrorPrint(NartOther, buffer);
	SformatOutput(buffer, MBUFFER-1, " | GPIO wlan Disable        %d        |  GPIO wlan LED           0x%02x     |",
		pBaseEepHeader->wlan_disable_gpio, pBaseEepHeader->wlan_led_gpio);
	ErrorPrint(NartOther, buffer);
	SformatOutput(buffer, MBUFFER-1, " | txrxGain                 0x%02x     |  pwrTableOffset          %d        |",
		pBaseEepHeader->txrxgain, pBaseEepHeader->pwrTableOffset);
	ErrorPrint(NartOther, buffer);
	SformatOutput(buffer, MBUFFER-1, " | internal regulator     0x%08x                                     |",
		pBaseEepHeader->swreg);
	ErrorPrint(NartOther,buffer);
}



void print9300_5GHzHeader(int client, const OSPREY_MODAL_EEP_HEADER   *pModalHeader, const OSPREY_BASE_EXTENSION_2 *base_ext2, const OSPREY_BASE_EXTENSION_1 *base_ext1)
{
	char  buffer[MBUFFER];

	SformatOutput(buffer, MBUFFER-1," |                                                                       |");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1," |===========================5GHz Modal Header===========================|");
	ErrorPrint(NartOther, buffer);

	SformatOutput(buffer, MBUFFER-1," |                                                                       |");
	ErrorPrint(NartOther,buffer);

    SformatOutput(buffer, MBUFFER-1," | Antenna Common        0x%08X  |  Antenna Common2      0x%08X  |",
        pModalHeader->ant_ctrl_common,
		pModalHeader->ant_ctrl_common2
		);
	ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," | Ant Chain 0              0x%04X   |  Ant Chain 1              0x%04X  |",
        pModalHeader->ant_ctrl_chain[0],
		pModalHeader->ant_ctrl_chain[1]
		);
	ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," | Ant Chain 2              0x%04X   |                                   |",
                 pModalHeader->ant_ctrl_chain[2]);
		ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," | xatten 1DB Ch0 Low      %2x        |  xatten 1DB Ch1 Low      %2x       |",
                 base_ext2->xatten1_db_low[0],
				 base_ext2->xatten1_db_low[1]
				 );
		ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," | xatten 1DB Ch2 Low      %2x        |  xatten 1DB Ch0          %2x       |",
                 base_ext2->xatten1_db_low[2],
				 pModalHeader->xatten1_db[0]
				 );
		ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," | xatten 1DB Ch1          %2x        |  xatten 1DB Ch2          %2x       |",
                 pModalHeader->xatten1_db[1],
				 pModalHeader->xatten1_db[2]
				 );
		ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," | xatten 1DB Ch0 High     %2x        |  xatten 1DB Ch1 High     %2x       |",
                 base_ext2->xatten1_db_high[0],
				 base_ext2->xatten1_db_high[1]
				 );
		ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," | xatten 1DB Ch2 high     %2x        |                                   |",
				 base_ext2->xatten1_db_high[2]
				 );
		ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," | xatten 1Margin0 Low     %2x        |  xatten 1Margin1 Low     %2x       |",
                 base_ext2->xatten1_margin_low[0],
				 base_ext2->xatten1_margin_low[1]
				 );
		ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," | xatten 1Margin2 Low     %2x        |  xatten 1Margin0         %2x       |",
                 base_ext2->xatten1_margin_low[2],
                 pModalHeader->xatten1_margin[0]
				 );
		ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," | xatten 1Margin1         %2x        |  xatten 1Margin2         %2x       |",
                 pModalHeader->xatten1_margin[1],
				 pModalHeader->xatten1_margin[2]
				 );
		ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," | xatten 1Margin0 High    %2x        |  xatten 1Margin1 High    %2x       |",
                 base_ext2->xatten1_margin_high[0],
				 base_ext2->xatten1_margin_high[1]
				 );
		ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," | xatten 1Margin2 High    %2x        |                                   |",
				 base_ext2->xatten1_margin_high[2]
				 );
		ErrorPrint(NartOther, buffer);
	if(!AR_SREV_SCORPION(AH)) {
	SformatOutput(buffer, MBUFFER-1," | TempL Slope            %3d        |  Temp Slope            %3d        |",
                                base_ext2->temp_slope_low,
                                 pModalHeader->temp_slope
                                 );
                ErrorPrint(NartOther, buffer);

                SformatOutput(buffer, MBUFFER-1," | TempH Slope            %3d        |  Volt Slope            %3d        |",
                                base_ext2->temp_slope_high,
                                 pModalHeader->voltSlope
                                 );
                ErrorPrint(NartOther, buffer);
	} else {
	/*Scorpion has individual tempslope registers*/
        SformatOutput(buffer, MBUFFER-1," | TempL Slope           %3d,%3d,%3d |  Temp Slope      %3d,%3d,%3d      |",
				base_ext1->tempslopextension[2],
				base_ext1->tempslopextension[3],
				base_ext1->tempslopextension[4],
				 pModalHeader->temp_slope,
				base_ext1->tempslopextension[0],
				base_ext1->tempslopextension[1]
				 );
		ErrorPrint(NartOther, buffer);

		SformatOutput(buffer, MBUFFER-1," | TempH Slope      %3d,%3d,%3d      |  Volt Slope             %3d       |",
				base_ext1->tempslopextension[5],
				base_ext1->tempslopextension[6],
				base_ext1->tempslopextension[7],
				 pModalHeader->voltSlope
				 );
		ErrorPrint(NartOther, buffer);
	}
	SformatOutput(buffer, MBUFFER-1," | Spur Channels:           %04d, %04d, %04d, %04d, %04d                 |",
                 FBIN2FREQ(pModalHeader->spur_chans[0],0),
				 FBIN2FREQ(pModalHeader->spur_chans[1],0),
				 FBIN2FREQ(pModalHeader->spur_chans[2],0),
				 FBIN2FREQ(pModalHeader->spur_chans[3],0),
				 FBIN2FREQ(pModalHeader->spur_chans[4],0)
				 );
		ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," | MinCCAPwr Thres0      %3d         |  MinCCAPwrThres1       %3d        |",
                 pModalHeader->noise_floor_thresh_ch[0],
				 pModalHeader->noise_floor_thresh_ch[1]
				 );
		ErrorPrint(NartOther, buffer);

		SformatOutput(buffer, MBUFFER-1," | MinCCAPwr Thres2      %3d         |                                   |",
                 pModalHeader->noise_floor_thresh_ch[2]
				 );
		ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," | xpa_bias_lvl            %2x        |  txFrame2DataStart       %2x       |",
                 pModalHeader->xpa_bias_lvl,
				 pModalHeader->tx_frame_to_data_start
				 );
		ErrorPrint(NartOther, buffer);

		SformatOutput(buffer, MBUFFER-1," | tx_frame_to_pa_on       %2x        |  txClip                  %2x       |",
                 pModalHeader->tx_frame_to_pa_on,
				 pModalHeader->txClip
				 );
		ErrorPrint(NartOther, buffer);

		SformatOutput(buffer, MBUFFER-1," | antenna_gain           %3d        |  switchSettling           %2x      |",
                 pModalHeader->antenna_gain,
				 pModalHeader->switchSettling
				 );
		ErrorPrint(NartOther, buffer);

		SformatOutput(buffer, MBUFFER-1," | adcDesiredSize          %3d       |  txEndToXpaOff           %2x       |",
                 pModalHeader->adcDesiredSize,
				 pModalHeader->tx_end_to_xpa_off
				 );
		ErrorPrint(NartOther, buffer);

		SformatOutput(buffer, MBUFFER-1," | txEndToRxOn             %2x        |  tx_frame_to_xpa_on      %2x       |",
                 pModalHeader->txEndToRxOn,
				 pModalHeader->tx_frame_to_xpa_on
				 );
		ErrorPrint(NartOther, buffer);

		SformatOutput(buffer, MBUFFER-1," | thresh62                 %2d       |                                   |",
                 pModalHeader->thresh62
				 );
		ErrorPrint(NartOther, buffer);

		SformatOutput(buffer, MBUFFER-1," | paprd_rate_mask_ht20    %8x  |  paprd_rate_mask_ht40   %8x  |",
                 pModalHeader->paprd_rate_mask_ht20,
				 pModalHeader->paprd_rate_mask_ht40
				 );
		ErrorPrint(NartOther, buffer);
}



void print9300_2GHzHeader(int client, const OSPREY_MODAL_EEP_HEADER   *pModalHeader, const OSPREY_BASE_EXTENSION_2 *base_ext2)
{
	char  buffer[MBUFFER];


	SformatOutput(buffer, MBUFFER-1," |                                                                       |");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1," |===========================2GHz Modal Header===========================|");
	ErrorPrint(NartOther, buffer);

	SformatOutput(buffer, MBUFFER-1," |                                                                       |");
	ErrorPrint(NartOther,buffer);

    SformatOutput(buffer, MBUFFER-1," | Antenna Common        0x%08X  |  Antenna Common2      0x%08X  |",
        pModalHeader->ant_ctrl_common,
		pModalHeader->ant_ctrl_common2
		);
	ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," | Ant Chain 0              0x%04X   |  Ant Chain 1              0x%04X  |",
        pModalHeader->ant_ctrl_chain[0],
		pModalHeader->ant_ctrl_chain[1]
		);
	ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," | Ant Chain 2              0x%04X   |                                   |",
                 pModalHeader->ant_ctrl_chain[2]);
		ErrorPrint(NartOther, buffer);


        SformatOutput(buffer, MBUFFER-1," | xatten 1DB Ch0          %2x        |  xatten 1DB Ch1          %2x       |",
                 pModalHeader->xatten1_db[0],
				 pModalHeader->xatten1_db[1]
				 );
		ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," | xatten 1DB Ch2          %2x        |                                   |",
                 pModalHeader->xatten1_db[2]
				 );
		ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," | xatten 1Margin0         %2x        |  xatten 1Margin1         %2x       |",
                 pModalHeader->xatten1_margin[0],
				 pModalHeader->xatten1_margin[1]
				 );
		ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," | xatten 1Margin2         %2x        |                                   |",
                 pModalHeader->xatten1_margin[2]
				 );
		ErrorPrint(NartOther, buffer);
	if(!AR_SREV_SCORPION(AH)) {
	 SformatOutput(buffer, MBUFFER-1," | Temp Slope             %3d        |  Volt Slope            %3d        |",
                 pModalHeader->temp_slope,
                                 pModalHeader->voltSlope
                                 );
                ErrorPrint(NartOther, buffer);
	} else {
	/*Scorpion has individual tempslope Registers*/
        SformatOutput(buffer, MBUFFER-1," | Temp Slope           %3d,%3d,%3d  |  Volt Slope             %3d       |",
                 base_ext2->temp_slope_low,
		 pModalHeader->temp_slope,
		 base_ext2->temp_slope_high,
		 pModalHeader->voltSlope
				 );
		ErrorPrint(NartOther, buffer);
	}
         SformatOutput(buffer, MBUFFER-1," | Spur Channels:           %04d, %04d, %04d, %04d, %04d                 |", 
		 	         FBIN2FREQ(pModalHeader->spur_chans[0],1),
				 FBIN2FREQ(pModalHeader->spur_chans[1],1),
				 FBIN2FREQ(pModalHeader->spur_chans[2],1),
				 FBIN2FREQ(pModalHeader->spur_chans[3],1),
				 FBIN2FREQ(pModalHeader->spur_chans[4],1)
				 );
		ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," | MinCCAPwr Thres0      %3d         |  MinCCAPwrThres1       %3d        |",
                 pModalHeader->noise_floor_thresh_ch[0],
				 pModalHeader->noise_floor_thresh_ch[1]
				 );
		ErrorPrint(NartOther, buffer);

		SformatOutput(buffer, MBUFFER-1," | MinCCAPwr Thres2      %3d         |                                   |",
                 pModalHeader->noise_floor_thresh_ch[2]
				 );
		ErrorPrint(NartOther, buffer);

        SformatOutput(buffer, MBUFFER-1," | xpa_bias_lvl            %2x        |  txFrame2DataStart       %2x       |",
                 pModalHeader->xpa_bias_lvl,
				 pModalHeader->tx_frame_to_data_start
				 );
		ErrorPrint(NartOther, buffer);

		SformatOutput(buffer, MBUFFER-1," | tx_frame_to_pa_on       %2x        |  txClip                  %2x       |",
                 pModalHeader->tx_frame_to_pa_on,
				 pModalHeader->txClip
				 );
		ErrorPrint(NartOther, buffer);

		SformatOutput(buffer, MBUFFER-1," | antenna_gain           %3d        |  switchSettling           %2x      |",
                 pModalHeader->antenna_gain,
				 pModalHeader->switchSettling
				 );
		ErrorPrint(NartOther, buffer);

		SformatOutput(buffer, MBUFFER-1," | adcDesiredSize          %3d       |  txEndToXpaOff           %2x       |",
                 pModalHeader->adcDesiredSize,
				 pModalHeader->tx_end_to_xpa_off
				 );
		ErrorPrint(NartOther, buffer);

		SformatOutput(buffer, MBUFFER-1," | txEndToRxOn             %2x        |  tx_frame_to_xpa_on      %2x       |",
                 pModalHeader->txEndToRxOn,
				 pModalHeader->tx_frame_to_xpa_on
				 );
		ErrorPrint(NartOther, buffer);

		SformatOutput(buffer, MBUFFER-1," | thresh62                 %2d       |                                   |",
                 pModalHeader->thresh62
				 );
		ErrorPrint(NartOther, buffer);

		SformatOutput(buffer, MBUFFER-1," | paprd_rate_mask_ht20    %8x  |  paprd_rate_mask_ht40   %8x  |",
                 pModalHeader->paprd_rate_mask_ht20,
				 pModalHeader->paprd_rate_mask_ht40
				 );
		ErrorPrint(NartOther, buffer);
}

void print9300_5GHzCalPiers(int client, A_UINT8 *pPiers)
{
	int i=0;
	char  buffer[MBUFFER];
	SformatOutput(buffer, MBUFFER-1,"5G Cal Freqncy Piers:");
	ErrorPrint(NartOther,buffer);
	for(i=0;i<8;i++){
		SformatOutput(buffer, MBUFFER-1,"[%d]: %d",i,FBIN2FREQ(pPiers[i], 0));
		ErrorPrint(NartOther,buffer);
	}
}

void print9300_2GHzCalPiers(int client, A_UINT8 *pPiers)
{
	int i=0;
	char  buffer[MBUFFER];
	SformatOutput(buffer, MBUFFER-1,"2G Cal Freqncy Piers:");
	ErrorPrint(NartOther,buffer);
	for(i=0;i<3;i++){
		SformatOutput(buffer, MBUFFER-1,"[%d]: %d",i,FBIN2FREQ(pPiers[i], 1));
		ErrorPrint(NartOther,buffer);
	}
}

void print9300_5GHzCalData(int client, const ar9300_eeprom_t *p9300)
{
	A_UINT16 numPiers = OSPREY_NUM_5G_CAL_PIERS;
    A_UINT16 pc; //pier count
    char  buffer[MBUFFER];
    const A_UINT8 *pPiers = &p9300->cal_freq_pier_5g[0];
    const OSP_CAL_DATA_PER_FREQ_OP_LOOP *pData = &p9300->cal_pier_data_5g[0][0];

	SformatOutput(buffer, MBUFFER-1, " |=================5G Power Calibration Information =====================|");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1, " |                   Chain 0                          Chain 1            |");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1, " |                     rxNF rxNF rx  |                   rxNF rxNF   rx  |");
	ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1, " | Freq  Pwr Volt Temp  Cal pwr Temp |     Pwr Volt Temp  Cal  pwr  Temp |");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
	ErrorPrint(NartOther,buffer);
    for(pc = 0; pc < numPiers; pc++) {
        SformatOutput(buffer, MBUFFER-1, " | %04d %4d %3d  %3d  %04d %4d %3d |    %4d %3d  %3d  %04d %4d  %3d  |",
                FBIN2FREQ(pPiers[pc], 0),
			pData->ref_power, pData->volt_meas, pData->temp_meas, pData->rx_noisefloor_cal, pData->rx_noisefloor_power, pData->rxTempMeas,
				(pData+1*OSPREY_NUM_5G_CAL_PIERS)->ref_power, (pData+1*OSPREY_NUM_5G_CAL_PIERS)->volt_meas, (pData+1*OSPREY_NUM_5G_CAL_PIERS)->temp_meas,
			(pData+1*OSPREY_NUM_5G_CAL_PIERS)->rx_noisefloor_cal, (pData+1*OSPREY_NUM_5G_CAL_PIERS)->rx_noisefloor_power, (pData+1*OSPREY_NUM_5G_CAL_PIERS)->rxTempMeas
			);
		ErrorPrint(NartOther,buffer);
        pData++;
	}
	SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1, " |                   Chain 2                                             |");
	ErrorPrint(NartOther,buffer);	
	SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
	ErrorPrint(NartOther,buffer);
	pData = &p9300->cal_pier_data_5g[0][0];
	for(pc = 0; pc < numPiers; pc++) {
        SformatOutput(buffer, MBUFFER-1, " | %04d %4d %3d  %3d  %04d %4d %3d |                                   |",
            FBIN2FREQ(pPiers[pc], 0),
			(pData+2*OSPREY_NUM_5G_CAL_PIERS)->ref_power, (pData+2*OSPREY_NUM_5G_CAL_PIERS)->volt_meas, (pData+2*OSPREY_NUM_5G_CAL_PIERS)->temp_meas,
			(pData+2*OSPREY_NUM_5G_CAL_PIERS)->rx_noisefloor_cal, (pData+2*OSPREY_NUM_5G_CAL_PIERS)->rx_noisefloor_power, (pData+2*OSPREY_NUM_5G_CAL_PIERS)->rxTempMeas
				);
			ErrorPrint(NartOther,buffer);
            pData++;
    }
	SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
	ErrorPrint(NartOther,buffer);

	SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	ErrorPrint(NartOther,buffer);
}

void print9300_2GHzCalData(int client, const ar9300_eeprom_t *p9300)
{
	A_UINT16 numPiers = OSPREY_NUM_2G_CAL_PIERS;
    A_UINT16 pc; //pier count
   char  buffer[MBUFFER];
    const A_UINT8 *pPiers = &p9300->cal_freq_pier_2g[0];
    const OSP_CAL_DATA_PER_FREQ_OP_LOOP *pData = &p9300->cal_pier_data_2g[0][0];



	SformatOutput(buffer, MBUFFER-1, " |=================2G Power Calibration Information =====================|");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1, " |                   Chain 0                          Chain 1            |");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1, " |                     rxNF rxNF rx  |                   rxNF rxNF   rx  |");
	ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1, " | Freq  Pwr Volt Temp  Cal pwr Temp |     Pwr Volt Temp  Cal  pwr  Temp |");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
	ErrorPrint(NartOther,buffer);
    for(pc = 0; pc < numPiers; pc++) {
        SformatOutput(buffer, MBUFFER-1, " | %04d %4d %3d  %3d  %04d %4d %3d |    %4d %3d  %3d  %04d %4d  %3d  |",
                FBIN2FREQ(pPiers[pc], 1),
			pData->ref_power, pData->volt_meas, pData->temp_meas, pData->rx_noisefloor_cal, pData->rx_noisefloor_power, pData->rxTempMeas,
				(pData+1*OSPREY_NUM_2G_CAL_PIERS)->ref_power, (pData+1*OSPREY_NUM_2G_CAL_PIERS)->volt_meas, (pData+1*OSPREY_NUM_2G_CAL_PIERS)->temp_meas,
			(pData+1*OSPREY_NUM_2G_CAL_PIERS)->rx_noisefloor_cal, (pData+1*OSPREY_NUM_2G_CAL_PIERS)->rx_noisefloor_power, (pData+1*OSPREY_NUM_2G_CAL_PIERS)->rxTempMeas
				);
			ErrorPrint(NartOther,buffer);
            pData++;
    }
	SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1, " |                   Chain 2                                             |");
	ErrorPrint(NartOther,buffer);	
	SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
	ErrorPrint(NartOther,buffer);
	pData = &p9300->cal_pier_data_2g[0][0];
	for(pc = 0; pc < numPiers; pc++) {
        SformatOutput(buffer, MBUFFER-1, " | %04d %4d %3d  %3d  %04d %4d %3d |                                   |",
            FBIN2FREQ(pPiers[pc], 1),
			(pData+2*OSPREY_NUM_2G_CAL_PIERS)->ref_power, (pData+2*OSPREY_NUM_2G_CAL_PIERS)->volt_meas, (pData+2*OSPREY_NUM_2G_CAL_PIERS)->temp_meas,
			(pData+2*OSPREY_NUM_2G_CAL_PIERS)->rx_noisefloor_cal, (pData+2*OSPREY_NUM_2G_CAL_PIERS)->rx_noisefloor_power, (pData+2*OSPREY_NUM_2G_CAL_PIERS)->rxTempMeas
				);
			ErrorPrint(NartOther,buffer);
            pData++;
    }
	SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	ErrorPrint(NartOther,buffer);
}

void print9300_5GLegacyTargetPower(int client, const CAL_TARGET_POWER_LEG *pVals, const A_UINT8 *pFreq)
{
	int i,j;
	char buffer[MBUFFER],buffer2[MBUFFER];

	SformatOutput(buffer, MBUFFER-1, " |===========================5G Target Powers============================|");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	ErrorPrint(NartOther,buffer);
			SformatOutput(buffer, MBUFFER-1," | OFDM  ");

			for (j = 0; j < 8; j++) {

                    SformatOutput(buffer2, MBUFFER-1,"| %04d  ", FBIN2FREQ(*(pFreq+j),0));
					strcat(buffer,buffer2);
            }

            strcat(buffer,"|");
			ErrorPrint(NartOther, buffer);

            SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
			ErrorPrint(NartOther, buffer);



            for (j = 0; j < 4; j++) {

					SformatOutput(buffer, MBUFFER-1," | %s  ",sRatePrintLegacy[j]);
					for(i=0;i<8;i++){
						SformatOutput(buffer2, MBUFFER-1,"| %2d.%d  ", pVals[i].t_pow2x[j]/2, (pVals[i].t_pow2x[j] % 2) * 5);
						strcat(buffer,buffer2);
                }

				strcat(buffer,"|");
				ErrorPrint(NartOther, buffer);

            }
			SformatOutput(buffer, MBUFFER-1," |========================================================================");
			ErrorPrint(NartOther, buffer);
}

void print9300_2GLegacyTargetPower(int client, const CAL_TARGET_POWER_LEG *pVals, const A_UINT8 *pFreq)
{
	int i,j;
	char buffer[MBUFFER],buffer2[MBUFFER];

	SformatOutput(buffer, MBUFFER-1, " |===========================2G Target Powers============================|");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	ErrorPrint(NartOther,buffer);
			SformatOutput(buffer, MBUFFER-1," | OFDM   ");

			for (j = 0; j < 3; j++) {

                    SformatOutput(buffer2, MBUFFER-1,"|        %04d        ", FBIN2FREQ(*(pFreq+j),1));
					strcat(buffer,buffer2);
            }

            strcat(buffer,"|");
			ErrorPrint(NartOther, buffer);

            SformatOutput(buffer, MBUFFER-1," |========|====================|====================|====================|");
			ErrorPrint(NartOther, buffer);



            for (j = 0; j < 4; j++) {

					SformatOutput(buffer, MBUFFER-1," | %s   ",sRatePrintLegacy[j]);
					for(i=0;i<3;i++){
						SformatOutput(buffer2, MBUFFER-1,"|        %2d.%d        ", pVals[i].t_pow2x[j]/2, (pVals[i].t_pow2x[j] % 2) * 5);
						strcat(buffer,buffer2);
                }

				strcat(buffer,"|");
				ErrorPrint(NartOther, buffer);

            }
			SformatOutput(buffer, MBUFFER-1," |========================================================================");
			ErrorPrint(NartOther, buffer);
}


void print9300_2GCCKTargetPower(int client, const CAL_TARGET_POWER_LEG *pVals, const A_UINT8 *pFreq)
{
	int i,j;
	char buffer[MBUFFER],buffer2[MBUFFER];

	SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	ErrorPrint(NartOther,buffer);
			SformatOutput(buffer, MBUFFER-1," | CCK    ");

			for (j = 0; j < 2; j++) {

                    SformatOutput(buffer2, MBUFFER-1,"|        %04d        ", FBIN2FREQ(*(pFreq+j),1));
					strcat(buffer,buffer2);
            }

            strcat(buffer,"|====================|");
			ErrorPrint(NartOther, buffer);

            SformatOutput(buffer, MBUFFER-1," |========|====================|====================|====================|");
			ErrorPrint(NartOther, buffer);



            for (j = 0; j < 4; j++) {

					SformatOutput(buffer, MBUFFER-1," | %s  ",sRatePrintCck[j]);
					for(i=0;i<2;i++){
						SformatOutput(buffer2, MBUFFER-1,"|        %2d.%d        ", pVals[i].t_pow2x[j]/2, (pVals[i].t_pow2x[j] % 2) * 5);
						strcat(buffer,buffer2);
                }

				strcat(buffer,"|====================|");
				ErrorPrint(NartOther, buffer);

            }
			SformatOutput(buffer, MBUFFER-1," |========================================================================");
			ErrorPrint(NartOther, buffer);
}


void print9300_5GHT20TargetPower(int client, const OSP_CAL_TARGET_POWER_HT *pVals, const A_UINT8 *pFreq) {

	int i,j;
	char buffer[MBUFFER],buffer2[MBUFFER];

	SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	ErrorPrint(NartOther,buffer);
			SformatOutput(buffer, MBUFFER-1," | HT20  ");

			for (j = 0; j < 8; j++) {

                    SformatOutput(buffer2, MBUFFER-1,"| %04d  ", FBIN2FREQ(*(pFreq+j),0));
					strcat(buffer,buffer2);
            }

            strcat(buffer,"|");
			ErrorPrint(NartOther, buffer);

            SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
			ErrorPrint(NartOther, buffer);



            for (j = 0; j < 24; j++) {

					SformatOutput(buffer, MBUFFER-1," | %s ",sRatePrintHT[j]);
					for(i=0;i<8;i++){
						SformatOutput(buffer2, MBUFFER-1,"| %2d.%d  ", pVals[i].t_pow2x[mapRate2Index[j]]/2, (pVals[i].t_pow2x[mapRate2Index[j]] % 2) * 5);
						strcat(buffer,buffer2);
                }

				strcat(buffer,"|");
				ErrorPrint(NartOther, buffer);

            }
			SformatOutput(buffer, MBUFFER-1," |========================================================================");
			ErrorPrint(NartOther, buffer);
}


void print9300_5GHT40TargetPower(int client, const OSP_CAL_TARGET_POWER_HT *pVals, const A_UINT8 *pFreq) {

	int i,j;
	char buffer[MBUFFER],buffer2[MBUFFER];

	SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	ErrorPrint(NartOther,buffer);
			SformatOutput(buffer, MBUFFER-1," | HT40  ");

			for (j = 0; j < 8; j++) {

                    SformatOutput(buffer2, MBUFFER-1,"| %04d  ", FBIN2FREQ(*(pFreq+j),0));
					strcat(buffer,buffer2);
            }

            strcat(buffer,"|");
			ErrorPrint(NartOther, buffer);

            SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
			ErrorPrint(NartOther, buffer);



            for (j = 0; j < 24; j++) {

					SformatOutput(buffer, MBUFFER-1," | %s ",sRatePrintHT[j]);
					for(i=0;i<8;i++){
						SformatOutput(buffer2, MBUFFER-1,"| %2d.%d  ", pVals[i].t_pow2x[mapRate2Index[j]]/2, (pVals[i].t_pow2x[mapRate2Index[j]] % 2) * 5);
						strcat(buffer,buffer2);
                }

				strcat(buffer,"|");
				ErrorPrint(NartOther, buffer);

            }
			SformatOutput(buffer, MBUFFER-1," |========================================================================");
			ErrorPrint(NartOther, buffer);
}

void print9300_2GHT20TargetPower(int client, const OSP_CAL_TARGET_POWER_HT *pVals, const A_UINT8 *pFreq) {

	int i,j;
	char buffer[MBUFFER],buffer2[MBUFFER];

	SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	ErrorPrint(NartOther,buffer);
			SformatOutput(buffer, MBUFFER-1," | HT20   ");

			for (j = 0; j < 3; j++) {

                    SformatOutput(buffer2, MBUFFER-1,"|        %04d        ", FBIN2FREQ(*(pFreq+j),1));
					strcat(buffer,buffer2);
            }

            strcat(buffer,"|");
			ErrorPrint(NartOther, buffer);

            SformatOutput(buffer, MBUFFER-1," |========|====================|====================|====================|");
			ErrorPrint(NartOther, buffer);



            for (j = 0; j < 24; j++) {

					SformatOutput(buffer, MBUFFER-1," | %s  ",sRatePrintHT[j]);
					for(i=0;i<3;i++){
						SformatOutput(buffer2, MBUFFER-1,"|        %2d.%d        ", pVals[i].t_pow2x[mapRate2Index[j]]/2, (pVals[i].t_pow2x[mapRate2Index[j]] % 2) * 5);
						strcat(buffer,buffer2);
                }

				strcat(buffer,"|");
				ErrorPrint(NartOther, buffer);

            }
			SformatOutput(buffer, MBUFFER-1," |========================================================================");
			ErrorPrint(NartOther, buffer);
}


void print9300_2GHT40TargetPower(int client, const OSP_CAL_TARGET_POWER_HT *pVals, const A_UINT8 *pFreq) {

	int i,j;
	char buffer[MBUFFER],buffer2[MBUFFER];

	SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	ErrorPrint(NartOther,buffer);
			SformatOutput(buffer, MBUFFER-1," | HT40   ");

			for (j = 0; j < 3; j++) {

                    SformatOutput(buffer2, MBUFFER-1,"|        %04d        ", FBIN2FREQ(*(pFreq+j),1));
					strcat(buffer,buffer2);
            }

            strcat(buffer,"|");
			ErrorPrint(NartOther, buffer);

            SformatOutput(buffer, MBUFFER-1," |========|====================|====================|====================|");
			ErrorPrint(NartOther, buffer);



            for (j = 0; j < 24; j++) {

					SformatOutput(buffer, MBUFFER-1," | %s  ",sRatePrintHT[j]);
					for(i=0;i<3;i++){
						SformatOutput(buffer2, MBUFFER-1,"|        %2d.%d        ", pVals[i].t_pow2x[mapRate2Index[j]]/2, (pVals[i].t_pow2x[mapRate2Index[j]] % 2) * 5);
						strcat(buffer,buffer2);
                }

				strcat(buffer,"|");
				ErrorPrint(NartOther, buffer);

            }
			SformatOutput(buffer, MBUFFER-1," |========================================================================");
			ErrorPrint(NartOther, buffer);
}

void print9300_5GCTLIndex(int client, const A_UINT8 *pCtlIndex){
	int i;
	char buffer[MBUFFER];
	SformatOutput(buffer, MBUFFER-1,"5G CTL Index:");
	ErrorPrint(NartOther,buffer);
	for(i=0;i<9;i++){
		SformatOutput(buffer, MBUFFER-1,"[%d] :0x%x",i,pCtlIndex[i]);
		ErrorPrint(NartOther,buffer);
	}
}

void print9300_2GCTLIndex(int client, const A_UINT8 *pCtlIndex){
	int i;
	char buffer[MBUFFER];
	SformatOutput(buffer, MBUFFER-1,"2G CTL Index:");
	ErrorPrint(NartOther,buffer);
	for(i=0;i<12;i++){
		SformatOutput(buffer, MBUFFER-1,"[%d] :0x%x",i,pCtlIndex[i]);
		ErrorPrint(NartOther,buffer);
	}
}

void print9300_5GCTLData(int client, const A_UINT8 *ctlIndex, const OSP_CAL_CTL_DATA_5G Data[OSPREY_NUM_CTLS_5G], const A_UINT8 *pFreq){

	int i,j;
	char buffer[MBUFFER],buffer2[MBUFFER];

	SformatOutput(buffer, MBUFFER-1," |                                                                       |");
	ErrorPrint(NartOther, buffer);

	SformatOutput(buffer, MBUFFER-1," |=======================Test Group Band Edge Power======================|");
	ErrorPrint(NartOther, buffer);
	SformatOutput(buffer, MBUFFER-1," |                                                                       |");
	ErrorPrint(NartOther, buffer);
    for (i = 0; i<OSPREY_NUM_CTLS_5G; i++) {

        SformatOutput(buffer, MBUFFER-1," |                                                                       |");
		ErrorPrint(NartOther, buffer);
        SformatOutput(buffer, MBUFFER-1," | CTL: 0x%02x %s                                           |",
                 ctlIndex[i], sCtlType[ctlIndex[i] & OSPREY_CTL_MODE_M]);
		ErrorPrint(NartOther, buffer);
        SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
		ErrorPrint(NartOther, buffer);
		//FBIN2FREQ(*(pFreq++),0),Data[i].ctl_edges[j].t_power,Data[i].ctl_edges[j].flag

            SformatOutput(buffer, MBUFFER-1," | edge  ");

            for (j = 0; j < OSPREY_NUM_BAND_EDGES_5G; j++) {
                if (*(pFreq+j) == OSPREY_BCHAN_UNUSED) {
                    SformatOutput(buffer2, MBUFFER-1,"|  --   ");
                } else {
                    SformatOutput(buffer2, MBUFFER-1,"| %04d  ", FBIN2FREQ(*(pFreq+j),0));
                }
				strcat(buffer,buffer2);
            }

            strcat(buffer,"|");
			ErrorPrint(NartOther, buffer);

            SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
			ErrorPrint(NartOther, buffer);

            SformatOutput(buffer, MBUFFER-1," | power ");

            for (j = 0; j < OSPREY_NUM_BAND_EDGES_5G; j++) {
                if (*(pFreq+j) == OSPREY_BCHAN_UNUSED) {
                    SformatOutput(buffer2, MBUFFER-1,"|  --   ");
                } else {
                    SformatOutput(buffer2, MBUFFER-1,"| %2d.%d  ", Data[i].ctl_edges[j].t_power / 2,
                        (Data[i].ctl_edges[j].t_power % 2) * 5);
                }
				strcat(buffer,buffer2);
            }

            strcat(buffer,"|");
			ErrorPrint(NartOther, buffer);

            SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
			ErrorPrint(NartOther, buffer);

            SformatOutput(buffer, MBUFFER-1," | flag  ");

            for (j = 0; j < OSPREY_NUM_BAND_EDGES_5G; j++) {
                if (*(pFreq+j) == OSPREY_BCHAN_UNUSED) {
                    SformatOutput(buffer2, MBUFFER-1,"|  --   ");

                } else {
                    SformatOutput(buffer2, MBUFFER-1,"|   %1d   ", Data[i].ctl_edges[j].flag);
                }
				strcat(buffer,buffer2);
            }

            strcat(buffer,"|");
			ErrorPrint(NartOther, buffer);

			pFreq+=OSPREY_NUM_BAND_EDGES_5G;

            SformatOutput(buffer, MBUFFER-1," =========================================================================");
			ErrorPrint(NartOther, buffer);

    }
}

void print9300_2GCTLData(int client,  const A_UINT8 *ctlIndex, const OSP_CAL_CTL_DATA_2G Data[OSPREY_NUM_CTLS_2G], const A_UINT8 *pFreq){

	int i,j;
	char buffer[MBUFFER],buffer2[MBUFFER];

    SformatOutput(buffer, MBUFFER-1," |                                                                       |");
	ErrorPrint(NartOther, buffer);

	SformatOutput(buffer, MBUFFER-1," |=======================Test Group Band Edge Power======================|");
	ErrorPrint(NartOther, buffer);
	SformatOutput(buffer, MBUFFER-1," |                                                                       |");
	ErrorPrint(NartOther, buffer);
    for (i = 0; i<OSPREY_NUM_CTLS_2G; i++) {

        SformatOutput(buffer, MBUFFER-1," |                                                                       |");
		ErrorPrint(NartOther, buffer);
        SformatOutput(buffer, MBUFFER-1," | CTL: 0x%02x %s                                           |",
                 ctlIndex[i], sCtlType[ctlIndex[i] & OSPREY_CTL_MODE_M]);
		ErrorPrint(NartOther, buffer);
        SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
		ErrorPrint(NartOther, buffer);
		//FBIN2FREQ(*(pFreq++),0),Data[i].ctl_edges[j].t_power,Data[i].ctl_edges[j].flag

            SformatOutput(buffer, MBUFFER-1," | edge  ");

            for (j = 0; j < OSPREY_NUM_BAND_EDGES_2G; j++) {
                if (*(pFreq+j) == OSPREY_BCHAN_UNUSED) {
                    SformatOutput(buffer2, MBUFFER-1,"|  --   ");
                } else {
                    SformatOutput(buffer2, MBUFFER-1,"| %04d  ", FBIN2FREQ(*(pFreq+j),1));
                }
				strcat(buffer,buffer2);
            }

            strcat(buffer,"|");
			ErrorPrint(NartOther, buffer);

            SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
			ErrorPrint(NartOther, buffer);

            SformatOutput(buffer, MBUFFER-1," | power ");

            for (j = 0; j < OSPREY_NUM_BAND_EDGES_2G; j++) {
                if (*(pFreq+j) == OSPREY_BCHAN_UNUSED) {
                    SformatOutput(buffer2, MBUFFER-1,"|  --   ");
                } else {
                    SformatOutput(buffer2, MBUFFER-1,"| %2d.%d  ", Data[i].ctl_edges[j].t_power / 2,
                        (Data[i].ctl_edges[j].t_power % 2) * 5);
                }
				strcat(buffer,buffer2);
            }

            strcat(buffer,"|");
			ErrorPrint(NartOther, buffer);

            SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
			ErrorPrint(NartOther, buffer);

            SformatOutput(buffer, MBUFFER-1," | flag  ");

            for (j = 0; j < OSPREY_NUM_BAND_EDGES_2G; j++) {
                if (*(pFreq+j) == OSPREY_BCHAN_UNUSED) {
                    SformatOutput(buffer2, MBUFFER-1,"|  --   ");

                } else {
                    SformatOutput(buffer2, MBUFFER-1,"|   %1d   ", Data[i].ctl_edges[j].flag);
                }
				strcat(buffer,buffer2);
            }

            strcat(buffer,"|");
			ErrorPrint(NartOther, buffer);

			pFreq+=OSPREY_NUM_BAND_EDGES_2G;

            SformatOutput(buffer, MBUFFER-1," =========================================================================");
			ErrorPrint(NartOther, buffer);
	}
}

void print9300Struct(int client, int useDefault)

{
	char buffer[MBUFFER],buffer2[MBUFFER];
	int i;
	const ar9300_eeprom_t *pDefault9300=ar9300_eeprom_struct_default(2); //calling for default 2

	if(!useDefault){
		pDefault9300=Ar9300EepromStructGet();   // prints the Current EEPROM structure
	}
    SformatOutput(buffer, MBUFFER-1,"start get all");
    ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1,"                         ----------------------                           ");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1,  " =======================| AR9300 CAL STRUCTURE |==========================");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1," |                       -----------------------                         |");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1," |                                   |                                   |");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1," | CAL Version             %2d        |  Template Version       %2d        |",
		pDefault9300->eeprom_version,
		pDefault9300->template_version
		);
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1," |-----------------------------------------------------------------------|");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1," | MacAddress: 0x%02X:%02X:%02X:%02X:%02X:%02X                                       |",
		  pDefault9300->mac_addr[0],pDefault9300->mac_addr[1],pDefault9300->mac_addr[2],
		  pDefault9300->mac_addr[3],pDefault9300->mac_addr[4],pDefault9300->mac_addr[5]);
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1," | Customer Data in hex                                                  |");
	SformatOutput(buffer, MBUFFER-1," |");
	for(i=0;i<OSPREY_CUSTOMER_DATA_SIZE;i++){
		SformatOutput(buffer2, MBUFFER-1," %02X", pDefault9300->custData[i] );
		strcat(buffer,buffer2);
	}
	strcat(buffer,"           |");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1," |-----------------------------------------------------------------------|");
	ErrorPrint(NartOther,buffer);


    print9300BaseHeader(client, &pDefault9300->base_eep_header);

    print9300_5GHzHeader(client, &pDefault9300->modal_header_5g, &pDefault9300->base_ext2, &pDefault9300->base_ext1);
	print9300_Header_newItems(client, pDefault9300, band_A);
	print9300_5GHzCalData(client, pDefault9300);

	print9300_5GLegacyTargetPower(client, pDefault9300->cal_target_power_5g, pDefault9300->cal_target_freqbin_5g);
    print9300_5GHT20TargetPower(client, pDefault9300->cal_target_power_5g_ht20, pDefault9300->cal_target_freqbin_5g_ht20);
    print9300_5GHT40TargetPower(client, pDefault9300->cal_target_power_5g_ht40, pDefault9300->cal_target_freqbin_5g_ht40);
    print9300_5GCTLData(client, pDefault9300->ctl_index_5g, pDefault9300->ctl_power_data_5g, &pDefault9300->ctl_freqbin_5G[0][0]);

    print9300_2GHzHeader(client, &pDefault9300->modal_header_2g, &pDefault9300->base_ext2);
	print9300_Header_newItems(client, pDefault9300, band_BG);
    print9300_2GHzCalData(client, pDefault9300);
    print9300_2GLegacyTargetPower(client, pDefault9300->cal_target_power_2g, pDefault9300->cal_target_freqbin_2g);
    print9300_2GCCKTargetPower(client, pDefault9300->cal_target_power_cck, pDefault9300->cal_target_freqbin_cck);
    print9300_2GHT20TargetPower(client, pDefault9300->cal_target_power_2g_ht20, pDefault9300->cal_target_freqbin_2g_ht20);
    print9300_2GHT40TargetPower(client, pDefault9300->cal_target_power_2g_ht40, pDefault9300->cal_target_freqbin_2g_ht40);

    print9300_2GCTLData(client, pDefault9300->ctl_index_2g, pDefault9300->ctl_power_data_2g, &pDefault9300->ctl_freqbin_2G[0][0]);

}

void Ar9300EepromDisplayAll(void)
{
	print9300Struct(0,0);
}

