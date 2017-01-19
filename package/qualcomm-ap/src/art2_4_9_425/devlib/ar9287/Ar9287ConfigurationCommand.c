#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "wlantype.h"
#include "smatch.h"
#include "UserPrint.h"
#include "TimeMillisecond.h"
#include "CommandParse.h"
#include "NewArt.h"
#include "MyDelay.h"
#include "ParameterSelect.h"
#include "Card.h"
#include "Field.h"

#include "Device.h"

#include "ParameterParse.h"
#include "ParameterParseNart.h"
#include "Link.h"
#include "Calibrate.h"
#include "ConfigurationCommand.h"
//#include "ConfigurationSet.h"
//#include "ConfigurationGet.h"
#include "ConfigurationStatus.h"

#include "ErrorPrint.h"
#include "CardError.h"
#include "ParseError.h"
#include "NartError.h"

#include "Ar9287EepromParameter.h"
#include "Ar9287SetList.h"


//
// hal header files
//
#include "ah.h"
#include "ah_internal.h"
#include "ar5416/ar5416.h"
#include "ar9287eep.h"

#include "Ar9287Device.h"
#include "Ar9287EepromStructSet.h"
#include "Ar9287EepromStructGet.h"
#include "mEepStruct9287.h"
#include "Ar9287EepromPrint.h"
#include "Ar9287EepromSave.h"
#include "Ar9287PcieConfig.h"

//#include "templatelist.h"

#define MBUFFER 1024

#ifndef max
#define max(a,b)	(((a) > (b)) ? (a) : (b))
#endif

extern int Ar9287FlashCal(int value);

#define MALLOWED 100



static int PrintMacAddress(char *buffer, int max, unsigned char mac[6])
{
    int nc;
    //
	// try with colons
	//
	nc=SformatOutput(buffer,max-1,"%02x:%02x:%02x:%02x:%02x:%02x",
		mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    buffer[max-1]=0;
	if (nc!=17) 
		return ERR_VALUE_BAD;
	return VALUE_OK;
}

static int PrintClient;

static void Print(char *format, ...)
{
    va_list ap;
    char buffer[MBUFFER];

    va_start(ap, format);
#if defined LINUX || defined __APPLE__ 
    vsnprintf(buffer,MBUFFER,format,ap);
#else
    _vsnprintf(buffer,MBUFFER,format,ap);
#endif
    va_end(ap);
	ErrorPrint(NartData,buffer);
}



static void ReturnSigned(char *command, char *name, char *atext, int *value, int nvalue)
{
	char buffer[MBUFFER];
	int lc, nc;
	int it;

	lc=0;
	nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|%s|%s%s|%+d",command,name,atext,value[0]);
	if(nc>0)
	{
		lc+=nc;
	}
	for(it=1; it<nvalue; it++)
	{
		nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,",%+d",value[it]);
		if(nc>0)
		{
			lc+=nc;
		}
	}
	nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|",value[it]);
    buffer[MBUFFER-1]=0;
	ErrorPrint(NartData,buffer);
}


static void ReturnUnsigned(char *command, char *name, char *atext, int *value, int nvalue)
{
	char buffer[MBUFFER];
	int lc, nc;
	int it;

	lc=0;
	nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|%s|%s%s|%u",command,name,atext,value[0]);
	if(nc>0)
	{
		lc+=nc;
	}
	for(it=1; it<nvalue; it++)
	{
		nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,",%u",value[it]);
		if(nc>0)
		{
			lc+=nc;
		}
	}
	nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|",value[it]);
    buffer[MBUFFER-1]=0;
	ErrorPrint(NartData,buffer);
}


static void ReturnHex(char *command, char *name, char *atext, int *value, int nvalue)
{
	char buffer[MBUFFER];
	int lc, nc;
	int it;

	lc=0;
	nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|%s|%s%s|0x%x",command,name,atext,value[0]);
	if(nc>0)
	{
		lc+=nc;
	}
	for(it=1; it<nvalue; it++)
	{
		nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,",0x%x",value[it]);
		if(nc>0)
		{
			lc+=nc;
		}
	}
	nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|",value[it]);
    buffer[MBUFFER-1]=0;
	ErrorPrint(NartData,buffer);
}


static void ReturnDouble(char *command, char *name, char *atext, double *value, int nvalue)
{
	char buffer[MBUFFER];
	int lc, nc;
	int it;

	lc=0;
	nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|%s|%s%s|%.1lf",command,name,atext,value[0]);
	if(nc>0)
	{
		lc+=nc;
	}
	for(it=1; it<nvalue; it++)
	{
		nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,",%.1lf",value[it]);
		if(nc>0)
		{
			lc+=nc;
		}
	}
	nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|",value[it]);
    buffer[MBUFFER-1]=0;
	ErrorPrint(NartData,buffer);
}


static void ReturnText(char *command, char *name, char *atext, char *value)
{
	char buffer[MBUFFER];
	int lc, nc;
	int it;

	lc=0;
	nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|%s|%s%s|%s|",command,name,atext,value);
	if(nc>0)
	{
		lc+=nc;
	}
	ErrorPrint(NartData,buffer);
}




struct _ParameterList GetParameter[]=
{
	//
	// "all" must be element [0] because we use the same list for "get" and "set" except 
	// for "set" it starts at element [1]
	//
	AR9287_SET_EEPROM_ALL,

    AR9287_SET_EEPROM_VERSION,
    AR9287_SET_TEMPLATE_VERSION,
	AR9287_SET_EEPROM_CHECKSUM,
	AR9287_SET_EEPROM_BINBUILDNUMBER,
    AR9287_SET_MAC_ADDRESS,
    AR9287_SET_CUSTOMER_DATA,
    AR9287_SET_REGULATORY_DOMAIN,
    AR9287_SET_TX_RX_MASK,
		AR9287_SET_TX_RX_MASK_TX,
		AR9287_SET_TX_RX_MASK_RX,
    AR9287_SET_OP_FLAGS,
    AR9287_SET_EEP_MISC,
    AR9287_SET_RF_SILENT,
		AR9287_SET_RF_SILENT_B0,
		AR9287_SET_RF_SILENT_B1,
		AR9287_SET_RF_SILENT_GPIO,
    AR9287_SET_BLUETOOTH_OPTIONS,
    AR9287_SET_DEVICE_CAPABILITY,
	AR9287_SET_DEVICE_TYPE,
    AR9287_SET_POWER_TABLE_OFFSET,
    AR9287_SET_TUNING,
    AR9287_SET_FEATURE_ENABLE,
		AR9287_SET_FEATURE_ENABLE_TEMPERATURE_COMPENSATION, 
		AR9287_SET_FEATURE_ENABLE_VOLTAGE_COMPENSATION, 
		AR9287_SET_FEATURE_ENABLE_FAST_CLOCK, 
		AR9287_SET_FEATURE_ENABLE_DOUBLING, 
		AR9287_SET_FEATURE_ENABLE_INTERNAL_SWITCHING_REGULATOR, 
		AR9287_SET_FEATURE_ENABLE_PA_PREDISTORTION, 
		AR9287_SET_FEATURE_ENABLE_TUNING_CAPS,
		AR9287_SET_FEATURE_ENABLE_TX_FRAME_TO_XPA_ON,
    AR9287_SET_MISCELLANEOUS,
		AR9287_SET_MISCELLANEOUS_DRIVERS_STRENGTH,
		AR9287_SET_MISCELLANEOUS_THERMOMETER,
		AR9287_SET_MISCELLANEOUS_CHAIN_MASK_REDUCE,
		AR9287_SET_MISCELLANEOUS_QUICK_DROP_ENABLE,
    AR9287_SET_MISCELLANEOUS_TEMP_SLOP_EXTENSION_ENABLE,
		AR9287_SET_MISCELLANEOUS_XLNA_BIAS_STRENGTH_ENABLE,
		AR9287_SET_MISCELLANEOUS_RF_GAIN_CAP_ENABLE,
    AR9287_SET_EEPROM_WRITE_ENABLE_GPIO,
    AR9287_SET_WLAN_DISABLE_GPIO,
    AR9287_SET_WLAN_LED_GPIO,
    AR9287_SET_RX_BAND_SELECT_GPIO,
    AR9287_SET_TX_RX_GAIN,
	    AR9287_SET_TX_RX_GAIN_TX,
	    AR9287_SET_TX_RX_GAIN_RX,
    AR9287_SET_SWITCHING_REGULATOR,
	AR9287_SET_ANTA_DIV_CTL,
//    AR9287_SET_FUTURE,

    AR9287_SET_2GHZ_ANTENNA_CONTROL_COMMON,
    AR9287_SET_2GHZ_ANTENNA_CONTROL_COMMON_2 ,
    AR9287_SET_2GHZ_ANTENNA_CONTROL_CHAIN,
    AR9287_SET_2GHZ_ATTENUATION_DB,
    AR9287_SET_2GHZ_ATTENUATION_MARGIN ,
    AR9287_SET_2GHZ_TEMPERATURE_SLOPE,
    AR9287_SET_2GHZ_TEMPERATURE_SLOPE_PAL_ON,
    AR9287_SET_2GHZ_FUTUREBASE,
    AR9287_SET_2GHZ_VOLTAGE_SLOPE,
    AR9287_SET_2GHZ_SPUR,
    AR9287_SET_2GHZ_NOISE_FLOOR_THRESHOLD,
    AR9287_SET_2GHZ_XPDGAIN,
    AR9287_SET_2GHZ_XPD,
    AR9287_SET_2GHZ_IQCALICH,
    AR9287_SET_2GHZ_IQCALQCH,
//	AR9287_SET_2GHZ_RESERVED,
	AR9287_SET_2GHZ_QUICK_DROP,
    AR9287_SET_2GHZ_XPA_BIAS_LEVEL,
    AR9287_SET_2GHZ_BSWATTEN,
    AR9287_SET_2GHZ_BSWMARGIN,
    AR9287_SET_2GHZ_SWSETTLEHT40,
    AR9287_SET_2GHZ_MODALHEADERVERSION,
    AR9287_SET_2GHZ_DB1,
    AR9287_SET_2GHZ_DB2,
    AR9287_SET_2GHZ_OBCCK,
    AR9287_SET_2GHZ_OBPSK,
    AR9287_SET_2GHZ_OBQAM,
    AR9287_SET_2GHZ_OBPALOFF,
    AR9287_SET_2GHZ_FUTUREMODAL,
    AR9287_SET_2GHZ_SPURRANGELOW,
    AR9287_SET_2GHZ_SPURRANGEHIGH,
    AR9287_SET_2GHZ_CTLDATA,
    AR9287_SET_2GHZ_PADDING,
	AR9287_SET_2GHZ_TX_FRAME_TO_DATA_START,
	AR9287_SET_2GHZ_TX_FRAME_TO_PA_ON,
	AR9287_SET_2GHZ_TX_FRAME_TO_XPA_ON,
	AR9287_SET_2GHZ_TX_END_TO_XPA_OFF,
	AR9287_SET_2GHZ_TX_END_TO_RX_ON,
	AR9287_SET_2GHZ_TX_CLIP,
	AR9287_SET_2GHZ_DAC_SCALE_CCK,
	AR9287_SET_2GHZ_ANTENNA_GAIN,
	AR9287_SET_2GHZ_SWITCH_SETTLING,
	AR9287_SET_2GHZ_ADC_DESIRED_SIZE,
	AR9287_SET_2GHZ_THRESH62,
	AR9287_SET_2GHZ_PAPD_HT20,
	AR9287_SET_2GHZ_PAPD_HT40,
    AR9287_SET_2GHZ_WLAN_SPDT_SWITCH_GLOBAL_CONTROL,
    AR9287_SET_2GHZ_XLNA_BIAS_STRENGTH,
	AR9287_SET_2GHZ_RF_GAIN_CAP,
//	AR9287_SET_2GHZ_FUTURE,
    AR9287_SET_2GHZ_CALIBRATION_FREQUENCY,
	AR9287_SET_2GHZ_CALIBRATION_DATA,
    AR9287_SET_2GHZ_POWER_CORRECTION,
    AR9287_SET_2GHZ_CALIBRATION_VOLTAGE,
    AR9287_SET_2GHZ_CALIBRATION_TEMPERATURE,
    AR9287_SET_2GHZ_CALIBRATION_PCDAC,
	AR9287_SET_2GHZ_NOISE_FLOOR,
    AR9287_SET_2GHZ_NOISE_FLOOR_POWER,
    AR9287_SET_2GHZ_NOISE_FLOOR_TEMPERATURE,
	AR9287_SET_2GHZ_TARGET_DATA_CCK,
    AR9287_SET_2GHZ_TARGET_FREQUENCY_CCK,
    AR9287_SET_2GHZ_TARGET_POWER_CCK,
    AR9287_SET_2GHZ_TARGET_DATA,
    AR9287_SET_2GHZ_TARGET_FREQUENCY,
    AR9287_SET_2GHZ_TARGET_POWER,
	AR9287_SET_2GHZ_TARGET_DATA_HT20,
    AR9287_SET_2GHZ_TARGET_FREQUENCY_HT20,
    AR9287_SET_2GHZ_TARGET_POWER_HT20,
	AR9287_SET_2GHZ_TARGET_DATA_HT40,
    AR9287_SET_2GHZ_TARGET_FREQUENCY_HT40,
    AR9287_SET_2GHZ_TARGET_POWER_HT40,
    AR9287_SET_2GHZ_CTL_INDEX,
	AR9287_SET_2GHZ_CTL_FREQUENCY,
    AR9287_SET_2GHZ_CTL_POWER,
	AR9287_SET_2GHZ_CTL_BANDEDGE,

#if 0 // Ar9287 is 2G only
    AR9287_SET_5GHZ_ANTENNA_CONTROL_COMMON,
    AR9287_SET_5GHZ_ANTENNA_CONTROL_COMMON_2 ,
    AR9287_SET_5GHZ_ANTENNA_CONTROL_CHAIN,
    AR9287_SET_5GHZ_ATTENUATION_DB_LOW,
    AR9287_SET_5GHZ_ATTENUATION_DB,
    AR9287_SET_5GHZ_ATTENUATION_DB_HIGH,
    AR9287_SET_5GHZ_ATTENUATION_MARGIN_LOW,
    AR9287_SET_5GHZ_ATTENUATION_MARGIN,
    AR9287_SET_5GHZ_ATTENUATION_MARGIN_HIGH,
    AR9287_SET_5GHZ_TEMPERATURE_SLOPE_LOW,
    AR9287_SET_5GHZ_TEMPERATURE_SLOPE,
    AR9287_SET_5GHZ_TEMPERATURE_SLOPE_HIGH,
    AR9287_SET_5GHZ_TEMPERATURE_SLOPE_EXTENSION,
    AR9287_SET_5GHZ_VOLTAGE_SLOPE,
    AR9287_SET_5GHZ_SPUR,
    AR9287_SET_5GHZ_NOISE_FLOOR_THRESHOLD,
	AR9287_SET_5GHZ_RESERVED,
	AR9287_SET_5GHZ_QUICK_DROP_LOW,
	AR9287_SET_5GHZ_QUICK_DROP,
	AR9287_SET_5GHZ_QUICK_DROP_HIGH,
    AR9287_SET_5GHZ_XPA_BIAS_LEVEL,
	AR9287_SET_5GHZ_TX_FRAME_TO_DATA_START,
	AR9287_SET_5GHZ_TX_FRAME_TO_PA_ON,
	AR9287_SET_5GHZ_TX_FRAME_TO_XPA_ON,
	AR9287_SET_5GHZ_TX_END_TO_XPA_OFF,
	AR9287_SET_5GHZ_TX_END_TO_RX_ON,
	AR9287_SET_5GHZ_TX_CLIP,
	AR9287_SET_5GHZ_ANTENNA_GAIN,
	AR9287_SET_5GHZ_SWITCH_SETTLING,
	AR9287_SET_5GHZ_ADC_DESIRED_SIZE,
	AR9287_SET_5GHZ_THRESH62,
	AR9287_SET_5GHZ_PAPD_HT20,
	AR9287_SET_5GHZ_PAPD_HT40,
    AR9287_SET_5GHZ_WLAN_SPDT_SWITCH_GLOBAL_CONTROL,
    AR9287_SET_5GHZ_XLNA_BIAS_STRENGTH,
	AR9287_SET_5GHZ_RF_GAIN_CAP,
	AR9287_SET_5GHZ_FUTURE,
    AR9287_SET_5GHZ_CALIBRATION_FREQUENCY,
    AR9287_SET_5GHZ_POWER_CORRECTION,
    AR9287_SET_5GHZ_CALIBRATION_VOLTAGE,
    AR9287_SET_5GHZ_CALIBRATION_TEMPERATURE,
	AR9287_SET_5GHZ_NOISE_FLOOR,
    AR9287_SET_5GHZ_NOISE_FLOOR_POWER,
    AR9287_SET_5GHZ_NOISE_FLOOR_TEMPERATURE,
    AR9287_SET_5GHZ_TARGET_FREQUENCY,
    AR9287_SET_5GHZ_TARGET_POWER,
    AR9287_SET_5GHZ_TARGET_FREQUENCY_HT20,
    AR9287_SET_5GHZ_TARGET_POWER_HT20,
    AR9287_SET_5GHZ_TARGET_FREQUENCY_HT40,
    AR9287_SET_5GHZ_TARGET_POWER_HT40,
    AR9287_SET_5GHZ_CTL_INDEX,
	AR9287_SET_5GHZ_CTL_FREQUENCY,
    AR9287_SET_5GHZ_CTL_POWER,
	AR9287_SET_5GHZ_CTL_BANDEDGE,
#endif
	AR9287_SET_CONFIG,
	AR9287_SET_CONFIG_PCIE,
	AR9287_SET_EEPROM_DEVICEID,
	AR9287_SET_EEPROM_SSID,
	AR9287_SET_EEPROM_VID,
	AR9287_SET_EEPROM_SVID,

    AR9287_SET_CALDATA_MEMORY_TYPE,

    AR9287_LENGTH_EEPROM,
    AR9287_OPEN_LOOP_POWER_CONTROL,
    Ar9287_Tx_Rx_Atten,
    AR9287_RX_TX_MARGIN,
    AR9287_HT40_POWER_INC_FOR_PDADC,
    AR9287_CAL_FREQ_PIER_SET,
    AR9287_PDGAIN_OVERLAP,
};


AR9287DLLSPEC int Ar9287GetParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(GetParameter)/sizeof(GetParameter[0]);
    list->special=GetParameter;
	return 0;
}


AR9287DLLSPEC int Ar9287SetParameterSplice(struct _ParameterList *list)
{
    list->nspecial=(sizeof(GetParameter)/sizeof(GetParameter[0]))-1;
    list->special=&GetParameter[1];
	return 0;
}


#define MAXVALUE 1000

static void ReturnGet(int ix, int iy, int iz, int status, int *done, int *error, int ip, int index, int *value, int num)
{
	char atext[MBUFFER];

				//
				// remember array indexing
				//
				if(ix<0)
				{
					SformatOutput(atext,MBUFFER-1,"");
					ix=0;
					iy=0;
					iz=0;
				}
				else if(iy<0)
				{
					SformatOutput(atext,MBUFFER-1,"[%d]",ix);
					iy=0;
					iz=0;
				}
				else if(iz<0)
				{
					SformatOutput(atext,MBUFFER-1,"[%d][%d]",ix,iy);
					iz=0;
				}
				else
				{
					SformatOutput(atext,MBUFFER-1,"[%d][%d][%d]",ix,iy,iz);
					iz=0;
				}
	if(status==VALUE_OK)
	{
		if (GetParameter[index].type=='u' || GetParameter[index].type=='z') {
			ReturnUnsigned("get",GetParameter[index].word[0],atext,value,num);
		} else if (GetParameter[index].type=='d') {
			ReturnSigned("get",GetParameter[index].word[0],atext,value,num);
		} else if (GetParameter[index].type=='x') {
			ReturnHex("get",GetParameter[index].word[0],atext,value,num);
		}
		(*done)+=1;
	} else {	// ErrorPrint();
		(*error)+=1; 
	}
}

static void ReturnGetl(int ix, int iy, int iz, int status, int *done, int *error, int ip, int index, long *value, int num)
{
	char atext[MBUFFER];

				//
				// remember array indexing
				//
				if(ix<0)
				{
					SformatOutput(atext,MBUFFER-1,"");
					ix=0;
					iy=0;
					iz=0;
				}
				else if(iy<0)
				{
					SformatOutput(atext,MBUFFER-1,"[%d]",ix);
					iy=0;
					iz=0;
				}
				else if(iz<0)
				{
					SformatOutput(atext,MBUFFER-1,"[%d][%d]",ix,iy);
					iz=0;
				}
				else
				{
					SformatOutput(atext,MBUFFER-1,"[%d][%d][%d]",ix,iy,iz);
					iz=0;
				}
	if(status==VALUE_OK)
	{
		if (GetParameter[index].type=='u' || GetParameter[index].type=='z') {
			ReturnUnsigned("get",GetParameter[index].word[0],atext,value,num);
		} else if (GetParameter[index].type=='d') {
			ReturnSigned("get",GetParameter[index].word[0],atext,value,num);
		} else if (GetParameter[index].type=='x') {
			ReturnHex("get",GetParameter[index].word[0],atext,value,num);
		}
		(*done)+=1;
	} else {	// ErrorPrint();
		(*error)+=1; 
	}
}

static void ReturnGetf(int ix, int iy, int iz, int status, int *done, int *error, int ip, int index, double *value, int num)
{
	char atext[MBUFFER];

				//
				// remember array indexing
				//
				if(ix<0)
				{
					SformatOutput(atext,MBUFFER-1,"");
					ix=0;
					iy=0;
					iz=0;
				}
				else if(iy<0)
				{
					SformatOutput(atext,MBUFFER-1,"[%d]",ix);
					iy=0;
					iz=0;
				}
				else if(iz<0)
				{
					SformatOutput(atext,MBUFFER-1,"[%d][%d]",ix,iy);
					iz=0;
				}
				else
				{
					SformatOutput(atext,MBUFFER-1,"[%d][%d][%d]",ix,iy,iz);
					iz=0;
				}
	if(status==VALUE_OK)
	{
			ReturnDouble("get",GetParameter[index].word[0],atext,value,num);
		(*done)+=1;
	} else {	// ErrorPrint();
		(*error)+=1; 
	}
}


static void ConfigSets(int *done, int *error, int ip, int index, int ix, int iy, int iz, int iband, 
							int (*_pSetsBand)(int *value, int ix, int iy, int iz, int num, int iBand))
{
	int status;
	int ngot=0;
	char *name;	
    int value[MAXVALUE]; 
	unsigned int uvalue[MAXVALUE]; 
	char buffer[MBUFFER];
	char *cvalue;
	char *text;
	char atext[MBUFFER];

	status=0;
				//
				// remember array indexing
				//
				if(ix<0)
				{
					SformatOutput(atext,MBUFFER-1,"");
					ix=0;
					iy=0;
					iz=0;
				}
				else if(iy<0)
				{
					SformatOutput(atext,MBUFFER-1,"[%d]",ix);
					iy=0;
					iz=0;
				}
				else if(iz<0)
				{
					SformatOutput(atext,MBUFFER-1,"[%d][%d]",ix,iy);
					iz=0;
				}
				else
				{
					SformatOutput(atext,MBUFFER-1,"[%d][%d][%d]",ix,iy,iz);
				}

	name=CommandParameterName(ip);
	if (GetParameter[index].type=='u' || GetParameter[index].type=='z') {
		ngot=ParseIntegerList(ip,name,uvalue,&GetParameter[index]);
		if(ngot>0) {
			status=_pSetsBand(uvalue, ix, iy, iz, ngot, iband);	
			if(status==VALUE_OK)
				ReturnUnsigned("set",GetParameter[index].word[0],atext,uvalue,ngot);
		}
	} else if (GetParameter[index].type=='d') {
		ngot=ParseIntegerList(ip,name,value,&GetParameter[index]);
		if(ngot>0) {
			status=_pSetsBand(value, ix, iy, iz, ngot, iband);	
			if(status==VALUE_OK)
				ReturnSigned("set",GetParameter[index].word[0],atext,value,ngot);
		}
	} else if (GetParameter[index].type=='x') {
		ngot=ParseHexList(ip,name,uvalue,&GetParameter[index]);
		if(ngot>0) {
			status=_pSetsBand(uvalue, ix, iy, iz, ngot, iband);	
			if(status==VALUE_OK)
				ReturnHex("set",GetParameter[index].word[0],atext,uvalue,ngot);
		}
	}
	if (ngot<=0) {
		(*error)+=1; 
	} else {
		if(status==VALUE_OK) // ErrorPrint();
			(*done)++;
		else
			(*error)+=1; 
	}
}

static void ConfigfSets(int *done, int *error, int ip, int index, int ix, int iy, int iz, int iband, 
							int (*_pfSetsBand)(double *value, int ix, int iy, int iz, int num, int iBand))
{
	int status;
	int ngot=0;
	char *name;	
    double value[MAXVALUE]; 
	char atext[MBUFFER];

				//
				// remember array indexing
				//
				if(ix<0)
				{
					SformatOutput(atext,MBUFFER-1,"");
					ix=0;
					iy=0;
					iz=0;
				}
				else if(iy<0)
				{
					SformatOutput(atext,MBUFFER-1,"[%d]",ix);
					iy=0;
					iz=0;
				}
				else if(iz<0)
				{
					SformatOutput(atext,MBUFFER-1,"[%d][%d]",ix,iy);
					iz=0;
				}
				else
				{
					SformatOutput(atext,MBUFFER-1,"[%d][%d][%d]",ix,iy,iz);
				}

	name=CommandParameterName(ip);
	ngot=ParseDoubleList(ip,name,value,&GetParameter[index]);
	if(ngot>0) {
		status=_pfSetsBand(value, ix, iy, iz, ngot, iband);	
		if(status==VALUE_OK)
			ReturnDouble("set",GetParameter[index].word[0],atext,value,ngot);
	}
}


static void ConfigSet(int *done, int *error, int ip, int index, int iband, 
							int (*_pSet)(int value), int (*_pSetBand)(int value, int iBand))
{
	int status;
	int ngot=0;
	char *name;	
    int value[MAXVALUE]; 
	unsigned int uvalue[MAXVALUE]; 
//	char buffer[MBUFFER];
	char *cvalue;
	char *text;

	status=0;
	name=CommandParameterName(ip);
	if (GetParameter[index].type=='u' || GetParameter[index].type=='z') {
		ngot=ParseIntegerList(ip,name,uvalue,&GetParameter[index]);
		if(ngot>0) {
			if (iband==-1)
				status=_pSet(uvalue[0]);	
			else
				status=_pSetBand(uvalue[0], iband);
			if(status==VALUE_OK)
				ReturnUnsigned("set",GetParameter[index].word[0],"",uvalue,ngot);
		}
	} else if (GetParameter[index].type=='d') {
		ngot=ParseIntegerList(ip,name,value,&GetParameter[index]);
		if(ngot>0) {
			if (iband==-1)
				status=_pSet(value[0]);	
			else
				status=_pSetBand(value[0], iband);	
			if(status==VALUE_OK)
				ReturnSigned("set",GetParameter[index].word[0],"",value,ngot);
		}
	} else if (GetParameter[index].type=='x') {
		ngot=ParseHexList(ip,name,uvalue,&GetParameter[index]);
		if(ngot>0) {
			if (iband==-1)
				status=_pSet(uvalue[0]);	// don't need buffer here, just call ErrorPrint() and return error code.
			else
				status=_pSetBand(uvalue[0], iband);	
			if(status==VALUE_OK)
				ReturnHex("set",GetParameter[index].word[0],"",uvalue,ngot);
		}
	}
	if (ngot<=0) {
		(*error)+=1; 
	} else {
		if(status==VALUE_OK) // ErrorPrint();
			(*done)++;
		else
			(*error)+=1; 
	}
}


static void ConfigSetUnsigned(int *done, int *error, int ip, int index, int iband, 
							int (*_pSet)(unsigned int value), int (*_pSetBand)(unsigned int value, int iBand))
{
	int status;
	int ngot=0;
	char *name;	
    int value[MAXVALUE]; 
	unsigned int uvalue[MAXVALUE]; 
//	char buffer[MBUFFER];
	char *cvalue;
	char *text;

	status=0;
	name=CommandParameterName(ip);
	if (GetParameter[index].type=='u' || GetParameter[index].type=='z') {
		ngot=ParseIntegerList(ip,name,uvalue,&GetParameter[index]);
		if(ngot>0) {
			if (iband==-1)
				status=_pSet(uvalue[0]);	
			else
				status=_pSetBand(uvalue[0], iband);
			if(status==VALUE_OK)
				ReturnUnsigned("set",GetParameter[index].word[0],"",uvalue,ngot);
		}
	} else if (GetParameter[index].type=='d') {
		ngot=ParseIntegerList(ip,name,value,&GetParameter[index]);
		if(ngot>0) {
			if (iband==-1)
				status=_pSet(value[0]);	
			else
				status=_pSetBand(value[0], iband);	
			if(status==VALUE_OK)
				ReturnSigned("set",GetParameter[index].word[0],"",value,ngot);
		}
	} else if (GetParameter[index].type=='x') {
		ngot=ParseHexList(ip,name,uvalue,&GetParameter[index]);
		if(ngot>0) {
			if (iband==-1)
				status=_pSet(uvalue[0]);	// don't need buffer here, just call ErrorPrint() and return error code.
			else
				status=_pSetBand(uvalue[0], iband);	
			if(status==VALUE_OK)
				ReturnHex("set",GetParameter[index].word[0],"",uvalue,ngot);
		}
	}
	if (ngot<=0) {
		(*error)+=1; 
	} else {
		if(status==VALUE_OK) // ErrorPrint();
			(*done)++;
		else
			(*error)+=1; 
	}
}
//
// parse and then set a configuration parameter in the internal structure
//
AR9287DLLSPEC int Ar9287SetCommand(int client)
{
	int np, ip;
	char *name;	
	int error;
	int done;
    int ngot;
	int index;
    int code;
    unsigned char cmac[6];
    int value[MAXVALUE]; 
	unsigned int uvalue[MAXVALUE]; 
	char buffer[MBUFFER];
	char *text;
	int status;
	int ix, iy, iz;
	unsigned long lvalue, addr; 
	int parseStatus;
	char atext[MBUFFER];

	error=0;
	done=0;
	//
	// parse arguments and do it
	//
	np=CommandParameterMany();
	for(ip=0; ip<np; ip++)
	{
		name=CommandParameterName(ip);
		index=ParameterSelectIndexArray(name,&GetParameter[1],sizeof(GetParameter)/sizeof(GetParameter[1])-1, &ix, &iy, &iz);
		if(index>=0)
		{

			index++;
			code=GetParameter[index].code;
			switch(code) 
			{
				case Ar9287SetEeprom2GHzLengthEeprom:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_LengthSet);
					//ConfigSet(&done, &error, ip, index, band_BG, Ar9287_LengthSet, 0);
					break;
				case Ar9287SetEepromChecksum:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_ChecksumSet);
					break;
				case Ar9287SetEepromVersion:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_eepromVersionSet);
					break;
				case Ar9287SetEepromTemplateVersion:
					ConfigSet(&done, &error, ip, index, -1, Ar9287templateVersion, 0);
					break;
				case Ar9287SetEepromBinBuildNumber:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_CalibrationBinaryVersionSet);
					break;
				case Ar9287SetEepromDeviceId:
					ConfigSetUnsigned(&done, &error, ip, index, -1, Ar9287deviceIDSet, 0);
					break;
				case Ar9287SetEepromSSID:
					ConfigSetUnsigned(&done, &error, ip, index, -1, Ar9287SSIDSet, 0);
					break;
				case Ar9287SetEepromVID:
					ConfigSetUnsigned(&done, &error, ip, index, -1, Ar9287vendorSet, 0);
					break;
				case Ar9287SetEepromSVID:
					ConfigSetUnsigned(&done, &error, ip, index, -1, Ar9287SubVendorSet, 0);
					break;
				case Ar9287SetEepromConfig:
					// set config=32bitHexValue; address=16bitHexAddress; 
					parseStatus = ParseHex(ip, name, 1, &lvalue);
					if (parseStatus==1) {
						parseStatus = ParseHex(++ip, name, 1, &addr);
						if (parseStatus==1) {
							status=Ar9287pcieAddressValueDataSet(addr, lvalue, 0);
							SformatOutput(buffer, MBUFFER-1, "%s(0x%x) 0x%x", name, addr, lvalue);
							ReturnText("set",GetParameter[index].word[0],"",buffer);
						}
					} 
					break;
				case Ar9287SetEepromConfigPCIe:
					break;
				case Ar9287SetEepromMacAddress:
					text=CommandParameterValue(ip,0);
					status=ParseMacAddress(text,cmac);
					if (status == VALUE_OK) 
					{
						status=Ar9287_MacAdressSet(cmac);
						if(status==VALUE_OK)
						{
							PrintMacAddress(buffer,MBUFFER,cmac);
							ReturnText("set",GetParameter[index].word[0],"",buffer);
							done++;
						}
						else	// ErrorPrint();
							error++;
					} else {
						ErrorPrint(ParseBadValue,text,name);
						error++;
					}
					break;
				case Ar9287SetEepromCustomerData:
					text=CommandParameterValue(ip,0);
					status=Ar9287_CustomerDataSet(text, 20);  
					if(status==VALUE_OK)
					{
						ReturnText("set",GetParameter[index].word[0],"",text);
						done++;
					}
					else	// ErrorPrint();
						error++;
					break;
				case Ar9287SetEepromRegulatoryDomain:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, -1, Ar9287_RegDmnSet);
					break;
				case Ar9287SetEepromTxRxMask:
					ErrorPrint(NartData,"Please use set Mask.Tx/Mask.Rx");
					break;
				case Ar9287SetEepromTxRxMaskTx:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_TxMaskSet, 0);
					break;
				case Ar9287SetEepromTxRxMaskRx:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_RxMaskSet, 0);
					break;
				case Ar9287SetEepromOpFlags:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_OpFlagsSet, 0);
					break;
				case Ar9287SetEepromEepMisc:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_eepMiscSet, 0);
					break;
				case Ar9287SetEepromRfSilent:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_RFSilentSet, 0);
					break;
				case Ar9287SetEepromBlueToothOptions:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_BlueToothOptionsSet, 0);
					break;
				case Ar9287SetEepromDeviceCapability:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_DeviceCapSet, 0);
					break;
				case Ar9287SetEepromDeviceType:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_DeviceTypeSet, 0);
					break;
				case Ar9287SetEepromPowerTableOffset:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_PwrTableOffsetSetSet, 0);
					break;
				case Ar9287SetEepromFeatureEnableTxFrameToXpaOn:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_TxFrameToXpaOnSet, 0);
					break;
				case Ar9287SetEeprom2GHzAntennaControlCommon:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_AntCtrlCommonSet, 0);
					break;
				case Ar9287SetEeprom2GHzAntennaControlChain:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_AntCtrlChainSet);
					break;
				case Ar9287SetEeprom2GHzOpenLoopPowerControl:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_OpenLoopPwrCntlSet, 0);
					break;
				case Ar9287SetEeprom2GHzTemperatureSlope:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_TempSensSlopeSet, 0);
					break;
				case Ar9287SetEeprom2GHzTemperatureSlopePalOn:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_TempSensSlopePalOnSet, 0);
					break;
				case Ar9287SetEeprom2GHzFutureBase:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_FutureBaseSet);
					break;
				case Ar9287SetEeprom2GHzSpur:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_SpurChansSet);
					break;
				case Ar9287SetEeprom2GHzSpurRangeLow:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_SpurRangeLowSet);
					break;
				case Ar9287SetEeprom2GHzSpurRangeHigh:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_SpurRangeHighSet);
					break;
				case Ar9287SetEeprom2GHzNoiseFloorThreshold:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_NoiseFloorThreshChSet);
					break;
				case Ar9287SetEeprom2GHzXpdGain:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_XpdGainSet);
					break;
				case Ar9287SetEeprom2GHzXpd:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_XpdSet);
					break;
				case Ar9287SetEeprom2GHzIqCalICh:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_IQCalIChSet);
					break;
				case Ar9287SetEeprom2GHzIqCalQCh:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_IQCalQChSet);
					break;
				case Ar9287SetEeprom2GHzBswAtten:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_BswAttenSet);
					break;
				case Ar9287SetEeprom2GHzBswMargin:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_BswMarginSet);
					break;
				case Ar9287SetEeprom2GHzSwSettleHt40:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_SwSettleHT40Set);
					break;
				case Ar9287SetEeprom2GHzModalHeaderversion:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_ModalHeaderVersionSet);
					break;
				case Ar9287SetEeprom2GHzDb1:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_db1Set);
					break;
				case Ar9287SetEeprom2GHzDb2:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_db2Set);
					break;
				case Ar9287SetEeprom2GHzObCck:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_ob_cckSet);
					break;
				case Ar9287SetEeprom2GHzObPsk:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_ob_pskSet);
					break;
				case Ar9287SetEeprom2GHzObQam:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_ob_qamSet);
					break;
				case Ar9287SetEeprom2GHzObPalOff:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_ob_pal_offSet);
					break;
				case Ar9287SetEeprom2GHzFutureModal:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_FutureModalSet);
					break;
				case Ar9287SetEeprom2GHzPadding:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_PaddingSet, 0);
					break;
				case Ar9287SetEeprom2GHzPdGainOverLap:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_PdGainOverlapSet, 0);
					break;
				case Ar9287SetEeprom2GHzXpaBiasLevel:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_XpaBiasLvlSet, 0);
					break;
				case Ar9287SetEeprom2GHzTxFrameToDataStart:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_TxFrameToDataStartSet, 0);
					break;
				case Ar9287SetEeprom2GHzTxFrameToPaOn:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_TxFrameToPaOnSet, 0);
					break;
				case Ar9287SetEeprom2GHzHt40PowerIncForPdadc:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_HT40PowerIncForPdadcSet, 0);
					break;
				case Ar9287SetEeprom2GHzAntennaGain:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_AntennaGainSet);
					break;
				case Ar9287SetEeprom2GHzSwitchSettling:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_SwitchSettlingSet, 0);
					break;
				case Ar9287SetEeprom2GHzAdcSize:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_AdcDesiredSizeSet, 0);
					break;
				case Ar9287SetEeprom2GHzTxEndToXpaOff:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_TxEndToXpaOffSet, 0);
					break;
				case Ar9287SetEeprom2GHzTxEndToRxOn:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_TxEndToRxOnSet, 0);
					break;
				case Ar9287SetEeprom2GHzTxFrameToXpaOn:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_TxFrameToXpaOnSet, 0);
					break;
				case Ar9287SetEeprom2GHzThresh62:
					ConfigSet(&done, &error, ip, index, -1, Ar9287_Thresh62Set, 0);
					break;
				case Ar9287SetEeprom2GHzCalibrationFrequency:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_CalPierFreqSet);
					break;
				case Ar9287SetEeprom2GHzCalibrationData:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_CalPierDataSet);
					break;
				case Ar9287SetEeprom2GHzTargetDataCck:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_CalTgtDatacckSet);
					break;
				case Ar9287SetEeprom2GHzTargetData:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_CalTgtDataSet);
					break;
				case Ar9287SetEeprom2GHzTargetDataHt20:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_CalTgtDataHt20Set);
					break;
				case Ar9287SetEeprom2GHzTargetDataHt40:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_CalTgtDataHt40Set);
					break;
				case Ar9287SetEeprom2GHzTargetFrequencyCck:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_CalTGTpwrcckChannelSet);
					break;
				case Ar9287SetEeprom2GHzTargetFrequency:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_CalTGTPwrChannelSet);
					break;
				case Ar9287SetEeprom2GHzTargetFrequencyHt20:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_CalTGTpwrht20ChannelSet);
					break;
				case Ar9287SetEeprom2GHzTargetFrequencyHt40:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_CalTGTpwrht40ChannelSet);
					break;
				case Ar9287SetEeprom2GHzTargetPowerCck:
					ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_CalTGTpwrcckSet);
					break;
				case Ar9287SetEeprom2GHzTargetPower:
					ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_CalTGTPwrLegacyOFDMSet);
					break;
				case Ar9287SetEeprom2GHzTargetPowerHt20:
					ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_CalTGTpwrht20Set);
					break;
				case Ar9287SetEeprom2GHzTargetPowerHt40:
					ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_CalTGTpwrht40Set);
					break;
				case Ar9287SetEeprom2GHzCtlIndex:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_CtlIndexSet);
					break;
				case Ar9287SetEeprom2GHzCtlData:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_CtlDataSet);
					break;
				case Ar9287SetEeprom2GHzCtlFrequency:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_CtlChannelSet);
					break;
				case Ar9287SetEeprom2GHzCtlPower:
					ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_CtlPowerSet);
					break;
				case Ar9287SetEeprom2GHzCtlBandEdge:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_CtlFlagSet);
					break;
				case Ar9287SetEeprom2GHzCaldataMemoryType:
					text=CommandParameterValue(ip,0);
					status=Ar9287_CaldataMemoryTypeSet(text);  
					if(status==VALUE_OK)
					{
						ReturnText("set",GetParameter[index].word[0],"",text);
						done++;
					}
					else	// ErrorPrint();
						error++;
					break;
				case Ar9287SetEeprom2GHzTxRxAtten:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_TxRxAttenChSet);
					break;
				case Ar9287SetEeprom2GHzRxTxMargin:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9287_RxTxMarginChSet);
					break;
				default:
					error++;
					ErrorPrint(ParseBadParameter,name);
					break;
			}
		}
		else
		{
			error++;
			ErrorPrint(ParseBadParameter,name);
		}
	}
	return 0;
}


//
// parse and then get a configuration parameter in the internal structure
//
AR9287DLLSPEC int Ar9287GetCommand(int client)
{
	int np, ip;
	char *name;	
	int error;
	int done;
    int ngot;
	int index;
    int code;
    unsigned char cmac[6];
    int value[MAXVALUE]; 
	u_int8_t  value2[AR9287_EEPROM_MODAL_SPURS][2];
	double fvalue[MAXVALUE]; 
	unsigned int uvalue[MAXVALUE]; 
	char cvalue[MAXVALUE];
	short svalue[MAXVALUE];
	long lvalue[MAXVALUE];
	char buffer[MBUFFER];
	int status;
	int ix, iy, iz, num=0;

	error=0;
	done=0;
	//
	// parse arguments and do it
	//
	np=CommandParameterMany();
	for(ip=0; ip<np; ip++)
	{
		name=CommandParameterName(ip);
		index=ParameterSelectIndexArray(name,&GetParameter[0],sizeof(GetParameter)/sizeof(GetParameter[0]), &ix, &iy, &iz);	
		if(index>=0)
		{
			code=GetParameter[index].code;
			buffer[0]=0;
			switch(code) 
			{
				//
				// special "get all" command
				//
				case Ar9287SetEepromALL:
					print9287Struct(client,0);
					done++;
					break;					
				//
			    // chip pcie initialization space
				//
				case Ar9287SetEepromDeviceId:
					status=Ar9287deviceIDGet(&value[0]);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEepromSSID:
					status=Ar9287SSIDGet(&value[0]);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEepromVID:
					status=Ar9287vendorGet(&value[0]);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEepromSVID:
					status=Ar9287SubVendorGet(&value[0]);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEepromConfig:
					ngot = 0;
					while (1) {
						status = Ar9287pcieAddressValueDataOfNumGet(ngot, &uvalue[ngot*2], &uvalue[ngot*2+1]);
						ngot++;
						if(status!=VALUE_OK)
							break;
					}
					if(ngot-1 > 0) {
						ReturnHex("get",GetParameter[index].word[0],"",uvalue,(ngot-1)*2);
						done++;
					} else
						error++;
					break;
#ifdef UNUSED
				case Ar9287SetEepromConfigPCIe:
					ngot=0;
					while (1) {
						status=ConfigPCIeOnBoard(ngot, &uvalue[ngot*2]);
						ngot++;
						if (status==ERR_MAX_REACHED)
							break;
					}
					if(ngot-1 > 0) {
						ReturnHex("get",GetParameter[index].word[0],"",uvalue,(ngot-1)*2);
						done++;
					} else
						 ErrorPrint(NartData,"Blank eep");
					break;	
#endif
				//
				// configuration and calibration structure
				//
				case Ar9287SetEepromVersion:
	                value[0]=Ar9287_EepromVersionGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEepromChecksum:
	                value[0]=Ar9287_ChecksumGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEepromTemplateVersion:
	                value[0]=Ar9287_TemplateVersionGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEepromBinBuildNumber:
	                lvalue[0]=Ar9287_CalibrationBinaryVersionGet();
					ReturnGetl(ix,iy,iz,VALUE_OK, &done, &error, ip, index, lvalue, 1);
					break;
				case Ar9287SetEepromMacAddress:
					status=Ar9287_MacAdressGet(cmac);
					if(status==VALUE_OK) 
					{
						status=PrintMacAddress(buffer,MBUFFER,cmac);
						ReturnText("get",GetParameter[index].word[0],"",buffer);
						done++;
					}
					else
					{	// ErrorPrint();
						error++;
					}
					break;
				case Ar9287SetEepromCustomerData:
					status=Ar9287_CustomerDataGet(buffer,20);
					if(status==VALUE_OK) 
					{
						ReturnText("get",GetParameter[index].word[0],"",buffer);
						done++;
					}
					else // ErrorPrint();
						error++;
					break;
				case Ar9287SetEepromRegulatoryDomain:
	                status=Ar9287_RegDmnGet(value, ix, &num);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEepromTxRxMask:
	                value[0]=Ar9287_TxRxMaskGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEepromTxRxMaskTx:
	                value[0]=Ar9287_TxMaskGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEepromTxRxMaskRx:
	                value[0]=Ar9287_RxMaskGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEepromOpFlags:
	                value[0]=Ar9287_OpFlagsGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEepromEepMisc:
	                value[0]=Ar9287_eepMiscGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEepromRfSilent:
	                value[0]=Ar9287_RFSilentGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEepromBlueToothOptions:
	                value[0]=Ar9287_BlueToothOptionsGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEepromDeviceCapability:
	                value[0]=Ar9287_DeviceCapGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEepromDeviceType:
	                value[0]=Ar9287_DeviceTypeGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEepromPowerTableOffset:
	                value[0]=Ar9287_PwrTableOffsetSetGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEepromFeatureEnableTxFrameToXpaOn:
	                value[0]=Ar9287_TxFrameToXpaOnGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzAntennaControlCommon:
					value[0]=Ar9287_AntCtrlCommonGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzAntennaControlChain:
	                status=Ar9287_AntCtrlChainGet(value, ix, &num);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzOpenLoopPowerControl:
	                value[0]=Ar9287_OpenLoopPwrCntlGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzTemperatureSlope:
	                value[0]=Ar9287_TempSensSlopeGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzTemperatureSlopePalOn:
					value[0]=Ar9287_TempSensSlopePalOnGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzFutureBase:
					status=Ar9287_FutureBaseGet(value, ix, &num);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzSpur:
					status=Ar9287_SpurChansGet(value, ix, 0, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzSpurRangeLow:
					status=Ar9287_SpurRangeLowGet(value, ix, 0, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzSpurRangeHigh:
					status=Ar9287_SpurRangeHighGet(value, ix, 0, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzNoiseFloorThreshold:
					status=Ar9287_NoiseFloorThreshChGet(value, ix, &num);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzXpdGain:
					value[0]=Ar9287_XpdGainGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzXpd:
					value[0]=Ar9287_XpdGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzIqCalICh:
	                status=Ar9287_IQCalIChGet(value, ix, &num);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzIqCalQCh:
	                status=Ar9287_IQCalQChGet(value, ix, &num);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzBswAtten:
					status=Ar9287_BswAttenGet(value, ix, &num);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzBswMargin:
					status=Ar9287_BswMarginGet(value, ix, &num);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzSwSettleHt40:
					value[0]=Ar9287_SwSettleHT40Get(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzModalHeaderversion:
					value[0]=Ar9287_ModalHeaderVersionGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzDb1:
					value[0]=Ar9287_db1Get(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzDb2:
					value[0]=Ar9287_db2Get(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzObCck:
					value[0]=Ar9287_ob_cckGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzObPsk:
					value[0]=Ar9287_ob_pskGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzObQam:
					value[0]=Ar9287_ob_qamGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzObPalOff:
					value[0]=Ar9287_ob_pal_offGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzFutureModal:
	                status=Ar9287_FutureModalGet(value, ix, &num);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzPadding:
					value[0]=Ar9287_PaddingGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzPdGainOverLap:
	                value[0]=Ar9287_PdGainOverlapGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzXpaBiasLevel:
	                value[0]=Ar9287_XpaBiasLvlGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzTxFrameToDataStart:
	                value[0]=Ar9287_TxFrameToDataStartGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzTxFrameToPaOn:
	                value[0]=Ar9287_TxFrameToPaOnGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzHt40PowerIncForPdadc:
	                value[0]=Ar9287_HT40PowerIncForPdadcGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzAntennaGain:
	                status=Ar9287_AntennaGainGet(value, ix, &num);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzSwitchSettling:
	                value[0]=Ar9287_SwitchSettlingGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzAdcSize:
	                value[0]=Ar9287_AdcDesiredSizeGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzTxEndToXpaOff:
	                value[0]=Ar9287_TxEndToXpaOffGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzTxEndToRxOn:
	                value[0]=Ar9287_TxEndToRxOnGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzTxFrameToXpaOn:
	                value[0]=Ar9287_TxFrameToXpaOnGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzThresh62:
	                value[0]=Ar9287_Thresh62Get(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzCalibrationFrequency:
					status=Ar9287_CalPierFreqGet(value, ix, 0, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzPowerCorrection:
					status=Ar9287_CalPierOpenPowerGet(value, ix, iy, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzCalibrationVoltage:
					status=Ar9287_CalPierOpenVoltMeasGet(value, ix, iy, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzCalibrationPcdac:
					status=Ar9287_CalPierOpenPcdacGet(value, ix, iy, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzTargetFrequencyCck:
					status=Ar9287_CalTGTpwrcckChannelGet(value, ix, 0, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzTargetFrequency:
					status=Ar9287_CalTGTpwrChannelGet(value, ix, 0, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzTargetFrequencyHt20:
					status=Ar9287_CalTGTpwrht20ChannelGet(value, ix, 0, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzTargetFrequencyHt40:
					status=Ar9287_CalTGTpwrht40ChannelGet(value, ix, 0, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzTargetPowerCck:
					status=Ar9287_CalTGTpwrcckGet(fvalue, ix, iy, 0, &num, band_BG);
					ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
					break;
				case Ar9287SetEeprom2GHzTargetPower:
					status=Ar9287_CalTGTpwrGet(fvalue, ix, iy, 0, &num, band_BG);
					ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
					break;
				case Ar9287SetEeprom2GHzTargetPowerHt20:
					status=Ar9287_CalTGTpwrht20Get(fvalue, ix, iy, 0, &num, band_BG);
					ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
					break;
				case Ar9287SetEeprom2GHzTargetPowerHt40:
					status=Ar9287_CalTGTpwrht40Get(fvalue, ix, iy, 0, &num, band_BG);
					ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
					break;
				case Ar9287SetEeprom2GHzCtlIndex:
					status=Ar9287_CtlIndexGet(value, ix, iy, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzCtlFrequency:
					status=Ar9287_CtlChannelGet(value, ix, iy, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzCtlPower:
					status=Ar9287_CtlPowerGet(fvalue, ix, iy, 0, &num, band_BG);
					ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
					break;
				case Ar9287SetEeprom2GHzCtlBandEdge:
					status=Ar9287_CtlFlagGet(value, ix, iy, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzCaldataMemoryType:
					status=Ar9287_CaldataMemoryTypeGet(buffer,20);
					if(status==VALUE_OK) 
					{
						ReturnText("get",GetParameter[index].word[0],"",buffer);
						done++;
					}
					else // ErrorPrint();
						error++;
					break;
				case Ar9287SetEeprom2GHzLengthEeprom:
	                value[0]=Ar9287_LengthGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9287SetEeprom2GHzTxRxAtten:
					status=Ar9287_TxRxAttenChGet(value, ix, &num);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9287SetEeprom2GHzRxTxMargin:
					status=Ar9287_RxTxMarginChGet(value, ix, &num);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;


				default:
					error++;
					ErrorPrint(ParseBadParameter,name);
					break;
			}
		}
		else
		{
			error++;
			ErrorPrint(ParseBadParameter,name);
		}
	}
	return 0;
}	

