
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Compress.h"

int CompressPairs(char *input, int size, char *output, int max)
{
    return -1;
}


int CompressBlock(char *input, char *source, int size, char *output, int max)
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
                //ErrorPrint(EepromNoRoom);
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
                //ErrorPrint(EepromNoRoom);
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
int CompressionHeaderPack(unsigned char *best, int code, int reference, int length, int major, int minor)
{
    best[0]=((code&0x0007)<<5)|((reference&0x001f));        // code[2:0], reference[4:0]
    best[1]=((reference&0x0020)<<2)|((length&0x07f0)>>4);   // reference[5], length[10:4]
    best[2]=((length&0x000f)<<4)|(major&0x000f);            // length[3:0], major[3:0]
    best[3]=(minor&0xff);                                   // minor[7:0]
    return 4;
}

void CheckCompression(char *mptr, int msize, char *dptr, int it,
    int *balgorithm, int*breference, int *bsize, char *best, int max)
{
    int ib;
    int osize;
    //int overhead;
    //int first, last;
    //int length;
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

int LocalUncompressBlock(int mcount, unsigned char *mptr, int mdataSize, unsigned char *block, int size, void (*print)(char *format, ...))
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

unsigned short CompressionChecksum (unsigned char *data, int dsize)
{
    int it;
    int checksum = 0;

    for (it = 0; it < dsize; it++) {
        checksum += data[it];
        checksum &= 0xffff;
    }

    return checksum;
}

int CompressionHeaderUnpack (unsigned char *best, int *code, int *reference, int *length, int *major, int *minor)
{
    unsigned long value[4];

    value[0] = best[0];
    value[1] = best[1];
    value[2] = best[2];
    value[3] = best[3];
    *code = ((value[0] >> 5) & 0x0007);
    *reference = (value[0] & 0x001f) | ((value[1] >> 2) & 0x0020);
    *length = ((value[1] << 4) & 0x07f0) | ((value[2] >> 4) & 0x000f);
    *major = (value[2] & 0x000f);
    *minor = (value[3] & 0x00ff);

    return 4;
}

int UncompressBlock (unsigned char *mptr, int mdata_size, unsigned char *block, int size)
{
    int it;
    int spot;
    int offset;
    int length;

    spot = 0;
    for (it = 0; it < size; it += (length + 2)) 
	{
        offset = block[it];
        offset &= 0xff;
        spot += offset;
        length = block[it + 1];
        length &= 0xff;
        if (length > 0 && spot >= 0 && spot + length <= mdata_size) 
		{
            memcpy(&mptr[spot], &block[it + 2], length);
            spot += length;
        } 
		else if (length > 0) 
		{
            return 0;
        }
    }
    return 1;
}

