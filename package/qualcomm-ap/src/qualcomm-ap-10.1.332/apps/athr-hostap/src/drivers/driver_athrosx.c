/*
 * Mac OS X Atheros driver interface
 * Copyright (c) 2009-2010, Atheros Communications
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "utils/includes.h"

#include <stddef.h>
#include <sys/ioctl.h>
#include <sys/kern_event.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <ifaddrs.h>
#define ieee80211_rx_status __renamed_ieee80211_rx_status
#include <drv_ioctl.h>
#include <ieee80211_ioctl.h>
#undef ieee80211_rx_status

#include <apple80211_ioctl.h>

#include "utils/common.h"
#include "utils/eloop.h"
#include "common/ieee802_11_defs.h"
#include "l2_packet/l2_packet.h"
#include "p2p/p2p.h"
#include "driver.h"


/* apple80211_ioctl.h structures without the version field */
#define AREQ(a) ((user_addr_t) ((u8 *) &(a) + sizeof((a).version)))
#define ALEN(a) (sizeof(a) - sizeof((a).version))

#define getbit(a, p) (((a)[(p) / 8] >> (p) % 8) & 0x01)


struct athrosx_data {
	void *ctx;
	char ifname[IFNAMSIZ];
	u8 own_addr[ETH_ALEN];
	char shared_ifname[IFNAMSIZ];
	int sock;
	int ev_sock;
	int athr_ev_sock;
	u32 athr_vendor_code;
	struct wpa_driver_capa capa;
	int ignore_scan_done;
	int report_probe_req;
	unsigned int pending_set_chan_freq;
	unsigned int pending_set_chan_dur;
	int group_iface_allocated;
	u8 group_iface[ETH_ALEN];
	int group_iface_setup;
	struct l2_packet_data *l2;
	int ifindex;
	int last_assoc_mode;

	u8 pending_rx_mgmt[2000];
	size_t pending_rx_mgmt_len;
	u32 pending_rx_mgmt_id;
};


static void athrosx_scan_timeout(void *eloop_ctx, void *timeout_ctx);
static int athrosx_deinit_ap(void *priv);
static int athrosx_get_freq(struct athrosx_data *drv, const char *ifname);


static int darwin_ifhwaddr(const char *ifname, u8 *addr)
{
	struct ifaddrs *ifap, *i;
	if (getifaddrs(&ifap) < 0) {
		wpa_printf(MSG_ERROR, "athrosx: getifaddrs failed: %s",
			   strerror(errno));
		return -1;
	}
	for (i = ifap; i; i = i->ifa_next) {
		if (i->ifa_addr && i->ifa_addr->sa_family == AF_LINK &&
		    os_strcmp(i->ifa_name, ifname) == 0)
			break;
	}
	if (i) {
		struct sockaddr_dl *dl = (struct sockaddr_dl *) i->ifa_addr;
		os_memcpy(addr, dl->sdl_data + dl->sdl_nlen, ETH_ALEN);
	}
	freeifaddrs(ifap);
	if (!i)
		return -1;
	return 0;
}


static const char * athrosx_ioc(int type)
{
	switch (type) {
	case IEEE80211_IOC_SSID:
		return "SSID";
	case IEEE80211_IOC_MLME:
		return "MLME";
	case IEEE80211_IOC_AUTHMODE:
		return "AUTHMODE";
	case IEEE80211_IOC_WPA:
		return "WPA";
	case IEEE80211_IOC_WPAKEY:
		return "WPAKEY";
	case IEEE80211_IOC_DELKEY:
		return "DELKEY";
	case IEEE80211_IOC_SCAN_FLUSH:
		return "SCAN_FLUSH";
	case IEEE80211_IOC_CHANNEL:
		return "CHANNEL";
	case IEEE80211_IOC_START_HOSTAP:
		return "START_HOSTAP";
	case IEEE80211_IOC_P2P_OPMODE:
		return "P2P_OPMODE";
	case IEEE80211_IOC_APPIEBUF:
		return "APPIEBUF";
	case IEEE80211_IOC_WPS_MODE:
		return "WPS_MODE";
	default:
		return "??";
	}
}


static int athrosx_get80211param_ifname(struct athrosx_data *drv,
					const char *ifname, int type, int *val)
{
	struct ieee80211req ifr;
	os_memset(&ifr, 0, sizeof(ifr));
	os_strlcpy(ifr.i_name, ifname, IFNAMSIZ);
	ifr.i_type = type;
	if (ioctl(drv->sock, SIOCG80211, &ifr) < 0) {
		wpa_printf(MSG_DEBUG, "athrosx: ioctl SIOCG80211 type=%d (%s) "
			   "failed: %d (%s)",
			   type, athrosx_ioc(type), errno, strerror(errno));
		return -1;
	}
	*val = ifr.i_val;
	return 0;
}


static int athrosx_get80211param(struct athrosx_data *drv, int type, int *val)
{
	return athrosx_get80211param_ifname(drv, drv->ifname, type, val);
}


static int athrosx_set80211(struct athrosx_data *drv, int type,
			    const void *arg, size_t len)
{
	struct ieee80211req ifr;
	os_memset(&ifr, 0, sizeof(ifr));
	os_strlcpy(ifr.i_name, drv->ifname, IFNAMSIZ);
	ifr.i_type = type;
	ifr.i_data = (void *) arg;
	ifr.i_len = len;
	if (ioctl(drv->sock, SIOCS80211, &ifr) < 0) {
		wpa_printf(MSG_DEBUG, "athrosx: ioctl SIOCS80211 type=%d (%s) "
			   "failed: %d (%s)",
			   type, athrosx_ioc(type), errno, strerror(errno));
		return -1;
	}
	return 0;
}


static int athrosx_set80211param_ifname(struct athrosx_data *drv,
					const char *ifname, int type, int arg)
{
	struct ieee80211req ifr;
	os_memset(&ifr, 0, sizeof(ifr));
	os_strlcpy(ifr.i_name, ifname, IFNAMSIZ);
	ifr.i_type = type;
	ifr.i_val = arg;
	if (ioctl(drv->sock, SIOCS80211, &ifr) < 0) {
		wpa_printf(MSG_DEBUG, "athrosx: ioctl SIOCS80211 type=%d (%s) "
			   "val=%d failed: %d (%s)",
			   type, athrosx_ioc(type), arg, errno,
			   strerror(errno));
		return -1;
	}
	return 0;
}


static int athrosx_set80211param(struct athrosx_data *drv, int type, int arg)
{
	return athrosx_set80211param_ifname(drv, drv->ifname, type, arg);
}


static int athrosx_set_ifflags(int sock, const char *ifname, int dev_up)
{
	struct ifreq ifr;

	os_memset(&ifr, 0, sizeof(ifr));
	os_strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
		perror("ioctl[SIOCGIFFLAGS]");
		return -1;
	}

	if (dev_up)
		ifr.ifr_flags |= IFF_UP;
	else
		ifr.ifr_flags &= ~IFF_UP;

	if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
		perror("ioctl[SIOCSIFFLAGS]");
		return -1;
	}

	return 0;
}


static int athrosx_get_caps(struct athrosx_data *drv)
{
	struct apple80211req req;
	struct apple80211_capability_data cap;

	os_memset(&req, 0, sizeof(req));
	os_strlcpy(req.req_if_name, drv->ifname, IFNAMSIZ);
	req.req_type = APPLE80211_IOC_CARD_CAPABILITIES;
	os_memset(&cap, 0, sizeof(cap));
	req.req_data = AREQ(cap);
	req.req_len = ALEN(cap);
	if (ioctl(drv->sock, SIOCGA80211, &req) < 0) {
		perror("ioctl[SIOCGA80211,APPLE80211_IOC_CARD_CAPABILITIES]");
		return -1;
	}
	wpa_printf(MSG_DEBUG, "athrosx: Capabilities:"
		   "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
		   getbit(cap.capabilities, APPLE80211_CAP_WEP) ?
		   " [WEP]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_TKIP) ?
		   " [TKIP]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_AES) ?
		   " [AES]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_AES_CCM) ?
		   " [AES_CCM]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_CKIP) ?
		   " [CKIP]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_IBSS) ?
		   " [IBSS]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_PMGT) ?
		   " [PMGT]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_HOSTAP) ?
		   " [HOSTAP]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_TXPMGT) ?
		   " [TXPMGT]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_SHSLOT) ?
		   " [SHSLOT]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_SHPREAMBLE) ?
		   " [SHPREAMBLE]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_MONITOR) ?
		   " [MONITOR]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_TKIPMIC) ?
		   " [TKIPMIC]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_WPA1) ?
		   " [WPA1]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_WPA2) ?
		   " [WPA2]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_WPA) ?
		   " [WPA]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_BURST) ?
		   " [BURST]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_WME) ?
		   " [WME]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_SHORT_GI_40MHZ) ?
		   " [SHORT_GI_40MHZ]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_SHORT_GI_20MHZ) ?
		   " [SHORT_GI_20MHZ]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_WOW) ?
		   " [WOW]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_TSN) ?
		   " [TSN]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_MULTI_SCAN) ?
		   " [MULTI_SCAN]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_BT_COEX) ?
		   " [COEX]" : "",
		   getbit(cap.capabilities, APPLE80211_CAP_ACT_FRM) ?
		   " [ACT_FRM]" : "");

	if (getbit(cap.capabilities, APPLE80211_CAP_WPA1))
		drv->capa.key_mgmt |= WPA_DRIVER_CAPA_KEY_MGMT_WPA |
			WPA_DRIVER_CAPA_KEY_MGMT_WPA_PSK;
	if (getbit(cap.capabilities, APPLE80211_CAP_WPA2))
		drv->capa.key_mgmt |= WPA_DRIVER_CAPA_KEY_MGMT_WPA2 |
			WPA_DRIVER_CAPA_KEY_MGMT_WPA2_PSK;
	if (getbit(cap.capabilities, APPLE80211_CAP_WEP))
		drv->capa.enc |= WPA_DRIVER_CAPA_ENC_WEP40 |
			WPA_DRIVER_CAPA_ENC_WEP104;
	if (getbit(cap.capabilities, APPLE80211_CAP_TKIP))
		drv->capa.enc |= WPA_DRIVER_CAPA_ENC_TKIP;
	if (getbit(cap.capabilities, APPLE80211_CAP_AES_CCM))
		drv->capa.enc |= WPA_DRIVER_CAPA_ENC_CCMP;
	drv->capa.auth = WPA_DRIVER_AUTH_OPEN |
		WPA_DRIVER_AUTH_SHARED |
		WPA_DRIVER_AUTH_LEAP;
	drv->capa.max_scan_ssids = MAX_SCANREQ_SSID;
	drv->capa.flags |= WPA_DRIVER_FLAGS_P2P_CONCURRENT;
	if (getbit(cap.capabilities, APPLE80211_CAP_HOSTAP))
		drv->capa.flags |= WPA_DRIVER_FLAGS_AP;
	drv->capa.max_remain_on_chan = 1000;

	return 0;
}


static void athrosx_kernel_event(int sock, void *eloop_ctx, void *sock_ctx)
{
	/* struct athrosx_data *drv = eloop_ctx; */
	u8 buf[1000], *data;
	size_t data_len;
	int len;
	struct kern_event_msg *hdr;

	len = recv(sock, buf, sizeof(buf), 0);
	if (len < 0) {
		perror("recv[SYSPROTO_EVENT]");
		return;
	}

	if (len < KEV_MSG_HEADER_SIZE) {
		wpa_printf(MSG_DEBUG, "athrosx: Too short kernel event "
			   "message> len=%d", len);
		return;
	}
	hdr = (struct kern_event_msg *) buf;
	if (hdr->total_size != len) {
		wpa_printf(MSG_DEBUG, "athrosx: Kernel event message length "
			   "mismatch: %d != %d", hdr->total_size, len);
		if (hdr->total_size > len ||
		    hdr->total_size < KEV_MSG_HEADER_SIZE)
			return;
	}
	if (hdr->vendor_code != KEV_VENDOR_APPLE ||
	    hdr->kev_class != KEV_IEEE80211_CLASS ||
	    hdr->kev_subclass != KEV_APPLE80211_EVENT_SUBCLASS) {
		wpa_printf(MSG_DEBUG, "athrosx: Unrecognized kernel event: "
			   "vendor %d class %d subclass %d",
			   hdr->vendor_code, hdr->kev_class,
			   hdr->kev_subclass);
		return;
	}

	wpa_printf(MSG_DEBUG, "athrosx: Received kernel event message: id %u "
		   "code %u", hdr->id, hdr->event_code);
	data = (u8 *) hdr->event_data;
	data_len = hdr->total_size - KEV_MSG_HEADER_SIZE;
	wpa_hexdump(MSG_MSGDUMP, "athrosx: Kernel event data", data, data_len);

	switch (hdr->event_code) {
	case APPLE80211_M_POWER_CHANGED:
		wpa_printf(MSG_DEBUG, "athrosx: POWER_CHANGED");
		break;
	case APPLE80211_M_SSID_CHANGED:
		wpa_printf(MSG_DEBUG, "athrosx: SSID_CHANGED");
		break;
	case APPLE80211_M_BSSID_CHANGED:
		wpa_printf(MSG_DEBUG, "athrosx: BSSID_CHANGED");
		break;
	case APPLE80211_M_LINK_CHANGED:
		wpa_printf(MSG_DEBUG, "athrosx: LINK_CHANGED");
		break;
	case APPLE80211_M_MIC_ERROR_UCAST:
		wpa_printf(MSG_DEBUG, "athrosx: MIC_ERROR_UCAST");
		break;
	case APPLE80211_M_MIC_ERROR_MCAST:
		wpa_printf(MSG_DEBUG, "athrosx: MIC_ERROR_MCAST");
		break;
	case APPLE80211_M_INT_MIT_CHANGED:
		wpa_printf(MSG_DEBUG, "athrosx: INT_MIT_CHANGED");
		break;
	case APPLE80211_M_MODE_CHANGED:
		wpa_printf(MSG_DEBUG, "athrosx: MODE_CHANGED");
		break;
	case APPLE80211_M_ASSOC_DONE:
		wpa_printf(MSG_DEBUG, "athrosx: ASSOC_DONE");
		break;
	case APPLE80211_M_SCAN_DONE:
		wpa_printf(MSG_DEBUG, "athrosx: SCAN_DONE");
		break;
	case APPLE80211_M_COUNTRY_CODE_CHANGED:
		wpa_printf(MSG_DEBUG, "athrosx: COUNTRY_CODE_CHANGED");
		break;
	case APPLE80211_M_STA_ARRIVE:
		wpa_printf(MSG_DEBUG, "athrosx: STA_ARRIVE");
		break;
	case APPLE80211_M_STA_LEAVE:
		wpa_printf(MSG_DEBUG, "athrosx: STA_LEAVE");
		break;
	case APPLE80211_M_DECRYPTION_FAILURE:
		wpa_printf(MSG_DEBUG, "athrosx: DECRYPTION_FAILURE");
		break;
	case APPLE80211_M_SCAN_CACHE_UPDATED:
		wpa_printf(MSG_DEBUG, "athrosx: SCAN_CACHE_UPDATED");
		break;
	case APPLE80211_M_INTERNAL_SCAN_DONE:
		wpa_printf(MSG_DEBUG, "athrosx: INTERNAL_SCAN_DONE");
		break;
	case APPLE80211_M_LINK_QUALITY:
		wpa_printf(MSG_DEBUG, "athrosx: LINK_QUALITY");
		break;
	case APPLE80211_M_IBSS_PEER_ARRIVED:
		wpa_printf(MSG_DEBUG, "athrosx: IBSS_PEER_ARRIVED");
		break;
	case APPLE80211_M_IBSS_PEER_LEFT:
		wpa_printf(MSG_DEBUG, "athrosx: IBSS_PEER_LEFT");
		break;
	case APPLE80211_M_RSN_HANDSHAKE_DONE:
		wpa_printf(MSG_DEBUG, "athrosx: RSN_HANDSHAKE_DONE");
		break;
	case APPLE80211_M_BT_COEX_CHANGED:
		wpa_printf(MSG_DEBUG, "athrosx: BT_COEX_CHANGED");
		break;
	case APPLE80211_M_P2P_PEER_DETECTED:
		wpa_printf(MSG_DEBUG, "athrosx: P2P_PEER_DETECTED");
		break;
	case APPLE80211_M_P2P_LISTEN_COMPLETE:
		wpa_printf(MSG_DEBUG, "athrosx: P2P_LISTEN_COMPLETE");
		break;
	case APPLE80211_M_P2P_SCAN_COMPLETE:
		wpa_printf(MSG_DEBUG, "athrosx: P2P_SCAN_COMPLETE");
		break;
	default:
		wpa_printf(MSG_DEBUG, "athrosx: Unrecognized kernel event "
			   "code %d", hdr->event_code);
		break;
	}
}


static void athrosx_ev_rx_mgmt(struct athrosx_data *drv,
			       u8 *data, size_t data_len)
{
	union wpa_event_data event;
	const struct ieee80211_mgmt *mgmt;
	u16 fc, stype;

	if (data_len < 4 + 24) {
		wpa_printf(MSG_DEBUG, "athrosx: Too short management frame");
		return;
	}
	mgmt = (const struct ieee80211_mgmt *) (data + 4);

	fc = le_to_host16(mgmt->frame_control);
	stype = WLAN_FC_GET_STYPE(fc);

	if (stype == WLAN_FC_STYPE_ACTION) {
		u32 val32;
		os_memset(&event, 0, sizeof(event));
		event.rx_action.da = mgmt->da;
		event.rx_action.sa = mgmt->sa;
		event.rx_action.bssid = mgmt->bssid;
		event.rx_action.category = mgmt->u.action.category;
		event.rx_action.data = &mgmt->u.action.category + 1;
		event.rx_action.len = data + data_len - event.rx_action.data;
		os_memcpy(&val32, data, 4);
		event.rx_action.freq = val32;
		wpa_supplicant_event(drv->ctx, EVENT_RX_ACTION, &event);
		return;
	}

	if (stype != WLAN_FC_STYPE_PROBE_REQ)
		return;
	if (!drv->report_probe_req)
		return;
	os_memset(&event, 0, sizeof(event));
	event.rx_mgmt.frame = data + 4;
	event.rx_mgmt.frame_len = data_len - 4;
	wpa_supplicant_event(drv->ctx, EVENT_RX_MGMT, &event);
}


static void athrosx_ev_rx_mgmt_frag(struct athrosx_data *drv, u32 id,
				    u8 *data, size_t data_len, int last)
{
	if (drv->pending_rx_mgmt_id == 0 && drv->pending_rx_mgmt_len == 0) {
		/* Start a new fragment */
	} else if (id != drv->pending_rx_mgmt_id + 1) {
		wpa_printf(MSG_DEBUG, "athrosx: Unexpected EV_RX_MGMT frag "
			   "id=%u (expected %u)",
			   id, drv->pending_rx_mgmt_id);
		drv->pending_rx_mgmt_id = 0;
		drv->pending_rx_mgmt_len = 0;
		return;
	}

	if (drv->pending_rx_mgmt_len + data_len > sizeof(drv->pending_rx_mgmt))
 	{
		wpa_printf(MSG_DEBUG, "athrosx: Too long EV_RX_MGMT frag");
		drv->pending_rx_mgmt_id = 0;
		drv->pending_rx_mgmt_len = 0;
		return;
	}

	drv->pending_rx_mgmt_id = id;

	os_memcpy(drv->pending_rx_mgmt + drv->pending_rx_mgmt_len,
		  data, data_len);
	drv->pending_rx_mgmt_len += data_len;
	wpa_printf(MSG_DEBUG, "athrosx: Pending RX MGMT len %u (last=%d)",
		   (int) drv->pending_rx_mgmt_len, last);
	if (last) {
		wpa_hexdump(MSG_MSGDUMP, "athrosx: Indicating reassembled "
			    "EV_RX_MGMT event",
			    drv->pending_rx_mgmt,
			    drv->pending_rx_mgmt_len);
		athrosx_ev_rx_mgmt(drv, drv->pending_rx_mgmt,
				   drv->pending_rx_mgmt_len);
		drv->pending_rx_mgmt_id = 0;
		drv->pending_rx_mgmt_len = 0;
	}
}


static void athrosx_ev_rx_mgmt_cont(struct athrosx_data *drv, u32 id,
				    u8 *data, size_t data_len)
{
	athrosx_ev_rx_mgmt_frag(drv, id, data, data_len, 0);
}


static void athrosx_ev_rx_mgmt_last(struct athrosx_data *drv, u32 id,
				    u8 *data, size_t data_len)
{
	athrosx_ev_rx_mgmt_frag(drv, id, data, data_len, 1);
}


static void athrosx_ev_p2p_send_action_cb(struct athrosx_data *drv,
					  u8 *data, size_t data_len)
{
	union wpa_event_data event;
	struct ieee80211_send_action_cb *sa;
	const struct ieee80211_hdr *hdr;
	u16 fc;

	if (data_len < sizeof(*sa) + 24) {
		wpa_printf(MSG_DEBUG, "athrosx: Too short event message");
		return;
	}

	sa = (struct ieee80211_send_action_cb *) data;

	hdr = (const struct ieee80211_hdr *) (sa + 1);
	fc = le_to_host16(hdr->frame_control);

	os_memset(&event, 0, sizeof(event));
	event.tx_status.type = WLAN_FC_GET_TYPE(fc);
	event.tx_status.stype = WLAN_FC_GET_STYPE(fc);
	event.tx_status.dst = sa->dst_addr;
	event.tx_status.data = (u8 *) hdr;
	event.tx_status.data_len = data_len - sizeof(*sa);
	event.tx_status.ack = sa->ack;
	wpa_supplicant_event(drv->ctx, EVENT_TX_STATUS, &event);
}


static void athrosx_ev_ap_sta_connected(struct athrosx_data *drv,
					u8 *data, size_t data_len)
{
	u8 *addr;
	struct ieee80211_mgmt *mgmt;
	u16 fc, stype;
	u8 *ie;

	if (data_len < ETH_ALEN)
		return;
	addr = data;
	wpa_printf(MSG_DEBUG, "athrosx: Connection event for " MACSTR,
		   MAC2STR(addr));
	data += ETH_ALEN;
	data_len -= ETH_ALEN;
	if (data_len < 24)
		return;

	mgmt = (struct ieee80211_mgmt *) data;
	fc = le_to_host16(mgmt->frame_control);
	stype = WLAN_FC_GET_STYPE(fc);
	if (stype == WLAN_FC_STYPE_ASSOC_REQ) {
		wpa_printf(MSG_DEBUG, "athrosx: Association Request");
		ie = mgmt->u.assoc_req.variable;
	} else if (stype == WLAN_FC_STYPE_REASSOC_REQ) {
		wpa_printf(MSG_DEBUG, "athrosx: Reassociation Request");
		ie = mgmt->u.reassoc_req.variable;
	} else
		return;

	if (ie > data + data_len)
		return;
	wpa_hexdump(MSG_MSGDUMP, "athrosx: (Re)AssocReq IEs",
		    ie, data + data_len - ie);
	drv_event_assoc(drv->ctx, addr, ie, data + data_len - ie,
			stype == WLAN_FC_STYPE_REASSOC_REQ);
}


static void athrosx_ev_ap_sta_connected_frag(struct athrosx_data *drv, u32 id,
				    u8 *data, size_t data_len, int last)
{
	if (drv->pending_rx_mgmt_id == 0 && drv->pending_rx_mgmt_len == 0) {
		/* Start a new fragment */
	} else if (id != drv->pending_rx_mgmt_id + 1) {
		wpa_printf(MSG_DEBUG, "athrosx: Unexpected "
			   "EV_AP_STA_CONNECTED frag id=%u (expected %u)",
			   id, drv->pending_rx_mgmt_id);
		drv->pending_rx_mgmt_id = 0;
		drv->pending_rx_mgmt_len = 0;
		return;
	}

	if (drv->pending_rx_mgmt_len + data_len > sizeof(drv->pending_rx_mgmt))
 	{
		wpa_printf(MSG_DEBUG, "athrosx: Too long EV_AP_STA_CONNECTED "
			   "frag");
		drv->pending_rx_mgmt_id = 0;
		drv->pending_rx_mgmt_len = 0;
		return;
	}

	drv->pending_rx_mgmt_id = id;

	os_memcpy(drv->pending_rx_mgmt + drv->pending_rx_mgmt_len,
		  data, data_len);
	drv->pending_rx_mgmt_len += data_len;
	wpa_printf(MSG_DEBUG, "athrosx: Pending EV_AP_STA_CONNECTED "
		   "len %u (last=%d)",
		   (int) drv->pending_rx_mgmt_len, last);
	if (last) {
		wpa_hexdump(MSG_MSGDUMP, "athrosx: Indicating reassembled "
			    "EV_AP_STA_CONNECTED event",
			    drv->pending_rx_mgmt,
			    drv->pending_rx_mgmt_len);
		athrosx_ev_ap_sta_connected(drv, drv->pending_rx_mgmt,
					    drv->pending_rx_mgmt_len);
		drv->pending_rx_mgmt_id = 0;
		drv->pending_rx_mgmt_len = 0;
	}
}


static void athrosx_ev_ap_sta_connected_cont(struct athrosx_data *drv, u32 id,
				    u8 *data, size_t data_len)
{
	athrosx_ev_ap_sta_connected_frag(drv, id, data, data_len, 0);
}


static void athrosx_ev_ap_sta_connected_last(struct athrosx_data *drv, u32 id,
				    u8 *data, size_t data_len)
{
	athrosx_ev_ap_sta_connected_frag(drv, id, data, data_len, 1);
}


static void athrosx_athr_kernel_event(int sock, void *eloop_ctx,
				      void *sock_ctx)
{
	struct athrosx_data *drv = eloop_ctx;
	u8 buf[2000], *data;
	size_t data_len;
	int len;
	struct kern_event_msg *hdr;
	union wpa_event_data event;
	int freq;

	len = recv(sock, buf, sizeof(buf), 0);
	if (len < 0) {
		perror("recv[SYSPROTO_EVENT]");
		return;
	}

	if (len < KEV_MSG_HEADER_SIZE) {
		wpa_printf(MSG_DEBUG, "athrosx: Too short kernel event "
			   "message> len=%d", len);
		return;
	}
	hdr = (struct kern_event_msg *) buf;
	if (hdr->total_size != len) {
		wpa_printf(MSG_DEBUG, "athrosx: Kernel event message length "
			   "mismatch: %d != %d", hdr->total_size, len);
		if (hdr->total_size > len ||
		    hdr->total_size < KEV_MSG_HEADER_SIZE)
			return;
	}

	wpa_printf(MSG_DEBUG, "athrosx: Atheros kernel event: "
		   "vendor %d class %d subclass %d",
		   hdr->vendor_code, hdr->kev_class,
		   hdr->kev_subclass);
	if (hdr->vendor_code != drv->athr_vendor_code ||
	    hdr->kev_class != KEV_ATHEROS_IEEE80211_CLASS) {
		wpa_printf(MSG_DEBUG, "athrosx: Ignore unrecogniced kernel "
			   "event");
		return;
	}

	if (hdr->kev_subclass != drv->ifindex)
		return;

	wpa_printf(MSG_DEBUG, "athrosx: Received Atheros kernel event "
		   "message: id %u code %u", hdr->id, hdr->event_code);
	data = (u8 *) hdr->event_data;
	data_len = hdr->total_size - KEV_MSG_HEADER_SIZE;
	wpa_hexdump(MSG_MSGDUMP, "athrosx: Kernel event data", data, data_len);

	switch (hdr->event_code) {
	case IEEE80211_EV_SCAN_DONE:
		if (drv->ignore_scan_done) {
			wpa_printf(MSG_DEBUG, "athrosx: EV_SCAN_DONE "
				   "(ignore)");
			break;
		}
		wpa_printf(MSG_DEBUG, "athrosx: EV_SCAN_DONE");
		eloop_cancel_timeout(athrosx_scan_timeout, drv, NULL);
		wpa_supplicant_event(drv->ctx, EVENT_SCAN_RESULTS, NULL);
		break;
	case IEEE80211_EV_CHAN_START: {
		u32 ev_freq, ev_dur;
		if (data_len < 8) {
			wpa_printf(MSG_DEBUG, "athrosx: Too short "
				   "EV_CHAN_START event");
			break;
		}
		os_memcpy(&ev_freq, data, 4);
		os_memcpy(&ev_dur, data + 4, 4);
		wpa_printf(MSG_DEBUG, "athrosx: EV_CHAN_START (freq=%u "
			   "dur=%u)", ev_freq, ev_dur);
		if (ev_freq != drv->pending_set_chan_freq) {
			wpa_printf(MSG_DEBUG, "athrosx: Ignore EV_CHAN_START -"
				   " pending_set_chan_freq=%u",
				   drv->pending_set_chan_freq);
			break;
		}
		if (drv->pending_set_chan_freq) {
			os_memset(&event, 0, sizeof(event));
			event.remain_on_channel.freq =
				drv->pending_set_chan_freq;
			event.remain_on_channel.duration =
				drv->pending_set_chan_dur;
			wpa_supplicant_event(drv->ctx, EVENT_REMAIN_ON_CHANNEL,
					     &event);
			drv->pending_set_chan_freq = 0;
		}
		break;
	}
	case IEEE80211_EV_CHAN_END: {
		u32 ev_freq, ev_dur, ev_reason;
		if (data_len < 12) {
			wpa_printf(MSG_DEBUG, "athrosx: Too short "
				   "EV_CHAN_END event");
			break;
		}
		os_memcpy(&ev_freq, data, 4);
		os_memcpy(&ev_dur, data + 4, 4);
		os_memcpy(&ev_reason, data + 8, 4);
		wpa_printf(MSG_DEBUG, "athrosx: EV_CHAN_END (freq=%u "
			   "dur=%u reason=%u)", ev_freq, ev_dur, ev_reason);

		os_memset(&event, 0, sizeof(event));
		event.remain_on_channel.freq = ev_freq;
		event.remain_on_channel.duration = ev_dur;
		wpa_supplicant_event(drv->ctx, EVENT_CANCEL_REMAIN_ON_CHANNEL,
				     &event);
		break;
	}
	case IEEE80211_EV_RX_MGMT:
		wpa_printf(MSG_DEBUG, "athrosx: EV_RX_MGMT");
		athrosx_ev_rx_mgmt(drv, data, data_len);
		break;
	case IEEE80211_EV_P2P_SEND_ACTION_CB:
		wpa_printf(MSG_DEBUG, "athrosx: EV_P2P_SEND_ACTION_CB");
		athrosx_ev_p2p_send_action_cb(drv, data, data_len);
		break;
	case IEEE80211_EV_AP_STA_CONNECTED:
		wpa_printf(MSG_DEBUG, "athrosx: EV_AP_STA_CONNECTED");
		athrosx_ev_ap_sta_connected(drv, data, data_len);
		break;
	case IEEE80211_EV_AP_STA_DISCONNECTED:
		wpa_printf(MSG_DEBUG, "athrosx: EV_AP_STA_DISCONNECTED");
		if (data_len >= ETH_ALEN)
			drv_event_disassoc(drv->ctx, data);
		break;
	case IEEE80211_EV_STA_CONNECTED:
		wpa_printf(MSG_DEBUG, "athrosx: EV_STA_CONNECTED");
		os_memset(&event, 0, sizeof(event));
		freq = athrosx_get_freq(drv, drv->ifname);
		event.assoc_info.freq = freq;
		wpa_supplicant_event(drv->ctx, EVENT_ASSOC,
				     freq > 0 ? &event : NULL);
		break;
	case IEEE80211_EV_STA_DISCONNECTED:
		wpa_printf(MSG_DEBUG, "athrosx: EV_STA_DISCONNECTED");
		wpa_supplicant_event(drv->ctx, EVENT_DISASSOC, NULL);
		break;
	case IEEE80211_EV_RX_MGMT_CONT:
		wpa_printf(MSG_DEBUG, "athrosx: EV_RX_MGMT_CONT");
		athrosx_ev_rx_mgmt_cont(drv, hdr->id, data, data_len);
		break;
	case IEEE80211_EV_RX_MGMT_LAST:
		wpa_printf(MSG_DEBUG, "athrosx: EV_RX_MGMT_LAST");
		athrosx_ev_rx_mgmt_last(drv, hdr->id, data, data_len);
		break;
	case IEEE80211_EV_AP_STA_CONNECTED_CONT:
		wpa_printf(MSG_DEBUG, "athrosx: EV_AP_STA_CONNECTED_CONT");
		athrosx_ev_ap_sta_connected_cont(drv, hdr->id, data, data_len);
		break;
	case IEEE80211_EV_AP_STA_CONNECTED_LAST:
		wpa_printf(MSG_DEBUG, "athrosx: EV_AP_STA_CONNECTED_LAST");
		athrosx_ev_ap_sta_connected_last(drv, hdr->id, data, data_len);
		break;
	default:
		wpa_printf(MSG_DEBUG, "athrosx: Unrecognized Atheros kernel "
			   "event code %d", hdr->event_code);
		break;
	}
}


static void athrosx_deinit(void *priv)
{
	struct athrosx_data *drv = priv;
	athrosx_deinit_ap(drv);
	if (drv->l2)
		l2_packet_deinit(drv->l2);
	eloop_cancel_timeout(athrosx_scan_timeout, drv, NULL);
	if (drv->ev_sock >= 0) {
		eloop_unregister_read_sock(drv->ev_sock);
		close(drv->ev_sock);
	}
	if (drv->athr_ev_sock >= 0) {
		eloop_unregister_read_sock(drv->athr_ev_sock);
		close(drv->athr_ev_sock);
	}
#if 0
	/*
	 * Setting the interface down can cause some problems with the
	 * current driver, so do not do this for now..
	 */
	athrosx_set_ifflags(drv->sock, drv->ifname, 0);
#endif
	if (drv->sock >= 0)
		close(drv->sock);
	os_free(drv);
}


static int athrosx_open_event_sock(struct athrosx_data *drv)
{
	struct kev_request req;
	struct kev_vendor_code vc;

	/* Atheros events */
	drv->athr_ev_sock = socket(PF_SYSTEM, SOCK_RAW, SYSPROTO_EVENT);
	if (drv->athr_ev_sock < 0) {
		perror("socket(PF_SYSTEM,SYSPROTO_EVENT)");
		return -1;
	}

	os_memset(&vc, 0, sizeof(vc));
	os_strlcpy(vc.vendor_string, "com.atheros", sizeof(vc.vendor_string));
	if (ioctl(drv->athr_ev_sock, SIOCGKEVVENDOR, &vc) < 0) {
		wpa_printf(MSG_ERROR, "athrosx: Could not obtain vendor code "
			   "for Atheros kernel events: %s", strerror(errno));
		return -1;
	}
	drv->athr_vendor_code = vc.vendor_code;
	wpa_printf(MSG_DEBUG, "athrosx: kev vendor code: %u",
		   drv->athr_vendor_code);

	os_memset(&req, 0, sizeof(req));
	req.vendor_code = drv->athr_vendor_code;
	req.kev_class = KEV_ANY_CLASS;
	req.kev_subclass = KEV_ANY_SUBCLASS;
	if (ioctl(drv->athr_ev_sock, SIOCSKEVFILT, &req) < 0) {
		perror("ioctl(SIOCSKEVFILT)");
		return -1;
	}

	if (eloop_register_read_sock(drv->athr_ev_sock,
				     athrosx_athr_kernel_event,
				     drv, NULL) < 0)
		return -1;

	/* Apple events */
	drv->ev_sock = socket(PF_SYSTEM, SOCK_RAW, SYSPROTO_EVENT);
	if (drv->ev_sock < 0) {
		perror("socket(PF_SYSTEM,SYSPROTO_EVENT)");
		return -1;
	}

	os_memset(&req, 0, sizeof(req));
	req.vendor_code = KEV_VENDOR_APPLE;
	req.kev_class = KEV_IEEE80211_CLASS;
	req.kev_subclass = KEV_APPLE80211_EVENT_SUBCLASS;
	if (ioctl(drv->ev_sock, SIOCSKEVFILT, &req) < 0) {
		perror("ioctl(SIOCSKEVFILT)");
		return -1;
	}

	if (eloop_register_read_sock(drv->ev_sock, athrosx_kernel_event, drv,
				     NULL) < 0)
		return -1;

	return 0;
}


static void athrosx_l2_read(void *ctx, const u8 *src_addr, const u8 *buf,
			    size_t len)
{
	struct athrosx_data *drv = ctx;
	wpa_printf(MSG_DEBUG, "athrosx: Received %d bytes from l2_packet "
		   "(mode=%d)", (int) len, drv->last_assoc_mode);
	if (drv->last_assoc_mode == IEEE80211_MODE_AP)
		drv_event_eapol_rx(drv->ctx, src_addr, buf, len);
}


static void * athrosx_init(void *ctx, const char *ifname)
{
	struct athrosx_data *drv;
	int opmode;

	drv = os_zalloc(sizeof(*drv));
	if (drv == NULL)
		return NULL;
	drv->ctx = ctx;
	os_strlcpy(drv->ifname, ifname, sizeof(drv->ifname));
	drv->ev_sock = -1;
	drv->ifindex = if_nametoindex(ifname);
	wpa_printf(MSG_DEBUG, "athrosx: ifname=%s ifindex=%d",
		   ifname, drv->ifindex);

	if (darwin_ifhwaddr(drv->ifname, drv->own_addr) < 0)
		goto err;

	drv->sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (drv->sock < 0) {
		perror("socket[PF_INET,SOCK_DGRAM]");
		goto err;
	}

	if (athrosx_get_caps(drv) < 0)
		goto err;

	if (athrosx_open_event_sock(drv) < 0)
		goto err;

	if (athrosx_set_ifflags(drv->sock, drv->ifname, 1) < 0)
		goto err;

	athrosx_set80211param(drv, IEEE80211_IOC_SCAN_FLUSH, 0);

	if (athrosx_get80211param(drv, IEEE80211_IOC_P2P_OPMODE, &opmode) < 0)
	{
		wpa_printf(MSG_DEBUG, "athrosx: Failed to get interface P2P "
			   "opmode");
		/* Assume this is not P2P Device interface */
		opmode = 0;
	} else {
		wpa_printf(MSG_DEBUG, "athrosx: Interface P2P opmode: 0x%x",
			   opmode);
	}
	if (opmode != IEEE80211_M_P2P_DEVICE) {
		drv->l2 = l2_packet_init(drv->ifname, NULL, ETH_P_EAPOL,
					 athrosx_l2_read, drv, 0);
		if (drv->l2 == NULL)
			goto err;
	} else
		drv->capa.flags |= WPA_DRIVER_FLAGS_P2P_DEDICATED_INTERFACE;

	if (opmode == IEEE80211_M_P2P_DEVICE || opmode == IEEE80211_M_P2P_GO ||
	    opmode == IEEE80211_M_P2P_CLIENT)
		drv->capa.flags |= WPA_DRIVER_FLAGS_P2P_CAPABLE;

	return drv;

err:
	athrosx_deinit(drv);
	return NULL;
}


static int athrosx_get_capa(void *priv, struct wpa_driver_capa *capa)
{
	struct athrosx_data *drv = priv;
	os_memcpy(capa, &drv->capa, sizeof(*capa));
	return 0;
}


static void athrosx_scan_timeout(void *eloop_ctx, void *timeout_ctx)
{
	struct athrosx_data *drv = eloop_ctx;
	wpa_printf(MSG_DEBUG, "Scan timeout - try to get results");
	wpa_supplicant_event(drv->ctx, EVENT_SCAN_RESULTS, NULL);
}


static int athrosx_scan(void *priv, struct wpa_driver_scan_params *params)
{
	struct athrosx_data *drv = priv;
	int timeout = 10;
	int ret = 0;
	size_t req_len;
	struct ieee80211_scan_req *req;
	size_t i;

	athrosx_set80211param(drv, IEEE80211_IOC_SCAN_FLUSH, 0);

	drv->ignore_scan_done = 0;

	req_len = sizeof(*req) + params->extra_ies_len;
	req = os_zalloc(req_len);
	if (req == NULL)
		return -1;
	if (params->extra_ies) {
		req->ie_len = params->extra_ies_len;
		os_memcpy(req + 1, params->extra_ies, params->extra_ies_len);
	}

	req->num_ssid = params->num_ssids;
	for (i = 0; i < params->num_ssids; i++) {
		req->ssid_len[i] = params->ssids[i].ssid_len;
		os_memcpy(req->ssid[i], params->ssids[i].ssid,
			  params->ssids[i].ssid_len);
	}

	if (params->freqs) {
		i = 0;
		while (params->freqs[i]) {
			req->freq[i] = params->freqs[i];
			i++;
			if (i == MAX_SCANREQ_FREQ)
				break;
		}
		req->num_freq = i;
	}

	if (athrosx_set80211(drv, IEEE80211_IOC_SCAN_REQ, req, req_len) < 0)
		ret = -1;

	os_free(req);

	wpa_printf(MSG_DEBUG, "Scan requested (ret=%d) - scan timeout %d "
		   "seconds", ret, timeout);
	eloop_cancel_timeout(athrosx_scan_timeout, drv, NULL);
	eloop_register_timeout(timeout, 0, athrosx_scan_timeout, drv, NULL);

	return ret;
}


static struct wpa_scan_results * athrosx_get_scan_results(void *priv)
{
	struct athrosx_data *drv = priv;
	struct ieee80211req ifr;
	u8 results[24 * 1024], *pos, *end, *next;
	struct ieee80211req_scan_result *sr;
	struct wpa_scan_results *res;
	struct wpa_scan_res **tmp;

	os_memset(&ifr, 0, sizeof(ifr));
	os_strlcpy(ifr.i_name, drv->ifname, IFNAMSIZ);
	ifr.i_type = IEEE80211_IOC_SCAN_RESULTS;
	ifr.i_data = results;
	ifr.i_len = sizeof(results);
	if (ioctl(drv->sock, SIOCG80211, &ifr) < 0) {
		perror("ioctl[SIOCG80211,SCAN_RESULTS]");
		return NULL;
	}
	if (ifr.i_len < 0) {
		wpa_printf(MSG_DEBUG, "athrosx: No scan results data "
			   "received");
		return NULL;
	}

	wpa_printf(MSG_DEBUG, "athrosx: Received %d bytes of scan results",
		   ifr.i_len);

	res = os_zalloc(sizeof(*res));
	if (res == NULL)
		return NULL;

	pos = results;
	end = pos + ifr.i_len;
	while (end - pos > sizeof(*sr)) {
		struct wpa_scan_res *r;

		sr = (struct ieee80211req_scan_result *) pos;
		next = pos + sr->isr_len;
		if (next > end)
			break;
		pos += sizeof(*sr);
		if (pos + sr->isr_ssid_len + sr->isr_ie_len > end)
			break;

		r = os_zalloc(sizeof(*r) + sr->isr_ie_len);
		if (r == NULL)
			break;
		os_memcpy(r->bssid, sr->isr_bssid, ETH_ALEN);
		r->freq = sr->isr_freq;
		if (r->freq > 0 && r->freq <= 13)
			r->freq = 2407 + r->freq * 5;
		else if (r->freq == 14)
			r->freq = 2484;
		else if (r->freq > 14 && r->freq < 200)
			r->freq = 5000 + r->freq * 5;
		r->beacon_int = sr->isr_intval;
		r->caps = sr->isr_capinfo;
		r->noise = sr->isr_noise;
		r->level = sr->isr_rssi;
		pos += sr->isr_ssid_len;
		r->ie_len = sr->isr_ie_len;
		os_memcpy(r + 1, pos, sr->isr_ie_len);
		tmp = os_realloc(res->res, (res->num + 1) *
				 sizeof(struct wpa_scan_res *));
		if (tmp == NULL) {
			os_free(r);
			break;
		}
		tmp[res->num++] = r;
		res->res = tmp;

		pos = next;
	}

	return res;
}


static int athrosx_set_auth_alg(struct athrosx_data *drv,
				enum wpa_key_mgmt key_mgmt_suite, int auth_alg)
{
	int authmode;
	if (key_mgmt_suite == KEY_MGMT_802_1X)
		authmode = IEEE80211_AUTH_8021X;
	else if (key_mgmt_suite == KEY_MGMT_PSK)
		authmode = IEEE80211_AUTH_WPA;
	else if ((auth_alg & WPA_AUTH_ALG_OPEN) &&
		 (auth_alg & WPA_AUTH_ALG_SHARED))
		authmode = IEEE80211_AUTH_AUTO;
	else if (auth_alg & WPA_AUTH_ALG_SHARED)
		authmode = IEEE80211_AUTH_SHARED;
	else
		authmode = IEEE80211_AUTH_OPEN;

	return athrosx_set80211param(drv, IEEE80211_IOC_AUTHMODE, authmode);
}


static int athrosx_set_ssid(struct athrosx_data *drv, const u8 *ssid,
			    size_t ssid_len)
{
	return athrosx_set80211(drv, IEEE80211_IOC_SSID, ssid, ssid_len);
}


static int athrosx_set_mlme(struct athrosx_data *drv, u8 op, u16 reason,
			    const u8 *addr)
{
	struct ieee80211req_mlme mlme;

	os_memset(&mlme, 0, sizeof(mlme));
	mlme.im_op = op;
	mlme.im_reason = reason;
	if (addr)
		os_memcpy(mlme.im_macaddr, addr, ETH_ALEN);

	return athrosx_set80211(drv, IEEE80211_IOC_MLME, &mlme, sizeof(mlme));
}


static int athrosx_set_cipher(struct athrosx_data *drv, int type,
			      enum wpa_cipher suite)
{
	int cipher;

	switch (suite) {
	case CIPHER_CCMP:
		cipher = IEEE80211_CIPHER_AES_CCM;
		break;
	case CIPHER_TKIP:
		cipher = IEEE80211_CIPHER_TKIP;
		break;
	case CIPHER_NONE:
		cipher = IEEE80211_CIPHER_NONE;
		break;
	default:
		return -1;
	}

	return athrosx_set80211param(drv, type, cipher);
}


static int athrosx_set_freq(struct athrosx_data *drv, int freq)
{
	struct ieee80211_ioc_channel chan;

	os_memset(&chan, 0, sizeof(chan));
	if (freq >= 2412 && freq <= 2484) {
		chan.channel = freq == 2484 ? 14 : (freq - 2407) / 5;
		chan.phymode = IEEE80211_MODE_11G;
	} else if (freq >= 5000 && freq <= 6000) {
		chan.channel = (freq - 5000) / 5;
		chan.phymode = IEEE80211_MODE_11A;
	} else
		return -1;

	return athrosx_set80211(drv, IEEE80211_IOC_CHANNEL, &chan,
				sizeof(chan));
}


static int athrosx_start_ap(struct athrosx_data *drv,
			    struct wpa_driver_associate_params *params)
{
	int opmode;
	/* FIX: only do this for P2P group interface(?) */
	if (athrosx_get80211param(drv, IEEE80211_IOC_P2P_OPMODE, &opmode) < 0)
		opmode = -1;
	if (opmode != IEEE80211_IOC_P2P_OPMODE &&
	    athrosx_set80211param(drv, IEEE80211_IOC_P2P_OPMODE,
				  IEEE80211_M_P2P_GO) < 0) {
		wpa_printf(MSG_ERROR, "athrosx: Failed to set interface "
			   "P2P opmode to GO");
		return -1;
	}

	if (params->uapsd >= 0) {
		wpa_printf(MSG_DEBUG, "athr: Set UAPSD for AP mode: %d",
			   params->uapsd);
		if (athrosx_set80211param(drv, IEEE80211_IOC_UAPSDINFO,
					  params->uapsd) < 0)
			return -1;
	}

	if (athrosx_set_freq(drv, params->freq) < 0)
		return -1;
	if (params->key_mgmt_suite == KEY_MGMT_PSK) {
		athrosx_set80211param(drv, IEEE80211_IOC_WPA, 2);
		athrosx_set80211param(drv, IEEE80211_IOC_AUTHMODE,
				      IEEE80211_AUTH_RSNA);
		athrosx_set80211param(drv, IEEE80211_IOC_KEYMGTALGS,
				      WPA_ASE_8021X_PSK);
	} else if (params->key_mgmt_suite == KEY_MGMT_NONE) {
		if (athrosx_set80211param(drv, IEEE80211_IOC_AUTHMODE,
					  IEEE80211_AUTH_OPEN) < 0)
			return -1;
	} else {
		wpa_printf(MSG_INFO, "athrosx: Unsupported AP key_mgmt_suite "
			   "%d", params->key_mgmt_suite);
		return -1;
	}

	athrosx_set80211param(drv, IEEE80211_IOC_WPS_MODE, 1);

	athrosx_set_cipher(drv, IEEE80211_IOC_UCASTCIPHER,
			   params->pairwise_suite);
	athrosx_set_cipher(drv, IEEE80211_IOC_MCASTCIPHER,
			   params->group_suite);

	/* TODO: IEs */
	if (athrosx_set_ssid(drv, params->ssid, params->ssid_len) < 0)
		return -1;
	if (athrosx_set80211param(drv, IEEE80211_IOC_START_HOSTAP, 0) < 0) {
		wpa_printf(MSG_DEBUG, "athrosx: Try START_HOSTAP again after "
			   "one second delay");
		os_sleep(1, 0);
		if (athrosx_set80211param(drv, IEEE80211_IOC_START_HOSTAP, 0) <
		    0)
		return -1;
	}
	return 0;
}


static int athrosx_associate(void *priv,
			     struct wpa_driver_associate_params *params)
{
	struct athrosx_data *drv = priv;
	int privacy;
	int ret = 0;

	drv->last_assoc_mode = params->mode;
	if (params->mode == IEEE80211_MODE_AP)
		return athrosx_start_ap(drv, params);

	if (params->p2p) {
		int opmode;
		if (athrosx_get80211param(drv, IEEE80211_IOC_P2P_OPMODE,
					  &opmode) < 0)
			opmode = -1;
		if (opmode != IEEE80211_M_P2P_CLIENT &&
		    athrosx_set80211param(drv, IEEE80211_IOC_P2P_OPMODE,
					  IEEE80211_M_P2P_CLIENT) < 0) {
			wpa_printf(MSG_ERROR, "athrosx: Failed to set "
				   "interface P2P opmode to client");
			return -1;
		}
	}

	if (params->uapsd >= 0) {
		wpa_printf(MSG_DEBUG, "athr: Set UAPSD for station mode: 0x%x",
			   params->uapsd);
		if (athrosx_set80211param(drv, IEEE80211_IOC_UAPSDINFO,
					  params->uapsd) < 0)
			return -1;
	}

	athrosx_set80211param(drv, IEEE80211_IOC_DROPUNENCRYPTED,
			      params->drop_unencrypted);
	if (params->key_mgmt_suite == KEY_MGMT_802_1X ||
	    params->key_mgmt_suite == KEY_MGMT_PSK) {
		if (athrosx_set80211param(drv, IEEE80211_IOC_WPA, 3) < 0)
			ret = -1;
	} else if (athrosx_set80211param(drv, IEEE80211_IOC_WPA, 0) < 0)
		ret = -1;

	privacy = params->pairwise_suite != CIPHER_NONE ||
		params->key_mgmt_suite != KEY_MGMT_NONE;
	if (athrosx_set80211param(drv, IEEE80211_IOC_PRIVACY, privacy) < 0)
		ret = -1;
	if (athrosx_set_auth_alg(drv, params->key_mgmt_suite, params->auth_alg)
	    < 0)
		ret = -1;
	if (params->wpa_ie_len) {
		if (athrosx_set_cipher(drv, IEEE80211_IOC_UCASTCIPHER,
				       params->pairwise_suite) < 0)
			ret = -1;
		if (athrosx_set_cipher(drv, IEEE80211_IOC_MCASTCIPHER,
				       params->group_suite) < 0)
			ret = -1;

		if (athrosx_set80211(drv, IEEE80211_IOC_OPTIE,
				     params->wpa_ie, params->wpa_ie_len) < 0)
			ret = -1;
	}
	if (athrosx_set_ssid(drv, params->ssid, params->ssid_len) < 0)
		ret = -1;
	if (athrosx_set_mlme(drv, IEEE80211_MLME_ASSOC, 0, params->bssid) < 0)
		ret = -1;

	return ret;
}


static int athrosx_get_bssid(void *priv, u8 *bssid)
{
	struct athrosx_data *drv = priv;
	struct ieee80211req ifr;

	os_memset(&ifr, 0, sizeof(ifr));
	os_strlcpy(ifr.i_name, drv->ifname, IFNAMSIZ);
	ifr.i_type = IEEE80211_IOC_BSSID;
	ifr.i_data = bssid;
	ifr.i_len = ETH_ALEN;
	if (ioctl(drv->sock, SIOCG80211, &ifr) < 0) {
		perror("ioctl[SIOCG80211,BSSID]");
		return -1;
	}
	return 0;
}


static int athrosx_get_ssid(void *priv, u8 *ssid)
{
	struct athrosx_data *drv = priv;
	struct ieee80211req ifr;

	os_memset(&ifr, 0, sizeof(ifr));
	os_strlcpy(ifr.i_name, drv->ifname, IFNAMSIZ);
	ifr.i_type = IEEE80211_IOC_SSID;
	ifr.i_data = ssid;
	ifr.i_len = 32;
	if (ioctl(drv->sock, SIOCG80211, &ifr) < 0) {
		perror("ioctl[SIOCG80211,SSID]");
		return -1;
	}
	return ifr.i_len;
}


static int athrosx_del_key(struct athrosx_data *drv,
			   const u8 *addr, int key_idx)
{
	struct ieee80211req_del_key dk;
	os_memset(&dk, 0, sizeof(dk));
	dk.idk_keyix = key_idx;
	if (addr)
		os_memcpy(dk.idk_macaddr, addr, ETH_ALEN);
	return athrosx_set80211(drv, IEEE80211_IOC_DELKEY, &dk, sizeof(dk));
}


static int athrosx_set_key(const char *ifname, void *priv, enum wpa_alg alg,
			   const u8 *addr, int key_idx, int set_tx,
			   const u8 *seq, size_t seq_len,
			   const u8 *key, size_t key_len)
{
	struct athrosx_data *drv = priv;
	struct ieee80211req_key k;

	if (alg == WPA_ALG_NONE)
		return athrosx_del_key(drv, addr, key_idx);

	os_memset(&k, 0, sizeof(k));
	switch (alg) {
	case WPA_ALG_WEP:
		k.ik_type = IEEE80211_CIPHER_WEP;
		break;
	case WPA_ALG_TKIP:
		k.ik_type = IEEE80211_CIPHER_TKIP;
		break;
	case WPA_ALG_CCMP:
		k.ik_type = IEEE80211_CIPHER_AES_CCM;
		break;
	default:
		wpa_printf(MSG_ERROR, "%s: unknown alg=%d", __func__, alg);
		return -1;
	}

	k.ik_flags = IEEE80211_KEY_RECV;
	if (set_tx)
		k.ik_flags |= IEEE80211_KEY_XMIT;

	if (addr == NULL ||
	    os_memcmp(addr, "\xff\xff\xff\xff\xff\xff", ETH_ALEN) == 0) {
		athrosx_set_cipher(drv, IEEE80211_IOC_MCASTCIPHER, alg);
		os_memset(k.ik_macaddr, 0xff, IEEE80211_ADDR_LEN);
#ifndef IEEE80211_KEY_GROUP
#define IEEE80211_KEY_GROUP 0x04
#endif
		k.ik_flags |= IEEE80211_KEY_GROUP;
		k.ik_keyix = key_idx;
	} else {
		athrosx_set_cipher(drv, IEEE80211_IOC_UCASTCIPHER, alg);
		os_memcpy(k.ik_macaddr, addr, IEEE80211_ADDR_LEN);
		k.ik_keyix = key_idx == 0 ? IEEE80211_KEYIX_NONE : key_idx;
	}
	if (k.ik_keyix != IEEE80211_KEYIX_NONE && set_tx)
		k.ik_flags |= IEEE80211_KEY_DEFAULT;
	k.ik_keylen = key_len;
	if (seq)
		os_memcpy(&k.ik_keyrsc, seq, seq_len);
	os_memcpy(k.ik_keydata, key, key_len);

	return athrosx_set80211(drv, IEEE80211_IOC_WPAKEY, &k, sizeof(k));
}


static int athrosx_send_action(void *priv, unsigned int freq,
			       unsigned int wait,
			       const u8 *dst, const u8 *src,
			       const u8 *bssid,
			       const u8 *data, size_t data_len, int no_cck)
{
	struct athrosx_data *drv = priv;
	struct ieee80211_p2p_send_action *act;
	int res;

	wpa_printf(MSG_DEBUG, "athrosx: IOC_P2P_SEND_ACTION dst=" MACSTR
		   " freq=%u MHz", MAC2STR(dst), freq);
	act = os_zalloc(sizeof(*act) + data_len);
	if (act == NULL)
		return -1;
	act->freq = freq;
	os_memcpy(act->dst_addr, dst, ETH_ALEN);
	os_memcpy(act->src_addr, src, ETH_ALEN);
	os_memcpy(act->bssid, bssid, ETH_ALEN);
	os_memcpy(act + 1, data, data_len);
	res = athrosx_set80211(drv, IEEE80211_IOC_P2P_SEND_ACTION,
			       act, sizeof(*act) + data_len);
	os_free(act);
	return res;
}


static int athrosx_remain_on_channel(void *priv, unsigned int freq,
				     unsigned int duration)
{
	struct athrosx_data *drv = priv;
	struct ieee80211_p2p_set_channel sc;
	int res;
	static unsigned int req_id = 0;

	req_id++;
	wpa_printf(MSG_DEBUG, "athrosx: IOC_P2P_SET_CHANNEL: req_id=%u "
		   "freq=%u MHz dur=%u msec", req_id, freq, duration);
	os_memset(&sc, 0, sizeof(sc));
	sc.freq = freq;
	sc.req_id = req_id;
	sc.channel_time = duration;
	res = athrosx_set80211(drv, IEEE80211_IOC_P2P_SET_CHANNEL,
			       &sc, sizeof(sc));
	if (res == 0) {
		drv->pending_set_chan_freq = freq;
		drv->pending_set_chan_dur = duration;
		/*
		 * IEEE80211_EV_CHAN_START event is generated once the driver
		 * has completed the request.
		 */
	} else
		drv->pending_set_chan_freq = 0;
	return res;
}


static int athrosx_cancel_remain_on_channel(void *priv)
{
	struct athrosx_data *drv = priv;
	wpa_printf(MSG_DEBUG, "athrosx: IOC_P2P_CANCEL_CHANNEL");
	drv->pending_set_chan_freq = 0;
	return athrosx_set80211param(drv, IEEE80211_IOC_P2P_CANCEL_CHANNEL, 0);
}


static int athrosx_probe_req_report(void *priv, int report)
{
	struct athrosx_data *drv = priv;
	drv->report_probe_req = report;
	return 0;
}


static int athrosx_if_add(void *priv,
			  enum wpa_driver_if_type type, const char *ifname,
			  const u8 *addr, void *bss_ctx, void **drv_priv,
			  char *force_ifname, u8 *if_addr,
			  const char *bridge)
{
	struct athrosx_data *drv = priv;

	/*
	 * TODO: Support dynamic interface add/remove operations. For now,
	 * the driver does not have enough functionality for this, so we have
	 * to do with a single hardcoded virtual interface.
	 */
#define P2P_GROUP_IFNAME "p2p1"

	if (drv->group_iface_allocated) {
		wpa_printf(MSG_INFO, "athrosx: No available virtual "
			   "interface");
		return -1;
	}

	if (!drv->group_iface_setup) {
		if (darwin_ifhwaddr(P2P_GROUP_IFNAME, drv->group_iface) < 0)
			return -1;
		wpa_printf(MSG_DEBUG, "athrosx: Group interface %s - "
			   MACSTR, P2P_GROUP_IFNAME,
			   MAC2STR(drv->group_iface));
		drv->group_iface_setup = 1;
	}

	os_memcpy(if_addr, drv->group_iface, ETH_ALEN);
	drv->group_iface_allocated = 1;
	os_snprintf(force_ifname, IFNAMSIZ, P2P_GROUP_IFNAME);

	switch (type) {
	case WPA_IF_P2P_CLIENT:
		return athrosx_set80211param_ifname(drv, force_ifname,
						    IEEE80211_IOC_P2P_OPMODE,
						    IEEE80211_M_P2P_CLIENT);
	case WPA_IF_P2P_GO:
		return athrosx_set80211param_ifname(drv, force_ifname,
						    IEEE80211_IOC_P2P_OPMODE,
						    IEEE80211_M_P2P_GO);
	case WPA_IF_P2P_GROUP:
		break;
	default:
		wpa_printf(MSG_ERROR, "athrosx: Unsupported if_add type %d",
			   type);
		return -1;
	}

	return 0;
}


static int athrosx_if_remove(void *priv, enum wpa_driver_if_type type,
			     const char *ifname)
{
	struct athrosx_data *drv = priv;

	/* TODO: Proper support for dynamic interface add/remove */
	if (os_strcmp(ifname, P2P_GROUP_IFNAME) != 0 ||
	    (type != WPA_IF_P2P_CLIENT && type != WPA_IF_P2P_GO &&
	     type != WPA_IF_P2P_GROUP)) {
		wpa_printf(MSG_DEBUG, "athrosx: if_remove failed: type=%d "
			   "ifname='%s'", type, ifname);
		return -1;
	}
	drv->group_iface_allocated = 0;
	return 0;
}


static int athrosx_hapd_send_eapol(void *priv, const u8 *addr, const u8 *data,
				   size_t data_len, int encrypt,
				   const u8 *own_addr, u32 flags)
{
	struct athrosx_data *drv = priv;
	if (!drv->l2)
		return -1;
	return l2_packet_send(drv->l2, addr, ETH_P_EAPOL, data,
			      data_len);
}


static int athrosx_set_appie(struct athrosx_data *drv, int frametype,
			     const struct wpabuf *ie)
{
	struct ieee80211req_getset_appiebuf *buf;
	size_t len = ie ? wpabuf_len(ie) : 0;
	int res;

	wpa_printf(MSG_DEBUG, "athrosx: Set APPIE frametype=%d len=%d",
		   frametype, (int) len);
	if (ie)
		wpa_hexdump_buf(MSG_MSGDUMP, "athrosx: IE buffer", ie);
	buf = os_zalloc(sizeof(*buf) + len);
	if (buf == NULL)
		return -1;
	buf->app_frmtype = frametype;
	buf->app_buflen = len;
	if (ie)
		os_memcpy(buf->app_buf, wpabuf_head(ie), len);
	res = athrosx_set80211(drv, IEEE80211_IOC_APPIEBUF,
			       buf, sizeof(*buf) + len);
	os_free(buf);

	return res;
}


static int athrosx_set_ap_wps_ie(void *priv, const struct wpabuf *beacon,
				 const struct wpabuf *proberesp)
{
	struct athrosx_data *drv = priv;
	if (athrosx_set_appie(drv, IEEE80211_APPIE_FRAME_BEACON, beacon) < 0)
		return -1;
	return athrosx_set_appie(drv, IEEE80211_APPIE_FRAME_PROBE_RESP,
				 proberesp);
}


static const u8 * athrosx_get_mac_addr(void *priv)
{
	struct athrosx_data *drv = priv;
	return drv->own_addr;
}


static int athrosx_deinit_ap(void *priv)
{
	struct athrosx_data *drv = priv;
	wpa_printf(MSG_DEBUG, "athrosx: Deinit AP mode");
	return athrosx_set_mlme(drv, IEEE80211_MLME_STOP_BSS, 0, NULL);
}


static int athrosx_get_freq(struct athrosx_data *drv, const char *ifname)
{
	int val, freq;

	if (athrosx_get80211param_ifname(drv, ifname, IEEE80211_IOC_CHANNEL,
					 &val) < 0) {
		wpa_printf(MSG_DEBUG, "athrosx: Could not fetch current "
			   "channel for the interface %s", ifname);
		return -1;
	}

	if (val >= 1 && val <= 13)
		freq = 2407 + 5 * val;
	else if (val == 14)
		freq = 2484;
	else if (val > 14 && val <= 255)
		freq = 5000 + 5 * val;
	else
		freq = -1;

	return freq;
}


static int athrosx_shared_freq(void *priv)
{
	struct athrosx_data *drv = priv;
	int val, freq;

	if (!drv->shared_ifname[0])
		return -1;

	if (athrosx_get80211param_ifname(drv, drv->shared_ifname,
					 IEEE80211_IOC_CONNECTION_STATE,
					 &val) < 0) {
		wpa_printf(MSG_DEBUG, "athrosx: Could not fetch connection "
			   "state for the shared interface %s",
			   drv->shared_ifname);
		return -1;
	}

	if (!val) {
		wpa_printf(MSG_DEBUG, "athrosx: Shared interface %s is "
			   "disconnected", drv->shared_ifname);
		return 0;
	}

	freq = athrosx_get_freq(drv, drv->shared_ifname);
	if (freq < 0)
		return -1;

	wpa_printf(MSG_DEBUG, "athrosx: Shared interface %s is connected on "
		   "channel %d (freq %d MHz)", drv->shared_ifname, val, freq);

	return freq;
}


static int athrosx_set_param(void *priv, const char *param)
{
	struct athrosx_data *drv = priv;
	const char *pos, *end;

	if (param == NULL)
		return 0;

	wpa_printf(MSG_DEBUG, "athrosx: Set param '%s'", param);
	pos = os_strstr(param, "shared_interface=");
	if (pos) {
		pos += 17;
		end = os_strchr(pos, ' ');
		if (end == NULL)
			end = pos + os_strlen(pos);
		if (end - pos >= IFNAMSIZ) {
			wpa_printf(MSG_ERROR, "athrosx: Too long "
				   "shared_interface name");
			return -1;
		}
		os_memcpy(drv->shared_ifname, pos, end - pos);
		drv->shared_ifname[end - pos] = '\0';
		wpa_printf(MSG_DEBUG, "athrosx: Shared interface: %s",
			   drv->shared_ifname);
		athrosx_shared_freq(drv);
	}

	return 0;
}


struct noa_descriptor {
	u32 type_count;
	u32 duration;
	u32 interval;
	u32 start_time;
};

#define MAX_NOA_DESCRIPTORS 4
struct p2p_noa {
	u32 curr_tsf32;
	u8 index;
	u8 opp_ps;
	u8 ctwindow;
	u8 num_desc;
	struct noa_descriptor noa_desc[MAX_NOA_DESCRIPTORS];
};


static int athrosx_get_noa(void *priv, u8 *buf, size_t buf_len)
{
	struct athrosx_data *drv = priv;
	struct ieee80211req ifr;
	struct p2p_noa noa;
	u8 *pos;
	int i;

	wpa_printf(MSG_DEBUG, "athrosx: Get NoA");
	os_memset(&ifr, 0, sizeof(ifr));
	os_memset(&noa, 0, sizeof(noa));
	os_strlcpy(ifr.i_name, drv->ifname, IFNAMSIZ);
	ifr.i_type = IEEE80211_IOC_P2P_NOA_INFO;
	ifr.i_data = &noa;
	ifr.i_len = sizeof(noa);
	if (ioctl(drv->sock, SIOCG80211, &ifr) < 0) {
		perror("ioctl[SIOCG80211,P2P_NOA_INFO]");
		return -1;
	}
	if (ifr.i_len < 0)
		return -1;

	if (!noa.opp_ps && noa.ctwindow == 0 && noa.num_desc == 0)
		return 0;

	if (buf_len < (size_t) 2 + 13 * noa.num_desc) {
		wpa_printf(MSG_DEBUG, "athrosx: No room for returning NoA");
		return -1;
	}

	buf[0] = noa.index;
	buf[1] = (noa.opp_ps ? 0x80 : 0) | (noa.ctwindow & 0x7f);
	pos = buf + 2;
	for (i = 0; i < noa.num_desc; i++) {
		*pos++ = noa.noa_desc[i].type_count & 0xff;
		WPA_PUT_LE32(pos, noa.noa_desc[i].duration);
		pos += 4;
		WPA_PUT_LE32(pos, noa.noa_desc[i].interval);
		pos += 4;
		WPA_PUT_LE32(pos, noa.noa_desc[i].start_time);
		pos += 4;
	}

	return pos - buf;
}


static int athrosx_set_noa(void *priv, u8 count, int start, int duration)
{
	struct athrosx_data *drv = priv;
	struct ieee80211_p2p_go_noa noa;

	wpa_printf(MSG_DEBUG, "athrosx: Set NoA %d %d %d",
		   count, start, duration);
	os_memset(&noa, 0, sizeof(noa));
	noa.num_iterations = count;
	noa.offset_next_tbtt = start;
	noa.duration = duration;
	return athrosx_set80211(drv, IEEE80211_IOC_P2P_GO_NOA,
				&noa, sizeof(noa));
}


static int athrosx_set_p2p_powersave(void *priv, int legacy_ps, int opp_ps,
				  int ctwindow)
{
	struct athrosx_data *drv = priv;
	int ret = 0;

	wpa_printf(MSG_DEBUG, "athr: Set P2P Power Save: legacy=%d opp=%d "
		   "ctwindow=%d", legacy_ps, opp_ps, ctwindow);

	if (legacy_ps != -1) {
		if (athrosx_set80211param(drv, IEEE80211_IOC_POWERSAVE,
					  legacy_ps) < 0)
			ret = -1;
	}

	if (opp_ps != -1) {
		if (athrosx_set80211param(drv, IEEE80211_IOC_P2P_GO_OPPPS,
					  opp_ps) < 0)
			ret = -1;
	}

	if (ctwindow != -1) {
		if (athrosx_set80211param(drv, IEEE80211_IOC_P2P_GO_CTWINDOW,
					  ctwindow) < 0)
			ret = -1;
	}

	return ret;
}


const struct wpa_driver_ops wpa_driver_athrosx_ops = {
	.name = "athrosx",
	.desc = "Mac OS X Atheros driver",
	.init = athrosx_init,
	.deinit = athrosx_deinit,
	.get_capa = athrosx_get_capa,
	.scan2 = athrosx_scan,
	.get_scan_results2 = athrosx_get_scan_results,
	.associate = athrosx_associate,
	.get_bssid = athrosx_get_bssid,
	.get_ssid = athrosx_get_ssid,
	.set_key = athrosx_set_key,
	.send_action = athrosx_send_action,
	.remain_on_channel = athrosx_remain_on_channel,
	.cancel_remain_on_channel = athrosx_cancel_remain_on_channel,
	.probe_req_report = athrosx_probe_req_report,
	.if_add = athrosx_if_add,
	.if_remove = athrosx_if_remove,
	.hapd_send_eapol = athrosx_hapd_send_eapol,
	.set_ap_wps_ie = athrosx_set_ap_wps_ie,
	.get_mac_addr = athrosx_get_mac_addr,
	.deinit_ap = athrosx_deinit_ap,
	.set_param = athrosx_set_param,
	.shared_freq = athrosx_shared_freq,
	.get_noa = athrosx_get_noa,
	.set_noa = athrosx_set_noa,
	.set_p2p_powersave = athrosx_set_p2p_powersave,
};
