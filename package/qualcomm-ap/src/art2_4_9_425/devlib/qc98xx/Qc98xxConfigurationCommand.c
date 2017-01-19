#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "wlantype.h"
#include "smatch.h"
#include "UserPrint.h"
#include "TimeMillisecond.h"
#include "CommandParse.h"
#include "ParameterSelect.h"
#include "Field.h"

#include "Device.h"

#include "ParameterParse.h"
#include "ParameterParseNart.h"
#include "ConfigurationCommand.h"
//#include "ConfigurationSet.h"
//#include "ConfigurationGet.h"
#include "ConfigurationStatus.h"
#include "Qc98xxPcieConfig.h"

#include "ErrorPrint.h"
#include "CardError.h"
#include "ParseError.h"
#include "NartError.h"

//
// hal header files
//
#include "qc98xx_eeprom.h"

#include "Qc98xxDevice.h"
#include "Qc98xxEepromStructSet.h"
#include "Qc98xxEepromStructGet.h"
#include "Qc98xxEepromPrint.h"
#include "Qc98xxSetList.h"


#define MBUFFER 1024

#ifndef max
#define max(a,b)	(((a) > (b)) ? (a) : (b))
#endif

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
    nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|");
    buffer[MBUFFER-1]=0;
    ErrorPrint(NartData,buffer);
}


static void ReturnUnsigned(char *command, char *name, char *atext, int *value, int nvalue, char *units)
{
    char buffer[MBUFFER];
    int lc, nc;
    int it;
	int isUnitsMhz;

    lc=0;
	// detect the case of unsued channels
	isUnitsMhz = Smatch(units, "MHz");

	if (isUnitsMhz && (value[0] == -1 || value[0] == 2555))
	{
		nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|%s|%s%s|N/A",command,name,atext);
	}
	else
	{
		nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|%s|%s%s|%u",command,name,atext,value[0]);
	}
    if(nc>0)
    {
        lc+=nc;
    }


    for(it=1; it<nvalue; it++)
    {
		if (isUnitsMhz && (value[it] == -1 || value[it] == 2555))
		{
			nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,",N/A");
		}
		else
		{
			nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,",%u", value[it]);
		}
        if(nc>0)
        {
            lc+=nc;
        }
    }
    nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|");
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
    nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|");
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
    nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|");
    buffer[MBUFFER-1]=0;
    ErrorPrint(NartData,buffer);
}


static void ReturnText(char *command, char *name, char *atext, char *value)
{
    char buffer[MBUFFER];
    int lc, nc;

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
    QC98XX_SET_EEPROM_ALL,
    QC98XX_SET_EEPROM_ALL_FIRMWARE,
    //pcie config
    QC98XX_SET_CONFIG,
    QC98XX_SET_EEPROM_DEVICEID,
    QC98XX_SET_EEPROM_SSID,
    QC98XX_SET_EEPROM_VID,
    QC98XX_SET_EEPROM_SVID,
	// Base Header
    QC98XX_SET_EEPROM_VERSION,
    QC98XX_SET_TEMPLATE_VERSION,
    QC98XX_SET_MAC_ADDRESS,
    QC98XX_SET_REGULATORY_DOMAIN,
    QC98XX_SET_OP_FLAGS,
    QC98XX_SET_OP_FLAGS2,
    QC98XX_SET_BOARD_FLAGS,
    QC98XX_SET_BLUETOOTH_OPTIONS,
    QC98XX_SET_FEATURE_ENABLE,
    QC98XX_SET_FEATURE_ENABLE_TEMPERATURE_COMPENSATION,
    QC98XX_SET_FEATURE_ENABLE_VOLTAGE_COMPENSATION,
    //QC98XX_SET_FEATURE_ENABLE_FAST_CLOCK,
    QC98XX_SET_FEATURE_ENABLE_DOUBLING,
    QC98XX_SET_FEATURE_ENABLE_INTERNAL_SWITCHING_REGULATOR,
    //QC98XX_SET_FEATURE_ENABLE_PA_PREDISTORTION,
    QC98XX_SET_BOARD_FLAGS_ENABLE_RBIAS,
    QC98XX_SET_FEATURE_ENABLE_TUNING_CAPS,
    QC98XX_SET_MISCELLANEOUS,
    QC98XX_SET_MISCELLANEOUS_DRIVERS_STRENGTH,
    QC98XX_SET_MISCELLANEOUS_THERMOMETER,
    QC98XX_SET_MISCELLANEOUS_CHAIN_MASK_REDUCE,
    QC98XX_SET_MISCELLANEOUS_QUICK_DROP_ENABLE,
    QC98XX_SET_MISCELLANEOUS_EEP,
    QC98XX_SET_BOARD_FLAG1_BIBXOSC0,
	QC98XX_SET_BOARD_FLAG1_NOISE_FLR_THR,
    //QC98XX_SET_TX_RX_MASK,
    QC98XX_SET_TX_RX_MASK_TX,
    QC98XX_SET_TX_RX_MASK_RX,
    QC98XX_SET_RF_SILENT,
    QC98XX_SET_RF_SILENT_B0,
    QC98XX_SET_RF_SILENT_B1,
    QC98XX_SET_RF_SILENT_GPIO,
    QC98XX_SET_WLAN_LED_GPIO,
    //QC98XX_SET_DEVICE_TYPE,
    //QC98XX_SET_CTL_VERSION,
    QC98XX_SET_SPUR_BASE_A,
    QC98XX_SET_SPUR_BASE_B,
    QC98XX_SET_SPUR_RSSI_THRESH,
    QC98XX_SET_SPUR_RSSI_THRESH_CCK,
    QC98XX_SET_SPUR_MIT_FLAG,
    QC98XX_SET_SWITCHING_REGULATOR,
    QC98XX_SET_TX_RX_GAIN,
    QC98XX_SET_TX_RX_GAIN_TX,
    QC98XX_SET_TX_RX_GAIN_RX,
    QC98XX_SET_POWER_TABLE_OFFSET,
    QC98XX_SET_TUNING_CAPS,
    QC98XX_SET_DELTA_CCK_20,
    QC98XX_SET_DELTA_40_20,
    QC98XX_SET_DELTA_80_20,
    QC98XX_SET_CUSTOMER_DATA,

	// 2GHz Bimodal Header

    QC98XX_SET_2GHZ_VOLTAGE_SLOPE,
    QC98XX_SET_2GHZ_SPUR,
    QC98XX_SET_2GHZ_SPURA,
    QC98XX_SET_2GHZ_SPURB,

//    QC98XX_SET_2GHZ_NOISE_FLOOR_THRESHOLD,
    QC98XX_SET_2GHZ_XPA_BIAS_LEVEL,
    QC98XX_SET_2GHZ_XPA_BIAS_BYPASS,
    QC98XX_SET_2GHZ_ANTENNA_GAIN,
    QC98XX_SET_2GHZ_ANTENNA_CONTROL_COMMON,
    QC98XX_SET_2GHZ_ANTENNA_CONTROL_COMMON_2,
    QC98XX_SET_2GHZ_ANTENNA_CONTROL_CHAIN,
    QC98XX_SET_2GHZ_RX_FILTER_CAP,
    QC98XX_SET_2GHZ_RX_GAIN_CAP,
    QC98XX_SET_2GHZ_TX_RX_GAIN_TX,
    QC98XX_SET_2GHZ_TX_RX_GAIN_RX,
	QC98XX_SET_2GHZ_NOISE_FLR_THR,
	QC98XX_SET_2GHZ_MIN_CCA_PWR_CHAIN,
//	QC98XX_SET_2GHZ_NOISE_FLOOR
//	QC98XX_SET_2GHZ_NOISE_FLOOR_POWER
//	QC98XX_SET_2GHZ_NOISE_FLOOR_TEMPERATURE

	// 5GHz Bimodal Header

    QC98XX_SET_5GHZ_VOLTAGE_SLOPE,
    QC98XX_SET_5GHZ_SPUR,
    QC98XX_SET_5GHZ_SPURA,
    QC98XX_SET_5GHZ_SPURB,
//    QC98XX_SET_5GHZ_NOISE_FLOOR_THRESHOLD,
    QC98XX_SET_5GHZ_XPA_BIAS_LEVEL,
    QC98XX_SET_5GHZ_XPA_BIAS_BYPASS,
    QC98XX_SET_5GHZ_ANTENNA_GAIN,
    QC98XX_SET_5GHZ_ANTENNA_CONTROL_COMMON,
    QC98XX_SET_5GHZ_ANTENNA_CONTROL_COMMON_2,
    QC98XX_SET_5GHZ_ANTENNA_CONTROL_CHAIN,
    QC98XX_SET_5GHZ_RX_FILTER_CAP,
    QC98XX_SET_5GHZ_RX_GAIN_CAP,
    QC98XX_SET_5GHZ_TX_RX_GAIN_TX,
    QC98XX_SET_5GHZ_TX_RX_GAIN_RX,
	QC98XX_SET_5GHZ_NOISE_FLR_THR,
	QC98XX_SET_5GHZ_MIN_CCA_PWR_CHAIN,
//	QC98XX_SET_5GHZ_NOISE_FLOOR
//	QC98XX_SET_5GHZ_NOISE_FLOOR_POWER
//	QC98XX_SET_5GHZ_NOISE_FLOOR_TEMPERATURE


	// FreqModal Header
    QC98XX_SET_2GHZ_XATTEN1_DB,
    QC98XX_SET_5GHZ_XATTEN1_DB_LOW,
    QC98XX_SET_5GHZ_XATTEN1_DB_MID,
    QC98XX_SET_5GHZ_XATTEN1_DB_HIGH,
    QC98XX_SET_2GHZ_XATTEN1_MARGIN,
    QC98XX_SET_5GHZ_XATTEN1_MARGIN_LOW,
    QC98XX_SET_5GHZ_XATTEN1_MARGIN_MID,
    QC98XX_SET_5GHZ_XATTEN1_MARGIN_HIGH,

	// Chip Cal Data
    QC98XX_SET_THERM_ADC_SCALED_GAIN,
    QC98XX_SET_THERM_ADC_OFFSET,

	// 2GHz Cal Info
    QC98XX_SET_2GHZ_CALIBRATION_FREQUENCY,
    QC98XX_SET_2GHZ_CALPOINT_TXGAIN_INDEX,
    QC98XX_SET_2GHZ_CALPOINT_DAC_GAIN,
    QC98XX_SET_2GHZ_CALPOINT_POWER,
    QC98XX_SET_2GHZ_CALIBRATION_TEMPERATURE,
    QC98XX_SET_2GHZ_CALIBRATION_VOLTAGE,

	QC98XX_SET_2GHZ_TARGET_FREQUENCY_CCK,
    QC98XX_SET_2GHZ_TARGET_FREQUENCY,
    QC98XX_SET_2GHZ_TARGET_FREQUENCY_VHT20,
    QC98XX_SET_2GHZ_TARGET_FREQUENCY_VHT40,
    QC98XX_SET_2GHZ_TARGET_POWER_CCK,
    QC98XX_SET_2GHZ_TARGET_POWER,
    QC98XX_SET_2GHZ_TARGET_POWER_VHT20,
    QC98XX_SET_2GHZ_TARGET_POWER_VHT40,

	// 2GHz Ctl
    QC98XX_SET_2GHZ_CTL_INDEX,
    QC98XX_SET_2GHZ_CTL_FREQUENCY,
    QC98XX_SET_2GHZ_CTL_POWER,
    QC98XX_SET_2GHZ_CTL_BANDEDGE,

	QC98XX_SET_2GHZ_NOISE_FLOOR,
    QC98XX_SET_2GHZ_NOISE_FLOOR_POWER,
	QC98XX_SET_2GHZ_NOISE_FLOOR_TEMPERATURE,
	QC98XX_SET_2GHZ_NOISE_FLOOR_TEMPERATURE_SLOPE,

	// 2GHz Alpha Therm
    QC98XX_SET_2GHZ_ALPHA_THERM_TABLE,

	// 5GHz Cal Info
    QC98XX_SET_5GHZ_CALIBRATION_FREQUENCY,
    QC98XX_SET_5GHZ_CALPOINT_TXGAIN_INDEX,
    QC98XX_SET_5GHZ_CALPOINT_DAC_GAIN,
    QC98XX_SET_5GHZ_CALPOINT_POWER,
    QC98XX_SET_5GHZ_CALIBRATION_TEMPERATURE,
    QC98XX_SET_5GHZ_CALIBRATION_VOLTAGE,

	QC98XX_SET_5GHZ_TARGET_FREQUENCY,
    QC98XX_SET_5GHZ_TARGET_FREQUENCY_VHT20,
    QC98XX_SET_5GHZ_TARGET_FREQUENCY_VHT40,
    QC98XX_SET_5GHZ_TARGET_FREQUENCY_VHT80,
    QC98XX_SET_5GHZ_TARGET_POWER,
    QC98XX_SET_5GHZ_TARGET_POWER_VHT20,
    QC98XX_SET_5GHZ_TARGET_POWER_VHT40,
    QC98XX_SET_5GHZ_TARGET_POWER_VHT80,

	// 5GHz Ctl
    QC98XX_SET_5GHZ_CTL_INDEX,
    QC98XX_SET_5GHZ_CTL_FREQUENCY,
    QC98XX_SET_5GHZ_CTL_POWER,
    QC98XX_SET_5GHZ_CTL_BANDEDGE,

	QC98XX_SET_5GHZ_NOISE_FLOOR,
	QC98XX_SET_5GHZ_NOISE_FLOOR_POWER,
	QC98XX_SET_5GHZ_NOISE_FLOOR_TEMPERATURE,
	QC98XX_SET_5GHZ_NOISE_FLOOR_TEMPERATURE_SLOPE,

	// 5GHz Alpha Therm
    QC98XX_SET_5GHZ_ALPHA_THERM_TABLE,

	// Config
    QC98XX_SET_CONFIG_ADDR,

	//non-eeporm parameters
    DEV_STBC,
    DEV_LDPC,
    DEV_NON_CENTER_FREQ_ALLOWED,
};


QC98XXDLLSPEC int Qc98xxGetParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(GetParameter)/sizeof(struct _ParameterList);
    list->special=GetParameter;
    return 0;
}

QC98XXDLLSPEC int Qc98xxSetParameterSplice(struct _ParameterList *list)
{
    list->nspecial=(sizeof(GetParameter)/sizeof(struct _ParameterList))-1;
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
                }
    if(status==VALUE_OK)
    {
        if (GetParameter[index].type=='u' || GetParameter[index].type=='z') {
			ReturnUnsigned("get",GetParameter[index].word[0],atext,value,num, GetParameter[index].units);
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
                ReturnUnsigned("set",GetParameter[index].word[0],atext,uvalue,ngot, GetParameter[index].units);
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
    int status=0;
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
	*error=status;
}


static void ConfigSet(int *done, int *error, int ip, int index, int iband,
                            int (*_pSet)(int value), int (*_pSetBand)(int value, int iBand))
{
    int status;
    int ngot=0;
    char *name;
    int value[MAXVALUE];
    unsigned int uvalue[MAXVALUE];

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
                ReturnUnsigned("set",GetParameter[index].word[0],"",uvalue,ngot,GetParameter[index].units);
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
                ReturnUnsigned("set",GetParameter[index].word[0],"",uvalue,ngot,GetParameter[index].units);
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
// Process a "Set" command specified by the input command line
//
QC98XXDLLSPEC int Qc98xxSetCommandLine(char *input)
{
	int ArgMany;
	ArgMany = CommandParse(input);
	if (ArgMany<=0)
		return 0;
	return Qc98xxSetCommand(0);
}

//
// parse and then set a configuration parameter in the internal structure
//
QC98XXDLLSPEC int Qc98xxSetCommand(int client)
{
    int np, ip;
    char *name;
    int error;
    int done;
    int index;
    int code;
    unsigned char cmac[6];
    char buffer[MBUFFER];
    char *text;
    int status;
	unsigned long lvalue, addr;
	int parseStatus;
    int ix, iy, iz;
    int size;

    error=0;
    done=0;
    size = 4;
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
                case Qc9KSetEepromVersion:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxEepromVersion, 0);
                    break;
                case Qc9KSetEepromTemplateVersion:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxTemplateVersion, 0);
                    break;
				case Qc9KSetEepromDeviceId:
					ConfigSetUnsigned(&done, &error, ip, index, -1, Qc98xxDeviceIDSet, 0);
					break;
				case Qc9KSetEepromSSID:
					ConfigSetUnsigned(&done, &error, ip, index, -1, Qc98xxSSIDSet, 0);
					break;
				case Qc9KSetEepromVID:
					ConfigSetUnsigned(&done, &error, ip, index, -1, Qc98xxVendorSet, 0);
					break;
				case Qc9KSetEepromSVID:
					ConfigSetUnsigned(&done, &error, ip, index, -1, Qc98xxSubVendorSet, 0);
					break;
				case Qc9KSetEepromConfig:
					// set config=32bitHexValue; address=16bitHexAddress;
					parseStatus = ParseHex(ip, name, 1, &lvalue);
					if (parseStatus==1)
                    {
						parseStatus = ParseHex(++ip, name, 1, &addr);
						if (parseStatus==1)
                        {
						    parseStatus = ParseHex(++ip, name, 1, &size);
                            if (parseStatus!=1)
                            {
                                size = 4;
                            }
                            if ((size == 4 && (addr & 3) != 0) || (size == 2 && (addr & 1) != 0))
                            {
                                ErrorPrint(ParseBadValue,"address and size are not matching",name);
                                error++;
                            }
                            else
                            {
							    status=Qc98xxPcieAddressValueDataSet(addr, lvalue, size);
							    SformatOutput(buffer, MBUFFER-1, "%s(0x%x) 0x%x ", name, addr, lvalue);
							    ReturnText("set",GetParameter[index].word[0],"",buffer);
                            }
						}
					}
					break;
				case Qc9KSetEepromConfigPCIe:
					break;
                case Qc9KSetEepromMacAddress:
                    text=CommandParameterValue(ip,0);
                    status=ParseMacAddress(text,cmac);
                    if (status == VALUE_OK)
                    {
                        status=Qc98xxMacAddressSet(cmac);
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
                case Qc9KSetEepromCustomerData:
                    text=CommandParameterValue(ip,0);
                    status=Qc98xxCustomerDataSet(text, strlen(text));
                    if(status==VALUE_OK)
                    {
                        ReturnText("set",GetParameter[index].word[0],"",text);
                        done++;
                    }
                    else	// ErrorPrint();
                        error++;
                    break;
                case Qc9KSetEepromRegulatoryDomain:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, -1, Qc98xxRegDmnSet);
                    break;
                case Qc9KSetEepromTxRxMask:
					error++;
					ErrorPrint(NartData,"Please use set Mask.Tx/Mask.Rx");
					ErrorPrint(ParseBadParameter,name);
                    break;
                case Qc9KSetEepromTxRxMaskTx:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxTxMaskSet, 0);
                    break;
                case Qc9KSetEepromTxRxMaskRx:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxRxMaskSet, 0);
                    break;
                case Qc9KSetEepromOpFlags:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxOpFlagsSet, 0);
                    break;
                case Qc9KSetEepromOpFlags2:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxOpFlags2Set, 0);
                    break;
                case Qc9KSetEepromBoardFlags:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxBoardFlagsSet, 0);
                    break;
                case Qc9KSetEepromRfSilent:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxRfSilentSet, 0);
                    break;
                case Qc9KSetEepromRfSilentB0:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxRfSilentB0Set, 0);
                    break;
                case Qc9KSetEepromRfSilentB1:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxRfSilentB1Set, 0);
                    break;
                case Qc9KSetEepromRfSilentGpio:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxRfSilentGPIOSet, 0);
                    break;
                case Qc9KSetEepromBlueToothOptions:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxBlueToothOptionsSet, 0);
                    break;
                case Qc9KSetEepromPowerTableOffset:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxPwrTableOffsetSet, 0);
                    break;
                case Qc9KSetEepromTuningCaps:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, -1, Qc98xxPwrTuningCapsParamsSet);
                    break;
				case Qc9KSetEepromDeltaCck20:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxDeltaCck20Set, 0);
					break;
				case Qc9KSetEepromDelta4020:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxDelta4020Set, 0);
					break;
				case Qc9KSetEepromDelta8020:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxDelta8020Set, 0);
					break;
                case Qc9KSetEepromFeatureEnable:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxEnableFeatureSet, 0);
                    break;
                case Qc9KSetEepromFeatureEnableTemperatureCompensation:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxEnableTempCompensationSet, 0);
                    break;
                case Qc9KSetEepromFeatureEnableVoltageCompensation:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxEnableVoltCompensationSet, 0);
                    break;
                //case Qc9KSetEepromFeatureEnableFastClock:
                //    ConfigSet(&done, &error, ip, index, -1, Qc98xxEnableFastClockSet, 0);
                //    break;
                case Qc9KSetEepromFeatureEnableDoubling:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxEnableDoublingSet, 0);
                    break;
                case Qc9KSetEepromFeatureEnableInternalSwitchingRegulator:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxInternalRegulatorSet, 0);
                    break;
                case Qc9KSetEepromFeatureEnablePaPredistortion:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxPapdSet, 0);
                    break;
                case Qc9KSetEepromEnableRbias:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxRbiasSet, 0);
                    break;
                case Qc9KSetEepromFeatureEnableTuningCaps:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxEnableTuningCapsSet, 0);
                    break;
                case Qc9KSetEepromMiscellaneous:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxMiscConfigurationSet, 0);
                    break;
                case Qc9KSetEepromMiscellaneousDriveStrength:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxReconfigDriveStrengthSet, 0);
                    break;
                case Qc9KSetEepromMiscellaneousThermometer:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxThermometerSet, 0);
                    break;
                case Qc9KSetEepromMiscellaneousChainMaskReduce:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxChainMaskReduceSet, 0);
                    break;
                case Qc9KSetEepromMiscellaneousQuickDropEnable:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxReconfigQuickDropSet, 0);
                    break;
                case Qc9KSetEepromMiscellaneousEep:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxEepMiscSet, 0);
                    break;
                case Qc9KSetEepromBibxosc0:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxBibxosc0Set, 0);
                    break;
				case Qc9KSetEepromFlag1NoiseFlrThr:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxFlag1NoiseFlrThrSet, 0);
                    break;
                case Qc9KSetEepromWlanLedGpio:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxWlanLedGpioSet, 0);
                    break;
				case Qc9KSetEepromSpurBaseA:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxSpurBaseASet, 0);
                    break;
				case Qc9KSetEepromSpurBaseB:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxSpurBaseBSet, 0);
                    break;
				case Qc9KSetEepromSpurRssiThresh:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxSpurRssiThreshSet, 0);
                    break;
				case Qc9KSetEepromSpurRssiThreshCck:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxSpurRssiThreshCckSet, 0);
                    break;
				case Qc9KSetEepromSpurMitFlag:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxSpurMitFlagSet, 0);
                    break;
                case Qc9KSetEepromTxRxGain:
                    ConfigSet(&done, &error, ip, index, band_both, 0, Qc98xxTxRxGainSet);
                    break;
                case Qc9KSetEepromTxRxGainTx:
                    ConfigSet(&done, &error, ip, index, band_both, 0, Qc98xxTxGainSet);
                    break;
                case Qc9KSetEepromTxRxGainRx:
                    ConfigSet(&done, &error, ip, index, band_both, 0, Qc98xxRxGainSet);
                    break;
                case Qc9KSetEepromSwregProgram:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxSWREGProgramSet, 0);
                    break;
                case Qc9KSetEepromSwreg:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxSWREGSet, 0);
                    break;
                case Qc9KSetEeprom2GHzAntennaControlCommon:
                    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxAntCtrlCommonSet);
                    break;
                case Qc9KSetEeprom2GHzAntennaControlCommon2:
                    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxAntCtrlCommon2Set);
                    break;
                case Qc9KSetEeprom2GHzAntennaControlChain:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxAntCtrlChainSet);
                    break;
				case Qc9KSetEeprom2GHzRxFilterCap:
                    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxRxFilterCapSet);
                    break;
				case Qc9KSetEeprom2GHzRxGainCap:
                    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxRxGainCapSet);
                    break;
				case Qc9KSetEeprom2GHzTxRxGainTx:
                    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxTxGainSet);
                    break;
				case Qc9KSetEeprom2GHzTxRxGainRx:
                    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxRxGainSet);
                    break;
				case Qc9KSetEeprom2GHzNoiseFlrThr:
					ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxNoiseFlrThrSet);
					break;
				case Qc9KSetEeprom2GHzMinCcaPwrChain:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxMinCcaPwrChainSet);
					break;
                case Qc9KSetEeprom2GHzXatten1Db:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxXatten1DBSet);
                    break;
                case Qc9KSetEeprom2GHzXatten1Margin:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxXatten1MarginSet);
                    break;
                case Qc9KSetEeprom2GHzVoltageSlope:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxVoltSlopeSet);
                    break;
                case Qc9KSetEeprom2GHzSpur:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxSpurChansSet);
                    break;
				case Qc9KSetEeprom2GHzSpurAPrimSecChoose:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxSpurAPrimSecChooseSet);
                    break;
				case Qc9KSetEeprom2GHzSpurBPrimSecChoose:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxSpurBPrimSecChooseSet);
                    break;
                case Qc9KSetEeprom2GHzAntennaGain:
                    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxAntennaGainSet);
                    break;
                case Qc9KSetEeprom2GHzXpaBiasLevel:
                    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxXpaBiasLvlSet);
                    break;
                case Qc9KSetEeprom2GHzXpaBiasBypass:
                    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxXpaBiasBypassSet);
                    break;
#if 0
                case Qc9KSetEeprom2GHzTemperatureSlope:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxTempSlopeSet);
                    break;
                case Qc9KSetEeprom2GHzNoiseFloorThreshold:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxNoiseFloorThreshChSet);
                    break;
                //case Qc9KSetEeprom2GHzQuickDrop:
                //    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxQuickDropSet);
                //    break;
                case Qc9KSetEeprom2GHzTxFrameToDataStart:
                    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxTxFrameToDataStartSet);
                    break;
                case Qc9KSetEeprom2GHzTxFrameToPaOn:
                    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxTxFrameToPaOnSet);
                    break;
                case Qc9KSetEeprom2GHzTxClip:
                    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxTxClipSet);
                    break;
                case Qc9KSetEeprom2GHzDacScaleCCK:
                    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxDacScaleCckSet);
                    break;
                case Qc9KSetEeprom2GHzSwitchSettling:
                    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxSwitchSettlingSet);
                    break;
                case Qc9KSetEeprom2GHzSwitchSettlingHt40:
                    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxSwitchSettlingHt40Set);
                    break;
                case Qc9KSetEeprom2GHzAdcSize:
                    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxAdcDesiredSizeSet);
                    break;
                case Qc9KSetEeprom2GHzTxEndToXpaOff:
                    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxTxEndToXpaOffSet);
                    break;
                case Qc9KSetEeprom2GHzTxEndToRxOn:
                    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxTxEndToRxOnSet);
                    break;
                case Qc9KSetEeprom2GHzTxFrameToXpaOn:
                    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxTxFrameToXpaOnSet);
                    break;
                case Qc9KSetEeprom2GHzThresh62:
                    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxThresh62Set);
                    break;
                //case Qc9KSetEeprom2GHzPaPredistortionHt20:
                //    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxPapdRateMaskHt20Set);
                //    break;
                //case Qc9KSetEeprom2GHzPaPredistortionHt40:
                //    ConfigSet(&done, &error, ip, index, band_BG, 0, Qc98xxPapdRateMaskHt40Set);
                //    break;
                //case Qc9KSetEeprom2GHzFuture:
                //    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxFutureSet);
                //    break;
                //case Qc9KSetEepromCommonFuture:
                //    ConfigSets(&done, &error, ip, index, ix, iy, iz, -1, Qc98xxFutureSet);
                //    break;
                //case Qc9KSetEepromAntennaDiversityControl:
                //    ConfigSet(&done, &error, ip, index, -1, Qc98xxAntDivCtrlSet, 0);
                //    break;
                case Qc9KSetEeprom2GHzAttenuation1Hyst:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxXatten1HystSet);
                    break;
#endif //0
				case Qc9KSetEepromThermAdcScaledGain:
					ConfigSet(&done, &error, ip, index, -1, Qc98xxThermAdcScaledGainSet, 0);
                    break;
				case Qc9KSetEepromThermAdcOffset:
					ConfigSet(&done, &error, ip, index, -1, Qc98xxThermAdcOffsetSet, 0);
                    break;
                case Qc9KSetEeprom2GHzCalibrationFrequency:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxCalFreqPierSet);
                    break;
				case Qc9KSetEeprom2GHzCalPointTxGainIdx:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxCalPointTxGainIdxSet);
                    break;
				case Qc9KSetEeprom2GHzCalPointDacGain:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxCalPointDacGainSet);
                    break;
				case Qc9KSetEeprom2GHzCalPointPower:
                    ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxCalPointPowerSet);
					break;
                case Qc9KSetEeprom2GHzCalibrationVoltage:
                    ConfigSets(&done, &error, ip, index, ix, 0, 0, band_BG, Qc98xxCalPierDataVoltMeasSet);
                    break;
                case Qc9KSetEeprom2GHzCalibrationTemperature:
                    ConfigSets(&done, &error, ip, index, ix, 0, 0, band_BG, Qc98xxCalPierDataTempMeasSet);
                    break;
                case Qc9KSetEeprom2GHzTargetFrequencyCck:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxCalFreqTGTcckSet);
                    break;
                case Qc9KSetEeprom2GHzTargetFrequency:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxCalFreqTGTLegacyOFDMSet);
                    break;
                case Qc9KSetEeprom2GHzTargetFrequencyVht20:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxCalFreqTGTHT20Set);
                    break;
                case Qc9KSetEeprom2GHzTargetFrequencyVht40:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxCalFreqTGTHT40Set);
                    break;
                case Qc9KSetEeprom2GHzTargetPowerCck:
                    ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxCalTGTPwrCCKSet);
                    break;
                case Qc9KSetEeprom2GHzTargetPower:
                    ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxCalTGTPwrLegacyOFDMSet);
                    break;
                case Qc9KSetEeprom2GHzTargetPowerVht20:
                    ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxCalTGTPwrHT20Set);
                    break;
                case Qc9KSetEeprom2GHzTargetPowerVht40:
                    ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxCalTGTPwrHT40Set);
                    break;
                case Qc9KSetEeprom2GHzCtlIndex:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxCtlIndexSet);
                    break;
                case Qc9KSetEeprom2GHzCtlFrequency:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxCtlFreqSet);
                    break;
                case Qc9KSetEeprom2GHzCtlPower:
                    ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxCtlPowerSet);
                    break;
                case Qc9KSetEeprom2GHzCtlBandEdge:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxCtlFlagSet);
                    break;
			    case Qc9KSetEeprom2GHzNoiseFloor:
				    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxNoiseFloorSet);
				    break;
    			case Qc9KSetEeprom2GHzNoiseFloorPower:
				    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxNoiseFloorPowerSet);
				    break;
			    case Qc9KSetEeprom2GHzNoiseFloorTemperature:
				    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxNoiseFloorTemperatureSet);
				    break;
                case Qc9KSetEeprom2GHzNoiseFloorTemperatureSlope:
				    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxNoiseFloorTemperatureSlopeSet);
				    break;
				case Qc9KSetEeprom2GHzAlphaThermTable:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_BG, Qc98xxAlphaThermTableSet);
                    break;

                case Qc9KSetEeprom5GHzAntennaControlCommon:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxAntCtrlCommonSet);
                    break;
                case Qc9KSetEeprom5GHzAntennaControlCommon2:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxAntCtrlCommon2Set);
                    break;
                case Qc9KSetEeprom5GHzAntennaControlChain:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxAntCtrlChainSet);
                    break;
				case Qc9KSetEeprom5GHzRxFilterCap:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxRxFilterCapSet);
                    break;
				case Qc9KSetEeprom5GHzRxGainCap:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxRxGainCapSet);
                    break;
				case Qc9KSetEeprom5GHzTxRxGainTx:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxTxGainSet);
                    break;
				case Qc9KSetEeprom5GHzTxRxGainRx:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxRxGainSet);
                    break;
				case Qc9KSetEeprom5GHzNoiseFlrThr:
					ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxNoiseFlrThrSet);
					break;
				case Qc9KSetEeprom5GHzMinCcaPwrChain:
					ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxMinCcaPwrChainSet);
					break;
                case Qc9KSetEeprom5GHzXatten1DbMid:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxXatten1DBSet);
                    break;
                case Qc9KSetEeprom5GHzXatten1MarginMid:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxXatten1MarginSet);
                    break;
                case Qc9KSetEeprom5GHzXatten1DbLow:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxXatten1DBLowSet);
                    break;
                case Qc9KSetEeprom5GHzXatten1DbHigh:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxXatten1DBHighSet);
                    break;
                case Qc9KSetEeprom5GHzXatten1MarginLow:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxXatten1MarginLowSet);
                    break;
                case Qc9KSetEeprom5GHzXatten1MarginHigh:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxXatten1MarginHighSet);
                    break;
                case Qc9KSetEeprom5GHzVoltageSlope:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxVoltSlopeSet);
                    break;
                case Qc9KSetEeprom5GHzSpur:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxSpurChansSet);
                    break;
				case Qc9KSetEeprom5GHzSpurAPrimSecChoose:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxSpurAPrimSecChooseSet);
                    break;
				case Qc9KSetEeprom5GHzSpurBPrimSecChoose:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxSpurBPrimSecChooseSet);
                    break;
                case Qc9KSetEeprom5GHzAntennaGain:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxAntennaGainSet);
                    break;
                case Qc9KSetEeprom5GHzXpaBiasLevel:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxXpaBiasLvlSet);
                    break;
                case Qc9KSetEeprom5GHzXpaBiasBypass:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxXpaBiasBypassSet);
                    break;
#if 0
                case Qc9KSetEeprom5GHzAttenuation1Hyst:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxXatten1HystSet);
                    break;
                case Qc9KSetEeprom5GHzAttenuation2Db:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxXatten2DBSet);
                    break;
                case Qc9KSetEeprom5GHzAttenuation2Margin:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxXatten2MarginSet);
                    break;
                case Qc9KSetEeprom5GHzAttenuation2Hyst:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxXatten2HystSet);
                    break;
                case Qc9KSetEeprom5GHzTemperatureSlope:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxTempSlopeSet);
                    break;
                case Qc9KSetEeprom5GHzTemperatureSlopeLow:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxTempSlopeLowSet, 0);
                    break;
                case Qc9KSetEeprom5GHzTemperatureSlopeHigh:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxTempSlopeHighSet, 0);
                    break;
                case Qc9KSetEeprom5GHzNoiseFloorThreshold:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxNoiseFloorThreshChSet);
                    break;
                case Qc9KSetEeprom5GHzQuickDrop:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxQuickDropSet);
                    break;
                case Qc9KSetEeprom5GHzQuickDropLow:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxQuickDropLowSet, 0);
                    break;
                case Qc9KSetEeprom5GHzQuickDropHigh:
                    ConfigSet(&done, &error, ip, index, -1, Qc98xxQuickDropHighSet, 0);
                    break;
                case Qc9KSetEeprom5GHzTxFrameToDataStart:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxTxFrameToDataStartSet);
                    break;
                case Qc9KSetEeprom5GHzTxFrameToPaOn:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxTxFrameToPaOnSet);
                    break;
                case Qc9KSetEeprom5GHzTxClip:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxTxClipSet);
                    break;
                case Qc9KSetEeprom5GHzSwitchSettling:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxSwitchSettlingSet);
                    break;
                case Qc9KSetEeprom5GHzSwitchSettlingHt40:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxSwitchSettlingHt40Set);
                    break;
                case Qc9KSetEeprom5GHzAdcSize:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxAdcDesiredSizeSet);
                    break;
                case Qc9KSetEeprom5GHzTxEndToXpaOff:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxTxEndToXpaOffSet);
                    break;
                case Qc9KSetEeprom5GHzTxEndToRxOn:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxTxEndToRxOnSet);
                    break;
                case Qc9KSetEeprom5GHzTxFrameToXpaOn:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxTxFrameToXpaOnSet);
                    break;
                case Qc9KSetEeprom5GHzThresh62:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxThresh62Set);
                    break;
                case Qc9KSetEeprom5GHzPaPredistortionHt20:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxPapdRateMaskHt20Set);
                    break;
                case Qc9KSetEeprom5GHzPaPredistortionHt40:
                    ConfigSet(&done, &error, ip, index, band_A, 0, Qc98xxPapdRateMaskHt40Set);
                    break;
                case Qc9KSetEeprom5GHzFuture:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxFutureSet);
                    break;
#endif //0
                case Qc9KSetEeprom5GHzCalibrationFrequency:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxCalFreqPierSet);
                    break;
				case Qc9KSetEeprom5GHzCalPointTxGainIdx:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxCalPointTxGainIdxSet);
                    break;
				case Qc9KSetEeprom5GHzCalPointDacGain:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxCalPointDacGainSet);
                    break;
				case Qc9KSetEeprom5GHzCalPointPower:
                    ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxCalPointPowerSet);
					break;
                case Qc9KSetEeprom5GHzCalibrationVoltage:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxCalPierDataVoltMeasSet);
                    break;
                case Qc9KSetEeprom5GHzCalibrationTemperature:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxCalPierDataTempMeasSet);
                    break;
                case Qc9KSetEeprom5GHzTargetFrequency:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxCalFreqTGTLegacyOFDMSet);
                    break;
                case Qc9KSetEeprom5GHzTargetFrequencyVht20:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxCalFreqTGTHT20Set);
                    break;
                case Qc9KSetEeprom5GHzTargetFrequencyVht40:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxCalFreqTGTHT40Set);
                    break;
                case Qc9KSetEeprom5GHzTargetFrequencyVht80:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxCalFreqTGTHT80Set);
                    break;
                case Qc9KSetEeprom5GHzTargetPower:
                    ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxCalTGTPwrLegacyOFDMSet);
                    break;
                case Qc9KSetEeprom5GHzTargetPowerVht20:
                    ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxCalTGTPwrHT20Set);
                    break;
                case Qc9KSetEeprom5GHzTargetPowerVht40:
                    ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxCalTGTPwrHT40Set);
                    break;
                case Qc9KSetEeprom5GHzTargetPowerVht80:
                    ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxCalTGTPwrHT80Set);
                    break;
                case Qc9KSetEeprom5GHzCtlIndex:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxCtlIndexSet);
                    break;
                case Qc9KSetEeprom5GHzCtlFrequency:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxCtlFreqSet);
                    break;
                case Qc9KSetEeprom5GHzCtlPower:
                    ConfigfSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxCtlPowerSet);
                    break;
                case Qc9KSetEeprom5GHzCtlBandEdge:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxCtlFlagSet);
                    break;
                case Qc9KSetEeprom5GHzNoiseFloor:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxNoiseFloorSet);
                    break;
                case Qc9KSetEeprom5GHzNoiseFloorPower:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxNoiseFloorPowerSet);
                    break;
                case Qc9KSetEeprom5GHzNoiseFloorTemperature:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxNoiseFloorTemperatureSet);
                    break;
                case Qc9KSetEeprom5GHzNoiseFloorTemperatureSlope:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxNoiseFloorTemperatureSlopeSet);
                    break;
				case Qc9KSetEeprom5GHzAlphaThermTable:
                    ConfigSets(&done, &error, ip, index, ix, iy, iz, band_A, Qc98xxAlphaThermTableSet);
                    break;
				case Qc9KSetEepromConfigAddr:
                    ConfigSets(&done, &error, ip, index, ix, 0, 0, -1, Qc98xxConfigAddrSet1);
                    break;

                // non-eeprom parameters
                case DevSetStbc:
                    ConfigSet(&done, &error, ip, index, -1, DevStbcSet, 0);
                    break;
                case DevSetLdpc:
                    ConfigSet(&done, &error, ip, index, -1, DevLdpcSet, 0);
                    break;
                case DevSetNonCenterFreqAllowed:
                    ConfigSet(&done, &error, ip, index, -1, DevNonCenterFreqAllowedSet, 0);
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
QC98XXDLLSPEC int Qc98xxGetCommand(int client)
{
    int np, ip;
    char *name;
    int error;
    int done;
    int index;
    int code;
    unsigned char cmac[6];
    int value[MAXVALUE];
    double fvalue[MAXVALUE];
	unsigned int uvalue[MAXVALUE];
    char buffer[MBUFFER];
    int status;
    int ix, iy, iz, num;
    int ngot;

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
                case Qc9KSetEepromALL:
                    PrintQc98xxStructHost(client,0);
                    done++;
                    break;
                case Qc9KSetEepromAllFirmware:
                    PrintQc98xxStructDut(client,0);
                    done++;
                    break;
				//
			    // chip pcie initialization space
				//
				case Qc9KSetEepromDeviceId:
					status=Qc98xxDeviceIDGet(&value[0]);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, 1);
					break;
				case Qc9KSetEepromSSID:
					status=Qc98xxSSIDGet(&value[0]);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, 1);
					break;
				case Qc9KSetEepromVID:
					status=Qc98xxVendorGet(&value[0]);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, 1);
					break;
				case Qc9KSetEepromSVID:
					status=Qc98xxSubVendorGet(&value[0]);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, 1);
					break;
				case Qc9KSetEepromConfig:
					ngot = 0;
					while (1)
					{
						status = Qc98xxPcieAddressValueDataOfNumGet(ngot, &uvalue[ngot*2], &uvalue[ngot*2+1]);
						ngot++;
						if(status!=VALUE_OK) break;
					}
					if(ngot-1 > 0)
					{
						ReturnHex("get",GetParameter[index].word[0],"",(int *)uvalue,(ngot-1)*2);
						done++;
					}
					else
					{
						error++;
					}
					break;
#ifdef UNUSED
				case Qc9KSetEepromConfigPCIe:
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
                case Qc9KSetEepromVersion:
                    value[0]= Qc98xxEepromVersionGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromTemplateVersion:
                    value[0]=Qc98xxTemplateVersionGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromMacAddress:
                    status=Qc98xxMacAddressGet(cmac);
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
                case Qc9KSetEepromCustomerData:
                    status=Qc98xxCustomerDataGet(buffer,CUSTOMER_DATA_SIZE);
                    if(status==VALUE_OK)
                    {
                        ReturnText("get",GetParameter[index].word[0],"",buffer);
                        done++;
                    }
                    else // ErrorPrint();
                        error++;
                    break;
                case Qc9KSetEepromRegulatoryDomain:
                    status=Qc98xxRegDmnGet(value, ix, &num);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEepromTxRxMask:
                    value[0]=Qc98xxTxRxMaskGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromTxRxMaskTx:
                    value[0]=Qc98xxTxMaskGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromTxRxMaskRx:
                    value[0]=Qc98xxRxMaskGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromOpFlags:
                    value[0]=Qc98xxOpFlagsGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromOpFlags2:
                    value[0]=Qc98xxOpFlags2Get();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromBoardFlags:
                    value[0]=Qc98xxBoardFlagsGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromRfSilent:
                    value[0]=Qc98xxRfSilentGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromRfSilentB0:
                    value[0]=Qc98xxRfSilentB0Get();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromRfSilentB1:
                    value[0]=Qc98xxRfSilentB1Get();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromRfSilentGpio:
                    value[0]=Qc98xxRfSilentGPIOGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromBlueToothOptions:
                    value[0]=Qc98xxBlueToothOptionsGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromPowerTableOffset:
                    value[0]=Qc98xxPwrTableOffsetGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromTuningCaps:
					status=Qc98xxPwrTuningCapsParamsGet(value, ix, &num);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
				case Qc9KSetEepromDeltaCck20:
                    value[0]=Qc98xxDeltaCck20Get();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Qc9KSetEepromDelta4020:
                    value[0]=Qc98xxDelta4020Get();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
				case Qc9KSetEepromDelta8020:
                    value[0]=Qc98xxDelta8020Get();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
					break;
                case Qc9KSetEepromFeatureEnable:
                    value[0]=Qc98xxEnableFeatureGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromFeatureEnableTemperatureCompensation:
                    value[0]=Qc98xxEnableTempCompensationGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromFeatureEnableVoltageCompensation:
                    value[0]=Qc98xxEnableVoltCompensationGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                //case Qc9KSetEepromFeatureEnableFastClock:
                //    value[0]=Qc98xxEnableFastClockGet();
                //    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                //    break;
                case Qc9KSetEepromFeatureEnableDoubling:
                    value[0]=Qc98xxEnableDoublingGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromFeatureEnableInternalSwitchingRegulator:
                    value[0]=Qc98xxInternalRegulatorGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromFeatureEnablePaPredistortion:
                    value[0]=Qc98xxPapdGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromEnableRbias:
                    value[0]=Qc98xxRbiasGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromFeatureEnableTuningCaps:
                    value[0]=Qc98xxEnableTuningCapsGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromMiscellaneous:
                    value[0]=Qc98xxReconfigMiscGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromMiscellaneousDriveStrength:
                    value[0]=Qc98xxReconfigDriveStrengthGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromMiscellaneousThermometer:
                    value[0]=Qc98xxThermometerGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromMiscellaneousChainMaskReduce:
                    value[0]=Qc98xxChainMaskReduceGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromMiscellaneousQuickDropEnable:
                    value[0]=Qc98xxReconfigQuickDropGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromMiscellaneousEep:
                    value[0]=Qc98xxEepMiscGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromBibxosc0:
                    value[0]=Qc98xxBibxosc0Get();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromFlag1NoiseFlrThr:
                    status=Qc98xxFlag1NoiseFlrThrGet(value);
                    ReturnGet(ix,iy,iz, status, &done, &error, ip, index, value, 1);
                    break;
				case Qc9KSetEepromWlanLedGpio:
                    value[0]=Qc98xxWlanLedGpioGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
				case Qc9KSetEepromSpurBaseA:
                    value[0]=Qc98xxSpurBaseAGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
				case Qc9KSetEepromSpurBaseB:
                    value[0]=Qc98xxSpurBaseBGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
				case Qc9KSetEepromSpurRssiThresh:
                    value[0]=Qc98xxSpurRssiThreshGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
				case Qc9KSetEepromSpurRssiThreshCck:
                    value[0]=Qc98xxSpurRssiThreshCckGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
				case Qc9KSetEepromSpurMitFlag:
                    value[0]=Qc98xxSpurMitFlagGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromTxRxGain:
                    value[0]=Qc98xxTxRxGainGet(band_both);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromTxRxGainTx:
                    value[0]=Qc98xxTxGainGet(band_both);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromTxRxGainRx:
                    value[0]=Qc98xxRxGainGet(band_both);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEepromSwregProgram:
                    break;
                case Qc9KSetEepromSwreg:
                    value[0]=Qc98xxSWREGGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom2GHzAntennaControlCommon:
                    value[0]=Qc98xxAntCtrlCommonGet(band_BG);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom2GHzAntennaControlCommon2:
                    value[0]=Qc98xxAntCtrlCommon2Get(band_BG);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom2GHzAntennaControlChain:
                    status=Qc98xxAntCtrlChainGet(value, ix, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
				case Qc9KSetEeprom2GHzRxFilterCap:
                    value[0]=Qc98xxRxFilterCapGet(band_BG);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
				case Qc9KSetEeprom2GHzRxGainCap:
                    value[0]=Qc98xxRxGainCapGet(band_BG);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom2GHzTxRxGainTx:
                    value[0]=Qc98xxTxGainGet(band_BG);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom2GHzTxRxGainRx:
                    value[0]=Qc98xxRxGainGet(band_BG);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
				case Qc9KSetEeprom2GHzNoiseFlrThr:
					status=Qc98xxNoiseFlrThrGet(value, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, 1);
					break;
				case Qc9KSetEeprom2GHzMinCcaPwrChain:
					status=Qc98xxMinCcaPwrChainGet(value, ix, &num, band_BG);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
                case Qc9KSetEeprom2GHzXatten1Db:
                    status=Qc98xxXatten1DBGet(value, ix, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom2GHzXatten1Margin:
                    status=Qc98xxXatten1MarginGet(value, ix, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom2GHzVoltageSlope:
                    status=Qc98xxVoltSlopeGet(value, ix, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom2GHzSpur:
                    status=Qc98xxSpurChansGet(value, ix, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom2GHzSpurAPrimSecChoose:
                    status=Qc98xxSpurAPrimSecChooseGet(value, ix, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom2GHzSpurBPrimSecChoose:
                    status=Qc98xxSpurBPrimSecChooseGet(value, ix, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom2GHzXpaBiasLevel:
                    value[0]=Qc98xxXpaBiasLvlGet(band_BG);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom2GHzXpaBiasBypass:
                    value[0]=Qc98xxXpaBiasBypassGet(band_BG);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom2GHzAntennaGain:
                    value[0]=Qc98xxAntennaGainGet(band_BG);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, num);
                    break;
#if 0
                case Qc9KSetEeprom2GHzAttenuation1Hyst:
                    status=Qc98xxXatten1HystGet(value, ix, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom2GHzAttenuation2Db:
                    status=Qc98xxXatten2DBGet(value, ix, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom2GHzAttenuation2Margin:
                    status=Qc98xxXatten2MarginGet(value, ix, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom2GHzAttenuation2Hyst:
                    status=Qc98xxXatten2HystGet(value, ix, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom2GHzNoiseFloorThreshold:
                    status=Qc98xxNoiseFloorThreshChGet(value, ix, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                //case Qc9KSetEeprom2GHzQuickDrop:
                //    value[0]=Qc98xxQuickDropGet(band_BG);
                //    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                //    break;
                case Qc9KSetEeprom2GHzTxFrameToDataStart:
                    value[0]=Qc98xxTxFrameToDataStartGet(band_BG);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom2GHzTxFrameToPaOn:
                    value[0]=Qc98xxTxFrameToPaOnGet(band_BG);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom2GHzTxClip:
                    value[0]=Qc98xxTxClipGet(band_BG);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom2GHzDacScaleCCK:
                    value[0]=Qc98xxDacScaleCckGet(band_BG);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom2GHzSwitchSettling:
                    value[0]=Qc98xxSwitchSettlingGet(band_BG);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom2GHzSwitchSettlingHt40:
                    value[0]=Qc98xxSwitchSettlingHt40Get(band_BG);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom2GHzAdcSize:
                    value[0]=Qc98xxAdcDesiredSizeGet(band_BG);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom2GHzTxEndToXpaOff:
                    value[0]=Qc98xxTxEndToXpaOffGet(band_BG);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom2GHzTxEndToRxOn:
                    value[0]=Qc98xxTxEndToRxOnGet(band_BG);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom2GHzTxFrameToXpaOn:
                    value[0]=Qc98xxTxFrameToXpaOnGet(band_BG);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom2GHzThresh62:
                    value[0]=Qc98xxThresh62Get(band_BG);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                //case Qc9KSetEeprom2GHzPaPredistortionHt20:
                //    value[0]=Qc98xxPapdRateMaskHt20Get(band_BG);
                //    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                //    break;
                //case Qc9KSetEeprom2GHzPaPredistortionHt40:
                //    value[0]=Qc98xxPapdRateMaskHt40Get(band_BG);
                //    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                //    break;
                //case Qc9KSetEeprom2GHzFuture:
                //    Qc98xxFutureGet(value, ix, &num, band_BG);
                //    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, num);
                //    break;
                //case Qc9KSetEepromCommonFuture:
                //    Qc98xxFutureGet(value, ix, &num, -1);
                //    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, num);
                //    break;
                //case Qc9KSetEepromAntennaDiversityControl:
                //    value[0]=Qc98xxAntDivCtrlGet();
                //    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                //    break;
#endif //0
				case Qc9KSetEepromThermAdcScaledGain:
                    value[0]=Qc98xxThermAdcScaledGainGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
				case Qc9KSetEepromThermAdcOffset:
                    value[0]=Qc98xxThermAdcOffsetGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom2GHzCalibrationFrequency:
                    status=Qc98xxCalFreqPierGet(value, ix, 0, 0, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
				case Qc9KSetEeprom2GHzCalPointTxGainIdx:
                    status=Qc98xxCalPointTxGainIdxGet(value, ix, iy, iz, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
				case Qc9KSetEeprom2GHzCalPointDacGain:
                    status=Qc98xxCalPointDacGainGet(value, ix, iy, iz, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
				case Qc9KSetEeprom2GHzCalPointPower:
                    status=Qc98xxCalPointPowerGet(fvalue, ix, iy, iz, &num, band_BG);
                    ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
					break;
                case Qc9KSetEeprom2GHzCalibrationVoltage:
                    status=Qc98xxCalPierDataVoltMeasGet(value, ix, 0, 0, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom2GHzCalibrationTemperature:
                    status=Qc98xxCalPierDataTempMeasGet(value, ix, 0, 0, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                    break;
                case Qc9KSetEeprom2GHzTargetFrequencyCck:
                    status=Qc98xxCalFreqTGTCckGet(value, ix, 0, 0, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom2GHzTargetFrequency:
                    status=Qc98xxCalFreqTGTLegacyOFDMGet(value, ix, 0, 0, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom2GHzTargetFrequencyVht20:
                    status=Qc98xxCalFreqTGTHT20Get(value, ix, 0, 0, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom2GHzTargetFrequencyVht40:
                    status=Qc98xxCalFreqTGTHT40Get(value, ix, 0, 0, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom2GHzTargetPowerCck:
                    status=Qc98xxCalTGTPwrCCKGet(fvalue, ix, iy, 0, &num, band_BG);
                    ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
                    break;
                case Qc9KSetEeprom2GHzTargetPower:
                    status=Qc98xxCalTGTPwrLegacyOFDMGet(fvalue, ix, iy, 0, &num, band_BG);
                    ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
                    break;
                case Qc9KSetEeprom2GHzTargetPowerVht20:
                    status=Qc98xxCalTGTPwrHT20Get(fvalue, ix, iy, 0, &num, band_BG);
                    ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
                    break;
                case Qc9KSetEeprom2GHzTargetPowerVht40:
                    status=Qc98xxCalTGTPwrHT40Get(fvalue, ix, iy, 0, &num, band_BG);
                    ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
                    break;
                case Qc9KSetEeprom2GHzCtlIndex:
                    status=Qc98xxCtlIndexGet(value, ix, iy, 0, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom2GHzCtlFrequency:
                    status=Qc98xxCtlFreqGet(value, ix, iy, 0, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom2GHzCtlPower:
                    status=Qc98xxCtlPowerGet(fvalue, ix, iy, 0, &num, band_BG);
                    ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
                    break;
                case Qc9KSetEeprom2GHzCtlBandEdge:
                    status=Qc98xxCtlFlagGet(value, ix, iy, 0, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
			    case Qc9KSetEeprom2GHzNoiseFloor:
				    status=Qc98xxNoiseFloorGet(value, ix, iy, 0, &num, band_BG);
				    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
				    break;
			    case Qc9KSetEeprom2GHzNoiseFloorPower:
				    status=Qc98xxNoiseFloorPowerGet(value, ix, iy, 0, &num, band_BG);
				    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
				    break;
			    case Qc9KSetEeprom2GHzNoiseFloorTemperature:
				    status=Qc98xxNoiseFloorTemperatureGet(value, ix, 0, 0, &num, band_BG);
				    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
				    break;
                case Qc9KSetEeprom2GHzNoiseFloorTemperatureSlope:
                    status=Qc98xxNoiseFloorTemperatureSlopeGet(value, ix, 0, 0, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
				case Qc9KSetEeprom2GHzAlphaThermTable:
                    status=Qc98xxAlphaThermTableGet(value, ix, iy, iz, &num, band_BG);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;

                case Qc9KSetEeprom5GHzAntennaControlCommon:
                    value[0]=Qc98xxAntCtrlCommonGet(band_A);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom5GHzAntennaControlCommon2:
                    value[0]=Qc98xxAntCtrlCommon2Get(band_A);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom5GHzAntennaControlChain:
                    status=Qc98xxAntCtrlChainGet(value, ix, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
				case Qc9KSetEeprom5GHzRxFilterCap:
                    value[0]=Qc98xxRxFilterCapGet(band_A);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
				case Qc9KSetEeprom5GHzRxGainCap:
                    value[0]=Qc98xxRxGainCapGet(band_A);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom5GHzTxRxGainTx:
                    value[0]=Qc98xxTxGainGet(band_A);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom5GHzTxRxGainRx:
                    value[0]=Qc98xxRxGainGet(band_A);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
				case Qc9KSetEeprom5GHzNoiseFlrThr:
					status=Qc98xxNoiseFlrThrGet(value, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, 1);
					break;
				case Qc9KSetEeprom5GHzMinCcaPwrChain:
					status=Qc98xxMinCcaPwrChainGet(value, ix, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
                case Qc9KSetEeprom5GHzXatten1DbLow:
                    status=Qc98xxXatten1DBLowGet(value, ix, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom5GHzXatten1DbMid:
                    status=Qc98xxXatten1DBGet(value, ix, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom5GHzXatten1DbHigh:
                    status=Qc98xxXatten1DBHighGet(value, ix, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom5GHzXatten1MarginLow:
                    status=Qc98xxXatten1MarginLowGet(value, ix, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom5GHzXatten1MarginMid:
                    status=Qc98xxXatten1MarginGet(value, ix, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom5GHzXatten1MarginHigh:
                    status=Qc98xxXatten1MarginHighGet(value, ix, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom5GHzVoltageSlope:
                    status=Qc98xxVoltSlopeGet(value, ix, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom5GHzSpur:
                    status=Qc98xxSpurChansGet(value, ix, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom5GHzSpurAPrimSecChoose:
                    status=Qc98xxSpurAPrimSecChooseGet(value, ix, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom5GHzSpurBPrimSecChoose:
                    status=Qc98xxSpurBPrimSecChooseGet(value, ix, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom5GHzXpaBiasLevel:
                    value[0]=Qc98xxXpaBiasLvlGet(band_A);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom5GHzXpaBiasBypass:
                    value[0]=Qc98xxXpaBiasBypassGet(band_A);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom5GHzAntennaGain:
                    value[0]=Qc98xxAntennaGainGet(band_A);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, num);
                    break;
#if 0
                case Qc9KSetEeprom5GHzAttenuation1Hyst:
                    status=Qc98xxXatten1HystGet(value, ix, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom5GHzAttenuation2Db:
                    status=Qc98xxXatten2DBGet(value, ix, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom5GHzAttenuation2Margin:
                    status=Qc98xxXatten2MarginGet(value, ix, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom5GHzAttenuation2Hyst:
                    status=Qc98xxXatten2HystGet(value, ix, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom5GHzTemperatureSlope:
                    status=Qc98xxTempSlopeGet(value, ix, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom5GHzTemperatureSlopeLow:
                    status=Qc98xxTempSlopeLowGet(value);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom5GHzTemperatureSlopeHigh:
                    status=Qc98xxTempSlopeHighGet(value);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom5GHzNoiseFloorThreshold:
                    status=Qc98xxNoiseFloorThreshChGet(value, ix, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                //case Qc9KSetEeprom5GHzQuickDrop:
                //    value[0]=Qc98xxQuickDropGet(band_A);
                //    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                //    break;
                //case Qc9KSetEeprom5GHzQuickDropLow:
                //    value[0]=Qc98xxQuickDropLowGet(band_A);
                //    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                //    break;
                //case Qc9KSetEeprom5GHzQuickDropHigh:
                //    value[0]=Qc98xxQuickDropHighGet(band_A);
                //    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                //    break;
                case Qc9KSetEeprom5GHzTxFrameToDataStart:
                    value[0]=Qc98xxTxFrameToDataStartGet(band_A);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom5GHzTxFrameToPaOn:
                    value[0]=Qc98xxTxFrameToPaOnGet(band_A);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom5GHzTxClip:
                    value[0]=Qc98xxTxClipGet(band_A);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom5GHzSwitchSettling:
                    value[0]=Qc98xxSwitchSettlingGet(band_A);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom5GHzSwitchSettlingHt40:
                    value[0]=Qc98xxSwitchSettlingHt40Get(band_A);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom5GHzAdcSize:
                    value[0]=Qc98xxAdcDesiredSizeGet(band_A);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom5GHzTxEndToXpaOff:
                    value[0]=Qc98xxTxEndToXpaOffGet(band_A);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom5GHzTxEndToRxOn:
                    value[0]=Qc98xxTxEndToRxOnGet(band_A);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom5GHzTxFrameToXpaOn:
                    value[0]=Qc98xxTxFrameToXpaOnGet(band_A);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                case Qc9KSetEeprom5GHzThresh62:
                    value[0]=Qc98xxThresh62Get(band_A);
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;
                //case Qc9KSetEeprom5GHzPaPredistortionHt20:
                //    value[0]=Qc98xxPapdRateMaskHt20Get(band_A);
                //    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                //    break;
                //case Qc9KSetEeprom5GHzPaPredistortionHt40:
                //    value[0]=Qc98xxPapdRateMaskHt40Get(band_A);
                //    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                //    break;
                //case Qc9KSetEeprom5GHzFuture:
                //    Qc98xxFutureGet(value, ix, &num, band_A);
                //    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, num);
                //    break;
#endif //0
                case Qc9KSetEeprom5GHzCalibrationFrequency:
                    status=Qc98xxCalFreqPierGet(value, ix, 0, 0, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
				case Qc9KSetEeprom5GHzCalPointTxGainIdx:
                    status=Qc98xxCalPointTxGainIdxGet(value, ix, iy, iz, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
				case Qc9KSetEeprom5GHzCalPointDacGain:
                    status=Qc98xxCalPointDacGainGet(value, ix, iy, iz, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
				case Qc9KSetEeprom5GHzCalPointPower:
                    status=Qc98xxCalPointPowerGet(fvalue, ix, iy, iz, &num, band_A);
                    ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
					break;
                case Qc9KSetEeprom5GHzCalibrationVoltage:
                    status=Qc98xxCalPierDataVoltMeasGet(value, ix, 0, 0, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom5GHzCalibrationTemperature:
                    status=Qc98xxCalPierDataTempMeasGet(value, ix, 0, 0, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom5GHzTargetFrequency:
                    status=Qc98xxCalFreqTGTLegacyOFDMGet(value, ix, 0, 0, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom5GHzTargetFrequencyVht20:
                    status=Qc98xxCalFreqTGTHT20Get(value, ix, 0, 0, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom5GHzTargetFrequencyVht40:
                    status=Qc98xxCalFreqTGTHT40Get(value, ix, 0, 0, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                 case Qc9KSetEeprom5GHzTargetFrequencyVht80:
                    status=Qc98xxCalFreqTGTHT80Get(value, ix, 0, 0, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
               case Qc9KSetEeprom5GHzTargetPower:
                    status=Qc98xxCalTGTPwrLegacyOFDMGet(fvalue, ix, iy, 0, &num, band_A);
                    ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
                    break;
                case Qc9KSetEeprom5GHzTargetPowerVht20:
                    status=Qc98xxCalTGTPwrHT20Get(fvalue, ix, iy, 0, &num, band_A);
                    ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
                    break;
                case Qc9KSetEeprom5GHzTargetPowerVht40:
                    status=Qc98xxCalTGTPwrHT40Get(fvalue, ix, iy, 0, &num, band_A);
                    ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
                    break;
                case Qc9KSetEeprom5GHzTargetPowerVht80:
                    status=Qc98xxCalTGTPwrHT80Get(fvalue, ix, iy, 0, &num, band_A);
                    ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
                    break;
                case Qc9KSetEeprom5GHzCtlIndex:
                    status=Qc98xxCtlIndexGet(value, ix, iy, 0, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom5GHzCtlFrequency:
                    status=Qc98xxCtlFreqGet(value, ix, iy, 0, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom5GHzCtlPower:
                    status=Qc98xxCtlPowerGet(fvalue, ix, iy, 0, &num, band_A);
                    ReturnGetf(ix,iy,iz,status, &done, &error, ip, index, fvalue, num);
                    break;
                case Qc9KSetEeprom5GHzCtlBandEdge:
                    status=Qc98xxCtlFlagGet(value, ix, iy, 0, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
                case Qc9KSetEeprom5GHzNoiseFloor:
				     status=Qc98xxNoiseFloorGet(value, ix, iy, 0, &num, band_A);
				     ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
				     break;
                case Qc9KSetEeprom5GHzNoiseFloorPower:
					status=Qc98xxNoiseFloorPowerGet(value, ix, iy, 0, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
			    case Qc9KSetEeprom5GHzNoiseFloorTemperature:
					status=Qc98xxNoiseFloorTemperatureGet(value, ix, 0, 0, &num, band_A);
					ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
					break;
                case Qc9KSetEeprom5GHzNoiseFloorTemperatureSlope:
                    status=Qc98xxNoiseFloorTemperatureSlopeGet(value, ix, 0, 0, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
				case Qc9KSetEeprom5GHzAlphaThermTable:
                    status=Qc98xxAlphaThermTableGet(value, ix, iy, iz, &num, band_A);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;
				case Qc9KSetEepromConfigAddr:
                    status=Qc98xxConfigAddrGet(value, ix, &num);
                    ReturnGet(ix,iy,iz,status, &done, &error, ip, index, value, num);
                    break;

                // non-eeprom parameters
                case DevSetStbc:
                    value[0]=DevStbcGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;

                case DevSetLdpc:
                    value[0]=DevLdpcGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
                    break;

                case DevSetUsbManufacturerString:
                    UserPrint("[Get]Ar6KSetUsbManufacturerString\n");
                    status = DevUsbManufacturerStringGet(buffer,20);
                    if(status==VALUE_OK)
					{
						ReturnText("get",GetParameter[index].word[0],"",buffer);
                        done++;
					}
                    else
                        error++;
					break;
				case DevSetUsbProductString:
					UserPrint("[Get]Ar6KSetUsbProductString\n");
					status = DevUsbProductStringGet(buffer,32);
                    if(status==VALUE_OK)
					{
						ReturnText("get",GetParameter[index].word[0],"",buffer);
                        done++;
					}
                    else
                        error++;
					break;
				case DevSetUsbSerialString:
					UserPrint("[Get]Ar6KSetUsbSerialString\n");
					status = DevUsbSerialStringGet(buffer,16);
                    if(status==VALUE_OK)
					{
						ReturnText("get",GetParameter[index].word[0],"",buffer);
                        done++;
					}
                    else
                        error++;
					break;
                case DevSetNonCenterFreqAllowed:
                    value[0]=DevNonCenterFreqAllowedGet();
                    ReturnGet(ix,iy,iz,VALUE_OK, &done, &error, ip, index, value, 1);
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

