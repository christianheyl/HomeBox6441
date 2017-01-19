
#ifdef _WINDOWS
#ifdef FIELDDLL
		#define FIELDDLLSPEC __declspec(dllexport)
	#else
		#define FIELDDLLSPEC __declspec(dllimport)
	#endif
#else
	#define FIELDDLLSPEC
#endif



extern FIELDDLLSPEC int NartVersionMajor();

extern FIELDDLLSPEC int NartVersionMinor();

extern FIELDDLLSPEC char* NartVersionDate();

extern FIELDDLLSPEC char* NartVersionTime();
