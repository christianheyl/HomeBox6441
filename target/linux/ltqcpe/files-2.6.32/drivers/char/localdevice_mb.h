
#ifdef VPE1_SW
MB_ERROR_CODES LocalDeviceMsgApi(void *pt_ArgStruct, unsigned int *pul_PayloadLength, int l_MsgType);
#endif


//
// Debug Access command
//


typedef struct {
    unsigned char  uc_command;
    unsigned char  uc_dataType; 
    unsigned short us_count;
    unsigned int   ul_addr;

} DebugAccessCommand_t;

typedef struct {
  DebugAccessCommand_t t_DebugAccessCommand;
#ifdef VPE1_SW
  unsigned Payload[1];
#else
  unsigned int data;
#endif
} LocalDeviceDebugAccess_t;

#ifdef VPE1_SW
    #define DEBUG_ACCESS_COMMAND_HDR_SIZE       sizeof(DebugAccessCommand_t)
    #define DEBUG_ACCESS_ERROR_PAYLOAD_SIZE     sizeof(LocalDeviceDebugAccess_t)
#endif

typedef enum {
    MB_DEBUG_ACCESS_READ = 1,
    MB_DEBUG_ACCESS_WRITE
}DebugAccessCmdTypeE;

typedef enum {
    MB_DEBUG_ACCESS_TYPE_8BIT  = 1,
    MB_DEBUG_ACCESS_TYPE_16BIT = 2,
    MB_DEBUG_ACCESS_TYPE_32BIT = 4
}DebugAccessDataTypeE;

//
// Hello command
//

typedef struct {
    unsigned int Payload[1];
} LocalDeviceHello_t;

#define HELLO_PAYLOAD_SIZE                      sizeof(LocalDeviceHello_t)


