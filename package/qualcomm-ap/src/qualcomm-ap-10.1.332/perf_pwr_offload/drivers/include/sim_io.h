/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * 
 */

#ifndef __SIM_IO_H__
#define __SIM_IO_H__

#ifdef A_SIMOS_DEVHOST  /* entire file */

#include <linux/ioctl.h>

#define ATH_SIM_DEVICE                  "/dev/ath_sim"
#define SIM_IOCTL_ID                   (0xa1)

#define SIM_IOCTL_GET_DATA      _IOWR(SIM_IOCTL_ID, 0, struct sim_ioctl_get_data_s)
#define SIM_IOCTL_DMA_WRITE     _IOW(SIM_IOCTL_ID, 1, struct sim_ioctl_dma_write_s)
#define SIM_IOCTL_MEMCPY        _IOWR(SIM_IOCTL_ID, 2, struct sim_ioctl_memcpy_s)

// SIM_IOCTL_GET_DATA
struct sim_ioctl_get_data_s {
    A_UINT32    mode;
    A_UINT32    frag_desc_ptr;
    A_UINT32    fraglen_or_buf_ptr;
};

// SIM_IOCTL_DMA_WRITE
struct sim_ioctl_dma_write_s {
    A_UINT32    target_buf;             // used as ptr to char
    A_UINT32    len;
    A_UINT32    dest_addr_ptr;
    A_UINT32    host_buf_offset;
};

// SIM_IOCTL_MEMCPY
struct sim_ioctl_memcpy_s {
    A_UINT32    host_addr;
    A_UINT32    target_addr;
    A_UINT32    len;
};

#endif	/* A_SIMOS_DEVHOST */

#endif  /* __SIM_IO_H__ */
