
#ifndef _INC_IMPORT_H
#define _INC_IMPORT_H
#ifdef _WINDOWS
#ifdef  __cplusplus
extern "C" {
#endif

#define DEVDRV_INTERFACE_SDIO       1
#define DEVDRV_INTERFACE_ETH        2
#define DEVDRV_INTERFACE_UART       3
#define DEVDRV_INTERFACE_USB		4

#define USE_ART_AGENT       0xffffffff

 __declspec(dllimport) void  __cdecl Test_dragon1(void);

 __declspec(dllimport) A_BOOL __cdecl closehandle(void);
 
 __declspec(dllimport) A_BOOL  __cdecl loadDriver_ENE(A_BOOL bOnOff,A_UINT32 subSystemID);
 
 __declspec(dllimport) A_BOOL __cdecl  InitAR6000_ene(HANDLE *handle, A_UINT32 *nTargetID);

 __declspec(dllimport) A_BOOL __cdecl  InitAR6000_eneX(HANDLE *handle, A_UINT32 *nTargetID, A_UINT8 devdrvInterface);

 __declspec(dllimport) A_BOOL __cdecl  EndAR6000_ene(int devIndex);
 
 __declspec(dllimport) int  __cdecl BMIWriteSOCRegister_win(HANDLE device, A_UINT32 address, A_UINT32 param);
 
  __declspec(dllimport) int __cdecl BMIReadSOCRegister_win(HANDLE device, A_UINT32 address, A_UINT32 *param);
  
  __declspec(dllimport) int __cdecl  DisableDragonSleep(HANDLE device);

  __declspec(dllimport) int __cdecl BMIReadMemory_win(HANDLE device, A_UINT32 address, A_UCHAR *buffer, A_UINT32 length);

__declspec(dllimport) int __cdecl BMIWriteMemory_win(HANDLE device, A_UINT32 address, A_UCHAR *buffer, A_UINT32 length);

__declspec(dllimport) int __cdecl BMIDone_win(HANDLE device);

__declspec(dllimport) int __cdecl BMISetAppStart_win(HANDLE handle, A_UINT32 address);

__declspec(dllimport) int __cdecl BMIFastDownload_win(HANDLE device, A_UINT32 address, A_UCHAR *buffer, A_UINT32 length);

__declspec(dllimport) int __cdecl BMIExecute_win(HANDLE handle, A_UINT32 address, A_UINT32 *param);

__declspec(dllexport) int __cdecl BMITransferEepromFile_win(HANDLE handle, A_UCHAR *eeprom, A_UINT32 length);

__declspec(dllimport) HANDLE __cdecl open_device_ene(A_UINT32 device_fn, A_UINT32 devIndex, char * pipeName);

__declspec(dllimport) DWORD __cdecl DRG_Write(  HANDLE COM_Write,  PUCHAR buf, ULONG length );

__declspec(dllimport) DWORD __cdecl DRG_Read( HANDLE pContext,  PUCHAR buf, ULONG length,  PULONG pBytesRead);

__declspec(dllimport) A_STATUS _cdecl DEVDRV_WMI_Write(  HANDLE COM_Write, PUCHAR inBuf, ULONG inLength, PUCHAR outBuf, ULONG *outLength );

__declspec(dllimport) A_STATUS _cdecl DEVDRV_WMI_Write_Raw(  HANDLE COM_Write, PUCHAR inBuf, ULONG inLength, PUCHAR outBuf, ULONG *outLength );

__declspec(dllimport) void __cdecl devdrv_MyMallocStart ();

__declspec(dllimport) void __cdecl devdrv_MyMallocEnd ();

#ifdef  __cplusplus
}
#endif

#endif //_WINDOWS
#endif //_INC_IMPORT_H

