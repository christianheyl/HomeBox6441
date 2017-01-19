

#include "UserPrint.h"
#include "Field.h"
#include "RxDescriptor.h"
//#include "LinkRx.h"
#include "AnwiDriverInterface.h"
#include "Ar9300SpectralScan.h"


int Ar9300SpectralScanEnable(void)
{
	int DO_SPECTRAL_SCANNING = 1;
	int TURN_OFF_TRAFFIC = 1; // turn-off traffic
	int SENSITIZE_RECEIVER = 0;
	int TURN_OFF_RADAR = 1;
	int ENABLE_RESTART = 0;
	int SCAN_PERIOD = 10; //18;
	// -----------------------------------------------------------------
	// Enable Receive.
	// -----------------------------------------------------------------
	FieldWrite("MAC_DMA_CR.RXE_LP",  0x1); // Enable Receiver.
	FieldWrite("MAC_PCU_RX_FILTER.BROADCAST", 0x1); 
	FieldWrite("MAC_PCU_RX_FILTER.PROMISCUOUS", 0x1);
	FieldWrite("MAC_PCU_RX_FILTER.ASSUME_RADAR", 0x1); // Pass only radar errors ?
	FieldWrite("MAC_PCU_PHY_ERROR_MASK.VALUE", 0x0); // enable phy error mask
	FieldWrite("MAC_PCU_PHY_ERROR_MASK_CONT.MASK_VALUE", 0x40); // disable error mask
	FieldWrite("MAC_DMA_RXCFG.ZERO_LEN_DMA_EN", 0x2); // allow zero length packets?

	if (TURN_OFF_TRAFFIC) 
	{
		//////////////// To disable weak signal detection, m1/m2 thresholds can also be considered ////////////////////
		FieldWrite("BB_find_signal.firpwr", 0x7f); // set firpwr to max (signed)
		FieldWrite("BB_find_signal.firstep", 0x3f); // set firstep to max
		FieldWrite("BB_find_signal.relpwr", 0x1f); // set relpwr to max (signed)
		FieldWrite("BB_find_signal.relstep", 0x1f); // set relstep to max (signed)
		FieldWrite("BB_find_signal_low.firpwr_low", 0x7f); // set firpwr_low to max (signed)
		FieldWrite("BB_find_signal_low.firstep_low", 0x3f); // set firstep_low to max
		FieldWrite("BB_find_signal_low.relstep_low", 0x1f); // set relstep to max (signed)
		FieldWrite("BB_timing_control_5.cycpwr_thr1",0x7f);
		FieldWrite("BB_ext_chan_pwr_thr_2_b0.cycpwr_thr1_ext",0x7f);
        	FieldWrite("BB_bbb_dagc_ctrl.enable_barker_rssi_thr", 0x0);
        	FieldWrite("BB_bbb_dagc_ctrl.barker_rssi_thr", 0x1f);
		UserPrint("Weak signal detection is disabled\n");

		//////////////// To disable strong signal detection ////////////////////
		FieldWrite("BB_timing_control_5.rssi_thr1a", 0x7f); // rssi_thr1a  is set to all ones.
		FieldWrite("BB_timing_control_5.enable_rssi_thr1a", 0x1); // enable_rssi_thr1a is set to one
		// Disable strong signal detect due to DC signal
		FieldWrite("BB_extension_radar.radar_dc_pwr_thresh", 0x7f); // set radar_dc_pwr_thresh to max (signed).
		UserPrint("Strong signal detection is disabled\n");
        
		FieldWrite("BB_modes_select.disable_dyn_cck_det",0x0);
        	FieldWrite("BB_modes_select.dyn_ofdm_cck_mode",0x1);
        	FieldWrite("BB_timing_controls_2.enable_dc_offset",0x0);
        	FieldWrite("BB_timing_controls_2.enable_dc_offset_track",0x0);
        	UserPrint("Disabled dynamic ofdm cck mode (fast clock) and dynamic DC offset tracking for Spectral Scan.\n");

	} 
	else if (SENSITIZE_RECEIVER)
	{
		FieldWrite("BB_find_signal.firpwr", 0x0); // set firpwr to max (signed)
		FieldWrite("BB_find_signal.firstep", 0x0); // set firstep to max
		FieldWrite("BB_find_signal.relpwr", 0x0); // set relpwr to max (signed)
		FieldWrite("BB_find_signal.relstep", 0x0); // set relstep to max (signed)
		FieldWrite("BB_find_signal_low.firpwr_low", 0x0); // set firpwr_low to max (signed)
		FieldWrite("BB_find_signal_low.firstep_low", 0x0); // set firstep_low to max
		FieldWrite("BB_find_signal_low.relstep_low", 0x0); // set relstep to max (signed)
		FieldWrite("BB_timing_control_5.cycpwr_thr1",0x0);
		FieldWrite("BB_ext_chan_pwr_thr_2_b0.cycpwr_thr1_ext",0x0);
		UserPrint("Receiver Sensitized\n");
	}



	// -------------- Spectral Scan Initialization ------------

	if (TURN_OFF_RADAR) 
	{
		FieldWrite("BB_radar_detection.pulse_detect_enable", 0x0); 
		UserPrint("Turned off radar\n");
	}
	if (ENABLE_RESTART) 
	{
		FieldWrite("BB_restart.enable_restart", 0x0); 
		UserPrint("Enabled restart\n");
	}
	if (DO_SPECTRAL_SCANNING) 
	{
	  //////////// Spectral Scanning Related Parameters //////////////////////
		FieldWrite("BB_spectral_scan.spectral_scan_use_err5", 0x0); 
		FieldWrite("BB_spectral_scan.spectral_scan_priority", 0x1);
		FieldWrite("BB_spectral_scan.spectral_scan_active", 0x1);   // this bit has to be set, chip will start outputting fft data
	  // until spectral scan count reports
		FieldWrite("BB_spectral_scan.spectral_scan_short_rpt", 0x1);  // report only a one set of fft results for timer triggered scans.
		FieldWrite("BB_spectral_scan.spectral_scan_count", 0);  // this refers to number of times the spectral scan is done. 0 means loop infinitely
	  // until spectral scan active is enabled. Note that if we
	  // set this bit to 0, then spectral scan active will not be self cleared
		FieldWrite("BB_spectral_scan.spectral_scan_period", SCAN_PERIOD);  // time between successive scan entries
		FieldWrite("BB_spectral_scan.spectral_scan_fft_period", 0x1); // this will alow us to report only the first of the fft symbol with in the spectral
		FieldWrite("BB_spectral_scan.spectral_scan_ena", 0x0);
		UserPrint("Spectral Scan enabled\n");
		FieldWrite("BB_radar_detection_2.cf_radar_bin_thresh_sel", 0x1);
		FieldWrite("BB_radar_detection.enable_radar_fft", 0x1); // Set enable_radar_fft
		UserPrint("Spectral Scan Initialization done....\n\n");
	}

	FieldWrite("BB_timing_controls_2.enable_dc_offset", 0x1);
	FieldWrite("BB_timing_controls_2.enable_dc_offset_track", 0x0);

	FieldWrite("BB_spectral_scan.spectral_scan_ena", 0x1);

	return 0;
}

int Ar9300SpectralScanDisable(void)
{
	// ########### needs content
	FieldWrite("BB_timing_controls_2.enable_dc_offset", 0x0);
	FieldWrite("BB_spectral_scan.spectral_scan_ena", 0x0);
	return -1;
}


#define MBUFFER 1024

static void Ar9300SpectralScanDump(void *dr, unsigned int dptr)
{
	char buffer[MBUFFER];
	//int datalen;
	int it;
	int ndump;
	//
	// the data is in the packet
	//
	ndump = RxDescriptorDataLen(dr);
	if(ndump>MBUFFER)
	{
		ndump=MBUFFER;
	}
    //
	// fetch the data
	//
    MyMemoryRead(dptr, (unsigned int *)buffer, ndump);
	//
	// print
	//
	UserPrint("Ar9300SpectralScanDump: ptr=%x %d bytes\n",dptr,ndump);
	for(it=0; it<ndump; it++)
	{
		UserPrint("%02x ",buffer[it]);
	}
	UserPrint("\n");
}


int Ar9300SpectralScanProcess(unsigned char *data, int ndata, int *spectrum, int max)
{
	int nspectrum;
	int it;

	if(ndata==63 || ndata==65)			// correct length for ht20 data is 63, hw bug sometimes gives 65
	{
		if(max>=57)
		{
			nspectrum=57;
		}
		else
		{
			nspectrum= -1;
		}
	}
	else if(ndata==138 || ndata==140)	// correct length for ht40 data is 138, hw bug sometimes gives 140
	{
		if(max>=135)
		{
			nspectrum=135;
		}
		else
		{
			nspectrum= -1;
		}
	}
	else
	{
		nspectrum=-1;
	}
	//
	// copy the fft bins
	// ######## may need to scale this data
	//
	for(it=0; it<nspectrum; it++)
	{
		spectrum[it]=(0x000000ff&data[it]);
	}

	return nspectrum;
}

