

#ifdef _WINDOWS
#ifdef AR9300DLL
		#define AR9300DLLSPEC __declspec(dllexport)
	#else
		#define AR9300DLLSPEC __declspec(dllimport)
	#endif
#else
	#define AR9300DLLSPEC
#endif

//
// Select the ar9300 as the active chip set.
// Makes the generic functions point to the chip specific functions defined here.
//
extern int Ar9300RxDescriptorFunctionSelect();


#ifdef UNUSED
AR9300DLLSPEC int Ar9300RxDescriptorPrint(void *block, char *buffer, int max);

AR9300DLLSPEC unsigned int Ar9300RxDescriptorLinkPtr(void *block);

AR9300DLLSPEC unsigned int Ar9300RxDescriptorBufPtr(void *block);

AR9300DLLSPEC unsigned int Ar9300RxDescriptorBufLen(void *block);

AR9300DLLSPEC unsigned int Ar9300RxDescriptorRssiCombined(void *block);

AR9300DLLSPEC unsigned int Ar9300RxDescriptorRssiAnt00(void *block);

AR9300DLLSPEC unsigned int Ar9300RxDescriptorRssiAnt01(void *block);

AR9300DLLSPEC unsigned int Ar9300RxDescriptorRssiAnt02(void *block);

AR9300DLLSPEC unsigned int Ar9300RxDescriptorRssiAnt10(void *block);

AR9300DLLSPEC unsigned int Ar9300RxDescriptorRssiAnt11(void *block);

AR9300DLLSPEC unsigned int Ar9300RxDescriptorRssiAnt12(void *block);

AR9300DLLSPEC unsigned int Ar9300RxDescriptorRxRate(void *block);

AR9300DLLSPEC unsigned int Ar9300RxDescriptorDataLen(void *block);

AR9300DLLSPEC unsigned char Ar9300RxDescriptorMore(void *block);

AR9300DLLSPEC unsigned int Ar9300RxDescriptorNumDelim(void *block);

AR9300DLLSPEC unsigned int Ar9300RxDescriptorRcvTimestamp(void *block);

AR9300DLLSPEC unsigned char Ar9300RxDescriptorGi(void *block);

AR9300DLLSPEC unsigned char Ar9300RxDescriptorH2040(void *block);

AR9300DLLSPEC unsigned char Ar9300RxDescriptorDuplicate(void *block);

AR9300DLLSPEC unsigned int Ar9300RxDescriptorRxAntenna(void *block);

AR9300DLLSPEC double Ar9300RxDescriptorEvm0(void *block);

AR9300DLLSPEC double Ar9300RxDescriptorEvm1(void *block);

AR9300DLLSPEC double Ar9300RxDescriptorEvm2(void *block);

AR9300DLLSPEC unsigned char Ar9300RxDescriptorDone(void *block);

AR9300DLLSPEC unsigned char Ar9300RxDescriptorFrameRxOk(void *block);

AR9300DLLSPEC unsigned char Ar9300RxDescriptorCrcError(void *block);

AR9300DLLSPEC unsigned char Ar9300RxDescriptorDecryptCrcErr(void *block);

AR9300DLLSPEC unsigned char Ar9300RxDescriptorPhyErr(void *block);

AR9300DLLSPEC unsigned char Ar9300RxDescriptorMicError(void *block);

AR9300DLLSPEC unsigned char Ar9300RxDescriptorPreDelimCrcErr(void *block);

AR9300DLLSPEC unsigned char Ar9300RxDescriptorKeyIdxValid(void *block);

AR9300DLLSPEC unsigned int Ar9300RxDescriptorKeyIdx(void *block);

AR9300DLLSPEC unsigned char Ar9300RxDescriptorFirstAgg(void *block);

AR9300DLLSPEC unsigned char Ar9300RxDescriptorMoreAgg(void *block);

AR9300DLLSPEC unsigned char Ar9300RxDescriptorAggregate(void *block);

AR9300DLLSPEC unsigned char Ar9300RxDescriptorPostDelimCrcErr(void *block);

AR9300DLLSPEC unsigned char Ar9300RxDescriptorDecryptBusyErr(void *block);

AR9300DLLSPEC unsigned char Ar9300RxDescriptorKeyMiss(void *block);

AR9300DLLSPEC void Ar9300RxDescriptorSetup(void *block, 
	unsigned int link_ptr, unsigned int buf_ptr, unsigned int buf_len);

AR9300DLLSPEC void Ar9300RxDescriptorReset(void *block);

AR9300DLLSPEC int Ar9300RxDescriptorSize();
#endif
