#ifndef _COMPRESS_H_
#define _COMPRESS_H_

//
// the maximum number of states we write in the memory
//
#define MSTATE 100

#define MOUTPUT 2048+256

#define MDEFAULT 15

#define MVALUE 100

enum CompressAlgorithm
{
    _compress_none=0,
    _compress_lzma,
    _compress_pairs,
    _compress_block,
    _Compress4,
    _Compress5,
    _Compress6,
    _Compress7,
};


#define REFERENCE_CURRENT			0
#define COMPRESSION_HEADER_LENGTH	4
#define COMPRESSION_CHECKSUM_LENGTH 2

extern int CompressPairs(char *input, int size, char *output, int max);

extern int CompressBlock(char *input, char *source, int size, char *output, int max);

extern int CompressionHeaderPack(unsigned char *best, int code, int reference, int length, int major, int minor);

extern void CheckCompression(char *mptr, int msize, char *dptr, int it,
							int *balgorithm, int*breference, int *bsize, char *best, int max);

extern int LocalUncompressBlock(int mcount, unsigned char *mptr, int mdataSize, 
								unsigned char *block, int size, void (*print)(char *format, ...));

extern unsigned short CompressionChecksum (unsigned char *data, int dsize);

//
// code[4], reference [4], minor[8], major[4], length[12]
//
extern int CompressionHeaderUnpack(unsigned char *best, int *code, int *reference, int *length, int *major, int *minor);

extern int UncompressBlock (unsigned char *mptr, int mdata_size, unsigned char *block, int size);



#endif //_COMPRESS_H_