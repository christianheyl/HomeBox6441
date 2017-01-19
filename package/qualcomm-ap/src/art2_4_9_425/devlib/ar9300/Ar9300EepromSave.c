
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "AquilaNewmaMapping.h"

#include "smatch.h"
#include "TimeMillisecond.h"

#include "Ar9300Device.h"

#include "ErrorPrint.h"
#include "EepromError.h"

#include "wlantype.h"

#if !defined(LINUX) && !defined(__APPLE__)
#include "osdep.h"
#endif


#include "Ar9300EepromSave.h"
#include "Ar9300PcieConfig.h"

#include "ah.h"
#include "ah_internal.h"
#include "ar9300eep.h"
#include "ar9300.h"
#include "ar9300reg.h"

#include "Ar9300EepromStructSet.h"
#include "ar9300EepromPrint.h"
#include "ParameterSelect.h"
#include "NartVersion.h"
#include "Card.h"
#include "instance.h"

// 
// this is the hal pointer, 
// returned by ath_hal_attach
// used as the first argument by most (all?) HAL routines
//
extern struct ath_hal *AH;

extern void  ar9300SwapEeprom(ar9300_eeprom_t *eep);


static int CompressPairs(char *input, int size, char *output, int max)
{
    return -1;
}


static int CompressBlock(char *input, char *source, int size, char *output, int max)
{
    int first,last,offset;
    int length;
    int nout;
    int block;
    int it;

    nout=0;
    first=0;
    offset=0;
    for(first=0; first<size; first=last)
    {
        //
        // find beginning of non-zero block
        //
        for( ; first<size; first++)
        {
            if(input[first]!=0)
            {
                break;
            }
        }
        //
        // did we reach the end?
        //
        if(first>=size)
        {
            break;
        }
        //
        // find end of non-zero block
        //
        for(last=first+1; last<size; last++)
        {
            //
            // possible zero block
            // must stay zero for at least 2 bytes, since that is how int our header is
            //
            if(input[last]==0)
            {
                if((last==size-1) || (last==size-2 && input[last+1]==0) || input[last+1]==0 && input[last+2]==0)
                {
                    break;
                }
            }
        }
        //
        // we have a non-zero block of data that goes from first to last-1;
        //
        // first check if the offset from the last block is too big.
        // if so we have to write some 0 length blocks to get the offset up
        //
        for(block=offset+255; block<first; block+=255)
        {
            if(nout+2>=max)
            {
                ErrorPrint(EepromNoRoom);
                return -1;
            }
            //
            // write the header
            //
            output[nout]=block-offset;
            offset=block;
            nout++;
            output[nout]=0;             
            nout++;
        }
        //
        // then we can only write 255 bytes in a block, so we may
        // have to write more than one block
        //
        for(block=first; block<last; block+=255)
        {
            if(last-block>=255)
            {
                length=255;
            }
            else
            {
                length=last-block;
            }
            if(nout+2+length>=max)
            {
                ErrorPrint(EepromNoRoom);
                return -1;
            }
            //
            // write the header
            //
            output[nout]=block-offset;
            nout++;
            output[nout]=length;             
            nout++;
            //
            // and now the data
            //
            for(it=0; it<length; it++)
            {
                output[nout]=source[block+it];
                nout++;
            }
            offset=block+length;        // added +length, th 090924
        }
    }

    return nout;
}


//
// code[3], reference [6], length[11], major[4], minor[8],  
//
static int CompressionHeaderPack(unsigned char *best, int code, int reference, int length, int major, int minor)
{
    best[0]=((code&0x0007)<<5)|((reference&0x001f));        // code[2:0], reference[4:0]
    best[1]=((reference&0x0020)<<2)|((length&0x07f0)>>4);   // reference[5], length[10:4]
    best[2]=((length&0x000f)<<4)|(major&0x000f);            // length[3:0], major[3:0]
    best[3]=(minor&0xff);                                   // minor[7:0]
    return 4;
}


static int CalibrationDataWrite(int address, unsigned char *buffer, int many)
{
    int it;

	switch(ar9300_calibration_data_get(AH))
	{
		case calibration_data_flash:
#ifdef MDK_AP
			{
				int fd;
				// UserPrint("Do block write \n");
				// for(it=0;it<many;it++)
				//    printf("a = %x, data = %x \n",(address-it), buffer[it]);
				if((fd = open("/dev/caldata", O_RDWR)) < 0) {
					perror("Could not open flash\n");
					return 1 ;
				}

				for(it=0; it<many; it++) {
					lseek(fd, address-it, SEEK_SET);
					if (write(fd, &buffer[it], 1) < 1) {
						perror("\nwrite\n");
						return -1;
					}
				}
				close(fd);
				}
			return 0;
#endif
		case calibration_data_eeprom:
			for(it=0; it<many; it++)
			{
				Ar9300EepromWrite(address-it,&buffer[it],1);
			}
			return 0;
		case calibration_data_otp:
			for(it=0; it<many; it++)
			{
				Ar9300OtpWrite(address-it,&buffer[it],1,1);
			}
			return 0;
	}
	return -1;
}


static int CalibrationDataWriteAndVerify(int address, unsigned char *buffer, int many)
{
    int it;
    unsigned char check;

	switch(ar9300_calibration_data_get(AH))
	{
		case calibration_data_flash:
#ifdef MDK_AP
			{
				int fd;
				// UserPrint("Do block write \n");
				// for(it=0;it<many;it++)
				//    printf("a = %x, data = %x \n",(address-it), buffer[it]);
				if((fd = open("/dev/caldata", O_RDWR)) < 0) {
					perror("Could not open flash\n");
					return 1 ;
				}

				for(it=0; it<many; it++) {
					lseek(fd, address-it, SEEK_SET);
					if (write(fd, &buffer[it], 1) < 1) {
						perror("\nwrite\n");
						return -1;
					}
				}
				close(fd);
				}
			return 0;
#endif
		case calibration_data_eeprom:
			for(it=0; it<many; it++)
			{
				Ar9300EepromWrite(address-it,&buffer[it],1);
				Ar9300EepromRead(address-it,&check,1);

				if(check!=buffer[it])
				{
					ErrorPrint(EepromVerify,address-it,buffer[it],check);
					return -1;
				}
			}
			break;
		case calibration_data_otp:
			for(it=0; it<many; it++)
			{
				Ar9300OtpWrite(address-it,&buffer[it],1,1);
				Ar9300OtpRead(address-it,&check,1,1);

				if(check!=buffer[it])
				{
					ErrorPrint(EepromVerify,address-it,buffer[it],check);
					return -1;
				}
			}
			break;
			return -1;
    }
    return 0;
}


static void CheckCompression(char *mptr, int msize, char *dptr, int it,
    int *balgorithm, int*breference, int *bsize, char *best, int max)
{
    int ib;
    int osize;
    int overhead;
    int first, last;
    int length;
    char difference[MOUTPUT], output[MOUTPUT];

    for(ib=0; ib<msize; ib++)
    {
        difference[ib]=mptr[ib]^dptr[ib];
    }
    //
    // compress with LZMA
    //
    osize= -1;
//            osize=CompressLzma(difference, dsize, output, MOUTPUT);
    if(osize>=0)
    {
        if(osize< *bsize)
        {
            *balgorithm=_compress_lzma;
            *breference=it;
            *bsize=osize;
            memcpy(best,output,osize);
        }
    }
    //
    // look for (offset,value) pairs
    //
    osize=CompressPairs(dptr, msize, output, MOUTPUT);
    if(osize>=0)
    {
        if(osize< *bsize)
        {
            *balgorithm=_compress_pairs;
            *breference=it;
            *bsize=osize;
            memcpy(best,output,osize);
       }
    }
    // 
    // look for block
    //
    osize=CompressBlock(difference, mptr, msize, output, MOUTPUT);
    if(osize>=0)
    {
        if(osize+4< *bsize)
        {
            *balgorithm=_compress_block;
            *breference=it;
            *bsize=osize;
            memcpy(best,output,osize);
       }
    }
}

static int LocalUncompressBlock(int mcount, u_int8_t *mptr, int mdataSize, u_int8_t *block, int size, void (*print)(char *format, ...))
{
    int it;
    int spot;
    int offset;
    int length;
	int ncopy;

    spot=0;
	ncopy=0;
    for(it=0; it<size; it+=(length+2))
    {
        offset=block[it];
        offset&=0xff;
        spot+=offset;
        length=block[it+1];
        length&=0xff;
        if(length>0 && spot>=0 && spot+length<mdataSize)
        {
            memcpy(&mptr[spot],&block[it+2],length);
			if(print!=0)
			{
				(*print)("|ecb|%d|%d|%d|%d|", mcount, ncopy, spot, length);
			}
            spot+=length;
			ncopy++;
        }
        else if(length>0)
        {
            return -1;
        }
    }
    return 0;
}




#define MCHECK 20

/*
 * Read the configuration data from the eeprom and reports what it finds.
 * Does not restore the data.
 * This function closely parallels Ar9300EepromRestore() in the hal.
 *
 * Returns -1 on error. 
 * Returns address of next memory location on success.
 */
int Ar9300EepromReportAddress(void (*print)(char *format, ...), int all, int cptr)
{
    u_int8_t word[MOUTPUT]; 
    u_int8_t *dptr;
    int code;
    int reference,length,major,minor;
    int osize;
    int it;
    u_int16_t checksum, mchecksum;
	ar9300_eeprom_t mptr[MCHECK], *xptr;
	char *ptr;
	int mcount;
	int mdataSize;
	int restored=0;

	mcount=0;
	//
	// this gets the first template as a default starting point
	//
	mdataSize=ar9300_eeprom_struct_size();
    //
	// return header 
	//
	if(print!=0)
	{
		(*print)("|ec|block|address|code|template|length|major|minor|csm|csc|status|");
		(*print)("|ecb|block|portion|offset|length|");
	}
    //
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
                        memcpy(&mptr[mcount],(u_int8_t *)(word+compression_header_length),length);
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
                        dptr=(u_int8_t *)ar9300_eeprom_struct_default_find_by_id(reference);
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
                            (void)LocalUncompressBlock(mcount,(u_int8_t *)&mptr[mcount],mdataSize,(u_int8_t *)(word+compression_header_length),length,print);
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
                        (void)LocalUncompressBlock(mcount,(u_int8_t *)&mptr[mcount],mdataSize,(u_int8_t *)(word+compression_header_length),length,print);
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
 
	if((mcount<=0)&&(ar9300_calibration_data_get(AH)!=calibration_data_flash)) // flash cal data is always in uncompressed format
	{
		cptr= -1;
	}
	else
	{
		if(print!=0)
		{
			(*print)("|ec|%d|%x|%d|%d|%d|%d|%d|%x|%x|free|", mcount, cptr, 0, 0, 0, 0, 0, 0, 0);

			//
			// include the current state
			//
			if(mcount<MCHECK)
			{
				xptr=Ar9300EepromStructGet();
				memcpy(&mptr[mcount],xptr,mdataSize);
				mcount++;
			}

			Ar9300EepromDifferenceAnalyze(print,mptr,mcount,all);
		}
	}
    return cptr;
}


/*
 * Read the configuration data from the eeprom and reports what it finds.
 * Does not restore the data.
 * This function closely parallels Ar9300EepromRestore() in the hal.
 *
 * Returns -1 on error. 
 * Returns address of next memory location on success.
 */
AR9300DLLSPEC int Ar9300EepromReport(void (*print)(char *format, ...), int all)
{
    int cptr;
	int nptr;
	//
 	// first find the data with print turned off
 	//
 	nptr= -1;
 	cptr=Ar9300CalibrationDataAddressGet();
 	if(cptr>0)
 	{
 		nptr=Ar9300EepromReportAddress(0, 0, cptr);
 	}	
	if(nptr<0)
	{
		// #### want to look at highest eeprom address as well as at BaseAddress
		cptr=ar9300_eeprom_base_address(AH);
		nptr=Ar9300EepromReportAddress(0, 0, cptr);
		if(nptr<0)
		{
			if(cptr!=0x3ff)
			{
				cptr=0x3ff;
				nptr=Ar9300EepromReportAddress(0, 0, cptr);
			}
		}
	}
 	//
 	// then go and do the job that was requested
 	//
 	nptr=Ar9300EepromReportAddress(print, all, cptr);
 
	return nptr;
}


#define MTEMPLATE 100
static unsigned int TemplateAllowedMany=1;
static unsigned int TemplateAllowed[MTEMPLATE]={ar9300_eeprom_template_generic,0,0};

AR9300DLLSPEC int Ar9300EepromTemplateAllowed(unsigned int *value, unsigned int many)
{
	unsigned int it;

	if(many>MTEMPLATE)
	{
		many=MTEMPLATE;
	}
	for(it=0; it<many; it++)
	{
		TemplateAllowed[it]=value[it];
	}
	TemplateAllowedMany=many;

	return 0;
}

//
// do not compress the data
// this flag is overridden if the data will not fit uncompressed in the available memory
//
static int Compress=1;

AR9300DLLSPEC int Ar9300EepromCompress(unsigned int value)
{
	Compress=value;
	return 0;
}

//
// overwrite the calibration data
// this floag is overridden if the memory type does not allow rewriting (otp)
//
static int Overwrite=1;

AR9300DLLSPEC int Ar9300EepromOverwrite(unsigned int value)
{
	Overwrite=value;
	return 0;
}

static int SaveAddress=0x3ff;

AR9300DLLSPEC int Ar9300EepromSaveAddressSet(int address)
{
	SaveAddress=address;
	return 0;
}

static int SaveMemory=calibration_data_none;

AR9300DLLSPEC int Ar9300EepromSaveMemorySet(int memory)
{
	SaveMemory=memory;
	return 0;
}


int Ar9300EepromUsed(void)
{
	int calibration;

	calibration=Ar9300EepromReport(0,0);
	if(calibration<0)
	{
		calibration=ar9300_eeprom_base_address(AH);
	}
	return calibration;
}


/*
 * the lower limit on configuration data
 */
static const int LowLimitDefault=0x040;

static int LowLimit()
{
	int limit;

	limit=Ar9300ConfigSpaceUsed();
	if(limit<LowLimitDefault)
	{
		limit=LowLimitDefault;
	}
	return limit;
}

//
// save any information required to support calibration
//
static int Ar9300EepromSave()
{
    ar9300_eeprom_t *mptr;        // pointer to data
    int msize;
    unsigned char header[compression_header_length], cheader[compression_header_length], before[compression_header_length];
    char best[MOUTPUT];
    int osize;
    int it;
    ar9300_eeprom_t *dptr;
    int dsize;
    int first;
    int last;
    int ib;
    int bsize;
    int breference;
    int balgorithm;
    int offset[MVALUE],value[MVALUE];
    int overhead;
    ar9300_eeprom_t xptr;
    int nextaddress;
    int major, minor;
    int ccode,creference,cmajor,cminor,clength;
    unsigned short checksum;
    unsigned char csum[2];
    int error;
    int written;
	int ntry;
    unsigned char ones[compression_header_length]={0xff,0xff,0xff,0xff};
    int status;
	int calmem;
	int restored;
	int trysave;
    //
    // get a pointer to the current data structure
    //
    mptr=Ar9300EepromStructGet();
    msize=ar9300_eeprom_struct_size();

	calmem=SaveMemory;
	if(calmem==calibration_data_none)
	{
		calmem=ar9300_calibration_data_get(AH);
	}
	if(calmem==calibration_data_none)
	{
	    if(ar9300_eeprom_size(AH)>0)
		{
			calmem=calibration_data_eeprom;
		}
		else if(ar9300_calibration_data_read_flash(AH, 0x1000, header, 1)==AH_TRUE)
		{
			calmem=calibration_data_flash;
		}
		else
		{
			calmem=calibration_data_otp;
		}
	}

	 // Calculate Checksum
 	checksum=ar9300_compression_checksum((unsigned char*)mptr,msize);
	printf("checksum is %x\n",checksum);
#if AH_BYTE_ORDER == AH_BIG_ENDIAN
//	ar9300_eeprom_t *eeprom_ptr;
//	eeprom_ptr=Ar9300EepromStructGet();
    ar9300_eeprom_template_swap();
    ar9300_swap_eeprom(mptr);
#endif
   
    // For AP with flash calibration data storage save structure uncompressed.
#ifdef MDK_AP
    if(calmem==calibration_data_flash)
    {
        int fd;
        int offset;
        // for(it=0;it<many;it++)
        //    printf("a = %x, data = %x \n",(address-it), buffer[it]);
        if((fd = open("/dev/caldata", O_RDWR)) < 0) {
            perror("Could not open flash\n");
            status = -1 ;
            goto return_status;
        }

        // First 0x1000 are reserved for ethernet mac address and other config writes.
        offset = instance*AR9300_EEPROM_SIZE+FLASH_BASE_CALDATA_OFFSET;  // Need for boards with more than one radio
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
    
    //
    // try all of our compression schemes, 
    // starting with the assumption that uncompressed is best
    //
    balgorithm=_compress_none;
    breference= -1;
    bsize=msize;
	nextaddress= -1;
    //
	// restore the existing eeprom structure to a temporary buffer
	// because we need it later for several reasons. Returns 0
	// if a default template was loaded because there was no data in memory.
	// Return -1 on error. Otherwise returns the next available address.
	//
#ifdef USE_AQUILA_HAL
 	trysave=ar9300CalibrationDataGet(0);	
 	ar9300CalibrationDataSet(0,calmem);	
#else
	trysave=AH9300(AH)->calibration_data_try;	
	AH9300(AH)->calibration_data_try = calmem;	
#endif
	restored=ar9300_eeprom_restore_internal(AH, &xptr, msize);
	if(restored==0)
	{
		nextaddress= -1;
	}
	else
	{
		nextaddress=restored;
	}
	ar9300_calibration_data_set(AH,calmem);
#ifdef USE_AQUILA_HAL
	ar9300CalibrationDataSet(0,trysave);
#else
	AH9300(AH)->calibration_data_try = trysave;	
#endif
	//
	// do we over write or append
	// if overwrite, move the starting address to the top of the memory
	//
	if((Overwrite && ar9300_eeprom_volatile(AH)) || nextaddress<0)
	{ 
		if(SaveAddress==0)
		{
			nextaddress=ar9300_eeprom_base_address(AH);
		}
		else
		{
			nextaddress=SaveAddress;
		}
	}
    //
	// if compression is requested or if the uncompressed data won't fit
	//
	if(Compress || nextaddress-msize-compression_header_length-compression_checksum_length<=ar9300_eeprom_low_limit(AH))
	{
		//
		// try difference with each standard default structure
		//
		if(TemplateAllowedMany<=0)
		{
			for(it=0; it<ar9300_eeprom_struct_default_many(); it++)
			{
				dptr=ar9300_eeprom_struct_default(it);
				dsize=msize;
				if(dptr!=0 && dsize<MOUTPUT && dsize==msize)
				{
					CheckCompression((char *)mptr,msize,(char *)dptr,dptr->template_version,&balgorithm,&breference,&bsize,best,MOUTPUT);
				}
			}
		}
		else
		{
			for(it=0; it<TemplateAllowedMany; it++)
			{
				dptr=ar9300_eeprom_struct_default_find_by_id(TemplateAllowed[it]);
				dsize=msize;
				if(dptr!=0 && dsize<MOUTPUT && dsize==msize)
				{
					CheckCompression((char *)mptr,msize,(char *)dptr,dptr->template_version,&balgorithm,&breference,&bsize,best,MOUTPUT);
				}
			}
		}
		//
		// also try difference with existing eeprom if it was restored from memory
		//
		if((!(Overwrite && ar9300_eeprom_volatile(AH))) && restored>0 && xptr.eeprom_version==mptr->eeprom_version)
		{
			CheckCompression((char *)mptr,msize,(char *)&xptr,reference_current,&balgorithm,&breference,&bsize,best,MOUTPUT);
		}
	}
    //
    // if the uncompressed size is the smallest, might as well go with it
    //
    if(bsize>=msize)
    {
        balgorithm=_compress_none;
        breference= reference_current;
        bsize=msize;
        memcpy(best,mptr,msize);        // the uncompressed data
		//
		// if using OTP we have to find the first free spot.
		// if using eeprom, we can overwrite
		//
    }
    //
    // Now we know the best method and we have the data, so write it
    //
    ErrorPrint(EepromAlgorithm,balgorithm,breference,bsize,nextaddress);
    if(bsize>0 || restored==0)
    {
        written=0;
        osize=bsize;
		ntry=0;
        while(written==0 &&
			nextaddress-osize-compression_header_length-compression_checksum_length>ar9300_eeprom_low_limit(AH))
        {
			ntry++;
            if(ntry>3)
            {
                ErrorPrint(EepromTooMany);
                status = -2;
                goto return_status;
            }
            //
            // read the spot where we're going to write the header
			// this is so we can check on error if the chip managed to write anything at all
            //
            (void)ar9300_calibration_data_read_array(AH,nextaddress,(unsigned char*)before,compression_header_length);
            //
            // create and write header
            //
            major=NartVersionMajor();
            minor=NartVersionMinor();
            CompressionHeaderPack(header, balgorithm, breference, bsize, major, minor);
            error=CalibrationDataWriteAndVerify(nextaddress,header,compression_header_length);
            if(error==0)
			{
				//
				// write data
				//
				nextaddress-=compression_header_length;
				error=CalibrationDataWriteAndVerify(nextaddress,(unsigned char*)best,osize);
				nextaddress-=osize;
				if(error==0)
				{
					//
					// create and write checksum
					//
					checksum=ar9300_compression_checksum((unsigned char*)best,bsize);
                    csum[0]=checksum&0xff;
                    csum[1]=(checksum>>8)&0xff;
					error=CalibrationDataWriteAndVerify(nextaddress,csum,compression_checksum_length);
					nextaddress-=compression_checksum_length;
					if(error==0)
					{
						//
						// everything written successfully, so we're done
						//
						written=1;
					}
					else
					{
						 //
						 // and try again
						 //
					}
				}
				else
				{
					// 
					// write bad checksum
					//
					checksum=ar9300_compression_checksum((unsigned char*)best,bsize);
					checksum^=0xffff;
                    csum[0]=checksum&0xff;
                    csum[1]=(checksum>>8)&0xff;
					error=CalibrationDataWriteAndVerify(nextaddress,csum,compression_checksum_length);
					nextaddress-=compression_checksum_length;
				}
			}
			else
            {
                //
                // read it back. see if we didn't write anything
                //
                (void)ar9300_calibration_data_read_array(AH,nextaddress,cheader,compression_header_length);
				for(it=0; it<compression_header_length; it++)
				{
					if(cheader[it]==before[it])
					{
						break;
					}
				}
				if(it>=compression_header_length)
				{
					ErrorPrint(EepromWrite);
                    status = -1;
					goto return_status;
				}
                //
                // try to obliterate the header by writing all ones
                //
                for(it=0; it<compression_header_length; it++)
                {
                    ones[it]=0xff;
                }
                CalibrationDataWrite(nextaddress,ones,compression_header_length);
                //
                // read it back. if it is clearly bad, go on.
                // if it makes sense, then reject the chip
                //
                (void)ar9300_calibration_data_read_array(AH,nextaddress,(unsigned char*)cheader,compression_header_length);
                ar9300_compression_header_unpack((unsigned char*)cheader,&ccode,&creference,&clength,&cmajor,&cminor);
                if(clength<1024)
                {
                    ErrorPrint(EepromFatal);
                    status = -3;
                    goto return_status;
                }
                nextaddress-=compression_header_length;
            }
		}
        if(!written)
        {
            ErrorPrint(EepromWontFit,nextaddress,nextaddress-bsize,LowLimit());
            status = -1;
            goto return_status;
        }
        //
        // if OTP we want to skip this
        //
		if(ar9300_eeprom_volatile(AH))
		{
			CalibrationDataWrite(nextaddress,ones,compression_header_length);
		}
    }

    status = bsize;

return_status:

#if AH_BYTE_ORDER == AH_BIG_ENDIAN
    ar9300_eeprom_template_swap();
    ar9300_swap_eeprom(mptr);
#endif

    return status;    
}

extern AR9300DLLSPEC int Ar9300EepromTemplatePreference(int preference)
{
	ar9300_eeprom_template_preference(preference);
	return 0;
}

extern AR9300DLLSPEC int Ar9300EepromSize(void)
{
	return ar9300_eeprom_size(AH);
}

extern AR9300DLLSPEC int Ar9300CalibrationDataAddressSet(int size)
{
	ar9300_calibration_data_address_set(AH, size);
	return 0;
}

extern AR9300DLLSPEC int Ar9300CalibrationDataAddressGet(void)
{
	return ar9300_calibration_data_address_get(AH);
}

extern AR9300DLLSPEC int Ar9300CalibrationDataSet(int source)
{
	ar9300_calibration_data_set(AH, source);
	return 0;
}

extern AR9300DLLSPEC int Ar9300CalibrationDataGet(void)
{
	return ar9300_calibration_data_get(AH);
}

extern AR9300DLLSPEC int Ar9300EepromTemplateInstall(int preference)
{
	return ar9300_eeprom_template_install(AH, preference);
}

extern AR9300DLLSPEC int Ar9300ConfigurationSave(void)
{
	return Ar9300EepromSave();
}

extern AR9300DLLSPEC int Ar9300ConfigurationRestore(void)
{
	return ar9300_eeprom_restore(AH);
}
