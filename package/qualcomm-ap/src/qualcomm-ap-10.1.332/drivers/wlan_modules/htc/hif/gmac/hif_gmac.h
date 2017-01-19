#ifndef __HIF_GMAC_H
#define __HIF_GMAC_H

#include <linux/if_ether.h>
#include <adf_os_io.h>
#include <adf_nbuf.h>
#include <adf_os_lock.h>
#include <adf_os_defer.h>
#include <if_ath_gmac.h>
#include <hif.h>

#define ETH_P_ATH               0x88BD
#define ATH_P_MAGBOOT           0x12 /*Magpie is booting*/
#define ATH_P_MAGNORM           0x13 /*Magpie is in default state*/
#define ATH_HLEN                sizeof(struct athhdr)
#define GMAC_HLEN               sizeof(struct gmac_hdr)
#define NUM_RECV_FNS            2
/**
 * Change this for handling multiple magpies
 */
#define GMAC_HASH_SHIFT     2 
#define GMAC_TBL_SIZE       (1 << GMAC_HASH_SHIFT)


typedef struct ethhdr     ethhdr_t;

typedef struct athtype{
#if defined (ADF_LITTLE_ENDIAN_MACHINE)
    a_uint8_t    proto:6,
                 res :2;
#elif defined (ADF_BIG_ENDIAN_MACHINE)
    a_uint8_t    res :  2,
                 proto : 6;
#else
#error  "Please fix"
#endif
    a_uint8_t   res_lo;
    a_uint16_t  res_hi;
}__attribute__((packed))  athtype_t;

typedef struct athhdr{
    athtype_t   type;
    a_uint8_t  rel;
    a_uint8_t  seq_no;
}athhdr_t;

typedef struct gmac_hdr{
    ethhdr_t    eth;
    athhdr_t    ath;
}gmac_hdr_t;

struct hif_gmac_softc;
struct hif_gmac_node;


typedef void (*__gmac_recv_fn_t)(adf_nbuf_t  skb, struct hif_gmac_node  *node, 
                                a_uint8_t proto);

typedef enum {
	APP_FWD = 0,
	APP_HTC = 1,
    NUM_APPS
} hif_app_type_t;

typedef enum {
    RECV_EXCEP,
    RECV_DEF,
    NUM_RECV
}hif_gmac_recv_idx_t;



typedef void (*gmac_os_defer_fn_t)(void *);

#if LINUX_VERSION_CODE  <= KERNEL_VERSION(2,6,19)
typedef struct work_struct     gmac_os_work_t;
#else
/**
 *  * wrapper around the real task func
 *   */


typedef struct {   
    struct work_struct   work;
    gmac_os_defer_fn_t    fn;
    void                 *arg;
} gmac_os_work_t;

void gmac_os_defer_func(struct work_struct *work);
#endif



static inline void
gmac_os_sched_work(gmac_os_work_t *work)
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,19)
    schedule_work(work);
#else
    schedule_work(&work->work);
#endif

}
static inline void
gmac_os_init_work(gmac_os_work_t *work,gmac_os_defer_fn_t func, void *arg)
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,19)
    INIT_WORK(work, func, arg);
#else
    work->fn = func ;
    work->arg  = arg  ;
    INIT_WORK(&work->work, gmac_os_defer_func);
#endif

}
/**
 * Per device structure
 */
typedef struct hif_gmac_node{
    os_gmac_dev_t          *dev; /* Netdev associated with this device */
    gmac_hdr_t              hdr; /* GMAC header to insert per packet */
    a_uint32_t              host_is_rdy; /* State of the HOST APP */
    adf_nbuf_t              tgt_rdy_skb; /* State of the TGT FW */
    adf_os_handle_t         os_hdl; /* OS handle to be used during attach */
    hif_app_type_t          app_cur; /* per device */
    gmac_os_work_t          work;           
}hif_gmac_node_t;

/* Per APP structure */
typedef struct hif_gmac_app{
    struct htc_drvreg_callbacks      dev_cb;
    struct htc_callbacks  pkt_cb; /* Device specific callbacks */
    void                   *ctx; /* APP's context */
}hif_gmac_app_t;

typedef void (* os_dev_fn_t)(hif_gmac_node_t  *node);
typedef int (* os_dev_discv_t)(adf_nbuf_t  skb, hif_gmac_node_t   *node);

/* HIF GMAC softc */
typedef struct hif_gmac_softc{
    os_dev_fn_t             attach; /* defer able version of attach */
    os_dev_fn_t             detach;
    os_dev_discv_t          discv;
    hif_gmac_app_t          app_reg[NUM_APPS];
    hif_app_type_t          next_app;
    __gmac_recv_fn_t        recv_fn[NUM_RECV_FNS];
    adf_os_spinlock_t       lock_bh;
    hif_gmac_node_t         node_tbl[GMAC_TBL_SIZE]; /* XXX: size should be 
                                                        programmable */
}hif_gmac_softc_t;

hif_gmac_softc_t  * hif_gmac_init(void);
void                hif_gmac_exit(hif_gmac_softc_t *sc);
void                hif_gmac_dev_attach(hif_gmac_node_t  *node);
void                hif_gmac_dev_detach(hif_gmac_node_t  *node);


static inline athhdr_t *
ath_hdr(adf_nbuf_t skb)
{
    a_uint8_t  *data = NULL;
    a_uint32_t  len = 0;

    adf_nbuf_peek_header(skb, &data, &len);

    return (athhdr_t *)data;
}
static inline a_bool_t
is_ath_header(athhdr_t  *ath, a_uint8_t  sub_type)
{
    return ( ath->type.proto == sub_type);
}

static inline void
ath_put_hdr(struct sk_buff *skb, athhdr_t  *hdr)
{
    athhdr_t  *ath;

    ath  = (athhdr_t *)adf_nbuf_push_head(skb, ATH_HLEN);
    ath->type.proto = hdr->type.proto;
}
#endif

