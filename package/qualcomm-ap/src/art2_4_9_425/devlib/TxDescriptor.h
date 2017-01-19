
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
struct _TxDescriptorFunction
{
    int (*LinkPtrSet)(void *block, unsigned int ptr);
    int (*TxRateSet)(void *block, unsigned int rate);
    unsigned char (*BaStatus)(void *block);
    unsigned int (*AggLength)(void *block);
    unsigned int (*BaBitmapLow)(void *block);
    unsigned int (*BaBitmapHigh)(void *block);
    unsigned char (*FifoUnderrun)(void *block);
    unsigned char (*ExcessiveRetries)(void *block);
    unsigned int (*RtsFailCount)(void *block);
    unsigned int (*DataFailCount)(void *block);
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
    unsigned int (*TxRate)(void *block);
    unsigned int (*DataLen)(void *block);
    unsigned char (*More)(void *block);
    unsigned int (*NumDelim)(void *block);
    unsigned int (*SendTimestamp)(void *block);
    unsigned char (*Gi)(void *block);
    unsigned char (*H2040)(void *block);
    unsigned char (*Duplicate)(void *block);
    unsigned int (*TxAntenna)(void *block);
    double (*Evm0)(void *block);
    double (*Evm1)(void *block);
    double (*Evm2)(void *block);
    unsigned char (*Done)(void *block);
    unsigned char (*FrameTxOk)(void *block);
    unsigned char (*CrcError)(void *block);
    unsigned char (*DecryptCrcErr)(void *block);
    unsigned char (*PhyErr)(void *block);
    unsigned char (*MicError)(void *block);
    unsigned char (*PreDelimCrcErr)(void *block);
    unsigned char (*KeyIdxValid)(void *block);
    unsigned int (*KeyIdx)(void *block);
    unsigned char (*MoreAgg)(void *block);
    unsigned char (*Aggregate)(void *block);
    unsigned char (*PostDelimCrcErr)(void *block);
    unsigned char (*DecryptBusyErr)(void *block);
    unsigned char (*KeyMiss)(void *block);
    int (*Setup)(void *block, 
		unsigned int link_ptr, unsigned int buf_ptr, int buf_len,
		int broadcast, int retry,
		int rate, int ht40, int shortGi, unsigned int txchain,
		int isagg, int moreagg,
		int id);
    int (*StatusSetup)(void *block); 
    int (*Reset)(void *block);
    int (*Size)();
    int (*StatusSize)();
    int (*Print)(void *block, char *buffer, int max);
    int  (*PAPDSetup)(void *block, int chainNum);
#ifdef UNUSED
    void (*Write)(void *block, unsigned int physical);
    void (*Read)(void *block, unsigned int physical);
#endif
};


//
// set the link pointer in a descriptor
//
extern DEVICEDLLSPEC int TxDescriptorLinkPtrSet(void *block, unsigned int ptr);

//
// set the transmit rate in a descriptor
//
extern DEVICEDLLSPEC int TxDescriptorTxRateSet(void *block, unsigned int rate);



extern DEVICEDLLSPEC int TxDescriptorPrint(void *block, char *buffer, int max);

extern DEVICEDLLSPEC unsigned char TxDescriptorBaStatus(void *block);

extern DEVICEDLLSPEC unsigned int TxDescriptorAggLength(void *block);

extern DEVICEDLLSPEC unsigned int TxDescriptorBaBitmapLow(void *block);

extern DEVICEDLLSPEC unsigned int TxDescriptorBaBitmapHigh(void *block);

extern DEVICEDLLSPEC unsigned char TxDescriptorFifoUnderrun(void *block);

extern DEVICEDLLSPEC unsigned char TxDescriptorExcessiveRetries(void *block);

extern DEVICEDLLSPEC unsigned int TxDescriptorRtsFailCount(void *block);

extern DEVICEDLLSPEC unsigned int TxDescriptorDataFailCount(void *block);

extern DEVICEDLLSPEC unsigned int TxDescriptorLinkPtr(void *block);

extern DEVICEDLLSPEC unsigned int TxDescriptorBufPtr(void *block);

extern DEVICEDLLSPEC unsigned int TxDescriptorBufLen(void *block);

extern DEVICEDLLSPEC unsigned char TxDescriptorIntReq(void *block);

extern DEVICEDLLSPEC unsigned int TxDescriptorRssiCombined(void *block);

extern DEVICEDLLSPEC unsigned int TxDescriptorRssiAnt00(void *block);

extern DEVICEDLLSPEC unsigned int TxDescriptorRssiAnt01(void *block);

extern DEVICEDLLSPEC unsigned int TxDescriptorRssiAnt02(void *block);

extern DEVICEDLLSPEC unsigned int TxDescriptorRssiAnt10(void *block);

extern DEVICEDLLSPEC unsigned int TxDescriptorRssiAnt11(void *block);

extern DEVICEDLLSPEC unsigned int TxDescriptorRssiAnt12(void *block);

extern DEVICEDLLSPEC unsigned int TxDescriptorTxRate(void *block);

extern DEVICEDLLSPEC unsigned int TxDescriptorDataLen(void *block);

extern DEVICEDLLSPEC unsigned char TxDescriptorMore(void *block);

extern DEVICEDLLSPEC unsigned int TxDescriptorNumDelim(void *block);

extern DEVICEDLLSPEC unsigned int TxDescriptorSendTimestamp(void *block);

extern DEVICEDLLSPEC unsigned char TxDescriptorGi(void *block);

extern DEVICEDLLSPEC unsigned char TxDescriptorH2040(void *block);

extern DEVICEDLLSPEC unsigned char TxDescriptorDuplicate(void *block);

extern DEVICEDLLSPEC unsigned int TxDescriptorTxAntenna(void *block);

extern DEVICEDLLSPEC double TxDescriptorEvm0(void *block);

extern DEVICEDLLSPEC double TxDescriptorEvm1(void *block);

extern DEVICEDLLSPEC double TxDescriptorEvm2(void *block);

extern DEVICEDLLSPEC unsigned char TxDescriptorDone(void *block);

extern DEVICEDLLSPEC unsigned char TxDescriptorFrameTxOk(void *block);

extern DEVICEDLLSPEC unsigned char TxDescriptorCrcError(void *block);

extern DEVICEDLLSPEC unsigned char TxDescriptorDecryptCrcErr(void *block);

extern DEVICEDLLSPEC unsigned char TxDescriptorPhyErr(void *block);

extern DEVICEDLLSPEC unsigned char TxDescriptorMicError(void *block);

extern DEVICEDLLSPEC unsigned char TxDescriptorPreDelimCrcErr(void *block);

extern DEVICEDLLSPEC unsigned char TxDescriptorKeyIdxValid(void *block);

extern DEVICEDLLSPEC unsigned int TxDescriptorKeyIdx(void *block);

extern DEVICEDLLSPEC unsigned char TxDescriptorMoreAgg(void *block);

extern DEVICEDLLSPEC unsigned char TxDescriptorAggregate(void *block);

extern DEVICEDLLSPEC unsigned char TxDescriptorPostDelimCrcErr(void *block);

extern DEVICEDLLSPEC unsigned char TxDescriptorDecryptBusyErr(void *block);

extern DEVICEDLLSPEC unsigned char TxDescriptorKeyMiss(void *block);
	
//
// setup a descriptor with the standard required fields
//
extern DEVICEDLLSPEC int TxDescriptorSetup(void *block, 
	unsigned int link_ptr, unsigned int buf_ptr, int buf_len,
	int broadcast, int retry,
	int rate, int ht40, int shortGi, unsigned int txchain,
	int isagg, int moreagg,
	int id);


//
// setup a descriptor with the standard required fields
//
extern DEVICEDLLSPEC int TxDescriptorStatusSetup(void *block);



//
// reset the descriptor so that it can be used again
//
extern DEVICEDLLSPEC int TxDescriptorReset(void *block);


//
// return the size of a descriptor 
//
extern DEVICEDLLSPEC int TxDescriptorSize();


//
// return the size of a descriptor 
//
extern DEVICEDLLSPEC int TxDescriptorStatusSize();

// set descritop bit PA predistortion chain num
extern DEVICEDLLSPEC int TxDescriptorPAPDSetup(void *block, int chainNum);


//
// set the chip specific function
//
extern DEVICEDLLSPEC int TxDescriptorFunctionSelect(struct _TxDescriptorFunction *txDescriptor);


//
// clear all tx descriptor function pointers and set to default behavior
//
extern DEVICEDLLSPEC int TxDescriptorFunctionReset(void);


