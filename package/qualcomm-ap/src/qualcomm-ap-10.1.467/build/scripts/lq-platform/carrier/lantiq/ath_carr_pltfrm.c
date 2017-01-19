/*
 *  Copyright ACE(c) 2005 Atheros Communications Inc.  All rights reserved.
 * OS interface private file.
 */
 
#include "osif_private.h"
#include <ieee80211_var.h>
#include <ieee80211_extap.h>

#ifdef CONFIG_IFX_PPA
#include <net/ifx_ppa_stack_al.h>
#include <net/ifx_ppa_api.h>
#include <net/ifx_ppa_api_directpath.h>
#include <net/ifx_ppa_hook.h>
#include <net/ifx_ppa_ppe_hal.h>
#endif

#define MAX_VAP 8
#define OSIF_TO_NETDEV(_osif) (((osif_dev *)(_osif))->netdev)

#ifdef CONFIG_IFX_PPA
typedef struct {
    int ppa_if_id[MAX_VAP];
    int dev_if_index[MAX_VAP];
    int ppa_enable[MAX_VAP];
    PPA_DIRECTPATH_CB ppa_pDirectpathCb;
}__pltfrm_private;

typedef __pltfrm_private pltfrm_dev;
typedef __pltfrm_private* pltfrm_dev_t;

pltfrm_dev ppa_pltfrm_data;

#if QCA_OL_11AC_FAST_PATH
extern int osif_ol_vap_hardstart_fast(struct sk_buff *skb, struct net_device *dev);
#endif
extern int osif_vap_hardstart(struct sk_buff *skb, struct net_device *dev);

static int wlan_pltfrm_ppa_recv(struct net_device *rxif, 
                            struct net_device *txif, 
                            struct sk_buff *skb,
                            int len)
{
    char hdr[12];

    if( txif != NULL){
#if QCA_OL_11AC_FAST_PATH
        osif_dev  *osifp = ath_netdev_priv(txif);
#endif
        skb->dev = txif;
        //take off 4 bytes vlan if exists
        if (skb->data[12] == 0x81)
        {
            memcpy(hdr, skb->data, 12); 
            memcpy(skb->data + 4, hdr, 12);
            skb_pull(skb, 4);
        }
#if QCA_OL_11AC_FAST_PATH
        if (osifp->osif_is_mode_offload)
            osif_ol_vap_hardstart_fast(skb,txif);
        else
#endif
            osif_vap_hardstart(skb,txif);
    }
    else if( rxif != NULL){
    /* as usual shift the eth header with skb->data */
    skb->protocol = eth_type_trans(skb, skb->dev);
    /* push up to protocol stacks */
    netif_rx(skb);
    } else {
    dev_kfree_skb_any(skb);
    }

    return 0;  
}
 
static int wlan_pltfrm_ppa_enable(wlan_if_t vap)
{   
    osif_dev  *osdev = (osif_dev *)vap->iv_ifp;
    struct net_device *dev = ((osif_dev *)vap->iv_ifp)->netdev; 
    int ppa_if_id = 0;
    int ret = 0;

    if(!ppa_hook_directpath_register_dev_fn){     
    printk("ppa_hook_directpath_register_dev_fn not found\n");
    return -1;
    }
    printk("Before hook ppa\n");      
    ret = ppa_hook_directpath_register_dev_fn(&ppa_if_id, dev,
            &ppa_pltfrm_data.ppa_pDirectpathCb, 
            PPA_F_DIRECTPATH_ETH_IF|PPA_F_DIRECTPATH_REGISTER);
   
    if(ret){
    printk("PPA REG FAIL\n");
    ppa_pltfrm_data.ppa_enable[osdev->os_unit] = 0;      
        return -1;
    } else {
        ppa_pltfrm_data.ppa_if_id[osdev->os_unit] = ppa_if_id;
        ppa_pltfrm_data.dev_if_index[osdev->os_unit] = dev->ifindex;
        ppa_pltfrm_data.ppa_enable[osdev->os_unit] = 1;
        printk("PPA REG DONE, if_id=%d, dev_if_index = %d, os_unit=%d \n", ppa_if_id, dev->ifindex, osdev->os_unit);      
    }

    return 0;
}

static int wlan_pltfrm_ppa_disable(wlan_if_t vap)
{   
    osif_dev  *osdev = (osif_dev *)vap->iv_ifp;
    int ppa_if_id = 0;
    int ret = 0;
    
    if( !ppa_pltfrm_data.ppa_enable[osdev->os_unit])
       return 0;

    if(!ppa_hook_directpath_register_dev_fn){     
    printk("ppa_hook_directpath_register_dev_fn not found\n");
    return -1;
    }
    printk("Before hook ppa\n");          
    ppa_if_id = ppa_pltfrm_data.ppa_if_id[osdev->os_unit];
    ret = ppa_hook_directpath_register_dev_fn(&ppa_if_id,NULL,NULL,0);
    
    if (ret){
        printk("deregister PPA FAIL!!\n");
    ppa_pltfrm_data.ppa_enable[osdev->os_unit] = 0;
        return -1;
    } else {
        ppa_pltfrm_data.ppa_if_id[osdev->os_unit] = 0;
    ppa_pltfrm_data.ppa_enable[osdev->os_unit] = 0;
        printk("deregister PPA Done!!\n");
    }
    
    return 0;
}

static int wlan_pltfrm_ppa_get(wlan_if_t vap)
{
    osif_dev  *osdev = (osif_dev *)vap->iv_ifp;

    return (ppa_pltfrm_data.ppa_enable[osdev->os_unit] ? 1 : 0 );
}
#endif

int wlan_pltfrm_set_param(wlan_if_t vaphandle, u_int32_t val)
{
#ifdef CONFIG_IFX_PPA    
    wlan_if_t vap = vaphandle;
    int retv = 0;

    if (val)
        wlan_pltfrm_ppa_enable(vap);
    else
        wlan_pltfrm_ppa_disable(vap);

    return retv;
#else
    return 0;
#endif
}

int wlan_pltfrm_get_param(wlan_if_t vaphandle)
{
#ifdef  CONFIG_IFX_PPA
    wlan_if_t vap = vaphandle;
    int val = 0;
    val = wlan_pltfrm_ppa_get(vap);

    return val;
#else
    return 0;
#endif
}

void wlan_pltfrm_attach(struct net_device *dev)
{
#ifdef CONFIG_IFX_PPA
    memset(&ppa_pltfrm_data,0,sizeof(pltfrm_dev));
    ppa_pltfrm_data.ppa_pDirectpathCb.rx_fn = wlan_pltfrm_ppa_recv;
    ppa_pltfrm_data.ppa_pDirectpathCb.start_tx_fn = NULL;
    ppa_pltfrm_data.ppa_pDirectpathCb.stop_tx_fn = NULL;
#endif
}

void wlan_pltfrm_detach(struct net_device *dev)
{

}

void osif_pltfrm_receive (os_if_t osif, wbuf_t wbuf,
                          u_int16_t type, u_int16_t subtype,
                          ieee80211_recv_status *rs)
{
    struct net_device *dev = OSIF_TO_NETDEV(osif);
    struct sk_buff *skb = (struct sk_buff *)wbuf;
//#if ATH_SUPPORT_VLAN
    osif_dev  *osifp = (osif_dev *) osif;
//#endif
#ifdef CONFIG_IFX_PPA
    wlan_if_t vap = ((osif_dev *)osif)->os_if;
    int ret = 0;
#endif

    if (type != IEEE80211_FC0_TYPE_DATA) {
        wbuf_free(wbuf);
        return; 
    }
      
    skb->dev = dev;

#ifdef CONFIG_IFX_PPA
    if( ppa_hook_directpath_send_fn && ppa_pltfrm_data.ppa_enable[osifp->os_unit] && (ppa_pltfrm_data.dev_if_index[osifp->os_unit] == dev->ifindex )) {
        ret = ppa_hook_directpath_send_fn(ppa_pltfrm_data.ppa_if_id[osifp->os_unit], skb, skb->len, 0);
        if(ret == 0){
            skb = NULL;
            return;
        } else {
            /* WAR for ppa_hook_directpath_send_fn() return -1 but still hold the skb */
            printk("%s: ret=%d, os_unit=%d ppa_enable=%d ppa_dev_if_index=%d dev_if_index=%d \n", 
                __func__, ret, osifp->os_unit,  ppa_pltfrm_data.ppa_enable[osifp->os_unit], 
                ppa_pltfrm_data.dev_if_index[osifp->os_unit], dev->ifindex);
            skb = NULL;
            return;
        }
     }
#endif

#ifdef USE_HEADERLEN_RESV
    skb->protocol = ath_eth_type_trans(skb, dev);
#else
    skb->protocol = eth_type_trans(skb, dev);
#endif

#if ATH_SUPPORT_VLAN
    if ( osifp->vlanID != 0 && osifp->vlgrp != NULL){
        /* attach vlan tag */
        vlan_hwaccel_rx(skb, osifp->vlgrp, osifp->vlanID);
    }
    else {
#endif 
        netif_rx(skb);
#if ATH_SUPPORT_VLAN
    }   
#endif
    dev->last_rx = jiffies;
}

void
osif_pltfrm_deliver_data_ol(os_if_t osif, struct sk_buff *skb_list)
{
    struct net_device *dev = OSIF_TO_NETDEV(osif);
//#if ATH_SUPPORT_VLAN
    osif_dev  *osifp = (osif_dev *) osif;
//#endif
#ifdef CONFIG_IFX_PPA
    wlan_if_t vap = ((osif_dev *)osif)->os_if;
    int ret = 0;
#endif
#ifdef ATH_EXT_AP
    struct ether_header *eh;
#endif

#if ATH_RXBUF_RECYCLE
    struct net_device *comdev;
    struct ath_softc_net80211 *scn;
    struct ath_softc *sc;
#endif /* ATH_RXBUF_RECYCLE */

#if ATH_RXBUF_RECYCLE
    comdev = ((osif_dev *)osif)->os_comdev;
    scn = ath_netdev_priv(comdev);
    sc = ATH_DEV_TO_SC(scn->sc_dev);
#endif /* ATH_RXBUF_RECYCLE */

    while (skb_list) {
        struct sk_buff *skb;

        skb = skb_list;
        skb_list = skb_list->next;

        skb->dev = dev;

#ifdef ATH_EXT_AP
    if (adf_os_unlikely(IEEE80211_VAP_IS_EXT_AP_ENABLED(vap))) {
        eh = (struct ether_header *)skb->data;
        if (vap->iv_opmode == IEEE80211_M_STA) {
            if (ieee80211_extap_input(vap, eh)) {
                dev_kfree_skb(skb);
                printk("Freeing the SKB in Ext AP Input translation \n");
                return NULL;
            }
        } else {
#ifdef EXTAP_DEBUG
            extern char *arps[];
            eth_arphdr_t *arp = (eth_arphdr_t *)(eh + 1);
            if (eh->ether_type == ETHERTYPE_ARP) {
                printk("InP %s\t" eaistr "\t" eamstr "\t" eaistr "\t" eamstr "\n"
                       "s: " eamstr "\td: " eamstr "\n",
                       arps[arp->ar_op],
                       eaip(arp->ar_sip), eamac(arp->ar_sha),
                       eaip(arp->ar_tip), eamac(arp->ar_tha),
                       eamac(eh->ether_shost), eamac(eh->ether_dhost));
            }
#endif
        }
    }
#endif /* ATH_EXT_AP */

#ifdef CONFIG_IFX_PPA
    if( ppa_hook_directpath_send_fn && ppa_pltfrm_data.ppa_enable[osifp->os_unit] && (ppa_pltfrm_data.dev_if_index[osifp->os_unit] == dev->ifindex )) {
        ret = ppa_hook_directpath_send_fn(ppa_pltfrm_data.ppa_if_id[osifp->os_unit], skb, skb->len, 0);
        if(ret == 0){
            skb = NULL;
            continue;
        } else {
            /* WAR for ppa_hook_directpath_send_fn() return -1 but still hold the skb */
//            printk("%s: ret=%d, os_unit=%d ppa_enable=%d ppa_dev_if_index=%d dev_if_index=%d \n", 
//                __func__, ret, osifp->os_unit,  ppa_pltfrm_data.ppa_enable[osifp->os_unit], 
//                ppa_pltfrm_data.dev_if_index[osifp->os_unit], dev->ifindex);
            skb = NULL;
            continue;
        }
     }
#endif

#ifdef USE_HEADERLEN_RESV
        skb->protocol = ath_eth_type_trans(skb, dev);
#else
        skb->protocol = eth_type_trans(skb, dev);
#endif

#if ATH_RXBUF_RECYCLE
        /*
         * Do not recycle the received mcast frame b/c it will be cloned twice
         */
        if (sc->sc_osdev->rbr_ops.osdev_wbuf_collect && !(wbuf_is_cloned(skb)))
        {    
            sc->sc_osdev->rbr_ops.osdev_wbuf_collect((void *)sc, (void *)skb);    
        }
#endif /* ATH_RXBUF_RECYCLE */
#if ATH_SUPPORT_VLAN
        if ( osifp->vlanID != 0 && osifp->vlgrp != NULL)
        {
            /* attach vlan tag */
            vlan_hwaccel_rx(skb, osifp->vlgrp, osifp->vlanID);
        }
        else
#endif
#ifdef ATH_SUPPORT_HTC
        if (in_interrupt())
            netif_rx(skb);
        else
        netif_rx_ni(skb);
#else
        netif_rx(skb);
#endif
    }
    dev->last_rx = jiffies;
}

void osif_pltfrm_create_vap(osif_dev *osifp)
{
}

void osif_pltfrm_delete_vap(osif_dev *osifp)
{
#ifdef CONFIG_IFX_PPA
    wlan_if_t vap = osifp->os_if;  
 
    wlan_pltfrm_ppa_disable(vap);
#endif
}

void osif_pltfrm_record_macinfor(unsigned char unit, unsigned char* mac)
{
}

void osif_pltfrm_vlan_feature_set(struct net_device *dev)
{
        dev->features |= NETIF_F_HW_VLAN_TX | NETIF_F_HW_VLAN_RX |
            NETIF_F_HW_VLAN_FILTER;
}

DEFINE_SPINLOCK(pltfrm_pciwar_lock);

#ifdef PLATFORM_BYTE_SWAP
void
WAR_PLTFRM_PCI_WRITE32(char *addr, u32 offset, u32 value, unsigned int war1)
{
    if (war1) {
        unsigned long irq_flags;

        spin_lock_irqsave(&pltfrm_pciwar_lock, irq_flags);

        (void)(__bswap32(ioread32((void __iomem *)(addr+offset+4)))); /* 3rd read prior to write */
        (void)(__bswap32(ioread32((void __iomem *)(addr+offset+4)))); /* 2nd read prior to write */
        (void)(__bswap32(ioread32((void __iomem *)(addr+offset+4)))); /* 1st read prior to write */
        iowrite32(__bswap32((u32)(value)), (void __iomem *)(addr+offset));

        spin_unlock_irqrestore(&pltfrm_pciwar_lock, irq_flags);
    } else {
        iowrite32(__bswap32((u32)(value)), (void __iomem *)(addr+offset));
    }
}
#else
void
WAR_PLTFRM_PCI_WRITE32(char *addr, u32 offset, u32 value, unsigned int war1)
{
    if (war1) {
        unsigned long irq_flags;

        spin_lock_irqsave(&pltfrm_pciwar_lock, irq_flags);

        (void)(ioread32((void __iomem *)(addr+offset+4))); /* 3rd read prior to write */
        (void)(ioread32((void __iomem *)(addr+offset+4))); /* 2nd read prior to write */
        (void)(ioread32((void __iomem *)(addr+offset+4))); /* 1st read prior to write */
        iowrite32((u32)(value), (void __iomem *)(addr+offset));

        spin_unlock_irqrestore(&pltfrm_pciwar_lock, irq_flags);
    } else {
        iowrite32((u32)(value), (void __iomem *)(addr+offset));
    }
}
#endif /* PLATFORM_BYTE_SWAP */