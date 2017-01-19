/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
#ifndef _H_AMP_IOCTL_
#define _H_AMP_IOCTL_

#define FILE_ATH_MINIPORT    FILE_DEVICE_UNKNOWN

#define ATH_MP_IOCTL(_index_) \
        CTL_CODE(FILE_ATH_MINIPORT, _index_+0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define PAL_REGISTER_CALLBACKS      ATH_MP_IOCTL(0x01)
#define PAL_DEREGISTER_CALLBACKS    ATH_MP_IOCTL(0x02)

// PAL USER SPACE TEST INTERFACE
#define PAL_SEND_HCI_CMD            ATH_MP_IOCTL(0x40)
#define PAL_GET_HCI_EVENT           ATH_MP_IOCTL(0x41)
#define PAL_GET_HCI_DATA			ATH_MP_IOCTL(0x42)
#define PAL_START_THREAD            ATH_MP_IOCTL(0x43)
#define PAL_CREATE_AMP              ATH_MP_IOCTL(0x44) 
#define PAL_DELETE_AMP              ATH_MP_IOCTL(0x45)
#define PAL_GET_PHY_TYPE            ATH_MP_IOCTL(0x46)
#define PAL_GET_MAX_RATE            ATH_MP_IOCTL(0x47)
#define PAL_GET_CHANNEL_LIST        ATH_MP_IOCTL(0x48)
#define PAL_START_BEACON            ATH_MP_IOCTL(0x49)
#define PAL_STOP_BEACON             ATH_MP_IOCTL(0x4A)
#define PAL_FAKE_PHY_LINK           ATH_MP_IOCTL(0x4B)
#define PAL_SET_SRM                 ATH_MP_IOCTL(0x4C)
#define PAL_SET_RTS                 ATH_MP_IOCTL(0x4D)



// For data path, use WriteFile and ReadFile.



#endif
