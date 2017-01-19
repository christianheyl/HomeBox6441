
#ifndef _DEV_SET_LIST_H_
#define _DEV_SET_LIST_H_

#include "DevNonEepromParameter.h"


static int ThermometerMinimum=-1;
static int ThermometerMaximum=2;
static int ThermometerDefault=1;

static unsigned int TwoBitsMinimum=0;
static unsigned int TwoBitsMaximum=0x3;
static unsigned int TwoBitsDefault=0;

static unsigned int SixBitsMinimum=0;
static unsigned int SixBitsMaximum=0x3f;
static unsigned int SixBitsDefault=0;

static unsigned int HalfByteMinimum=0;
static unsigned int HalfByteMaximum=0xf;
static unsigned int HalfByteDefault=0;

static unsigned int UnsignedByteMinimum=0;
static unsigned int UnsignedByteMaximum=0xff;
static unsigned int UnsignedByteDefault=0;

static int SignedByteMinimum=-127;
static int SignedByteMaximum=127;
static int SignedByteDefault=0;

static unsigned int UnsignedShortMinimum=0;
static unsigned int UnsignedShortMaximum=0xffff;
static unsigned int UnsignedShortDefault=0;

static int SignedShortMinimum=-32767;
static int SignedShortMaximum=32767;
static int SignedShortDefault=0;

static unsigned int UnsignedIntMinimum=0;
static unsigned int UnsignedIntMaximum=0xffffffff;
static unsigned int UnsignedIntDefault=0;

static int SignedIntMinimum= -1000000;
static int SignedIntMaximum=0x3fffffff;
static int SignedIntDefault=0;

static int FreqZeroMinimum=0;
static int FrequencyMinimum2GHz=2300;
static int FrequencyMaximum2GHz=2600;
static int FrequencyDefault2GHz=2412;

static int FrequencyMinimum5GHz=4000;
static int FrequencyMaximum5GHz=7000;
static int FrequencyDefault5GHz=5180;

static double PowerMinimum=0.0;
static double PowerMaximum=35.0;
static double PowerDefault=10.0;

static int PowerOffsetMinimum=-10;
static int PowerOffsetMaximum=35;
static int PowerOffsetDefault=0;

static struct _ParameterList LogicalParameter[]=
{
    {0,{"no",0,0},0,0,0,0,0,0,0,0,0,0,0},
    {1,{"yes",0,0},0,0,0,0,0,0,0,0,0,0,0},
};

// Non-eeprom parameters
#define DEV_STBC {DevSetStbc,{DevStbc,0,0},"non-eeprom parameter to enable STBC in transmission",'z',0,1,1,1,\
    0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define DEV_LDPC {DevSetLdpc,{DevLdpc,0,0},"non-eeprom parameter to enable LDPC in transmission",'z',0,1,1,1,\
    0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define DEV_REMOTE_WAKEUP {DevSetRemoteWakeup,{DevRemoteWakeup,"remotewakeup",0},"non-eeprom parameter to enable USB remote wakeup",'z',0,1,1,1,\
    0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define DEV_USB_MANUFACTURER_STRING {DevSetUsbManufacturerString,{DevUsbManufacturerString,"usbmanu",0},"any text, usually used for device serial number",'t',0,1,1,1,\
    0,0,0,0,0}

#define DEV_USB_PRODUCT_STRING {DevSetUsbProductString,{DevUsbProductString,"usbproc",0},"any text, usually used for device serial number",'t',0,1,1,1,\
    0,0,0,0,0}

#define DEV_USB_SERIAL_STRING {DevSetUsbSerialString,{DevUsbSerialString,"usbseri",0},"any text, usually used for device serial number",'t',0,1,1,1,\
    0,0,0,0,0}

#define DEV_FIRMWARE_VERSION {DevGetFirmwareVersion,{"Devfirmwareversion","fwver","fwversion"}, "Get Firmware version only",'z',0,1,1,1,\
        0,0,0,0,0}

#define DEV_SET_2G_PSAT_PWOER {DevSet2GPastPower,{Dev2GPastPower,"2gpsatpower",0},"2G Past power",'f',"dBm",2,3,1,\
    &PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define DEV_SET_5G_PSAT_PWOER {DevSet5GPastPower,{Dev5GPastPower,"5gpsatpower",0},"5G Past power",'f',"dBm",2,8,1,\
    &PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define DEV_SET_2G_DIFF_OFDM_CW_POWER {DevSet2GDiff_OFDM_CW_Power,{Dev2GDiff_OFDM_CW_Power,"2gpsatdiff",0},"2G Psat diff OFDM and CWTone",'f',"dB",2,3,1,\
    0,0,0,0,0}

#define DEV_SET_5G_DIFF_OFDM_CW_POWER {DevSet5GDiff_OFDM_CW_Power,{Dev5GDiff_OFDM_CW_Power,"5gpsatdiff",0},"5G Psat diff OFDM and CWTone",'f',"dB",2,8,1,\
    0,0,0,0,0}

#define DEV_NON_CENTER_FREQ_ALLOWED {DevSetNonCenterFreqAllowed,{DevNonCenterFreqAllowed,0,0},"non-center frequency is allowed in tx and rx",'z',0,1,1,1,\
    0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#endif //_DEV_SET_LIST_H_