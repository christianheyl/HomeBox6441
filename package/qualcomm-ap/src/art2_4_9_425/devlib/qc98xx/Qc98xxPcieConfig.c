#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

//#include "AquilaNewmaMapping.h"

#include "wlantype.h"

#include "Qc98xxDevice.h"
#include "Qc98xxPcieConfig.h"
#include "ConfigurationStatus.h"
//#include "ah_osdep.h"
//#include "opt_ah.h"

//#include "ah.h"
//#include "ah_internal.h"
//#include "ah_devid.h"
#include "art_utf_common.h"
#include "otpstream_id.h"
#include "qc98xx_eeprom.h"
#define CONFIG_PCIE_SUPPORT
#include "pcihw_api.h"
#include "dsetid.h"

#include "Device.h"
#include "ChipIdentify.h"
#include "dk_cmds.h"
#include "tlvCmd_if.h"
#include "Qc98xxEepromSave.h"
#include "Qc98xxEepromStructSet.h"

#include "UserPrint.h"
#include "ErrorPrint.h"
#include "EepromError.h"

static PCIE_CONFIG_STRUCT pcieAddressValueData[MAX_pcieAddressValueData];
static int num_pcieAddressValueData;

// from the offset, get the byteth (0, 1, 2 or 3).
// 0th byte's flags are in pcieAddressValueData[it].flags[7:0] 
// 1th byte's flags are in pcieAddressValueData[it].flags[15:8] 
// 2th byte's flags are in pcieAddressValueData[it].flags[23:16] 
// 3th byte's flags are in pcieAddressValueData[it].flags[31:24] 
// start position of the flags of each byte in pcieAddressValueData[it].flags = (offset & 0x3) * 8
#define DWORD_ALIGNED(offset)		((offset) & (~0x3))
#define BYTE_INDEX(offset)          ((offset) & 3)
#define WORD_INDEX(offset)			(((offset) >> 1) & 1)
#define FLAG_POSITION(offset)       (((offset)&0x3)<<3)
#define HAS_VALUE(i,offset)			(pcieAddressValueData[i].flags & (PCIE_HASVALUE_FLAG << FLAG_POSITION(offset)))
#define HAS_CHANGED(i,offset)		(pcieAddressValueData[i].flags & (PCIE_CHANGED_FLAG << FLAG_POSITION(offset)))
#define FROM_OTP(i,offset)          (pcieAddressValueData[i].flags & (PCIE_FROMOTP_FLAG << FLAG_POSITION(offset)))
#define FROM_DEFAULT(i,offset)      (pcieAddressValueData[i].flags & (PCIE_DEFAULT_FLAG << FLAG_POSITION(offset)))
#define FROM_OTP_OR_DEFAULT(i,offset) (FROM_OTP(i,offset) || FROM_DEFAULT(i,offset))
#define SET_HASVALUE_FLAG(i,offset)	(pcieAddressValueData[i].flags | (PCIE_HASVALUE_FLAG << FLAG_POSITION(offset)))
#define SET_CHANGED_FLAG(i,offset)	(pcieAddressValueData[i].flags | (PCIE_CHANGED_FLAG << FLAG_POSITION(offset)))
#define RESET_CHANGED_FLAG(i,offset) (pcieAddressValueData[i].flags & (~(PCIE_CHANGED_FLAG << FLAG_POSITION(offset))))
#define RESET_DEFAULT_FLAG(i,offset) (pcieAddressValueData[i].flags & (~(PCIE_DEFAULT_FLAG << FLAG_POSITION(offset))))
#define SET_FROMOTP_FLAG(i,offset)	(pcieAddressValueData[i].flags | (PCIE_FROMOTP_FLAG << FLAG_POSITION(offset)))
#define SET_DEFAULT_FLAG(i,offset)	(pcieAddressValueData[i].flags | (PCIE_DEFAULT_FLAG << FLAG_POSITION(offset)))
#define SET_OP_FLAG(i,offset,op)    (pcieAddressValueData[i].flags | ((op & PCIE_OP_FLAG) << FLAG_POSITION(offset)))
#define GET_OP_FLAG(i,offset)       ((pcieAddressValueData[i].flags >> FLAG_POSITION(offset)) & PCIE_OP_FLAG) 
#define CHANGE_FLAG(i,offset)		((FROM_OTP(i,offset) || FROM_DEFAULT(i,offset)) ? SET_CHANGED_FLAG(i,offset) : SET_HASVALUE_FLAG(i,offset))
#define OFFSET_AND_SIZE_OK(offset,size) ((BYTE_INDEX(offset) + size) <= 4)

A_INT32 Qc98xxDeviceIDSet(unsigned int deviceID);
A_INT32 Qc98xxVendorSet(unsigned int vendorID);
A_INT32 Qc98xxPcieAddressValueDataAdd( unsigned int address, unsigned int data, unsigned int op, int size);
A_INT32 Qc98xxPcieAddressValueDataOtpSet( unsigned int offset, unsigned int data, unsigned int op, int size);

#define MPCIE 100
#define PCIE_OTP_BASE (32)

#ifdef AP_BUILD
int swapDone=0;
#endif

#if defined(WIN32) || defined(WIN64)
#pragma pack (push, 1)
#endif

struct {
    unsigned char patchid;
    struct patch_s p[14];
} __ATTRIB_PACK otppatch1 =
{
        OTPSTREAM_ID_PATCH1,
        {
            /*{0x60008, 0x02800000},
            {0x60040, 0xffc25001},
            {0x60044, 0x00000000},
            {0x60050, 0x01087005},
            {0x60074, 0x05048701},
            {0x6007c, 0x00035c11},
            {0x60080, 0x10110000},
            {0x6070c, 0x273f3f01},
            {PATCH_MODE_SWITCH, PATCH_MODE_NAND},
            {0x5000,  0x00000040},
            {PATCH_MODE_SWITCH, PATCH_MODE_WRITE},
            {0x6c640, 0x18212ede},
            {0x6c648, 0x0003580c},*/
            {0x60000, 0x003c168c},
            {0x60008, 0x02800000},
            {0x60040, 0x07c25001},
            {0x60044, 0x00000000},
            {0x60050, 0x01067005},
            {0x60074, 0x05048dc1},
            {0x6007c, 0x00036c11},
            {0x60080, 0x10110000},
            {0x6070c, 0x273f3f01},
            {PATCH_MODE_SWITCH, PATCH_MODE_NAND},
            {0x05000, 0x00000040}, // enable SRIF 
            {PATCH_MODE_SWITCH, PATCH_MODE_WRITE},
            {0x6c640, 0x18253ede},
            {0x6c648, 0x001b580c},
        }
};

struct {
    unsigned char patchid;
    struct patch_s p[4];
} __ATTRIB_PACK otppatch2 =
{
        OTPSTREAM_ID_PATCH1,
        {
            {0x0040820c, 0x009412fc},
            {0x0000800c, 0x0000118f},
            {PATCH_MODE_SWITCH, PATCH_MODE_OR},
            {0x00009000, 0x00040000},
        }
};

struct {
    unsigned char patchid;
    struct patch_s p[4];
} __ATTRIB_PACK otppatch2_old =
{
        OTPSTREAM_ID_PATCH1,
        {
            {0x0040820c, 0x009412fc},
            {0x0000800c, 0x000011cf},
            {PATCH_MODE_SWITCH, PATCH_MODE_OR},
            {0x00009000, 0x00040000},
        }
};

//struct {
//    unsigned char patchid;
//    struct patch_s p[2];
//} __ATTRIB_PACK otppatch1_v2_old =
//{
//    OTPSTREAM_ID_PATCH1,
//    {
//        {0x00060040, 0x07c25001},
//        {0x0000800c, 0x0000518f},
//    }
//};

// The format of PCIE config OTP stream
// 1-byte streamID = OTPSTREAM_ID_PCIE_CONFIG
// 1-byte meta:  the high nibble = PCIE_CONFIG_OP_SET; the low nibble = size of the data	
// 2-byte offset: offset to the configuration space. 0 for vendorID, 2 for deviceID, 0x2c for subSystemVendorID, and 0x2e for subSystemID	
// Size-byte data
A_UINT8 PcieConfigPatch_v2[] =
{
    OTPSTREAM_ID_PCIE_CONFIG,
    ((PCIE_CONFIG_OP_SET << 4) |  4),
    0x00, 0x40,
#ifdef AP_BUILD
    0x07, 0xc2, 0x50, 0x01,
#else
    0xff, 0xc2, 0x50, 0x01,
#endif //AP_BUILD
};

struct {
    unsigned char patchid;
    struct patch_s p[5];
} __ATTRIB_PACK otppatch1_v2 =
{
    OTPSTREAM_ID_PATCH1,
    {
#ifdef AP_BUILD
        {PATCH_MODE_SWITCH, PATCH_MODE_OR},
        {0x0000800c, 0x00002000},
        {PATCH_MODE_SWITCH, PATCH_MODE_NAND},
        {0x0000800c, 0x00000200},
        {PATCH_MODE_SWITCH, PATCH_MODE_WRITE},
#else
        {PATCH_MODE_SWITCH, PATCH_MODE_OR},
        {0x0000800c, 0x00002040},
        {PATCH_MODE_SWITCH, PATCH_MODE_NAND},
        {0x0000800c, 0x00000200},
        {PATCH_MODE_SWITCH, PATCH_MODE_WRITE},
#endif //AP_BUILD
    }
};

struct patch_s PcieConfigDefault_v2[] =
{
    {0x60000, 0x003c168c},
    {0x60008, 0x02800000},
    //{0x60040, 0x5bc35001},
    {0x60044, 0x00000000},
    {0x60050, 0x01067005},
    {0x60074, 0x05048dc1},
    {0x6007c, 0x00036c11},
    {0x60080, 0x10110000},
    {0x6070c, 0x273f3f01},     
    //{0x6c640, 0x18253ede},
    //{0x6c648, 0x001b780c},
};

#if defined(WIN32) || defined(WIN64)
#pragma pack(pop)
#endif

static A_BOOL otpPatch1Written = FALSE;
static A_BOOL otpPatch2Present = FALSE;
static A_BOOL otpPatchPcieV2Written = FALSE;

static int PcieTop;

//
// Patch Functions
//
A_BOOL comparePatch1OtpStream(A_UCHAR *buffer, A_UINT32 nbytes)
{
    A_UCHAR *patch;
    A_UINT32 i, size;

    if (Qc98xxIsVersion1())
    {
        patch = (A_UCHAR*)&otppatch1;
        size = sizeof(otppatch1);
    }
    else
    {
        patch = (A_UCHAR*)&otppatch1_v2;
        size = sizeof(otppatch1_v2);
    }
    
    if(size == nbytes)
    {
        for(i=0;i<nbytes;i++)
        {
            if ( patch[i] != buffer[i] ) 
			{
                return FALSE;
            }
        }
        UserPrint("Found match Patch1 PCIE\n");
		return TRUE;
    }
    return FALSE;
}

A_BOOL comparePatch2OtpStream(A_UCHAR *buffer, A_UINT32 nbytes, A_BOOL oldPatch2)
{
    A_UCHAR *patch;
	A_UINT32 patchSize;
    A_UINT32 i;

    if (Qc98xxIsVersion1())
    {
	    if (oldPatch2)
	    {
		    patch = (A_UCHAR*)&otppatch2_old;
		    patchSize = sizeof(otppatch2_old);
	    }
	    else
	    {
		    patch = (A_UCHAR*)&otppatch2;
		    patchSize = sizeof(otppatch2);
	    }
    }
    else //v2
    {
        //patch = (A_UCHAR*)&otppatch2_v2;
		//patchSize = sizeof(otppatch2_v2);
        return TRUE;
    }
    
	if(patchSize == nbytes)
    {
        for(i=0;i<nbytes;i++)
        {
            if ( patch[i] != buffer[i] ) 
			{
                return FALSE;
            }
        }
		UserPrint("Found match Patch2 PCIE\n");
		return TRUE;
    }
    return FALSE;
}

A_BOOL comparePatch2OtpStreamNoLength(A_UCHAR *buffer, A_BOOL oldPatch2)
{
    A_UCHAR *patch;
    int i, size;

    size = 0;
    if (Qc98xxIsVersion1())
    {
	    patch = oldPatch2 ? (A_UCHAR*)&otppatch2_old : (A_UCHAR*)&otppatch2;
        size = oldPatch2 ? sizeof(otppatch2_old) : sizeof(otppatch2);
    }
    //else //v2
    //{
    //    patch = (A_UCHAR*)&otppatch1_v2_old;
    //    size = sizeof(otppatch1_v2_old);
    //}
	for(i = 0; i < size; i++)
    {
        if ( patch[i] != buffer[i] ) 
        {
            return FALSE;
        }
    }
    return (size > 0);
}

A_BOOL comparePcieConfigPatchV2OtpStream(A_UCHAR *buffer, A_UINT32 nbytes)
{
    if (nbytes != sizeof(PcieConfigPatch_v2))
    {
        return FALSE;
    }
    return (memcmp(PcieConfigPatch_v2, buffer, nbytes) == 0);
}

void Qc98xxWritePatch1()
{
    A_UINT8 *patch;
    A_UINT32 size;
    
    if (Qc98xxIsVersion1())
    {
        patch = (A_UINT8 *)&otppatch1;
        size = sizeof(otppatch1);
    }
    else
    {
        patch = (A_UINT8 *)&otppatch1_v2;
        size = sizeof(otppatch1_v2);
    }

    UserPrint("Write Patch 1 PCIE info to OTP\n");
    if (art_otpReset(OTPSTREAM_WRITE_APP) != A_OK)
	{
		UserPrint("Qc98xxWritePatch1: error in art_otpReset for write\n");
		return;
	}
    if (A_OK != art_otpWrite(patch, size))
    {
        UserPrint("Error: art_otpWrite in OTP patch 1\n");
        return;
    }
    otpPatch1Written = TRUE;
}

void Qc98xxWritePatch2()
{
    A_UINT8 *patch;
    A_UINT32 size;
    
    if (Qc98xxIsVersion1())
    {
        patch = (A_UINT8 *)&otppatch2;
        size = sizeof(otppatch2);
    }
    else
    {
        //patch = (A_UINT8 *)&otppatch2_v2;
        //size = sizeof(otppatch2_v2);
        return;
    }
    UserPrint("Writing Patch 2 to OTP\n");

    if (art_otpReset(OTPSTREAM_WRITE_APP) != A_OK)
	{
		UserPrint("Qc98xxWritePatch2: error in art_otpReset for write\n");
		return;
	}
    if (A_OK != art_otpWrite(patch, size))
    {
        UserPrint("Error: art_otpWrite in OTP patch 2\n");
    }
}

void Qc98xxWritePcieConfigPatch_v2()
{
    A_UINT8 *patch;
    A_UINT32 size;
    
    patch = PcieConfigPatch_v2;
    size = sizeof(PcieConfigPatch_v2);
    UserPrint("Writing PcieConfigPatch_v2 to OTP\n");

    if (art_otpReset(OTPSTREAM_WRITE_APP) != A_OK)
	{
		UserPrint("Qc98xxWritePcieConfigPatch_v2: error in art_otpReset for write\n");
		return;
	}
    if (A_OK != art_otpWrite(patch, size))
    {
        UserPrint("Error: art_otpWrite in OTP PcieConfigPatch_v2\n");
    }
}

A_BOOL WritePatchesAndFixedBytes_v1()
{
    A_UCHAR buffer[OTPSTREAM_MAXSZ_APP];
    QC98XX_OTP_STREAM_HEADER *pStreamHdr;
    A_UINT32 nbytes, nStreams;
    A_BOOL status;
    A_BOOL patch1Otp, patch2Otp;

	// To reset the OTP read pointer to the beginning of OTP
    if (art_otpReset(OTPSTREAM_READ_APP) != A_OK)
    {
        UserPrint("Qc98xxCheckPatch1InOtp: error in art_otpReset\n");
        return(FALSE);
    }

	nbytes = 0;
	nStreams = 0;
    status = TRUE;
    patch1Otp = FALSE;
    patch2Otp = FALSE;
    memset(buffer, 0, sizeof(buffer));
 
    // read stream by stream and process
    while (((status = art_otpRead(buffer, &nbytes)) == A_OK) && (nbytes > 0) && (patch1Otp == FALSE || patch2Otp == FALSE))
    {     
        nStreams++;

		// check stream header
        pStreamHdr = (QC98XX_OTP_STREAM_HEADER *)buffer;

        if (pStreamHdr->applID == OTPSTREAM_ID_PATCH1)
        {
            if ( patch1Otp == FALSE )
			{
                patch1Otp = comparePatch1OtpStream(buffer,nbytes);
				if (patch1Otp) continue;
			}
            if ( patch2Otp == FALSE )
			{
				// check the new patch2
                patch2Otp = comparePatch2OtpStream(buffer,nbytes,0);
				if (patch2Otp) continue;
			}
            if ( patch2Otp == FALSE )
			{
				// check the old patch2
                patch2Otp = comparePatch2OtpStream(buffer,nbytes,1);
			}
        }
    }// end of while(art_otpRead)
    
    // v1 only
    if ((status == A_OK) && (nStreams == 0))
    {
		// Write the fixed bytes if the OTP is empty
		Qc98xxWriteFixedBytesIfOtpEmpty(1);
	}

    if (patch1Otp == FALSE) 
    {
        Qc98xxWritePatch1();
    }

    // Only write patch2 after patch1 and all pcie config
    if (patch2Otp) 
    {
        otpPatch2Present = TRUE;
    }
    return TRUE;
}


A_BOOL WritePatchesAndFixedBytes_v2()
{
    A_UCHAR buffer[OTPSTREAM_MAXSZ_APP];
    QC98XX_OTP_STREAM_HEADER *pStreamHdr;
    A_UINT32 nbytes, nStreams;
    A_BOOL status;
    A_BOOL patch1Otp;
    A_BOOL pciePatch;

	// To reset the OTP read pointer to the beginning of OTP
    if (art_otpReset(OTPSTREAM_READ_APP) != A_OK)
    {
        UserPrint("Qc98xxCheckPatch1InOtp: error in art_otpReset\n");
        return(FALSE);
    }

	nbytes = 0;
	nStreams = 0;
    status = TRUE;
    patch1Otp = FALSE;
    pciePatch = FALSE;
    memset(buffer, 0, sizeof(buffer));
 
    // read stream by stream and process
    while (((status = art_otpRead(buffer, &nbytes)) == A_OK) && (nbytes > 0) && (patch1Otp == FALSE))
    {     
        nStreams++;

		// check stream header
        pStreamHdr = (QC98XX_OTP_STREAM_HEADER *)buffer;

        if (pStreamHdr->applID == OTPSTREAM_ID_PATCH1)
        {
            if ( patch1Otp == FALSE )
			{
                patch1Otp = comparePatch1OtpStream(buffer,nbytes);
			}
        }
        else if (pStreamHdr->applID == OTPSTREAM_ID_PCIE_CONFIG)
        {
            if (pciePatch == FALSE)
            {
                pciePatch = comparePcieConfigPatchV2OtpStream(buffer,nbytes);
            }
        }
        if (patch1Otp && pciePatch)
        {
            break;
        }

    }// end of while(art_otpRead)
    
    if (patch1Otp == FALSE) 
    {
        Qc98xxWritePatch1();
    }
    if (pciePatch == FALSE)
    {
        Qc98xxWritePcieConfigPatch_v2();
    }
    return TRUE;
}


A_BOOL WritePatchesAndFixedBytes()
{
	// For BE AP build otppatch1.patch_s has to be swapped.
#ifdef AP_BUILD
    if(swapDone==0)
    {
        A_UINT32 byte4, swappedByte4;
        int i;
        swapDone=1;
        for(i=0;i<sizeof(otppatch1.p)/sizeof(struct patch_s);i++)
        {
           byte4=otppatch1.p[i].address;
           swappedByte4=SWAP32(byte4);
           otppatch1.p[i].address=swappedByte4;
           byte4=otppatch1.p[i].data;
           swappedByte4=SWAP32(byte4);
           otppatch1.p[i].data=swappedByte4;
        }
        for(i=0;i<sizeof(otppatch2.p)/sizeof(struct patch_s);i++)
        {
           byte4=otppatch2.p[i].address;
           swappedByte4=SWAP32(byte4);
           otppatch2.p[i].address=swappedByte4;
           byte4=otppatch2.p[i].data;
           swappedByte4=SWAP32(byte4);
           otppatch2.p[i].data=swappedByte4;
        }
        for(i=0;i<sizeof(otppatch2_old.p)/sizeof(struct patch_s);i++)
        {
           byte4=otppatch2_old.p[i].address;
           swappedByte4=SWAP32(byte4);
           otppatch2_old.p[i].address=swappedByte4;
           byte4=otppatch2_old.p[i].data;
           swappedByte4=SWAP32(byte4);
           otppatch2_old.p[i].data=swappedByte4;
        }
        for(i=0;i<sizeof(otppatch1_v2.p)/sizeof(struct patch_s);i++)
        {
           byte4=otppatch1_v2.p[i].address;
           swappedByte4=SWAP32(byte4);
           otppatch1_v2.p[i].address=swappedByte4;
           byte4=otppatch1_v2.p[i].data;
           swappedByte4=SWAP32(byte4);
           otppatch1_v2.p[i].data=swappedByte4;
        }
        /*for(i=0;i<sizeof(otppatch2_v2.p)/sizeof(struct patch_s);i++)
        {
           byte4=otppatch2_v2.p[i].address;
           swappedByte4=SWAP32(byte4);
           otppatch2_v2.p[i].address=swappedByte4;
           byte4=otppatch2_v2.p[i].data;
           swappedByte4=SWAP32(byte4);
           otppatch2_v2.p[i].data=swappedByte4;
        }*/
    }
#endif

    otpPatch1Written = FALSE;
    otpPatch2Present = FALSE;
    otpPatchPcieV2Written = FALSE;

    if (Qc98xxIsVersion1())
    {
        return WritePatchesAndFixedBytes_v1();
    }
    //v2
    return WritePatchesAndFixedBytes_v2();
}

//
//	PICE Functions
//

void UpdateFlags (int i, unsigned int offset, unsigned int op, int size)
{
    int j;

    for (j = 0; j < size; j++)
    {
        pcieAddressValueData[i].flags = CHANGE_FLAG(i,offset+j) | SET_OP_FLAG(i,offset+j,op);
    }
}

void SetFromOtpFlag (int i, unsigned int offset, int size)
{
    int j;

    for (j = 0; j < size; j++)
    {
        pcieAddressValueData[i].flags = SET_FROMOTP_FLAG(i,offset+j);
        pcieAddressValueData[i].flags = RESET_DEFAULT_FLAG(i,offset+j);
        pcieAddressValueData[i].flags = RESET_CHANGED_FLAG(i,offset+j);
    }

}

void SetDefaultFlag (int i, unsigned int offset, int size)
{
    int j;

    for (j = 0; j < size; j++)
    {
        pcieAddressValueData[i].flags = SET_DEFAULT_FLAG(i,offset+j);
        pcieAddressValueData[i].flags = RESET_CHANGED_FLAG(i,offset+j);
    }

}

void ResetFlags (int i, unsigned int offset, int size)
{
    int j;

    for (j = 0; j < size; j++)
    {
        pcieAddressValueData[i].flags &= (~(0xff << FLAG_POSITION(offset+j)));
    }
}

A_BOOL IsSameValue(int i, unsigned int offset, unsigned int data, int size)
{
    if (!OFFSET_AND_SIZE_OK(offset, size))
    {
        return FALSE;
    }
    return (memcmp(&pcieAddressValueData[i].data.bData[BYTE_INDEX(offset)], &data, size) == 0);
}

A_BOOL Qc98xxPcieOtpStreamParse_v2(A_UCHAR *pBuffer, A_UINT32 nbytes)
{
	A_UINT8 meta, size, op;
	A_UINT16 offset;
    A_UINT32 i, data;

	if (pBuffer[0] != OTPSTREAM_ID_PCIE_CONFIG) 
	{
        //UserPrint("Qc98xxPcieOtpStreamParse_v2: this is not a PCIe Config stream\n");
        return FALSE; 
    }
        
#ifdef _DEBUG
    UserPrint("Qc98xxPcieOtpStreamParse_v2: PCIe OTP stream");
    for (i=0; i< nbytes; i++)
    {
        if ((i % 16) == 0) UserPrint("\n");
        UserPrint("%02x  ", pBuffer[i]);
    }
    UserPrint("\n");
#endif

	// The format of PCIE config OTP stream
	// 1-byte streamID = OTPSTREAM_ID_PCIE_CONFIG
	// 1-byte meta:  the high nibble = PCIE_CONFIG_OP_SET; the low nibble = size of the data	
	// 2-byte offset: offset to the configuration space. 0 for vendorID, 2 for deviceID, 0x2c for subSystemVendorID, and 0x2e for subSystemID	
	// Size-byte data
    i = 1;
    while (i < nbytes)
    {
        meta = pBuffer[i];
        i+=1; /* skip meta-byte */

        size = meta & 0xf;
        op = meta >> 4;

        if ((size == 3) || (size > 4) || ((i+2+size) > nbytes))
        {
            UserPrint("Qc98xxPcieOtpStreamParse_v2: the stream is corrupted; ignore the rest of the stream, read next\n");
            return FALSE;
        }

        offset = (pBuffer[i]<<8) | (pBuffer[i+1]);
        i+=2; /* skip offset */
        if (size == 1)
        {
            data = pBuffer[i];
        }
        else if (size == 2)
        {
            data = pBuffer[i] << 8 | pBuffer[i+1];
        }
        else //if (size == 4)
        {
            data = pBuffer[i] << 24 | pBuffer[i+1] << 16 |  pBuffer[i+2] << 8 |  pBuffer[i+3];
        }
        Qc98xxPcieAddressValueDataOtpSet(offset, data, op, size);
        i +=size;
    }
    return TRUE;
}

A_BOOL Qc98xxPcieOtpStreamParse(A_UCHAR *pBuffer, A_UINT32 nbytes)
{
	A_UINT8 size, op;
	A_UINT32 offset;
    A_UINT32 i, data;

#if 0
    UserPrint("Qc98xxPcieOtpStreamParse: PCIe OTP stream");
    for (i=0; i< nbytes; i++)
    {
        if ((i % 16) == 0) UserPrint("\n");
        UserPrint("%02x  ", pBuffer[i]);
    }
    UserPrint("\n");
#endif
	if (pBuffer[0] != OTPSTREAM_ID_PATCH1) 
	{
        UserPrint("Qc98xxPcieOtpStreamParse: this is not a PCIe Config stream\n");
        return FALSE; 
    }
    if ((nbytes - 1) % 8)
	{
        UserPrint("Qc98xxPcieOtpStreamParse: this stream's length is incorrect, ignore the stream\n");
        return FALSE; 
    }

    i = 1;
    // We now use OTPSTREAM_ID_PATCH1 otpstreams for PCIE config instead of OTPSTREAM_ID_PCIE_CONFIG otpstreams
    // There is no "op" in the stream anymore, but just leave it there 
    // so the other functions are not needed to be changed.
    while (i < nbytes)
    {
        offset = pBuffer[i] | (pBuffer[i+1] << 8) | (pBuffer[i+2] << 16) | (pBuffer[i+3] << 24);
        data = pBuffer[i+4] | (pBuffer[i+5] << 8) | (pBuffer[i+6] << 16) | (pBuffer[i+7] << 24);
        offset -= PCIE_BASE_ADDRESS;
        op = PCIE_CONFIG_OP_SET;
        size = 4;
        Qc98xxPcieAddressValueDataOtpSet(offset, data, op, size);
        i += 8;
    }
    return TRUE;
}


A_INT32 Qc98xxPcieAddressValueDataInitOtp(void)
{
    A_UCHAR buffer[EFUSE_MAX_NUM_BYTES];
    A_STATUS status;
    A_UINT32 nbytes;

	// To reset the OTP read pointer to the beginning of OTP
    if (art_otpReset(OTPSTREAM_READ_APP) != A_OK)
    {
        UserPrint("Qc98xxPcieAddressValueDataInitOtp: error in art_otpReset\n");
        return ERR_RETURN;
    }

    nbytes = 0;
	status = A_OK;
	memset(buffer, 0, sizeof(buffer));
 
    // read stream by stream and process
    while (((status = art_otpRead(buffer, &nbytes)) == A_OK) && (nbytes > 0))
    {
        if (Qc98xxIsVersion1())
        {
            Qc98xxPcieOtpStreamParse(buffer, nbytes); 
        }
        else
        {
            Qc98xxPcieOtpStreamParse_v2(buffer, nbytes); 
        }
    }
    return (status == A_OK ? VALUE_OK : ERR_RETURN);
}

A_INT32 Qc98xxPcieMany(void)
{
	return num_pcieAddressValueData;
}

int Qc98xxPcieDefault()
{
    int i, size;
    A_UINT32 offset;
    struct patch_s *patch;

    if (Qc98xxIsVersion1())
    {
        // Read patch1 to get the default PCIE config
        patch = otppatch1.p;
        size = sizeof(otppatch1.p)/sizeof(struct patch_s);
    }
    else //v2
    {
        // Read PcieConfigDefault_v2 to get the default PCIE config
        patch = PcieConfigDefault_v2;
        size = sizeof(PcieConfigDefault_v2)/sizeof(struct patch_s);
    }

    for (i = 0; i < size; ++i)
    {
        if (patch[i].address >= PCIE_BASE_ADDRESS && patch[i].address < PCIE_BASE_ADDRESS+0xfff)
        {
            offset = patch[i].address - PCIE_BASE_ADDRESS;
            Qc98xxPcieAddressValueDataSet(offset, patch[i].data, 4);
            SetDefaultFlag (i, offset, 4);
        }
    }
	return num_pcieAddressValueData;
}

A_INT32 Qc98xxPcieAddressValueDataInit(void)
{
	int error;

	num_pcieAddressValueData = 0;
    error=0;
	memset(pcieAddressValueData, 0, sizeof(pcieAddressValueData));
    Qc98xxPcieDefault();

	error = Qc98xxPcieAddressValueDataInitOtp();
    return (error == 0 ? num_pcieAddressValueData : error);
}


A_INT32 Qc98xxPcieAddressValueDataAdd( unsigned int offset, unsigned int data, unsigned int op, int size)
{
    pcieAddressValueData[num_pcieAddressValueData].offset = DWORD_ALIGNED(offset);
    memcpy(&pcieAddressValueData[num_pcieAddressValueData].data.bData[BYTE_INDEX(offset)], &data, size);
    UpdateFlags(num_pcieAddressValueData, offset, op, size);
    num_pcieAddressValueData++;
    return VALUE_NEW;
}


A_INT32 Qc98xxPcieAddressValueDataSetCommon( unsigned int offset, unsigned int data, unsigned int op, int size, int *idx)
{
	int i;

#ifdef AP_BUILD
    {
        A_UINT32 swappedByte4;
        swappedByte4 = SWAP32(data);
        data = swappedByte4;
    }
#endif
	
    for (i=0; i< num_pcieAddressValueData; i++) 
	{
		if (pcieAddressValueData[i].offset == DWORD_ALIGNED(offset)) 
		{
            *idx = i;
            if (HAS_VALUE(i,offset) && IsSameValue(i, offset, data, size))
            {
                return VALUE_SAME;
            }
            memcpy(&pcieAddressValueData[i].data.bData[BYTE_INDEX(offset)], &data, size);
            UpdateFlags(i, offset, op, size);

			return VALUE_NEW;
        }
    }
	if (num_pcieAddressValueData>=MAX_pcieAddressValueData)
    {
        *idx = MAX_pcieAddressValueData;
		return ERR_MAX_REACHED;	
    }
    Qc98xxPcieAddressValueDataAdd(offset, data, op, size);
    *idx = num_pcieAddressValueData-1;
    return VALUE_NEW;
}

A_INT32 Qc98xxPcieAddressValueDataSet( unsigned int offset, unsigned int data, int size)
{
	int i;
    return (Qc98xxPcieAddressValueDataSetCommon(offset, data, PCIE_CONFIG_OP_SET, size, &i));
}

A_INT32 Qc98xxPcieAddressValueDataOtpSet( unsigned int offset, unsigned int data, unsigned int op, int size)
{
    int i;
    int status;

    status = Qc98xxPcieAddressValueDataSetCommon(offset, data, op, size, &i);

    if (status >= VALUE_OK)
    {
        SetFromOtpFlag (i, offset, size);
    }
    return status;
}

A_INT32 Qc98xxPcieAddressValueDataGet( unsigned int offset, unsigned int *data)
{
	int i, byteth;
	int status = ERR_NOT_FOUND;	
    unsigned char *pValue;
    unsigned int value;

	for (i=0; i< num_pcieAddressValueData; i++) 
	{
		if (pcieAddressValueData[i].offset == DWORD_ALIGNED(offset)) 
		{
            value = pcieAddressValueData[i].data.dwData;
            pValue = (unsigned char *)&value;
#ifdef AP_BUILD
            {
                A_UINT32 swappedByte4;
                swappedByte4 = SWAP32(value);
                value = swappedByte4;
            }
#endif

			byteth = offset & 3;
			if (byteth == 0)
			{
				*data = value;
				//*data = pcieAddressValueData[i].data.dwData;
			}
			else if (byteth & 1)
			{
                *data = pValue[byteth];
				//*data = pcieAddressValueData[i].data.bData[byteth];
			}
			else
			{
                 *data=pValue[2];
				//*data = pcieAddressValueData[i].data.wData[1];
			}
			status = VALUE_OK;
			break;
		}
	}
    return status;
}
A_INT32 Qc98xxPcieAddressValueDataOfNumGet( int num, unsigned int *address, unsigned int *data)
{
	if (num <0 || num>=num_pcieAddressValueData)
		return ERR_RETURN;
	*address = pcieAddressValueData[num].offset;
	*data = pcieAddressValueData[num].data.dwData;
#ifdef AP_BUILD
    {
        A_UINT32 swappedByte4;
        swappedByte4 = SWAP32(*data);
        *data = swappedByte4;
    }
#endif
	return VALUE_OK;
}

A_INT32 Qc98xxPcieAddressValueDataRemove( unsigned int address, int size)
{
	int i, iFound=ERR_NOT_FOUND;
	
	for (i=0; i< num_pcieAddressValueData; i++) 
	{
		if (pcieAddressValueData[i].offset == DWORD_ALIGNED(address))
		{
			if (HAS_VALUE(i, address))
			{
				iFound = i;
                if (size < 4)
                {
                    ResetFlags(i, address, size);
                    return iFound;
                }
			}
			break;
		}
	}
	if (iFound!=ERR_NOT_FOUND) 
	{
		for (i=iFound; i<num_pcieAddressValueData-1; i++) 
        {
			pcieAddressValueData[i].offset = pcieAddressValueData[i+1].offset;
			pcieAddressValueData[i].data.dwData = pcieAddressValueData[i+1].data.dwData;
			pcieAddressValueData[i].flags = pcieAddressValueData[i+1].flags;
		}
		pcieAddressValueData[i].offset = 0;
		pcieAddressValueData[i].data.dwData = 0;
		pcieAddressValueData[i].flags = 0;
		num_pcieAddressValueData--;
	}
    return iFound;
}


A_INT32 Qc98xxDeviceIDSet(unsigned int deviceID) 
{  
	int i;

    return (Qc98xxPcieAddressValueDataSetCommon(PCIE_DEVICE_ID_OFFSET, deviceID, PCIE_CONFIG_OP_SET, 2, &i));
}

A_INT32 Qc98xxDeviceIDGet(unsigned int *deviceID) 
{  
	int it;
    unsigned short data;

	for(it=0; it<num_pcieAddressValueData; it++)
	{
		if(pcieAddressValueData[it].offset == DWORD_ALIGNED(PCIE_DEVICE_ID_OFFSET))
		{
			if (HAS_VALUE(it, PCIE_DEVICE_ID_OFFSET))
			{
				data = pcieAddressValueData[it].data.wData[WORD_INDEX(PCIE_DEVICE_ID_OFFSET)];
#ifdef AP_BUILD
    {
        A_UINT16 swappedByte2;
        swappedByte2 = SWAP16(data);
        data = swappedByte2;
    }
#endif
        *deviceID = (unsigned short) data;
	    return VALUE_OK;
			}
			break;
		}
	}
	*deviceID = 0xDEAD;
	return ERR_RETURN;
}

A_INT32 Qc98xxVendorSet(unsigned int vendorID)
{   
	int i;

    return (Qc98xxPcieAddressValueDataSetCommon(PCIE_VENDOR_ID_OFFSET, vendorID, PCIE_CONFIG_OP_SET, 2, &i));
}

A_INT32 Qc98xxVendorGet(unsigned int *vendorID)
{   
	int it;
    unsigned short data;

	for(it=0; it<num_pcieAddressValueData; it++)
	{
		if(pcieAddressValueData[it].offset == DWORD_ALIGNED(PCIE_VENDOR_ID_OFFSET))
		{
			if (HAS_VALUE(it, PCIE_VENDOR_ID_OFFSET))
			{
				data = pcieAddressValueData[it].data.wData[WORD_INDEX(PCIE_VENDOR_ID_OFFSET)];
#ifdef AP_BUILD
    {
        A_UINT16 swappedByte2;
        swappedByte2 = SWAP16(data);
        data = swappedByte2;
    }
#endif
            
        *vendorID = (unsigned int)data;

				return VALUE_OK;
			}
			break;
		}
	}
	*vendorID = 0xDEAD;
	return ERR_RETURN;
}

A_INT32 Qc98xxSSIDSet(unsigned int SSID) 
{  
	int i;

    return (Qc98xxPcieAddressValueDataSetCommon(PCIE_SUBSYSTEM_ID_OFFSET, SSID, PCIE_CONFIG_OP_SET, 2, &i));
}

A_INT32 Qc98xxSSIDGet(unsigned int *SSID) 
{  
	int it;
    unsigned short data;

	for(it=0; it<num_pcieAddressValueData; it++)
	{
		if(pcieAddressValueData[it].offset == DWORD_ALIGNED(PCIE_SUBSYSTEM_ID_OFFSET))
		{
			if (HAS_VALUE(it, PCIE_SUBSYSTEM_ID_OFFSET))
			{
				data = pcieAddressValueData[it].data.wData[WORD_INDEX(PCIE_SUBSYSTEM_ID_OFFSET)];
#ifdef AP_BUILD
    {
        A_UINT16 swappedByte2;
        swappedByte2 = SWAP16(data);
        data = swappedByte2;
    }
#endif
        *SSID = (unsigned int)data;
				return VALUE_OK;
			}
			break;
		}
	}
	*SSID = 0xDEAD;
	return ERR_RETURN;
}

A_INT32 Qc98xxSubVendorSet(unsigned int subVendorID)
{   
	int i;

    return (Qc98xxPcieAddressValueDataSetCommon(PCIE_SUBVENDOR_ID_OFFSET, subVendorID, PCIE_CONFIG_OP_SET, 2, &i));
}

A_INT32 Qc98xxSubVendorGet(unsigned int *subVendorID)
{   
	int it;
    unsigned short data;

	for(it=0; it<num_pcieAddressValueData; it++)
	{
		if(pcieAddressValueData[it].offset == DWORD_ALIGNED(PCIE_SUBVENDOR_ID_OFFSET ))
		{
			if (HAS_VALUE(it, PCIE_SUBVENDOR_ID_OFFSET))
			{
				data = pcieAddressValueData[it].data.wData[WORD_INDEX(PCIE_SUBVENDOR_ID_OFFSET)];
#ifdef AP_BUILD
    {
        A_UINT32 swappedByte2;
        swappedByte2 = SWAP16(data);
        data = swappedByte2;
    }
#endif
        *subVendorID = (unsigned short)data;
				return VALUE_OK;
			}
			break;
		}
	}
	*subVendorID = 0xDEAD;
	return ERR_RETURN;
}

A_BOOL Qc98xxGetSizeOffsetDataDone_v2(int i, unsigned int *byteth, unsigned int *size, A_UINT16 *offset, unsigned int *op, A_UINT8 **pData)
{
    int j, curOffset;

    *size = 0;
    *offset = (A_UINT16)(pcieAddressValueData[i].offset + *byteth);
    *op = GET_OP_FLAG(i, *offset);
    *pData =  &pcieAddressValueData[i].data.bData[*byteth];
    for (j = *byteth; j < 4; j++)
    {
        curOffset = pcieAddressValueData[i].offset+j;
        if ((FROM_OTP_OR_DEFAULT(i, curOffset) && HAS_CHANGED(i, curOffset)) || (!FROM_OTP_OR_DEFAULT(i, curOffset) && HAS_VALUE(i,curOffset )))
        {
            (*size)++;
        }
        else
        {
            j++;
            break;
        }
    }
    // If size is 3 then break it into 2 segments
    // if the offset is 0, break to 2 and 1; if the offset is 1, break to 1 and 2
    if (*size == 3)
    {
        if (BYTE_INDEX(*offset) == 0)
        {
            (*size)--;
            j--;
        }
        else
        {
            *size -= 2;
            j -= 2;
        }
    }
    *byteth = j;
    return (j == 4 ? TRUE : FALSE);
}

A_INT32 Qc98xxConfigSpaceCommitOtp_v2() 
{
	int it, i, j;
	int nmany;
	unsigned int invalidate=0xffff;
	unsigned int readit=0x4;
    A_UINT8 stream[EFUSE_MAX_NUM_BYTES];
    A_UINT8 *pCur;
    int byteth, size, op;
    A_UINT16 offset;
    A_UINT8 *pData;
    A_UINT32 streamLen;
 
    if (num_pcieAddressValueData == 0)
    {
        return VALUE_OK;
    }

    stream[0] = OTPSTREAM_ID_PCIE_CONFIG;
    pCur = &stream[1];

    nmany=num_pcieAddressValueData;
    pData = NULL;

    for(it=0; it<nmany; it++)
    {
        byteth = 0;
        do
        {
            size = 0;
            Qc98xxGetSizeOffsetDataDone_v2(it, &byteth, &size, &offset, &op, &pData);
            if (size == 0)
            {
                continue;
            }

            *pCur++ = (op << 4) | size;                 //1-byte meta
            *pCur++ = (offset >> 8) & 0xff;             // 2-byte offset: MSB first then LSB
            *pCur++ = offset & 0xff;
            for (i = 0, j = size-1; i < size; i++, j--) // size-byte data
            {
                pCur[i] = pData[j];
            }
            pCur += size;
        } while (byteth < 4);
    }
   
    streamLen = pCur - stream;

    if (streamLen > 1)
    {
        // To reset the OTP write pointer to the beginning of OTP
        if (art_otpReset(OTPSTREAM_WRITE_APP) != A_OK)
        {
            UserPrint("Qc98xxConfigSpaceCommitOtp: error in art_otpReset\n");
            return ERR_RETURN;
        }

        // write to OTP
        if (A_OK != art_otpWrite(stream, streamLen))
        {
            UserPrint("Qc98xxConfigSpaceCommitOtp: Error in art_otpWrite \n");
            return ERR_RETURN;
        }
        UserPrint("%d bytes of PCIe config were written to OTP\n", streamLen);
    }
	return (streamLen <= 1 ? 0 : streamLen);
}

A_BOOL Qc98xxGetSizeOffsetDataDone(int it, A_UINT32 *address, A_UINT8 *data)
{
    A_UINT32 value;
    A_UINT32 offset;
    A_BOOL offsetRead = FALSE;
    int i;

    offset = pcieAddressValueData[it].offset;
    *address = offset + PCIE_BASE_ADDRESS;

    for (i = 0; i < 4; i++)
    {
        if ((FROM_OTP_OR_DEFAULT(it, offset+i) && HAS_CHANGED(it, offset+i)) ||
            (!FROM_OTP_OR_DEFAULT(it, offset+i) && HAS_VALUE(it,offset+i)))
        {
            break;
        }
    }
    if (i == 4)
    {
        //no change, return
        return FALSE;
    }

    for (i = 0; i < 4; i++)
    {
        if ((FROM_OTP_OR_DEFAULT(it, offset+i) && HAS_CHANGED(it, offset+i)) ||
            (!FROM_OTP_OR_DEFAULT(it, offset+i) && HAS_VALUE(it,offset+i)))
        {
            data[i] = pcieAddressValueData[it].data.bData[i];
            continue;
        }
        if (offsetRead == FALSE)
        {
            Qc98xxReadPciConfigSpace(offset, &value);
            offsetRead = TRUE;
        }
        data[i] = ((A_UINT8 *)(&value))[i];
    }
    return TRUE;
}

A_INT32 Qc98xxConfigSpaceCommitOtpPatch_v1() 
{
	int it;
    A_UINT8 stream[EFUSE_MAX_NUM_BYTES];
    A_UINT8 *pCur, aByte;
    A_UINT32 address, data;
    A_UINT32 streamLen, efuseLen;
    A_UINT32 patch2Offset;
    A_BOOL patch1Otp = FALSE;
    A_BOOL patch2Otp = FALSE;

	if (WritePatchesAndFixedBytes() == FALSE)
    {
        return ERR_RETURN;
    }

    // ---- For v1 -----
    //The patch1 and patch2 were designed to REPLACE PCIe Config Space initialization.
    //patch2 actually patches the ROM code to SKIP ROM's PCIe configuration function
    //As a result, we cannot use the normal PCIE_CONFIG initialization.  
    //The original patches were split into two pieces in case we ever needed to make further changes to Config Space.  
    //The way to make further changes is to write 0xff (replacing 0xf9) to patch2.
    //[NB: Need to use efuse_write() API for this.]  That causes us to SKIP patch2.  
    //Then we can add another PATCH1 otpstream (but not a PCIE_CONFIG otpstream!) which performs additional Config Space writes.  
    //Finally, rewrite patch2.

    patch2Offset = 0;
    if (otpPatch2Present)
    {
        //First search for patch2 and get its offset
        efuseLen = EFUSE_MAX_NUM_BYTES-EFUSE_HEAD_RESERVED_BYTES-EFUSE_TAIL_RESERVED_BYTES;
        if (Qc98xxOtpRead(EFUSE_HEAD_RESERVED_BYTES, stream, efuseLen, 1) != 0)
        {
            UserPrint("Qc98xxConfigSpaceCommitOtp: Cannot read OTP\n");
            return ERR_RETURN;
        }

        // Now search for patch2
        for (it = 0; it < (int)efuseLen; ++it)
        {
            if (stream[it] == OTPSTREAM_ID_PATCH1)
            {
                if (comparePatch2OtpStreamNoLength(&stream[it], 0))
                {
                    // patch2 found at offset it+EFUSE_HEAD_RESERVED_BYTES
                    patch2Offset = it+EFUSE_HEAD_RESERVED_BYTES;
                    break;
                }
            }
        }
		if (patch2Offset == 0)
		{
			// Now search for old patch2
			for (it = 0; it < (int)efuseLen; ++it)
			{
				if (stream[it] == OTPSTREAM_ID_PATCH1)
				{
					if (comparePatch2OtpStreamNoLength(&stream[it], 1))
					{
						// old patch2 found at offset it+EFUSE_HEAD_RESERVED_BYTES
						patch2Offset = it+EFUSE_HEAD_RESERVED_BYTES;
						
						//Corrupt the old patch2
						aByte = OTPSTREAM_ID_INVALID1;
						Qc98xxOtpWrite(patch2Offset, &aByte, 1, 1);
						patch2Offset = 0;
						break;
					}
				}
			}
		}
#ifdef _DEBUG
		if (patch2Offset == 0)
		{
			UserPrint("Qc98xxConfigSpaceCommitOtp - old otpPatch2 has been invalid!!!\n");
		}
#endif //_DEBUG
	}
    memset(stream, 0, sizeof(stream));
    stream[0] = OTPSTREAM_ID_PATCH1;
    pCur = &stream[1];

    for(it=0; it<num_pcieAddressValueData; it++)
    {
        if (Qc98xxGetSizeOffsetDataDone(it, &address, (A_UINT8 *)&data) == FALSE)
        {
            continue;
        }

        printf("address=0x%x, data=0x%x\n",address,data);
#ifdef AP_BUILD
        {
            A_UINT32 swappedByte4;
            swappedByte4 = SWAP32(address);
            address = swappedByte4;
        }
#endif
        memcpy(pCur, &address, 4);
        memcpy(pCur+4, &data, 4);
        pCur += 8;
    }
   
    streamLen = pCur - (A_UINT8 *)stream;

	// streamLen should be > 1 for PCIE update; streamLen=1 means only OTPSTREAM_ID_PATCH1
	if (streamLen > 1)
	{
		// To reset the OTP write pointer to the beginning of OTP
		if (art_otpReset(OTPSTREAM_WRITE_APP) != A_OK)
		{
			UserPrint("Qc98xxConfigSpaceCommitOtp: error in art_otpReset\n");
			return ERR_RETURN;
		}

		// write to OTP
		if (A_OK != art_otpWrite(stream, streamLen))
		{
			UserPrint("Qc98xxConfigSpaceCommitOtp: Error in art_otpWrite \n");
			return ERR_RETURN;
		}
		UserPrint("%d bytes of PCIe config were written to OTP\n", streamLen);
	}
	// write patch2
	//Corrupt the patch2, and rewrite it
	if ((patch2Offset > 0) && (streamLen > 1 || otpPatch1Written))
	{
		aByte = OTPSTREAM_ID_INVALID1;
		Qc98xxOtpWrite(patch2Offset, &aByte, 1, 1);
	    Qc98xxWritePatch2();
	}
	// write patch2 if it's not there yet
    else if (patch2Offset == 0)
    {
	    Qc98xxWritePatch2();
    }
	return (streamLen <= 1 ? 0 : streamLen);
}

A_INT32 Qc98xxConfigSpaceCommitOtpPatch_v2() 
{
	//int it;
    //A_UINT8 stream[EFUSE_MAX_NUM_BYTES];
    //A_UINT8 aByte;
    //A_UINT32 efuseLen;
    //A_UINT32 oldPatchOffset;

	if (WritePatchesAndFixedBytes() == FALSE)
    {
        return ERR_RETURN;
    }
#if 0
    oldPatchOffset = 0;

    //Search for the old patch and invalidate it
    efuseLen = EFUSE_MAX_NUM_BYTES-EFUSE_HEAD_RESERVED_BYTES-EFUSE_TAIL_RESERVED_BYTES;
    if (Qc98xxOtpRead(EFUSE_HEAD_RESERVED_BYTES, stream, efuseLen, 1) != 0)
    {
        UserPrint("Qc98xxConfigSpaceCommitOtp_v2: Cannot read OTP\n");
        return ERR_RETURN;
    }

	for (it = 0; it < (int)efuseLen; ++it)
	{
		if (stream[it] == OTPSTREAM_ID_PATCH1)
		{
			if (comparePatch2OtpStreamNoLength(&stream[it], 1))
			{
				// old patch found at offset it+EFUSE_HEAD_RESERVED_BYTES
				oldPatchOffset = it+EFUSE_HEAD_RESERVED_BYTES;
				
				//Corrupt the old patch2
				aByte = OTPSTREAM_ID_INVALID1;
				Qc98xxOtpWrite(oldPatchOffset, &aByte, 1, 1);
				break;
			}
		}
	}
#ifdef _DEBUG
    if (oldPatchOffset)
	{
		UserPrint("Qc98xxConfigSpaceCommitOtp - old otpPatch2 has been invalid!!!\n");
	}
#endif //_DEBUG
#endif //0
    return Qc98xxConfigSpaceCommitOtp_v2();
}

A_INT32 Qc98xxConfigSpaceCommitOtp() 
{
    if (Qc98xxIsVersion1())
    {
        return Qc98xxConfigSpaceCommitOtpPatch_v1();
    }
    return Qc98xxConfigSpaceCommitOtpPatch_v2();
}

A_INT32 Qc98xxConfigSpaceCommit() 
{
	switch(Qc98xxCalibrationDataGet())
	{
		case DeviceCalibrationDataFile:
		case DeviceCalibrationDataFlash:
		case DeviceCalibrationDataOtp:
			return Qc98xxConfigSpaceCommitOtp();
        default:
            UserPrint("PCIe config can only be committed to OTP\n");
            break;
	}
	return ERR_RETURN;
}

A_INT32 Qc98xxConfigSpaceUsedOtp(void)
{
#if 0
	int address, addrMax, start_address;
	unsigned int eregister;

    addrMax = EFUSE_MAX_NUM_BYTES;	
    start_address = PCIE_OTP_BASE;

	for(address=start_address; address<addrMax; address+=8)
	{
		ReadIt(address,&eregister);
		//
		// are we done?
		//
		if(eregister==0 /*|| eregister==0xffffffff*/)			// DEBUG TEST 
		{
//			UserPrint("end of exisiting at %x out=%d\n",address,out);
			break;
		}
	}
	PcieTop=address;
#endif //0
	return PcieTop;
}


A_INT32 Qc98xxConfigSpaceUsed() 
{
	switch(Qc98xxCalibrationDataGet())
	{
		case DeviceCalibrationDataFile:
		case DeviceCalibrationDataFlash:
		case DeviceCalibrationDataOtp:
			return Qc98xxConfigSpaceUsedOtp();
        default:
            break;
	}
	return ERR_RETURN;
}
