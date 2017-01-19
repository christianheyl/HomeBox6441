//
// File: Qc98xxEepromSave.c
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "smatch.h"
#include "UserPrint.h"
#include "TimeMillisecond.h"

#include "Device.h"
#include "Qc98xxDevice.h"

#include "wlantype.h"
#include "dk_cmds.h"

#include "Qc9KEeprom.h"
#include "qc98xx_eeprom.h"
#include "qc98xxtemplate.h"
#include "Qc98xxmEep.h"
#include "Qc98xxEepromSave.h"
#include "Qc98xxEepromStructGet.h"
#include "Qc98xxPcieConfig.h"
#include "DevSetConfig.h"
#include "tlvCmd_if.h"
#include "DevConfigDiff.h"

#include "art_utf_common.h"
#include "otpstream_id.h"
#include "qc98xx_eeprom.h"
#include "Qc98xxmEep.h"
//#include "lz.h"
#include "DevNonEepromParameter.h"
#include "Qc9KEepromPrint.h"
#include "crc.h"
#include "Compress.h"
#include "templatelist.h"
#include "ErrorPrint.h"
#include "EepromError.h"
#include "NartVersion.h"
#include "Sticky.h"


A_UINT8 leading_contents[]  = {0x20, 0xff}; /* OTP bytes 10 and 11 */
A_UINT8 trailing_contents[] = {0x80, 0x80}; /* OTP bytes 31 and 32 */

/*
 * This is where we found the calibration data. really should be in the ah structure
 */
static int CalibrationDataSource = DeviceCalibrationDataNone;
static int CalibrationDataSourceAddress = 0;

/*
 * This is where we look for the calibration data. must be set before ath_attach() is called
 */
static int CalibrationDataTry = DeviceCalibrationDataNone;

#define MTEMPLATE 100
static unsigned int Qc98xxTemplateAllowedMany=1;
static unsigned int Qc98xxTemplateAllowed[MTEMPLATE]={qc98xx_eeprom_template_generic,0,0};

static int Compress=1;
static int SaveMemory=DeviceCalibrationDataNone;


//
// static function declarations
//
static A_BOOL WriteBoardDataToFile(QC98XX_EEPROM *pEeprom);
static A_BOOL WriteBoardDataToOtp(A_UINT8 *best, int bsize, int balgorithm, int breference);
//static A_BOOL WriteUSB_PID_VID_ToOtp();
static A_BOOL WriteXtalToOtp();

#ifdef AP_BUILD
extern QC98XX_EEPROM *Qc98xxEepromTemplatePtr[20];
static void Qc98xx_eeprom_template_swap(void);
void Qc98xx_swap_eeprom(QC98XX_EEPROM *eep);
#endif

int Qc98xxCalibrationDataAddressSet(int address)
{
	CalibrationDataSourceAddress = address;
	return 0;
}

int Qc98xxCalibrationDataAddressGet(void)
{
	return CalibrationDataSourceAddress;
}
/*
 * Set the type of memory used to store calibration data.
 * Used by nart to force reading/writing of a specific type.
 * The driver can normally allow autodetection by setting source to CalibrationDataNone=0.
 */
int Qc98xxCalibrationDataSet(int source)
{
	if(Qc98xxValid())
	{
		CalibrationDataSource=source;
	}
	else
	{
		CalibrationDataTry=source;
	}
    return 0;
}

int Qc98xxCalibrationDataGet(void)
{
	if(Qc98xxValid())
	{
		return CalibrationDataSource;
	}
	else
	{
		return CalibrationDataTry;
	}
}

#define MCHECK 20
//#define MOUTPUT 2048
/*
 * Read the configuration data from the eeprom and reports what it finds.
 * Does not restore the data.
 * This function closely parallels Qc98xxEepromRestore() in the hal.
 *
 * Returns -1 on error. 
 * Returns address of next memory location on success.
 */
int Qc98xxEepromReportAddress(void (*print)(char *format, ...), int all, int cptr)
{
    //A_UINT8 word[MOUTPUT]; 
    //A_UINT8 *dptr;
    //int code;
    //int reference,length,major,minor;
    //int osize;
    //int it;
    //A_UINT16 checksum, mchecksum;
	QC98XX_EEPROM mptr[MCHECK], *xptr;
	//char *ptr;
	int mcount;
	int mdataSize;

	mcount=0;
	//
	// this gets the first template as a default starting point
	//
	mdataSize=sizeof(QC98XX_EEPROM);
    //
	// return header 
	//
	if(print!=0)
	{
		(*print)("|ec|block|address|code|template|length|major|minor|csm|csc|status|");
		(*print)("|ecb|block|portion|offset|length|");
	}
    /*//
    // get a pointer to the current data structure
    //
//    msize=ar9300_eeprom_struct_size();
//    cptr=ar9300_eeprom_base_address(AH);
    for(it=0; it<MSTATE; it++)
    {            
        (void)ar9300_calibration_data_read_array(AH,cptr,word,compression_header_length);
        if((word[0]==0 && word[1]==0 && word[2]==0 && word[3]==0) || 
            (word[0]==0xff && word[1]==0xff && word[2]==0xff && word[3]==0xff))
        {
            break;
        }
        ar9300_compression_header_unpack(word, &code, &reference, &length, &major, &minor);
#ifdef DONTUSE
        if(length>=1024)
        {
            (*print)("|ec|%d|%x|%d|%d|%d|%d|%d|%x|%x|bad header|", mcount, cptr, code, reference, length, major, minor, 0, 0);
            cptr-=compression_header_length;
            continue;
        }
#endif
        osize=length;                
        (void)ar9300_calibration_data_read_array(AH,cptr,word,compression_header_length+osize+compression_checksum_length);
        checksum=ar9300_compression_checksum(&word[compression_header_length], length);
        mchecksum= word[compression_header_length+osize]|(word[compression_header_length+osize+1]<<8);
        if(checksum==mchecksum)
        {
			switch(code)
            {
                case _compress_none:
                    if(length!=mdataSize)
                    {
						if(print!=0)
						{
							(*print)("|ec|%d|%x|%d|%d|%d|%d|%d|%x|%x|bad size mismatch|", mcount, cptr, code, reference, length, major, minor, mchecksum, checksum);
						}
                        continue;
                    }
					else
					{
                        memcpy(&mptr[mcount],(A_UINT8 *)(word+compression_header_length),length);
						if(print!=0)
						{
							(*print)("|ec|%d|%x|%d|%d|%d|%d|%d|%x|%x|good|", mcount, cptr, code, reference, length, major, minor, mchecksum, checksum);
						}
						mcount++;
					}
                    break;
                case _compress_block:
                    if(reference!=reference_current)
					{
                        dptr=(A_UINT8 *)ar9300_eeprom_struct_default_find_by_id(reference);
                        if(dptr==0)
                        {
							if(print!=0)
							{
								(*print)("|ec|%d|%x|%d|%d|%d|%d|%d|%x|%x|bad template|", mcount, cptr, code, reference, length, major, minor, mchecksum, checksum);
							}
                            continue;
                        }
						else
						{
                            //
                            // save first reference structure as the first state
                            //
                            if(mcount==0)
                            {
     						    memcpy(&mptr[mcount],dptr,mdataSize);
                                mcount++;
                            }
                            //
                            // then apply the changes and save as next state
                            //
 						    memcpy(&mptr[mcount],dptr,mdataSize);
                            (void)LocalUncompressBlock(mcount,(A_UINT8 *)&mptr[mcount],mdataSize,(A_UINT8 *)(word+compression_header_length),length,print);
							if(print!=0)
							{
								(*print)("|ec|%d|%x|%d|%d|%d|%d|%d|%x|%x|good|", mcount, cptr, code, reference, length, major, minor, mchecksum, checksum);
							}
							mcount++;
						}
                    }
					else
					{
					    memcpy(&mptr[mcount],&mptr[mcount-1],mdataSize);
                        (void)LocalUncompressBlock(mcount,(A_UINT8 *)&mptr[mcount],mdataSize,(A_UINT8 *)(word+compression_header_length),length,print);
						if(print!=0)
						{
							(*print)("|ec|%d|%x|%d|%d|%d|%d|%d|%x|%x|good|", mcount, cptr, code, reference, length, major, minor, mchecksum, checksum);
						}
						mcount++;
					}
                    break;
                default:
					if(print!=0)
					{
						(*print)("|ec|%d|%x|%d|%d|%d|%d|%d|%x|%x|bad compression code|", mcount, cptr, code, reference, length, major, minor, mchecksum, checksum);
					}
                    return -1;
            }
        }
        else
        {
			if(print!=0)
			{
				(*print)("|ec|%d|%x|%d|%d|%d|%d|%d|%x|%x|bad check sum|", mcount, cptr, code, reference, length, major, minor, mchecksum, checksum);
			}
        }
        cptr-=(compression_header_length+osize+compression_checksum_length);
    }
 
	if(mcount<=0)
	{
		cptr= -1;
	}
	else*/
	{
		if(print!=0)
		{
			(*print)("|ec|%d|%x|%d|%d|%d|%d|%d|%x|%x|free|", mcount, cptr, 0, 0, 0, 0, 0, 0, 0);

			//
			// include the current state
			//
			if(mcount<MCHECK)
			{
				xptr=Qc98xxEepromStructGet();
				memcpy(&mptr[mcount],xptr,mdataSize);
				mcount++;
			}

			Qc9KEepromDifferenceAnalyze(print,mptr,mcount,all);
		}
	}
    return cptr;
}

/*
 * Read the configuration data from the eeprom and reports what it finds.
 * Does not restore the data.
 * This function closely parallels Qc98xxEepromRestore() in the hal.
 *
 * Returns -1 on error. 
 * Returns address of next memory location on success.
 */
int Qc98xxEepromReport(void (*print)(char *format, ...), int all)
{
    int cptr;
	int nptr;
	//
 	// first find the data with print turned off
 	//
 	nptr= -1;
 	cptr=Qc98xxCalibrationDataAddressGet();
 	if(cptr>0)
 	{
 		nptr=Qc98xxEepromReportAddress(0, 0, cptr);
 	}	
	if(nptr<0)
	{
		// #### want to look at highest eeprom address as well as at BaseAddress
		cptr=(int)Qc98xxEepromStructGet();
		nptr=Qc98xxEepromReportAddress(0, 0, cptr);
		if(nptr<0)
		{
			if(cptr!=0x3ff)
			{
				cptr=0x3ff;
				nptr=Qc98xxEepromReportAddress(0, 0, cptr);
			}
		}
	}
 	//
 	// then go and do the job that was requested
 	//
 	nptr=Qc98xxEepromReportAddress(print, all, cptr);
 
	return nptr;
}

//
// write the calibration data to file
//
void Qc98xxEepromFile(int value)
{
	SaveMemory = DeviceCalibrationDataFile;
}

A_BOOL Qc98xxLoadOtp()
{
    A_UCHAR buffer[OTPSTREAM_MAXSZ_APP];
    A_UCHAR *pCurStream=NULL;
    A_UCHAR *mptr, *dptr;
    QC98XX_OTP_STREAM_HEADER *pStreamHdr;
    A_UINT32 nbytes, nstreams;
    A_UINT16 checksum, mchecksum;
    A_BOOL status;
    int code, reference, length, major, minor, msize;
    int curReference = -1;


    // To reset the OTP read pointer to the beginning of OTP
    if (art_otpReset(OTPSTREAM_READ_APP) != A_OK)
    {
        UserPrint("Ar6004LoadOTP2ART: error in art_otpReset\n");
        return(FALSE);
    }
    nstreams = 0;
    nbytes = 0;
	mptr = pQc9kEepromArea;
	msize = sizeof(QC98XX_EEPROM);
	status = TRUE;
	memset(buffer, 0, sizeof(buffer));
 
#if AP_BUILD
    Qc98xx_eeprom_template_swap();
#endif

    // read stream by stream and process
    while (((status = art_otpRead(buffer, &nbytes)) == A_OK) && (nbytes > 0))
    {
#ifdef _DEBUG  
        A_UINT32 i;
        UserPrint("Qc98xxLoadOtp: OTP stream - %d", nstreams);
        for (i=0; i< nbytes; i++)
        {
            if ((i % 16) == 0) UserPrint("\n");
            UserPrint("%02x  ", buffer[i]);
        }
        UserPrint("\n");
#endif
        pCurStream = buffer;
       
		// check stream header
        pStreamHdr = (QC98XX_OTP_STREAM_HEADER *)pCurStream;
        if ((pStreamHdr->applID != QC98XX_OTP_APPL_ID_WLAN) || (pStreamHdr->version != QC98XX_OTP_VER_1))
        {
            if ((nstreams == 0) && (pStreamHdr->applID == 0) && (pStreamHdr->version == 0))
            {
                UserPrint("NO OTP DATA\n");
                return TRUE;
            }
            else 
            {
                //UserPrint("OTP STREAM HEADER ERROR\n");
                continue;
            }
        }
        UserPrint("OTP version = %d\n", pStreamHdr->version);

	    // get compression header
	    pCurStream += sizeof(QC98XX_OTP_STREAM_HEADER);
	    CompressionHeaderUnpack (pCurStream, &code, &reference, &length, &major, &minor);

	    // check if there is error in length
        if(length >= OTPSTREAM_MAXSZ_APP)
        {
            UserPrint("skipping bad header\n");
            continue;
        }

	    if (nbytes != (sizeof(QC98XX_OTP_STREAM_HEADER) + COMPRESSION_HEADER_LENGTH + length + COMPRESSION_CHECKSUM_LENGTH))
	    {
		    UserPrint("not match in stream length and data length, skip!!!\n");
		    continue;
	    }
        //
        // process data
        //
        pCurStream += COMPRESSION_HEADER_LENGTH;                
        //
        // compute and check the checksum;
        //
        checksum = CompressionChecksum(pCurStream, length);
        mchecksum = *(pCurStream+length) | (*(pCurStream+length+1) << 8);
        //printf("checksum %x %x\n",checksum,mchecksum);
        if(checksum==mchecksum)
        {
            switch(code)
            {
                case _compress_none:
					// should not be here!!!!
                    if(length != msize)
                    {
                        UserPrint("eeprom struct size mismatch memory=%d eeprom=%d\n",msize,length);
                        return -1;
                    }
                    //
                    // interpret the data
                    //
                    memcpy(mptr, pCurStream, length);
//                    printf("restored eeprom %d: uncompressed, length %d\n",it,length);
                    break;
#ifdef UNUSED
                case _compress_lzma:
                    //
                    // find the reference data
                    //
                    if(reference==reference_current)
                    {
                        dptr=mptr;
                    }
                    else
                    {
                        dptr=(unsigned char *)Ar9300EepromStructDefault(reference);
                        if(dptr==0)
                        {
//                            printf("cant find reference eeprom struct %d\n",reference);
                            return -1;
                        }
                    }
                    //
                    // uncompress the data
                    //
                    usize= -1;
    //                usize=UnCompressLzma(word+overhead,length,output,MOUTPUT);
                    if(usize!=msize)
                    {
//                        printf("uncompressed data is wrong size %d %d\n",usize,msize);
                        return -1;
                    }
                    //
                    // interpret the data
                    //
                    for(ib=0; ib<msize; ib++)
                    {
                        mptr[ib]=dptr[ib]^word[ib+overhead];
                    }
//                    printf("restored eeprom %d: compressed, reference %d, length %d\n",it,reference,length);
                    break;
                case _compress_pairs:
                    //
                    // find the reference data
                    //
                    if(reference==reference_current)
                    {
                        dptr=mptr;
                    }
                    else
                    {
                        dptr=(unsigned char *)Ar9300EepromStructDefault(reference);
                        if(dptr==0)
                        {
//                            printf("cant find reference eeprom struct %d\n",reference);
                            return -1;
                        }
                    }
                    //
                    // interpret the data
                    //
                    // NEED SOMETHING HERE
//                    printf("restored eeprom %d: pairs, reference %d, length %d, \n",it,reference,length);
                    break;
#endif
                case _compress_block:
                    //
                    // find the reference data
                    //
                    if(reference != REFERENCE_CURRENT)
                    {
						// First stream, get the template to eeprom area; 
						// or if the subsequence stream uses different template, get the new template
						if (curReference == -1 || curReference != reference)
						{
							dptr = (A_UCHAR *)Qc98xxEepromStructDefaultFindByTemplateVersion(reference);
							if(dptr==0)
							{
								UserPrint("cant find reference eeprom struct %d\n",reference);
								return -1;
							}
							curReference = reference;
							// copy the template to pQc9kEepromArea
							memcpy (mptr, dptr, msize);
						}
                    }
                    //
                    // interpret the data
                    //
//                    printf("restore eeprom %d: block, reference %d, length %d\n",it,reference,length);
                    UncompressBlock (mptr, msize, pCurStream, length);
                    break;
                default:
                    UserPrint("unknown compression code %d\n",code);
                    return -1;
            }
        }
        else
        {
            UserPrint("skipping block with bad checksum\n");
        }
        nstreams++;
    }// end of while(art_otpRead)
        
    if (status != A_OK)
    {
        UserPrint("Qc98xxLoadOtp: Error in art_otpRead\n");
        return(FALSE);
    }

    // if empty OTP, get the preference template, or default template
    if (nstreams == 0)
    {
	dptr = (A_UCHAR *)Qc98xxEepromStructDefaultFindById(Qc98xxGetEepromTemplatePreference());
	if (!dptr)
	{
		dptr = (A_UCHAR *)Qc98xxEepromStructDefaultFindById(qc98xx_eeprom_template_default);
		UserPrint("OTP is empty. Use the generic template\n");
	}
	memcpy (mptr, dptr, msize);
    }

    computeChecksum((QC98XX_EEPROM *)mptr);

#if AP_BUILD
    Qc98xx_eeprom_template_swap();
    Qc98xx_swap_eeprom(mptr);
#endif

    return TRUE;
}

int Qc98xxEepromTemplateAllowed(unsigned int *value, unsigned int many)
{
	unsigned int it;

	if(many>MTEMPLATE)
	{
		many=MTEMPLATE;
	}
	for(it=0; it<many; it++)
	{
		Qc98xxTemplateAllowed[it]=value[it];
	}
	Qc98xxTemplateAllowedMany=many;

	return 0;
}

//
// do not compress the data
// this flag is overridden if the data will not fit uncompressed in the available memory
//
int Qc98xxEepromCompress(unsigned int value)
{
	Compress=value;
	return 0;
}

int Qc98xxEepromSaveMemorySet(int memory)
{
	SaveMemory=memory;
	return 0;
}

static A_BOOL WriteBoardDataToFile(QC98XX_EEPROM *pEeprom)
{
    A_UINT16 *pData;
    A_UINT32 address;
    A_CHAR fileName[MAX_FILE_LENGTH];
	A_CHAR fullFileName[MAX_FILE_LENGTH];
    FILE *fStream;
    A_BOOL rc;
    A_UINT32 eepromSize;
		
    eepromSize = sizeof(QC98XX_EEPROM);

    // Write to a text file
    if (pEeprom->baseEepHeader.custData[0] != 0)
    {
        sprintf(fileName, "calData_%s_%02x%02x%02x%02x%02x%02x.txt", 
			pEeprom->baseEepHeader.custData,
			pEeprom->baseEepHeader.macAddr[0], pEeprom->baseEepHeader.macAddr[1], pEeprom->baseEepHeader.macAddr[2],
			pEeprom->baseEepHeader.macAddr[3], pEeprom->baseEepHeader.macAddr[4], pEeprom->baseEepHeader.macAddr[5]);
    }
    else
    {
        sprintf(fileName, "calData_%02x%02x%02x%02x%02x%02x.txt", 
                    pEeprom->baseEepHeader.macAddr[0], pEeprom->baseEepHeader.macAddr[1], pEeprom->baseEepHeader.macAddr[2],
                    pEeprom->baseEepHeader.macAddr[3], pEeprom->baseEepHeader.macAddr[4], pEeprom->baseEepHeader.macAddr[5]);
    }
	strcpy (fullFileName, configSetup.boardDataPath);
	strcat (fullFileName, fileName);
    if( (fStream = fopen(fullFileName, "w")) == NULL)
    {
        UserPrint("Could not open calDataFile %s\n", fullFileName);
        return FALSE;
    }

    pData = (A_UINT16 *)pEeprom;
        
    for (address = 0; address < eepromSize/2;  address++)
    {
        fprintf(fStream, "%04x    ;%04x\n", pData[address], address*2);
    }
    fclose(fStream);

    // Write to a binary eeprom
    memcpy(&fullFileName[strlen(fullFileName)-3], "bin", 3);
    /*if (pEeprom->baseEepHeader.custData[0] != 0)
    {
        sprintf(fileName, "calData_%s.bin", pEeprom->baseEepHeader.custData);
    }
    else
    {
        sprintf(fileName, "calData_%02x%02x%02x%02x%02x%02x.bin", 
                    pEeprom->baseEepHeader.macAddr[0], pEeprom->baseEepHeader.macAddr[1], pEeprom->baseEepHeader.macAddr[2],
                    pEeprom->baseEepHeader.macAddr[3], pEeprom->baseEepHeader.macAddr[4], pEeprom->baseEepHeader.macAddr[5]);
    }*/
    if( (fStream = fopen(fullFileName, "wb")) == NULL)
    {
        UserPrint("Could not open calDataFile %s to write\n", fullFileName);
        return FALSE;
    }

    if (eepromSize != fwrite((A_UCHAR *)pEeprom, 1, eepromSize, fStream))
    {
        UserPrint("Error writing to %s\n", fullFileName);
        rc = FALSE;
    }
    else
    {
        UserPrint("... written to %s\n", fullFileName);
        rc = TRUE;
    }
    fclose(fStream);
    return rc;
}

static void CreateCompressionHeader(A_UINT8 *header, int balgorithm, int breference, int bsize)
{
    int major, minor;

	major=NartVersionMajor();
    minor=NartVersionMinor();
    CompressionHeaderPack(header, balgorithm, breference, bsize, major, minor);
}

A_BOOL Qc98xxWriteFixedBytesIfOtpEmpty(A_BOOL otpEmptyChecked)
{
    A_UCHAR buffer[OTPSTREAM_MAXSZ_APP];
    A_UINT32 nbytes;
    A_BOOL status;

	// Write fixed bytes to OTP for chip v1 only
	if (!Qc98xxIsVersion1())
	{
		return FALSE;
	}

	// if has not checked for OTP empty, do it here
	if (!otpEmptyChecked)
	{
		// To reset the OTP read pointer to the beginning of OTP
		if (art_otpReset(OTPSTREAM_READ_APP) != A_OK)
		{
			UserPrint("Qc98xxWriteFixedBytesIfOtpEmpty: error in art_otpReset\n");
			return(FALSE);
		}
		nbytes = 0;
		status = TRUE;
		memset(buffer, 0, sizeof(buffer));
 
		// check if the OTP is empty
		status = art_otpRead(buffer, &nbytes);
		if (status != A_OK || nbytes)
		{
			return FALSE; //OTP is not empty, no write, return
		}
	}
    // Efuse Write
    if (A_OK != art_efuseWrite((A_UCHAR *)&leading_contents[0], 2, 10) ) 
	{
        UserPrint("Efuse Write leading_contents FAILED\n");
    }

    /* NB: Yes, the 32nd and 33rd byte of OTP. */
    if (A_OK != art_efuseWrite((A_UCHAR *)&trailing_contents[0], 2, 31) ) 
	{
        UserPrint("Efuse Write trailing_contents FAILED\n");
    }
	return TRUE;
}

static A_BOOL WriteBoardDataToOtp(A_UINT8 *best, int bsize, int balgorithm, int breference)
{
    A_UINT8 stream[OTPSTREAM_MAXSZ_APP];
    QC98XX_OTP_STREAM_HEADER *pStreamHdr;
    A_UINT8 *pStream;
    A_BOOL rc;//, streamGood;
    A_INT32 i;//, repeatedReadCount, goodReadCount;
    int streamLen;
    unsigned short checksum;

    rc = TRUE;
    pStreamHdr = (QC98XX_OTP_STREAM_HEADER *)stream;
    pStreamHdr->applID = QC98XX_OTP_APPL_ID_WLAN;
    pStreamHdr->version = QC98XX_OTP_VER_1;
    
    pStream = stream + sizeof(QC98XX_OTP_STREAM_HEADER);
	
	// Fill compression header to the stream
	CreateCompressionHeader(pStream, balgorithm, breference, bsize);
	pStream += COMPRESSION_HEADER_LENGTH;
	// Fill the stream with data if any
	if (bsize > 0)
	{
		memcpy (pStream, best, bsize);
		pStream += bsize;
		checksum = CompressionChecksum (best, bsize);
		*pStream++ = (unsigned char)(checksum & 0xff);
		*pStream++ = (unsigned char)((checksum >> 8) & 0xff);
	}

	streamLen = pStream - stream;
    if (streamLen > sizeof(stream))
    {
        UserPrint("WriteBoardDataToOtp - stream overflowed\n");
        return FALSE;
    }

    for (i = 0; i < streamLen; ++i)
    {
        if (i > 0 && ((i % 16) == 0))
        {
            UserPrint("\n");
        }
        UserPrint("0x%02x ", stream[i]);
    }
    UserPrint("\nOTP Stream length = %d\n", streamLen);

    // write to OTP
	Qc98xxWriteFixedBytesIfOtpEmpty(0);

    if (A_OK != art_otpWrite(stream, streamLen))
    {
        UserPrint("Error: art_otpWrite in WriteCalDataToOtp\n");
        //incrDebugCounter(0, OTP_WRITE_ERROR);
        rc = FALSE;
    }
    else
    {
        UserPrint(".OTP written %d bytes\n", streamLen); 
        rc = TRUE;
    }
    //EepromSaveSectionSource = DeviceEepromSaveSectionAll; //default all sections (same as if bit 0 is set)
    return(rc);
}

int Qc98xxEepromSave()
{
    QC98XX_EEPROM *mptr;        // pointer to data
    int msize;
    char best[sizeof(QC98XX_EEPROM)];
    int it;
    QC98XX_EEPROM *dptr;
    int dsize;
    int bsize;
    int breference;
    int balgorithm;
    int status;
	int calmem;
    int mreference;

	//
    // get a pointer to the current data structure
    //
    mptr = Qc98xxEepromStructGet();
    msize = sizeof(QC98XX_EEPROM);

	// check if memory parameter set in commit command
	calmem = SaveMemory;
	if(calmem == DeviceCalibrationDataNone)
	{
		// if no, get memory parameter value in load command
		calmem = Qc98xxCalibrationDataGet();
	}
	if(calmem == DeviceCalibrationDataNone)
	{
		// if none is set, save to OTP
		calmem=DeviceCalibrationDataOtp;
	}
    // push cal data to DUT
    CalInfoExecute();

	// push "sw" to config area in eeprom
	StickyListToEeprom(DEF_LINKLIST_IDX);

#if AP_BUILD
    Qc98xx_eeprom_template_swap();
    Qc98xx_swap_eeprom(mptr);
    dptr = (QC98XX_EEPROM *)pQc9kEepromBoardArea;
    Qc98xx_swap_eeprom(dptr);
#endif

	if(calmem == DeviceCalibrationDataFile)
	{
		// calculate chechsum 
		computeChecksum(mptr);
		// write the whole board data to a file, no need for compression
		if ((status = WriteBoardDataToFile(mptr)) == 0)
		{
			return -1;
		}
		UserPrint("data written successfully \n");
		status = msize;
        goto return_status;
	}

   
    // For AP with flash calibration data storage save structure uncompressed.
#ifdef AP_BUILD
    if(calmem==DeviceCalibrationDataFlash)
    {
        int fd;
        int offset;
        //int it;
        //unsigned char *buffer;

        computeChecksum(mptr);
        //buffer=(unsigned char *)mptr;
        //for(it=0;it<2116;it++)
        //     printf("a = %x, data = %x \n",it, buffer[it]);

        if((fd = open("/dev/caldata", O_RDWR)) < 0) {
            perror("Could not open flash\n");
            status = -1 ;
            goto return_status;
        }

        // First 0x1000 are reserved for ethernet mac address and other config writes.
        offset = MAX_EEPROM_SIZE+FLASH_BASE_CALDATA_OFFSET;  // Need for boards with more than one radio
        lseek(fd, offset, SEEK_SET);
        if (write(fd, mptr, msize) < 1) {
            perror("\nwrite\n");
            status = -2 ;
            goto return_status;
        }
		close(fd);

        status = msize ;
        goto return_status;
    }    
#endif

	if (calmem == DeviceCalibrationDataOtp)
	{

	    //
		// try all of our compression schemes, 
		// starting with the assumption that uncompressed is best
		//
		balgorithm=_compress_none;
		breference= -1;
	    bsize=msize;
	
		//
		// if compression is requested or if the uncompressed data won't fit
		//
		if(Compress)
		{
			mreference = mptr->baseEepHeader.template_version;
			// if no template version
			if (Qc98xxEepromTemplateVersionValid(mreference) == 0)
			{
				mreference = QC98XX_TEMPLATEID_TO_TEMPLATEVERSION(Qc98xxGetEepromTemplatePreference());
				if (Qc98xxEepromTemplateVersionValid(mreference) == 0)
				{
					mreference = QC98XX_FIRST_TEMPLATEVERSION;
				}
			}
			// First check the different between Qc98xxEepromArea and  Qc98xxEepromBoardArea
			dptr = (QC98XX_EEPROM *)pQc9kEepromBoardArea;
			dsize = msize;
			if(dptr!=0 && dsize<MOUTPUT && dsize==msize)
			{
				CheckCompression((char *)mptr,msize,(char *)dptr,mreference,&balgorithm,&breference,&bsize,best,MOUTPUT);
			}
			
			// If compressed size is too big
			if(bsize>=msize)
			{
				// try difference with each standard default structure
				//
				if(Qc98xxTemplateAllowedMany<=0)
				{
					for(it=0; it<Qc98xxEepromStructDefaultMany(); it++)
					{
						dptr=Qc98xxEepromStructDefault(it);
						dsize=msize;
						if(dptr!=0 && dsize<MOUTPUT && dsize==msize)
						{
							CheckCompression((char *)mptr,msize,(char *)dptr,dptr->baseEepHeader.template_version,&balgorithm,&breference,&bsize,best,MOUTPUT);
						}
					}
				}
				else
				{
					for(it=0; it < (int)Qc98xxTemplateAllowedMany; it++)
					{
						dptr=Qc98xxEepromStructDefaultFindById((int)Qc98xxTemplateAllowed[it]);
						dsize=msize;
						if(dptr!=0 && dsize<MOUTPUT && dsize==msize)
						{
							CheckCompression((char *)mptr,msize,(char *)dptr,dptr->baseEepHeader.template_version,&balgorithm,&breference,&bsize,best,MOUTPUT);
						}
					}
				}
			}
		}

		//no different, no template reference, return
		if (bsize == 0)
		{
			return bsize;
		}
		//
		// if the uncompressed size is the smallest, might as well go with it
		//
		if(bsize>=msize)
		{
	        balgorithm=_compress_none;
			breference= REFERENCE_CURRENT;
			bsize=msize;
			memcpy (best,(void *)mptr, msize);        // the uncompressed data
			//
			// if using OTP we have to find the first free spot.
			// if using eeprom, we can overwrite
			//
		}
		//
		// Now we know the best method and we have the data, so write it
		//
		ErrorPrint(EepromAlgorithm,balgorithm,breference,bsize);
        if ( (status = WriteBoardDataToOtp ((unsigned char *)best, bsize, balgorithm, breference)) == 0)
		{
			return -1;
		}
        

		// Update EEPROM areas after commit to OTP
		computeChecksum(mptr);
		memcpy (pQc9kEepromBoardArea, pQc9kEepromArea, sizeof(QC98XX_EEPROM));
		Qc98xxCalibrationDataSet(calmem);
		
		status = bsize;
	}
return_status:
#if AP_BUILD
    Qc98xx_eeprom_template_swap();
    Qc98xx_swap_eeprom(mptr);
    dptr = (QC98XX_EEPROM *)pQc9kEepromBoardArea;
    Qc98xx_swap_eeprom(dptr);
#endif


    return status;    
}

#ifdef AP_BUILD
void Qc98xx_swap_eeprom(QC98XX_EEPROM *eep)
{
    u_int32_t dword;
    u_int16_t word;
    int i,j,k;

    word = SWAP16(eep->baseEepHeader.length);
    eep->baseEepHeader.length = word;
    word = SWAP16(eep->baseEepHeader.checksum);
    eep->baseEepHeader.checksum = word;
    word = SWAP16(eep->baseEepHeader.regDmn[0]);
    eep->baseEepHeader.regDmn[0] = word;
    word = SWAP16(eep->baseEepHeader.regDmn[1]);
    eep->baseEepHeader.regDmn[1] = word;
    dword = SWAP32(eep->baseEepHeader.opCapBrdFlags.boardFlags);
    eep->baseEepHeader.opCapBrdFlags.boardFlags = dword;
    word = SWAP16(eep->baseEepHeader.opCapBrdFlags.blueToothOptions);
    eep->baseEepHeader.opCapBrdFlags.blueToothOptions = word;
    word = SWAP16(eep->baseEepHeader.opCapBrdFlags.flag2);
    eep->baseEepHeader.opCapBrdFlags.flag2 = word;
    word = SWAP16(eep->baseEepHeader.binBuildNumber);
    eep->baseEepHeader.binBuildNumber = word;

    for(i=0;i<WHAL_NUM_BI_MODAL;i++)
    {
        dword = SWAP32(eep->biModalHeader[i].antCtrlCommon);
        eep->biModalHeader[i].antCtrlCommon = dword;
        dword = SWAP32(eep->biModalHeader[i].antCtrlCommon2);
        eep->biModalHeader[i].antCtrlCommon2 = dword;

        for(j=0;j<WHAL_NUM_CHAINS;j++)
        {
            word = SWAP16(eep->biModalHeader[i].antCtrlChain[j]);
            eep->biModalHeader[i].antCtrlChain[j] = word;
        }
    }

    word = SWAP16(eep->chipCalData.thermAdcScaledGain);
    eep->chipCalData.thermAdcScaledGain = word;

    for(i=0;i<WHAL_NUM_11G_CAL_PIERS;i++)
    {
        for(j=0;j<WHAL_NUM_CHAINS;j++)
        {
            for(k=0;k<WHAL_NUM_CAL_GAINS;k++)
            {
                word = SWAP16(eep->calPierData2G[i].calPerPoint[j].power_t8[k]);
                eep->calPierData2G[i].calPerPoint[j].power_t8[k] = word;
            }
        }
    }

    for(i=0;i<WHAL_NUM_11A_CAL_PIERS;i++)
    {
        for(j=0;j<WHAL_NUM_CHAINS;j++)
        {
            for(k=0;k<WHAL_NUM_CAL_GAINS;k++)
            {
                word = SWAP16(eep->calPierData5G[i].calPerPoint[j].power_t8[k]);
                eep->calPierData5G[i].calPerPoint[j].power_t8[k] = word;
            }
        }
    }
}

void Qc98xx_eeprom_template_swap(void)
{
    int it;
    QC98XX_EEPROM *dptr;

    for (it = 0; it < 20; it++) {
        dptr = Qc98xxEepromTemplatePtr[it];
        if (dptr != NULL) {
            Qc98xx_swap_eeprom(dptr);
        } else {
            return;
        }
    }
}
#endif
