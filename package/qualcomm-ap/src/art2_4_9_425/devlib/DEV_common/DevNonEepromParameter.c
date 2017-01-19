#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "wlantype.h"
#include "dk_cmds.h"
#include "DevSetConfig.h"
#ifdef AR6004_BUILD
#include "AR6K_version.h"
#else
#include "sw_version.h"
#endif

#ifdef QDART_BUILD
#include "qmslCmd.h"
#endif 

#define USB_MANUFACTURER_STRING_MAX 20
#define USB_PRODUCT_STRING_MAX 32
#define USB_SERIAL_STRING_NAX 16

static A_UINT16 TxDataStartMiscFlags = 0;
static A_UINT16 DEVICE_VID = 0;
static A_UINT16 DEVICE_PID = 0;
static A_UINT16 REMOTE_WAKEUP = 0;
static A_UCHAR UsbManufacturerString[USB_MANUFACTURER_STRING_MAX]={0x00};
static A_UCHAR UsbProductString[USB_PRODUCT_STRING_MAX]={0x00};
static A_UCHAR UsbSerialString[USB_SERIAL_STRING_NAX]={0x00};
static A_UINT8 NotCenterFreqAllowed = 1;

//
// Set Funtions
//

int DevStbcSet(int value)
{
    TxDataStartMiscFlags = (TxDataStartMiscFlags & ~DESC_STBC_ENA_MASK) | 
                           ((value & 1) << DESC_STBC_ENA_SHIFT);
    return 0;
}

int  DevLdpcSet(int value)
{
    TxDataStartMiscFlags = (TxDataStartMiscFlags & ~DESC_LDPC_ENA_MASK) |
                           ((value & 1) << DESC_LDPC_ENA_SHIFT);
    return 0;
}

int DevSvidSet(A_UINT16 value)
{
	DEVICE_VID = value;
	return 0;
}

int DevSsidSet(A_UINT16 value)
{
	DEVICE_PID = value;
	return 0;
}

int DevRemoteWakeupSet(A_UINT16 value)
{
	REMOTE_WAKEUP = value;
	return 0;
}

int DevNonCenterFreqAllowedSet(int value)
{
    NotCenterFreqAllowed = value == 0 ? 0 : 1;
	return 0;
}

//
// Get Functions
//

int DevStbcGet()
{
    return ((TxDataStartMiscFlags >> DESC_STBC_ENA_SHIFT) & 1);
}

int DevLdpcGet()
{
    return ((TxDataStartMiscFlags >> DESC_LDPC_ENA_SHIFT) & 1);
}
A_UINT16 DevSvidGet()
{
	return (DEVICE_VID & 0xFFFF);
}

A_UINT16 DevSsidGet()
{
	return (DEVICE_PID & 0xFFFF);
}

A_UINT16 DevRemoteWakeupGet()
{
	return (REMOTE_WAKEUP & 1);
}

int DevUsbManufacturerStringGet(A_UCHAR *data, int maxlength)
{
	int length, i;
	length = (maxlength>USB_MANUFACTURER_STRING_MAX) ? USB_MANUFACTURER_STRING_MAX : maxlength;
	if(length == 0)
		length = USB_MANUFACTURER_STRING_MAX;

	for (i=0; i<length; i++)
	{
		data[i] = UsbManufacturerString[i];
	}
	for(i=length; i<maxlength; i++)
	{
		data[i]=0;
	}
    return 0;
}

int DevUsbProductStringGet(A_UCHAR *data, int maxlength)
{
	int length, i;
	length = (maxlength>USB_PRODUCT_STRING_MAX) ? USB_PRODUCT_STRING_MAX : maxlength;
	if(length == 0)
		length = USB_PRODUCT_STRING_MAX;

	for (i=0; i<length; i++)
	{
		data[i] = UsbProductString[i];
	}
	for(i=length; i<maxlength; i++)
	{
		data[i]=0;
	}
    return 0;
}

int DevUsbSerialStringGet(A_UCHAR *data, int maxlength)
{
	int length, i;
	length = (maxlength>USB_SERIAL_STRING_NAX) ? USB_SERIAL_STRING_NAX : maxlength;
	if(length == 0)
		length = USB_SERIAL_STRING_NAX;

	for (i=0; i<length; i++)
	{
		data[i] = UsbSerialString[i];
	}
	for(i=length; i<maxlength; i++)
	{
		data[i]=0;
	}
    return 0;
}

A_UINT32 DevFirmwareVersionGet(A_UCHAR *data)
{
    A_UINT32 swVersion = configSetup.SwVersion;
    sprintf(data,"%d.%d.%d build %d", 
                (unsigned int)((swVersion & VER_MAJOR_BIT_MASK) >> VER_MAJOR_BIT_OFFSET),
                (unsigned int)((swVersion & VER_MINOR_BIT_MASK) >> VER_MINOR_BIT_OFFSET),
                (unsigned int)((swVersion & VER_PATCH_BIT_MASK) >> VER_PATCH_BIT_OFFSET),
                (unsigned int)((swVersion & VER_BUILD_NUM_BIT_MASK) >> VER_BUILD_NUM_BIT_OFFSET));
    return 0;
}

int DevUsbManufacturerStringSet(A_UCHAR *data, int maxlength)
{
	int length, i;
	length = strlen(data);

	length = (length>USB_PRODUCT_STRING_MAX) ? USB_PRODUCT_STRING_MAX : length;

	for (i=0; i<length; i++)
	{
		UsbManufacturerString[i] = data[i];
	}
	for(i=length; i<maxlength; i++)
	{
		UsbManufacturerString[i]=0;
	}
    return 0;
}

int DevUsbProductStringSet(A_UCHAR *data, int maxlength)
{
	int length, i;
	length = strlen(data);

	length = (length>USB_PRODUCT_STRING_MAX) ? USB_PRODUCT_STRING_MAX : length;
	
	for (i=0; i<length; i++)
	{
		UsbProductString[i] = data[i];
	}
	for(i=length; i<maxlength; i++)
	{
		UsbProductString[i]=0;
	}
    return 0;
}

int DevUsbSerialStringSet(A_UCHAR *data, int maxlength)
{
	int length, i;
	length = strlen(data);

	length = (length>USB_PRODUCT_STRING_MAX) ? USB_PRODUCT_STRING_MAX : length;

	for (i=0; i<length; i++)
	{
		UsbSerialString[i] = data[i];
	}
	for(i=length; i<maxlength; i++)
	{
		UsbSerialString[i]=0;
	}
    return 0;
}

int DevNonCenterFreqAllowedGet()
{
    return NotCenterFreqAllowed;
}
