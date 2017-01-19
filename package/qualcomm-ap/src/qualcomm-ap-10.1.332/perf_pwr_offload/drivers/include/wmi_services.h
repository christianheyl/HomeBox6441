
/**
 * This file defines WMI services bitmap and the set of WMI services . 
 * defines macrso to set/clear/get different service bits from the bitmap. 
 * the service bitmap is sent up to the host via WMI_READY command.
 *    
 */

#ifndef _WMI_SERVICES_H_
#define _WMI_SERVICES_H_


#ifdef __cplusplus
extern "C" {
#endif



typedef  enum  {
    WMI_SERVICE_BEACON_OFFLOAD=0,     /* beacon offload */
    WMI_SERVICE_SCAN_OFFLOAD,         /* scan offload */
    WMI_SERVICE_ROAM_OFFLOAD,         /* roam offload */
    WMI_SERVICE_BCN_MISS_OFFLOAD,     /* beacon miss offload */
    WMI_SERVICE_STA_PWRSAVE,          /* fake sleep + basic power save */
    WMI_SERVICE_STA_ADVANCED_PWRSAVE, /* uapsd, pspoll, force sleep */
    WMI_SERVICE_AP_UAPSD,             /* uapsd on AP */
    WMI_SERVICE_AP_DFS,               /* DFS on AP */
    WMI_SERVICE_11AC,                 /* supports 11ac */
    WMI_SERVICE_BLOCKACK,             /* Supports triggering ADDBA/DELBA from host*/
    WMI_SERVICE_PHYERR,               /* PHY error */
    WMI_SERVICE_BCN_FILTER,           /* Beacon filter support */
    WMI_SERVICE_RTT,                  /* RTT (round trip time) support */
    WMI_SERVICE_RATECTRL,             /* Rate-control */
    WMI_SERVICE_WOW,                  /* WOW Support */
    WMI_SERVICE_RATECTRL_CACHE,       /* Rate-control caching */
    WMI_SERVICE_IRAM_TIDS,            /* TIDs in IRAM */
    WMI_MAX_SERVICE=64                /* max service */
} WMI_SERVICE;

#define WMI_SERVICE_BM_SIZE   ((WMI_MAX_SERVICE + sizeof(A_UINT32)- 1)/sizeof(A_UINT32))


/*
 * turn on the WMI service bit corresponding to  the WMI service.
 */
#define WMI_SERVICE_ENABLE(pwmi_svc_bmap,svc_id) \
    ( (pwmi_svc_bmap)[(svc_id)/(sizeof(A_UINT32))] |= \
         (1 << ((svc_id)%(sizeof(A_UINT32)))) ) 

#define WMI_SERVICE_DISABLE(pwmi_svc_bmap,svc_id) \
    ( (pwmi_svc_bmap)[(svc_id)/(sizeof(A_UINT32))] &=  \
      ( ~(1 << ((svc_id)%(sizeof(A_UINT32)))) ) ) 
      
#define WMI_SERVICE_IS_ENABLED(pwmi_svc_bmap,svc_id) \
    ( ((pwmi_svc_bmap)[(svc_id)/(sizeof(A_UINT32))] &  \
       (1 << ((svc_id)%(sizeof(A_UINT32)))) ) != 0) 

#ifdef __cplusplus
}
#endif

#endif /*_WMI_SERVICES_H_*/
