/*-
 * Copyright (c) 2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2010 Atheros Communications, Inc.
 * All rights reserved.
 */

/*
 * athdbg athX cmd args 
 */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/if.h>
#include <linux/wireless.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <getopt.h>


/*
 * Linux uses __BIG_ENDIAN and __LITTLE_ENDIAN while BSD uses _foo
 * and an explicit _BYTE_ORDER.  Sorry, BSD got there first--define
 * things in the BSD way...
 */
#ifndef _LITTLE_ENDIAN
#define	_LITTLE_ENDIAN	1234	/* LSB first: i386, vax */
#endif
#ifndef _BIG_ENDIAN
#define	_BIG_ENDIAN	4321	/* MSB first: 68000, ibm, net */
#endif
#include <asm/byteorder.h>
#if defined(__LITTLE_ENDIAN)
#define	_BYTE_ORDER	_LITTLE_ENDIAN
#elif defined(__BIG_ENDIAN)
#define	_BYTE_ORDER	_BIG_ENDIAN
#else
#error "Please fix asm/byteorder.h"
#endif

/*
** Need to find proper references in UMAC code
*/

#include "os/linux/include/ieee80211_external.h"

#define	streq(a,b)	(strncasecmp(a,b,sizeof(b)-1) == 0)


static void
usage(void)
{
	fprintf(stderr, "usage: wifitool athX cmd args\n");
	fprintf(stderr, "cmd: [sendaddba senddelba setaddbaresp getaddbastats  sendaddts senddelts  \n"); 
    fprintf(stderr, "cmd: [sendtsmrpt sendneigrpt sendlmreq sendbstmreq  sendbcnrpt ] \n");
    fprintf(stderr, "cmd: [sendstastats sendchload sendnhist sendlcireq rrmstats bcnrpt] \n");
	exit(-1);
}

static void
usage_getrrrmstats(void)
{
   fprintf(stderr, "usage: wifitool athX get_rrmstats  [dstmac]\n");
   fprintf(stderr, "[dstmac] - stats reported by the given station\n");
}
static void 
usage_getrssi(void)
{
 	fprintf(stderr, "usage: wifitool athX get_rssi  [dstmac]\n");
	fprintf(stderr, "[dstmac] - stats reported by the given station\n");
}
static void 
usage_acsreport(void)
{
 	fprintf(stderr, "usage: wifitool athX acsreport\n");
}
static void usage_sendfrmreq(void)
{
   fprintf(stderr, "usage: wifitool athX sendfrmreq  <dstmac> <n_rpts> <reg_class> <chnum> \n");
   fprintf(stderr, "<rand_invl> <mandatory_duration> <req_type> <ref mac> \n");
   exit(-1);
}

static void 
usage_sendlcireq(void)
{
   fprintf(stderr, "usage: wifitool athX sendlcireq  <dstmac> <location> <latitude_res> <longitude_res> \n");
   fprintf(stderr, "<altitude_res> [azimuth_res] [azimuth_type]\n");
   fprintf(stderr, "<dstmac> - MAC address of the receiving station \n");
   fprintf(stderr, "<location> - location of requesting/reporting station \n");
   fprintf(stderr, "<latitude_res> - Number of most significant bits(max 34) for fixed-point value of latitude \n");
   fprintf(stderr, "<longitude_res> - Number of most significant bits(max 34) for fixed-point value of longitude\n");
   fprintf(stderr, "<altitude_res> - Number of most significant bits(max 30) for fixed-point value of altitude\n");
   fprintf(stderr, "<azimuth_res> -  Number of most significant bits(max 9) for fixed-point value of Azimuth\n");
   fprintf(stderr, "<azimuth_type> - specifies report of azimuth of radio reception(0) or front surface(1) of reporting station\n");
   exit(-1);
}

static void
usage_sendchloadrpt(void)
{
   fprintf(stderr, "usage: wifitool athX sendchload  <dstmac> <n_rpts> <reg_class> <chnum> \n");
   fprintf(stderr, "<rand_invl> <mandatory_duration> <optional_condtion> <condition_val>\n");
   exit(-1);
}

static void
usage_sendnhist(void)
{
   fprintf(stderr, "usage: wifitool athX sendnhist  <dstmac> <n_rpts> <reg_class> <chnum> \n");
   fprintf(stderr, "<rand_invl> <mandator_duration> <optional_condtion> <condition_val>\n");
   exit(-1);
}

static void
usage_sendstastatsrpt(void)
{
   fprintf(stderr, "usage: wifitool athX sendstastats  <dstmac> <duration> <gid>\n");
   exit(-1);
}

static void
usage_sendaddba(void)
{
   fprintf(stderr, "usage: wifitool athX sendaddba <aid> <tid> <buffersize>\n");
   exit(-1);
}


static void
usage_senddelba(void)
{
   fprintf(stderr, "usage: wifitool athX senddelba <aid> <tid> <initiator> <reasoncode> \n");
   exit(-1);
}

static void
usage_setaddbaresp(void)
{
   fprintf(stderr, "usage: wifitool athX setaddbaresp <aid> <tid> <statuscode> \n");
   exit(-1);
}

static void
usage_sendsingleamsdu(void)
{
   fprintf(stderr, "usage: wifitool athX sendsingleamsdu <aid> <tid> \n");
   exit(-1);
}


static void
usage_getaddbastats(void)
{
   fprintf(stderr, "usage: wifitool athX setaddbaresp <aid> <tid> \n");
   exit(-1);
}

static void
usage_sendbcnrpt(void)
{
   fprintf(stderr, "usage: wifitool athX sendbcnrpt <dstmac> <regclass> <channum> \n");
   fprintf(stderr, "       <rand_ivl> <duration> <mode> \n");
   fprintf(stderr, "       <req_ssid> <rep_cond> <rpt_detail>\n");
   fprintf(stderr, "       <req_ie> <chanrpt_mode> \n");
   fprintf(stderr, "       req_ssid = 1 for ssid, 2 for wildcard ssid \n");
   exit(-1);
}

static void
usage_sendtsmrpt(void)
{
   fprintf(stderr, "usage: wifitool athX sendtsmrpt <num_rpt> <rand_ivl> <meas_dur>\n");
   fprintf(stderr, "       <tid> <macaddr> <bin0-range> <trig_cond> \n");
   fprintf(stderr, "       <avg_err_thresh> <cons_err_thresh> <delay_thresh> <trig_timeout>\n");
   exit(-1);
}

static void
usage_sendneigrpt(void)
{
   fprintf(stderr, "usage: wifitool athX sendneigrpt <mac_addr> <ssid> \n");
   exit(-1);
}

static void
usage_sendlmreq(void)
{
   fprintf(stderr, "usage: wifitool athX sendlmreq <mac_addr> \n");
   exit(-1);
}

static void
usage_sendbstmreq(void)
{
   fprintf(stderr, "usage: wifitool athX sendbstmreq <mac_addr> <candidate_list> <disassoc> <validityItrv>\n");
   exit(-1);
}

static void
usage_senddelts(void)
{
   fprintf(stderr, "usage: wifitool athX senddelts <mac_addr> <tid> \n");
   exit(-1);
}

static void
usage_sendaddts(void)
{
   fprintf(stderr, "usage: wifitool athX sendaddts <mac_addr> <tid> <dir> <up>\
           <nominal_msdu> <mean_data_rate> <mean_phy_rate> <surplus_bw>\
                             <uapsd-bit> <ack_policy> <max_burst_size>\n");
   exit(-1);
}

/*
 * Input an arbitrary length MAC address and convert to binary.
 * Return address size.
 */
int
wifitool_mac_aton(const char *  orig,
            unsigned char *     mac,
            int                 macmax)
{
  const char *  p = orig;
  int           maclen = 0;

  /* Loop on all bytes of the string */
  while(*p != '\0')
    {
      int       temph;
      int       templ;
      int       count;
      /* Extract one byte as two chars */
      count = sscanf(p, "%1X%1X", &temph, &templ);
      if(count != 2)
        break;                  /* Error -> non-hex chars */
      /* Output two chars as one byte */
      templ |= temph << 4;
      mac[maclen++] = (unsigned char) (templ & 0xFF);

      /* Check end of string */
      p += 2;
      if(*p == '\0')
        {
          return(maclen);               /* Normal exit */
        }

      /* Check overflow */
      if(maclen >= macmax)
        {
          fprintf(stderr, "maclen overflow \n");
          return(0);                    /* Error -> overflow */
        }

      /* Check separator */
      if(*p != ':')
        break;
      p++;
    }

  /* Error... */
  fprintf(stderr, "Invlaid macstring %s \n", orig);
  return(0);
}


static void
send_addba(const char *ifname, int argc, char *argv[])
{
    struct iwreq iwr;
    int s, len;
    struct ieee80211req_athdbg req;
    if (argc < 6) {
        usage_sendaddba();
    }
    else {
        memset(&req, 0, sizeof(struct ieee80211req_athdbg));
        s = socket(AF_INET, SOCK_DGRAM, 0);
        (void) memset(&iwr, 0, sizeof(iwr));
        (void) strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));
        req.cmd = IEEE80211_DBGREQ_SENDADDBA;
        req.data.param[0] = atoi(argv[3]); 
        req.data.param[1] = atoi(argv[4]);
        req.data.param[2] = atoi(argv[5]);
        iwr.u.data.pointer = (void *) &req;
        iwr.u.data.length = (sizeof(struct ieee80211req_athdbg));
        if (ioctl(s, IEEE80211_IOCTL_DBGREQ, &iwr) < 0) {
            errx(1, "unable to send addba");
        }
    }
    return;
}

static void
send_delba(const char *ifname, int argc, char *argv[])
{
    struct iwreq iwr;
    int s, len;
    struct ieee80211req_athdbg req;
    if (argc < 7) {
        usage_senddelba();
    }
    else {
        memset(&req, 0, sizeof(struct ieee80211req_athdbg));
        s = socket(AF_INET, SOCK_DGRAM, 0);
        (void) memset(&iwr, 0, sizeof(iwr));
        (void) strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));
        req.cmd = IEEE80211_DBGREQ_SENDDELBA;
        req.data.param[0] = atoi(argv[3]);
        req.data.param[1] = atoi(argv[4]);
        req.data.param[2] = atoi(argv[5]);
        req.data.param[3] = atoi(argv[6]);
        iwr.u.data.pointer = (void *) &req;
        iwr.u.data.length = (sizeof(struct ieee80211req_athdbg));
        if (ioctl(s, IEEE80211_IOCTL_DBGREQ, &iwr) < 0) {
            errx(1, "unable to send delba");
        }
    }
    return;
}

static void
set_addbaresp(const char *ifname, int argc, char *argv[])
{
    struct iwreq iwr;
    int s, len;
    struct ieee80211req_athdbg req;
    if (argc < 6) {
        usage_sendaddba();
    }
    else {
        memset(&req, 0, sizeof(struct ieee80211req_athdbg));
        s = socket(AF_INET, SOCK_DGRAM, 0);
        (void) memset(&iwr, 0, sizeof(iwr));
        (void) strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));
        req.cmd = IEEE80211_DBGREQ_SETADDBARESP;
        req.data.param[0] = atoi(argv[3]);
        req.data.param[1] = atoi(argv[4]);
        req.data.param[2] = atoi(argv[5]);
        iwr.u.data.pointer = (void *) &req;
        iwr.u.data.length = (sizeof(struct ieee80211req_athdbg));
        if (ioctl(s, IEEE80211_IOCTL_DBGREQ, &iwr) < 0) {
            errx(1, "unable to addba response");
        }
    }
    return;
}

static void
send_singleamsdu(const char *ifname, int argc, char *argv[])
{
    struct iwreq iwr;
    int s, len;
    struct ieee80211req_athdbg req;
    if (argc < 5) {
        usage_sendsingleamsdu();
    }
    else {
        memset(&req, 0, sizeof(struct ieee80211req_athdbg));
        s = socket(AF_INET, SOCK_DGRAM, 0);
        (void) memset(&iwr, 0, sizeof(iwr));
        (void) strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));
        req.cmd = IEEE80211_DBGREQ_SENDSINGLEAMSDU;
        req.data.param[0] = atoi(argv[3]);
        req.data.param[1] = atoi(argv[4]);
        iwr.u.data.pointer = (void *) &req;
        iwr.u.data.length = (sizeof(struct ieee80211req_athdbg));
        if (ioctl(s, IEEE80211_IOCTL_DBGREQ, &iwr) < 0) {
            errx(1, "unable to send single AMSDU ");
        }
    }
    return;
}

static void
get_addbastats(const char *ifname, int argc, char *argv[])
{
    struct iwreq iwr;
    int s, len;
    struct ieee80211req_athdbg req;
    if (argc < 5) {
        usage_getaddbastats();
    }
    else {
        memset(&req, 0, sizeof(struct ieee80211req_athdbg));
        s = socket(AF_INET, SOCK_DGRAM, 0);
        (void) memset(&iwr, 0, sizeof(iwr));
        (void) strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));
        req.cmd = IEEE80211_DBGREQ_GETADDBASTATS;
        req.data.param[0] = atoi(argv[3]);
        req.data.param[1] = atoi(argv[4]);
        iwr.u.data.pointer = (void *) &req;
        iwr.u.data.length = (sizeof(struct ieee80211req_athdbg));
        if (ioctl(s, IEEE80211_IOCTL_DBGREQ, &iwr) < 0) {
            errx(1, "unable to get addba stats");
        }
    }
    return;
}

static void
send_bcnrpt(const char *ifname, int argc, char *argv[])
{
    struct iwreq iwr;
    int s, len;
    struct ieee80211req_athdbg req;
    ieee80211_rrm_beaconreq_info_t* bcnrpt = &req.data.bcnrpt;
    int chan_rptmode = 0;
    if (argc < 14) {
        usage_sendbcnrpt();
    }
    else {
        memset(&req, 0, sizeof(struct ieee80211req_athdbg));
        s = socket(AF_INET, SOCK_DGRAM, 0);
        (void) memset(&iwr, 0, sizeof(iwr));
        (void) strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));
        req.cmd = IEEE80211_DBGREQ_SENDBCNRPT;
        if (!wifitool_mac_aton(argv[3], req.dstmac, 6)) {
            errx(1, "Invalid destination mac address");
            return;
        }
        bcnrpt->regclass = atoi(argv[4]);
        bcnrpt->channum = atoi(argv[5]);
        bcnrpt->random_ivl = atoi(argv[6]);
        bcnrpt->duration = atoi(argv[7]);
        bcnrpt->mode = atoi(argv[8]);
        bcnrpt->req_ssid = atoi(argv[9]);
        bcnrpt->rep_cond = atoi(argv[10]);
        bcnrpt->rep_detail = atoi(argv[11]);
        bcnrpt->req_ie = atoi(argv[12]);

        bcnrpt->bssid[0] = 0xff;
        bcnrpt->bssid[1] = 0xff;
        bcnrpt->bssid[2] = 0xff;
        bcnrpt->bssid[3] = 0xff;
        bcnrpt->bssid[4] = 0xff;
        bcnrpt->bssid[5] = 0xff;
        chan_rptmode = atoi(argv[13]);
        if (!chan_rptmode) {
            bcnrpt->num_chanrep = 0;
        }
        else {
            bcnrpt->num_chanrep = 2;
            bcnrpt->apchanrep[0].regclass = 12;
            bcnrpt->apchanrep[0].numchans = 2;
            bcnrpt->apchanrep[0].channum[0] = 1;
            bcnrpt->apchanrep[0].channum[1] = 6;
            bcnrpt->apchanrep[1].regclass = 1;
            bcnrpt->apchanrep[1].numchans = 2;
            bcnrpt->apchanrep[1].channum[0] = 36;
            bcnrpt->apchanrep[1].channum[1] = 48;
        }
        iwr.u.data.pointer = (void *) &req;
        iwr.u.data.length = (sizeof(struct ieee80211req_athdbg));
        if (ioctl(s, IEEE80211_IOCTL_DBGREQ, &iwr) < 0) {
            errx(1, "IEEE80211_IOCTL_DBGREQ failed");
        }
    }
    return;
}

static void
send_tsmrpt(const char *ifname, int argc, char *argv[])
{
    struct iwreq iwr;
    int s, len;
    struct ieee80211req_athdbg req;
    ieee80211_rrm_tsmreq_info_t *tsmrpt = &req.data.tsmrpt;
    if (argc < 14) {
        usage_sendtsmrpt();
    }
    else {
        memset(&req, 0, sizeof(struct ieee80211req_athdbg));
        s = socket(AF_INET, SOCK_DGRAM, 0);
        (void) memset(&iwr, 0, sizeof(iwr));
        (void) strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));
        req.cmd = IEEE80211_DBGREQ_SENDTSMRPT;
        tsmrpt->num_rpt = atoi(argv[3]);
        tsmrpt->rand_ivl = atoi(argv[4]);
        tsmrpt->meas_dur = atoi(argv[5]);
        tsmrpt->tid = atoi(argv[6]);
        if (!wifitool_mac_aton(argv[7], tsmrpt->macaddr, 6)) {
            errx(1, "Invalid mac address");
            return;
        }
        tsmrpt->bin0_range = atoi(argv[8]);
        tsmrpt->trig_cond = atoi(argv[9]);
        tsmrpt->avg_err_thresh = atoi(argv[10]);
        tsmrpt->cons_err_thresh = atoi(argv[11]);
        tsmrpt->delay_thresh = atoi(argv[12]);
        tsmrpt->trig_timeout = atoi(argv[13]);
        memcpy(req.dstmac, tsmrpt->macaddr, 6);
        iwr.u.data.pointer = (void *) &req;
        iwr.u.data.length = (sizeof(struct ieee80211req_athdbg));
        if (ioctl(s, IEEE80211_IOCTL_DBGREQ, &iwr) < 0) {
            errx(1, "IEEE80211_IOCTL_DBGREQ: IEEE80211_DBGREQ_SENDTSMRPT failed");
        }
    }
    return;
}

static void
send_neigrpt(const char *ifname, int argc, char *argv[])
{
    struct iwreq iwr;
    int s, len;
    struct ieee80211req_athdbg req;
    ieee80211_rrm_nrreq_info_t *neigrpt = &req.data.neigrpt;
    if (argc < 4) {
        usage_sendneigrpt();
    }
    else {
        memset(&req, 0, sizeof(struct ieee80211req_athdbg));
        s = socket(AF_INET, SOCK_DGRAM, 0);
        (void) memset(&iwr, 0, sizeof(iwr));
        (void) strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));
        req.cmd = IEEE80211_DBGREQ_SENDNEIGRPT;
        if (!wifitool_mac_aton(argv[3], req.dstmac, 6)) {
            errx(1, "Invalid destination mac address");
            return;
        }
        strcpy((char *)neigrpt->ssid, argv[4]);
        neigrpt->ssid_len = strlen((char *)neigrpt->ssid);
        iwr.u.data.pointer = (void *) &req;
        iwr.u.data.length = (sizeof(struct ieee80211req_athdbg));
        if (ioctl(s, IEEE80211_IOCTL_DBGREQ, &iwr) < 0) {
            errx(1, "IEEE80211_IOCTL_DBGREQ: IEEE80211_DBGREQ_SENDNEIGRPT failed");
        }
    }
    return;
}

static void
send_lmreq(const char *ifname, int argc, char *argv[])
{
    struct iwreq iwr;
    int s, len;
    struct ieee80211req_athdbg req;
    if (argc < 4) {
        usage_sendlmreq();
    }
    else {
        memset(&req, 0, sizeof(struct ieee80211req_athdbg));
        s = socket(AF_INET, SOCK_DGRAM, 0);
        (void) memset(&iwr, 0, sizeof(iwr));
        (void) strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));
        req.cmd = IEEE80211_DBGREQ_SENDLMREQ;
        if (!wifitool_mac_aton(argv[3], req.dstmac, 6)) {
            errx(1, "Invalid destination mac address");
            return;
        }
        iwr.u.data.pointer = (void *) &req;
        iwr.u.data.length = (sizeof(struct ieee80211req_athdbg));
        if (ioctl(s, IEEE80211_IOCTL_DBGREQ, &iwr) < 0) {
            errx(1, "IEEE80211_IOCTL_DBGREQ: IEEE80211_DBGREQ_SENDLMREQ failed");
        }
    }
    return;
}

static void
send_bstmreq(const char *ifname, int argc, char *argv[])
{
    struct iwreq iwr;
    int s, len;
    struct ieee80211req_athdbg req;
    struct ieee80211_bstm_reqinfo* reqinfo = &req.data.bstmreq;
    if (argc < 7) {
        usage_sendbstmreq();
    }
    else {
        memset(&req, 0, sizeof(struct ieee80211req_athdbg));
        s = socket(AF_INET, SOCK_DGRAM, 0);
        (void) memset(&iwr, 0, sizeof(iwr));
        (void) strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));
        req.cmd = IEEE80211_DBGREQ_SENDBSTMREQ;
        if (!wifitool_mac_aton(argv[3], req.dstmac, 6)) {
            errx(1, "Invalid destination mac address");
            return;
        }
        reqinfo->dialogtoken = 1;
        reqinfo->candidate_list = atoi(argv[4]);
        reqinfo->disassoc = atoi(argv[5]);
        reqinfo->validity_itvl = atoi(argv[6]);
        iwr.u.data.pointer = (void *) &req;
        iwr.u.data.length = (sizeof(struct ieee80211req_athdbg));
        if (ioctl(s, IEEE80211_IOCTL_DBGREQ, &iwr) < 0) {
            errx(1, "IEEE80211_IOCTL_DBGREQ: IEEE80211_DBGREQ_SENDBSTMREQ failed");
        }
    }
    return;
}

static void
send_delts(const char *ifname, int argc, char *argv[])
{
    struct iwreq iwr;
    int s, len;
    struct ieee80211req_athdbg req;
    if (argc < 5) {
        usage_senddelts();
    }
    else {
        memset(&req, 0, sizeof(struct ieee80211req_athdbg));
        s = socket(AF_INET, SOCK_DGRAM, 0);
        (void) memset(&iwr, 0, sizeof(iwr));
        (void) strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));
        req.cmd = IEEE80211_DBGREQ_SENDDELTS;
        if (!wifitool_mac_aton(argv[3], req.dstmac, 6)) {
            errx(1, "Invalid destination mac address");
            return;
        }
        req.data.param[0] = atoi(argv[4]);
        iwr.u.data.pointer = (void *) &req;
        iwr.u.data.length = (sizeof(struct ieee80211req_athdbg));
        if (ioctl(s, IEEE80211_IOCTL_DBGREQ, &iwr) < 0) {
            errx(1, "unable to delts");
        }
    }
    return;
}

static void
send_addts(const char *ifname, int argc, char *argv[])
{
    struct iwreq iwr;
    int s, len;
    struct ieee80211req_athdbg req;
    ieee80211_tspec_info* tsinfo = &req.data.tsinfo;
    if (argc < 13) {
        usage_sendaddts();
    }
    else {
        memset(&req, 0, sizeof(struct ieee80211req_athdbg));
        s = socket(AF_INET, SOCK_DGRAM, 0);
        (void) memset(&iwr, 0, sizeof(iwr));
        (void) strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));
        req.cmd = IEEE80211_DBGREQ_SENDADDTSREQ;
        if (!wifitool_mac_aton(argv[3], req.dstmac, 6)) {
            errx(1, "Invalid destination mac address");
            return;
        }
        tsinfo->tid = atoi(argv[4]);
        tsinfo->direction = atoi(argv[5]);
        tsinfo->dot1Dtag = atoi(argv[6]);
        tsinfo->norminal_msdu_size = atoi(argv[7]);
        tsinfo->mean_data_rate = atoi(argv[8]);
        tsinfo->min_phy_rate = atoi(argv[9]);
        tsinfo->surplus_bw = atoi(argv[10]);
        tsinfo->psb = atoi(argv[11]);
        tsinfo->ack_policy = atoi(argv[12]);
        tsinfo->max_burst_size = atoi(argv[13]);
		tsinfo->acc_policy_edca = 1;
        iwr.u.data.pointer = (void *) &req;
        iwr.u.data.length = (sizeof(struct ieee80211req_athdbg));
        if (ioctl(s, IEEE80211_IOCTL_DBGREQ, &iwr) < 0) {
            errx(1, "unable to send ADDTS REQ");
        }
    }
    return;
}

static void
send_noisehistogram(const char *ifname, int argc, char *argv[])
{
    int s, len;
    struct iwreq iwr;
    struct ieee80211req_athdbg req;
    ieee80211_rrm_nhist_info_t *nhist = &req.data.nhist;

    if ((argc < 9) || (argc > 11)) {
        usage_sendnhist();
    }
    else {
        memset(&req, 0, sizeof(struct ieee80211req_athdbg));
        s = socket(AF_INET, SOCK_DGRAM, 0);
        (void) memset(&iwr, 0, sizeof(iwr));
        (void) strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));
        req.cmd = IEEE80211_DBGREQ_SENDNHIST;
        if (!wifitool_mac_aton(argv[3], nhist->dstmac, 6)) {
            errx(1, "Invalid mac address");
            return;
        }
        nhist->num_rpts = atoi(argv[4]);
        nhist->regclass = atoi(argv[5]);
        nhist->chnum = atoi(argv[6]);
        nhist->r_invl = atoi(argv[7]);
        nhist->m_dur  = atoi(argv[8]);
        if(argc > 9 ) { /*optional element */
            nhist->cond  = atoi(argv[9]);
            nhist->c_val  = atoi(argv[10]);
        }
        memcpy(req.dstmac, nhist->dstmac, 6);
        iwr.u.data.pointer = (void *) &req;
        iwr.u.data.length = (sizeof(struct ieee80211req_athdbg));
        if (ioctl(s, IEEE80211_IOCTL_DBGREQ, &iwr) < 0) {
            errx(1, "IEEE80211_IOCTL_DBGREQ: IEEE80211_DBGREQ_SENDNHISTT failed");
        }
    }
    return;
}
void print_rrmstats(FILE *fd, 
                    ieee80211_rrmstats_t *rrmstats,int unicast)
{
   u_int32_t chnum=0, i;
   u_int8_t buf[80];
   ieee80211_rrm_noise_data_t  noise_invalid;
   ieee80211_rrm_noise_data_t  *noise_dptr;
   ieee80211_rrm_lci_data_t    *lci_info;
   ieee80211_rrm_statsgid0_t   *gid0;
   ieee80211_rrm_statsgid10_t  *gid10;
   ieee80211_rrm_statsgid1_t   *gid1;
   ieee80211_rrm_statsgidupx_t *gidupx;
   ieee80211_rrm_tsm_data_t    *tsmdata;
   ieee80211_rrm_lm_data_t     *lmdata;
   ieee80211_rrm_frmcnt_data_t *frmcnt;
  

   memset(&noise_invalid, 0x0, sizeof(ieee80211_rrm_noise_data_t));


   if(!unicast ) {
       fprintf(fd, "Channel# Chan_load \tANPI\t\tIPI[0 - 11]");
       for (chnum = 0; chnum < IEEE80211_CHAN_MAX;chnum++)
       {
           if (rrmstats->noise_data[chnum].anpi != 0 || rrmstats->chann_load[chnum] != 0)
               {
                   fprintf(fd,"\n");
                   fprintf(fd ,"%d\t ",chnum);
                   fprintf(fd ,"%d \t\t",rrmstats->chann_load[chnum]);
                   fprintf(fd, "%d\t\t ",rrmstats->noise_data[chnum].anpi);
                   noise_dptr = &rrmstats->noise_data[chnum];
                   for (i = 0; i < 11; i++)
                   {
                       fprintf(fd, "%d, ", noise_dptr->ipi[i]);
                   }
               }
       } 
       fprintf(fd,"\n");
   }else {
       lci_info = &rrmstats->ni_rrm_stats.ni_vap_lciinfo;
       fprintf(fd, "\n");
       fprintf(fd, "LCI local information :\n");
       fprintf(fd, "--------------------\n");
       fprintf(fd, "\t\t latitude %d.%d longitude %d.%d Altitude %d.%d\n", lci_info->lat_integ, 
               lci_info->lat_frac, lci_info->alt_integ, lci_info->alt_frac, 
               lci_info->alt_integ, lci_info->alt_frac);
       lci_info = &rrmstats->ni_rrm_stats.ni_rrm_lciinfo;
       fprintf(fd, "\n");
       fprintf(fd, "LCI local information :\n");
       fprintf(fd, "--------------------\n");
       fprintf(fd, "\t\t latitude %d.%d longitude %d.%d Altitude %d.%d\n", lci_info->lat_integ, 
               lci_info->lat_frac, lci_info->alt_integ, lci_info->alt_frac, 
               lci_info->alt_integ, lci_info->alt_frac);
       gid0 = &rrmstats->ni_rrm_stats.gid0;
       fprintf(fd, "GID0 stats: \n");
       fprintf(fd, "\t\t txfragcnt %d mcastfrmcnt %d failcnt %d rxfragcnt %d mcastrxfrmcnt %d \n",
               gid0->txfragcnt, gid0->mcastfrmcnt, gid0->failcnt,gid0->rxfragcnt,gid0->mcastrxfrmcnt);
       fprintf(fd, "\t\t fcserrcnt %d  txfrmcnt %d\n",  gid0->fcserrcnt, gid0->txfrmcnt);
       gid1 = &rrmstats->ni_rrm_stats.gid1;
       fprintf(fd, "GID1 stats: \n");
       fprintf(fd, "\t\t rty %d multirty %d frmdup %d rtsuccess %d rtsfail %d ackfail %d\n", gid1->rty, gid1->multirty,gid1->frmdup,
               gid1->rtsuccess, gid1->rtsfail, gid1->ackfail);
       for (i = 0; i < 8; i++)
       {
           gidupx = &rrmstats->ni_rrm_stats.gidupx[i];
           fprintf(fd, "dup stats[%d]: \n", i);
           fprintf(fd, "\t\t qostxfragcnt %d qosfailedcnt %d qosrtycnt %d multirtycnt %d\n"
                   "\t\t qosfrmdupcnt %d qosrtssuccnt %d qosrtsfailcnt %d qosackfailcnt %d\n"
                   "\t\t qosrxfragcnt %d qostxfrmcnt %d qosdiscadrfrmcnt %d qosmpdurxcnt %d qosrtyrxcnt %d \n", 
                   gidupx->qostxfragcnt,gidupx->qosfailedcnt,
                   gidupx->qosrtycnt,gidupx->multirtycnt,gidupx->qosfrmdupcnt,
                   gidupx->qosrtssuccnt,gidupx->qosrtsfailcnt,gidupx->qosackfailcnt,
                   gidupx->qosrxfragcnt,gidupx->qostxfrmcnt,gidupx->qosdiscadrfrmcnt,
                   gidupx->qosmpdurxcnt,gidupx->qosrtyrxcnt);
       }
       gid10 = &rrmstats->ni_rrm_stats.gid10;
       fprintf(fd, "GID10 stats: \n", i); 
       fprintf(fd, "\t\tap_avg_delay %d be_avg_delay %d bk_avg_delay %d\n",
               "vi_avg_delay %d vo_avg_delay %d st_cnt %d ch_util %d\n",
               gid10->ap_avg_delay,gid10->be_avg_delay,gid10->bk_avg_delay,
               gid10->vi_avg_delay,gid10->vo_avg_delay,gid10->st_cnt,gid10->ch_util);
       tsmdata = &rrmstats->ni_rrm_stats.tsm_data;
       fprintf(fd, "TSM data : \n");
       fprintf(fd, "\t\ttid %d brange %d mac:%02x:%02x:%02x:%02x:%02x:%02x tx_cnt %d\n",tsmdata->tid,tsmdata->brange,
               tsmdata->mac[0],tsmdata->mac[1],tsmdata->mac[2],tsmdata->mac[3],tsmdata->mac[4],tsmdata->mac[5],tsmdata->tx_cnt);
       fprintf(fd,"\t\tdiscnt %d multirtycnt %d cfpoll %d qdelay %d txdelay %d bin[0-5]: %d %d %d %d %d %d\n\n",            
               tsmdata->discnt,tsmdata->multirtycnt,tsmdata->cfpoll,
               tsmdata->qdelay,tsmdata->txdelay,tsmdata->bin[0],tsmdata->bin[1],tsmdata->bin[2],
               tsmdata->bin[3],tsmdata->bin[4],tsmdata->bin[5]);
       lmdata = &rrmstats->ni_rrm_stats.lm_data;
       fprintf(fd, "Link Measurement information :\n");
       fprintf(fd, "\t\ttx_pow %d lmargin %d rxant %d txant %d rcpi %d rsni %d\n\n",
               lmdata->tx_pow,lmdata->lmargin,lmdata->rxant,lmdata->txant,
               lmdata->rcpi,lmdata->rsni);
       fprintf(fd, "Frame Report Information : \n\n");
       for (i = 0; i < 12; i++)
       {
           frmcnt = &rrmstats->ni_rrm_stats.frmcnt_data[i];
           fprintf(fd,"Transmitter MAC: %02x:%02x:%02x:%02x:%02x:%02x",frmcnt->ta[0], frmcnt->ta[1],frmcnt->ta[2],frmcnt->ta[3],frmcnt->ta[4],frmcnt->ta[5]);
           fprintf(fd," BSSID: %02x:%02x:%02x:%02x:%02x:%02x",frmcnt->bssid[0], frmcnt->bssid[1], frmcnt->bssid[2],\
                   frmcnt->bssid[3], frmcnt->bssid[4],frmcnt->bssid[5]);
           fprintf(fd," phytype %d arsni %d lrsni %d lrcpi %d antid %d frame count %d\n",
                   frmcnt->phytype,frmcnt->arcpi,frmcnt->lrsni,frmcnt->lrcpi,frmcnt->antid, frmcnt->frmcnt);
       }
   }
   return;
}

static void get_bcnrpt(const char *ifname, int argc, char *argv[]) 
{
    struct iwreq iwr;
    int s;
    struct ieee80211req_athdbg req;
    ieee80211req_rrmstats_t *rrmstats_req;
    ieee80211_bcnrpt_t *bcnrpt; 

    (void) memset(&iwr, 0, sizeof(iwr));
    (void) strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));
    s = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&req, 0, sizeof(struct ieee80211req_athdbg));
    req.cmd = IEEE80211_DBGREQ_GETBCNRPT;
    iwr.u.data.pointer = (void *) &req;
    iwr.u.data.length = (sizeof(struct ieee80211req_athdbg));

    bcnrpt  = (ieee80211_bcnrpt_t *)(malloc(sizeof(ieee80211_bcnrpt_t)));
    rrmstats_req = &req.data.rrmstats_req;
    rrmstats_req->data_addr = (void *) bcnrpt;
    rrmstats_req->data_size = (sizeof(ieee80211_bcnrpt_t));
    rrmstats_req->index = 1;

    printf("\t BSSID \t\t\tCHNUM\tRCPI \n");
    while(rrmstats_req->index) {
        if (ioctl(s, IEEE80211_IOCTL_DBGREQ, &iwr) < 0) {
            errx(1, "IEEE80211_IOCTL_DBGREQ: ieee80211_dbgreq_bcnrpt failed");
        }

        if (bcnrpt->more) {
            rrmstats_req->index++;
            printf(" \t%02x %02x %02x %02x %02x %02x\t %d \t %d \n",
                    bcnrpt->bssid[0],bcnrpt->bssid[1],
                    bcnrpt->bssid[2],bcnrpt->bssid[3],
                    bcnrpt->bssid[4],bcnrpt->bssid[5],
                    bcnrpt->chnum,bcnrpt->rcpi);
        } else {
            rrmstats_req->index = 0;
        }
    }
}
static void get_rssi(const char *ifname, int argc, char *argv[])
{

	struct iwreq iwr;
	int s, len;
	struct ieee80211req_athdbg req;
	if ((argc < 4) || (argc > 4))
	{
    	usage_getrssi();
    	return;
	}
  	memset(&req, 0, sizeof(struct ieee80211req_athdbg));
 	s = socket(AF_INET, SOCK_DGRAM, 0);
	(void) memset(&iwr, 0, sizeof(iwr));
  	(void) strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));



  	req.cmd = IEEE80211_DBGREQ_GETRRSSI;

  	req.dstmac[0] = 0x00;
  	req.dstmac[1] = 0x00;
  	req.dstmac[2] = 0x00;
  	req.dstmac[3] = 0x00;
  	req.dstmac[4] = 0x00;
  	req.dstmac[5] = 0x00;
 	if (!wifitool_mac_aton(argv[3], &req.dstmac[0], 6))
    {
       errx(1, "Invalid mac address");
       return;
    }
  	iwr.u.data.pointer = (void *) &req;
  	iwr.u.data.length = (sizeof(struct ieee80211req_athdbg));


  	if (ioctl(s, IEEE80211_IOCTL_DBGREQ, &iwr) < 0)
  	{
      errx(1, "IEEE80211_IOCTL_DBGREQ: IEEE80211_DBGREQ_RRMSTATSREQ failed");
  	}

}

static void acs_report(const char *ifname, int argc, char *argv[])
{

	struct iwreq iwr;
	int s, len,i; 
	struct ieee80211req_athdbg req;
        struct ieee80211_acs_dbg *acs;
	if ((argc < 3) || (argc > 3))
	{
    	   usage_acsreport();
    	   return;
	}
  	memset(&req, 0, sizeof(struct ieee80211req_athdbg));
 	s = socket(AF_INET, SOCK_DGRAM, 0);
	(void) memset(&iwr, 0, sizeof(iwr));
  	(void) strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));


  	iwr.u.data.pointer = (void *) &req;
  	iwr.u.data.length = (sizeof(struct ieee80211req_athdbg));

  	req.cmd = IEEE80211_DBGREQ_GETACSREPORT;
	acs = (void *)malloc(sizeof(struct ieee80211_acs_dbg));
        req.data.acs_rep.data_addr = acs;
        req.data.acs_rep.data_size = sizeof(struct ieee80211_acs_dbg);
        req.data.acs_rep.index = 0;
        acs->entry_id = 0;
        if (ioctl(s, IEEE80211_IOCTL_DBGREQ, &iwr) < 0)
  	{
            errx(1, "IEEE80211_IOCTL_DBGREQ: IEEE80211_DBGREQ_RRMSTATSREQ failed");
            printf("error in ioctl \n");
    	    return;
  	}
        
        fprintf(stdout," Channel | BSS  | minrssi | maxrssi | NF | Ch load | spect load | sec_chan \n"); 
        fprintf(stdout,"---------------------------------------------------------------------\n");
             /* output the current configuration */
        for (i = 0; i < acs->nchans; i++) {
            acs->entry_id = i;
  	    req.cmd = IEEE80211_DBGREQ_GETACSREPORT;
	    if (ioctl(s, IEEE80211_IOCTL_DBGREQ, &iwr) < 0) {
                perror("DBG req failed");
                return;
            }
            fprintf(stdout," %4d(%3d) %4d     %4d      %4d   %4d    %4d        %4d       %4d   \n",
                                  acs->chan_freq,
                                  acs->ieee_chan,
                                  acs->chan_nbss,
                                  acs->chan_minrssi,
                                  acs->chan_maxrssi,
                                  acs->noisefloor,
                                  acs->chan_load,
                                  acs->channel_loading,
                                  acs->sec_chan);
        }

    return;
}

static void get_rrmstats(const char *ifname, int argc, char *argv[])
{
  struct iwreq iwr;
  int s, len,unicast=0;
  struct ieee80211req_athdbg req;
  ieee80211req_rrmstats_t *rrmstats_req;
  ieee80211_rrmstats_t *rrmstats;

  if ((argc < 3) || (argc > 4))
  {
    usage_getrrrmstats();
    return;
  }

  memset(&req, 0, sizeof(struct ieee80211req_athdbg));
  s = socket(AF_INET, SOCK_DGRAM, 0);
  (void) memset(&iwr, 0, sizeof(iwr));
  (void) strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));

  req.cmd = IEEE80211_DBGREQ_GETRRMSTATS;

  req.dstmac[0] = 0x00;
  req.dstmac[1] = 0x00;
  req.dstmac[2] = 0x00;
  req.dstmac[3] = 0x00;
  req.dstmac[4] = 0x00;
  req.dstmac[5] = 0x00;

  if (argc == 4)
  {
    unicast = 1;
    if (!wifitool_mac_aton(argv[3], &req.dstmac[0], 6)) 
    {
       errx(1, "Invalid mac address");
       return;
    } 
  }

  iwr.u.data.pointer = (void *) &req;
  iwr.u.data.length = (sizeof(struct ieee80211req_athdbg));
  rrmstats = (ieee80211_rrmstats_t *)(malloc(sizeof(ieee80211_rrmstats_t)));
  rrmstats_req = &req.data.rrmstats_req;
  rrmstats_req->data_addr = (void *) rrmstats;
  rrmstats_req->data_size = (sizeof(ieee80211_rrmstats_t));

  if (ioctl(s, IEEE80211_IOCTL_DBGREQ, &iwr) < 0)
  {
      errx(1, "IEEE80211_IOCTL_DBGREQ: IEEE80211_DBGREQ_RRMSTATSREQ failed");
  }

  print_rrmstats(stdout, rrmstats,unicast);
}

static void send_frmreq(const char *ifname, int argc, char *argv[])
{
  struct iwreq iwr;
  int s, len;
  struct ieee80211req_athdbg req;
  ieee80211_rrm_frame_req_info_t *frm_req = &req.data.frm_req;

  if (argc != 11)
  {
    usage_sendfrmreq();
    return;
  }

  memset(&req, 0, sizeof(struct ieee80211req_athdbg));
  s = socket(AF_INET, SOCK_DGRAM, 0);
  (void) memset(&iwr, 0, sizeof(iwr));
  (void) strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));

  req.cmd = IEEE80211_DBGREQ_SENDFRMREQ;

  if (!wifitool_mac_aton(argv[3], frm_req->dstmac, 6)) 
  {
      errx(1, "Invalid mac address");
      return;
  }

  memcpy(req.dstmac, frm_req->dstmac, 6);
  frm_req->num_rpts = atoi(argv[4]);
  frm_req->regclass = atoi(argv[5]);
  frm_req->chnum = atoi(argv[6]);
  frm_req->r_invl = atoi(argv[7]);
  frm_req->m_dur = atoi(argv[8]);
  frm_req->ftype = atoi(argv[9]);

  if (!wifitool_mac_aton(argv[10], frm_req->peermac, 6)) 
  {
      errx(1, "Invalid mac address");
      return;
  }

  iwr.u.data.pointer = (void *) &req;
  iwr.u.data.length = (sizeof(struct ieee80211req_athdbg));

  if (ioctl(s, IEEE80211_IOCTL_DBGREQ, &iwr) < 0)
  {
      errx(1, "IEEE80211_IOCTL_DBGREQ: IEEE80211_DBGREQ_SENDSTASTATSREQ failed");
  }
  return;
}

static void send_lcireq(const char *ifname, int argc, char *argv[])
{
  struct iwreq iwr;
  int s, len;
  struct ieee80211req_athdbg req;
  ieee80211_rrm_lcireq_info_t *lci_req = &req.data.lci_req;

  if ((argc < 9) || (argc > 11) || (argc == 10))
  {
    usage_sendlcireq();
    return;
  }

  memset(&req, 0, sizeof(struct ieee80211req_athdbg));
  s = socket(AF_INET, SOCK_DGRAM, 0);
  (void) memset(&iwr, 0, sizeof(iwr));
  (void) strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));

  req.cmd = IEEE80211_DBGREQ_SENDLCIREQ;

  if (!wifitool_mac_aton(argv[3], lci_req->dstmac, 6)) 
  {
      errx(1, "Invalid mac address");
      return;
  }

  memcpy(req.dstmac, lci_req->dstmac, 6);
  lci_req->num_rpts = atoi(argv[4]);
  lci_req->location = atoi(argv[5]);
  lci_req->lat_res = atoi(argv[6]);
  lci_req->long_res = atoi(argv[7]);
  lci_req->alt_res = atoi(argv[8]);

  if ((lci_req->lat_res > 34) || (lci_req->long_res > 34) ||
      (lci_req->alt_res > 30))
  {
    fprintf(stderr, "Incorrect number of resolution bits !!\n");
    usage_sendlcireq();
    exit(-1);
  } 

  if (argc == 11)
  {
    lci_req->azi_res = atoi(argv[9]);
    lci_req->azi_type =  atoi(argv[10]);

    if (lci_req->azi_type !=1)
    {
       fprintf(stderr, "Incorrect azimuth type !!\n");
       usage_sendlcireq();
       exit(-1);
    }

    if (lci_req->azi_res > 9)
    {
       fprintf(stderr, "Incorrect azimuth resolution value(correct range 0 - 9) !!\n");
       usage_sendlcireq();
       exit(-1);
    } 
  }

  iwr.u.data.pointer = (void *) &req;
  iwr.u.data.length = (sizeof(struct ieee80211req_athdbg));

  if (ioctl(s, IEEE80211_IOCTL_DBGREQ, &iwr) < 0)
  {
      errx(1, "IEEE80211_IOCTL_DBGREQ: IEEE80211_DBGREQ_SENDSTASTATSREQ failed");
  }
  return;
}

static void
send_stastats(const char *ifname, int argc, char *argv[])
{
    struct iwreq iwr;
    int s, len;
    struct ieee80211req_athdbg req;
    ieee80211_rrm_stastats_info_t *stastats = &req.data.stastats;

    if (argc < 6) {
        usage_sendstastatsrpt();
    }
    else{
        s = socket(AF_INET, SOCK_DGRAM, 0);
        (void) memset(&iwr, 0, sizeof(iwr));
        (void) strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));
        req.cmd = IEEE80211_DBGREQ_SENDSTASTATSREQ;
        if (!wifitool_mac_aton(argv[3], stastats->dstmac, 6)) {
            errx(1, "Invalid mac address");
            return;
        }
        stastats->m_dur = atoi(argv[4]);
        stastats->gid = atoi(argv[5]);
        memcpy(req.dstmac,stastats->dstmac, 6);
        iwr.u.data.pointer = (void *) &req;
        iwr.u.data.length = (sizeof(struct ieee80211req_athdbg));
        if (ioctl(s, IEEE80211_IOCTL_DBGREQ, &iwr) < 0){
            errx(1, "IEEE80211_IOCTL_DBGREQ: IEEE80211_DBGREQ_SENDSTASTATSREQ failed");
        }
        return;
    }
}

static void
send_chload(const char *ifname, int argc, char *argv[])
{
    struct iwreq iwr;
    int s, len;
    struct ieee80211req_athdbg req;
    ieee80211_rrm_chloadreq_info_t * chloadrpt = &req.data.chloadrpt;

    if ((argc < 9) || (argc > 11)) {
        usage_sendchloadrpt();
    }
    else {
        memset(&req, 0, sizeof(struct ieee80211req_athdbg));
        s = socket(AF_INET, SOCK_DGRAM, 0);
        (void) memset(&iwr, 0, sizeof(iwr));
        (void) strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));
        req.cmd = IEEE80211_DBGREQ_SENDCHLOADREQ;
        if (!wifitool_mac_aton(argv[3], chloadrpt->dstmac, 6)) {
            errx(1, "Invalid mac address");
            return;
        }
        chloadrpt->num_rpts = atoi(argv[4]);
        chloadrpt->regclass = atoi(argv[5]);
        chloadrpt->chnum = atoi(argv[6]);
        chloadrpt->r_invl = atoi(argv[7]);
        chloadrpt->m_dur  = atoi(argv[8]);
        if(argc > 9 ) { /*optional element */
            chloadrpt->cond  = atoi(argv[9]);
            chloadrpt->c_val  = atoi(argv[10]);
        }
        memcpy(req.dstmac, chloadrpt->dstmac, 6);
        iwr.u.data.pointer = (void *) &req;
        iwr.u.data.length = (sizeof(struct ieee80211req_athdbg));
        if (ioctl(s, IEEE80211_IOCTL_DBGREQ, &iwr) < 0) {
            errx(1, "IEEE80211_IOCTL_DBGREQ: IEEE80211_DBGREQ_SENDCHLOADREQ failed");
        }
    }
    return;
}

int
main(int argc, char *argv[])
{
	const char *ifname, *cmd;

	if (argc < 3)
		usage();

	ifname = argv[1];
	cmd = argv[2];
	if (streq(cmd, "sendaddba")) {
            send_addba(ifname, argc, argv);
	} else if (streq(cmd, "senddelba")) {
            send_delba(ifname, argc, argv);
	} else if (streq(cmd, "setaddbaresp")) {
            set_addbaresp(ifname, argc, argv);
    } else if (streq(cmd, "sendsingleamsdu")) {
            send_singleamsdu(ifname, argc, argv);
	} else if (streq(cmd, "getaddbastats")) {
            get_addbastats(ifname, argc, argv);
	} else if (streq(cmd, "sendbcnrpt")) {
            send_bcnrpt(ifname, argc, argv);
	} else if (streq(cmd, "sendtsmrpt")) {
            send_tsmrpt(ifname, argc, argv);
	} else if (streq(cmd, "sendneigrpt")) {
            send_neigrpt(ifname, argc, argv);
	} else if (streq(cmd, "sendlmreq")) {
            send_lmreq(ifname, argc, argv);
	} else if (streq(cmd, "sendbstmreq")) {
            send_bstmreq(ifname, argc, argv);
	} else if (streq(cmd, "senddelts")) {
            send_delts(ifname, argc, argv);
	} else if (streq(cmd, "sendaddts")) {
        send_addts(ifname, argc, argv);
    } else if (streq(cmd, "sendchload")) {
        send_chload(ifname, argc, argv);
    } else if (streq(cmd, "sendnhist")) {
        send_noisehistogram(ifname,argc,argv);
    } else if (streq(cmd, "sendstastats")) {
        send_stastats(ifname, argc, argv);
    } else if (streq(cmd, "sendlcireq")) {
        send_lcireq(ifname, argc, argv);
    } else if (streq(cmd, "rrmstats")) {
        get_rrmstats(ifname, argc, argv);
    } else if (streq(cmd, "sendfrmreq")) {
        send_frmreq(ifname, argc, argv);
    } else if (streq(cmd, "bcnrpt")) {
        get_bcnrpt(ifname, argc, argv);
    } else if (streq(cmd, "getrssi")) {
		get_rssi(ifname, argc, argv);
    } else if (streq(cmd, "acsreport")) {
		acs_report(ifname, argc, argv);
	} else {
        usage();
    }
	return 0;
}
