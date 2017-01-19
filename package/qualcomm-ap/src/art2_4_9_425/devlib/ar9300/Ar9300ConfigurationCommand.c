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

#include "Ar9300EepromParameter.h"
#include "Ar9300SetList.h"


//
// hal header files
//
#include "ah.h"
#include "ah_internal.h"
#include "ar9300.h"
#include "ar9300eep.h"

#include "Ar9300Device.h"
#include "Ar9300EepromStructSet.h"
#include "mEepStruct9300.h"
#include "ar9300_EEPROM_print.h"
#include "Ar9300EepromSave.h"
#include "Ar9300PcieConfig.h"

//#include "templatelist.h"

#define MBUFFER 1024

#ifndef max
#define max(a,b)	(((a) > (b)) ? (a) : (b))
#endif

extern int Ar9300FlashCal(int value);

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
	AR9300_SET_EEPROM_ALL,

    AR9300_SET_EEPROM_VERSION,
	AR9300_SET_TEMPLATE_VERSION,
    AR9300_SET_MAC_ADDRESS,
    AR9300_SET_CUSTOMER_DATA,
    AR9300_SET_REGULATORY_DOMAIN,
    AR9300_SET_TX_RX_MASK,
		AR9300_SET_TX_RX_MASK_TX,
		AR9300_SET_TX_RX_MASK_RX,
    AR9300_SET_OP_FLAGS,
    AR9300_SET_EEP_MISC,
    AR9300_SET_RF_SILENT,
		AR9300_SET_RF_SILENT_B0,
		AR9300_SET_RF_SILENT_B1,
		AR9300_SET_RF_SILENT_GPIO,
    AR9300_SET_BLUETOOTH_OPTIONS,
    AR9300_SET_DEVICE_CAPABILITY,
	AR9300_SET_DEVICE_TYPE,
    AR9300_SET_POWER_TABLE_OFFSET,
    AR9300_SET_TUNING,
    AR9300_SET_FEATURE_ENABLE,
		AR9300_SET_FEATURE_ENABLE_TEMPERATURE_COMPENSATION, 
		AR9300_SET_FEATURE_ENABLE_VOLTAGE_COMPENSATION, 
		AR9300_SET_FEATURE_ENABLE_FAST_CLOCK, 
		AR9300_SET_FEATURE_ENABLE_DOUBLING, 
		AR9300_SET_FEATURE_ENABLE_INTERNAL_SWITCHING_REGULATOR, 
		AR9300_SET_FEATURE_ENABLE_PA_PREDISTORTION, 
		AR9300_SET_FEATURE_ENABLE_TUNING_CAPS,
		AR9300_SET_FEATURE_ENABLE_TX_FRAME_TO_XPA_ON,
    AR9300_SET_MISCELLANEOUS,
		AR9300_SET_MISCELLANEOUS_DRIVERS_STRENGTH,
		AR9300_SET_MISCELLANEOUS_THERMOMETER,
		AR9300_SET_MISCELLANEOUS_CHAIN_MASK_REDUCE,
		AR9300_SET_MISCELLANEOUS_QUICK_DROP_ENABLE,
    AR9300_SET_MISCELLANEOUS_TEMP_SLOP_EXTENSION_ENABLE,
		AR9300_SET_MISCELLANEOUS_XLNA_BIAS_STRENGTH_ENABLE,
		AR9300_SET_MISCELLANEOUS_RF_GAIN_CAP_ENABLE,
	AR9300_SET_MISC_ENABLE,
		AR9300_SET_MISC_ENABLE_TX_GAIN_CAP_ENABLE,
		AR9300_SET_MISC_ENABLE_MINCCA_PWR_THRESHOLD,
    AR9300_SET_EEPROM_WRITE_ENABLE_GPIO,
    AR9300_SET_WLAN_DISABLE_GPIO,
    AR9300_SET_WLAN_LED_GPIO,
    AR9300_SET_RX_BAND_SELECT_GPIO,
    AR9300_SET_TX_RX_GAIN,
	    AR9300_SET_TX_RX_GAIN_TX,
	    AR9300_SET_TX_RX_GAIN_RX,
    AR9300_SET_SWITCHING_REGULATOR,
	AR9300_SET_ANTA_DIV_CTL,
    AR9300_SET_FUTURE,

    AR9300_SET_2GHZ_ANTENNA_CONTROL_COMMON,
    AR9300_SET_2GHZ_ANTENNA_CONTROL_COMMON_2 ,
    AR9300_SET_2GHZ_ANTENNA_CONTROL_CHAIN,
    AR9300_SET_2GHZ_ATTENUATION_DB,
    AR9300_SET_2GHZ_ATTENUATION_MARGIN ,
    AR9300_SET_2GHZ_TEMPERATURE_SLOPE,
    AR9300_SET_2GHZ_VOLTAGE_SLOPE,
    AR9300_SET_2GHZ_SPUR,
    AR9300_SET_2GHZ_MINCCA_PWR_THRESHOLD,	
	AR9300_SET_2GHZ_RESERVED,
	AR9300_SET_2GHZ_QUICK_DROP,
    AR9300_SET_2GHZ_XPA_BIAS_LEVEL,
	AR9300_SET_2GHZ_TX_FRAME_TO_DATA_START,
	AR9300_SET_2GHZ_TX_FRAME_TO_PA_ON,
	AR9300_SET_2GHZ_TX_FRAME_TO_XPA_ON,
	AR9300_SET_2GHZ_TX_END_TO_XPA_OFF,
	AR9300_SET_2GHZ_TX_END_TO_RX_ON,
	AR9300_SET_2GHZ_TX_CLIP,
	AR9300_SET_2GHZ_DAC_SCALE_CCK,
	AR9300_SET_2GHZ_ANTENNA_GAIN,
	AR9300_SET_2GHZ_SWITCH_SETTLING,
	AR9300_SET_2GHZ_ADC_DESIRED_SIZE,
	AR9300_SET_2GHZ_THRESH62,
	AR9300_SET_2GHZ_PAPD_HT20,
	AR9300_SET_2GHZ_PAPD_HT40,
    AR9300_SET_2GHZ_WLAN_SPDT_SWITCH_GLOBAL_CONTROL,
    AR9300_SET_2GHZ_XLNA_BIAS_STRENGTH,
	AR9300_SET_2GHZ_RF_GAIN_CAP,
	AR9300_SET_2GHZ_TX_GAIN_CAP,
	AR9300_SET_2GHZ_FUTURE,
    AR9300_SET_2GHZ_CALIBRATION_FREQUENCY,
    AR9300_SET_2GHZ_POWER_CORRECTION,
    AR9300_SET_2GHZ_CALIBRATION_VOLTAGE,
    AR9300_SET_2GHZ_CALIBRATION_TEMPERATURE,
	AR9300_SET_2GHZ_NOISE_FLOOR,
    AR9300_SET_2GHZ_NOISE_FLOOR_POWER,
    AR9300_SET_2GHZ_NOISE_FLOOR_TEMPERATURE,
    AR9300_SET_2GHZ_TARGET_FREQUENCY_CCK,
    AR9300_SET_2GHZ_TARGET_POWER_CCK,
    AR9300_SET_2GHZ_TARGET_FREQUENCY,
    AR9300_SET_2GHZ_TARGET_POWER,
    AR9300_SET_2GHZ_TARGET_FREQUENCY_HT20,
    AR9300_SET_2GHZ_TARGET_POWER_HT20,
    AR9300_SET_2GHZ_TARGET_FREQUENCY_HT40,
    AR9300_SET_2GHZ_TARGET_POWER_HT40,
    AR9300_SET_2GHZ_CTL_INDEX,
	AR9300_SET_2GHZ_CTL_FREQUENCY,
    AR9300_SET_2GHZ_CTL_POWER,
	AR9300_SET_2GHZ_CTL_BANDEDGE,

    AR9300_SET_5GHZ_ANTENNA_CONTROL_COMMON,
    AR9300_SET_5GHZ_ANTENNA_CONTROL_COMMON_2 ,
    AR9300_SET_5GHZ_ANTENNA_CONTROL_CHAIN,
    AR9300_SET_5GHZ_ATTENUATION_DB_LOW,
    AR9300_SET_5GHZ_ATTENUATION_DB,
    AR9300_SET_5GHZ_ATTENUATION_DB_HIGH,
    AR9300_SET_5GHZ_ATTENUATION_MARGIN_LOW,
    AR9300_SET_5GHZ_ATTENUATION_MARGIN,
    AR9300_SET_5GHZ_ATTENUATION_MARGIN_HIGH,
    AR9300_SET_5GHZ_TEMPERATURE_SLOPE_LOW,
    AR9300_SET_5GHZ_TEMPERATURE_SLOPE,
    AR9300_SET_5GHZ_TEMPERATURE_SLOPE_HIGH,
    AR9300_SET_5GHZ_TEMPERATURE_SLOPE_EXTENSION,
    AR9300_SET_5GHZ_VOLTAGE_SLOPE,
    AR9300_SET_5GHZ_SPUR,
    AR9300_SET_5GHZ_MINCCA_PWR_THRESHOLD,
	AR9300_SET_5GHZ_RESERVED,
	AR9300_SET_5GHZ_QUICK_DROP_LOW,
	AR9300_SET_5GHZ_QUICK_DROP,
	AR9300_SET_5GHZ_QUICK_DROP_HIGH,
    AR9300_SET_5GHZ_XPA_BIAS_LEVEL,
	AR9300_SET_5GHZ_TX_FRAME_TO_DATA_START,
	AR9300_SET_5GHZ_TX_FRAME_TO_PA_ON,
	AR9300_SET_5GHZ_TX_FRAME_TO_XPA_ON,
	AR9300_SET_5GHZ_TX_END_TO_XPA_OFF,
	AR9300_SET_5GHZ_TX_END_TO_RX_ON,
	AR9300_SET_5GHZ_TX_CLIP,
	AR9300_SET_5GHZ_ANTENNA_GAIN,
	AR9300_SET_5GHZ_SWITCH_SETTLING,
	AR9300_SET_5GHZ_ADC_DESIRED_SIZE,
	AR9300_SET_5GHZ_THRESH62,
	AR9300_SET_5GHZ_PAPD_HT20,
	AR9300_SET_5GHZ_PAPD_HT40,
    AR9300_SET_5GHZ_WLAN_SPDT_SWITCH_GLOBAL_CONTROL,
    AR9300_SET_5GHZ_XLNA_BIAS_STRENGTH,
	AR9300_SET_5GHZ_RF_GAIN_CAP,
	AR9300_SET_5GHZ_TX_GAIN_CAP,
	AR9300_SET_5GHZ_FUTURE,
    AR9300_SET_5GHZ_CALIBRATION_FREQUENCY,
    AR9300_SET_5GHZ_POWER_CORRECTION,
    AR9300_SET_5GHZ_CALIBRATION_VOLTAGE,
    AR9300_SET_5GHZ_CALIBRATION_TEMPERATURE,
	AR9300_SET_5GHZ_NOISE_FLOOR,
    AR9300_SET_5GHZ_NOISE_FLOOR_POWER,
    AR9300_SET_5GHZ_NOISE_FLOOR_TEMPERATURE,
    AR9300_SET_5GHZ_TARGET_FREQUENCY,
    AR9300_SET_5GHZ_TARGET_POWER,
    AR9300_SET_5GHZ_TARGET_FREQUENCY_HT20,
    AR9300_SET_5GHZ_TARGET_POWER_HT20,
    AR9300_SET_5GHZ_TARGET_FREQUENCY_HT40,
    AR9300_SET_5GHZ_TARGET_POWER_HT40,
    AR9300_SET_5GHZ_CTL_INDEX,
	AR9300_SET_5GHZ_CTL_FREQUENCY,
    AR9300_SET_5GHZ_CTL_POWER,
	AR9300_SET_5GHZ_CTL_BANDEDGE,

	AR9300_SET_CONFIG,
	AR9300_SET_CONFIG_PCIE,
	AR9300_SET_EEPROM_DEVICEID,
	AR9300_SET_EEPROM_SSID,
	AR9300_SET_EEPROM_VID,
	AR9300_SET_EEPROM_SVID,

    AR9300_SET_CALDATA_MEMORY_TYPE,
};


AR9300DLLSPEC int Ar9300GetParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(GetParameter)/sizeof(GetParameter[0]);
    list->special=GetParameter;
	return 0;
}


AR9300DLLSPEC int Ar9300SetParameterSplice(struct _ParameterList *list)
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
					iz=0;
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
					iz=0;
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
AR9300DLLSPEC int Ar9300SetCommand(int client)
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
				case Ar9300SetEepromVersion:
					ConfigSet(&done, &error, ip, index, -1, Ar9300eepromVersion, 0);
					break;
				case Ar9300SetEepromTemplateVersion:
					ConfigSet(&done, &error, ip, index, -1, Ar9300templateVersion, 0);
					break;
				case Ar9300SetEepromDeviceId:
					ConfigSetUnsigned(&done, &error, ip, index, -1, Ar9300deviceIDSet, 0);
					break;
				case Ar9300SetEepromSSID:
					ConfigSetUnsigned(&done, &error, ip, index, -1, Ar9300SSIDSet, 0);
					break;
				case Ar9300SetEepromVID:
					ConfigSetUnsigned(&done, &error, ip, index, -1, Ar9300vendorSet, 0);
					break;
				case Ar9300SetEepromSVID:
					ConfigSetUnsigned(&done, &error, ip, index, -1, Ar9300SubVendorSet, 0);
					break;
				case Ar9300SetEepromConfig:
					// set config=32bitHexValue; address=16bitHexAddress; 
					parseStatus = ParseHex(ip, name, 1, &lvalue);
					if (parseStatus==1) {
						parseStatus = ParseHex(++ip, name, 1, &addr);
						if (parseStatus==1) {
							status=Ar9300pcieAddressValueDataSet(addr, lvalue);
							SformatOutput(buffer, MBUFFER-1, "%s(0x%x) 0x%x", name, addr, lvalue);
							ReturnText("set",GetParameter[index].word[0],"",buffer);
						}
					} 
					break;
				case Ar9300SetEepromConfigPCIe:
					break;
				case Ar9300SetEepromMacAddress:
					text=CommandParameterValue(ip,0);
					status=ParseMacAddress(text,cmac);
					if (status == VALUE_OK) 
					{
						status=Ar9300MacAddressSet(cmac);
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
				case Ar9300SetEepromCustomerData:
					text=CommandParameterValue(ip,0);
					status=Ar9300CustomerDataSet(text, 20);  
					if(status==VALUE_OK)
					{
						ReturnText("set",GetParameter[index].word[0],"",text);
						done++;
					}
					else	// ErrorPrint();
						error++;
					break;
				case Ar9300SetEepromRegulatoryDomain:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, -1, Ar9300regDmnSet);
					break;
				case Ar9300SetEepromTxRxMask:
					ErrorPrint(NartData,"Please use set Mask.Tx/Mask.Rx");
					break;
				case Ar9300SetEepromTxRxMaskTx:
					ConfigSet(&done, &error, ip, index, -1, Ar9300txMaskSet, 0);
					break;
				case Ar9300SetEepromTxRxMaskRx:
					ConfigSet(&done, &error, ip, index, -1, Ar9300rxMaskSet, 0);
					break;
				case Ar9300SetEepromOpFlags:
					ConfigSet(&done, &error, ip, index, -1, Ar9300opFlagsSet, 0);
					break;
				case Ar9300SetEepromEepMisc:
					ConfigSet(&done, &error, ip, index, -1, Ar9300eepMiscSet, 0);
					break;
				case Ar9300SetEepromRfSilent:
					ConfigSet(&done, &error, ip, index, -1, Ar9300rfSilentSet, 0);
					break;
				case Ar9300SetEepromRfSilentB0:
					ConfigSet(&done, &error, ip, index, -1, Ar9300rfSilentB0Set, 0);
					break;
				case Ar9300SetEepromRfSilentB1:
					ConfigSet(&done, &error, ip, index, -1, Ar9300rfSilentB1Set, 0);
					break;
				case Ar9300SetEepromRfSilentGpio:
					ConfigSet(&done, &error, ip, index, -1, Ar9300rfSilentGPIOSet, 0);
					break;
				case Ar9300SetEepromBlueToothOptions:
					ConfigSet(&done, &error, ip, index, -1, Ar9300blueToothOptionsSet, 0);
					break;
				case Ar9300SetEepromDeviceCapability:
					ConfigSet(&done, &error, ip, index, -1, Ar9300deviceCapSet, 0);
					break;
				case Ar9300SetEepromDeviceType:
					ConfigSet(&done, &error, ip, index, -1, Ar9300deviceTypetSet, 0);
					break;
				case Ar9300SetEepromPowerTableOffset:
					ConfigSet(&done, &error, ip, index, -1, Ar9300pwrTableOffsetSet, 0);
					break;
				case Ar9300SetEepromTuningCaps:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, -1, Ar9300pwrTuningCapsParamsSet);
					break;
				case Ar9300SetEepromFeatureEnable:
					ConfigSet(&done, &error, ip, index, -1, Ar9300EnableFeatureSet, 0);
					break;
				case Ar9300SetEepromFeatureEnableTemperatureCompensation:
					ConfigSet(&done, &error, ip, index, -1, Ar9300EnableTempCompensationSet, 0);
					break;
				case Ar9300SetEepromFeatureEnableVoltageCompensation:
					ConfigSet(&done, &error, ip, index, -1, Ar9300EnableVoltCompensationSet, 0);
					break;
				case Ar9300SetEepromFeatureEnableFastClock:
					ConfigSet(&done, &error, ip, index, -1, Ar9300EnableFastClockSet, 0);
					break;
				case Ar9300SetEepromFeatureEnableDoubling:
					ConfigSet(&done, &error, ip, index, -1, Ar9300EnableDoublingSet, 0);
					break;
				case Ar9300SetEepromFeatureEnableInternalSwitchingRegulator:
					ConfigSet(&done, &error, ip, index, -1, Ar9300InternalRegulatorSet, 0);
					break;
				case Ar9300SetEepromFeatureEnablePaPredistortion:
					ConfigSet(&done, &error, ip, index, -1, Ar9300PapdSet, 0);
					break;
				case Ar9300SetEepromFeatureEnableTuningCaps:
					ConfigSet(&done, &error, ip, index, -1, Ar9300EnableTuningCapsSet, 0);
					break;
				case Ar9300SetEepromFeatureEnableTxFrameToXpaOn:
					ConfigSet(&done, &error, ip, index, -1, Ar9300EnableTxFrameToXpaOnSet, 0);
					break;
				case Ar9300SetEepromMiscellaneous:
					ConfigSet(&done, &error, ip, index, -1, Ar9300MiscConfigurationSet, 0);
					break;
				case Ar9300SetEepromMiscellaneousDriveStrength:
					ConfigSet(&done, &error, ip, index, -1, Ar9300ReconfigDriveStrengthSet, 0);
					break;
				case Ar9300SetEepromMiscellaneousThermometer:
					ConfigSet(&done, &error, ip, index, -1, Ar9300ThermometerSet, 0);
					break;
				case Ar9300SetEepromMiscellaneousChainMaskReduce:
					ConfigSet(&done, &error, ip, index, -1, Ar9300ChainMaskReduceSet, 0);
					break;
				case Ar9300SetEepromMiscellaneousQuickDropEnable:
					ConfigSet(&done, &error, ip, index, -1, Ar9300ReconfigQuickDropSet, 0);
					break;
				case Ar9300SetEepromMiscellaneousTempSlopExtensionEnable:
					ConfigSet(&done, &error, ip, index, -1, Ar9300ReconfigTempSlopExtensionSet, 0);
					break;
				case Ar9300SetEepromMiscellaneousXLNABiasStrengthEnable:
					ConfigSet(&done, &error, ip, index, -1, Ar9300EnableXLNABiasStrengthSet, 0);
					break;
				case Ar9300SetEepromMiscellaneousRFGainCAPEnable:
					ConfigSet(&done, &error, ip, index, -1, Ar9300EnableRFGainCAPSet, 0);
					break;
				case Ar9300SetEepromMiscEnableTXGainCAPEnable:
					ConfigSet(&done, &error, ip, index, -1, Ar9300EnableTXGainCAPSet, 0);
					break;
				case Ar9300SetEepromMiscEnableMinCCAPwrthresholdEnable:
                                        ConfigSet(&done, &error, ip, index, -1, Ar9300EnableMinCCAPwrThresholdSet, 0);
                                        break;
				case Ar9300SetEepromEepromWriteEnableGpio:
					ConfigSet(&done, &error, ip, index, -1, Ar9300EepromWriteEnableGpioSet, 0);
					break;
				case Ar9300SetEepromWlanDisableGpio:
					ConfigSet(&done, &error, ip, index, -1, Ar9300WlanDisableGpioSet, 0);
					break;
				case Ar9300SetEepromWlanLedGpio:
					ConfigSet(&done, &error, ip, index, -1, Ar9300WlanLedGpioSet, 0);
					break;
				case Ar9300SetEepromRxBandSelectGpio:
					ConfigSet(&done, &error, ip, index, -1, Ar9300RxBandSelectGpioSet, 0);
					break;
				case Ar9300SetEepromTxRxGain:
					break;
				case Ar9300SetEepromTxRxGainTx:
					ConfigSet(&done, &error, ip, index, -1, Ar9300TxGainSet, 0);
					break;
				case Ar9300SetEepromTxRxGainRx:
					ConfigSet(&done, &error, ip, index, -1, Ar9300RxGainSet, 0);
					break;
				case Ar9300SetEepromSWREGProgram:
					ConfigSet(&done, &error, ip, index, -1, Ar9300_SWREG_PROGRAM_Set, 0);
					break;
				case Ar9300SetEepromSwitchingRegulator:
					ConfigSet(&done, &error, ip, index, -1, Ar9300_SWREG_Set, 0);
					break;
				case Ar9300SetEeprom2GHzAntennaControlCommon:
					ConfigSet(&done, &error, ip, index, band_BG, 0, Ar9300AntCtrlCommonSet);
					break;
				case Ar9300SetEeprom2GHzAntennaControlCommon2:
					ConfigSet(&done, &error, ip, index, band_BG, 0, Ar9300AntCtrlCommon2Set);
					break;
				case Ar9300SetEeprom2GHzAntennaControlChain:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300antCtrlChainSet);
					break;
				case Ar9300SetEeprom2GHzAttenuationDb:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300xatten1DBSet);
					break;
				case Ar9300SetEeprom2GHzAttenuationMargin:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300xatten1MarginSet);
					break;
				case Ar9300SetEeprom2GHzTemperatureSlope:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300TempSlopeSet);
					break;
				case Ar9300SetEeprom2GHzVoltageSlope:
					ConfigSet(&done, &error, ip, index, band_BG, 0, Ar9300VoltSlopeSet);
					break;
				case Ar9300SetEeprom2GHzSpur:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300spurChansSet);
					break;
				case Ar9300SetEeprom2GHzMinCCAPwrThreshold:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300MinCCAPwrThreshChSet);
					break;
				case Ar9300SetEeprom2GHzReserved:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300ReservedSet);
					break;
				case Ar9300SetEeprom2GHzQuickDrop:
					ConfigSet(&done, &error, ip, index, band_BG, 0, Ar9300QuickDropSet);
					break;
				case Ar9300SetEeprom2GHzXpaBiasLevel:
					ConfigSet(&done, &error, ip, index, band_BG, 0, Ar9300xpaBiasLvlSet);
					break;
				case Ar9300SetEeprom2GHzTxFrameToDataStart:
					ConfigSet(&done, &error, ip, index, band_BG, 0, Ar9300txFrameToDataStartSet);
					break;
				case Ar9300SetEeprom2GHzTxFrameToPaOn:
					ConfigSet(&done, &error, ip, index, band_BG, 0, Ar9300txFrameToPaOnSet);
					break;
				case Ar9300SetEeprom2GHzTxClip:
					ConfigSet(&done, &error, ip, index, band_BG, 0, Ar9300txClipSet);
					break;
				case Ar9300SetEeprom2GHzDacScaleCCK:
					ConfigSet(&done, &error, ip, index, band_BG, 0, Ar9300dac_scale_cckSet);
					break;
				case Ar9300SetEeprom2GHzAntennaGain:
					ConfigSet(&done, &error, ip, index, band_BG, 0, Ar9300antennaGainSet);
					break;
				case Ar9300SetEeprom2GHzSwitchSettling:
					ConfigSet(&done, &error, ip, index, band_BG, 0, Ar9300switchSettlingSet);
					break;
				case Ar9300SetEeprom2GHzAdcSize:
					ConfigSet(&done, &error, ip, index, band_BG, 0, Ar9300adcDesiredSizeSet);
					break;
				case Ar9300SetEeprom2GHzTxEndToXpaOff:
					ConfigSet(&done, &error, ip, index, band_BG, 0, Ar9300txEndToXpaOffSet);
					break;
				case Ar9300SetEeprom2GHzTxEndToRxOn:
					ConfigSet(&done, &error, ip, index, band_BG, 0, Ar9300txEndToRxOnSet);
					break;
				case Ar9300SetEeprom2GHzTxFrameToXpaOn:
					ConfigSet(&done, &error, ip, index, band_BG, 0, Ar9300txFrameToXpaOnSet);
					break;
				case Ar9300SetEeprom2GHzThresh62:
					ConfigSet(&done, &error, ip, index, band_BG, 0, Ar9300thresh62Set);
					break;
				case Ar9300SetEeprom2GHzPaPredistortionHt20:
					ConfigSet(&done, &error, ip, index, band_BG, 0, Ar9300PapdRateMaskHt20Set);
					break;
				case Ar9300SetEeprom2GHzPaPredistortionHt40:
					ConfigSet(&done, &error, ip, index, band_BG, 0, Ar9300PapdRateMaskHt40Set);
					break;
                case Ar9300SetEeprom2GHzWlanSpdtSwitchGlobalControl:
                    ConfigSet(&done, &error, ip, index, band_BG, 0, Ar9300WlanSpdtSwitchGlobalControlSet);
                    break;
                case Ar9300SetEeprom2GHzXLNABiasStrength:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300XLANBiasStrengthSet);
                    break;
				case Ar9300SetEeprom2GHzRFGainCAP:
					ConfigSet(&done, &error, ip, index, band_BG, 0, Ar9300RFGainCAPSet);
                    break;
				case Ar9300SetEeprom2GHzTXGainCAP:
					ConfigSet(&done, &error, ip, index, band_BG, 0, Ar9300TXGainCAPSet);
                    break;
				case Ar9300SetEeprom2GHzFuture:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300FutureSet);
					break;
				case Ar9300SetEepromFuture:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, -1, Ar9300FutureSet);
					break;
				case Ar9300SetEepromAntennaDiversityControl:
					ConfigSet(&done, &error, ip, index, -1, Ar9300AntDivCtrlSet, 0);
					break;
				case Ar9300SetEeprom2GHzCalibrationFrequency:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300calFreqPierSet);
					break;
				case Ar9300SetEeprom2GHzPowerCorrection:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300calPierDataRefPowerSet);
					break;
				case Ar9300SetEeprom2GHzCalibrationVoltage:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300calPierDataVoltMeasSet);
					break;
				case Ar9300SetEeprom2GHzCalibrationTemperature:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300calPierDataTempMeasSet);
					break;
				case Ar9300SetEeprom2GHzNoiseFloor:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300calPierDataRxNoisefloorCalSet);
					break;
				case Ar9300SetEeprom2GHzNoiseFloorPower:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300calPierDataRxNoisefloorPowerSet);
					break;
				case Ar9300SetEeprom2GHzNoiseFloorTemperature:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300calPierDataRxTempMeaSet);
					break;
				case Ar9300SetEeprom2GHzTargetFrequencyCck:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300calFreqTGTcckSet);
					break;
				case Ar9300SetEeprom2GHzTargetFrequency:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300calFreqTGTLegacyOFDMSet);
					break;
				case Ar9300SetEeprom2GHzTargetFrequencyHt20:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300calFreqTGTHT20Set);
					break;
				case Ar9300SetEeprom2GHzTargetFrequencyHt40:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300calFreqTGTHT40Set);
					break;
				case Ar9300SetEeprom2GHzTargetPowerCck:
					ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300calTGTPwrCCKSet);
					break;
				case Ar9300SetEeprom2GHzTargetPower:
					ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300calTGTPwrLegacyOFDMSet);
					break;
				case Ar9300SetEeprom2GHzTargetPowerHt20:
					ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300calTGTPwrHT20Set);
					break;
				case Ar9300SetEeprom2GHzTargetPowerHt40:
					ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300calTGTPwrHT40Set);
					break;
				case Ar9300SetEeprom2GHzCtlIndex:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300ctlIndexSet);
					break;
				case Ar9300SetEeprom2GHzCtlFrequency:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300ctlFreqSet);
					break;
				case Ar9300SetEeprom2GHzCtlPower:
					ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300ctlPowerSet);
					break;
				case Ar9300SetEeprom2GHzCtlBandEdge:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Ar9300ctlFlagSet);
					break;

				case Ar9300SetEeprom5GHzAntennaControlCommon:
					ConfigSet(&done, &error, ip, index, band_A, 0, Ar9300AntCtrlCommonSet);
					break;
				case Ar9300SetEeprom5GHzAntennaControlCommon2:
					ConfigSet(&done, &error, ip, index, band_A, 0, Ar9300AntCtrlCommon2Set);
					break;
				case Ar9300SetEeprom5GHzAntennaControlChain:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300antCtrlChainSet);
					break;
				case Ar9300SetEeprom5GHzAttenuationDb:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300xatten1DBSet);
					break;
				case Ar9300SetEeprom5GHzAttenuationDbLow:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300xatten1DBLowSet);
					break;
				case Ar9300SetEeprom5GHzAttenuationDbHigh:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300xatten1DBHighSet);
					break;
				case Ar9300SetEeprom5GHzAttenuationMargin:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300xatten1MarginSet);
					break;
				case Ar9300SetEeprom5GHzAttenuationMarginLow:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300xatten1MarginLowSet);
					break;
				case Ar9300SetEeprom5GHzAttenuationMarginHigh:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300xatten1MarginHighSet);
					break;
				case Ar9300SetEeprom5GHzTemperatureSlope:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300TempSlopeSet);
					break;
				case Ar9300SetEeprom5GHzTemperatureSlopeLow:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, -1, Ar9300TempSlopeLowSet);
					break;
				case Ar9300SetEeprom5GHzTemperatureSlopeHigh:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, -1, Ar9300TempSlopeHighSet);
					break;
				case Ar9300SetEeprom5GHzTemperatureSlopeExtension:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300TempSlopeExtensionSet);
					break;
				case Ar9300SetEeprom5GHzVoltageSlope:
					ConfigSet(&done, &error, ip, index, band_A, 0, Ar9300VoltSlopeSet);
					break;
				case Ar9300SetEeprom5GHzSpur:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300spurChansSet);
					break;
				case Ar9300SetEeprom5GHzMinCCAPwrThreshold:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300MinCCAPwrThreshChSet);
					break;
				case Ar9300SetEeprom5GHzReserved:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300ReservedSet);
					break;
				case Ar9300SetEeprom5GHzQuickDrop:
					ConfigSet(&done, &error, ip, index, band_A, 0, Ar9300QuickDropSet);
					break;
				case Ar9300SetEeprom5GHzQuickDropLow:
					ConfigSet(&done, &error, ip, index, -1, Ar9300QuickDropLowSet, 0);
					break;
				case Ar9300SetEeprom5GHzQuickDropHigh:
					ConfigSet(&done, &error, ip, index, -1, Ar9300QuickDropHighSet, 0);
					break;
				case Ar9300SetEeprom5GHzXpaBiasLevel:
					ConfigSet(&done, &error, ip, index, band_A, 0, Ar9300xpaBiasLvlSet);
					break;
				case Ar9300SetEeprom5GHzTxFrameToDataStart:
					ConfigSet(&done, &error, ip, index, band_A, 0, Ar9300txFrameToDataStartSet);
					break;
				case Ar9300SetEeprom5GHzTxFrameToPaOn:
					ConfigSet(&done, &error, ip, index, band_A, 0, Ar9300txFrameToPaOnSet);
					break;
				case Ar9300SetEeprom5GHzTxClip:
					ConfigSet(&done, &error, ip, index, band_A, 0, Ar9300txClipSet);
					break;
				case Ar9300SetEeprom5GHzAntennaGain:
					ConfigSet(&done, &error, ip, index, band_A, 0, Ar9300antennaGainSet);
					break;
				case Ar9300SetEeprom5GHzSwitchSettling:
					ConfigSet(&done, &error, ip, index, band_A, 0, Ar9300switchSettlingSet);
					break;
				case Ar9300SetEeprom5GHzAdcSize:
					ConfigSet(&done, &error, ip, index, band_A, 0, Ar9300adcDesiredSizeSet);
					break;
				case Ar9300SetEeprom5GHzTxEndToXpaOff:
					ConfigSet(&done, &error, ip, index, band_A, 0, Ar9300txEndToXpaOffSet);
					break;
				case Ar9300SetEeprom5GHzTxEndToRxOn:
					ConfigSet(&done, &error, ip, index, band_A, 0, Ar9300txEndToRxOnSet);
					break;
				case Ar9300SetEeprom5GHzTxFrameToXpaOn:
					ConfigSet(&done, &error, ip, index, band_A, 0, Ar9300txFrameToXpaOnSet);
					break;
				case Ar9300SetEeprom5GHzThresh62:
					ConfigSet(&done, &error, ip, index, band_A, 0, Ar9300thresh62Set);
					break;
				case Ar9300SetEeprom5GHzPaPredistortionHt20:
					ConfigSet(&done, &error, ip, index, band_A, 0, Ar9300PapdRateMaskHt20Set);
					break;
				case Ar9300SetEeprom5GHzPaPredistortionHt40:
					ConfigSet(&done, &error, ip, index, band_A, 0, Ar9300PapdRateMaskHt40Set);
					break;
                case Ar9300SetEeprom5GHzWlanSpdtSwitchGlobalControl:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Ar9300WlanSpdtSwitchGlobalControlSet);
                    break;
                case Ar9300SetEeprom5GHzXLNABiasStrength:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300XLANBiasStrengthSet);
                    break;
				case Ar9300SetEeprom5GHzRFGainCAP:
					ConfigSet(&done, &error, ip, index, band_A, 0, Ar9300RFGainCAPSet);
                    break;
				case Ar9300SetEeprom5GHzTXGainCAP:
					ConfigSet(&done, &error, ip, index, band_A, 0, Ar9300TXGainCAPSet);
                    break;
				case Ar9300SetEeprom5GHzFuture:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300FutureSet);
					break;
				case Ar9300SetEeprom5GHzCalibrationFrequency:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300calFreqPierSet);
					break;
				case Ar9300SetEeprom5GHzPowerCorrection:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300calPierDataRefPowerSet);
					break;
				case Ar9300SetEeprom5GHzCalibrationVoltage:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300calPierDataVoltMeasSet);
					break;
				case Ar9300SetEeprom5GHzCalibrationTemperature:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300calPierDataTempMeasSet);
					break;
				case Ar9300SetEeprom5GHzNoiseFloor:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300calPierDataRxNoisefloorCalSet);
					break;
				case Ar9300SetEeprom5GHzNoiseFloorPower:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300calPierDataRxNoisefloorPowerSet);
					break;
				case Ar9300SetEeprom5GHzNoiseFloorTemperature:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300calPierDataRxTempMeaSet);
					break;
				case Ar9300SetEeprom5GHzTargetFrequency:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300calFreqTGTLegacyOFDMSet);
					break;
				case Ar9300SetEeprom5GHzTargetFrequencyHt20:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300calFreqTGTHT20Set);
					break;
				case Ar9300SetEeprom5GHzTargetFrequencyHt40:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300calFreqTGTHT40Set);
					break;
				case Ar9300SetEeprom5GHzTargetPower:
					ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300calTGTPwrLegacyOFDMSet);
					break;
				case Ar9300SetEeprom5GHzTargetPowerHt20:
					ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300calTGTPwrHT20Set);
					break;
				case Ar9300SetEeprom5GHzTargetPowerHt40:
					ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300calTGTPwrHT40Set);
					break;
				case Ar9300SetEeprom5GHzCtlIndex:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300ctlIndexSet);
					break;
				case Ar9300SetEeprom5GHzCtlFrequency:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300ctlFreqSet);
					break;
				case Ar9300SetEeprom5GHzCtlPower:
					ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300ctlPowerSet);
					break;
				case Ar9300SetEeprom5GHzCtlBandEdge:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Ar9300ctlFlagSet);
					break;
				case Ar9300SetCaldataMemoryType:
					text=CommandParameterValue(ip,0);
					status=Ar9300CaldataMemoryTypeSet(text);  
					if(status==VALUE_OK)
					{
						ReturnText("set",GetParameter[index].word[0],"",text);
						done++;
					}
					else	// ErrorPrint();
						error++;
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
AR9300DLLSPEC int Ar9300GetCommand(int client)
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
	double fvalue[MAXVALUE]; 
	unsigned int uvalue[MAXVALUE]; 
	char cvalue[MAXVALUE];
	short svalue[MAXVALUE];
	char buffer[MBUFFER];
	int status;
	int ix, iy, iz, num;

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
				case Ar9300SetEepromALL:
					print9300Struct(client,0);
					done++;
					break;					
				//
			    // chip pcie initialization space
				//
				case Ar9300SetEepromDeviceId:
					status=Ar9300deviceIDGet(&value[0]);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromSSID:
					status=Ar9300SSIDGet(&value[0]);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromVID:
					status=Ar9300vendorGet(&value[0]);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromSVID:
					status=Ar9300SubVendorGet(&value[0]);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromConfig:
					ngot = 0;
					while (1) {
						status = Ar9300pcieAddressValueDataOfNumGet(ngot, &uvalue[ngot*2], &uvalue[ngot*2+1]);
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
				case Ar9300SetEepromConfigPCIe:
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
				case Ar9300SetEepromVersion:
	                value[0]=Ar9300eepromVersionGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromTemplateVersion:
	                value[0]=Ar9300templateVersionGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromMacAddress:
					status=Ar9300MacAddressGet(cmac);
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
				case Ar9300SetEepromCustomerData:
					status=Ar9300CustomerDataGet(buffer,20);
					if(status==VALUE_OK) 
					{
						ReturnText("get",GetParameter[index].word[0],"",buffer);
						done++;
					}
					else // ErrorPrint();
						error++;
					break;
				case Ar9300SetEepromRegulatoryDomain:
	                status=Ar9300regDmnGet(value, ix, &num);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEepromTxRxMask:
	                value[0]=Ar9300txrxMaskGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromTxRxMaskTx:
	                value[0]=Ar9300txMaskGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromTxRxMaskRx:
	                value[0]=Ar9300rxMaskGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromOpFlags:
	                value[0]=Ar9300opFlagsGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromEepMisc:
	                value[0]=Ar9300eepMiscGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromRfSilent:
	                value[0]=Ar9300rfSilentGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromRfSilentB0:
	                value[0]=Ar9300rfSilentB0Get();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromRfSilentB1:
	                value[0]=Ar9300rfSilentB1Get();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromRfSilentGpio:
	                value[0]=Ar9300rfSilentGPIOGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromBlueToothOptions:
	                value[0]=Ar9300blueToothOptionsGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromDeviceCapability:
	                value[0]=Ar9300deviceCapGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromDeviceType:
	                value[0]=Ar9300deviceTypeGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromPowerTableOffset:
	                value[0]=Ar9300pwrTableOffsetGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromTuningCaps:
					status=Ar9300pwrTuningCapsParamsGet(value, ix, &num);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEepromFeatureEnable:
	                value[0]=Ar9300EnableFeatureGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromFeatureEnableTemperatureCompensation:
	                value[0]=Ar9300EnableTempCompensationGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromFeatureEnableVoltageCompensation:
	                value[0]=Ar9300EnableVoltCompensationGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromFeatureEnableFastClock:
	                value[0]=Ar9300EnableFastClockGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromFeatureEnableDoubling:
	                value[0]=Ar9300EnableDoublingGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromFeatureEnableInternalSwitchingRegulator:
	                value[0]=Ar9300InternalRegulatorGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromFeatureEnablePaPredistortion:
					value[0]=Ar9300PapdGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromFeatureEnableTuningCaps:
	                value[0]=Ar9300EnableTuningCapsGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromFeatureEnableTxFrameToXpaOn:
	                value[0]=Ar9300EnableTxFrameToXpaOnGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromMiscellaneous:
					value[0]=Ar9300ReconfigMiscGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromMiscellaneousDriveStrength:
	                value[0]=Ar9300ReconfigDriveStrengthGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromMiscellaneousThermometer:
					value[0]=Ar9300ThermometerGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromMiscellaneousChainMaskReduce:
					value[0]=Ar9300ChainMaskReduceGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromMiscellaneousQuickDropEnable:
					value[0]=Ar9300ReconfigQuickDropGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromMiscellaneousTempSlopExtensionEnable:
					value[0]=Ar9300Reconfig8TempGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromMiscellaneousXLNABiasStrengthEnable:
	                value[0]=Ar9300EnableXLNABiasStrengthGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromMiscellaneousRFGainCAPEnable:
	                value[0]=Ar9300EnableRFGainCAPGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromMiscEnableTXGainCAPEnable:
	                value[0]=Ar9300EnableTXGainCAPGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				 case Ar9300SetEepromMiscEnableMinCCAPwrthresholdEnable:
                        value[0]=Ar9300EnableMinCCAPwrThresholdGet();
                                        ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                                        break;
				case Ar9300SetEepromEepromWriteEnableGpio:
					value[0]=Ar9300EepromWriteEnableGpioGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromWlanDisableGpio:
					value[0]=Ar9300WlanDisableGpioGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index,  value, 1);
					break;
				case Ar9300SetEepromWlanLedGpio:
					value[0]=Ar9300WlanLedGpioGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromRxBandSelectGpio:
					value[0]=Ar9300RxBandSelectGpioGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromTxRxGain:
					break;
				case Ar9300SetEepromTxRxGainTx:
					value[0]=Ar9300TxGainGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromTxRxGainRx:
					value[0]=Ar9300RxGainGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEepromSWREGProgram:
					break;
				case Ar9300SetEepromSwitchingRegulator:
					value[0]=Ar9300_SWREG_Get();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzAntennaControlCommon:
					value[0]=Ar9300AntCtrlCommonGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzAntennaControlCommon2:
					value[0]=Ar9300AntCtrlCommon2Get(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzAntennaControlChain:
					status=Ar9300antCtrlChainGet(value, ix, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom2GHzAttenuationDb:
					status=Ar9300xatten1DBGet(value, ix, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom2GHzAttenuationMargin:
					status=Ar9300xatten1MarginGet(value, ix, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom2GHzTemperatureSlope:
	                		status=Ar9300TempSlopeGet(value, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, 3);
					break;
				case Ar9300SetEeprom2GHzVoltageSlope:
	                		value[0]=Ar9300VoltSlopeGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzSpur:
					status=Ar9300spurChansGet(value, ix, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom2GHzMinCCAPwrThreshold:
					status=Ar9300MinCCAPwrThreshChGet(value, ix, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom2GHzReserved:
					status=Ar9300ReservedGet(value, ix, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom2GHzQuickDrop:
	                value[0]=Ar9300QuickDropGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzXpaBiasLevel:
	                value[0]=Ar9300xpaBiasLvlGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzTxFrameToDataStart:
	                value[0]=Ar9300txFrameToDataStartGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzTxFrameToPaOn:
	                value[0]=Ar9300txFrameToPaOnGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzTxClip:
	                value[0]=Ar9300txClipGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzDacScaleCCK:
	                value[0]=Ar9300dac_scale_cckGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzAntennaGain:
	                value[0]=Ar9300antennaGainGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzSwitchSettling:
	                value[0]=Ar9300switchSettlingGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzAdcSize:
	                value[0]=Ar9300adcDesiredSizeGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzTxEndToXpaOff:
	                value[0]=Ar9300txEndToXpaOffGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzTxEndToRxOn:
	                value[0]=Ar9300txEndToRxOnGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzTxFrameToXpaOn:
	                value[0]=Ar9300txFrameToXpaOnGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzThresh62:
	                value[0]=Ar9300thresh62Get(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzPaPredistortionHt20:
	                value[0]=Ar9300PapdRateMaskHt20Get(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzPaPredistortionHt40:
	                value[0]=Ar9300PapdRateMaskHt40Get(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzWlanSpdtSwitchGlobalControl:
					value[0]=Ar9300WlanSpdtSwitchGlobalControlGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzXLNABiasStrength:
					status=Ar9300XLANBiasStrengthGet(value, ix, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom2GHzRFGainCAP:
	                value[0]=Ar9300RFGainCAPGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzTXGainCAP:
	                value[0]=Ar9300TXGainCAPGet(band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzFuture:
					Ar9300FutureGet(value, ix, &num, band_BG);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEepromFuture:
					Ar9300FutureGet(value, ix, &num, -1);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEepromAntennaDiversityControl:
					value[0]=Ar9300AntDivCtrlGet();
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom2GHzCalibrationFrequency:
					status=Ar9300calFreqPierGet(value, ix, 0, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom2GHzPowerCorrection:
					status=Ar9300calPierDataRefPowerGet(value, ix, iy, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom2GHzCalibrationVoltage:
					status=Ar9300calPierDataVoltMeasGet(value, ix, iy, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom2GHzCalibrationTemperature:
					status=Ar9300calPierDataTempMeasGet(value, ix, iy, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom2GHzNoiseFloor:
					status=Ar9300calPierDataRxNoisefloorCalGet(value, ix, iy, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom2GHzNoiseFloorPower:
					status=Ar9300calPierDataRxNoisefloorPowerGet(value, ix, iy, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom2GHzNoiseFloorTemperature:
					status=Ar9300calPierDataRxTempMeasGet(value, ix, iy, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom2GHzTargetFrequencyCck:
					status=Ar9300calFreqTGTcckGet(value, ix, 0, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom2GHzTargetFrequency:
					status=Ar9300calFreqTGTLegacyOFDMGet(value, ix, 0, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom2GHzTargetFrequencyHt20:
					status=Ar9300calFreqTGTHT20Get(value, ix, 0, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom2GHzTargetFrequencyHt40:
					status=Ar9300calFreqTGTHT40Get(value, ix, 0, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom2GHzTargetPowerCck:
					status=Ar9300calTGTPwrCCKGet(fvalue, ix, iy, 0, &num, band_BG);
					ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
					break;
				case Ar9300SetEeprom2GHzTargetPower:
					status=Ar9300calTGTPwrLegacyOFDMGet(fvalue, ix, iy, 0, &num, band_BG);
					ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
					break;
				case Ar9300SetEeprom2GHzTargetPowerHt20:
					status=Ar9300calTGTPwrHT20Get(fvalue, ix, iy, 0, &num, band_BG);
					ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
					break;
				case Ar9300SetEeprom2GHzTargetPowerHt40:
					status=Ar9300calTGTPwrHT40Get(fvalue, ix, iy, 0, &num, band_BG);
					ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
					break;
				case Ar9300SetEeprom2GHzCtlIndex:
					status=Ar9300ctlIndexGet(value, ix, iy, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom2GHzCtlFrequency:
					status=Ar9300ctlFreqGet(value, ix, iy, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom2GHzCtlPower:
					status=Ar9300ctlPowerGet(fvalue, ix, iy, 0, &num, band_BG);
					ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
					break;
				case Ar9300SetEeprom2GHzCtlBandEdge:
					status=Ar9300ctlFlagGet(value, ix, iy, 0, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;

				case Ar9300SetEeprom5GHzAntennaControlCommon:
					value[0]=Ar9300AntCtrlCommonGet(band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom5GHzAntennaControlCommon2:
					value[0]=Ar9300AntCtrlCommon2Get(band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom5GHzAntennaControlChain:
					status=Ar9300antCtrlChainGet(value, ix, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzAttenuationDb:
					status=Ar9300xatten1DBGet(value, ix, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzAttenuationDbLow:
					status=Ar9300xatten1DBLowGet(value, ix, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzAttenuationDbHigh:
					status=Ar9300xatten1DBHighGet(value, ix, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzAttenuationMargin:
					status=Ar9300xatten1MarginGet(value, ix, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzAttenuationMarginLow:
					status=Ar9300xatten1MarginLowGet(value, ix, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzAttenuationMarginHigh:
					status=Ar9300xatten1MarginHighGet(value, ix, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzTemperatureSlope:
	                status=Ar9300TempSlopeGet(value, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, 3);
					break;
				case Ar9300SetEeprom5GHzTemperatureSlopeLow:
	                status=Ar9300TempSlopeLowGet(value);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, 3);
					break;
				case Ar9300SetEeprom5GHzTemperatureSlopeHigh:
	                status=Ar9300TempSlopeHighGet(value);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, 3);
					break;
					case Ar9300SetEeprom5GHzTemperatureSlopeExtension:
						status=Ar9300TempSlopeExtensionGet(value, ix, &num);
						ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
						break;
				case Ar9300SetEeprom5GHzVoltageSlope:
	                value[0]=Ar9300VoltSlopeGet(band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom5GHzSpur:
					status=Ar9300spurChansGet(value, ix, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzMinCCAPwrThreshold:
					status=Ar9300MinCCAPwrThreshChGet(value, ix, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzReserved:
					status=Ar9300ReservedGet(value, ix, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzQuickDrop:
	                value[0]=Ar9300QuickDropGet(band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom5GHzQuickDropLow:
	                value[0]=Ar9300QuickDropLowGet(band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom5GHzQuickDropHigh:
	                value[0]=Ar9300QuickDropHighGet(band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom5GHzXpaBiasLevel:
	                value[0]=Ar9300xpaBiasLvlGet(band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom5GHzTxFrameToDataStart:
	                value[0]=Ar9300txFrameToDataStartGet(band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom5GHzTxFrameToPaOn:
	                value[0]=Ar9300txFrameToPaOnGet(band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom5GHzTxClip:
	                value[0]=Ar9300txClipGet(band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom5GHzAntennaGain:
	                value[0]=Ar9300antennaGainGet(band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom5GHzSwitchSettling:
	                value[0]=Ar9300switchSettlingGet(band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom5GHzAdcSize:
	                value[0]=Ar9300adcDesiredSizeGet(band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom5GHzTxEndToXpaOff:
	                value[0]=Ar9300txEndToXpaOffGet(band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom5GHzTxEndToRxOn:
	                value[0]=Ar9300txEndToRxOnGet(band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom5GHzTxFrameToXpaOn:
	                value[0]=Ar9300txFrameToXpaOnGet(band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom5GHzThresh62:
	                value[0]=Ar9300thresh62Get(band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom5GHzPaPredistortionHt20:
	                value[0]=Ar9300PapdRateMaskHt20Get(band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom5GHzPaPredistortionHt40:
	                value[0]=Ar9300PapdRateMaskHt40Get(band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom5GHzWlanSpdtSwitchGlobalControl:
					value[0]=Ar9300WlanSpdtSwitchGlobalControlGet(band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom5GHzXLNABiasStrength:
					status=Ar9300XLANBiasStrengthGet(value, ix, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzRFGainCAP:
	                value[0]=Ar9300RFGainCAPGet(band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom5GHzTXGainCAP:
	                value[0]=Ar9300TXGainCAPGet(band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Ar9300SetEeprom5GHzFuture:
					Ar9300FutureGet(value, ix, &num, band_A);
					ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzCalibrationFrequency:
					status=Ar9300calFreqPierGet(value, ix, 0, 0, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzPowerCorrection:
					status=Ar9300calPierDataRefPowerGet(value, ix, iy, 0, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzCalibrationVoltage:
					status=Ar9300calPierDataVoltMeasGet(value, ix, iy, 0, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzCalibrationTemperature:
					status=Ar9300calPierDataTempMeasGet(value, ix, iy, 0, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzNoiseFloor:
					status=Ar9300calPierDataRxNoisefloorCalGet(value, ix, iy, 0, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzNoiseFloorPower:
					status=Ar9300calPierDataRxNoisefloorPowerGet(value, ix, iy, 0, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzNoiseFloorTemperature:
					status=Ar9300calPierDataRxTempMeasGet(value, ix, iy, 0, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzTargetFrequency:
					status=Ar9300calFreqTGTLegacyOFDMGet(value, ix, 0, 0, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzTargetFrequencyHt20:
					status=Ar9300calFreqTGTHT20Get(value, ix, 0, 0, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzTargetFrequencyHt40:
					status=Ar9300calFreqTGTHT40Get(value, ix, 0, 0, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzTargetPower:
					status=Ar9300calTGTPwrLegacyOFDMGet(fvalue, ix, iy, 0, &num, band_A);
					ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
					break;
				case Ar9300SetEeprom5GHzTargetPowerHt20:
					status=Ar9300calTGTPwrHT20Get(fvalue, ix, iy, 0, &num, band_A);
					ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
					break;
				case Ar9300SetEeprom5GHzTargetPowerHt40:
					status=Ar9300calTGTPwrHT40Get(fvalue, ix, iy, 0, &num, band_A);
					ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
					break;
				case Ar9300SetEeprom5GHzCtlIndex:
					status=Ar9300ctlIndexGet(value, ix, iy, 0, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzCtlFrequency:
					status=Ar9300ctlFreqGet(value, ix, iy, 0, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetEeprom5GHzCtlPower:
					status=Ar9300ctlPowerGet(fvalue, ix, iy, 0, &num, band_A);
					ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
					break;
				case Ar9300SetEeprom5GHzCtlBandEdge:
					status=Ar9300ctlFlagGet(value, ix, iy, 0, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
				case Ar9300SetCaldataMemoryType:
					status=Ar9300CaldataMemoryTypeGet(buffer,20);
					if(status==VALUE_OK) 
					{
						ReturnText("get",GetParameter[index].word[0],"",buffer);
						done++;
					}
					else // ErrorPrint();
						error++;
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

