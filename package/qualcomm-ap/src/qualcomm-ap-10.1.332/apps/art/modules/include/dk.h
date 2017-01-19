/* dk.h - contains variable declarations */

#ifndef __DK_H_
#define __DK_H_

// Driver information
#define DRV_NAME    "linuxdk"
#define DRV_MAJOR_VERSION 1
#define DRV_MINOR_VERSION 2

#ifdef MODULE
#define MOD_LICENCE "GPL"
#define MOD_AUTHOR "muddin@atheros.com"
#define MOD_DESCRIPTION "Linux MDK driver 1.0" 
#endif

// Common variable types are typedefed 
typedef short INT16;
typedef unsigned short UINT16;
typedef int INT32;
typedef long A_INT_PTR;
typedef unsigned long A_UINT_PTR;
typedef unsigned int UINT32;
typedef char CHAR8;
typedef unsigned char UCHAR8;
typedef char INT8;
typedef unsigned char UINT8;
typedef void VOID;

#ifndef NULL
#define NULL (void *)0
#endif

#define MAX_BARS 6

typedef enum 
{
		FALSE=0,
		TRUE
} BOOLEAN;

#endif // __DK_H_
