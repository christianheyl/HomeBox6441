
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>

#include "smatch.h"
#include "UserPrint.h"
#include "TimeMillisecond.h"
#include "NartError.h"


#include "wlantype.h"

#if !defined(LINUX) && !defined(__APPLE__)
#include "osdep.h"
#endif


#include "ErrorPrint.h"
#include "ah.h"
#include "ah_internal.h"
#include "ar5416/ar5416eep.h"
#include "ar5416/ar9287eep.h"
#include "Ar9287EepromPrint.h"
#include "Ar9287EepromParameter.h"
#include "kiwimac.h"
#include "ar9287/mEepStruct9287.h"

//
// This fun structure contains the names, sizes, and types for all of the fields in the eeprom structure.
// It exists so that we can easily compare two structures and report the differences.
// It must match the definition of ar9287_eeprom_t in ar9300eep.h.
// Fields must be defined in the same order as they occur in the structure.
//

#define MPRINTBUFFER 32

#define MBLOCK 32

#define MBUFFER 1024

enum {
    ch0 = 0,
    ch1,
    ch2,
    ch0_ch1,
    all_chains
};

struct _EepromPrint
{
	char *name;
	short offset;
	char size;
	short nx,ny,nz;
	char type;	// f -- floating point
				// d -- decimal
				// x -- hexadecimal
				// u -- unsigned
				// c -- character
				// t -- text
				// p -- transmit power, 0.5*value as floating point 
				// 2 -- compressed 2GHz frequency
				// 5 -- compressed 5GHz frequency
				// m -- mac address
	int interleave;
	int high;
	int low;
	int voff;
};

static struct _EepromPrint _Ar9287EepromList[]=
{

	{Ar9287EepromLength,offsetof(ar9287_eeprom_t,baseEepHeader.length),sizeof(u_int16_t),1,1,1,'u',1,-1,-1,0},
	{Ar9287EepromChecksum,offsetof(ar9287_eeprom_t,baseEepHeader.checksum),sizeof(u_int16_t),1,1,1,'u',1,-1,-1,0},
	{Ar9287EepromVersion,offsetof(ar9287_eeprom_t,baseEepHeader.version),sizeof(u_int16_t),1,1,1,'u',1,-1,-1,0},
	{Ar9287EepromOpFlags,offsetof(ar9287_eeprom_t,baseEepHeader.opCapFlags),sizeof(u_int8_t),1,1,1,'u',1,-1,-1,0},
	{Ar9287EepromEepMisc,offsetof(ar9287_eeprom_t,baseEepHeader.eepMisc),sizeof(u_int8_t),1,1,1,'u',1,-1,-1,0},
	{Ar9287EepromRegulatoryDomain,offsetof(ar9287_eeprom_t,baseEepHeader.regDmn),2,sizeof(u_int16_t),1,1,'u',1,-1,-1,0},    
    {Ar9287EepromMacAddress,offsetof(ar9287_eeprom_t,baseEepHeader.macAddr),6,sizeof(u_int8_t),1,1,'m',1,-1,-1,0},
	{Ar9287EepromTxRxMaskRx,offsetof(ar9287_eeprom_t,baseEepHeader.rxMask),sizeof(u_int8_t),1,1,1,'x',1,-1,-1,0},
	{Ar9287EepromTxRxMaskTx,offsetof(ar9287_eeprom_t,baseEepHeader.txMask),sizeof(u_int8_t),1,1,1,'x',1,-1,-1,0},
	{Ar9287EepromRfSilent,offsetof(ar9287_eeprom_t,baseEepHeader.rfSilent),sizeof(u_int16_t),1,1,1,'x',1,-1,-1,0},
	{Ar9287EepromBlueToothOptions,offsetof(ar9287_eeprom_t,baseEepHeader.blueToothOptions),sizeof(u_int16_t),1,1,1,'x',1,-1,-1,0},
	{Ar9287EepromDeviceCapability,offsetof(ar9287_eeprom_t,baseEepHeader.deviceCap),sizeof(u_int16_t),1,1,1,'x',1,-1,-1,0},
	{Ar9287EepromBinBuildNumber,offsetof(ar9287_eeprom_t,baseEepHeader.binBuildNumber),sizeof(u_int32_t),1,1,1,'x',1,-1,-1,0},
	{Ar9287EepromDeviceType,offsetof(ar9287_eeprom_t,baseEepHeader.deviceType),sizeof(u_int8_t),1,1,1,'x',1,-1,-1,0},
	{Ar9287EepromOpenLoopPwrCntl,offsetof(ar9287_eeprom_t,baseEepHeader.openLoopPwrCntl),sizeof(u_int8_t),1,1,1,'x',1,-1,-1,0},
	{Ar9287EepromPowerTableOffset,offsetof(ar9287_eeprom_t,baseEepHeader.pwrTableOffset),sizeof(int8_t),1,1,1,'d',1,-1,-1,0},
	{Ar9287Eeprom2GHzTemperatureSlope,offsetof(ar9287_eeprom_t,baseEepHeader.tempSensSlope),sizeof(int8_t),1,1,1,'d',1,-1,-1,0},
	{Ar9287Eeprom2GHzTemperatureSlopePalOn,offsetof(ar9287_eeprom_t,baseEepHeader.tempSensSlopePalOn),sizeof(int8_t),1,1,1,'d',1,-1,-1,0},
	{Ar9287Eeprom2GHzFutureBase,offsetof(ar9287_eeprom_t,baseEepHeader.futureBase),sizeof(u_int8_t),29,1,1,'x',1,-1,-1,0},
	{Ar9287EepromCustomerData,offsetof(ar9287_eeprom_t,custData),AR9287_DATA_SZ,sizeof(unsigned char),1,1,'t',1,-1,-1,0},
	{Ar9287Eeprom2GHzAntennaControlChain,offsetof(ar9287_eeprom_t,modalHeader.antCtrlChain),sizeof(u_int32_t),AR9287_MAX_CHAINS,1,1,'x',1,-1,-1,0},
	{Ar9287Eeprom2GHzAntennaControlCommon2,offsetof(ar9287_eeprom_t,modalHeader.antCtrlCommon),sizeof(u_int32_t),1,1,1,'x',1,-1,-1,0},
	{Ar9287Eeprom2GHzAntennaGain,offsetof(ar9287_eeprom_t,modalHeader.antennaGainCh),sizeof(int8_t),AR9287_MAX_CHAINS,1,1,'d',1,-1,-1,0},
	{Ar9287Eeprom2GHzSwitchSettling,offsetof(ar9287_eeprom_t,modalHeader.switchSettling),sizeof(u_int8_t),1,1,1,'x',1,-1,-1,0},
	{Ar9287Eeprom2GHztxRxAttenCh,offsetof(ar9287_eeprom_t,modalHeader.txRxAttenCh),sizeof(u_int8_t),AR9287_MAX_CHAINS,1,1,'x',1,-1,-1,0},
	{Ar9287Eeprom2GHztxrxTxMarginCh,offsetof(ar9287_eeprom_t,modalHeader.rxTxMarginCh),sizeof(u_int8_t),AR9287_MAX_CHAINS,1,1,'x',1,-1,-1,0},
	{Ar9287Eeprom2GHzAdcSize,offsetof(ar9287_eeprom_t,modalHeader.adcDesiredSize),sizeof(int8_t),1,1,1,'d',1,-1,-1,0},
	{Ar9287Eeprom2GHzTxEndToXpaOff,offsetof(ar9287_eeprom_t,modalHeader.txEndToXpaOff),sizeof(u_int8_t),1,1,1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzTxEndToRxOn,offsetof(ar9287_eeprom_t,modalHeader.txEndToRxOn),sizeof(u_int8_t),1,1,1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzTxFrameToXpaOn,offsetof(ar9287_eeprom_t,modalHeader.txFrameToXpaOn),sizeof(u_int8_t),1,1,1,'u',1,-1,-1,0},
 	{Ar9287Eeprom2GHzThresh62,offsetof(ar9287_eeprom_t,modalHeader.thresh62),sizeof(u_int8_t),1,1,1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzNoiseFloorThreshold,offsetof(ar9287_eeprom_t,modalHeader.noiseFloorThreshCh),sizeof(int8_t),AR9287_MAX_CHAINS,1,1,'d',1,-1,-1,0},
	{Ar9287Eeprom2GHzXpdGain,offsetof(ar9287_eeprom_t,modalHeader.xpdGain),sizeof(u_int8_t),1,1,1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzXpdGain,offsetof(ar9287_eeprom_t,modalHeader.xpd),sizeof(u_int8_t),1,1,1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzIqCalICh,offsetof(ar9287_eeprom_t,modalHeader.iqCalICh),sizeof(int8_t),AR9287_MAX_CHAINS,1,1,'d',1,-1,-1,0},
	{Ar9287Eeprom2GHzIqCalQCh,offsetof(ar9287_eeprom_t,modalHeader.iqCalQCh),sizeof(int8_t),AR9287_MAX_CHAINS,1,1,'d',1,-1,-1,0},
	{Ar9287Eeprom2GHzPdGainOverlap,offsetof(ar9287_eeprom_t,modalHeader.pdGainOverlap),sizeof(u_int8_t),1,1,1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzXpaBiasLevel,offsetof(ar9287_eeprom_t,modalHeader.xpaBiasLvl),sizeof(u_int8_t),1,1,1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzTxFrameToDataStart,offsetof(ar9287_eeprom_t,modalHeader.txFrameToDataStart),sizeof(u_int8_t),1,1,1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzTxFrameToPaOn,offsetof(ar9287_eeprom_t,modalHeader.txFrameToPaOn),sizeof(u_int8_t),1,1,1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzHt40PowerIncForPdadc,offsetof(ar9287_eeprom_t,modalHeader.ht40PowerIncForPdadc),sizeof(u_int8_t),1,1,1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzBswAtten,offsetof(ar9287_eeprom_t,modalHeader.bswAtten),sizeof(u_int8_t),AR9287_MAX_CHAINS,1,1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzBswMargin,offsetof(ar9287_eeprom_t,modalHeader.bswMargin),sizeof(u_int8_t),AR9287_MAX_CHAINS,1,1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzSwSettleHt40,offsetof(ar9287_eeprom_t,modalHeader.swSettleHt40),sizeof(u_int8_t),1,1,1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzModalHeaderVersion,offsetof(ar9287_eeprom_t,modalHeader.version),sizeof(u_int8_t),1,1,1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzDb1,offsetof(ar9287_eeprom_t,modalHeader.db1),sizeof(u_int8_t),1,1,1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzDb2,offsetof(ar9287_eeprom_t,modalHeader.db2),sizeof(u_int8_t),1,1,1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzObcck,offsetof(ar9287_eeprom_t,modalHeader.ob_cck),sizeof(u_int8_t),1,1,1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzObPsk,offsetof(ar9287_eeprom_t,modalHeader.ob_psk),sizeof(u_int8_t),1,1,1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzObQam,offsetof(ar9287_eeprom_t,modalHeader.ob_qam),sizeof(u_int8_t),1,1,1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzObPalOff,offsetof(ar9287_eeprom_t,modalHeader.ob_pal_off),sizeof(u_int8_t),1,1,1,'u',1,-1,-1,0},
 	{Ar9287Eeprom2GHzFutureModal,offsetof(ar9287_eeprom_t,modalHeader.futureModal),sizeof(u_int8_t),30,1,1,'x',1,-1,-1,0},
  	{Ar9287Eeprom2GHzSpur,offsetof(ar9287_eeprom_t,modalHeader.spurChans)+offsetof(SPUR_CHAN,spurChan),sizeof(u_int16_t),AR9287_EEPROM_MODAL_SPURS,1,1,'u',sizeof(SPUR_CHAN),-1,-1,0},
  	{Ar9287Eeprom2GHzSpurRangeLow,offsetof(ar9287_eeprom_t,modalHeader.spurChans)+offsetof(SPUR_CHAN,spurRangeLow),sizeof(u_int16_t),AR9287_EEPROM_MODAL_SPURS,1,1,'u',sizeof(SPUR_CHAN),-1,-1,0},
  	{Ar9287Eeprom2GHzSpurRangeHigh,offsetof(ar9287_eeprom_t,modalHeader.spurChans)+offsetof(SPUR_CHAN,spurRangeHigh),sizeof(u_int16_t),AR9287_EEPROM_MODAL_SPURS,1,1,'u',sizeof(SPUR_CHAN),-1,-1,0},
	{Ar9287Eeprom2GHzCalibrationFrequency,offsetof(ar9287_eeprom_t,calFreqPier2G),sizeof(u_int8_t),AR9287_NUM_2G_CAL_PIERS,1,1,'2',1,-1,-1,0},
	{Ar9287Eeprom2GHzCalibrationData,offsetof(ar9287_eeprom_t,calPierData2G),sizeof(u_int8_t),AR9287_MAX_CHAINS,AR9287_NUM_2G_CAL_PIERS,sizeof(CAL_DATA_PER_FREQ_OP_LOOP_AR9287),'d',1,-1,-1,0},
	{Ar9287Eeprom2GHzTargetPowerCck,offsetof(ar9287_eeprom_t,calTargetPowerCck),sizeof(u_int8_t),AR9287_NUM_2G_CCK_TARGET_POWERS,sizeof(CAL_TARGET_POWER_LEG),1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzTargetPower,offsetof(ar9287_eeprom_t,calTargetPower2G),sizeof(u_int8_t),AR9287_NUM_2G_20_TARGET_POWERS,sizeof(CAL_TARGET_POWER_LEG),1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzTargetPowerHt20,offsetof(ar9287_eeprom_t,calTargetPower2GHT20),sizeof(u_int8_t),AR9287_NUM_2G_20_TARGET_POWERS,sizeof(CAL_TARGET_POWER_HT),1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzTargetPowerHt40,offsetof(ar9287_eeprom_t,calTargetPower2GHT40),sizeof(u_int8_t),AR9287_NUM_2G_40_TARGET_POWERS,sizeof(CAL_TARGET_POWER_HT),1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzCtlIndex,offsetof(ar9287_eeprom_t,ctlIndex),sizeof(u_int8_t),AR9287_NUM_CTLS,1,1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzCtlData,offsetof(ar9287_eeprom_t,ctlData),sizeof(u_int8_t),AR9287_NUM_CTLS,sizeof(CAL_CTL_DATA_AR9287),1,'u',1,-1,-1,0},
	{Ar9287Eeprom2GHzPadding,offsetof(ar9287_eeprom_t,padding),sizeof(u_int8_t),1,1,1,'u',1,-1,-1,0},
};

static unsigned int Mask(int low, int high)
{
	unsigned int mask;
	int ib;

	mask=0;
	if(low<=0)
	{
		low=0;
	}
	if(high>=31)
	{
		high=31;
	}
	for(ib=low; ib<=high; ib++)
	{
		mask|=(1<<ib);
	}
	return mask;
}


static int Ar9287EepromPrintIt(unsigned char *data, int type, int size, int length, int high, int low, int voff,
	char *buffer, int max)
{
	char *vc;
	short *vs;
	int *vl;
	double *vd;
	float *vf;
	int lc, nc;
	int it;
	char text[MPRINTBUFFER];
	int vuse;
	unsigned int mask;
	int iuse;

	lc=0;
	switch(size)
	{
		case 1:
			vc=(char *)data;
			if(type=='t')
			{
				//
				// make sure there is a null
				//
				if(length>MPRINTBUFFER)
				{
					length=MPRINTBUFFER;
				}
				strncpy(text,vc,length);
				text[length]=0;

				nc=SformatOutput(&buffer[lc],max-lc-1,"%s",text);
				if(nc>0)
				{
					lc+=nc;
				}
			}
			else
			{
				for(it=0; it<length; it++)
				{
					vuse=vc[it]&0xff;
					if(high>=0 && low>=0)
					{
						mask=Mask(low,high);
						vuse=(vuse&mask)>>low;
					}
					switch(type)
					{
						case 'p':
#ifdef UNUSED
							if(vuse&0xc0)
							{
								nc=SformatOutput(&buffer[lc],max-lc-1,"%.1lf-%d",0.5*((int)(vuse&0x3f)),(int)(vuse>>6));
							}
							else
#endif
							{
								nc=SformatOutput(&buffer[lc],max-lc-1,"%.1lf",0.5*(voff+(int)(vuse&0x3f)));
							}
							break;
						case '2':
							if(vuse>0)
							{
								nc=SformatOutput(&buffer[lc],max-lc-1,"%d",2300+(voff+(int)vuse));
							}
							else
							{
								nc=SformatOutput(&buffer[lc],max-lc-1,"%d",0);
							}
							break;
						case '5':
							if(vuse>0)
							{
								nc=SformatOutput(&buffer[lc],max-lc-1,"%d",4800+5*(voff+(int)vuse));
							}
							else
							{
								nc=SformatOutput(&buffer[lc],max-lc-1,"%d",0);
							}
							break;
						case 'd':
							iuse=(int)vuse;
							if(vuse&0x80)
							{
								iuse=((int)vuse)|0xffffff00;
							}
							else
							{
								iuse=(int)vuse;
							}
							nc=SformatOutput(&buffer[lc],max-lc-1,"%+d",voff+iuse);
							break;
						case 'u':
							nc=SformatOutput(&buffer[lc],max-lc-1,"%u",(voff+(unsigned int)vuse)&0xff);
							break;
						case 'c':
							nc=SformatOutput(&buffer[lc],max-lc-1,"%1c",(voff+(unsigned int)vuse)&0xff);
							break;
						default:
						case 'x':
							nc=SformatOutput(&buffer[lc],max-lc-1,"0x%02x",(voff+(unsigned int)vuse)&0xff);
							break;
					}
					if(nc>0)
					{
						lc+=nc;
					}
					if(it<length-1)
					{
						nc=SformatOutput(&buffer[lc],max-lc-1,",");
						if(nc>0)
						{
							lc+=nc;
						}
					}
				}
			}
			break;
		case 2:
			vs=(short *)data;
			for(it=0; it<length; it++)
			{
				vuse=vs[it]&0xffff;
				if(high>=0 && low>=0)
				{
					mask=Mask(low,high);
					vuse=(vuse&mask)>>low;
				}
				switch(type)
				{
		            case 'd':
						iuse=(int)vuse;
						if(vuse&0x8000)
						{
							iuse=((int)vuse)|0xffff0000;
						}
						else
						{
							iuse=(int)vuse;
						}
			            nc=SformatOutput(&buffer[lc],max-lc-1,"%+d",voff+iuse);
						break;
		            case 'u':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"%u",(voff+(unsigned int)vuse)&0xffff);
						break;
		            case 'c':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"%2c",(voff+(unsigned int)vuse)&0xff,((voff+(unsigned int)vuse)>>8)&0xff);
						break;
					default:
		            case 'x':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"0x%04x",(voff+(unsigned int)vuse)&0xffff);
						break;
				}
				if(nc>0)
				{
					lc+=nc;
				}
				if(it<length-1)
				{
			        nc=SformatOutput(&buffer[lc],max-lc-1,",");
					if(nc>0)
					{
						lc+=nc;
					}
				}
			}
			break;
		case 4:
			vl=(int *)data;
			vf=(float *)data;
			for(it=0; it<length; it++)
			{
				vuse=vl[it]&0xffffffff;
				if(high>=0 && low>=0)
				{
					mask=Mask(low,high);
					vuse=(vuse&mask)>>low;
				}
				switch(type)
				{
		            case 'd':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"%+d",voff+vuse);
						break;
		            case 'u':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"%u",voff+(vuse&0xffffffff));
						break;
		            case 'f':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"%g",voff+vf[it]);
						break;
		            case 'c':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"%4c",(voff+vuse)&0xff,((voff+(unsigned int)vuse)>>8)&0xff,((voff+(unsigned int)vuse)>>16)&0xff,((voff+(unsigned int)vuse)>>24)&0xff);
						break;
					default:
		            case 'x':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"0x%08x",(voff+vuse)&0xffffffff);
						break;
				}
				if(nc>0)
				{
					lc+=nc;
				}
				if(it<length-1)
				{
			        nc=SformatOutput(&buffer[lc],max-lc-1,",");
					if(nc>0)
					{
						lc+=nc;
					}
				}
			}
			break;
		default:
			vc=(char *)data;
			if(type=='t')
			{
				//
				// make sure there is a null
				//
				if(size>MPRINTBUFFER)
				{
					size=MPRINTBUFFER;
				}
				strncpy(text,vc,size);
				text[size]=0;
				nc=SformatOutput(&buffer[lc],max-lc-1,"%s",text);
				if(nc>0)
				{
					lc+=nc;
				}
			}
			else if(type=='m')
			{
				nc=SformatOutput(&buffer[lc],max-lc-1,"%02x:%02x:%02x:%02x:%02x:%02x",
					vc[0]&0xff,vc[1]&0xff,vc[2]&0xff,vc[3]&0xff,vc[4]&0xff,vc[5]&0xff);
				if(nc>0)
				{
					lc+=nc;
				}
			}
			else
			{
				for(it=0; it<length; it++)
				{
					vuse=vc[it]&0xff;
					nc=SformatOutput(&buffer[lc],max-lc-1,"0x%02x",(voff+(unsigned int)vuse)&0xff);
					if(nc>0)
					{
						lc+=nc;
					}
				}
			}
			break;
    }
	return lc;
}


static void Ar9287EepromPrintEntry(void (*print)(char *format, ...), 
	char *name, int offset, int size, int high, int low, int voff,
	int nx, int ny, int nz, int interleave,
	char type, ar9287_eeprom_t *mptr, int mcount, int all)
{
	int im;
	int lc, nc;
	char buffer[MPRINTBUFFER],fullname[MPRINTBUFFER];
	int it, iuse;
	int different;
	int length;
	int ix, iy, iz;

	length=nx*ny*nz;

	for(it=0; it<length; it++)
	{
#ifdef WRONG
		iy=it%(nx*ny);
		iz=it/(nx*ny);
		ix=iy%nx;
		iy=iy/nx;
#else
		iz=it%nz;
		iy=it/nz;
		ix=iy/ny;
		iy=iy%ny;
#endif
		if(nz>1)
		{
			SformatOutput(fullname,MPRINTBUFFER-1,"%s[%d][%d][%d]",name,ix,iy,iz);
		}
		else if(ny>1)
		{
			SformatOutput(fullname,MPRINTBUFFER-1,"%s[%d][%d]",name,ix,iy);
		}
		else if(nx>1)
		{
			SformatOutput(fullname,MPRINTBUFFER-1,"%s[%d]",name,ix);
		}
		else
		{
			SformatOutput(fullname,MPRINTBUFFER-1,"%s",name);
		}
		fullname[MPRINTBUFFER-1]=0;
		lc=0;
		nc=SformatOutput(&buffer[lc],MPRINTBUFFER-lc-1,"|ecv|%s|%d|%d|%d|%d|%d|%d|%d|%d|%c|",
			fullname,it,offset+it*interleave,size,high,low,nx,ny,nz,type);
		if(nc>0)
		{
			lc+=nc;
		}
		//
		// put value from mptr[0]
		//
		nc=Ar9287EepromPrintIt(((unsigned char *)&mptr[0])+offset+it*interleave*size, type, size, 1, high, low, voff, &buffer[lc], MPRINTBUFFER-lc-1);
		if(nc>0)
		{
			lc+=nc;
		}
		nc=SformatOutput(&buffer[lc],MPRINTBUFFER-lc-1,"|");
		if(nc>0)
		{
			lc+=nc;
		}
		//
		// loop over subsequent iterations
		// add value only if different than previous value
		//
		different=0;
		for(im=1; im<mcount; im++)
		{
			if(memcmp(((unsigned char *)&mptr[im-1])+offset+it*interleave*size,((unsigned char *)&mptr[im])+offset+it*interleave*size,size)!=0)
			{
				nc=Ar9287EepromPrintIt(((unsigned char *)&mptr[im])+offset+it*interleave*size, type, size, 1, high, low, voff, &buffer[lc], MPRINTBUFFER-lc-1);
				if(nc>0)
				{
					lc+=nc;
				}
				different++;
			}
			else
			{
				nc=SformatOutput(&buffer[lc], MPRINTBUFFER-lc-1,".");
				if(nc>0)
				{
					lc+=nc;
				}
			}
			nc=SformatOutput(&buffer[lc],MPRINTBUFFER-lc-1,"|");
			if(nc>0)
			{
				lc+=nc;
			}
		}
		//
		// fill in up to the maximum number of blocks
		//
		for( ; im<MBLOCK; im++)
		{
			nc=SformatOutput(&buffer[lc],MPRINTBUFFER-lc-1,"|");
			if(nc>0)
			{
				lc+=nc;
			}
		}
		//
		// print it
		//
		if(different>0 || all)
		{
			(*print)("%s",buffer);
		}
	}
}


void Ar9287EepromDifferenceAnalyze(void (*print)(char *format, ...), ar9287_eeprom_t *mptr, int mcount, int all)
{
	int msize;
	int offset;
	int length;
	int im;
	int lc, nc;
	char buffer[MPRINTBUFFER];
	int it, nt;
	int io;
    //
	// make header
	//
	lc=SformatOutput(buffer,MPRINTBUFFER-1,"|ecv|name|index|offset|size|high|low|nx|ny|nz|type|");
	//
	// fill in up to the maximum number of blocks
	//
	for(im=0; im<MBLOCK; im++)
	{
		nc=SformatOutput(&buffer[lc],MPRINTBUFFER-lc-1,"b%d|",im);
		if(nc>0)
		{
			lc+=nc;
		}
	}
	(*print)("%s",buffer);

	offset=0;
	nt=sizeof(_Ar9287EepromList)/sizeof(_Ar9287EepromList[0]);
	for(it=0; it<nt; it++)
	{
		//
		// first we do any bytes that are not associated with a field name
		//
		for(io=offset; io<_Ar9287EepromList[it].offset; io++)
		{
            Ar9287EepromPrintEntry(print, "unknown", io, 1, -1, -1, 0, 1, 1, 1, 1, 'x', mptr, mcount, all);
		}
		//
		// do the field
		//
        Ar9287EepromPrintEntry(print,
			_Ar9287EepromList[it].name, _Ar9287EepromList[it].offset, _Ar9287EepromList[it].size, _Ar9287EepromList[it].high, _Ar9287EepromList[it].low, _Ar9287EepromList[it].voff,
			_Ar9287EepromList[it].nx, _Ar9287EepromList[it].ny, _Ar9287EepromList[it].nz, _Ar9287EepromList[it].interleave,
			_Ar9287EepromList[it].type, 
			mptr, mcount, all);
        if(_Ar9287EepromList[it].interleave==1 || (it<nt-1 && _Ar9287EepromList[it].interleave!=_Ar9287EepromList[it+1].interleave))
		{
			offset=_Ar9287EepromList[it].offset+
				(_Ar9287EepromList[it].size*_Ar9287EepromList[it].nx*_Ar9287EepromList[it].ny*_Ar9287EepromList[it].nz*_Ar9287EepromList[it].interleave)-
				(_Ar9287EepromList[it].interleave-1); 
		}
		else
		{
			offset=_Ar9287EepromList[it].offset+_Ar9287EepromList[it].size; 
		}
	}
	//
	// do any trailing bytes not associated with a field name
	//
	for(io=offset; io<sizeof(ar9287_eeprom_t); io++)
	{
        Ar9287EepromPrintEntry(print, "unknown", io, 1, -1, -1, 0, 1, 1, 1, 1, 'x', mptr, mcount, all);
	}
}

void print9287BaseHeader(int client, const BASE_EEPAR9287_HEADER *pBaseEepHeader)
{
	char  buffer[MBUFFER];

	SformatOutput(buffer, MBUFFER-1," |                                                                       |");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1," |===========================2GHz Base Header============================|");
	ErrorPrint(NartOther, buffer);
	SformatOutput(buffer, MBUFFER-1," |                                   |                                   |");
	SformatOutput(buffer, MBUFFER-1," | Length                     0x%04X |  Checksum                 0x%04X  |", pBaseEepHeader->length, pBaseEepHeader->checksum);
	ErrorPrint(NartOther, buffer);
	SformatOutput(buffer, MBUFFER-1," | Version                    0x%04X |  OpFlags                  0x%02X    |", pBaseEepHeader->version, pBaseEepHeader->opCapFlags);
	ErrorPrint(NartOther, buffer);
	SformatOutput(buffer, MBUFFER-1," | EepMisc                    0x%04X |                                   |", pBaseEepHeader->eepMisc);
	ErrorPrint(NartOther, buffer);
	SformatOutput(buffer, MBUFFER-1," | RegDomain 1                0x%04X |  RegDomain 2              0x%04X  |",
		pBaseEepHeader->regDmn[0],
		pBaseEepHeader->regDmn[1]
	);
	ErrorPrint(NartOther, buffer);
	SformatOutput(buffer, MBUFFER-1," | MAC Address: 0x%02X:%02X:%02X:%02X:%02X:%02X  |                                   |",
		pBaseEepHeader->macAddr[0],
		pBaseEepHeader->macAddr[1],
		pBaseEepHeader->macAddr[2],
		pBaseEepHeader->macAddr[3],
		pBaseEepHeader->macAddr[4],
		pBaseEepHeader->macAddr[5]
	);
	ErrorPrint(NartOther, buffer);
	SformatOutput(buffer, MBUFFER-1," | TX Mask                    0x%02X   |  RX Mask                  0x%02X    |",
		pBaseEepHeader->txMask&0x0F,
		pBaseEepHeader->rxMask&0x0F
	);
	ErrorPrint(NartOther, buffer);
	SformatOutput(buffer, MBUFFER-1," | RF Silent                  0x%04X |  Bluetooth                0x%04X  |", pBaseEepHeader->rfSilent, pBaseEepHeader->blueToothOptions);
	ErrorPrint(NartOther, buffer);
	SformatOutput(buffer, MBUFFER-1," | deviceCapabilities         0x%04X |  CalibrationBinary Ver    0x%06X|", pBaseEepHeader->deviceCap, pBaseEepHeader->binBuildNumber);
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1," | openLoopPwrCntl            0x%02X   |  DeviceType          %12s |", pBaseEepHeader->openLoopPwrCntl, sDeviceType[pBaseEepHeader->deviceType]);
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1," | tempSensSlope              %d     |  pwrTableOffset           %d      |", pBaseEepHeader->tempSensSlope, pBaseEepHeader->pwrTableOffset);
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1," | tempSensSlopePalOn         %d     |                                   |", pBaseEepHeader->tempSensSlopePalOn);
	ErrorPrint(NartOther,buffer);
}


void print9287ModalHeader(int client, const MODAL_EEPAR9287_HEADER   *pModalHeader)
{
	char  i;
	char buffer[MBUFFER],buffer2[MBUFFER];

	SformatOutput(buffer, MBUFFER-1," |                                                                       |");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1," |===========================2GHz Modal Header===========================|");
	ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," | Antenna Common         0x%08X |                                   |",
        pModalHeader->antCtrlCommon
	);
	ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," | Ant Chain 0                0x%04X |  Ant Chain 1             0x%04X   |",
        pModalHeader->antCtrlChain[0],
		pModalHeader->antCtrlChain[1]
	);
	ErrorPrint(NartOther, buffer);

#define NO_SPUR                      ( 0x8000 )

	SformatOutput(buffer, MBUFFER-1," | Spur Channels:           ");
	for (i = 0; i < AR9287_EEPROM_MODAL_SPURS; i++) {
		if((pModalHeader->spurChans[i].spurChan == NO_SPUR) || (pModalHeader->spurChans[i].spurChan==0)){
			SformatOutput(buffer2, MBUFFER-1,"No Spur, ");
		} else {
			SformatOutput(buffer2, MBUFFER-1,"%04d, ", FBIN2FREQ(pModalHeader->spurChans[i].spurChan,1));
		}
		strcat(buffer,buffer2);
    }
	ErrorPrint(NartOther,buffer);

    SformatOutput(buffer, MBUFFER-1," | NoiseFloorThres0         %3d      |  NoiseFloorThres1       %3d       |",
             pModalHeader->noiseFloorThreshCh[0],
			 pModalHeader->noiseFloorThreshCh[1]
	);
	ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," | xpaBiasLvl                 0x%02x   |  txFrame2DataStart       0x%02x     |",
             pModalHeader->xpaBiasLvl,
			 pModalHeader->txFrameToDataStart
	);
	ErrorPrint(NartOther, buffer);

	SformatOutput(buffer, MBUFFER-1," | txFrameToPaOn              0x%02x   |  switchSettling          0x%02x     |",
             pModalHeader->txFrameToPaOn, 
             pModalHeader->switchSettling
	);
	ErrorPrint(NartOther, buffer);
	SformatOutput(buffer, MBUFFER-1," | adcDesiredSize             %3d    |  txEndToXpaOff           0x%02x     |",
             pModalHeader->adcDesiredSize,
			 pModalHeader->txEndToXpaOff
	);
	ErrorPrint(NartOther, buffer);

	SformatOutput(buffer, MBUFFER-1," | txEndToRxOn                0x%02x   |  txFrameToXpaOn          0x%02x     |",
             pModalHeader->txEndToRxOn,
			 pModalHeader->txFrameToXpaOn
	);
	ErrorPrint(NartOther, buffer);

	SformatOutput(buffer, MBUFFER-1," | thresh62                   %2d     |                                   |",
             pModalHeader->thresh62
	);
	ErrorPrint(NartOther, buffer);
	SformatOutput(buffer, MBUFFER-1," |                                                                       |");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1," |-----------------------------------------------------------------------|");
	ErrorPrint(NartOther,buffer);
}

void print9287_2GPowerCalData(int client, const ar9287_eeprom_t *p9287)
{
	A_UINT16 numPiers = AR9287_NUM_2G_CAL_PIERS;
    A_UINT16 pi; //pier index
	A_UINT16 PierVal;
   	char  buffer[MBUFFER];
    const A_UINT8 *pPiers = &p9287->calFreqPier2G[0];
    const CAL_DATA_PER_FREQ_U_AR9287 *pData = &p9287->calPierData2G[0][0];

	SformatOutput(buffer, MBUFFER-1, " |=================2G Power Calibration Information =====================|");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1, " |                   Chain 0         |                Chain 1            |");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
	ErrorPrint(NartOther,buffer);
    SformatOutput(buffer, MBUFFER-1, " |      Freq    Pwr   Temp    TxGain |        Pwr   Temp    TxGain       |");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
	ErrorPrint(NartOther,buffer);
    for(pi = 0; pi < numPiers; pi++) {
		if(pPiers[pi]==0)
		{
			PierVal=0;
		}
		else
		{
			PierVal=FBIN2FREQ(pPiers[pi], 1);
		}
        SformatOutput(buffer, MBUFFER-1, " |      %4d   %4d    %3d      %3d  |       %4d    %3d       %3d       |",
            PierVal,
			(int8_t)pData->calDataOpen.pwrPdg[0][0], pData->calDataOpen.vpdPdg[0][0], pData->calDataOpen.pcdac[0][0], 
			(int8_t)(pData+1*AR9287_NUM_2G_CAL_PIERS)->calDataOpen.pwrPdg[0][0], (pData+1*AR9287_NUM_2G_CAL_PIERS)->calDataOpen.vpdPdg[0][0], (pData+1*AR9287_NUM_2G_CAL_PIERS)->calDataOpen.pcdac[0][0]
		);
		ErrorPrint(NartOther,buffer);
        pData++;
    }
	SformatOutput(buffer, MBUFFER-1, " |-----------------------------------------------------------------------|");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1, " |                                                                       |");
	ErrorPrint(NartOther,buffer);
}

const static char *sRatePrintCck[4] = 
{
	"1L-5L",
	" 5S  ", 
	" 11L ", 
	" 11S "
};

void print9287_2GCCKTargetPower(int client, const CAL_TARGET_POWER_LEG *pVals)
{
	int i,j;
	char buffer[MBUFFER],buffer2[MBUFFER];

	SformatOutput(buffer, MBUFFER-1, " |===========================2G Target Powers============================|");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1," | CCK    ");

	for (j = 0; j < AR9287_NUM_2G_20_TARGET_POWERS; j++) {
		if (((pVals+j)->bChannel) == AR9287_BCHAN_UNUSED) {
			SformatOutput(buffer2, MBUFFER-1,"|       ");
		} else {
			SformatOutput(buffer2, MBUFFER-1,"|        %04d        ", FBIN2FREQ(((pVals+j)->bChannel),1));
		}
		strcat(buffer,buffer2);
    }
	ErrorPrint(NartOther,buffer);

    SformatOutput(buffer, MBUFFER-1," |========|====================|====================|====================|");
	ErrorPrint(NartOther, buffer);

    for (j = 0; j < 4; j++) {
		SformatOutput(buffer, MBUFFER-1," | %s  ",sRatePrintCck[j]);
		for(i=0;i<2;i++){
			SformatOutput(buffer2, MBUFFER-1,"|        %2d.%d        ", (pVals+i)->tPow2x[j]/2, ((pVals+i)->tPow2x[j] % 2) * 5);
			strcat(buffer,buffer2);
    	}
		strcat(buffer,"|====================|");
		ErrorPrint(NartOther, buffer);
    }
	SformatOutput(buffer, MBUFFER-1," |=======================================================================|");
	ErrorPrint(NartOther, buffer);
}

const static char *sRatePrintLegacy[4] = 
{
	"6-24",
	" 36 ", 
	" 48 ", 
	" 54 "
};

void print9287_2GLegacyTargetPower(int client, const CAL_TARGET_POWER_LEG *pVals)
{
	int i,j;
	char buffer[MBUFFER],buffer2[MBUFFER];

	SformatOutput(buffer, MBUFFER-1," | OFDM   ");

	for (j = 0; j < AR9287_NUM_2G_20_TARGET_POWERS; j++) {
        SformatOutput(buffer2, MBUFFER-1,"|        %04d        ", FBIN2FREQ(((pVals+j)->bChannel),1));
		strcat(buffer,buffer2);
    }

    strcat(buffer,"|");
	ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," |========|====================|====================|====================|");
	ErrorPrint(NartOther, buffer);

    for (j = 0; j < 4; j++) {
		SformatOutput(buffer, MBUFFER-1," | %s   ",sRatePrintLegacy[j]);
		for(i=0;i<3;i++){
			SformatOutput(buffer2, MBUFFER-1,"|        %2d.%d        ", (pVals+i)->tPow2x[j]/2, ((pVals+i)->tPow2x[j] % 2) * 5);
			strcat(buffer,buffer2);
    	}

		strcat(buffer,"|");
		ErrorPrint(NartOther, buffer);
    }
	SformatOutput(buffer, MBUFFER-1," |=======================================================================|");
	ErrorPrint(NartOther, buffer);
}


/*Kiwi is 2x2, so only support two streams*/
const static int ar9287mapRate2Index[16]=
{
    0,1,2,3,4,5,6,7,	/* MCS 0 - MCS 7 */
	0,1,2,3,4,5,6,7		/* MCS 8 - MCS15 */
//	7,7,7,7//,1,1,1,1 	/* MCS16 - MCS23 */
};

const static char *sRatePrintHT[24] = 
{
	"MCS0 ",
	"MCS1 ", 
	"MCS2 ",
	"MCS3 ",
	"MCS4 ",
	"MCS5 ",
	"MCS6 ",
	"MCS7 ",
	"MCS8 ",
	"MCS9 ",
	"MCS10",
	"MCS11",
	"MCS12",
	"MCS13",
	"MCS14",
	"MCS15",
	"MCS16",
	"MCS17",
	"MCS18",
	"MCS19",
	"MCS20",
	"MCS21",
	"MCS22",
	"MCS23"
};

void print9287_2GHT20TargetPower(int client, const CAL_TARGET_POWER_HT *pVals) {

	int i,j;
	char buffer[MBUFFER],buffer2[MBUFFER];

	SformatOutput(buffer, MBUFFER-1," | HT20   ");

	for (j = 0; j < AR9287_NUM_2G_20_TARGET_POWERS; j++) {
        SformatOutput(buffer2, MBUFFER-1,"|        %04d        ", FBIN2FREQ(((pVals+j)->bChannel),1));
		strcat(buffer,buffer2);
    }

    strcat(buffer,"|");
	ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," |========|====================|====================|====================|");
	ErrorPrint(NartOther, buffer);

    for (j = 0; j < 16; j++) {
    //for (j = 0; j < 8; j++) {
		SformatOutput(buffer, MBUFFER-1," | %s  ",sRatePrintHT[j]);
		for(i=0;i<3;i++){
			SformatOutput(buffer2, MBUFFER-1,"|        %2d.%d        ", (pVals+i)->tPow2x[ar9287mapRate2Index[j]]/2, ((pVals+i)->tPow2x[ar9287mapRate2Index[j]] % 2) * 5);
			//SformatOutput(buffer2, MBUFFER-1,"|        %2d.%d        ", (pVals+i)->tPow2x[j]/2, ((pVals+i)->tPow2x[j] % 2) * 5);
			strcat(buffer,buffer2);
        }

		strcat(buffer,"|");
		ErrorPrint(NartOther, buffer);
    }
	SformatOutput(buffer, MBUFFER-1," |=======================================================================|");
	ErrorPrint(NartOther, buffer);
}


void print9287_2GHT40TargetPower(int client, const CAL_TARGET_POWER_HT *pVals) {

	int i,j;
	char buffer[MBUFFER],buffer2[MBUFFER];

    SformatOutput(buffer, MBUFFER-1," | HT40   ");

	for (j = 0; j < AR9287_NUM_2G_40_TARGET_POWERS; j++) {
        SformatOutput(buffer2, MBUFFER-1,"|        %04d        ", FBIN2FREQ(((pVals+j)->bChannel),1));
		strcat(buffer,buffer2);
    }

    strcat(buffer,"|");
	ErrorPrint(NartOther, buffer);

    SformatOutput(buffer, MBUFFER-1," |========|====================|====================|====================|");
	ErrorPrint(NartOther, buffer);

    //for (j = 0; j < 24; j++) {
    for (j = 0; j < 16; j++) {
		SformatOutput(buffer, MBUFFER-1," | %s  ",sRatePrintHT[j]);
		for(i=0;i<3;i++){
			SformatOutput(buffer2, MBUFFER-1,"|        %2d.%d        ", (pVals+i)->tPow2x[ar9287mapRate2Index[j]]/2, ((pVals+i)->tPow2x[ar9287mapRate2Index[j]] % 2) * 5);
			//SformatOutput(buffer2, MBUFFER-1,"|        %2d.%d        ", (pVals+i)->tPow2x[j]/2, ((pVals+i)->tPow2x[j] % 2) * 5);
			strcat(buffer,buffer2);
        }

		strcat(buffer,"|");
		ErrorPrint(NartOther, buffer);
    }
	SformatOutput(buffer, MBUFFER-1," |=======================================================================|");
	ErrorPrint(NartOther, buffer);
}

/*
void print9287_2GCTLIndex(int client, const A_UINT8 *pCtlIndex){
	int i;
	char buffer[MBUFFER];
	SformatOutput(buffer, MBUFFER-1,"| 2G CTL Index:                                                         |");
	ErrorPrint(NartOther,buffer);
	for(i=0;i<12;i++){
		SformatOutput(buffer, MBUFFER-1,"| [%d] :0x%x                                                           |",i,pCtlIndex[i]);
		ErrorPrint(NartOther,buffer);
	}
}
*/

const static char *sCtlType[] = {
	"[ 11A base mode ]",
	"[ 11B base mode ]",
	"[ 11G base mode ]",
	"[ INVALID		 ]",
	"[ INVALID		 ]",
	"[ 2G HT20 mode  ]",
	"[ 5G HT20 mode  ]",
	"[ 2G HT40 mode  ]",
	"[ 5G HT40 mode  ]",
};

void print9287_2GCTLData(int client,  const A_UINT8 *ctlIndex, const CAL_CTL_DATA_AR9287 *Data){

	int i,j, c;
	char buffer[MBUFFER],buffer2[MBUFFER];

    SformatOutput(buffer, MBUFFER-1," |                                                                       |");
	ErrorPrint(NartOther, buffer);

	SformatOutput(buffer, MBUFFER-1," |=======================Test Group Band Edge Power======================|");
	ErrorPrint(NartOther, buffer);
	
    for (i = 0; i<AR9287_NUM_CTLS; i++) {
		SformatOutput(buffer, MBUFFER-1," |                                                                       |");
		ErrorPrint(NartOther, buffer);

		SformatOutput(buffer, MBUFFER-1," | CTL: 0x%02x %s                                           |",
				 ctlIndex[i], sCtlType[ctlIndex[i] & AR9287_CTL_MODE_M]);
		ErrorPrint(NartOther, buffer);
		SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
		ErrorPrint(NartOther, buffer);

		for( c = 0; c < AR9287_MAX_CHAINS; c++ ) {
			SformatOutput(buffer, MBUFFER-1," |===================== Edges for chain%d operation ======================|",c);
			ErrorPrint(NartOther, buffer);

	        SformatOutput(buffer, MBUFFER-1," | edge  ");

			/*************************************************************
			*  for CTL freq/power/flag setting, same setting for both Chain0 and Chain1
			*  so, just use chain 0 (i.e. ctlEdges[0][x]) for display
			**************************************************************/
	        for (j = 0; j < AR9287_NUM_BAND_EDGES; j++) {
	            if ((Data+i)->ctlEdges[c][j].bChannel == AR9287_BCHAN_UNUSED) {
	                SformatOutput(buffer2, MBUFFER-1,"|  --   ");
	            } else {
	                SformatOutput(buffer2, MBUFFER-1,"| %04d  ", FBIN2FREQ(((Data+i)->ctlEdges[c][j].bChannel),1));
	            }
				strcat(buffer,buffer2);
	        }

	        strcat(buffer,"|");
			ErrorPrint(NartOther, buffer);

	        SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
			ErrorPrint(NartOther, buffer);

	        SformatOutput(buffer, MBUFFER-1," | power ");

	        for (j = 0; j < AR9287_NUM_BAND_EDGES; j++) {
	            if ((Data+i)->ctlEdges[c][j].tPower == AR9287_BCHAN_UNUSED) {
	                SformatOutput(buffer2, MBUFFER-1,"|  --   ");
	            } else {
	            	//UserPrint("tPower=%d, tPower/2=%d, (tPower/2)*5=%d\n", (Data+i)->ctlEdges[c][j].tPower, 
					//		(Data+i)->ctlEdges[c][j].tPower / 2, ((Data+i)->ctlEdges[c][j].tPower % 2) * 5);
	                SformatOutput(buffer2, MBUFFER-1,"| %2d.%d  ", (Data+i)->ctlEdges[c][j].tPower / 2,
	                    ((Data+i)->ctlEdges[c][j].tPower % 2) * 5);
	            }
				strcat(buffer,buffer2);
	        }

	        strcat(buffer,"|");
			ErrorPrint(NartOther, buffer);

	        SformatOutput(buffer, MBUFFER-1," |=======|=======|=======|=======|=======|=======|=======|=======|=======|");
			ErrorPrint(NartOther, buffer);

	        SformatOutput(buffer, MBUFFER-1," | flag  ");

	        for (j = 0; j < AR9287_NUM_BAND_EDGES; j++) {
	            if ((Data+i)->ctlEdges[c][j].flag == AR9287_BCHAN_UNUSED) {
	                SformatOutput(buffer2, MBUFFER-1,"|  --   ");

	            } else {
	            	//UserPrint("flag=%d\n", (Data+i)->ctlEdges[c][j].flag);
	               	SformatOutput(buffer2, MBUFFER-1,"|   %1d   ", (Data+i)->ctlEdges[c][j].flag);
	            }
				strcat(buffer,buffer2);
	        }

	        strcat(buffer,"|");
			ErrorPrint(NartOther, buffer);

	        SformatOutput(buffer, MBUFFER-1," |=======================================================================|");
			ErrorPrint(NartOther, buffer);
		}
	}
}

extern ar9287_eeprom_t *Ar9287EepromStructGet(void);

void print9287Struct(int client, int useDefault)
{
	char buffer[MBUFFER],buffer2[MBUFFER];
	int i;
	ar9287_eeprom_t *pDefault9287;  //calling for default 2
	int addr;
    u_int16_t val;

    /*=ar9287EepromStructDefault(2);*/
	pDefault9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

    SformatOutput(buffer, MBUFFER-1,"start get all");
    ErrorPrint(NartOther,buffer);

	SformatOutput(buffer, MBUFFER-1,"                         ----------------------                           ");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1,  " =======================| AR9287 CAL STRUCTURE |==========================");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1," |                       -----------------------                         |");
	ErrorPrint(NartOther,buffer);
	SformatOutput(buffer, MBUFFER-1," | Customer Data in hex                                                  |");
	ErrorPrint(NartOther,buffer);
	for(i=0;i<AR9287_DATA_SZ;i++){
		if(i%16==0)
		{
			SformatOutput(buffer, MBUFFER-1," |");
		}
		SformatOutput(buffer2, MBUFFER-1," %2.2X", pDefault9287->custData[i] );
		strcat(buffer,buffer2);
		if(i%16==15)
		{
			strcat(buffer,"                       |");
			ErrorPrint(NartOther,buffer);
		}
	}
	SformatOutput(buffer, MBUFFER-1," |-----------------------------------------------------------------------|");
	ErrorPrint(NartOther,buffer);

	/* EEPROM Initialization*/
	/* No display */

	/* Base EEPROM Header*/
	print9287BaseHeader(client, &pDefault9287->baseEepHeader);

	/* Modal EEPROM Header 2GHz*/
    print9287ModalHeader(client, &pDefault9287->modalHeader);

	/* Power Calibration Data 2 GHz*/
	print9287_2GPowerCalData(client, pDefault9287);

	/* Target Powers 802.11b/g CCK*/
    print9287_2GCCKTargetPower(client, pDefault9287->calTargetPowerCck);

	/* Target Powers 802.11g OFDM*/
    print9287_2GLegacyTargetPower(client, pDefault9287->calTargetPower2G);

	/* Target Powres 2 GHz 802.11n HT20 OFDM*/
    print9287_2GHT20TargetPower(client, pDefault9287->calTargetPower2GHT20);

	/* Target Powres 2 GHz 802.11n HT40 OFDM*/
    print9287_2GHT40TargetPower(client, pDefault9287->calTargetPower2GHT40);

	/* CTL Indexes*/
   // print9287_2GCTLIndex(client, &pDefault9287->ctlIndex);

	/* CTL Data*/
    print9287_2GCTLData(client, pDefault9287->ctlIndex, pDefault9287->ctlData);
}

