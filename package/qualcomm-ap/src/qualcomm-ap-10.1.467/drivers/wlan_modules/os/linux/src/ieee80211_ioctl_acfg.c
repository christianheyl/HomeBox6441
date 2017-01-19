#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
#include <linux/config.h>
#endif
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/utsname.h>
#include <linux/if_arp.h>       /* XXX for ARPHRD_ETHER */
#include <net/iw_handler.h>

#include <asm/uaccess.h>

#include "if_media.h"
#include "_ieee80211.h"
#include <osif_private.h>
#include <wlan_opts.h>
#include <ieee80211_var.h>
#include <ieee80211_ioctl.h>
#include "ieee80211_rateset.h"
#include "ieee80211_vi_dbg.h"
#if ATH_SUPPORT_IBSS_DFS
#include <ieee80211_regdmn.h>
#endif
#include "ieee80211_power_priv.h"
#include "../vendor/generic/ioctl/ioctl_vendor_generic.h"


#include "if_athvar.h"
#include "if_athproto.h"



#define __ACFG_PHYMODE_STRINGS__
#include <adf_os_types_pvt.h>
#include <acfg_drv_if.h>
#include <acfg_api_types.h>
#include <acfg_drv_dev.h>
#include <acfg_drv_cfg.h>
#include <acfg_drv_cmd.h>
#include <acfg_drv_event.h>

#include  "ieee80211_acfg.h"


unsigned long prev_jiffies = 0;

#define NETDEV_TO_VAP(_dev) (((osif_dev *)netdev_priv(_dev))->os_if)

#define IS_UP(_dev) \
    (((_dev)->flags & (IFF_RUNNING|IFF_UP)) == (IFF_RUNNING|IFF_UP))
#define IS_UP_AUTO(_vap) \
    (IS_UP((_vap)->iv_dev) && \
    (_vap)->iv_ic->ic_roaming == IEEE80211_ROAMING_AUTO)
#define RESCAN  1

#define IS_CONNECTED(osp) ((((osp)->os_opmode==IEEE80211_M_STA) || \
                        ((u_int8_t)(osp)->os_opmode == IEEE80211_M_P2P_CLIENT))? \
                        wlan_connection_sm_is_connected((osp)->sm_handle): \
                        (osp)->is_bss_started )

#define IEEE80211_MSG_IOCTL   IEEE80211_MSG_DEBUG

#define IEEE80211_BINTVAL_IWMAX       3500   /* max beacon interval */
#define IEEE80211_BINTVAL_IWMIN       40     /* min beacon interval */

enum {
    SPECIAL_PARAM_COUNTRY_ID,
    SPECIAL_PARAM_ASF_AMEM_PRINT,
    SPECIAL_PARAM_DISP_TPC,
};      


static const int legacy_rate_idx[][2] = {
    {1, 0x1b},
    {2, 0x1a},
    {5, 0x19},
    {6, 0xb},
    {9, 0xf},
    {11, 0x18},
    {12, 0xa},
    {18, 0xe},
    {24, 0x9},
    {36, 0xd},
    {48, 0x8},
    {54, 0xc},
};

static const u_int8_t ieee80211broadcastaddr[IEEE80211_ADDR_LEN] =
{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

static short int staOnlyCountryCodes[]= {
     36, /*  CRTY_AUSTRALIA */
    124, /*  CTRY_CANADA */
    410, /*  CTRY_KOREA_ROC */
    840, /*  CTRY_UNITED_STATES */
};                  


static int siwencode_wep(struct net_device *dev)
{
    osif_dev            *osifp = ath_netdev_priv(dev);
    wlan_if_t           vap    = osifp->os_if;
    int                 error  = 0;
    u_int               nmodes = 1;
    ieee80211_auth_mode modes[1];
        
    osifp->authmode = IEEE80211_AUTH_OPEN;

    modes[0] = IEEE80211_AUTH_OPEN;
    error = wlan_set_authmodes(vap, modes, nmodes);
    if (error == 0 ) {
        error = wlan_set_param(vap, IEEE80211_FEATURE_PRIVACY, 0);
        osifp->uciphers[0] = osifp->mciphers[0] = IEEE80211_CIPHER_NONE;
        osifp->u_count = osifp->m_count = 1;
    }
    
    return IS_UP(dev) ? -osif_vap_init(dev, RESCAN) : 0;
}


static int
getiwkeyix(wlan_if_t vap, const struct iw_point* erq, u_int16_t *kix)
{
    int kid;

    kid = erq->flags & IW_ENCODE_INDEX;
    if (kid < 1 || kid > IEEE80211_WEP_NKID)
    {
        kid = wlan_get_default_keyid(vap);
        if (kid == IEEE80211_KEYIX_NONE)
            kid = 0;
    }
    else
        --kid;
    if (0 <= kid && kid < IEEE80211_WEP_NKID)
    {
        *kix = kid;
        return 0;
    }
    else
        return -EINVAL;
}

static int
ieee80211_convert_mode(const char *mode)
{
#define IEEE80211_MODE_TURBO_STATIC_A   IEEE80211_MODE_MAX
#define TOUPPER(c) ((((c) > 0x60) && ((c) < 0x7b)) ? ((c) - 0x20) : (c))
    static const struct
    {
        char *name;
        int mode;
    } mappings[] = {
        /* NB: need to order longest strings first for overlaps */
        { "11AST" , IEEE80211_MODE_TURBO_STATIC_A },
        { "AUTO"  , IEEE80211_MODE_AUTO },
        { "11A"   , IEEE80211_MODE_11A },
        { "11B"   , IEEE80211_MODE_11B },
        { "11G"   , IEEE80211_MODE_11G },
        { "FH"    , IEEE80211_MODE_FH },
        { "0"     , IEEE80211_MODE_AUTO },
        { "1"     , IEEE80211_MODE_11A },
        { "2"     , IEEE80211_MODE_11B },
        { "3"     , IEEE80211_MODE_11G },
        { "4"     , IEEE80211_MODE_FH },
        { "5"     , IEEE80211_MODE_TURBO_STATIC_A },
        { "TA"      , IEEE80211_MODE_TURBO_A },
        { "TG"      , IEEE80211_MODE_TURBO_G },
        { "11NAHT20"      , IEEE80211_MODE_11NA_HT20 },
        { "11NGHT20"      , IEEE80211_MODE_11NG_HT20 },
        { "11NAHT40PLUS"  , IEEE80211_MODE_11NA_HT40PLUS },
        { "11NAHT40MINUS" , IEEE80211_MODE_11NA_HT40MINUS },
        { "11NGHT40PLUS"  , IEEE80211_MODE_11NG_HT40PLUS },
        { "11NGHT40MINUS" , IEEE80211_MODE_11NG_HT40MINUS },
        { "11NGHT40" , IEEE80211_MODE_11NG_HT40 },
        { "11NAHT40" , IEEE80211_MODE_11NA_HT40 },
        { "11ACVHT20" , IEEE80211_MODE_11AC_VHT20},
        { "11ACVHT40PLUS" , IEEE80211_MODE_11AC_VHT40PLUS},
        { "11ACVHT40MINUS" , IEEE80211_MODE_11AC_VHT40MINUS},
        { "11ACVHT40" , IEEE80211_MODE_11AC_VHT40},
        { "11ACVHT80" , IEEE80211_MODE_11AC_VHT80},
        { NULL }
    };
    int i, j;
    const char *cp;

    for (i = 0; mappings[i].name != NULL; i++) {
        cp = mappings[i].name;
        for (j = 0; j < strlen(mode) + 1; j++) {
            /* convert user-specified string to upper case */
            if (TOUPPER(mode[j]) != cp[j])
                break;
            if (cp[j] == '\0')
                return mappings[i].mode;
        }
    }
    return -1;
#undef TOUPPER
#undef IEEE80211_MODE_TURBO_STATIC_A
}


static void
acfg_convert_to_acfgprofile (struct ieee80211_profile *profile,
                                    acfg_radio_vap_info_t *acfg_profile)
{
    a_uint8_t i, kid = 0;

	strncpy(acfg_profile->radio_name, profile->radio_name, IFNAMSIZ);
	acfg_profile->chan = profile->channel;
	acfg_profile->freq = profile->freq;
	acfg_profile->country_code = profile->cc;
	memcpy(acfg_profile->radio_mac, profile->radio_mac, ACFG_MACADDR_LEN);
    for (i = 0; i < profile->num_vaps; i++) {
        strncpy(acfg_profile->vap_info[i].vap_name,
                                    profile->vap_profile[i].name,
                                    IFNAMSIZ);
		memcpy(acfg_profile->vap_info[i].vap_mac, 
				profile->vap_profile[i].vap_mac, 
				ACFG_MACADDR_LEN);
		acfg_profile->vap_info[i].phymode = 
									profile->vap_profile[i].phymode;		
		switch (profile->vap_profile[i].sec_method) {
			case IEEE80211_AUTH_NONE:
			case IEEE80211_AUTH_OPEN:
				if(profile->vap_profile[i].cipher & (1 << IEEE80211_CIPHER_WEP))

				{
					acfg_profile->vap_info[i].cipher = 
										ACFG_WLAN_PROFILE_CIPHER_METH_WEP;
					acfg_profile->vap_info[i].sec_method = 
											ACFG_WLAN_PROFILE_SEC_METH_OPEN;
				} else {
					acfg_profile->vap_info[i].cipher = 
										ACFG_WLAN_PROFILE_CIPHER_METH_NONE;
					acfg_profile->vap_info[i].sec_method = 
											ACFG_WLAN_PROFILE_SEC_METH_OPEN;
				}
				break;
			case IEEE80211_AUTH_AUTO:
				acfg_profile->vap_info[i].sec_method = 
											ACFG_WLAN_PROFILE_SEC_METH_AUTO;
				if(profile->vap_profile[i].cipher & 
						(1 << IEEE80211_CIPHER_WEP))

				{
					acfg_profile->vap_info[i].cipher = 
										ACFG_WLAN_PROFILE_CIPHER_METH_WEP;
				}
				break;
			case IEEE80211_AUTH_SHARED:
				if(profile->vap_profile[i].cipher & 
						(1 << IEEE80211_CIPHER_WEP))
				{
					acfg_profile->vap_info[i].cipher = 
										ACFG_WLAN_PROFILE_CIPHER_METH_WEP;
				}
				acfg_profile->vap_info[i].sec_method = 
											ACFG_WLAN_PROFILE_SEC_METH_SHARED;
				break;	
			case IEEE80211_AUTH_WPA:
				acfg_profile->vap_info[i].sec_method = 
											ACFG_WLAN_PROFILE_SEC_METH_WPA;
				break;
			case IEEE80211_AUTH_RSNA:
				acfg_profile->vap_info[i].sec_method = 
											ACFG_WLAN_PROFILE_SEC_METH_WPA2;
				break;
		}
		if (profile->vap_profile[i].cipher & (1 << IEEE80211_CIPHER_TKIP)) {
			acfg_profile->vap_info[i].cipher |= 
										ACFG_WLAN_PROFILE_CIPHER_METH_TKIP;
		}
		if ((profile->vap_profile[i].cipher & (1 << IEEE80211_CIPHER_AES_OCB)) 
			 ||(profile->vap_profile[i].cipher & 
											(1 <<IEEE80211_CIPHER_AES_CCM))) 
		{
			acfg_profile->vap_info[i].cipher |= 
										ACFG_WLAN_PROFILE_CIPHER_METH_AES;
		}
		for (kid = 0; kid < 4; kid++) {
			if (profile->vap_profile[i].wep_key_len[kid]) {
        		memcpy(acfg_profile->vap_info[i].wep_key,
                	    profile->vap_profile[i].wep_key[kid], 
						profile->vap_profile[i].wep_key_len[kid]);
				acfg_profile->vap_info[i].wep_key_idx = kid;
				acfg_profile->vap_info[i].wep_key_len =
									profile->vap_profile[i].wep_key_len[kid];
			}
		}
    }
    acfg_profile->num_vaps = profile->num_vaps;
}


static int
acfg_acfg2ieee(a_uint16_t param)
{
    a_uint16_t value;

    switch (param) {
        case ACFG_PARAM_VAP_SHORT_GI:
            value = IEEE80211_PARAM_SHORT_GI;
            break;
        case ACFG_PARAM_VAP_AMPDU:
            value = IEEE80211_PARAM_AMPDU;
            break;
        case ACFG_PARAM_AUTHMODE:
            value = IEEE80211_PARAM_AUTHMODE;
            break;
        case ACFG_PARAM_MACCMD:
            value = IEEE80211_PARAM_MACCMD;
            break;
        case ACFG_PARAM_BEACON_INTERVAL:
            value = IEEE80211_PARAM_BEACON_INTERVAL;
            break;
        case ACFG_PARAM_PUREG:
            value = IEEE80211_PARAM_PUREG;
            break;
        case ACFG_PARAM_WDS:
            value = IEEE80211_PARAM_WDS;
            break;
		case ACFG_PARAM_UAPSD:
            value = IEEE80211_PARAM_UAPSDINFO;
            break;
        case ACFG_PARAM_11N_RATE:
            value = IEEE80211_PARAM_11N_RATE;
            break;
        case ACFG_PARAM_11N_RETRIES:
            value = IEEE80211_PARAM_11N_RETRIES;
            break;
        case ACFG_PARAM_DBG_LVL:
            value = IEEE80211_PARAM_DBG_LVL;
            break;
        case ACFG_PARAM_STA_FORWARD:
            value = IEEE80211_PARAM_STA_FORWARD;
            break;
        case ACFG_PARAM_PUREN:
            value = IEEE80211_PARAM_PUREN;
            break;
        case ACFG_PARAM_NO_EDGE_CH:
            value = IEEE80211_PARAM_NO_EDGE_CH;
            break;
        case ACFG_PARAM_VAP_IND:
            value = IEEE80211_PARAM_VAP_IND;
            break;
        case ACFG_PARAM_WPS:
            value = IEEE80211_PARAM_WPS;
            break;
        case ACFG_PARAM_EXTAP:
            value = IEEE80211_PARAM_EXTAP;
            break;
        case ACFG_PARAM_TDLS_ENABLE:
            value = IEEE80211_PARAM_TDLS_ENABLE;
            break;
        case ACFG_PARAM_SET_TDLS_RMAC:
            value = IEEE80211_PARAM_SET_TDLS_RMAC;
            break;
        case ACFG_PARAM_CLR_TDLS_RMAC:
            value = IEEE80211_PARAM_CLR_TDLS_RMAC;
            break;
        case ACFG_PARAM_TDLS_MACADDR1:
            value = IEEE80211_PARAM_TDLS_MACADDR1;
            break;
        case ACFG_PARAM_TDLS_MACADDR2:
            value = IEEE80211_PARAM_TDLS_MACADDR2;
            break;
        case ACFG_PARAM_SW_WOW:
            value = IEEE80211_PARAM_SW_WOW;
            break;
        case ACFG_PARAM_WMMPARAMS_CWMIN:
            value = IEEE80211_WMMPARAMS_CWMIN;
            break;
        case ACFG_PARAM_WMMPARAMS_CWMAX:
            value = IEEE80211_WMMPARAMS_CWMAX;
            break;
        case ACFG_PARAM_WMMPARAMS_AIFS:
            value = IEEE80211_WMMPARAMS_AIFS;
            break;
        case ACFG_PARAM_WMMPARAMS_TXOPLIMIT:
            value = IEEE80211_WMMPARAMS_TXOPLIMIT;
            break;
        case ACFG_PARAM_WMMPARAMS_ACM:
            value = IEEE80211_WMMPARAMS_ACM;
            break;
        case ACFG_PARAM_WMMPARAMS_NOACKPOLICY:
            value = IEEE80211_WMMPARAMS_NOACKPOLICY;
            break;
		 case ACFG_PARAM_HIDE_SSID:
            value = IEEE80211_PARAM_HIDESSID;
            break;
		case ACFG_PARAM_DOTH:
            value = IEEE80211_PARAM_DOTH;
            break;
        case ACFG_PARAM_COEXT_DISABLE:
            value = IEEE80211_PARAM_COEXT_DISABLE;
            break;
        case ACFG_PARAM_APBRIDGE:
            value = IEEE80211_PARAM_APBRIDGE;
            break;
        case ACFG_PARAM_AMPDU:
            value = IEEE80211_PARAM_AMPDU;
            break;
        case ACFG_PARAM_ROAMING:
            value = IEEE80211_PARAM_ROAMING;
            break;
        case ACFG_PARAM_AUTO_ASSOC:
            value = IEEE80211_PARAM_AUTO_ASSOC;
            break;
		case ACFG_PARAM_UCASTCIPHERS:
			value = IEEE80211_PARAM_UCASTCIPHERS;
			break;
        case ACFG_PARAM_CHANBW:
            value = IEEE80211_PARAM_CHANBW;
            break;
        case ACFG_PARAM_BURST:
            value = IEEE80211_PARAM_BURST;
            break;
        case ACFG_PARAM_AMSDU:
            value = IEEE80211_PARAM_AMSDU;
            break;                      
        default:
            value = param;
            break;
    }
    return value;
}




static int 
acfg_mode2acfgmode(a_uint32_t param)
{
	return param;
#if not_yet
    switch(param) {
        case IW_MODE_ADHOC:
            return ACFG_OPMODE_IBSS;
            break;
        case IW_MODE_INFRA:
            return ACFG_OPMODE_STA;
            break;
        case IW_MODE_MASTER:
            return ACFG_OPMODE_HOSTAP;
            break;
        case IW_MODE_REPEAT:
            return ACFG_OPMODE_WDS;
            break;
        case IW_MODE_MONITOR:
            return ACFG_OPMODE_MONITOR;
            break;
        default:
            return -1;
        break;
    }
#endif
}


int
acfg_vap_create(void *ctx, a_uint16_t cmdid,
               a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    struct ieee80211_clone_params cp;
    acfg_vapinfo_t    *ptr;
    acfg_os_req_t   *req = NULL;
    struct ifreq ifr;
    int status;
	struct ath_softc_net80211 *scn = ath_netdev_priv(dev);


    req = (acfg_os_req_t *) buffer;
    ptr     = &req->data.vap_info;

    memset(&ifr, 0, sizeof(ifr));
    memset(&cp , 0, sizeof(cp));

    cp.icp_opmode = ptr->icp_opmode;
    cp.icp_flags  = ptr->icp_flags;
    memcpy(&cp.icp_name[0], &ptr->icp_name[0], ACFG_MAX_IFNAME);
    ifr.ifr_data = (void *) &cp;

    status  = osif_ioctl_create_vap(dev, &ifr, cp, scn->sc_osdev);

    return status;

}

int
acfg_vap_delete(void *ctx, a_uint16_t cmdid,
               a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_vapinfo_t    *ptr;
    acfg_os_req_t   *req = NULL;
    struct ifreq ifr;
    int status = -1;

    req = (acfg_os_req_t *) buffer;
    ptr     = &req->data.vap_info;

    memset(&ifr, 0, sizeof(ifr));
    if (dev) {
        status = osif_ioctl_delete_vap(dev);
    }

    return status;
}


int
acfg_is_offload_vap(void *ctx, a_uint16_t cmdid,
               a_uint8_t *buffer, a_int32_t Length)
{

    int status = -1;

    /* This function can be invoked on a VAP on 
     * a local radio only. We return false by default.
    */
    return status;
}


int acfg_set_vap_vendor_param(void *ctx, a_uint16_t cmdif,
               a_uint8_t  *buffer, a_uint32_t Length)
{
    int status = 0;
    struct net_device *dev = (struct net_device *) ctx;
    acfg_os_req_t   *req = NULL;

    req = (acfg_os_req_t *)buffer;
    status = osif_set_vap_vendor_param(dev, &req->data.vendor_param_req);

    return status;
}

int acfg_get_vap_vendor_param(void *ctx, a_uint16_t cmdif,
               a_uint8_t  *buffer, a_uint32_t Length)
{
    int status = 0;
    struct net_device *dev = (struct net_device *) ctx;
    acfg_os_req_t   *req = NULL;

    req = (acfg_os_req_t *)buffer;
    status = osif_get_vap_vendor_param(dev, &req->data.vendor_param_req);

    return status;
}

int
acfg_set_ssid(void *ctx, a_uint16_t cmdid,
             a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_ssid_t        *ptr;
    acfg_os_req_t   *req = NULL;
	osif_dev *osifp = ath_netdev_priv(dev);
	wlan_if_t vap = osifp->os_if;
    enum ieee80211_opmode opmode = wlan_vap_get_opmode(vap);
    ieee80211_ssid   tmpssid;
    
	req = (acfg_os_req_t *) buffer;
    ptr     = &req->data.ssid;

	 if (opmode == IEEE80211_M_WDS)
        return -EOPNOTSUPP;

	 OS_MEMZERO(&tmpssid, sizeof(ieee80211_ssid));

	if (ptr->len > IEEE80211_NWID_LEN)
    	ptr->len = IEEE80211_NWID_LEN;
    tmpssid.len = ptr->len;
    OS_MEMCPY(tmpssid.ssid, ptr->name, ptr->len);
	if (ptr->len > 0 &&
            tmpssid.ssid[ptr->len-1] == '\0')
        tmpssid.len--;
#ifdef ATH_SUPERG_XR
    if (vap->iv_xrvap != NULL && !(vap->iv_flags & IEEE80211_F_XR))
    {
        copy_des_ssid(vap->iv_xrvap, vap);
    }
#endif
    wlan_set_desired_ssidlist(vap,1,&tmpssid);

    printk(" \n DES SSID SET=%s \n", tmpssid.ssid);

#ifdef ATH_SUPPORT_P2P
        /* For P2P supplicant we do not want start connnection as soon as ssid is set */
        /* The difference in behavior between non p2p supplicant and p2p supplicant need to be fixed */
        /* see EV 73753 for more details */
    if (((u_int8_t)osifp->os_opmode == IEEE80211_M_P2P_CLIENT
         || osifp->os_opmode == IEEE80211_M_STA
         || (u_int8_t)osifp->os_opmode == IEEE80211_M_P2P_GO) && !vap->auto_assoc)
        return 0;
#endif

    return (IS_UP(dev) && (vap->iv_ic->ic_roaming != IEEE80211_ROAMING_MANUAL)) ? -osif_vap_init(dev, RESCAN) : 0;

}


int
acfg_get_ssid(void *ctx, a_uint16_t cmdid,
             a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_ssid_t        *ptr;
    acfg_os_req_t   *req = NULL;
	osif_dev *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
    enum ieee80211_opmode opmode = wlan_vap_get_opmode(vap);
    ieee80211_ssid ssidlist[1];
    int des_nssid;


    req = (acfg_os_req_t *) buffer;
    ptr     = &req->data.ssid;

	if (opmode == IEEE80211_M_WDS)
        return -EOPNOTSUPP;
    des_nssid = wlan_get_desired_ssidlist(vap, ssidlist, 1);
	if (des_nssid > 0)
    {
		ptr->len = ssidlist[0].len;	
		OS_MEMCPY(ptr->name, ssidlist[0].ssid, ssidlist[0].len);
	} else {
		if (opmode == IEEE80211_M_HOSTAP) ptr->len = 0;
        else
        {
            wlan_get_bss_essid(vap, ssidlist);
            ptr->len = ssidlist[0].len;
            OS_MEMCPY(ptr->name, ssidlist[0].ssid, ssidlist[0].len);
        }
	}
    ptr->name[ptr->len] = '\0';
	ptr->len = ptr->len + 1;
	return 0;
}


int
acfg_set_chan(void *ctx, a_uint16_t cmdid,
             a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
	osif_dev *osnetdev = ath_netdev_priv(dev);
    wlan_if_t vap = osnetdev->os_if;
    acfg_os_req_t   *req = NULL;
    int i;
    int retval = 0;
    
	req = (acfg_os_req_t *) buffer;

	i = req->data.chan;
	if (i == 0) {
		i = IEEE80211_CHAN_ANY;
	}
	if (vap->iv_opmode == IEEE80211_M_IBSS) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s : IBSS desired channel(%d)\n",
                                                     __func__, i);
        return wlan_set_desired_ibsschan(vap, i);
    } else if(vap->iv_opmode == IEEE80211_M_HOSTAP) {
        wlan_mlme_stop_bss(vap, 0);
        retval = wlan_set_channel(vap, i);
        if(!retval) {
            return IS_UP(dev) ? -osif_vap_init(dev, RESCAN) : 0;
        }
        return retval;
    } else {
        retval = wlan_set_channel(vap, i);
        return retval;
    }
}

int 
acfg_get_chan(void *ctx, a_uint16_t cmdid,
             a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
	osif_dev *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
    wlan_chan_t chan;
	acfg_freq_t        *ptr;
    acfg_os_req_t   *req = NULL;

	req = (acfg_os_req_t *) buffer;
    ptr     = &req->data.freq;

	if (dev->flags & (IFF_UP | IFF_RUNNING)) {
        chan = wlan_get_bss_channel(vap);
    } else {
        chan = wlan_get_current_channel(vap, true);
    }
    if (chan != IEEE80211_CHAN_ANYC) {
        ptr->m = chan->ic_freq * 100000;
    } else {
        ptr->m = 0;
    }
    ptr->e = 1;

    return 0;
}


int
acfg_get_opmode(void *ctx, a_uint16_t cmdid,
               a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_opmode_t        *ptr;
    acfg_os_req_t   *req = NULL;
	struct ifmediareq imr;
    osif_dev *osnetdev = ath_netdev_priv(dev);
	uint32_t mode;

    req = (acfg_os_req_t *) buffer;
    ptr     = &req->data.opmode;
	memset(&imr, 0, sizeof(imr));
    osnetdev->os_media.ifm_status(dev, &imr);
	if (imr.ifm_active & IFM_IEEE80211_HOSTAP)
        mode = IW_MODE_MASTER;
#if WIRELESS_EXT >= 15
    else if (imr.ifm_active & IFM_IEEE80211_MONITOR)
        mode = IW_MODE_MONITOR;
#endif
    else if (imr.ifm_active & IFM_IEEE80211_ADHOC)
        mode = IW_MODE_ADHOC;
    else if (imr.ifm_active & IFM_IEEE80211_WDS)
        mode = IW_MODE_REPEAT;
    else
        mode = IW_MODE_INFRA;

	*ptr = acfg_mode2acfgmode(mode);
    return 0;
}

int 
acfg_set_freq(void *ctx, a_uint16_t cmdid,
            a_uint8_t *buffer, a_int32_t Length)
{
	return 0;
}


int 
acfg_get_freq(void *ctx, a_uint16_t cmdid,
            a_uint8_t *buffer, a_int32_t Length)
{
	return 0;
}

int 
acfg_set_reg (void *ctx, a_uint16_t cmdid,
            a_uint8_t *buffer, a_int32_t Length)
{
	return 0;
}


int 
acfg_get_reg (void *ctx, a_uint16_t cmdid,
            a_uint8_t *buffer, a_int32_t Length)
{
	return 0;
}

int
acfg_set_rts(void *ctx, a_uint16_t cmdid,
            a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_rts_t        *ptr;
    acfg_os_req_t   *req = NULL;
	osif_dev *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
    u_int32_t val, curval;

    req = (acfg_os_req_t *) buffer;
    ptr     = &req->data.rts;
	
	if (ptr->flags & ACFG_RTS_DISABLED) {
		val = IEEE80211_RTS_MAX;
	} else if (ptr->flags & ACFG_RTS_FIXED) {
		if (IEEE80211_RTS_MIN <= ptr->val &&
    	    ptr->val <= IEEE80211_RTS_MAX) 
		{
    	    val = ptr->val;
		} else {
			return -EINVAL;
		}
		curval = wlan_get_param(vap, IEEE80211_RTS_THRESHOLD);
	    if (val != curval)
    	{
        	wlan_set_param(vap, IEEE80211_RTS_THRESHOLD, ptr->val);
	        if (IS_UP(dev))
    	        return osif_vap_init(dev, RESCAN);
	    }
	}
    return 0;
}


int
acfg_get_rts(void *ctx, a_uint16_t cmdid,
            a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_rts_t        *ptr;
    acfg_os_req_t   *req = NULL;
	osif_dev *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;

    req = (acfg_os_req_t *) buffer;
    ptr     = &req->data.rts;

	ptr->val = wlan_get_param(vap, IEEE80211_RTS_THRESHOLD);
	
	if (ptr->val == IEEE80211_RTS_MAX) {
		 ptr->flags |= ACFG_RTS_DISABLED;
	}	
	ptr->flags |= ACFG_RTS_FIXED;
    return 0;
}


int
acfg_set_frag(void *ctx, a_uint16_t cmdid,
             a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_frag_t        *ptr;
    acfg_os_req_t   *req = NULL;
	osif_dev *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
    u_int32_t val, curval;

    req = (acfg_os_req_t *) buffer;
    ptr     = &req->data.frag;
	
	if (ptr->flags & ACFG_FRAG_DISABLED) {
		val = IEEE80211_FRAGMT_THRESHOLD_MAX;
	} else if (ptr->flags & ACFG_FRAG_FIXED) {
		if (ptr->val < 256 || ptr->val > IEEE80211_FRAGMT_THRESHOLD_MAX)
    	    return -EINVAL;
	    else
    	    val = (ptr->val & ~0x1);
		if( wlan_get_desired_phymode(vap) < IEEE80211_MODE_11NA_HT20 )
    	{
        	curval = wlan_get_param(vap, IEEE80211_FRAG_THRESHOLD);
	        if (val != curval)
    	    {
        	    wlan_set_param(vap, IEEE80211_FRAG_THRESHOLD, val);
            	if (IS_UP(dev))
	                return -osif_vap_init(dev, RESCAN);
    	    }
	    } else {
    	    printk("WARNING: Fragmentation with HT mode NOT ALLOWED!!\n");
        	return -EINVAL;
	    }
	}
    return 0;
}


int
acfg_get_frag(void *ctx, a_uint16_t cmdid,
             a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_frag_t        *ptr;
    acfg_os_req_t   *req = NULL;
	osif_dev *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;


    req = (acfg_os_req_t *) buffer;
    ptr     = &req->data.frag;

	ptr->val = wlan_get_param(vap, IEEE80211_FRAG_THRESHOLD);
	if (ptr->val == IEEE80211_FRAGMT_THRESHOLD_MAX) {
		ptr->flags |= ACFG_FRAG_DISABLED;
	}
    ptr->flags |= ACFG_FRAG_FIXED;

    return 0;
}


int
acfg_set_txpow(void *ctx, a_uint16_t cmdid,
              a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_txpow_t        *ptr;
    acfg_os_req_t   *req = NULL;
	osif_dev *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
    int fixed, txpow, retv = EOK;

    req = (acfg_os_req_t *) buffer;
    ptr     = &req->data.txpow;

	wlan_get_txpow(vap, &txpow, &fixed);
	if (ptr->flags & ACFG_TXPOW_DISABLED) {
		 return -EOPNOTSUPP;
	}
	if (ptr->flags & ACFG_TXPOW_FIXED) {
		retv = wlan_set_txpow(vap, ptr->val);		
	} else {
		 retv = wlan_set_txpow(vap, 0);
	}
	return (retv != EOK) ? -EOPNOTSUPP : EOK;

}


int
acfg_get_txpow(void *ctx, a_uint16_t cmdid,
              a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_txpow_t        *ptr;
    acfg_os_req_t   *req = NULL;
	osif_dev *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
    int txpow, fixed;

    req = (acfg_os_req_t *) buffer;
    ptr     = &req->data.txpow;

	wlan_get_txpow(vap, &txpow, &fixed);
    ptr->val = txpow;
	if (fixed) {
		ptr->flags |= ACFG_TXPOW_FIXED;
	}
	if (fixed && txpow == 0) {
		ptr->flags |= ACFG_TXPOW_DISABLED;
	}

    return 0;
}


int
acfg_set_ap(void *ctx, a_uint16_t cmdid,
               a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_macaddr_t    *ptr;
    acfg_os_req_t *req;
	osif_dev *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
    u_int8_t des_bssid[IEEE80211_ADDR_LEN];
    u_int8_t zero_bssid[] = { 0,0,0,0,0,0 };

    req = (acfg_os_req_t *) buffer;
    ptr = &req->data.macaddr;

	if (wlan_vap_get_opmode(vap) != IEEE80211_M_STA &&
        (u_int8_t)osifp->os_opmode != IEEE80211_M_P2P_DEVICE &&
        (u_int8_t)osifp->os_opmode != IEEE80211_M_P2P_CLIENT) {
        return -EINVAL;
    }

	IEEE80211_ADDR_COPY(des_bssid, ptr->addr);

    if (IEEE80211_ADDR_EQ(des_bssid, zero_bssid)) {
        wlan_aplist_init(vap);
    } else {
        wlan_aplist_set_desired_bssidlist(vap, 1, &des_bssid);
    }
	if (IS_UP(dev))
        return osif_vap_init(dev, RESCAN);
    return 0;
}


int
acfg_get_ap(void *ctx, a_uint16_t cmdid,
           a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_macaddr_t        *ptr;
    acfg_os_req_t   *req = NULL;
	osif_dev *osnetdev = ath_netdev_priv(dev);
    wlan_if_t vap = osnetdev->os_if;
    u_int8_t bssid[IEEE80211_ADDR_LEN];

    req = (acfg_os_req_t *) buffer;
    ptr     = &req->data.macaddr;

#ifdef notyet
    if (vap->iv_flags & IEEE80211_F_DESBSSID)
        IEEE80211_ADDR_COPY(ptr->addr, vap->iv_des_bssid);
    else
#endif
    {
        static const u_int8_t zero_bssid[IEEE80211_ADDR_LEN];
        if(osnetdev->is_up) {
            wlan_vap_get_bssid(vap, bssid);
            IEEE80211_ADDR_COPY(ptr->addr, bssid);
        } else {
            IEEE80211_ADDR_COPY(ptr->addr, zero_bssid);
        }
    }
    return 0;
}


int
acfg_set_vap_param(void *ctx, a_uint16_t cmdid,
                  a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_param_req_t        *ptr;
    acfg_os_req_t   *req = NULL;
	osif_dev  *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
    wlan_dev_t ic = wlan_vap_get_devhandle(vap);
	int param, value;
	int retv = 0;
    int error = 0;
    unsigned char *extra;

    req = (acfg_os_req_t *) buffer;
    ptr  = &req->data.param_req;

	param = acfg_acfg2ieee(ptr->param);
	value = ptr->val;
    extra = (unsigned char *)&value;

    switch (param)
    {
    case IEEE80211_PARAM_MAXSTA: //set max stations allowed
        if (value > IEEE80211_AID_DEF || value < 1) { // At least one station can associate with.
            return EINVAL;
        }
        if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
            u_int16_t old_max_aid = vap->iv_max_aid;
            u_int16_t old_len = howmany(vap->iv_max_aid, 32) * sizeof(u_int32_t);
            if (value < vap->iv_sta_assoc) {
                printk("%d station associated with vap%d! refuse this request\n", 
                            vap->iv_sta_assoc, vap->iv_unit);
                return EINVAL;
            }

            vap->iv_max_aid = value + 1;
            /* The interface is up, we may need to reallocation bitmap(tim, aid) */
            if (IS_UP(dev)) {
                error = ieee80211_power_alloc_tim_bitmap(vap);
                if(!error)
                	error = wlan_node_alloc_aid_bitmap(vap, old_len);
            }
            if(!error)
            	printk("Setting Max Stations:%d\n", value);
           	else {
          		printk("Setting Max Stations fail\n");
          		vap->iv_max_aid = old_max_aid;
          		return ENOMEM;
          	} 		 	
        }
        else {
            printk("This command only support on Host AP mode.\n");
        }
        break;
    case IEEE80211_PARAM_AUTO_ASSOC:
        wlan_set_param(vap, IEEE80211_AUTO_ASSOC, value);
        break;
    case IEEE80211_PARAM_VAP_COUNTRY_IE:
        wlan_set_param(vap, IEEE80211_FEATURE_COUNTRY_IE, value);
        break;
    case IEEE80211_PARAM_VAP_DOTH:
        wlan_set_param(vap, IEEE80211_FEATURE_DOTH, value);
        break;
    case IEEE80211_PARAM_HT40_INTOLERANT:
        wlan_set_param(vap, IEEE80211_HT40_INTOLERANT, value);
        break;

    case IEEE80211_PARAM_CHWIDTH:
        wlan_set_param(vap, IEEE80211_CHWIDTH, value);
        break;

    case IEEE80211_PARAM_CHEXTOFFSET:
        wlan_set_param(vap, IEEE80211_CHEXTOFFSET, value);
        break;
#ifdef ATH_SUPPORT_QUICK_KICKOUT
    case IEEE80211_PARAM_STA_QUICKKICKOUT:
        wlan_set_param(vap, IEEE80211_STA_QUICKKICKOUT, value);
        break;
#endif
    case IEEE80211_PARAM_CHSCANINIT:
        wlan_set_param(vap, IEEE80211_CHSCANINIT, value);
        break;

    case IEEE80211_PARAM_COEXT_DISABLE:
        osifp->is_up = 0;
        if (value)
        {
            ic->ic_flags |= IEEE80211_F_COEXT_DISABLE;
        }
        else
        {
            ic->ic_flags &= ~IEEE80211_F_COEXT_DISABLE;
        }
        retv = ENETRESET;
        break;

    case IEEE80211_PARAM_AUTHMODE:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "set IEEE80211_IOC_AUTHMODE to %s\n",
        (value == IEEE80211_AUTH_WPA) ? "WPA" : (value == IEEE80211_AUTH_8021X) ? "802.1x" :
        (value == IEEE80211_AUTH_OPEN) ? "open" : (value == IEEE80211_AUTH_SHARED) ? "shared" :
        (value == IEEE80211_AUTH_AUTO) ? "auto" : "unknown" );
        osifp->authmode = value;

        if (value != IEEE80211_AUTH_WPA) {
            ieee80211_auth_mode modes[1];
            u_int nmodes=1;
            modes[0] = value;
            error = wlan_set_authmodes(vap,modes,nmodes);
            if (error == 0 ) {
                if ((value == IEEE80211_AUTH_OPEN) || (value == IEEE80211_AUTH_SHARED)) {
                    error = wlan_set_param(vap,IEEE80211_FEATURE_PRIVACY, 0);
                    osifp->uciphers[0] = osifp->mciphers[0] = IEEE80211_CIPHER_NONE;
                    osifp->u_count = osifp->m_count = 1;
                } else {
                    error = wlan_set_param(vap,IEEE80211_FEATURE_PRIVACY, 1);
                }
            }
        }
        /*
        * set_auth_mode will reset the ucast and mcast cipher set to defaults,
        * we will reset them from our cached values for non-open mode.
        */
        if ((value != IEEE80211_AUTH_OPEN) && (value != IEEE80211_AUTH_SHARED) 
                && (value != IEEE80211_AUTH_AUTO)) 
        {
            if (osifp->m_count)
                error = wlan_set_mcast_ciphers(vap,osifp->mciphers,osifp->m_count);
            if (osifp->u_count)
                error = wlan_set_ucast_ciphers(vap,osifp->uciphers,osifp->u_count);
        } 

#ifdef ATH_SUPPORT_P2P
        /* For P2P supplicant we do not want start connnection as soon as auth mode is set */
        /* The difference in behavior between non p2p supplicant and p2p supplicant need to be fixed */
        /* see EV 73753 for more details */
        if (error == 0 && (u_int8_t)osifp->os_opmode != IEEE80211_M_P2P_CLIENT && 
                osifp->os_opmode != IEEE80211_M_STA) {
            retv = ENETRESET;
        }
#else
        if (error == 0 )
	    {
            retv = ENETRESET;
        }
#endif /* ATH_SUPPORT_P2P */
        else {
            retv = error;
        }
        break;
    case IEEE80211_PARAM_MCASTKEYLEN:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "set IEEE80211_IOC_MCASTKEYLEN to %d\n", value);
        if (!(0 < value && value < IEEE80211_KEYBUF_SIZE)) {
            error = EINVAL;
            break;
        }
        error = wlan_set_rsn_cipher_param(vap,IEEE80211_MCAST_CIPHER_LEN,value);
        retv = error;
        break;
    case IEEE80211_PARAM_UCASTCIPHERS:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "set IEEE80211_IOC_UCASTCIPHERS (0x%x) %s %s %s %s %s %s\n",
                value, (value & 1<<IEEE80211_CIPHER_WEP) ? "WEP" : "",
                (value & 1<<IEEE80211_CIPHER_TKIP) ? "TKIP" : "",
                (value & 1<<IEEE80211_CIPHER_AES_OCB) ? "AES-OCB" : "",
                (value & 1<<IEEE80211_CIPHER_AES_CCM) ? "AES-CCMP" : "",
                (value & 1<<IEEE80211_CIPHER_CKIP) ? "CKIP" : "",
                (value & 1<<IEEE80211_CIPHER_NONE) ? "NONE" : "");
        {
            int count=0;
            if (value & 1<<IEEE80211_CIPHER_WEP)
                osifp->uciphers[count++] = IEEE80211_CIPHER_WEP;
            if (value & 1<<IEEE80211_CIPHER_TKIP)
                osifp->uciphers[count++] = IEEE80211_CIPHER_TKIP;
            if (value & 1<<IEEE80211_CIPHER_AES_CCM)
                osifp->uciphers[count++] = IEEE80211_CIPHER_AES_CCM;
            if (value & 1<<IEEE80211_CIPHER_CKIP)
                osifp->uciphers[count++] = IEEE80211_CIPHER_CKIP;
            if (value & 1<<IEEE80211_CIPHER_NONE)
                osifp->uciphers[count++] = IEEE80211_CIPHER_NONE;
            error = wlan_set_ucast_ciphers(vap,osifp->uciphers,count);
            if (error == 0) {
                error = ENETRESET;
            }
            else {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s Warning: wlan_set_ucast_cipher failed. cache the ucast cipher\n", __func__);
                error=0;
            }
            osifp->u_count=count;


        }
        retv = error;
        break;
    case IEEE80211_PARAM_UCASTCIPHER:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "set IEEE80211_IOC_UCASTCIPHER to %s\n",
                (value == IEEE80211_CIPHER_WEP) ? "WEP" :
                (value == IEEE80211_CIPHER_TKIP) ? "TKIP" :
                (value == IEEE80211_CIPHER_AES_OCB) ? "AES OCB" :
                (value == IEEE80211_CIPHER_AES_CCM) ? "AES CCM" :
                (value == IEEE80211_CIPHER_CKIP) ? "CKIP" :
                (value == IEEE80211_CIPHER_NONE) ? "NONE" : "unknown");
        {
            ieee80211_cipher_type ctypes[1];
            ctypes[0] = (ieee80211_cipher_type) value;
            error = wlan_set_ucast_ciphers(vap,ctypes,1);
            /* save the ucast cipher info */
            osifp->uciphers[0] = ctypes[0];
            osifp->u_count=1;
            if (error == 0) {
                retv = ENETRESET;
            }
            else {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s Warning: wlan_set_ucast_cipher failed. cache the ucast cipher\n", __func__);
                error=0;
            }
        }
        retv = error;
        break;
    case IEEE80211_IOC_MCASTCIPHER:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "set IEEE80211_IOC_MCASTCIPHER to %s\n",
                        (value == IEEE80211_CIPHER_WEP) ? "WEP" :
                        (value == IEEE80211_CIPHER_TKIP) ? "TKIP" :
                        (value == IEEE80211_CIPHER_AES_OCB) ? "AES OCB" :
                        (value == IEEE80211_CIPHER_AES_CCM) ? "AES CCM" :
                        (value == IEEE80211_CIPHER_CKIP) ? "CKIP" :
                        (value == IEEE80211_CIPHER_NONE) ? "NONE" : "unknown");
        {
            ieee80211_cipher_type ctypes[1];
            ctypes[0] = (ieee80211_cipher_type) value;
            error = wlan_set_mcast_ciphers(vap, ctypes, 1);
            /* save the mcast cipher info */
            osifp->mciphers[0] = ctypes[0];
            osifp->m_count=1;
            if (error) {
                /*
                * ignore the error for now.
                * both the ucast and mcast ciphers
                * are set again when auth mode is set.
                */
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,"%s", "Warning: wlan_set_mcast_cipher failed. cache the mcast cipher  \n");
                error=0;
            }
        }
        retv = error;
        break;
    case IEEE80211_PARAM_UCASTKEYLEN:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "set IEEE80211_IOC_UCASTKEYLEN to %d\n", value);
        if (!(0 < value && value < IEEE80211_KEYBUF_SIZE)) {
            error = EINVAL;
            break;
        }
        error = wlan_set_rsn_cipher_param(vap,IEEE80211_UCAST_CIPHER_LEN,value);
        retv = error;
        break;
    case IEEE80211_PARAM_PRIVACY:
        retv = wlan_set_param(vap,IEEE80211_FEATURE_PRIVACY,value);
        break;
    case IEEE80211_PARAM_COUNTERMEASURES:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_COUNTER_MEASURES, value);
        break;
    case IEEE80211_PARAM_HIDESSID:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_HIDE_SSID, value);
        if (retv == EOK) {
            retv = ENETRESET;
        }
        break;
    case IEEE80211_PARAM_APBRIDGE:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_APBRIDGE, value);
        break;
    case IEEE80211_PARAM_KEYMGTALGS:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "set IEEE80211_IOC_KEYMGTALGS (0x%x) %s %s\n",
        value, (value & WPA_ASE_8021X_UNSPEC) ? "802.1x Unspecified" : "",
        (value & WPA_ASE_8021X_PSK) ? "802.1x PSK" : "");
        retv = wlan_set_rsn_cipher_param(vap,IEEE80211_KEYMGT_ALGS,value);
        break;
    case IEEE80211_PARAM_RSNCAPS:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "set IEEE80211_IOC_RSNCAPS to 0x%x\n", value);
        error = wlan_set_rsn_cipher_param(vap,IEEE80211_RSN_CAPS,value);
        retv = error;
		if (value & RSN_CAP_MFP_ENABLED) {
			wlan_crypto_set_hwmfpQos(vap, 1);
		}
        break;
    case IEEE80211_PARAM_WPA:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "set IEEE80211_IOC_WPA to %s\n",
        (value == 1) ? "WPA" : (value == 2) ? "RSN" :
        (value == 3) ? "WPA and RSN" : (value == 0)? "off" : "unknown");
        if (value > 3) {
            error = EINVAL;
            break;
        } else {
            ieee80211_auth_mode modes[2];
            u_int nmodes=1;
            if (osifp->os_opmode == IEEE80211_M_STA ||
                (u_int8_t)osifp->os_opmode == IEEE80211_M_P2P_CLIENT) {
                error = wlan_set_rsn_cipher_param(vap,IEEE80211_KEYMGT_ALGS,WPA_ASE_8021X_PSK);
                if (!error) {
                    if ((value == 3) || (value == 2)) { /* Mixed mode or WPA2 */
                        modes[0] = IEEE80211_AUTH_RSNA;
                    } else { /* WPA mode */
                        modes[0] = IEEE80211_AUTH_WPA;
                    }
                }
                /* set supported cipher to TKIP and CCM
                * to allow WPA-AES, WPA2-TKIP: and MIXED mode
                */
                osifp->u_count = 2;
                osifp->uciphers[0] = IEEE80211_CIPHER_TKIP;
                osifp->uciphers[1] = IEEE80211_CIPHER_AES_CCM;
                osifp->m_count = 2;
                osifp->mciphers[0] = IEEE80211_CIPHER_TKIP;
                osifp->mciphers[1] = IEEE80211_CIPHER_AES_CCM;
            }
            else {
                if (value == 3) {
                    nmodes = 2;
                    modes[0] = IEEE80211_AUTH_WPA;
                    modes[1] = IEEE80211_AUTH_RSNA;
                } else if (value == 2) {
                    modes[0] = IEEE80211_AUTH_RSNA;
                } else {
                    modes[0] = IEEE80211_AUTH_WPA;
                }
            }
            error = wlan_set_authmodes(vap,modes,nmodes);
            /*
            * set_auth_mode will reset the ucast and mcast cipher set to defaults,
            * we will reset them from our cached values.
            */
            if (osifp->m_count)
                error = wlan_set_mcast_ciphers(vap,osifp->mciphers,osifp->m_count);
            if (osifp->u_count)
                error = wlan_set_ucast_ciphers(vap,osifp->uciphers,osifp->u_count);
        }
        retv = error;
        break;

    case IEEE80211_PARAM_CLR_APPOPT_IE:
        retv = wlan_set_clr_appopt_ie(vap);
        break;

    /*
    ** The setting of the manual rate table parameters and the retries are moved
    ** to here, since they really don't belong in iwconfig
    */

    case IEEE80211_PARAM_11N_RATE:
        retv = wlan_set_param(vap, IEEE80211_FIXED_RATE, value);
        break;

    case IEEE80211_PARAM_11N_RETRIES:
        if (value)
            retv = wlan_set_param(vap, IEEE80211_FIXED_RETRIES, value);
        break;
    case IEEE80211_PARAM_SHORT_GI :
        retv = wlan_set_param(vap, IEEE80211_SHORT_GI, value);
        if (retv == 0)
        retv = ENETRESET;
        break;
    case IEEE80211_PARAM_DBG_LVL:
        retv = wlan_set_debug_flags(vap, value);
        break;
#if UMAC_SUPPORT_IBSS
    case IEEE80211_PARAM_IBSS_CREATE_DISABLE:
        if (osifp->os_opmode != IEEE80211_M_IBSS) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,
                              "Can not be used in mode %d\n", osifp->os_opmode);
            return -EINVAL;
        }
        osifp->disable_ibss_create = !!value;
        break;
#endif
	case IEEE80211_PARAM_WEATHER_RADAR_CHANNEL:
        retv = wlan_set_param(vap, IEEE80211_WEATHER_RADAR, value);
        break;
    case IEEE80211_PARAM_WEP_KEYCACHE:
        retv = wlan_set_param(vap, IEEE80211_WEP_KEYCACHE, value);
        break;

    case IEEE80211_PARAM_BEACON_INTERVAL:
        if (value > IEEE80211_BINTVAL_IWMAX || value < IEEE80211_BINTVAL_IWMIN) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,
                              "BEACON_INTERVAL should be within %d to %d\n",
                              IEEE80211_BINTVAL_IWMIN,
                              IEEE80211_BINTVAL_IWMAX);
            return -EINVAL;
        }
        retv = wlan_set_param(vap, IEEE80211_BEACON_INTVAL, value);
        if (retv == EOK) {
            //retv = ENETRESET;
            wlan_if_t tmpvap;

            TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
                struct net_device *tmpdev = ((osif_dev *)tmpvap->iv_ifp)->netdev;
                retv = IS_UP(tmpdev) ? -osif_vap_init(tmpdev, RESCAN) : 0;
            }
        }
        break;
#if ATH_SUPPORT_AP_WDS_COMBO
    case IEEE80211_PARAM_NO_BEACON:
        retv = wlan_set_param(vap, IEEE80211_NO_BEACON, value);
        break;
#endif
    case IEEE80211_PARAM_PUREG:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_PUREG, value);
        /* NB: reset only if we're operating on an 11g channel */
        if (retv == 0) {
            wlan_chan_t chan = wlan_get_bss_channel(vap);
            if (chan != IEEE80211_CHAN_ANYC &&
                (IEEE80211_IS_CHAN_ANYG(chan) ||
                IEEE80211_IS_CHAN_11NG(chan)))
                retv = ENETRESET;
        }
        break;
    case IEEE80211_PARAM_PUREN:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_PURE11N, value);
        /* Reset only if we're operating on a 11ng channel */
        if (retv == 0) {
            wlan_chan_t chan = wlan_get_bss_channel(vap);
            if (chan != IEEE80211_CHAN_ANYC &&
            IEEE80211_IS_CHAN_11NG(chan))
            retv = ENETRESET;
        }
        break;
    case IEEE80211_PARAM_WDS:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_WDS, value);
        if (retv == 0) {
            /* WAR: set the auto assoc feature also for WDS */
            if (value) {
                wlan_set_param(vap, IEEE80211_AUTO_ASSOC, 1);
            }
        }
        break;
    case IEEE80211_PARAM_VAP_IND:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_VAP_IND, value);
        break;
    case IEEE80211_PARAM_BLOCKDFSCHAN:
        retv = wlan_set_device_param(ic, IEEE80211_DEVICE_BLKDFSCHAN, value);
        if (retv == EOK) {
            retv = ENETRESET;
        }
        break;
#if ATH_SUPPORT_WAPI
    case IEEE80211_PARAM_SETWAPI:
        retv = wlan_setup_wapi(vap, value);
        if (retv == 0) {
            retv = ENETRESET;
        }
        break;
    case IEEE80211_PARAM_WAPIREKEY_USK:
        retv = wlan_set_wapirekey_unicast(vap, value);
        break;
    case IEEE80211_PARAM_WAPIREKEY_MSK:
        retv = wlan_set_wapirekey_multicast(vap, value);
        break;		
    case IEEE80211_PARAM_WAPIREKEY_UPDATE:
        retv = wlan_set_wapirekey_update(vap, (unsigned char*)&extra[4]);
        break;
#endif

    case IEEE80211_IOCTL_GREEN_AP_PS_ENABLE:
        wlan_set_device_param(ic, IEEE80211_DEVICE_GREEN_AP_PS_ENABLE, value?1:0);
        retv = 0;
        break;

    case IEEE80211_IOCTL_GREEN_AP_PS_TIMEOUT:
        wlan_set_device_param(ic, IEEE80211_DEVICE_GREEN_AP_PS_TIMEOUT, value > 20 ? value : 20);
        retv = 0;
        break;

    case IEEE80211_IOCTL_GREEN_AP_PS_ON_TIME:
        wlan_set_device_param(ic, IEEE80211_DEVICE_GREEN_AP_PS_ON_TIME, value >= 0 ? value : 0);
        retv = 0;
        break;

#ifdef ATH_WPS_IE
    case IEEE80211_PARAM_WPS:
        retv = wlan_set_param(vap, IEEE80211_WPS_MODE, value);
        break;
#endif
#ifdef ATH_EXT_AP
    case IEEE80211_PARAM_EXTAP:
        if (value) {
            if (value == 3 /* dbg */) {
                extern void mi_tbl_dump(void *);
                mi_tbl_dump(vap->iv_ic->ic_miroot);
                break;
            }
            if (value == 2 /* dbg */) {
                extern void mi_tbl_purge(void *);
                IEEE80211_VAP_EXT_AP_DISABLE(vap);
                mi_tbl_purge(&vap->iv_ic->ic_miroot);
            }
            IEEE80211_VAP_EXT_AP_ENABLE(vap);
            /* Set the auto assoc feature for Extender Station */
            wlan_set_param(vap, IEEE80211_AUTO_ASSOC, 1);
        } else {
            IEEE80211_VAP_EXT_AP_DISABLE(vap);
        }
        break;
#endif
    case IEEE80211_PARAM_STA_FORWARD:
    retv = wlan_set_param(vap, IEEE80211_FEATURE_STAFWD, value);
    break;

    case IEEE80211_PARAM_CWM_EXTPROTMODE:
        if (value >= 0) {
            retv = wlan_set_device_param(ic,IEEE80211_DEVICE_CWM_EXTPROTMODE, value);
            if (retv == EOK) {
                retv = ENETRESET;
            }
        } else {
            retv =  EINVAL;
        }
        break;
    case IEEE80211_PARAM_CWM_EXTPROTSPACING:
        if (value >= 0) {
            retv = wlan_set_device_param(ic,IEEE80211_DEVICE_CWM_EXTPROTSPACING, value);
            if (retv == EOK) {
                retv = ENETRESET;
            }
        }
        else {
            retv =  EINVAL;
        }
        break;
    case IEEE80211_PARAM_CWM_ENABLE:
        if (value >= 0) {
            retv = wlan_set_device_param(ic,IEEE80211_DEVICE_CWM_ENABLE, value);
            if (retv == EOK) {
                retv = ENETRESET;
            }
        } else {
            retv =  EINVAL;
        }
        break;
    case IEEE80211_PARAM_CWM_EXTBUSYTHRESHOLD:
        if (value >=0 && value <=100) {
            retv = wlan_set_device_param(ic,IEEE80211_DEVICE_CWM_EXTBUSYTHRESHOLD, value);
            if (retv == EOK) {
                retv = ENETRESET;
            }
        } else {
            retv =  EINVAL;
        }
        break;
    case IEEE80211_PARAM_DOTH:
        retv = wlan_set_device_param(ic, IEEE80211_DEVICE_DOTH, value);
        if (retv == EOK) {
            retv = ENETRESET;   /* XXX: need something this drastic? */
        }
        break;
    case IEEE80211_PARAM_SETADDBAOPER:
        if (value > 1 || value < 0) {
            return EINVAL;
        }

        retv = wlan_set_device_param(ic, IEEE80211_DEVICE_ADDBA_MODE, value);
        break;
    case IEEE80211_PARAM_WMM:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_WMM, value);
        wlan_set_param(vap, IEEE80211_FEATURE_AMPDU, value);
        if (retv == EOK) {
            retv = ENETRESET;
        }
        break;
    case IEEE80211_PARAM_PROTMODE:
        retv = wlan_set_device_param(ic, IEEE80211_DEVICE_PROTECTION_MODE, value);
        /* NB: if not operating in 11g this can wait */
        if (retv == EOK) {
            wlan_chan_t chan = wlan_get_bss_channel(vap);
            if (chan != IEEE80211_CHAN_ANYC &&
                (IEEE80211_IS_CHAN_ANYG(chan) ||
                IEEE80211_IS_CHAN_11NG(chan))) {
                retv = ENETRESET;
            }
        }
        break;
    case IEEE80211_PARAM_ROAMING:
        if (!(IEEE80211_ROAMING_DEVICE <= value &&
            value <= IEEE80211_ROAMING_MANUAL))
            return -EINVAL;
        ic->ic_roaming = value;
        if(value == IEEE80211_ROAMING_MANUAL)
            IEEE80211_VAP_AUTOASSOC_DISABLE(vap);
        else
            IEEE80211_VAP_AUTOASSOC_ENABLE(vap);
        break;
    case IEEE80211_PARAM_DROPUNENCRYPTED:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_DROP_UNENC, value);
        break;
    case IEEE80211_PARAM_DRIVER_CAPS:
        retv = wlan_set_param(vap, IEEE80211_DRIVER_CAPS, value); /* NB: for testing */
        break;
/*
* Support for Mcast Enhancement
*/
#if ATH_SUPPORT_IQUE
    case IEEE80211_PARAM_ME:
        wlan_set_param(vap, IEEE80211_ME, value);
        break;
    case IEEE80211_PARAM_MEDEBUG:
        wlan_set_param(vap, IEEE80211_MEDEBUG, value);
        break;
    case IEEE80211_PARAM_ME_SNOOPLENGTH:
        wlan_set_param(vap, IEEE80211_ME_SNOOPLENGTH, value);
        break;
    case IEEE80211_PARAM_ME_TIMER:
        wlan_set_param(vap, IEEE80211_ME_TIMER, value);
        break;
    case IEEE80211_PARAM_ME_TIMEOUT:
        wlan_set_param(vap, IEEE80211_ME_TIMEOUT, value);
        break;
    case IEEE80211_PARAM_HBR_TIMER:
        wlan_set_param(vap, IEEE80211_HBR_TIMER, value);
        break;
    case IEEE80211_PARAM_ME_DROPMCAST:
        wlan_set_param(vap, IEEE80211_ME_DROPMCAST, value);
        break;
    case IEEE80211_PARAM_ME_CLEARDENY:
        wlan_set_param(vap, IEEE80211_ME_CLEARDENY, value);
        break;
#endif

#if  ATH_SUPPORT_AOW
    case IEEE80211_PARAM_SWRETRIES:
        wlan_set_aow_param(vap, IEEE80211_AOW_SWRETRIES, value);
        break;
	 case IEEE80211_PARAM_RTSRETRIES:
        wlan_set_aow_param(vap, IEEE80211_AOW_RTSRETRIES, value);
        break;
    case IEEE80211_PARAM_AOW_LATENCY:
        wlan_set_aow_param(vap, IEEE80211_AOW_LATENCY, value);
        break;
    case IEEE80211_PARAM_AOW_PLAY_LOCAL:
        wlan_set_aow_param(vap, IEEE80211_AOW_PLAY_LOCAL, value);
        break;
    case IEEE80211_PARAM_AOW_CLEAR_AUDIO_CHANNELS:
        wlan_set_aow_param(vap, IEEE80211_AOW_CLEAR_AUDIO_CHANNELS, value);
        break;        
    case IEEE80211_PARAM_AOW_STATS:
        wlan_set_aow_param(vap, IEEE80211_AOW_STATS, value);
        break;
    case IEEE80211_PARAM_AOW_ESTATS:
        wlan_set_aow_param(vap, IEEE80211_AOW_ESTATS, value);
        break;
    case IEEE80211_PARAM_AOW_INTERLEAVE:
        wlan_set_aow_param(vap, IEEE80211_AOW_INTERLEAVE, value);
        break;     
   case IEEE80211_PARAM_AOW_ER:
        wlan_set_aow_param(vap, IEEE80211_AOW_ER, value);
        break;
   case IEEE80211_PARAM_AOW_EC:
        wlan_set_aow_param(vap, IEEE80211_AOW_EC, value);
        break;
   case IEEE80211_PARAM_AOW_EC_RAMP:
        wlan_set_aow_param(vap, IEEE80211_AOW_EC_RAMP, value);
        break;
   case IEEE80211_PARAM_AOW_EC_FMAP:
        wlan_set_aow_param(vap, IEEE80211_AOW_EC_FMAP, value);
        break;
   case IEEE80211_PARAM_AOW_ES:
        wlan_set_aow_param(vap, IEEE80211_AOW_ES, value);
        break;
   case IEEE80211_PARAM_AOW_ESS:
        wlan_set_aow_param(vap, IEEE80211_AOW_ESS, value);
        break;
   case IEEE80211_PARAM_AOW_ESS_COUNT:
        wlan_set_aow_param(vap, IEEE80211_AOW_ESS_COUNT, value);
        break;
   case IEEE80211_PARAM_AOW_ENABLE_CAPTURE:
         wlan_set_aow_param(vap, IEEE80211_AOW_ENABLE_CAPTURE, value);
         break;
   case IEEE80211_PARAM_AOW_FORCE_INPUT:
        wlan_set_aow_param(vap, IEEE80211_AOW_FORCE_INPUT, value);
        break;
    case IEEE80211_PARAM_AOW_PRINT_CAPTURE:
        wlan_set_aow_param(vap, IEEE80211_AOW_PRINT_CAPTURE, value);
        break;
	 case IEEE80211_PARAM_AOW_AS:
        wlan_set_aow_param(vap, IEEE80211_AOW_AS, value);
        break;
    case IEEE80211_PARAM_AOW_PLAY_RX_CHANNEL:
        wlan_set_aow_param(vap, IEEE80211_AOW_PLAY_RX_CHANNEL, value);
        break;
    case IEEE80211_PARAM_AOW_SIM_CTRL_CMD:
        wlan_set_aow_param(vap, IEEE80211_AOW_SIM_CTRL_CMD, value);
        break;
    case IEEE80211_PARAM_AOW_FRAME_SIZE:
        wlan_set_aow_param(vap, IEEE80211_AOW_FRAME_SIZE, value);
        break;
    case IEEE80211_PARAM_AOW_ALT_SETTING:
        wlan_set_aow_param(vap, IEEE80211_AOW_ALT_SETTING, value);
        break;
    case IEEE80211_PARAM_AOW_ASSOC_ONLY:
        wlan_set_aow_param(vap, IEEE80211_AOW_ASSOC_ONLY, value);
        break;
    case IEEE80211_PARAM_AOW_DISCONNECT_DEVICE:
        printk("AOW : IEEE80211_PARAM_AOW_DISCONNECT_DEVICE\n");
        wlan_set_aow_param(vap, IEEE80211_AOW_DISCONNECT_DEVICE, value);
        break;

#endif  /* ATH_SUPPORT_AOW */

#if UMAC_SUPPORT_RPTPLACEMENT
        case IEEE80211_PARAM_CUSTPROTO_ENABLE:
            ieee80211_rptplacement_set_param(vap, IEEE80211_RPT_CUSTPROTO_ENABLE, value);
            break;

        case IEEE80211_PARAM_GPUTCALC_ENABLE:
            ieee80211_rptplacement_set_param(vap, IEEE80211_RPT_GPUTCALC_ENABLE, value);
            ieee80211_rptplacement_gput_est_init(vap, 0);
        break;

        case IEEE80211_PARAM_DEVUP:
            ieee80211_rptplacement_set_param(vap, IEEE80211_RPT_DEVUP, value);
            break;

        case IEEE80211_PARAM_MACDEV:
            ieee80211_rptplacement_set_param(vap, IEEE80211_RPT_MACDEV, value);
            ieee80211_rptplacement_get_mac_addr(vap, value);
            break;

        case IEEE80211_PARAM_MACADDR1:
            ieee80211_rptplacement_set_param(vap, IEEE80211_RPT_MACADDR1, value);
            break;

        case IEEE80211_PARAM_MACADDR2:
            ieee80211_rptplacement_set_param(vap, IEEE80211_RPT_MACADDR2, value);
            break;

        case IEEE80211_PARAM_GPUTMODE:
            ieee80211_rptplacement_set_param(vap, IEEE80211_RPT_GPUTMODE, value);
            ieee80211_rptplacement_get_gputmode(ic, value);
            break;

        case IEEE80211_PARAM_TXPROTOMSG:
            ieee80211_rptplacement_set_param(vap, IEEE80211_RPT_TXPROTOMSG, value);
            ieee80211_rptplacement_tx_proto_msg(vap, value);
            break;

        case IEEE80211_PARAM_RXPROTOMSG:
            ieee80211_rptplacement_set_param(vap, IEEE80211_RPT_RXPROTOMSG, value);
            break;

        case IEEE80211_PARAM_STATUS:
            ieee80211_rptplacement_set_param(vap, IEEE80211_RPT_STATUS, value);
            ieee80211_rptplacement_get_status(ic, value);
            break;

        case IEEE80211_PARAM_ASSOC:
            ieee80211_rptplacement_set_param(vap, IEEE80211_RPT_ASSOC, value);
            ieee80211_rptplacement_get_rptassoc(ic, value);
            break;

        case IEEE80211_PARAM_NUMSTAS:
            ieee80211_rptplacement_set_param(vap, IEEE80211_RPT_NUMSTAS, value);
            ieee80211_rptplacement_get_numstas(ic, value);
            break;

        case IEEE80211_PARAM_STA1ROUTE:
            ieee80211_rptplacement_set_param(vap, IEEE80211_RPT_STA1ROUTE, value);
            ieee80211_rptplacement_get_sta1route(ic, value);
            break;

        case IEEE80211_PARAM_STA2ROUTE:
            ieee80211_rptplacement_set_param(vap, IEEE80211_RPT_STA2ROUTE, value);
            ieee80211_rptplacement_get_sta2route(ic, value);
            break;

        case IEEE80211_PARAM_STA3ROUTE:
            ieee80211_rptplacement_set_param(vap, IEEE80211_RPT_STA3ROUTE, value);
            ieee80211_rptplacement_get_sta3route(ic, value);
            break;

        case IEEE80211_PARAM_STA4ROUTE:
            ieee80211_rptplacement_set_param(vap, IEEE80211_RPT_STA4ROUTE, value);
            ieee80211_rptplacement_get_sta4route(ic, value);
#endif

#if UMAC_SUPPORT_TDLS
        case IEEE80211_PARAM_TDLS_MACADDR1:
            wlan_set_param(vap, IEEE80211_TDLS_MACADDR1, value);
            break;

        case IEEE80211_PARAM_TDLS_MACADDR2:
            wlan_set_param(vap, IEEE80211_TDLS_MACADDR2, value);
            break;

        case IEEE80211_PARAM_TDLS_ACTION:
            wlan_set_param(vap, IEEE80211_TDLS_ACTION, value);
            break;
#endif

    case IEEE80211_PARAM_DTIM_PERIOD:
        if (!(osifp->os_opmode == IEEE80211_M_HOSTAP ||
            osifp->os_opmode == IEEE80211_M_IBSS)) {
            return -EINVAL;
        }
        if (value > IEEE80211_DTIM_MAX ||
            value < IEEE80211_DTIM_MIN) {
             
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,
                              "DTIM_PERIOD should be within %d to %d\n",
                              IEEE80211_DTIM_MIN,
                              IEEE80211_DTIM_MAX);
            return -EINVAL;
        }
        retv = wlan_set_param(vap, IEEE80211_DTIM_INTVAL, value);
        if (retv == EOK) {
            retv = ENETRESET;
        }

        break;
    case IEEE80211_PARAM_MACCMD:
        wlan_set_acl_policy(vap, value);
            break;
    case IEEE80211_PARAM_MCAST_RATE:
        /*
        * value is rate in units of Kbps
        * min: 1Mbps max: 300Mbps
        */
        if (value < 1000 || value > 300000)
            retv = EINVAL;
        else {
        wlan_set_param(vap, IEEE80211_MCAST_RATE, value);
        }
        break;
    case IEEE80211_PARAM_CCMPSW_ENCDEC:
        if (value) {
            IEEE80211_VAP_CCMPSW_ENCDEC_ENABLE(vap);
        } else {
            IEEE80211_VAP_CCMPSW_ENCDEC_DISABLE(vap);
        }
        break;

    case IEEE80211_PARAM_NETWORK_SLEEP:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s set IEEE80211_IOC_POWERSAVE parameter %d \n",
                          __func__,value );
        do {
            ieee80211_pwrsave_mode ps_mode = IEEE80211_PWRSAVE_NONE;
            switch(value) {
            case 0:
                ps_mode = IEEE80211_PWRSAVE_NONE;
                break;
            case 1:
                ps_mode = IEEE80211_PWRSAVE_LOW;
                break;
            case 2:
                ps_mode = IEEE80211_PWRSAVE_NORMAL;
                break;
            case 3:
                ps_mode = IEEE80211_PWRSAVE_MAXIMUM;
                break;
            }
            error= wlan_set_powersave(vap,ps_mode);
        } while(0);
        break;

#ifdef ATHEROS_LINUX_PERIODIC_SCAN
    case IEEE80211_PARAM_PERIODIC_SCAN:
        if (wlan_vap_get_opmode(vap) == IEEE80211_M_STA) {
            if (osifp->os_periodic_scan_period != value){
                if (value && (value < OSIF_PERIODICSCAN_MIN_PERIOD))
                    osifp->os_periodic_scan_period = OSIF_PERIODICSCAN_MIN_PERIOD;
                else 
                    osifp->os_periodic_scan_period = value;

                retv = ENETRESET;                
            }
        }
        break;
#endif        

#if ATH_SW_WOW
    case IEEE80211_PARAM_SW_WOW:
        if (wlan_vap_get_opmode(vap) == IEEE80211_M_STA) {
            retv = wlan_set_wow(vap, value);
        }
        break;
#endif
        
    case IEEE80211_PARAM_UAPSDINFO:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_UAPSD, value);
        if (retv == EOK) {
            retv = ENETRESET;
        }
	break ;

#ifdef ATH_SUPPORT_P2P
    /* WFD Sigma use these two to do reset and some cases. */
    case IEEE80211_PARAM_SLEEP:
        /* XXX: Forced sleep for testing. Does not actually place the
         *      HW in sleep mode yet. this only makes sense for STAs.
         */
        /* enable/disable force  sleep */
        wlan_pwrsave_force_sleep(vap,value);
        break;
#endif  


     case IEEE80211_PARAM_COUNTRY_IE:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_IC_COUNTRY_IE, value);
        if (retv == EOK) {
            retv = ENETRESET;
        }
        break;
#if ATH_RXBUF_RECYCLE
    case IEEE80211_PARAM_RXBUF_LIFETIME:
        ic->ic_osdev->rxbuf_lifetime = value;
        break;
#endif
	case IEEE80211_PARAM_2G_CSA:
        retv = wlan_set_device_param(ic, IEEE80211_DEVICE_2G_CSA, value);
        break;
#if UMAC_SUPPORT_BSSLOAD
    case IEEE80211_PARAM_QBSS_LOAD:
        if (value > 1 || value < 0) {
            return EINVAL;
        } else {
            retv = wlan_set_param(vap, IEEE80211_QBSS_LOAD, value);
            if (retv == EOK)
                retv = ENETRESET;
        }
        break;
#endif /* UMAC_SUPPORT_BSSLOAD */
#if UMAC_SUPPORT_CHANUTIL_MEASUREMENT
    case IEEE80211_PARAM_CHAN_UTIL_ENAB:
        if (value > 1 || value < 0) {
            return EINVAL;
        } else {
            retv = wlan_set_param(vap, IEEE80211_CHAN_UTIL_ENAB, value);
            if (retv == EOK)
                retv = ENETRESET;
        }
        break;
#endif /* UMAC_SUPPORT_CHANUTIL_MEASUREMENT */
#if UMAC_SUPPORT_QUIET
    case IEEE80211_PARAM_QUIET_PERIOD:
        if (value > 1 || value < 0) {
            return EINVAL;
        } else {
            retv = wlan_quiet_set_param(vap, value);
            if (retv == EOK)
                retv = ENETRESET;
        }
        break;
#endif /* UMAC_SUPPORT_QUIET */
case IEEE80211_PARAM_RRM_CAP:
        if (value > 1 || value < 0) {
            return EINVAL;
        } else {
            retv = wlan_set_param(vap, IEEE80211_RRM_CAP, value);
            if (retv == EOK)
                retv = ENETRESET;
        }
        break;

#if UMAC_SUPPORT_RRM_MISC
    case IEEE80211_PARAM_RRM_STATS:
        if (value > 1 || value < 0) {
        return EINVAL;
      } else {
        retv = ieee80211_set_rrmstats(vap,value);
     }
    break;
    case IEEE80211_PARAM_RRM_SLWINDOW:
        if (value > 2 || value < 0) {
            return EINVAL;
        } else {
            retv = ieee80211_rrm_set_slwindow(vap,value);
        }
        break;
#endif
	case IEEE80211_PARAM_WNM_CAP:
        if (value > 1 || value < 0) {
            return EINVAL;
        } else {
            retv = wlan_set_param(vap, IEEE80211_WNM_CAP, value);
            if (retv == EOK)
                retv = ENETRESET;
        }
        break;

#ifdef QCA_PARTNER_PLATFORM
    case IEEE80211_PARAM_PLTFRM_PRIVATE:
         retv = wlan_pltfrm_set_param(vap, value);
         if ( retv == EOK) {
             retv = ENETRESET;
         }
         break;
#endif 
#if ATH_SUPPORT_WPA_SUPPLICANT_CHECK_TIME
    case IEEE80211_PARAM_REJOINT_ATTEMP_TIME:
        retv = wlan_set_param(vap,IEEE80211_REJOINT_ATTEMP_TIME,value);
        break;
#endif

#if UMAC_SUPPORT_VI_DBG
        case IEEE80211_PARAM_DBG_CFG:
            ieee80211_vi_dbg_set_param(vap, IEEE80211_VI_DBG_CFG, value);
            break;
        case IEEE80211_PARAM_DBG_NUM_STREAMS:
            ieee80211_vi_dbg_set_param(vap, IEEE80211_VI_DBG_NUM_STREAMS, value);
            break;
        case IEEE80211_PARAM_STREAM_NUM:
            ieee80211_vi_dbg_set_param(vap, IEEE80211_VI_STREAM_NUM, value);
	        break;
        case IEEE80211_PARAM_DBG_NUM_MARKERS:
            ieee80211_vi_dbg_set_param(vap, IEEE80211_VI_DBG_NUM_MARKERS, value);
            break;
    	case IEEE80211_PARAM_MARKER_NUM:
            ieee80211_vi_dbg_set_param(vap, IEEE80211_VI_MARKER_NUM, value);
	        break;
        case IEEE80211_PARAM_MARKER_OFFSET_SIZE:
            ieee80211_vi_dbg_set_param(vap, IEEE80211_VI_MARKER_OFFSET_SIZE, value);  
            break;
        case IEEE80211_PARAM_MARKER_MATCH:
            ieee80211_vi_dbg_set_param(vap, IEEE80211_VI_MARKER_MATCH, value); 
	        ieee80211_vi_dbg_get_marker(vap);
            break;
        case IEEE80211_PARAM_RXSEQ_OFFSET_SIZE:
            ieee80211_vi_dbg_set_param(vap, IEEE80211_VI_RXSEQ_OFFSET_SIZE, value);  
            break;
        case IEEE80211_PARAM_RX_SEQ_RSHIFT:
            ieee80211_vi_dbg_set_param(vap, IEEE80211_VI_RX_SEQ_RSHIFT, value);
            break;
        case IEEE80211_PARAM_RX_SEQ_MAX:
            ieee80211_vi_dbg_set_param(vap, IEEE80211_VI_RX_SEQ_MAX, value);
            break;
        case IEEE80211_PARAM_RX_SEQ_DROP:
            ieee80211_vi_dbg_set_param(vap, IEEE80211_VI_RX_SEQ_DROP, value);
            break;
        case IEEE80211_PARAM_TIME_OFFSET_SIZE:
            ieee80211_vi_dbg_set_param(vap, IEEE80211_VI_TIME_OFFSET_SIZE, value);  
            break;
        case IEEE80211_PARAM_RESTART:
            ieee80211_vi_dbg_set_param(vap, IEEE80211_VI_RESTART, value);
            break;
        case IEEE80211_PARAM_RXDROP_STATUS:
            ieee80211_vi_dbg_set_param(vap, IEEE80211_VI_RXDROP_STATUS, value);
            break;
#endif            

    case IEEE80211_IOC_WPS_MODE:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,
                        "set IEEE80211_IOC_WPS_MODE to 0x%x\n", value);
        retv = wlan_set_param(vap, IEEE80211_WPS_MODE, value);
        break;
    case IEEE80211_IOC_SCAN_FLUSH:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "set %s\n",
                        "IEEE80211_IOC_SCAN_FLUSH");
        wlan_scan_table_flush(vap);
        retv = 0; /* success */
        break;
#ifdef ATH_SUPPORT_TxBF
    case IEEE80211_PARAM_TXBF_AUTO_CVUPDATE:
        wlan_set_param(vap, IEEE80211_TXBF_AUTO_CVUPDATE, value);
        ic->ic_set_config(vap);
        break;
    case IEEE80211_PARAM_TXBF_CVUPDATE_PER:
        wlan_set_param(vap, IEEE80211_TXBF_CVUPDATE_PER, value);
        ic->ic_set_config(vap);
        break;
#endif             
    case IEEE80211_PARAM_SCAN_BAND:
        osifp->os_scan_band = value;
        retv = 0;
        break;         

    case IEEE80211_PARAM_AMPDU:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_AMPDU, value);
        if (retv == EOK) {
            retv = ENETRESET;
        }
        
#if ATH_SUPPORT_IBSS_HT
        /*
         * config ic adhoc AMPDU capability
         */
        if (vap->iv_opmode == IEEE80211_M_IBSS) {
        
            wlan_dev_t ic = wlan_vap_get_devhandle(vap);    
            
            if (value && 
               (ieee80211_ic_ht20Adhoc_is_set(ic) || ieee80211_ic_ht40Adhoc_is_set(ic))) {
                wlan_set_device_param(ic, IEEE80211_DEVICE_HTADHOCAGGR, 1);
                printk("%s IEEE80211_PARAM_AMPDU = %d and HTADHOC enable\n", __func__, value);
            } else {
                wlan_set_device_param(ic, IEEE80211_DEVICE_HTADHOCAGGR, 0);
                printk("%s IEEE80211_PARAM_AMPDU = %d and HTADHOC disable\n", __func__, value);
            }
        }
        
        // don't reset
        retv = EOK;
#endif /* end of #if ATH_SUPPORT_IBSS_HT */
        
        break;
    
    case IEEE80211_PARAM_SHORTPREAMBLE:
        retv = wlan_set_param(vap, IEEE80211_SHORT_PREAMBLE, value);
        if (retv == EOK) {
            retv = ENETRESET;
        }
       break;

    case IEEE80211_PARAM_CHANBW:
        switch (value)
        {
        case 0:
            ic->ic_chanbwflag = 0;
            break;
        case 1:
            ic->ic_chanbwflag = IEEE80211_CHAN_HALF;
            break;
        case 2:
            ic->ic_chanbwflag = IEEE80211_CHAN_QUARTER;
            break;
        default:
            retv = EINVAL;
            break;
        }
        break;

    case IEEE80211_PARAM_INACT:
        wlan_set_param(vap, IEEE80211_RUN_INACT_TIMEOUT, value);
        break;
    case IEEE80211_PARAM_INACT_AUTH:
        wlan_set_param(vap, IEEE80211_AUTH_INACT_TIMEOUT, value);
        break;
    case IEEE80211_PARAM_INACT_INIT:
        wlan_set_param(vap, IEEE80211_INIT_INACT_TIMEOUT, value);
        break;
    case IEEE80211_PARAM_PWRTARGET:
		wlan_set_param(vap, IEEE80211_DEVICE_PWRTARGET, value);
        break;
    case IEEE80211_PARAM_WDS_AUTODETECT:
        wlan_set_param(vap, IEEE80211_WDS_AUTODETECT, value);
        break;
    case IEEE80211_PARAM_WEP_TKIP_HT:
		wlan_set_param(vap, IEEE80211_WEP_TKIP_HT, value);
        retv = ENETRESET;
        break;
    case IEEE80211_PARAM_IGNORE_11DBEACON:
        wlan_set_param(vap, IEEE80211_IGNORE_11DBEACON, value);
        break;
    case IEEE80211_PARAM_MFP_TEST:
        wlan_set_param(vap, IEEE80211_FEATURE_MFP_TEST, value);
        break;
#if UMAC_SUPPORT_TDLS
    case IEEE80211_PARAM_TDLS_ENABLE:
        if (value) {
            printk("Enabling TDLS: ");
            vap->iv_ath_cap |= IEEE80211_ATHC_TDLS;
			ic->ic_tdls->tdls_enable = 1;
        } else {
            printk("Disabling TDLS: ");
            vap->iv_ath_cap &= ~IEEE80211_ATHC_TDLS;
			ic->ic_tdls->tdls_enable = 0;
        }
        printf("%x\n", vap->iv_ath_cap & IEEE80211_ATHC_TDLS);
        break;
#if not_yet
	case IEEE80211_PARAM_TDLS_PEER_UAPSD_ENABLE:
        if (value) {
            ieee80211_ioctl_set_tdls_peer_uapsd_enable(dev, TDLS_PEER_UAPSD_ENABLE);
        }
        else {
            ieee80211_ioctl_set_tdls_peer_uapsd_enable(dev, TDLS_PEER_UAPSD_DISABLE);
        }
        break;

    case IEEE80211_PARAM_SET_TDLS_RMAC: {
        u_int8_t mac[ETH_ALEN];
        char smac[MACSTR_LEN];
		ieee80211_tdls_set_mac_addr(mac, vap->iv_tdls_macaddr1, vap->iv_tdls_macaddr2);
		snprintf(smac, MACSTR_LEN, "%s", ether_sprintf(mac));
    	printk("TDLS set_tdls_rmac ....%s \n", smac);
		ieee80211_ioctl_set_tdls_rmac(dev, info, w, smac);
        break;
        }
    case IEEE80211_PARAM_CLR_TDLS_RMAC: {
        u_int8_t mac[ETH_ALEN];
        char smac[MACSTR_LEN];
		ieee80211_tdls_set_mac_addr(mac, vap->iv_tdls_macaddr1, vap->iv_tdls_macaddr2);
		snprintf(smac, MACSTR_LEN, "%s", ether_sprintf(mac));
    	printk("TDLS clr_tdls_rmac ....%s\n", smac);
		ieee80211_ioctl_clr_tdls_rmac(dev, info, w, smac);
        break;
        }
	case IEEE80211_PARAM_TDLS_QOSNULL: {
        u_int8_t mac[ETH_ALEN];
        char smac[MACSTR_LEN];
        ieee80211_tdls_set_mac_addr(mac, vap->iv_tdls_macaddr1, vap->iv_tdls_macaddr2);
        snprintf(smac, MACSTR_LEN, "%s", ether_sprintf(mac));
        printk("TDLS send QOSNULL to ....%s\n", smac);
        ieee80211_ioctl_tdls_qosnull(dev, info, w, smac, value);
        break;
        }
	
#ifdef CONFIG_RCPI
    case IEEE80211_PARAM_TDLS_RCPI_HI:
        if (!IEEE80211_TDLS_ENABLED(vap))
            return -EFAULT;

        if ((value >=0) && (value<=300)) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
                "Setting TDLS:RCPI: Hi Threshold %d dB \n", value);
            vap->iv_ic->ic_tdls->hi_tmp = value;
        } else {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
                "Setting TDLS:RCPI: Hi Threshold - Invalid vaule %d dB\n", value);
            printk("Enter any value between 0dB-100dB \n");
        }
        break;
    case IEEE80211_PARAM_TDLS_RCPI_LOW:
        if (!IEEE80211_TDLS_ENABLED(vap))
            return -EFAULT;

        if ((value >=0) && (value<=300)) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
                "Setting TDLS:RCPI: Low Threshold %d dB \n", value);
            vap->iv_ic->ic_tdls->lo_tmp = value;
        } else {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
                "Setting TDLS:RCPI: Low Threshold - Invalid vaule %d dB\n", value);
            printk("Enter any value between 0dB-100dB \n");
        }
        break;
    case IEEE80211_PARAM_TDLS_RCPI_MARGIN:
        if (!IEEE80211_TDLS_ENABLED(vap))
            return -EFAULT;

        if ((value >=0) && (value<=300)) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
                "Setting TDLS:RCPI: Margin %d dB \n", value);
            vap->iv_ic->ic_tdls->mar_tmp = value;
        } else {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
                "Setting TDLS:RCPI: Margin - Invalid vaule %d dB\n", value);
            printk("Enter any value between 0dB-100dB \n");
        }
        break;
    case IEEE80211_PARAM_TDLS_SET_RCPI:
        if (!IEEE80211_TDLS_ENABLED(vap))
            return -EFAULT;
        if (value) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
                "Enabling TDLS:RCPI: %d \n", value);
            vap->iv_ic->ic_tdls->hithreshold = vap->iv_ic->ic_tdls->hi_tmp;
            vap->iv_ic->ic_tdls->lothreshold = vap->iv_ic->ic_tdls->lo_tmp;
            vap->iv_ic->ic_tdls->margin = vap->iv_ic->ic_tdls->mar_tmp;
        }
        break;
#endif /* CONFIG_RCPI */
	case IEEE80211_PARAM_TDLS_DIALOG_TOKEN:
        printk("Set Dialog_Token %d \n",value);
        vap->iv_tdls_dialog_token = (u_int8_t) value;
        break;
    case IEEE80211_PARAM_TDLS_DISCOVERY_REQ: {
        u_int8_t mac[ETH_ALEN];
        char smac[MACSTR_LEN];
        int sendret;
        ieee80211_tdls_set_mac_addr(mac, vap->iv_tdls_macaddr1,
                                         vap->iv_tdls_macaddr2);
        snprintf(smac, MACSTR_LEN, "%s", ether_sprintf(mac));
        printk("TDLS do_tdls_dc_req ....%s\n", smac);
        sendret = tdls_send_discovery_req(ic, vap, mac,NULL,0,
                                            vap->iv_tdls_dialog_token);
        printk("tdls_send_discovery_req (%x)\n", sendret);
        break;
        }
	case IEEE80211_PARAM_QOSNULL:
        /* Force a QoS Null for testing. */
        ieee80211_send_qosnulldata(vap->iv_bss, value, 0);
        break;

#endif
	case IEEE80211_PARAM_PSPOLL:
        /* Force a PS-POLL for testing. */
        ieee80211_send_pspoll(vap->iv_bss);
        break;
	case IEEE80211_PARAM_STA_PWR_SET_PSPOLL:
        wlan_set_param(vap, IEEE80211_FEATURE_PSPOLL, value);
        break;
#if ATH_TDLS_AUTO_CONNECT
    case IEEE80211_PARAM_TDLS_AUTO_ENABLE:
        if (value) {
            printk("Enabling TDLS_AUTO\n");
            ic->ic_tdls_auto_enable = 1;
            vap->iv_ath_cap |= IEEE80211_ATHC_TDLS;
            ic->ic_tdls->tdls_enable = 1;
        } else {
            printk("Disabling TDLS_AUTO\n");
            ic->ic_tdls_auto_enable = 0;
            vap->iv_ath_cap &= ~IEEE80211_ATHC_TDLS;
            ic->ic_tdls->tdls_enable = 0;
        }
        break;
    case IEEE80211_PARAM_TDLS_OFF_TIMEOUT:
        ic->ic_off_table_timeout = (u_int16_t) value;
        break;
    case IEEE80211_PARAM_TDLS_TDB_TIMEOUT:
        ic->ic_teardown_block_timeout = (u_int16_t) value;
        break;
    case IEEE80211_PARAM_TDLS_WEAK_TIMEOUT:
        ic->ic_weak_peer_timeout = (u_int16_t) value;
        break;
	case IEEE80211_PARAM_TDLS_RSSI_MARGIN:
        ic->ic_tdls_setup_margin = (u_int8_t) value;
        break;
    case IEEE80211_PARAM_TDLS_RSSI_UPPER_BOUNDARY:
        ic->ic_tdls_upper_boundary = (u_int8_t) value;
        break;
    case IEEE80211_PARAM_TDLS_RSSI_LOWER_BOUNDARY:
        ic->ic_tdls_lower_boundary = (u_int8_t) value;
        break;
    case IEEE80211_PARAM_TDLS_PATH_SELECT:
        ic->ic_tdls_path_select_enable = (u_int8_t) value;
        break;
    case IEEE80211_PARAM_TDLS_RSSI_OFFSET:
        ic->ic_tdls_setup_offset = (u_int8_t) value;
        break;
    case IEEE80211_PARAM_TDLS_PATH_SEL_PERIOD:
        ic->ic_path_select_period = (u_int16_t) value;
        break;
    case IEEE80211_PARAM_TDLS_TABLE_QUERY:
        ic->ic_tdls_table_query(vap);
        break;
#endif
#endif /* UMAC_SUPPORT_TDLS */
#if ATH_SUPPORT_FLOWMAC_MODULE
    case IEEE80211_PARAM_FLOWMAC:
        vap->iv_flowmac = value;
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,
                "%s flowmac enabled on this vap 0x%p \n", __func__, vap);
        break;
#endif
#if ATH_SUPPORT_IBSS_DFS
     case IEEE80211_PARAM_IBSS_DFS_PARAM:
        {
#define IBSSDFS_CSA_TIME_MASK 0x00ff0000
#define IBSSDFS_ACTION_MASK   0x0000ff00
#define IBSSDFS_RECOVER_MASK  0x000000ff
            u_int8_t csa_in_tbtt;
            u_int8_t actions_threshold;
            u_int8_t rec_threshold_in_tbtt;

            csa_in_tbtt = (value & IBSSDFS_CSA_TIME_MASK) >> 16;
            actions_threshold = (value & IBSSDFS_ACTION_MASK) >> 8;
            rec_threshold_in_tbtt = (value & IBSSDFS_RECOVER_MASK);

            if (rec_threshold_in_tbtt > csa_in_tbtt &&
                actions_threshold > 0) {
                vap->iv_ibss_dfs_csa_threshold = csa_in_tbtt;
                vap->iv_ibss_dfs_csa_measrep_limit = actions_threshold;
                vap->iv_ibss_dfs_enter_recovery_threshold_in_tbtt = rec_threshold_in_tbtt;
                ieee80211_ibss_beacon_update_start(ic);
            } else {
                printk("please enter a valid value .ex 0x010102\n");
                printk("Ex.0xaabbcc aa[channel switch time] bb[actions count] cc[recovery time]\n");
                printk("recovery time must be bigger than channel switch time, actions count must > 0\n");
            }
        
#undef IBSSDFS_CSA_TIME_MASK 
#undef IBSSDFS_ACTION_MASK   
#undef IBSSDFS_RECOVER_MASK              
    }
        break;
#endif  
#ifdef not_yet 
#if ATH_SUPPORT_IBSS_NETLINK_NOTIFICATION
    case IEEE80211_PARAM_IBSS_SET_RSSI_CLASS:
      {
	int i;
	u_int8_t rssi;
	u_int8_t *pvalue = (u_int8_t*)(extra + 4);

	/* 0th idx is 0 dbm(highest) always */
	vap->iv_ibss_rssi_class[0] = (u_int8_t)-1;

	for( i = 1; i < IBSS_RSSI_CLASS_MAX; i++ ) {
	  rssi = pvalue[i - 1];
	  /* Assumes the values in dbm are already sorted.
	   * Convert to rssi and store them */
	  vap->iv_ibss_rssi_class[i] = (rssi > 95 ? 0 : (95 - rssi));
	}
      }
      break;
    case IEEE80211_PARAM_IBSS_START_RSSI_MONITOR:
      vap->iv_ibss_rssi_monitor = value;
      /* set the hysteresis to atleast 1 */
      if (value && !vap->iv_ibss_rssi_hysteresis)
	vap->iv_ibss_rssi_hysteresis++;
      break;
    case IEEE80211_PARAM_IBSS_RSSI_HYSTERESIS:
      vap->iv_ibss_rssi_hysteresis = value;
        break;
#endif    
#endif  
#if ATH_SUPPORT_WIFIPOS
   case IEEE80211_PARAM_WIFIPOS_TXCORRECTION:
    ieee80211_wifipos_set_txcorrection(vap,(unsigned int)value);
    break;

   case IEEE80211_PARAM_WIFIPOS_RXCORRECTION:
    ieee80211_wifipos_set_rxcorrection(vap,(unsigned int)value);
    break;
#endif
  
    default:
        retv =
#if ATHEROS_LINUX_P2P_DRIVER
        ieee80211_ioctl_setp2p(dev, info, w, extra);
#else
        EOPNOTSUPP;
#endif
        if (retv) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s parameter 0x%x is "
                            "not supported retv=%d\n", __func__, param, retv);
        }
        break;
    }
	if (retv == ENETRESET)
    {
        retv = IS_UP(dev) ? -osif_vap_init(dev, RESCAN) : 0;
    }
    return -retv;
}



int
acfg_get_vap_param(void *ctx, a_uint16_t cmdid,
                  a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_param_req_t        *ptr;
    acfg_os_req_t   *req = NULL;
	osif_dev *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
    wlan_dev_t ic = wlan_vap_get_devhandle(vap);
	int retv = 0, param, value = 0;

    req = (acfg_os_req_t *) buffer;
    ptr  = &req->data.param_req;


    param = acfg_acfg2ieee(ptr->param);
    ptr->val = 0;

    switch (param)
    {
    case IEEE80211_PARAM_MAXSTA:
        printk("Getting Max Stations: %d\n", vap->iv_max_aid - 1);
        value = vap->iv_max_aid - 1;
        break;
    case IEEE80211_PARAM_AUTO_ASSOC:
        value = wlan_get_param(vap, IEEE80211_AUTO_ASSOC);
        break;
    case IEEE80211_PARAM_VAP_COUNTRY_IE:
        value = wlan_get_param(vap, IEEE80211_FEATURE_COUNTRY_IE);
        break;
    case IEEE80211_PARAM_VAP_DOTH:
        value = wlan_get_param(vap, IEEE80211_FEATURE_DOTH);
        break;
    case IEEE80211_PARAM_HT40_INTOLERANT:
        value = wlan_get_param(vap, IEEE80211_HT40_INTOLERANT);
        break;

    case IEEE80211_PARAM_CHWIDTH:
        value = wlan_get_param(vap, IEEE80211_CHWIDTH);
        break;

    case IEEE80211_PARAM_CHEXTOFFSET:
        value = wlan_get_param(vap, IEEE80211_CHEXTOFFSET);
        break;
#ifdef ATH_SUPPORT_QUICK_KICKOUT
    case IEEE80211_PARAM_STA_QUICKKICKOUT:
        value = wlan_get_param(vap, IEEE80211_STA_QUICKKICKOUT);
        break;
#endif
    case IEEE80211_PARAM_CHSCANINIT:
        value = wlan_get_param(vap, IEEE80211_CHSCANINIT);
        break;

    case IEEE80211_PARAM_COEXT_DISABLE:
        value = ((ic->ic_flags & IEEE80211_F_COEXT_DISABLE) != 0);
        break;

    case IEEE80211_PARAM_AUTHMODE:
        //fixme how it used to be done: value = osifp->authmode;
        {
            ieee80211_auth_mode modes[IEEE80211_AUTH_MAX];
            retv = wlan_get_auth_modes(vap, modes, IEEE80211_AUTH_MAX);
            if (retv > 0)
            {
                value = modes[0];
                if((retv > 1) && (modes[0] == IEEE80211_AUTH_OPEN) && (modes[1] == IEEE80211_AUTH_SHARED))
                    value =  IEEE80211_AUTH_AUTO;
                retv = 0;
            }
        }
        break;
    case IEEE80211_PARAM_MCASTCIPHER:
        {
            ieee80211_cipher_type mciphers[1];
            int count;
            count = wlan_get_mcast_ciphers(vap,mciphers,1);
            if (count == 1)
                value = mciphers[0];
        }
        break;
    case IEEE80211_PARAM_MCASTKEYLEN:
        value = wlan_get_rsn_cipher_param(vap, IEEE80211_MCAST_CIPHER_LEN);
        break;
    case IEEE80211_PARAM_UCASTCIPHERS:
        do {
            ieee80211_cipher_type uciphers[IEEE80211_CIPHER_MAX];
            int i, count;
            count = wlan_get_ucast_ciphers(vap, uciphers, IEEE80211_CIPHER_MAX);
            for (i = 0; i < count; i++) {
                value |= 1<<uciphers[i];
            }
    } while (0);
        break;
    case IEEE80211_PARAM_UCASTCIPHER:
        do {
            ieee80211_cipher_type uciphers[1];
            int count = 0;
            count = wlan_get_ucast_ciphers(vap, uciphers, 1);
            if (count == 1)
                value |= 1<<uciphers[0];
        } while (0);
        break;
    case IEEE80211_PARAM_UCASTKEYLEN:
        value = wlan_get_rsn_cipher_param(vap, IEEE80211_UCAST_CIPHER_LEN);
        break;
    case IEEE80211_PARAM_PRIVACY:
        value = wlan_get_param(vap, IEEE80211_FEATURE_PRIVACY);
        break;
    case IEEE80211_PARAM_COUNTERMEASURES:
        value = wlan_get_param(vap, IEEE80211_FEATURE_COUNTER_MEASURES);
        break;
    case IEEE80211_PARAM_HIDESSID:
        value = wlan_get_param(vap, IEEE80211_FEATURE_HIDE_SSID);
        break;
    case IEEE80211_PARAM_APBRIDGE:
        value = wlan_get_param(vap, IEEE80211_FEATURE_APBRIDGE);
        break;
    case IEEE80211_PARAM_KEYMGTALGS:
        value = wlan_get_rsn_cipher_param(vap, IEEE80211_KEYMGT_ALGS);
        break;
    case IEEE80211_PARAM_RSNCAPS:
        value = wlan_get_rsn_cipher_param(vap, IEEE80211_RSN_CAPS);
        break;
    case IEEE80211_PARAM_WPA:
    {
            ieee80211_auth_mode modes[IEEE80211_AUTH_MAX];
            int count, i;
            value = 0;
            count = wlan_get_auth_modes(vap,modes,IEEE80211_AUTH_MAX);
            for (i = 0; i < count; i++) {
            if (modes[i] == IEEE80211_AUTH_WPA)
                value |= 0x1;
            if (modes[i] == IEEE80211_AUTH_RSNA)
                value |= 0x2;
            }
    }
        break;
    case IEEE80211_PARAM_DBG_LVL:
        value = (u_int32_t)wlan_get_debug_flags(vap);
        break;
#if UMAC_SUPPORT_IBSS
    case IEEE80211_PARAM_IBSS_CREATE_DISABLE:
        value = osifp->disable_ibss_create;
        break;
#endif
    case IEEE80211_PARAM_BEACON_INTERVAL:
        value = wlan_get_param(vap, IEEE80211_BEACON_INTVAL);
        break;
#if ATH_SUPPORT_AP_WDS_COMBO
    case IEEE80211_PARAM_NO_BEACON:
        value = wlan_get_param(vap, IEEE80211_NO_BEACON);
        break;
#endif
    case IEEE80211_PARAM_PUREG:
        value = wlan_get_param(vap, IEEE80211_FEATURE_PUREG);
        break;
    case IEEE80211_PARAM_PUREN:
        value = wlan_get_param(vap, IEEE80211_FEATURE_PURE11N);
        break;
    case IEEE80211_PARAM_WDS:
        value = wlan_get_param(vap, IEEE80211_FEATURE_WDS);
        break;
    case IEEE80211_PARAM_VAP_IND:
        value = wlan_get_param(vap, IEEE80211_FEATURE_VAP_IND);
        break; 
    case IEEE80211_IOCTL_GREEN_AP_PS_ENABLE: 
        value = (wlan_get_device_param(ic, IEEE80211_DEVICE_GREEN_AP_PS_ENABLE) ? 1:0);
        break;
    case IEEE80211_IOCTL_GREEN_AP_PS_TIMEOUT:
        value = wlan_get_device_param(ic, IEEE80211_DEVICE_GREEN_AP_PS_TIMEOUT);
        break;
    case IEEE80211_IOCTL_GREEN_AP_PS_ON_TIME:
        value = wlan_get_device_param(ic, IEEE80211_DEVICE_GREEN_AP_PS_ON_TIME);
        break;

#ifdef ATH_WPS_IE
    case IEEE80211_PARAM_WPS:
        value = wlan_get_param(vap, IEEE80211_WPS_MODE);
        break;
#endif
#ifdef ATH_EXT_AP
    case IEEE80211_PARAM_EXTAP:
        value = (IEEE80211_VAP_IS_EXT_AP_ENABLED(vap) == IEEE80211_FEXT_AP);
        break;
#endif


    case IEEE80211_PARAM_STA_FORWARD:
    value  = wlan_get_param(vap, IEEE80211_FEATURE_STAFWD);
    break;

    case IEEE80211_PARAM_CWM_EXTPROTMODE:
        value = wlan_get_device_param(ic, IEEE80211_DEVICE_CWM_EXTPROTMODE);
        break;
    case IEEE80211_PARAM_CWM_EXTPROTSPACING:
        value = wlan_get_device_param(ic, IEEE80211_DEVICE_CWM_EXTPROTSPACING);
        break;
    case IEEE80211_PARAM_CWM_ENABLE:
        value = wlan_get_device_param(ic, IEEE80211_DEVICE_CWM_ENABLE);
        break;
    case IEEE80211_PARAM_CWM_EXTBUSYTHRESHOLD:
        value = wlan_get_device_param(ic, IEEE80211_DEVICE_CWM_EXTBUSYTHRESHOLD);
        break;
    case IEEE80211_PARAM_DOTH:
        value = wlan_get_device_param(ic, IEEE80211_DEVICE_DOTH);
        break;
    case IEEE80211_PARAM_WMM:
        value = wlan_get_param(vap, IEEE80211_FEATURE_WMM);
        break;
    case IEEE80211_PARAM_PROTMODE:
        value = wlan_get_device_param(ic, IEEE80211_DEVICE_PROTECTION_MODE);
        break;
    case IEEE80211_PARAM_DRIVER_CAPS:
        value = wlan_get_param(vap, IEEE80211_DRIVER_CAPS);
        break;
    case IEEE80211_PARAM_MACCMD:
        value = wlan_get_acl_policy(vap);
        break;
    case IEEE80211_PARAM_DROPUNENCRYPTED:
        value = wlan_get_param(vap, IEEE80211_FEATURE_DROP_UNENC);
    break;
    case IEEE80211_PARAM_DTIM_PERIOD:
        value = wlan_get_param(vap, IEEE80211_DTIM_INTVAL);
        break;
    case IEEE80211_PARAM_SHORT_GI:
        value = wlan_get_param(vap, IEEE80211_SHORT_GI);
        break;
   case IEEE80211_PARAM_SHORTPREAMBLE:
        value = wlan_get_param(vap, IEEE80211_SHORT_PREAMBLE);
        break;


    /*
    * Support to Mcast Enhancement
    */
#if ATH_SUPPORT_IQUE
    case IEEE80211_PARAM_ME:
        value = wlan_get_param(vap, IEEE80211_ME);
        break;
    case IEEE80211_PARAM_MEDUMP:
        value = wlan_get_param(vap, IEEE80211_MEDUMP);
        break;
    case IEEE80211_PARAM_MEDEBUG:
        value = wlan_get_param(vap, IEEE80211_MEDEBUG);
        break;
    case IEEE80211_PARAM_ME_SNOOPLENGTH:
        value = wlan_get_param(vap, IEEE80211_ME_SNOOPLENGTH);
        break;
    case IEEE80211_PARAM_ME_TIMER:
        value = wlan_get_param(vap, IEEE80211_ME_TIMER);
        break;
    case IEEE80211_PARAM_ME_TIMEOUT:
        value = wlan_get_param(vap, IEEE80211_ME_TIMEOUT);
        break;
    case IEEE80211_PARAM_HBR_TIMER:
        value = wlan_get_param(vap, IEEE80211_HBR_TIMER);
        break;
    case IEEE80211_PARAM_HBR_STATE:
        wlan_get_hbrstate(vap);
        value = 0;
        break;
    case IEEE80211_PARAM_ME_DROPMCAST:
        value = wlan_get_param(vap, IEEE80211_ME_DROPMCAST);
        break;
    case IEEE80211_PARAM_ME_SHOWDENY:
        value = wlan_get_param(vap, IEEE80211_ME_SHOWDENY);
        break;
    case IEEE80211_PARAM_GETIQUECONFIG:
        value = wlan_get_param(vap, IEEE80211_IQUE_CONFIG);
        break;
#endif /*ATH_SUPPORT_IQUE*/

#if  ATH_SUPPORT_AOW
    case IEEE80211_PARAM_SWRETRIES:
        value = wlan_get_aow_param(vap, IEEE80211_AOW_SWRETRIES);
        break;
    case IEEE80211_PARAM_AOW_LATENCY:
        value = wlan_get_aow_param(vap, IEEE80211_AOW_LATENCY);
        break;
    case IEEE80211_PARAM_AOW_PLAY_LOCAL:
        value = wlan_get_aow_param(vap, IEEE80211_AOW_PLAY_LOCAL);
        break;
    case IEEE80211_PARAM_AOW_STATS:
        value = wlan_get_aow_param(vap, IEEE80211_AOW_STATS);
        break;
    case IEEE80211_PARAM_AOW_ESTATS:
        value = wlan_get_aow_param(vap, IEEE80211_AOW_ESTATS);
        break;
    case IEEE80211_PARAM_AOW_INTERLEAVE:
        value = wlan_get_aow_param(vap, IEEE80211_AOW_INTERLEAVE);
        break;
    case IEEE80211_PARAM_AOW_ER:
        value = wlan_get_aow_param(vap, IEEE80211_AOW_ER);
        break;
    case IEEE80211_PARAM_AOW_EC:
        value = wlan_get_aow_param(vap, IEEE80211_AOW_EC);
        break;
    case IEEE80211_PARAM_AOW_EC_FMAP:
        value = wlan_get_aow_param(vap, IEEE80211_AOW_EC_FMAP);
        break;
    case IEEE80211_PARAM_AOW_ES:
        value = wlan_get_aow_param(vap, IEEE80211_AOW_ES);
        break;
    case IEEE80211_PARAM_AOW_ESS:
        value = wlan_get_aow_param(vap, IEEE80211_AOW_ESS);
        break;
    case IEEE80211_PARAM_AOW_LIST_AUDIO_CHANNELS:
        value = wlan_get_aow_param(vap, IEEE80211_AOW_LIST_AUDIO_CHANNELS);
        break;
        
#endif /*ATH_SUPPORT_AOW*/

#if UMAC_SUPPORT_RPTPLACEMENT
    case IEEE80211_PARAM_CUSTPROTO_ENABLE:
        value = ieee80211_rptplacement_get_param(vap, IEEE80211_RPT_CUSTPROTO_ENABLE);
        break;
    case IEEE80211_PARAM_GPUTCALC_ENABLE:
        value = ieee80211_rptplacement_get_param(vap, IEEE80211_RPT_GPUTCALC_ENABLE);
        break;
    case IEEE80211_PARAM_DEVUP:
        value = ieee80211_rptplacement_get_param(vap, IEEE80211_RPT_DEVUP);
        break;
    case IEEE80211_PARAM_MACDEV:
        value = ieee80211_rptplacement_get_param(vap, IEEE80211_RPT_MACDEV);
        break;
    case IEEE80211_PARAM_MACADDR1:
        value = ieee80211_rptplacement_get_param(vap, IEEE80211_RPT_MACADDR1);
        break;
    case IEEE80211_PARAM_MACADDR2:
        value = ieee80211_rptplacement_get_param(vap, IEEE80211_RPT_MACADDR2);
        break;
    case IEEE80211_PARAM_GPUTMODE:
        value = ieee80211_rptplacement_get_param(vap, IEEE80211_RPT_GPUTMODE);
        break;
    case IEEE80211_PARAM_TXPROTOMSG:
        value = ieee80211_rptplacement_get_param(vap, IEEE80211_RPT_TXPROTOMSG);
        break;
    case IEEE80211_PARAM_RXPROTOMSG:
        value = ieee80211_rptplacement_get_param(vap, IEEE80211_RPT_RXPROTOMSG);
        break;
    case IEEE80211_PARAM_STATUS:
        value = ieee80211_rptplacement_get_param(vap, IEEE80211_RPT_STATUS);
        break;
    case IEEE80211_PARAM_ASSOC:
        value = ieee80211_rptplacement_get_param(vap, IEEE80211_RPT_ASSOC);
        break;
    case IEEE80211_PARAM_NUMSTAS:
        value = ieee80211_rptplacement_get_param(vap, IEEE80211_RPT_NUMSTAS);
        break;
    case IEEE80211_PARAM_STA1ROUTE:
        value = ieee80211_rptplacement_get_param(vap, IEEE80211_RPT_STA1ROUTE);
        break;
    case IEEE80211_PARAM_STA2ROUTE:
        value = ieee80211_rptplacement_get_param(vap, IEEE80211_RPT_STA2ROUTE);
        break;
    case IEEE80211_PARAM_STA3ROUTE:
        value = ieee80211_rptplacement_get_param(vap, IEEE80211_RPT_STA3ROUTE);
        break;
    case IEEE80211_PARAM_STA4ROUTE:
        value = ieee80211_rptplacement_get_param(vap, IEEE80211_RPT_STA4ROUTE);
#endif

#if UMAC_SUPPORT_TDLS
    case IEEE80211_PARAM_TDLS_MACADDR1:
        value = wlan_get_param(vap, IEEE80211_TDLS_MACADDR1);
        break;
    case IEEE80211_PARAM_TDLS_MACADDR2:
        value = wlan_get_param(vap, IEEE80211_TDLS_MACADDR2);
        break;
    case IEEE80211_PARAM_TDLS_ACTION:
        value = wlan_get_param(vap, IEEE80211_TDLS_ACTION);
        break;
#endif

    case IEEE80211_PARAM_COUNTRYCODE:
        value = wlan_get_device_param(ic, IEEE80211_DEVICE_COUNTRYCODE);
        break;
    case IEEE80211_PARAM_11N_RATE:
        value = wlan_get_param(vap, IEEE80211_FIXED_RATE);
        printk("Getting Rate Series: %x\n",value);
        break;
    case IEEE80211_PARAM_11N_RETRIES:
        value = wlan_get_param(vap, IEEE80211_FIXED_RETRIES);
        printk("Getting Retry Series: %x\n",value);
        break;
    case IEEE80211_PARAM_MCAST_RATE:
        value = wlan_get_param(vap, IEEE80211_MCAST_RATE);
        break;
    case IEEE80211_PARAM_CCMPSW_ENCDEC:
        value = vap->iv_ccmpsw_seldec;
        break;
    case IEEE80211_PARAM_UAPSDINFO:
        value = wlan_get_param(vap, IEEE80211_FEATURE_UAPSD);
        break;
    case IEEE80211_PARAM_NETWORK_SLEEP:
        value= (u_int32_t)wlan_get_powersave(vap);
        break;
#ifdef ATHEROS_LINUX_PERIODIC_SCAN
    case IEEE80211_PARAM_PERIODIC_SCAN:
        value = osifp->os_periodic_scan_period;
        break;
#endif
#if ATH_SW_WOW
    case IEEE80211_PARAM_SW_WOW:
        value = wlan_get_wow(vap);
        break;
#endif
    case IEEE80211_PARAM_AMPDU:
        value = ((ic->ic_flags_ext & IEEE80211_FEXT_AMPDU) != 0);
        break;

    case IEEE80211_PARAM_COUNTRY_IE:
        value = wlan_get_param(vap, IEEE80211_FEATURE_IC_COUNTRY_IE);
        break;

    case IEEE80211_PARAM_CHANBW:
        switch(ic->ic_chanbwflag)
        {
        case IEEE80211_CHAN_HALF:
            value = 1;
            break;
        case IEEE80211_CHAN_QUARTER:
            value = 2;
            break;
        default:
            value = 0;
            break;
        }
        break;
    case IEEE80211_PARAM_MFP_TEST:
        value = wlan_get_param(vap, IEEE80211_FEATURE_MFP_TEST);
        break;

#if UMAC_SUPPORT_TDLS
    case IEEE80211_PARAM_TDLS_ENABLE:
        value = vap->iv_ath_cap & IEEE80211_ATHC_TDLS?1:0;
        break;
#if CONFIG_RCPI
        case IEEE80211_PARAM_TDLS_GET_RCPI:
            /* write the values from vap */

            value = vap->iv_ic->ic_tdls->hithreshold;
        break;
#endif /* CONFIG_RCPI */
#endif /* UMAC_SUPPORT_TDLS */
    case IEEE80211_PARAM_INACT:
        value = wlan_get_param(vap, IEEE80211_RUN_INACT_TIMEOUT);
        break;
    case IEEE80211_PARAM_INACT_AUTH:
        value = wlan_get_param(vap, IEEE80211_AUTH_INACT_TIMEOUT);
        break;
    case IEEE80211_PARAM_INACT_INIT:
        value = wlan_get_param(vap, IEEE80211_INIT_INACT_TIMEOUT);
        break;
    case IEEE80211_PARAM_PWRTARGET:
        value = wlan_get_param(vap, IEEE80211_DEVICE_PWRTARGET);
        break;
    case IEEE80211_PARAM_COMPRESSION:
        value = wlan_get_param(vap, IEEE80211_COMP);
        break;
    case IEEE80211_PARAM_FF:
        value = wlan_get_param(vap, IEEE80211_FF);
        break;
    case IEEE80211_PARAM_TURBO:
        value = wlan_get_param(vap, IEEE80211_TURBO);
        break;
    case IEEE80211_PARAM_BURST:
        value = wlan_get_param(vap, IEEE80211_BURST);
        break;
    case IEEE80211_PARAM_AR:
        value = wlan_get_param(vap, IEEE80211_AR);
        break;
    case IEEE80211_PARAM_SLEEP:
        value = wlan_get_param(vap, IEEE80211_SLEEP);
        break;
    case IEEE80211_PARAM_EOSPDROP:
        value = wlan_get_param(vap, IEEE80211_EOSPDROP);
        break;
    case IEEE80211_PARAM_MARKDFS:
		value = wlan_get_param(vap, IEEE80211_MARKDFS);
        break;
    case IEEE80211_PARAM_WDS_AUTODETECT:
        value = wlan_get_param(vap, IEEE80211_WDS_AUTODETECT);
        break;
    case IEEE80211_PARAM_WEP_TKIP_HT:
        value = wlan_get_param(vap, IEEE80211_WEP_TKIP_HT);
        break;
    /*
    ** Support for returning the radio number
    */
    case IEEE80211_PARAM_ATH_RADIO:
		value = wlan_get_param(vap, IEEE80211_ATH_RADIO);
        break;
    case IEEE80211_PARAM_IGNORE_11DBEACON:
        value = wlan_get_param(vap, IEEE80211_IGNORE_11DBEACON);
        break;

#if ATH_RXBUF_RECYCLE   
    case IEEE80211_PARAM_RXBUF_LIFETIME:
        value = ic->ic_osdev->rxbuf_lifetime;
        break;
#endif

#if ATH_SUPPORT_WAPI
    case IEEE80211_PARAM_WAPIREKEY_USK:
        value = wlan_get_wapirekey_unicast(vap);
        break;
    case IEEE80211_PARAM_WAPIREKEY_MSK:
        value = wlan_get_wapirekey_multicast(vap);
        break;		
#endif
#ifdef QCA_PARTNER_PLATFORM
    case IEEE80211_PARAM_PLTFRM_PRIVATE:
        value = wlan_pltfrm_get_param(vap);
        break;
#endif 
	case IEEE80211_PARAM_NO_STOP_DISASSOC:
        if (value)
            osifp->no_stop_disassoc = 1;
        else
            osifp->no_stop_disassoc = 0;
        break;

#if UMAC_SUPPORT_VI_DBG
    case IEEE80211_PARAM_DBG_CFG:
        value = ieee80211_vi_dbg_get_param(vap, IEEE80211_VI_DBG_CFG);
        break;
    case IEEE80211_PARAM_DBG_NUM_STREAMS:
        value = ieee80211_vi_dbg_get_param(vap, IEEE80211_VI_DBG_NUM_STREAMS);
        break;
    case IEEE80211_PARAM_STREAM_NUM:
        value = ieee80211_vi_dbg_get_param(vap, IEEE80211_VI_STREAM_NUM);
	    break;
    case IEEE80211_PARAM_DBG_NUM_MARKERS:
        value = ieee80211_vi_dbg_get_param(vap, IEEE80211_VI_DBG_NUM_MARKERS);
        break;
    case IEEE80211_PARAM_MARKER_NUM:
        value = ieee80211_vi_dbg_get_param(vap, IEEE80211_VI_MARKER_NUM);
	    break;
    case IEEE80211_PARAM_MARKER_OFFSET_SIZE:
        value = ieee80211_vi_dbg_get_param(vap, IEEE80211_VI_MARKER_OFFSET_SIZE);  
        break;
    case IEEE80211_PARAM_MARKER_MATCH:
        value = ieee80211_vi_dbg_get_param(vap, IEEE80211_VI_MARKER_MATCH);  
        break;
    case IEEE80211_PARAM_RXSEQ_OFFSET_SIZE:
        value = ieee80211_vi_dbg_get_param(vap, IEEE80211_VI_RXSEQ_OFFSET_SIZE);  
        break;
    case IEEE80211_PARAM_RX_SEQ_RSHIFT:
        value = ieee80211_vi_dbg_get_param(vap, IEEE80211_VI_RX_SEQ_RSHIFT);
        break;
    case IEEE80211_PARAM_RX_SEQ_MAX:
        value = ieee80211_vi_dbg_get_param(vap, IEEE80211_VI_RX_SEQ_MAX);
        break;
    case IEEE80211_PARAM_RX_SEQ_DROP:
        value = ieee80211_vi_dbg_get_param(vap, IEEE80211_VI_RX_SEQ_DROP);
        break;	        
    case IEEE80211_PARAM_TIME_OFFSET_SIZE:
        value = ieee80211_vi_dbg_get_param(vap, IEEE80211_VI_TIME_OFFSET_SIZE);  
        break;
    case IEEE80211_PARAM_RESTART:
        value = ieee80211_vi_dbg_get_param(vap, IEEE80211_VI_RESTART);
        break;
    case IEEE80211_PARAM_RXDROP_STATUS:
        value = ieee80211_vi_dbg_get_param(vap, IEEE80211_VI_RXDROP_STATUS);
        break;
#endif
#if ATH_SUPPORT_FLOWMAC_MODULE
    case IEEE80211_PARAM_FLOWMAC:
        value = vap->iv_flowmac;
        break;
#endif
#if ATH_SUPPORT_IBSS_DFS
    case IEEE80211_PARAM_IBSS_DFS_PARAM:
        value = vap->iv_ibss_dfs_csa_threshold << 16 |
                   vap->iv_ibss_dfs_csa_measrep_limit << 8 |
                   vap->iv_ibss_dfs_enter_recovery_threshold_in_tbtt;
        printk("channel swith time %d measurement report %d recover time %d \n",
                 vap->iv_ibss_dfs_csa_threshold,
                 vap->iv_ibss_dfs_csa_measrep_limit ,
                 vap->iv_ibss_dfs_enter_recovery_threshold_in_tbtt);
        break;
#endif
#ifdef ATH_SUPPORT_TxBF
    case IEEE80211_PARAM_TXBF_AUTO_CVUPDATE:
        value = wlan_get_param(vap, IEEE80211_TXBF_AUTO_CVUPDATE);
        break;
    case IEEE80211_PARAM_TXBF_CVUPDATE_PER:
        value = wlan_get_param(vap, IEEE80211_TXBF_CVUPDATE_PER);
        break;
#endif
    case IEEE80211_PARAM_SCAN_BAND:
        value = osifp->os_scan_band;
        break;
    case IEEE80211_PARAM_ROAMING:
        value = ic->ic_roaming;
        break;
    default:
#if ATHEROS_LINUX_P2P_DRIVER
        retv = ieee80211_ioctl_getp2p(dev, info, w, extra);
#else
		retv = EOPNOTSUPP;
#endif
    }

    ptr->val = value;
    ptr->param = 0;

    return retv;
}


int
acfg_set_wifi_param(void *ctx, a_uint16_t cmdid,
                   a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_param_req_t        *ptr;
    acfg_os_req_t   *req = NULL;
	struct ath_softc_net80211 *scn =  ath_netdev_priv(dev);
    struct ath_softc          *sc  =  ATH_DEV_TO_SC(scn->sc_dev);
    struct ath_hal            *ah =   sc->sc_ah;
	int param, value, retval = 0;


    req = (acfg_os_req_t *) buffer;
    ptr  = &req->data.param_req;
	param = ptr->param;
	value = ptr->val;

    if ( param & ATH_PARAM_SHIFT )
    {
        /*
        ** It's an ATH value.  Call the  ATH configuration interface
        */
        
        param -= ATH_PARAM_SHIFT;
        retval = scn->sc_ops->ath_set_config_param(scn->sc_dev,
                                                   (ath_param_ID_t)param,
                                                   &value);
    }
    else if ( param & SPECIAL_PARAM_SHIFT )
    {
        if ( param == (SPECIAL_PARAM_SHIFT | SPECIAL_PARAM_COUNTRY_ID) ) {
            int j; 
            for(j=0; j < sizeof(staOnlyCountryCodes)/sizeof(staOnlyCountryCodes[0]); j++) {
                if(staOnlyCountryCodes[j] == value) {
                    retval = -EOPNOTSUPP;
                    return (retval);
                }
            }
            if (sc->sc_ieee_ops->set_countrycode) {
                retval = sc->sc_ieee_ops->set_countrycode(
                    sc->sc_ieee, NULL, value, CLIST_NEW_COUNTRY);
            }
        } else if ( param  == (SPECIAL_PARAM_SHIFT | SPECIAL_PARAM_ASF_AMEM_PRINT) ) {
            asf_amem_status_print();
            if ( value ) {
                asf_amem_allocs_print(asf_amem_alloc_all, value == 1);
            }
        } else if (param  == (SPECIAL_PARAM_SHIFT | SPECIAL_PARAM_DISP_TPC) ) {
            ath_hal_display_tpctables(ah);
        } else {
            retval = -EOPNOTSUPP;
        }
    }
    else
    {
        retval = (int) ath_hal_set_config_param(
            ah, (HAL_CONFIG_OPS_PARAMS_TYPE)param, &value);
    }

    return (retval);    
}



int
acfg_get_wifi_param(void *ctx, a_uint16_t cmdid,
                   a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_param_req_t        *ptr;
    acfg_os_req_t   *req = NULL;
	struct ath_softc_net80211 *scn  = ath_netdev_priv(dev);
    struct ath_softc          *sc   = ATH_DEV_TO_SC(scn->sc_dev);
    struct ath_hal            *ah   = sc->sc_ah;
	int param, *val, retval = 0;

    req = (acfg_os_req_t *) buffer;
    ptr  = &req->data.param_req;
	val = &ptr->val;	
	param = ptr->param;

	if ( param & ATH_PARAM_SHIFT )
    {
        /*
 *         ** It's an ATH value.  Call the  ATH configuration interface
 *                 */

        param -= ATH_PARAM_SHIFT;
        if ( scn->sc_ops->ath_get_config_param(scn->sc_dev,
										(ath_param_ID_t)param,val))
        {
            retval = -EOPNOTSUPP;
        }
    }
	 else if ( param & SPECIAL_PARAM_SHIFT )
    {
        if ( param == (SPECIAL_PARAM_SHIFT | SPECIAL_PARAM_COUNTRY_ID) ) {
            HAL_COUNTRY_ENTRY         cval;

            scn->sc_ops->get_current_country(scn->sc_dev, &cval);
            val[0] = cval.countryCode;
        } else {
            retval = -EOPNOTSUPP;
        }
    }
    else
    {
        if ( !ath_hal_get_config_param(ah, 
						(HAL_CONFIG_OPS_PARAMS_TYPE)param, val) )
        {
            retval = -EOPNOTSUPP;
        }
    }

    ptr->param = 0;
    return (retval);

}


int
acfg_get_rate(void *ctx, a_uint16_t cmdid,
             a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    u_int32_t        *ptr;
    acfg_os_req_t   *req = NULL;
	osif_dev *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = NETDEV_TO_VAP(dev);
    struct ifmediareq imr;

    req = (acfg_os_req_t *) buffer;
    ptr     = &req->data.bitrate;
    memset(&imr, 0, sizeof(imr));
    osifp->os_media.ifm_status(dev, &imr);
	*ptr = wlan_get_maxphyrate(vap);

    return 0;
}


int
acfg_set_phymode(void *ctx, a_uint16_t cmdid,
                a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_os_req_t *req;
	osif_dev *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
	char s[24];      /* big enough for ``11nght40plus'' */
    int mode, len = 0;

    req = (acfg_os_req_t *) buffer;
	len = strlen(phymode_strings[req->data.phymode]) + 1;
	if (len > sizeof(s)) {
		len = sizeof(s);
	}
	memcpy(s, (char *)phymode_strings[req->data.phymode], len);
	s[sizeof(s)-1] = '\0';

	mode = ieee80211_convert_mode(s);
    if (mode < 0)
        return -EINVAL;

#if ATH_SUPPORT_IBSS_HT
    /*
 *      * config ic adhoc ht capability
 *           */
    if (vap->iv_opmode == IEEE80211_M_IBSS) {

        wlan_dev_t ic = wlan_vap_get_devhandle(vap);

        switch (mode) {
        case IEEE80211_MODE_11NA_HT20:
        case IEEE80211_MODE_11NG_HT20:
            /* enable adhoc ht20 and aggr */
            wlan_set_device_param(ic, IEEE80211_DEVICE_HT20ADHOC, 1);
            wlan_set_device_param(ic, IEEE80211_DEVICE_HT40ADHOC, 0);
            break;
        case IEEE80211_MODE_11NA_HT40PLUS:
        case IEEE80211_MODE_11NA_HT40MINUS:
        case IEEE80211_MODE_11NG_HT40PLUS:  
        case IEEE80211_MODE_11NG_HT40MINUS:
		 /* enable adhoc ht40 and aggr */
            wlan_set_device_param(ic, IEEE80211_DEVICE_HT20ADHOC, 1);
            wlan_set_device_param(ic, IEEE80211_DEVICE_HT40ADHOC, 1);
            break;
        default:
            /* clear adhoc ht20, ht40, aggr */
            wlan_set_device_param(ic, IEEE80211_DEVICE_HT20ADHOC, 0);
            wlan_set_device_param(ic, IEEE80211_DEVICE_HT40ADHOC, 0);
            break;
        } /* end of switch (mode) */
    }
#endif /* end of #if ATH_SUPPORT_IBSS_HT */

    return wlan_set_desired_phymode(vap, mode);
}



int
acfg_set_enc(void *ctx, a_uint16_t cmdid,
            a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_encode_t        *ptr;
    acfg_os_req_t   *req = NULL;
	osif_dev *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
    ieee80211_keyval key_val;
    u_int16_t kid;
    int error = -EOPNOTSUPP;
    u_int8_t keydata[IEEE80211_KEYBUF_SIZE];
    int wepchange = 0;
	struct iw_point erq;


    req = (acfg_os_req_t *) buffer;
    ptr     = &req->data.encode;
	erq.flags = ptr->flags ;
    erq.length = ptr->len;


	if (ieee80211_crypto_wep_mbssid_enabled())
        wlan_set_param(vap, IEEE80211_WEP_MBSSID, 1);  /* wep keys will start from 4 in keycache for support wep multi-bssid */
    else
        wlan_set_param(vap, IEEE80211_WEP_MBSSID, 0);  /* wep keys will allocate index 0-3 in keycache */
	if ((ptr->flags & IW_ENCODE_DISABLED) == 0) {
        error = getiwkeyix(vap, &erq, &kid);
        if (error)
            return error;
        if (erq.length > IEEE80211_KEYBUF_SIZE)
            return -EINVAL;
		if (erq.length > 0)
        {
        	if (osifp->os_opmode == IEEE80211_M_IBSS) {
                    /* set authmode to IEEE80211_AUTH_OPEN */
            	siwencode_wep(dev);

                    /* set keymgmtset to WPA_ASE_NONE */
                wlan_set_rsn_cipher_param(vap, IEEE80211_KEYMGT_ALGS, 
												WPA_ASE_NONE);
            }
			OS_MEMCPY(keydata, (char *)ptr->buff, erq.length);
            memset(&key_val, 0, sizeof(ieee80211_keyval));
            key_val.keytype = IEEE80211_CIPHER_WEP;
            key_val.keydir = IEEE80211_KEY_DIR_BOTH;
            key_val.keylen = erq.length;
            key_val.keydata = keydata;
            key_val.macaddr = (u_int8_t *)ieee80211broadcastaddr;

            if (wlan_set_key(vap,kid,&key_val) != 0)
                return -EINVAL;

		}
		else 
		{
			wlan_set_default_keyid(vap,kid);
		}
		if (error == 0)
        {
			if (erq.length == 0 ||
                    (wlan_get_param(vap,IEEE80211_FEATURE_PRIVACY)) == 0)
			{
                    wlan_set_default_keyid(vap,kid);
			}
			wepchange = (wlan_get_param(vap,IEEE80211_FEATURE_PRIVACY)) == 0;
            wlan_set_param(vap,IEEE80211_FEATURE_PRIVACY, 1);
		}
	}
	else 
	{
		if (wlan_get_param(vap,IEEE80211_FEATURE_PRIVACY) == 0)
            return 0;
        wlan_set_param(vap,IEEE80211_FEATURE_PRIVACY, 0);
        wepchange = 1;
        error = 0;
	}
	if (error == 0)
    {
        /* Set policy for unencrypted frames */
        if ((erq.flags & IW_ENCODE_OPEN) &&
            (!(erq.flags & IW_ENCODE_RESTRICTED)))
        {
            wlan_set_param(vap,IEEE80211_FEATURE_DROP_UNENC, 0);
        }
        else if (!(erq.flags & IW_ENCODE_OPEN) &&
            (erq.flags & IW_ENCODE_RESTRICTED))
        {
            wlan_set_param(vap,IEEE80211_FEATURE_DROP_UNENC, 1);
        }
        else
        {
            /* Default policy */
            if (wlan_get_param(vap,IEEE80211_FEATURE_PRIVACY))
                wlan_set_param(vap,IEEE80211_FEATURE_DROP_UNENC, 1);
            else
                wlan_set_param(vap,IEEE80211_FEATURE_DROP_UNENC, 0);
        }
    }
	if (error == 0 && IS_UP(dev) && wepchange)
    {
        error = -osif_vap_init(dev, RESCAN);
    }
#ifdef ATH_SUPERG_XR
    /* set the same params on the xr vap device if exists */
    if(!error && vap->iv_xrvap && !(vap->iv_flags & IEEE80211_F_XR) )
        ieee80211_ioctl_siwencode(vap->iv_xrvap->iv_dev,info,&erq, 
													(char *)ptr->buff);
#endif
    return error;
}


int
acfg_set_rate(void *ctx, a_uint16_t cmdid,
             a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_rate_t        *ptr;
    acfg_os_req_t   *req = NULL;
	osif_dev *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
    int retv, value;

    req = (acfg_os_req_t *) buffer;
    ptr     = &req->data.rate;

	if (ptr->fixed) {
        unsigned int rate = ptr->value;
        if (rate >= 1000) {
            /* convert rate to index */
            int i;
            int array_size = sizeof(legacy_rate_idx)/sizeof(legacy_rate_idx[0]);
            rate /= 1000000;
            for (i = 0; i < array_size; i++) {
                if (rate == legacy_rate_idx[i][0]) {
                    rate = legacy_rate_idx[i][1];
                    break;
                }
            }
            if (i == array_size) return -EINVAL;
        }
        value = rate;
    } else {
		value = IEEE80211_FIXED_RATE_NONE;
	}
	retv = wlan_set_param(vap, IEEE80211_FIXED_RATE, value);
    if (EOK == retv) {
        if (value != IEEE80211_FIXED_RATE_NONE) {
            /* set default retries when setting fixed rate */
            retv = wlan_set_param(vap, IEEE80211_FIXED_RETRIES, 4);
        }
        else {
            retv = wlan_set_param(vap, IEEE80211_FIXED_RETRIES, 0);
        }
    }
    return -retv;
}


int
acfg_get_rssi(void *ctx, a_uint16_t cmdid,
             a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_rssi_t    *ptr;
    acfg_vendor_t    *vendor;
    acfg_os_req_t   *req_rssi = NULL;
    a_uint8_t *results = NULL;
	int fromvirtual = 0;
	struct iwreq req;

    req_rssi = (acfg_os_req_t *) buffer;
    ptr     = (acfg_rssi_t *)&req_rssi->data.rssi;


    memset(&req, 0, sizeof(struct iwreq));
    results = kzalloc(sizeof(acfg_vendor_t)+sizeof(acfg_rssi_t), GFP_KERNEL);

    vendor = (acfg_vendor_t *)results;
    vendor->cmd = IOCTL_GET_MASK | IEEE80211_IOCTL_RSSI;

    req.u.data.pointer = (a_uint8_t *)results;
    req.u.data.length  = sizeof(acfg_vendor_t)+sizeof(acfg_rssi_t);
    req.u.data.flags   = 1;
	if (strncmp(dev->name, "wifi", 4) == 0) {
		fromvirtual = 0;
	} else {
		fromvirtual = 1;
	}
#ifdef ATH_SUPPORT_LINUX_VENDOR
	osif_ioctl_vendor(dev, (struct ifreq *)&req, fromvirtual);
#endif
    memcpy((char *)ptr, ( (char *)results + sizeof(acfg_vendor_t)),
            req.u.data.length - sizeof (acfg_vendor_t));

    kfree (results);

    return 0;
}


int
acfg_set_testmode(void *ctx, a_uint16_t cmdid,
                 a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_testmode_t    *ptr;
    acfg_vendor_t *vendor;
    acfg_os_req_t   *req_testmode = NULL;
    struct iwreq req;
    a_uint8_t *results = NULL;
	int fromvirtual = 0;

    req_testmode = (acfg_os_req_t *) buffer;
    ptr = &req_testmode->data.testmode;



    memset(&req, 0, sizeof(req));
    results = kzalloc(sizeof(acfg_vendor_t) + sizeof(acfg_testmode_t),
                      GFP_KERNEL);

    vendor = (acfg_vendor_t  *)results;
    vendor->cmd = IOCTL_SET_MASK | IEEE80211_IOCTL_TESTMODE;

    memcpy(results+sizeof(acfg_vendor_t), ptr, sizeof(acfg_testmode_t));

    req.u.data.pointer = (a_uint8_t *)results;
    req.u.data.length  = sizeof(acfg_vendor_t)+sizeof(acfg_testmode_t);
    req.u.data.flags   = 1;

	if (strncmp(dev->name, "wifi", 4) == 0) {
		fromvirtual = 0;
	} else {
		fromvirtual = 1;
	}
#ifdef ATH_SUPPORT_LINUX_VENDOR
	osif_ioctl_vendor(dev, (struct ifreq *)&req, fromvirtual);
    if(status != A_STATUS_OK){
    }
#endif

    memcpy((void *)ptr, (void *)results+sizeof(acfg_vendor_t),
            req.u.data.length - sizeof (acfg_vendor_t));

    kfree(results);

    return 0;
}


int
acfg_get_testmode(void *ctx, a_uint16_t cmdid,
                 a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_testmode_t    *ptr;
    acfg_vendor_t *vendor;
    acfg_os_req_t   *req_testmode = NULL;
    struct iwreq req;
    a_uint8_t *results = NULL;
	int fromvirtual = 0;

    req_testmode = (acfg_os_req_t *) buffer;
    ptr = &req_testmode->data.testmode;


    memset(&req, 0, sizeof(req));
    results = kzalloc(sizeof(acfg_vendor_t) + sizeof(acfg_testmode_t),
                      GFP_KERNEL);

    vendor = (acfg_vendor_t  *)results;
    vendor->cmd = IOCTL_GET_MASK | IEEE80211_IOCTL_TESTMODE;

    memcpy(results+sizeof(acfg_vendor_t), ptr, sizeof(acfg_testmode_t));

    req.u.data.pointer = (a_uint8_t *)results;
    req.u.data.length  = sizeof(acfg_vendor_t)+sizeof(acfg_testmode_t);
    req.u.data.flags   = 1;

	if (strncmp(dev->name, "wifi", 4) == 0) {
		fromvirtual = 0;
	} else {
		fromvirtual = 1;
	}
#ifdef ATH_SUPPORT_LINUX_VENDOR
	osif_ioctl_vendor(dev, (struct ifreq *)&req, fromvirtual);

    if(status != A_STATUS_OK){
    }

#endif

    memcpy((void *)ptr, (void *)results + sizeof(acfg_vendor_t), 
            req.u.data.length - sizeof (acfg_vendor_t));

    kfree(results);

    return 0;
}


int
acfg_get_custdata(void *ctx, a_uint16_t cmdid,
                 a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_vendor_t    *vendor;
    acfg_custdata_t    *ptr_custdata;
    acfg_os_req_t   *req_custdata = NULL;
    struct iwreq req;
    a_uint8_t *results;
	int fromvirtual = 0;

    req_custdata = (acfg_os_req_t *) buffer;
    ptr_custdata     = &req_custdata->data.custdata;

    memset(&req, 0, sizeof(req));
    results = kzalloc(sizeof(acfg_vendor_t) + sizeof(acfg_custdata_t),
                      GFP_KERNEL);

    vendor = (acfg_vendor_t *)results;
    vendor->cmd = IOCTL_GET_MASK | IEEE80211_IOCTL_CUSTDATA;

    req.u.data.pointer = (a_uint8_t *)results;
    req.u.data.length  = sizeof(acfg_vendor_t) + sizeof(acfg_custdata_t);
    req.u.data.flags   = 1;

	if (strncmp(dev->name, "wifi", 4) == 0) {
		fromvirtual = 0;
	} else {
		fromvirtual = 1;
	}
#ifdef ATH_SUPPORT_LINUX_VENDOR
	osif_ioctl_vendor(dev, (struct ifreq *)&req, fromvirtual);

    if(status != A_STATUS_OK){
    }
#endif

    memcpy(ptr_custdata, ((char *)results + sizeof(acfg_vendor_t)),
            req.u.data.length - sizeof (acfg_vendor_t));

    kfree (results);

    return 0;
}


int
acfg_get_phymode(void *ctx, a_uint16_t cmdid,
                a_uint8_t *buffer, a_int32_t Length)
{
#define IEEE80211_MODE_TURBO_STATIC_A   IEEE80211_MODE_MAX
    struct net_device *dev = (struct net_device *) ctx;
    acfg_os_req_t *req;
    int i;
	 static const struct
    {
        char *name;
        int mode;
    } mappings[] = {
		 /* NB: need to order longest strings first for overlaps */
        { "11AST" , IEEE80211_MODE_TURBO_STATIC_A },
        { "AUTO"  , IEEE80211_MODE_AUTO },
        { "11A"   , IEEE80211_MODE_11A },
        { "11B"   , IEEE80211_MODE_11B },
        { "11G"   , IEEE80211_MODE_11G },
        { "FH"    , IEEE80211_MODE_FH },
        { "TA"      , IEEE80211_MODE_TURBO_A },
        { "TG"      , IEEE80211_MODE_TURBO_G },
        { "11NAHT20"        , IEEE80211_MODE_11NA_HT20 },
        { "11NGHT20"        , IEEE80211_MODE_11NG_HT20 },
        { "11NAHT40PLUS"    , IEEE80211_MODE_11NA_HT40PLUS },
        { "11NAHT40MINUS"   , IEEE80211_MODE_11NA_HT40MINUS },
        { "11NGHT40PLUS"    , IEEE80211_MODE_11NG_HT40PLUS },
        { "11NGHT40MINUS"   , IEEE80211_MODE_11NG_HT40MINUS },
        { "11NGHT40"        , IEEE80211_MODE_11NG_HT40},
        { "11NAHT40"        , IEEE80211_MODE_11NA_HT40},
        { "11ACVHT20" , IEEE80211_MODE_11AC_VHT20},
        { "11ACVHT40PLUS" , IEEE80211_MODE_11AC_VHT40PLUS},
        { "11ACVHT40MINUS" , IEEE80211_MODE_11AC_VHT40MINUS},
        { "11ACVHT40" , IEEE80211_MODE_11AC_VHT40},
        { "11ACVHT80" , IEEE80211_MODE_11AC_VHT80},
        { NULL }
    };
	osif_dev *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
	enum ieee80211_phymode  phymode;
    int j, length;

	

    req = (acfg_os_req_t *) buffer;

	phymode = wlan_get_desired_phymode(vap);

	for (i = 0; mappings[i].name != NULL ; i++)
    {
        if (phymode == mappings[i].mode)
        {
            length = strlen(mappings[i].name);
			for (j = 0; j < ACFG_PHYMODE_INVALID; j++) {
				if (strncmp (phymode_strings[j], 
								mappings[i].name, length) == 0)
				{
                	req->data.phymode = j;
					break;
				}
			}
        }
    }

    return 0;
#undef IEEE80211_MODE_TURBO_STATIC_A
}



int
acfg_acl_addmac(void *ctx, a_uint16_t cmdid,
               a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_macaddr_t    *ptr;
    acfg_os_req_t *req;
	osif_dev *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
	int rc;

    req = (acfg_os_req_t *) buffer;
    ptr     = &req->data.macaddr;

	rc = wlan_set_acl_add(vap, ptr->addr);
    return rc;
}


int
acfg_acl_getmac(void *ctx, a_uint16_t cmdid,
               a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_macacl_t    *ptr;
    acfg_os_req_t *req;
	osif_dev *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
	int rc=0;
    u_int8_t *macList;
    int i, num_mac;

    req = (acfg_os_req_t *) buffer;
    ptr     = &req->data.maclist;

    memset(ptr->macaddr, 0, sizeof(*ptr) - sizeof(ptr->num));
	macList = (u_int8_t *)OS_MALLOC(osifp->os_handle, 
								(IEEE80211_ADDR_LEN * 256), GFP_KERNEL);
	if (!macList) {
    	return EFAULT;
    }
	rc = wlan_get_acl_list(vap, macList, (IEEE80211_ADDR_LEN * 256), &num_mac);
	if(rc) {
    	if (macList) {
        	OS_FREE(macList);
        }
        return EFAULT;
    }
	for (i = 0; i < num_mac; i++) {
    	memcpy(ptr->macaddr[i], &macList[i * IEEE80211_ADDR_LEN], 
												IEEE80211_ADDR_LEN); 
    }
    ptr->num = num_mac;
	if (macList) {
    	OS_FREE(macList);
    }
    return rc;
}


int
acfg_acl_delmac(void *ctx, a_uint16_t cmdid,
               a_uint8_t *buffer, a_int32_t Length)
{
	
    struct net_device *dev = (struct net_device *) ctx;
    acfg_macaddr_t    *ptr;
    acfg_os_req_t *req;
	osif_dev *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
	int rc;

    req = (acfg_os_req_t *) buffer;
    ptr     = &req->data.macaddr;

	rc = wlan_set_acl_remove(vap, ptr->addr);
    return rc;
}


int
acfg_get_profile (void *ctx, a_uint16_t cmdid,
               a_uint8_t *buffer, a_int32_t Length)
{
    struct net_device *dev = (struct net_device *) ctx;
    acfg_os_req_t   *req = NULL;
    acfg_radio_vap_info_t *ptr;
    struct ieee80211_profile *profile;
	int error = 0;

    req = (acfg_os_req_t *) buffer;
    ptr     = &req->data.radio_vap_info;
    memset(ptr, 0, sizeof (acfg_radio_vap_info_t));
    profile = (struct ieee80211_profile *)kmalloc(
                            sizeof (struct ieee80211_profile), GFP_KERNEL);
    if (profile == NULL) {
    	return -ENOMEM;
    }
    OS_MEMSET(profile, 0, sizeof (struct ieee80211_profile));
    error = osif_ioctl_get_vap_info(dev, profile);
    acfg_convert_to_acfgprofile(profile, ptr);
    if (profile != NULL) {
    	kfree(profile);
        profile = NULL;
    }

    return 0;
}


acfg_dispatch_entry_t acfgdispatchentry[] =
{
    { 0,                      ACFG_REQ_FIRST_WIFI        , 0 }, //--> 0
    { acfg_vap_create,        ACFG_REQ_CREATE_VAP        , 0 },
    { acfg_set_wifi_param,    ACFG_REQ_SET_RADIO_PARAM   , 0 },
    { acfg_get_wifi_param,    ACFG_REQ_GET_RADIO_PARAM   , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { acfg_get_profile,       ACFG_REQ_GET_PROFILE       , 0 },
    { 0,                      ACFG_REQ_LAST_WIFI         , 0 }, //--> 11
    { 0,                      ACFG_REQ_FIRST_VAP         , 0 }, //--> 12
    { acfg_set_ssid,          ACFG_REQ_SET_SSID          , 0 },
    { acfg_get_ssid,          ACFG_REQ_GET_SSID          , 0 },
    { acfg_set_chan,          ACFG_REQ_SET_CHANNEL       , 0 },
    { acfg_get_chan,          ACFG_REQ_GET_CHANNEL       , 0 },
    { acfg_vap_delete,        ACFG_REQ_DELETE_VAP        , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { acfg_get_opmode,        ACFG_REQ_GET_OPMODE        , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { acfg_set_freq,          ACFG_REQ_SET_FREQUENCY     , 0 },
    { acfg_get_freq,          ACFG_REQ_GET_FREQUENCY     , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { acfg_set_rts,           ACFG_REQ_SET_RTS           , 0 },
    { acfg_get_rts,           ACFG_REQ_GET_RTS           , 0 },
    { acfg_set_frag,          ACFG_REQ_SET_FRAG          , 0 },
    { acfg_get_frag,          ACFG_REQ_GET_FRAG          , 0 },
    { acfg_set_txpow,         ACFG_REQ_SET_TXPOW         , 0 },
    { acfg_get_txpow,         ACFG_REQ_GET_TXPOW         , 0 },
    { acfg_set_ap,            ACFG_REQ_SET_AP            , 0 },
    { acfg_get_ap,            ACFG_REQ_GET_AP            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { acfg_set_vap_param,     ACFG_REQ_SET_VAP_PARAM     , 0 },
    { acfg_get_vap_param,     ACFG_REQ_GET_VAP_PARAM     , 0 },
    { acfg_get_rate,          ACFG_REQ_GET_RATE          , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { acfg_set_phymode,       ACFG_REQ_SET_PHYMODE       , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { acfg_set_enc,           ACFG_REQ_SET_ENCODE        , 0 },
    { acfg_set_rate,          ACFG_REQ_SET_RATE          , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { acfg_get_rssi,          ACFG_REQ_GET_RSSI          , 0 },
    { acfg_set_reg,           ACFG_REQ_SET_REG           , 0 },
    { acfg_get_reg,           ACFG_REQ_GET_REG           , 0 },
    { acfg_set_testmode,      ACFG_REQ_SET_TESTMODE      , 0 },
    { acfg_get_testmode,      ACFG_REQ_GET_TESTMODE      , 0 },
    { acfg_get_custdata,      ACFG_REQ_GET_CUSTDATA      , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { acfg_get_phymode,       ACFG_REQ_GET_PHYMODE       , 0 },
    { acfg_acl_addmac,        ACFG_REQ_ACL_ADDMAC        , 0 },
    { acfg_acl_getmac,        ACFG_REQ_ACL_GETMAC        , 0 },
    { acfg_acl_delmac,        ACFG_REQ_ACL_DELMAC        , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    {acfg_is_offload_vap,     ACFG_REQ_IS_OFFLOAD_VAP    , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },  //--> security requests
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { 0,                      ACFG_REQ_UNUSED            , 0 },
    { acfg_acl_getmac,        ACFG_REQ_GET_MAC_ADDR      , 0 },
    { acfg_set_vap_vendor_param, ACFG_REQ_SET_VAP_VENDOR_PARAM, 0 },
    { acfg_get_vap_vendor_param, ACFG_REQ_GET_VAP_VENDOR_PARAM, 0 },
    { 0,                      ACFG_REQ_LAST_VAP          , 0 },  //--> 98
};



int
acfg_handle_ioctl(struct net_device *dev, void *data)
{
    acfg_os_req_t   *req = NULL;
    uint32_t    req_len = sizeof(struct acfg_os_req);
    int32_t status = 0;
    uint32_t i;
    bool cmd_found = false;
    req = kzalloc(req_len, GFP_KERNEL);
    if(!req)
        return ENOMEM;
    if(copy_from_user(req, data, req_len))
        goto done;
    /* Iterate the dispatch table to find the handler
     * for the command received from the user */
    for (i = 0; i < sizeof(acfgdispatchentry)/sizeof(acfgdispatchentry[0]); i++)
    { 
        if (acfgdispatchentry[i].cmdid == req->cmd) {
            cmd_found = true;
            break;
       }
    }
    if ((cmd_found == false) || (acfgdispatchentry[i].cmd_handler == NULL)) {
        status = -1;
        printk("ACFG ioctl CMD %d failed\n", acfgdispatchentry[i].cmdid);
        goto done;
    }
    status = acfgdispatchentry[i].cmd_handler(dev,
                    req->cmd, (uint8_t *)req, req_len);
    if(copy_to_user(data, req, req_len)) {
		printk("copy_to_user error\n");
        status = -1;
	}
done:
    kfree(req);
    return status;
}


void acfg_send_event(struct net_device *dev, acfg_event_type_t type, 
					acfg_event_data_t *acfg_event)
{
	acfg_os_event_t     event;
    acfg_assoc_t    *assoc;
    acfg_dauth_t    *deauth;
    acfg_auth_t     *auth;
    acfg_leave_ap_t   *node_leave_ap;
    acfg_disassoc_t    *disassoc;
    acfg_pbc_ev_t     *pbc;
	
	switch (type) {
		case WL_EVENT_TYPE_DISASSOC_AP:
        case WL_EVENT_TYPE_DISASSOC_STA:
            if (type == WL_EVENT_TYPE_DISASSOC_AP) {
                event.id = ACFG_EV_DISASSOC_AP;
            } else if (type == WL_EVENT_TYPE_DISASSOC_STA) {
                event.id = ACFG_EV_DISASSOC_STA;
            }
            disassoc = (void *)&event.data;
			disassoc->frame_send = 0;
            disassoc->reason = acfg_event->reason;
            if (acfg_event->downlink == 0) {
                disassoc->status = acfg_event->result;
                disassoc->frame_send = 1;
            }
            memcpy(disassoc->macaddr,  acfg_event->addr,
                                            ACFG_MACADDR_LEN);
            break;

		case WL_EVENT_TYPE_DEAUTH_AP:
        case WL_EVENT_TYPE_DEAUTH_STA:
            if (type == WL_EVENT_TYPE_DEAUTH_AP) {
                event.id = ACFG_EV_DEAUTH_AP;
            } else if (type == WL_EVENT_TYPE_DEAUTH_STA) {
                event.id = ACFG_EV_DEAUTH_STA;
            }
            deauth = (void *)&event.data;
			deauth->frame_send = 0;
            if (acfg_event->downlink == 0) {
                deauth->frame_send = 1;
                deauth->status = acfg_event->result;
            } else {
            	deauth->reason = acfg_event->reason;
			}
            memcpy(deauth->macaddr,  acfg_event->addr,
                                            ACFG_MACADDR_LEN);
            break;

		case WL_EVENT_TYPE_ASSOC_AP:
        case WL_EVENT_TYPE_ASSOC_STA:
            if (type == WL_EVENT_TYPE_ASSOC_AP) {
                event.id = ACFG_EV_ASSOC_AP;
            } else if (type == WL_EVENT_TYPE_ASSOC_STA) {
                event.id = ACFG_EV_ASSOC_STA;
            }
            assoc = (void *)&event.data;
			assoc->frame_send = 0;
            assoc->status = acfg_event->result;
            if (acfg_event->downlink == 0) {
                assoc->frame_send = 1;
            } else {
            	memcpy(assoc->bssid,  acfg_event->addr,
                                            ACFG_MACADDR_LEN);
			}
            break;

		case WL_EVENT_TYPE_AUTH_AP:
        case WL_EVENT_TYPE_AUTH_STA:
            if (type == WL_EVENT_TYPE_AUTH_AP) {
                event.id = ACFG_EV_AUTH_AP;
            } else if (type == WL_EVENT_TYPE_AUTH_STA) {
                event.id = ACFG_EV_AUTH_STA;
            }
            auth = (void *)&event.data;
			auth->frame_send = 0;
            auth->status = acfg_event->result;
            if (acfg_event->downlink == 0) {
                auth->frame_send = 1;
            } else {
            	memcpy(auth->macaddr, acfg_event->addr, ACFG_MACADDR_LEN);
			}

            break;

		case WL_EVENT_TYPE_NODE_LEAVE:
			event.id = ACFG_EV_NODE_LEAVE;
			node_leave_ap = (void *)&event.data;
			node_leave_ap->reason = acfg_event->reason;
			memcpy(node_leave_ap->mac,  acfg_event->addr, 
											ACFG_MACADDR_LEN);
			break;
	    case WL_EVENT_TYPE_PUSH_BUTTON:
			
			if (jiffies_to_msecs(jiffies - prev_jiffies)  < 2000 ) {
				prev_jiffies = jiffies;
				break;
			}
			prev_jiffies = jiffies;
            pbc = (void *)&event.data;
            event.id = ACFG_EV_PUSH_BUTTON;
            strncpy(pbc->ifname, dev->name, IFNAMSIZ);
            break;

		default:
			printk("Unknown event\n");
			return;
	}
    acfg_net_indicate_event(dev, &event, 0);
}
