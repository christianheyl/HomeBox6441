/*
 * Copyright (c) 2010, Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <osdep.h>
#include <if_llc.h>
#include <if_upperproto.h>
#include <if_media.h>
#include <ieee80211_var.h>
#include <ieee80211_defines.h>

#include <ieee80211_extap.h>
#include <ieee80211_mitbl.h>

#if 1
#define dbg4(...)
#else
#define dbg4(...)	IEEE80211_DPRINTF(NULL, 0, __VA_ARGS_)
#endif

#if 1
#define dbg6(...)
#else
#define dbg6(...)	IEEE80211_DPRINTF(NULL, 0, __VA_ARGS_)
#endif

static void ieee80211_extap_out_ipv4(wlan_if_t, struct ether_header *);
static int ieee80211_extap_out_arp(wlan_if_t, struct ether_header *);

#ifdef EXTAP_DEBUG
static void print_arp_pkt(const char *, int, struct ether_header *);
#endif

#define eaipv6	1
#if eaipv6
static void ieee80211_extap_in_ipv6(wlan_if_t, struct ether_header *);
static int ieee80211_extap_out_ipv6(wlan_if_t, struct ether_header *);
#ifdef EXTAP_DEBUG
static void print_ipv6_pkt(char *, wlan_if_t, struct ether_header *);
#endif
#endif

#ifdef EXTAP_DEBUG
static void
print_arp_pkt(const char *f, int l, struct ether_header *eh)
{
	eth_arphdr_t *arp = (eth_arphdr_t *)(eh + 1);

	if (arp->ar_op == ADF_ARP_REQ ||
	    arp->ar_op == ADF_ARP_RREQ) {
		eadbg1("%s(%d): arp request for " eaistr " from "
			eaistr "-" eamstr "\n", f, l,
			eaip(arp->ar_tip), eaip(arp->ar_sip),
			eamac(arp->ar_sha));
	}

	if (arp->ar_op == ADF_ARP_RSP || arp->ar_op == ADF_ARP_RRSP) {
		eadbg1("%s(%d): arp reply for " eaistr "-" eamstr " from "
			eaistr "-" eamstr "\n", f, l,
			eaip(arp->ar_tip), eamac(arp->ar_tha),
			eaip(arp->ar_sip), eamac(arp->ar_sha));
	}

}

#if eaipv6

static void
print_ipv6_pkt(char *s, wlan_if_t vap, struct ether_header *eh)
{
	adf_net_ipv6hdr_t	*iphdr = (adf_net_ipv6hdr_t *)(eh + 1);
	adf_net_nd_msg_t	*nd = (adf_net_nd_msg_t *)(iphdr + 1);

	if (iphdr->ipv6_nexthdr != NEXTHDR_ICMP) {
		return;
	}

	if (nd->nd_icmph.icmp6_type >= NDISC_ROUTER_SOLICITATION &&
	    nd->nd_icmph.icmp6_type <= NDISC_REDIRECT) {
		eth_icmp6_lladdr_t	*ha;
		char			*type,
					*nds[] = { "rsol", "r-ad", "nsol", "n-ad", "rdr" };
		ha = (eth_icmp6_lladdr_t *)nd->nd_opt;
		type = nds[nd->nd_icmph.icmp6_type - NDISC_ROUTER_SOLICITATION];
		dbg6(	"%s (%s):\ts-ip: " eastr6 "\n"
			"\t\td-ip: " eastr6 "\n"
			"s: " eamstr "\td: " eamstr "\n"
			"\tha: " eamstr " cksum = 0x%x len = %u\n", s, type,
			eaip6(iphdr->ipv6_saddr.s6_addr), eaip6(iphdr->ipv6_daddr.s6_addr),
			eamac(eh->ether_shost), eamac(eh->ether_dhost),
			eamac(ha->addr), nd->nd_icmph.icmp6_cksum, iphdr->ipv6_payload_len);
		return;
	} else if (nd->nd_icmph.icmp6_type == ICMPV6_ECHO_REQUEST) {
		dbg6(	"%s (ping req):\ts-ip: " eastr6 "\n"
			"\t\td-ip: " eastr6 "\n"
			"s: " eamstr "\td: " eamstr "\n", s,
			eaip6(iphdr->ipv6_saddr.s6_addr), eaip6(iphdr->ipv6_daddr.s6_addr),
			eamac(eh->ether_shost), eamac(eh->ether_dhost));
	} else if (nd->nd_icmph.icmp6_type == ICMPV6_ECHO_REPLY) {
		dbg6(	"%s (ping rpl):\ts-ip: " eastr6 "\n"
			"\t\td-ip: " eastr6 "\n"
			"s: " eamstr "\td: " eamstr "\n", s,
			eaip6(iphdr->ipv6_saddr.s6_addr), eaip6(iphdr->ipv6_daddr.s6_addr),
			eamac(eh->ether_shost), eamac(eh->ether_dhost));
	} else {
		dbg6("%s unknown icmp packet 0x%x\n", s, nd->nd_icmph.icmp6_type);
		return;
	}


}
#endif /* eaipv6 */
#endif /* EXTAP_DEBUG */

#if eaipv6
static void
ieee80211_extap_in_ipv6(wlan_if_t vap, struct ether_header *eh)
{
	adf_net_ipv6hdr_t	*iphdr = (adf_net_ipv6hdr_t *)(eh + 1);
	u_int8_t		*mac;

	print_ipv6("inp6", vap, eh);

	if (iphdr->ipv6_nexthdr == ADF_NEXTHDR_ICMP) {
		adf_net_nd_msg_t	*nd = (adf_net_nd_msg_t *)(iphdr + 1);
		eth_icmp6_lladdr_t	*ha;
		switch(nd->nd_icmph.icmp6_type) {
		case ADF_ND_NSOL:	/* ARP Request */
			ha = (eth_icmp6_lladdr_t *)nd->nd_opt;
			/* save source ip */
			mi_add(&vap->iv_ic->ic_miroot, iphdr->ipv6_saddr.s6_addr,
				ha->addr, ATH_MITBL_IPV6);
			return;
		case ADF_ND_NADVT:	/* ARP Response */
			ha = (eth_icmp6_lladdr_t *)nd->nd_opt;
			/* save target ip */
			mi_add(&vap->iv_ic->ic_miroot, nd->nd_target.s6_addr,
				ha->addr, ATH_MITBL_IPV6);
			/*
			 * Unlike ipv4, source IP and MAC is not present.
			 * Nothing to restore in the packet
			 */
			break;
		case ADF_ND_RSOL:
		case ADF_ND_RADVT:
		default:
			/* Don't know what to do */
			;
		dbg6("%s(%d): icmp type 0x%x\n", __func__,
			__LINE__, nd->nd_icmph.icmp6_type);
		}
	} else {
		dbg6(	"inp6 (0x%x):\ts-ip: " eastr6 "\n"
			"\t\td-ip: " eastr6 "\n"
			"s: " eamstr "\td: " eamstr "\n", iphdr->ipv6_nexthdr,
			eaip6(iphdr->ipv6_saddr.s6_addr), eaip6(iphdr->ipv6_daddr.s6_addr),
			eamac(eh->ether_shost), eamac(eh->ether_dhost));
	}
	mac = mi_lkup(vap->iv_ic->ic_miroot, iphdr->ipv6_daddr.s6_addr,
			ATH_MITBL_IPV6);
	if (mac) {
		eadbg2(eh->ether_dhost, mac);
		IEEE80211_ADDR_COPY(eh->ether_dhost, mac);
	}
	print_ipv6("INP6", vap, eh);
	return;
}
#endif

static inline bool is_multicast_group(u_int32_t addr)
{
	return ((addr & 0xf0000000) == 0xe0000000);
}

char *arps[] = { NULL, "req", "rsp", "rreq", "rrsp" };
int
ieee80211_extap_input(wlan_if_t vap, struct ether_header *eh)
{
	u_int8_t	*mac;
	u_int8_t	*sip, *dip;
	adf_net_iphdr_t	*iphdr = NULL;
	eth_arphdr_t	*arp = NULL;
      
	switch (eh->ether_type) {
	case ETHERTYPE_IP:
		iphdr = (adf_net_iphdr_t *)(eh + 1);
		sip = (u_int8_t *)&iphdr->ip_saddr;
		dip = (u_int8_t *)&iphdr->ip_daddr;
		break;

	case ETHERTYPE_ARP:
		arp = (eth_arphdr_t *)(eh + 1);
		dbg4("inp %s\t" eaistr "\t" eamstr "\t" eaistr "\t" eamstr "\n",
			arps[arp->ar_op],
			eaip(arp->ar_sip), eamac(arp->ar_sha),
			eaip(arp->ar_tip), eamac(arp->ar_tha));
		if (arp->ar_op == ADF_ARP_REQ ||
	    	    arp->ar_op == ADF_ARP_RREQ) {
			return 0;
		}
		mac = mi_lkup(vap->iv_ic->ic_miroot, arp->ar_sip,
				ATH_MITBL_IPV4);
		if (mac) IEEE80211_ADDR_COPY(arp->ar_sha, mac);

		print_arp(eh);
		sip = arp->ar_sip;
		dip = arp->ar_tip;
        	break;

	case ETHERTYPE_PAE:
		eadbg1("%s(%d): Not fwd-ing EAPOL packet from " eamstr "\n",
			__func__, __LINE__, eh->ether_shost);
		IEEE80211_ADDR_COPY(eh->ether_dhost, vap->iv_myaddr);
		return 0;
#if eaipv6
	case ETHERTYPE_IPV6:
		ieee80211_extap_in_ipv6(vap, eh);
		return 0;
#endif
	default:
		eadbg1("%s(%d): Uknown packet type - 0x%x\n",
			__func__, __LINE__, eh->ether_type);
		return 0;
	}

	mac = mi_lkup(vap->iv_ic->ic_miroot, dip, ATH_MITBL_IPV4);
	/*
	 * XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
	 * XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
	 *
	 * dont do anything if destination IP is self
	 *
	 * XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
	 * XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
	 */
	if (mac) {
		eadbg2(eh->ether_dhost, mac);
		IEEE80211_ADDR_COPY(eh->ether_dhost, mac);
	} else {
		/* XXX XXX what should we do XXX XXX */
		if ( iphdr != NULL ) {
			u_int32_t   groupaddr = 0;
			u_int8_t    groupaddr_l2[IEEE80211_ADDR_LEN];
			groupaddr = ntohl(iphdr->ip_daddr);
			if(is_multicast_group(groupaddr)) {
				groupaddr_l2[0] = 0x01;
				groupaddr_l2[1] = 0x00;
				groupaddr_l2[2] = 0x5e;
				groupaddr_l2[3] = (groupaddr >> 16) & 0x7f;
				groupaddr_l2[4] = (groupaddr >>  8) & 0xff;
				groupaddr_l2[5] = (groupaddr >>  0) & 0xff;

				IEEE80211_ADDR_COPY(eh->ether_dhost, groupaddr_l2);
			}
		}
	}
	if (arp) {
		dbg4("INP %s\t" eaistr "\t" eamstr "\t" eaistr "\t" eamstr "\n",
			arps[arp->ar_op],
			eaip(arp->ar_sip), eamac(arp->ar_sha),
			eaip(arp->ar_tip), eamac(arp->ar_tha));
	}
	return 0;
}

static int
ieee80211_extap_out_arp(wlan_if_t vap, struct ether_header *eh)
{
	eth_arphdr_t *arp = (eth_arphdr_t *)(eh + 1);

	print_arp(eh);

	/* For ARP requests, note down the sender's details */
	mi_add(&vap->iv_ic->ic_miroot, arp->ar_sip, arp->ar_sha,
		ATH_MITBL_IPV4);

#if 1
	dbg4("\t\t\tout %s\t" eaistr "\t" eamstr "\t" eaistr "\t" eamstr "\n",
		arps[arp->ar_op],
		eaip(arp->ar_sip), eamac(arp->ar_sha),
		eaip(arp->ar_tip), eamac(arp->ar_tha));
	dbg4("\t\t\ts: " eamstr "\td: " eamstr "\n",
		eamac(eh->ether_shost), eamac(eh->ether_dhost));
#endif

	/* Modify eth frame as if we sent */
	eadbg2(eh->ether_shost, vap->iv_myaddr);
	IEEE80211_ADDR_COPY(eh->ether_shost, vap->iv_myaddr);

	/* Modify ARP content as if we initiated the req */
	eadbg2(arp->ar_sha, vap->iv_myaddr);
	IEEE80211_ADDR_COPY(arp->ar_sha, vap->iv_myaddr);
	/* For ARP replys, note down the target's details */
	if (arp->ar_op == ADF_ARP_RSP || arp->ar_op == ADF_ARP_RRSP) {
		mi_add(&vap->iv_ic->ic_miroot, arp->ar_tip, arp->ar_tha,
			ATH_MITBL_IPV4);
	}

#if 1
	dbg4("\t\t\tOUT %s\t" eaistr "\t" eamstr "\t" eaistr "\t" eamstr "\n",
		arps[arp->ar_op],
		eaip(arp->ar_sip), eamac(arp->ar_sha),
		eaip(arp->ar_tip), eamac(arp->ar_tha));
	dbg4("\t\t\tS: " eamstr "\tD: " eamstr "\n",
		eamac(eh->ether_shost), eamac(eh->ether_dhost));
#endif

	return 0;
}

static void
ieee80211_extap_out_ipv4(wlan_if_t vap, struct ether_header *eh)
{
	adf_net_iphdr_t	*iphdr = (adf_net_iphdr_t *)(eh + 1);
	u_int8_t	*mac, *ip;

	ip = (u_int8_t *)&iphdr->ip_daddr;
	mac = mi_lkup(vap->iv_ic->ic_miroot, ip, ATH_MITBL_IPV4);

	/*
	 * XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
	 * XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
	 *
	 * dont do anything if destination IP is self ???
	 *
	 * XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
	 * XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
	 */

	if (mac) {
		eadbg2i(eh->ether_dhost, mac, ip);
		IEEE80211_ADDR_COPY(eh->ether_dhost, mac);
	} else {
		/* XXX XXX what should we do XXX XXX */
	}

	eadbg2(eh->ether_shost, vap->iv_myaddr);
	IEEE80211_ADDR_COPY(eh->ether_shost, vap->iv_myaddr);
}

#if eaipv6
static int
ieee80211_extap_out_ipv6(wlan_if_t vap, struct ether_header *eh)
{
	adf_net_ipv6hdr_t	*iphdr = (adf_net_ipv6hdr_t *)(eh + 1);
	u_int8_t		*mac, *ip;

	//dbg6("out ipv6(0x%x): " eastr6 " " eamstr,
	//	iphdr->ipv6_nexthdr,
	//	eaip6(iphdr->ipv6_saddr.s6_addr),
	//	eamac(eh->ether_shost));
	print_ipv6("out6", vap, eh);

	if (iphdr->ipv6_nexthdr == ADF_NEXTHDR_ICMP) {
		adf_net_nd_msg_t	*nd = (adf_net_nd_msg_t *)(iphdr + 1);
		eth_icmp6_lladdr_t	*ha;

		/* Modify eth frame as if we sent */
		eadbg2(eh->ether_shost, vap->iv_myaddr);
		IEEE80211_ADDR_COPY(eh->ether_shost, vap->iv_myaddr);

		switch(nd->nd_icmph.icmp6_type) {
		case ADF_ND_NSOL:	/* ARP Request */
			ha = (eth_icmp6_lladdr_t *)nd->nd_opt;
			/* save source ip */
			mi_add(&vap->iv_ic->ic_miroot, iphdr->ipv6_saddr.s6_addr,
				ha->addr, ATH_MITBL_IPV6);
#define recheck	1
#if recheck
			eadbg2(ha->addr, vap->iv_myaddr);
			IEEE80211_ADDR_COPY(ha->addr, vap->iv_myaddr);

			nd->nd_icmph.icmp6_cksum = 0;
			nd->nd_icmph.icmp6_cksum =
				adf_csum_ipv6(&iphdr->ipv6_saddr, &iphdr->ipv6_daddr,
					iphdr->ipv6_payload_len, IPPROTO_ICMPV6,
					csum_partial((__u8 *) nd,
						iphdr->ipv6_payload_len, 0));
#endif
			break;
		case ADF_ND_NADVT:	/* ARP Response */
			ha = (eth_icmp6_lladdr_t *)nd->nd_opt;
			/* save target ip */
			mi_add(&vap->iv_ic->ic_miroot, nd->nd_target.s6_addr,
				ha->addr, ATH_MITBL_IPV6);
#if recheck
			eadbg2(ha->addr, vap->iv_myaddr);
			IEEE80211_ADDR_COPY(ha->addr, vap->iv_myaddr);

			nd->nd_icmph.icmp6_cksum = 0;
			nd->nd_icmph.icmp6_cksum =
				adf_csum_ipv6(&iphdr->ipv6_saddr, &iphdr->ipv6_daddr,
					iphdr->ipv6_payload_len, IPPROTO_ICMPV6,
					csum_partial((__u8 *) nd,
						iphdr->ipv6_payload_len, 0));
#endif
			break;
		case ADF_ND_RSOL:
		case ADF_ND_RADVT:
		default:
			/* Don't know what to do */
			dbg6("%s(%d): icmp type 0x%x\n", __func__,
				__LINE__, nd->nd_icmph.icmp6_type);
		}
		print_ipv6("OUT6", vap, eh);
		return 0;
	} else {
		dbg6(	"out6 (0x%x):\ts-ip:" eastr6 "\n"
			"\t\td-ip: " eastr6 "\n"
			"s: " eamstr "\td: " eamstr "\n", iphdr->ipv6_nexthdr,
			eaip6(iphdr->ipv6_saddr.s6_addr), eaip6(iphdr->ipv6_daddr.s6_addr),
			eamac(eh->ether_shost), eamac(eh->ether_dhost));
	}

	ip = iphdr->ipv6_daddr.s6_addr;
	mac = mi_lkup(vap->iv_ic->ic_miroot, ip, ATH_MITBL_IPV6);

	/*
	 * XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
	 * XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
	 *
	 * dont do anything if destination IP is self ???
	 *
	 * XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
	 * XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
	 */

	if (mac) {
		eadbg2i(eh->ether_dhost, mac, ip);
		IEEE80211_ADDR_COPY(eh->ether_dhost, mac);
	} else {
		/* XXX XXX what should we do XXX XXX */
	}

	eadbg2(eh->ether_shost, vap->iv_myaddr);
	IEEE80211_ADDR_COPY(eh->ether_shost, vap->iv_myaddr);

	return 0;
}

#endif

int
ieee80211_extap_output(wlan_if_t vap, struct ether_header *eh)
{
	switch(eh->ether_type) {
	case ETHERTYPE_ARP:
		return ieee80211_extap_out_arp(vap, eh);

	case ETHERTYPE_IP:
		ieee80211_extap_out_ipv4(vap, eh);
		break;

	case ETHERTYPE_PAE:
		eadbg1("%s(%d): Not fwd-ing EAPOL packet from " eamstr "\n",
			__func__, __LINE__, eh->ether_shost);
		IEEE80211_ADDR_COPY(eh->ether_shost, vap->iv_myaddr);
		break;

#if eaipv6
	case ETHERTYPE_IPV6:
		return ieee80211_extap_out_ipv6(vap, eh);
#endif

	default:
		eadbg1("%s(%d): Uknown packet type - 0x%x\n",
			__func__, __LINE__, eh->ether_type);
	}
	return 0;
}
