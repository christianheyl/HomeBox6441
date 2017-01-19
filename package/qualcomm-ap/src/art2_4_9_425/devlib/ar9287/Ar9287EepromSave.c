#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wlantype.h"
//#include "opt_ah.h"
#include "ah.h"
#include "ar5416/ar5416.h"
#include "ar5416/ar5416eep.h"
#include "UserPrint.h"  
#undef REGR
#undef REGW

#include "Ar9287Device.h"
#include "Ar9287EepromStructSet.h"
#include "Ar9287EepromSave.h"
#include "mEepStruct9287.h"

// 
// this is the hal pointer, 
// returned by ath_hal_attach
// used as the first argument by most (all?) HAL routines
//
struct ath_hal *AH;



extern int MyRegisterRead(unsigned long address, unsigned long *value);
extern int MyRegisterWrite(unsigned long address, unsigned long value);

static unsigned long REGR(unsigned long devNum, unsigned long address)
{
	unsigned long value;

	devNum=0;

	MyRegisterRead(address,&value);

	return value;
}

static void REGW(unsigned long devNum,unsigned long address, unsigned long value)
{
    devNum =0;
    MyRegisterWrite(address, value);
}

int Ar9287EepromRead(unsigned int address, unsigned char *value, int count)
{
    unsigned long eepromValue, status, address_align;
    int           to = 50000;
    int           i;
    
    for (i=0; i<count; i++)
    {
        //address_align = 4*(address/2);
        address_align = 4*((address+i)/2);
    
        /*read the memory mapped eeprom location*/
        eepromValue = REGR(0, address_align+0x2000);
    
    
	    /*check busy bit to see if eeprom read succeeded and get valid data in read register*/
        while (to > 0 )
        {
            status = REGR(0, 0x407C) & 0x10000;
            if (status == 0) 
            {
                eepromValue = REGR(0, 0x407C);
                if ((address+i) & 0x01)
                {
                    //*value = (unsigned char)((eepromValue >> 8) & 0xff);
                    value[i] = (unsigned char)((eepromValue >> 8) & 0xff);
                }
                else
                {
                    //*value = (unsigned char)(eepromValue & 0xff);
                    value[i] = (unsigned char)(eepromValue & 0xff);
                }
                break;
                //return 0;
            } 
            to--;
        }
    } // end of for()
    if (to ==0)
        return 1; //bad return
    else
        return 0;
}

int Ar9287EepromWrite(unsigned int address, unsigned char *value, int count)
{
    int           to = 50000;
	int           i;
    unsigned long status, address_align;
    unsigned long devNum=0;
    unsigned long read_address;
    unsigned char read_value;
    unsigned long write_value;
    
    for (i=0; i<count; i++)
    {
		to = 50000;

        //read a value from EEPROM
        if ((address+i) & 0x1)
        {
            read_address = address + i - 1;
            Ar9287EepromRead(read_address, &read_value, 1);
			read_value &= 0xff;
            write_value = (value[i]<<8) | read_value;
        }
        else
        {
            read_address = address + i + 1;
            Ar9287EepromRead(read_address, &read_value, 1);
			read_value &= 0xff;
            write_value = (read_value<<8) | value[i];
        }

        //gpio configuration, set GPIOs output to value set in output reg
        REGW(devNum, 0x4054, REGR(devNum, 0x4054) | 0x20000); 
        REGW(devNum, 0x4060, 0);
        REGW(devNum, 0x4064, 0);
    
        //GPIO3 always drive output
        REGW(devNum, 0x404c, 0xc0);  
    
        //Set 0 on GPIO3
        REGW(devNum, 0x4048, 0x0);
    
        address_align = 4*((address+i)/2);
    
        //write to the memory mapped eeprom location 
        REGW(0, (0x2000 + address_align), write_value);
    
        //check busy bit to see if eeprom write succeeded
        while (to > 0) 
        {
            status = REGR(0, 0x407C) & 0x10000;
            if (status == 0)
            {
                break;
                //return 1;
            }
            to--;
        }
    }//end of for()
    if (to ==0)
        return 1;//bad return;
    else
        return 0;
}

void ar9287_computeCheckSum( ar9287_eeprom_t* pEepStruct)
{
    int eeprom_size = sizeof(ar9287_eeprom_t);
    int i;
    A_UINT16* u16_ptr; 
    A_UINT16 sum=0x0000;
    
    u16_ptr = (A_UINT16*) pEepStruct;
    for (i=0; i< (eeprom_size/2); i++)
    {
        sum ^= u16_ptr[i];
    }
    if (sum != 0xffff)
    {
        sum = (~sum)^(pEepStruct->baseEepHeader.checksum);
        //UserPrint("Generated checksum (%x) and the Existing checksum (%x) are different.. \n",sum, pEepStruct->baseEepHeader.checksum);
        pEepStruct->baseEepHeader.checksum = sum;
    }
    else
    {
        //UserPrint("Generated checksum and the Existing checksum are the same..\n");
    }
}

static int SaveAddress=0x3ff;

AR9287DLLSPEC int Ar9287EepromSaveAddressSet(int address)
{
	SaveAddress=address;
	return 0;
}

static int SaveMemory=calibration_data_none;

AR9287DLLSPEC int Ar9287EepromSaveMemorySet(int memory)
{
	SaveMemory=memory;
	return 0;
}

int Ar9287EepromSave(void)
{
 	struct ath_hal_5416 *ahp;
    ar9287_eeprom_t *eeprom_ptr;
    A_UINT8  *pPtr;
#ifdef ATH_SUPPORT_HTC
    int eep_start_location = AR9287_EEP_START_LOC;
#else
    int eep_start_location = AR9287_EEP_START_LOC<<1;
#endif
    int struct_length = sizeof(ar9287_eeprom_t);
    //
    // get a pointer to the current data structure
    //
	ahp=AH5416(AH);
		
    eeprom_ptr=(ar9287_eeprom_t *)&ahp->ah_eeprom;
    pPtr =   (A_UINT8 *)eeprom_ptr;
    
    //Compute checksum once all other areas are filled.
    ar9287_computeCheckSum(eeprom_ptr);  
    
    //write data into EEPROM    
    if (Ar9287EepromWrite(eep_start_location, pPtr, struct_length))
        return -1;//bad return
    else
        return struct_length;
}

#include "instance.h"
HAL_BOOL Ar9287FlashSave(struct ath_hal *ah)
{
    int status;
    int calmem;
    struct ath_hal_5416 *ahp = AH5416(ah);
    ar9287_eeprom_t *eep = &ahp->ah_eeprom.map.mapAr9287;

	Ar9287_ChecksumCalculate();

		#ifdef MDK_AP
			int fd;
			int offset;
			ar9287_eeprom_t *mptr;		  // pointer to data
			int msize;
			int addr;
			
			mptr=Ar9287EepromStructGet();
			msize=sizeof(ar9287_eeprom_t);

			if((fd = open("/dev/caldata", O_RDWR)) < 0) {
				perror("Could not open flash\n");
				status = -1 ;
			}

#define AR9287_FLASH_SIZE 16*1024	// byte addressable
#define AR9287_PCIE_CONFIG_SIZE 0x100  // byte addressable
#define FLASH_BASE_CALDATA_OFFSET  0x1000
			// First 0x100 bytes from offset 0x5000 are reserved for pcie config writes.
			offset = instance*AR9287_FLASH_SIZE+FLASH_BASE_CALDATA_OFFSET+AR9287_PCIE_CONFIG_SIZE;  // Need for boards with more than one radio
			lseek(fd, offset, SEEK_SET);
			if (write(fd, mptr, msize) < 1) {
				perror("\nwrite\n");
				status = -2 ;
			}
			close(fd);

			status = msize ;
		#endif

    return AH_TRUE;
}

extern AR9287DLLSPEC int Ar9287CalibrationDataAddressSet(int size)
{
	ar5416_calibration_data_address_set(AH, size);
	return 0;
}

extern AR9287DLLSPEC int Ar9287CalibrationDataAddressGet(void)
{
	return ar5416_calibration_data_address_get(AH);
}

extern AR9287DLLSPEC int Ar9287CalibrationDataSet(int source)
{
	ar5416_calibration_data_set(AH, source);
	return 0;
}

extern AR9287DLLSPEC int Ar9287CalibrationDataGet(void)
{
	return ar5416_calibration_data_get(AH);
}

A_INT32 Ar9287ConfigurationSave(void) 
{
	unsigned char header[4];
	int cal_mem=0;

	    if(ar5416_eeprom_size(AH)>0)
		{
			cal_mem=calibration_data_eeprom;
		}
		else if(ar9287_calibration_data_read_flash(AH, 0x1000, header, 1)==AH_TRUE)
		{
			cal_mem=calibration_data_flash;
		}
		else
		{
			cal_mem=calibration_data_otp;
		}

	switch(cal_mem)
	{
		case calibration_data_eeprom:
			return Ar9287EepromSave();
		case calibration_data_flash:
			return Ar9287FlashSave(AH);
		case calibration_data_otp:
			//return Ar9287ConfigSpaceCommitOtp();
			return 0;
	}
	return -1;
}

/*
void Ar9287SetTargetPowerFromEeprom(struct ath_hal *ah, u_int16_t freq)
{
    struct ath_hal_5416 *ahp = AH5416(ah);
    struct ath_hal_private *ahpriv = AH_PRIVATE(ah);
    HAL_CHANNEL_INTERNAL *ichan = ahpriv->ah_curchan;
    HAL_CHANNEL *chan = (HAL_CHANNEL *)ichan;
    
    if (ar5416EepromSetTransmitPower(ah, &ahp->ah_eeprom, ichan,
        ath_hal_getctl(ah, chan), ath_hal_getantennaallowed(ah, chan),
        chan->maxRegTxPower * 2,
        AH_MIN(MAX_RATE_POWER, ahpriv->ah_powerLimit)) != HAL_OK)
    {
        printf("ar5416EepromSetTransmitPower error\n");
    }
}
*/
int Ar9287EepromReport(void (*print)(char *format, ...), int all)
{
#ifdef ATH_SUPPORT_HTC
    int eep_start_location = AR9287_EEP_START_LOC;
#else
    int eep_start_location = AR9287_EEP_START_LOC<<1;
#endif
    int struct_length = sizeof(ar9287_eeprom_t);
    ar9287_eeprom_t eeprom_data;
    int reference,length,major,minor;
    u_int16_t checksum, mchecksum;
    int mcount;
    int cptr;
    int code;
    
    (*print)("|ec|block|address|code|template|length|major|minor|csm|csc|status|");
	(*print)("|ecb|block|portion|offset|length|");
	
	Ar9287EepromRead(eep_start_location, (unsigned char *)&eeprom_data, struct_length);
	length = eeprom_data.baseEepHeader.length;
	major = (eeprom_data.baseEepHeader.version >> 12) & 0xF;  
	minor = eeprom_data.baseEepHeader.version & AR9287_EEP_VER_MINOR_MASK;
	reference = 0; //kiwi is no this value
	mcount = 1;
	cptr = 0;
	code = 0;
	checksum = eeprom_data.baseEepHeader.checksum;
	mchecksum = eeprom_data.baseEepHeader.checksum;
	
	if (eeprom_data.baseEepHeader.length != struct_length)
	{
	    (*print)("|ec|%d|%x|%d|%d|%d|%d|%d|%x|%x|bad header|", mcount, cptr, code, reference, length, major, minor, 0, 0);
	}
	else
	{
	    (*print)("|ec|%d|%x|%d|%d|%d|%d|%d|%x|%x|good|", mcount, cptr, code, reference, length, major, minor, mchecksum, checksum);
	}
	(*print)("|ec|%d|%x|%d|%d|%d|%d|%d|%x|%x|free|", mcount, cptr, 0, 0, 0, 0, 0, 0, 0);
	
	return cptr;
}
