
#ifdef _WINDOWS
#ifdef FIELDDLL
		#define FIELDDLLSPEC __declspec(dllexport)
	#else
		#define FIELDDLLSPEC __declspec(dllimport)
	#endif
#else
	#define FIELDDLLSPEC
#endif


extern FIELDDLLSPEC void ResetForce();


extern FIELDDLLSPEC void ResetForceClear();


extern FIELDDLLSPEC int ResetForceGet();
