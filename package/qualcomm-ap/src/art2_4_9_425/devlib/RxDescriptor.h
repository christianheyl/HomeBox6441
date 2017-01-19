
#ifdef _WINDOWS
#ifdef FIELDDLL
		#define DEVICEDLLSPEC __declspec(dllexport)
	#else
		#define DEVICEDLLSPEC __declspec(dllimport)
	#endif
#else
	#define DEVICEDLLSPEC
#endif

//
// These are the function pointers to the device dependent control functions
//
struct _RxDescriptorFunction
{
    unsigned int (*LinkPtr)(void *block);
    unsigned int (*BufPtr)(void *block);
    unsigned int (*BufLen)(void *block);
    unsigned char (*IntReq)(void *block);
    unsigned int (*RssiCombined)(void *block);
    unsigned int (*RssiAnt00)(void *block);
    unsigned int (*RssiAnt01)(void *block);
    unsigned int (*RssiAnt02)(void *block);
    unsigned int (*RssiAnt10)(void *block);
    unsigned int (*RssiAnt11)(void *block);
    unsigned int (*RssiAnt12)(void *block);
    unsigned int (*RxRate)(void *block);
    unsigned int (*DataLen)(void *block);
    unsigned char (*More)(void *block);
    unsigned int (*NumDelim)(void *block);
    unsigned int (*RcvTimestamp)(void *block);
    unsigned char (*Gi)(void *block);
    unsigned char (*H2040)(void *block);
    unsigned char (*Duplicate)(void *block);
    unsigned int (*RxAntenna)(void *block);
    double (*Evm0)(void *block);
    double (*Evm1)(void *block);
    double (*Evm2)(void *block);
    unsigned char (*Done)(void *block);
    unsigned char (*FrameRxOk)(void *block);
    unsigned char (*CrcError)(void *block);
    unsigned char (*DecryptCrcErr)(void *block);
    unsigned char (*PhyErr)(void *block);
    unsigned char (*MicError)(void *block);
    unsigned char (*PreDelimCrcErr)(void *block);
    unsigned char (*KeyIdxValid)(void *block);
    unsigned int (*KeyIdx)(void *block);
    unsigned char (*MoreAgg)(void *block);
    unsigned char (*FirstAgg)(void *block);
    unsigned char (*Aggregate)(void *block);
    unsigned char (*PostDelimCrcErr)(void *block);
    unsigned char (*DecryptBusyErr)(void *block);
    unsigned char (*KeyMiss)(void *block);
    int (*Setup)(void *block, 
		unsigned int link_ptr, unsigned int buf_ptr, unsigned int buf_len);
    int (*Reset)(void *block);
    int (*Size)();
    int (*Print)(void *block, char *buffer, int max);
    int (*SpectralScan)(void *block);
#ifdef UNUSED
    void (*Write)(void *block, unsigned int physical);
    void (*Read)(void *block, unsigned int physical);
#endif
};


extern DEVICEDLLSPEC int RxDescriptorPrint(void *block, char *buffer, int max);

extern DEVICEDLLSPEC unsigned int RxDescriptorLinkPtr(void *block);

extern DEVICEDLLSPEC unsigned int RxDescriptorBufPtr(void *block);

extern DEVICEDLLSPEC unsigned int RxDescriptorBufLen(void *block);

extern DEVICEDLLSPEC unsigned char RxDescriptorIntReq(void *block);

extern DEVICEDLLSPEC unsigned int RxDescriptorRssiCombined(void *block);

extern DEVICEDLLSPEC unsigned int RxDescriptorRssiAnt00(void *block);

extern DEVICEDLLSPEC unsigned int RxDescriptorRssiAnt01(void *block);

extern DEVICEDLLSPEC unsigned int RxDescriptorRssiAnt02(void *block);

extern DEVICEDLLSPEC unsigned int RxDescriptorRssiAnt10(void *block);

extern DEVICEDLLSPEC unsigned int RxDescriptorRssiAnt11(void *block);

extern DEVICEDLLSPEC unsigned int RxDescriptorRssiAnt12(void *block);

extern DEVICEDLLSPEC unsigned int RxDescriptorRxRate(void *block);

extern DEVICEDLLSPEC unsigned int RxDescriptorDataLen(void *block);

extern DEVICEDLLSPEC unsigned char RxDescriptorMore(void *block);

extern DEVICEDLLSPEC unsigned int RxDescriptorNumDelim(void *block);

extern DEVICEDLLSPEC unsigned int RxDescriptorRcvTimestamp(void *block);

extern DEVICEDLLSPEC unsigned char RxDescriptorGi(void *block);

extern DEVICEDLLSPEC unsigned char RxDescriptorH2040(void *block);

extern DEVICEDLLSPEC unsigned char RxDescriptorDuplicate(void *block);

extern DEVICEDLLSPEC unsigned int RxDescriptorRxAntenna(void *block);

extern DEVICEDLLSPEC double RxDescriptorEvm0(void *block);

extern DEVICEDLLSPEC double RxDescriptorEvm1(void *block);

extern DEVICEDLLSPEC double RxDescriptorEvm2(void *block);

extern DEVICEDLLSPEC unsigned char RxDescriptorDone(void *block);

extern DEVICEDLLSPEC unsigned char RxDescriptorFrameRxOk(void *block);

extern DEVICEDLLSPEC unsigned char RxDescriptorCrcError(void *block);

extern DEVICEDLLSPEC unsigned char RxDescriptorDecryptCrcErr(void *block);

extern DEVICEDLLSPEC unsigned char RxDescriptorPhyErr(void *block);

extern DEVICEDLLSPEC unsigned char RxDescriptorMicError(void *block);

extern DEVICEDLLSPEC unsigned char RxDescriptorPreDelimCrcErr(void *block);

extern DEVICEDLLSPEC unsigned char RxDescriptorKeyIdxValid(void *block);

extern DEVICEDLLSPEC unsigned int RxDescriptorKeyIdx(void *block);

extern DEVICEDLLSPEC unsigned char RxDescriptorMoreAgg(void *block);

extern DEVICEDLLSPEC unsigned char RxDescriptorFirstAgg(void *block);

extern DEVICEDLLSPEC unsigned char RxDescriptorAggregate(void *block);

extern DEVICEDLLSPEC unsigned char RxDescriptorPostDelimCrcErr(void *block);

extern DEVICEDLLSPEC unsigned char RxDescriptorDecryptBusyErr(void *block);

extern DEVICEDLLSPEC unsigned char RxDescriptorKeyMiss(void *block);
	
//
// setup a descriptor with the standard required fields
//
extern DEVICEDLLSPEC int RxDescriptorSetup(void *block, 
	unsigned int link_ptr, unsigned int buf_ptr, unsigned int buf_len);


//
// reset the descriptor so that it can be used again
//
extern DEVICEDLLSPEC int RxDescriptorReset(void *block);


//
// return the size of a descriptor 
//
extern DEVICEDLLSPEC int RxDescriptorSize(void);

//
// return 1 if the descriptor contains spectral scan data
//
extern DEVICEDLLSPEC int RxDescriptorSpectralScan(void *block);

#ifdef UNUSED
//
// copy the descriptor from application memory to the shared memory
//
extern DEVICEDLLSPEC void RxDescriptorWrite(void *block, unsigned int physical);


//
// copy the descriptor from the shared memory to application memory
//
extern DEVICEDLLSPEC void RxDescriptorRead(void *block, unsigned int physical);
#endif


//
// clear all rx descriptor function pointers and set to default behavior
//
extern DEVICEDLLSPEC int RxDescriptorFunctionReset();


//
// set the chip specific function
//
extern DEVICEDLLSPEC int RxDescriptorFunctionSelect(struct _RxDescriptorFunction *device);


