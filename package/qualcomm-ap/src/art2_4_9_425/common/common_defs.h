#ifndef _INC_COMMON_DEFS_H
#define _INC_COMMON_DEFS_H
/* Reset Flags */
#define MAC_RESET       0x1
#define BB_RESET        0x2
#define BUS_RESET       0x4

//modes
#define MODE_11A				0
#define MODE_11G				1
#define MODE_11B				2
#define MODE_11O				3	//OFDM at 2.4
#define HALF_SPEED_MODE			50
#define QUARTER_SPEED_MODE		51
#define TURBO_ENABLE			1


#if defined(MDK_AP) || defined(SOC_AP) 
#define mSleep(x) (milliSleep(x))
extern A_UINT32 milliTime();
extern void milliSleep(A_UINT32);
#endif

#if !defined(ECOS)
#ifndef SWIG
A_INT32 uiPrintf ( const char *format, ...);


A_INT32 q_uiPrintf ( const char *format, ...);

A_INT16 statsPrintf ( FILE *pFile, const char *format, ...);
#endif
#endif

#ifdef WIN32
#define mSleep(x) (Sleep((DWORD) x))
#if !defined(__ATH_DJGPPDOS__)
#define milliTime() (GetTickCount())
#endif
#endif

#if defined(LINUX) || defined(__APPLE__)
#define mSleep(x) (milliSleep(x))
extern A_UINT32 milliTime(void);
extern void milliSleep(A_UINT32);
#define microSleep(x) usleep(x)
#endif	

#endif

