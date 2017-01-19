

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
// clear all device control function pointers and set to default behavior
//
extern AR9300DLLSPEC int Ar9300DeviceSelect();

extern AR9300DLLSPEC int Ar9300is2GHz(void);

extern AR9300DLLSPEC int Ar9300is5GHz(void);

extern AR9300DLLSPEC int Ar9300TxChainMany(void);

extern AR9300DLLSPEC int Ar9300RxChainMany(void);

extern AR9300DLLSPEC int Ar9300EepromRead(unsigned int address, unsigned char *buffer, int many);

extern AR9300DLLSPEC int Ar9300EepromWrite(unsigned int address, unsigned char *buffer, int many);

extern AR9300DLLSPEC int Ar9300OtpRead(unsigned int address, unsigned char *buffer, int many, int is_wifi);

extern AR9300DLLSPEC int Ar9300OtpWrite(unsigned int address, unsigned char *buffer, int many,int is_wifi);

extern AR9300DLLSPEC int Ar9300RxChainSet(int rxChain);

extern AR9300DLLSPEC int Ar9300DeafMode(int deaf); 

extern AR9300DLLSPEC int Ar9300Attach(int devid, int ssid);

extern AR9300DLLSPEC int Ar9300Reset(int frequency, unsigned int txchain, unsigned int rxchain, int bandwidth);

extern struct _ParameterList;

extern AR9300DLLSPEC int Ar9300GetParameterSplice(struct _ParameterList *list);

extern AR9300DLLSPEC int Ar9300SetParameterSplice(struct _ParameterList *list);

extern AR9300DLLSPEC int Ar9300SetCommand(int client);

extern AR9300DLLSPEC int Ar9300GetCommand(int client);

extern AR9300DLLSPEC int Ar9300_get_corr_coeff(int coeff_type, unsigned int **coeff_array, unsigned int *max_row, unsigned int *max_col);

extern struct ath_hal *AH;

