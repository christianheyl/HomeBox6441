/*
 * A sample test program to mimic an ANQP server to test hotspot functionality
 * Copyright (c) 2011-2012, Qualcomm Atheros, Inc.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>

#define CLIENT_PATH "/tmp/anqpserver-"

static const char *ctrl_iface_dir = "/var/run/hostapd";
static char *ctrl_ifname = NULL;


int ctrl_sock;
struct sockaddr_un local;
struct sockaddr_un dest;

char info_ids[256];
char venue_name[256];
char net_auth[256];
char roaming_consortium[256];
char ipaddr_type[256];
char nai_realm[256];
char cell_net[256];
char domain_name[256];
char oper_friendly_name[256];
char wan_metrics[256];
char conn_capability[256];
char nai_home_realm[256];
char operating_class[256];
uint8_t dyn_venue_name[256];
uint8_t dyn_net_auth[256];
uint8_t dyn_roaming_consortium[256];
uint8_t dyn_ipaddr_type[256];
uint8_t dyn_nai_realm[256];
uint8_t dyn_cell_net[256];
uint8_t dyn_domain_name[256];
uint8_t dyn_oper_friendly_name[256];
uint8_t dyn_wan_metrics[256];
uint8_t dyn_conn_capability[256];
uint8_t dyn_nai_home_realm[256];
uint8_t dyn_operating_class[256];
uint8_t dyn_venue_name_len;
uint8_t dyn_net_auth_len;
uint8_t dyn_roaming_consortium_len;
uint8_t dyn_ipaddr_type_len;
uint8_t dyn_nai_realm_len;
uint8_t dyn_cell_net_len;
uint8_t dyn_domain_name_len;
uint8_t dyn_oper_friendly_name_len;
uint8_t dyn_wan_metrics_len;
uint8_t dyn_conn_capability_len;
uint8_t dyn_nai_home_realm_len;
uint8_t dyn_operating_class_len;


size_t my_strncpy(char *dst, const char *src, size_t siz)
{
	const char *s = src;
	size_t left = siz;

	if (left) {
		/* Copy string up to the maximum size of the dst buffer */
		while (--left != 0) {
			if ((*dst++ = *s++) == '\0')
				break;
		}
	}

	if (left == 0) {
		/* Not enough room for the string; force NUL-termination */
		if (siz != 0)
			*dst = '\0';
		while (*s++)
			; /* determine total src string length */
	}

	return s - src - 1;
}


static int hex2num(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}


static int hex2byte(const char *hex)
{
	int a, b;
	a = hex2num(*hex++);
	if (a < 0)
		return -1;
	b = hex2num(*hex++);
	if (b < 0)
		return -1;
	return (a << 4) | b;
}


static int hexstr2bin(const char *hex, uint8_t *buf, size_t len)
{
	size_t i;
	int a;
	const char *ipos = hex;
	uint8_t *opos = buf;

	for (i = 0; i < len; i++) {
		a = hex2byte(ipos);
		if (a < 0)
			return -1;
		*opos++ = a;
		ipos += 2;
	}
	return 0;
}


static int ctrl_open(const char *ifname)
{
	int ret, counter = 0;
	char *ctrl_path;
	int flen;

	if (ifname == NULL) {
		printf("%s: err 1\n", __func__);
		return 1;
	}

	flen = strlen(ctrl_iface_dir) + strlen(ifname) + 2;
	ctrl_path = malloc(flen);
	if (ctrl_path == NULL) {
		printf("%s: err 2\n", __func__);
		return 2;
	}
	snprintf(ctrl_path, flen, "%s/%s", ctrl_iface_dir, ifname);

	ctrl_sock = socket(PF_UNIX, SOCK_DGRAM, 0);
	if (ctrl_sock < 0) {
		printf("%s: err 3\n", __func__);
		free(ctrl_path);
		return 3;
	}
	local.sun_family = AF_UNIX;
	ret = snprintf(local.sun_path, sizeof(local.sun_path),
			  CLIENT_PATH "%d-%d", (int) getpid(), counter);
	if (ret < 0 || (size_t) ret >= sizeof(local.sun_path)) {
		close(ctrl_sock);
		printf("%s: err 4\n", __func__);
		free(ctrl_path);
		return 4;
	}
	if (bind(ctrl_sock, (struct sockaddr *)&local, sizeof(local)) < 0) {
		close(ctrl_sock);
		printf("%s: err 5\n", __func__);
		free(ctrl_path);
		return 5;
	}
	dest.sun_family = AF_UNIX;
	my_strncpy(dest.sun_path, ctrl_path, sizeof(dest.sun_path));
	if ((unsigned int) ret >= sizeof(dest.sun_path)) {
		close(ctrl_sock);
		printf("%s: err 6\n", __func__);
		free(ctrl_path);
		return 6;
	}
	if (connect(ctrl_sock, (struct sockaddr *) &dest, sizeof(dest)) < 0) {
		close(ctrl_sock);
		unlink(local.sun_path);
		printf("%s: err 7\n", __func__);
		free(ctrl_path);
		return 7;
	}

	free(ctrl_path);

	return 0;
}


static void ctrl_close(void)
{
	unlink(local.sun_path);
	if (ctrl_sock)
		close(ctrl_sock);
}


static int ctrl_send(const char *cmd, size_t cmd_len)
{
	if (send(ctrl_sock, cmd, cmd_len, 0) < 0) {
		printf("%s: err\n", __func__);
		return 1;
	}
	return 0;
}


#define ANQP_SET_VENUE_NAME		"00000102 "
#define ANQP_SET_NET_AUTH		"00000104 "
#define ANQP_SET_ROAMING_CONSORTIUM	"00000105 "
#define ANQP_SET_IPADDR_TYPE		"00000106 "
#define ANQP_SET_NAI_REALM		"00000107 "
#define ANQP_SET_CELL_NET		"00000108 "
#define ANQP_SET_DOMAIN_NAME		"0000010c "
#define ANQP_SET_OPER_FRIENDLY_NAME	"00010003 "
#define ANQP_SET_WAN_METRICS		"00010004 "
#define ANQP_SET_CONN_CAPABILITY	"00010005 "
#define ANQP_SET_NAI_HOME_REALM		"00010006 "
#define ANQP_SET_OPERATING_CLASS	"00010007 "

static int ctrl_attach(void)
{
	char cmd[256];
	memcpy(cmd, "ANQP_ATTACH ", 12);
	memcpy(cmd + 12, info_ids, strlen(info_ids));
	ctrl_send(cmd, strlen(info_ids) + 12);

	if (strlen(venue_name)) {
		memcpy(cmd, "ANQP_SET ", 9);
		memcpy(cmd + 9, ANQP_SET_VENUE_NAME, 9);
		memcpy(cmd + 18, venue_name, strlen(venue_name));
		ctrl_send(cmd, strlen(venue_name) + 18);
	}
	if (strlen(net_auth)) {
		memcpy(cmd, "ANQP_SET ", 9);
		memcpy(cmd + 9, ANQP_SET_NET_AUTH, 9);
		memcpy(cmd + 18, net_auth, strlen(net_auth));
		ctrl_send(cmd, strlen(net_auth) + 18);
	}
	if (strlen(roaming_consortium)) {
		memcpy(cmd, "ANQP_SET ", 9);
		memcpy(cmd + 9, ANQP_SET_ROAMING_CONSORTIUM, 9);
		memcpy(cmd + 18, roaming_consortium,
		       strlen(roaming_consortium));
		ctrl_send(cmd, strlen(roaming_consortium) + 18);
	}
	if (strlen(ipaddr_type)) {
		memcpy(cmd, "ANQP_SET ", 9);
		memcpy(cmd + 9, ANQP_SET_IPADDR_TYPE, 9);
		memcpy(cmd + 18, ipaddr_type, strlen(ipaddr_type));
		ctrl_send(cmd, strlen(ipaddr_type) + 18);
	}
	if (strlen(nai_realm)) {
		memcpy(cmd, "ANQP_SET ", 9);
		memcpy(cmd + 9, ANQP_SET_NAI_REALM, 9);
		memcpy(cmd + 18, nai_realm, strlen(nai_realm));
		ctrl_send(cmd, strlen(nai_realm) + 18);
	}
	if (strlen(cell_net)) {
		memcpy(cmd, "ANQP_SET ", 9);
		memcpy(cmd + 9, ANQP_SET_CELL_NET, 9);
		memcpy(cmd + 18, cell_net, strlen(cell_net));
		ctrl_send(cmd, strlen(cell_net) + 18);
	}
	if (strlen(domain_name)) {
		memcpy(cmd, "ANQP_SET ", 9);
		memcpy(cmd + 9, ANQP_SET_DOMAIN_NAME, 9);
		memcpy(cmd + 18, domain_name, strlen(domain_name));
		ctrl_send(cmd, strlen(domain_name) + 18);
	}
	if (strlen(oper_friendly_name)) {
		memcpy(cmd, "ANQP_SET ", 9);
		memcpy(cmd + 9, ANQP_SET_OPER_FRIENDLY_NAME, 9);
		memcpy(cmd + 18, oper_friendly_name,
		       strlen(oper_friendly_name));
		ctrl_send(cmd, strlen(oper_friendly_name) + 18);
	}
	if (strlen(wan_metrics)) {
		memcpy(cmd, "ANQP_SET ", 9);
		memcpy(cmd + 9, ANQP_SET_WAN_METRICS, 9);
		memcpy(cmd + 18, wan_metrics, strlen(wan_metrics));
		ctrl_send(cmd, strlen(wan_metrics) + 18);
	}
	if (strlen(conn_capability)) {
		memcpy(cmd, "ANQP_SET ", 9);
		memcpy(cmd + 9, ANQP_SET_CONN_CAPABILITY, 9);
		memcpy(cmd + 18, conn_capability, strlen(conn_capability));
		ctrl_send(cmd, strlen(conn_capability) + 9);
	}
	if (strlen(nai_home_realm)) {
		memcpy(cmd, "ANQP_SET ", 9);
		memcpy(cmd + 9, ANQP_SET_NAI_HOME_REALM, 9);
		memcpy(cmd + 18, nai_home_realm, strlen(nai_home_realm));
		ctrl_send(cmd, strlen(nai_home_realm) + 9);
	}
	if (strlen(operating_class)) {
		memcpy(cmd, "ANQP_SET ", 9);
		memcpy(cmd + 9, ANQP_SET_OPERATING_CLASS, 9);
		memcpy(cmd + 18, operating_class, strlen(operating_class));
		ctrl_send(cmd, strlen(operating_class) + 18);
	}
	return 0;
}


static int ctrl_recv(char *recv_buf, size_t *recv_len)
{
	struct iovec iov[2];
	struct msghdr msg;
	int rc;

	memset(&msg, 0, sizeof(msg));
	memset(&iov, 0, sizeof(iov));

	iov[0].iov_base = recv_buf;
	iov[0].iov_len = *recv_len;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	rc = recvmsg(ctrl_sock, &msg, 0);
	if (rc < 0) {
		printf("recvmsg: failed\n");
		return -1;
	}
	*recv_len = rc;
	printf("rc = %d received bytes\n", rc);
	return 0;
}


int num_nai_home_realms;
struct _nai_home_realms {
	uint8_t *val;
	uint16_t val_len;
	uint8_t encoding;
	char *realm;
	uint16_t realm_len;
	/* for now, only support eap_count=1 */
	uint8_t eap_method;
	uint8_t num_auths;
	uint8_t auth_id[4];
	uint8_t auth_val[4];
};
#define MAX_NAI_HOME_REALMS 10
struct _nai_home_realms nai_home_realms[MAX_NAI_HOME_REALMS];
#define PUT_LE16(a, val)			\
	do {				    \
		(a)[1] = ((uint16_t) (val)) >> 8;    \
		(a)[0] = ((uint16_t) (val)) & 0xff;  \
	} while (0)

/*
 * the str will be encoded as <encoding=xx><realm=realm1;>[<eap_method=zz><auth_id=pp><auth_val=qq>[<auth_id=><auth_val=>]]
 * [<encoding=xx><realm=realm1><eap_count=yy>[<eap_method=zz><auth_id=pp><auth_val=qq>[<auth_id=><auth_val=>]]]
 */
void parse_nai_home_realm(char *pos)
{
	char *start = pos, *end, *tok_end;
	uint8_t encoding, eap_method, auth_id, auth_val;
	int i, num_loops = 0;
	struct _nai_home_realms *realm = NULL;

	num_nai_home_realms = -1;
	while ((end = strchr(start, '='))) {
		if (num_loops++ > 1000) {
			printf("ERROR: Couldn't finish parsing even after 1000 loop iterations. Bailing out... \n"
				"remaining str: %s\n", start);
			return;
		}

		if (strncmp(start, "encoding", 8) == 0) {
			if (hexstr2bin(end+1, &encoding, 1))
				printf("%s: expecting hexstr in encoding=%s\n",
				       __func__, end+1);
			start += 8 + 1 + 2;
			if (num_nai_home_realms == MAX_NAI_HOME_REALMS) {
				printf("Too many NAI home realms\n");
				return;
			}
			num_nai_home_realms++;
			realm = &nai_home_realms[num_nai_home_realms];
			/*printf("encoding = 0x%x (%s)\n", encoding, end+1);*/
			continue;
		}
		if (realm && strncmp(start, "realm", 5) == 0) {
			tok_end = strchr(end + 1, ';');
			realm->realm = malloc(tok_end - end - 1);
			if (realm->realm == NULL)
				return;
			realm->encoding = encoding;
			realm->realm_len = tok_end - end - 1;
			realm->num_auths = 0;
			strncpy(realm->realm, end + 1, tok_end - end - 1);
			start = tok_end + 1;
			/* printf("realm %d realm_len = 0x%x\n",
			   num_nai_home_realms, realm->realm_len); */
			continue;
		}
		if (realm && strncmp(start, "eap_method", 10) == 0) {
			if (hexstr2bin(end+1, &eap_method, 1))
				printf("%s: expecting hexstr in eap_method=%s\n",
				       __func__, end + 1);
			start += 10 + 1 + 2;
			realm->eap_method = eap_method;
			/* printf("eap_method = 0x%x (%s)\n", eap_method,
			   end+1); */
			continue;
		}
		if (realm && strncmp(start, "auth_id", 7) == 0) {
			if (hexstr2bin(end + 1, &auth_id, 1))
				printf("%s: expecting hexstr in auth_id=%s\n",
				       __func__, end + 1);
			start += 7 + 1 + 2;
			realm->auth_id[realm->num_auths] = auth_id;
			/*printf("auth_id = 0x%x (%s)\n", auth_id, end + 1);*/
			continue;
		}
		if (realm && strncmp(start, "auth_val", 8) == 0) {
			if (hexstr2bin(end+1, &auth_val, 1))
				printf("%s: expecting hexstr in auth_id=%s\n",
				       __func__, end+1);
			start += 8 + 1 + 2;
			realm->auth_val[realm->num_auths++] = auth_val;
			/*printf("auth_val = 0x%x (%s)\n", auth_val, end+1);*/
			continue;
		}
	}

	if (num_nai_home_realms == -1)
		return;

	for (i = 0; i <= num_nai_home_realms; i++) {
		uint8_t *val;

		realm = &nai_home_realms[i];
		uint16_t size = realm->realm_len + 5;
		if (realm->num_auths)
			size += (realm->num_auths * 3) + 3;

		realm->val = malloc(size);
		if (realm->val == NULL)
			return;
		val = realm->val;
		PUT_LE16(val, size - 2);
		val += 2;
		*val++ = realm->encoding;
		*val++ = realm->realm_len;
		memcpy(val, realm->realm, realm->realm_len);
		val += realm->realm_len;
		if (realm->num_auths) {
			*val++ = 1; /* eap method count */
			*val++ = 2 + (3 * realm->num_auths);
			*val++ = realm->eap_method;
			*val++ = realm->num_auths;
			*val++ = realm->auth_id[0];
			*val++ = 1;
			*val++ = realm->auth_val[0];
			if (realm->num_auths > 1) {
				*val++ = realm->auth_id[1];
				*val++ = 1;
				*val++ = realm->auth_val[1];
			}
		} else
			*val++ = 0;
		realm->val_len = size;
	}

#if 0 /* debug */
	for (i=0; i<=num_nai_home_realms; i++) {
		realm = &nai_home_realms[i];
		int j;
		printf("realm %s\n", realm->realm);
		printf("nai_home_realm_len = %d\n", realm->val_len);
		for (j = 0; j < realm->val_len; j++)
			printf(" %2.2x ", realm->val[j]);
		printf("\n");
	}
#endif
}


int ctrl_conf(void)
{
	FILE *fd = fopen("anqpserver.conf", "r");
	char buf[2048], *pos;
	int errors = 0;

	if (!fd) {
		printf("ERROR: can't read anqpserver.conf\n");
		return 1;
	}

	while (fgets(buf, sizeof(buf), fd)) {
		if (buf[0] == '#')
			continue;
		pos = buf;
		while (*pos != '\0') {
			if (*pos == '\n') {
				*pos = '\0';
				break;
			}
			pos++;
		}
		if (buf[0] == '\0')
			continue;

		pos = strchr(buf, '=');
		if (pos == NULL) {
			printf("invalid line '%s'", buf);
			errors++;
			continue;
		}
		*pos = '\0';
		pos++;

		if (strcmp(buf, "anqp_attach") == 0) {
			strcpy(info_ids, pos);
		} else if (strcmp(buf, "venue_name") == 0) {
			strcpy(venue_name, pos);
		} else if (strcmp(buf, "dyn_venue_name") == 0) {
			dyn_venue_name_len = strlen(pos) / 2;
			if (hexstr2bin(pos, dyn_venue_name, strlen(pos) / 2))
				printf("ERR: dyn_venue_name not a hex str\n");
		} else if (strcmp(buf, "net_auth") == 0) {
			strcpy(net_auth, pos);
		} else if (strcmp(buf, "dyn_net_auth") == 0) {
			dyn_net_auth_len = strlen(pos) / 2;
			if (hexstr2bin(pos, dyn_net_auth, strlen(pos) / 2))
				printf("ERR: dyn_net_auth not a hex str\n");
		} else if (strcmp(buf, "roaming_consortium") == 0) {
			strcpy(roaming_consortium, pos);
		} else if (strcmp(buf, "dyn_roaming_consortium") == 0) {
			dyn_roaming_consortium_len = strlen(pos) / 2;
			if (hexstr2bin(pos, dyn_roaming_consortium,
				       strlen(pos) / 2))
				printf("ERR: dyn_roaming_consortium not a hex str\n");
		} else if (strcmp(buf, "ipaddr_type") == 0) {
			strcpy(ipaddr_type, pos);
		} else if (strcmp(buf, "dyn_ipaddr_type") == 0) {
			dyn_ipaddr_type_len = strlen(pos) / 2;
			if (hexstr2bin(pos, dyn_ipaddr_type, strlen(pos) / 2))
				printf("ERR: dyn_ipaddr_type not a hex str\n");
		} else if (strcmp(buf, "nai_realm") == 0) {
			strcpy(nai_realm, pos);
		} else if (strcmp(buf, "dyn_nai_realm") == 0) {
			dyn_nai_realm_len = strlen(pos) / 2;
			if (hexstr2bin(pos, dyn_nai_realm, strlen(pos) / 2))
				printf("ERR: dyn_nai_realm not a hex str\n");
		} else if (strcmp(buf, "cell_net") == 0) {
			strcpy(cell_net, pos);
		} else if (strcmp(buf, "dyn_cell_net") == 0) {
			dyn_cell_net_len = strlen(pos) / 2;
			if (hexstr2bin(pos, dyn_cell_net, strlen(pos) / 2))
				printf("ERR: dyn_cell_net not a hex str\n");
		} else if (strcmp(buf, "domain_name") == 0) {
			strcpy(domain_name, pos);
		} else if (strcmp(buf, "dyn_domain_name") == 0) {
			dyn_domain_name_len = strlen(pos) / 2;
			if (hexstr2bin(pos, dyn_domain_name, strlen(pos) / 2))
				printf("ERR: dyn_domain_name not a hex str\n");
		} else if (strcmp(buf, "oper_friendly_name") == 0) {
			strcpy(oper_friendly_name, pos);
		} else if (strcmp(buf, "dyn_oper_friendly_name") == 0) {
			dyn_oper_friendly_name_len = strlen(pos) / 2;
			if (hexstr2bin(pos, dyn_oper_friendly_name,
				       strlen(pos) / 2))
				printf("ERR: dyn_oper_friendly_name not a hex str\n");
		} else if (strcmp(buf, "wan_metrics") == 0) {
			strcpy(wan_metrics, pos);
		} else if (strcmp(buf, "dyn_wan_metrics") == 0) {
			dyn_wan_metrics_len = strlen(pos) / 2;
			if (hexstr2bin(pos, dyn_wan_metrics, strlen(pos) / 2))
				printf("ERR: dyn_wan_metrics not a hex str\n");
		} else if (strcmp(buf, "conn_capability") == 0) {
			strcpy(conn_capability, pos);
		} else if (strcmp(buf, "dyn_conn_capability") == 0) {
			dyn_conn_capability_len = strlen(pos) / 2;
			if (hexstr2bin(pos, dyn_conn_capability,
				       strlen(pos) / 2))
				printf("ERR: dyn_conn_capability not a hex str\n");
		} else if (strcmp(buf, "nai_home_realm") == 0) {
			strcpy(nai_home_realm, pos);
		} else if (strcmp(buf, "dyn_nai_home_realm") == 0) {
			parse_nai_home_realm(pos);
		} else if (strcmp(buf, "operating_class") == 0) {
			strcpy(operating_class, pos);
		} else if (strcmp(buf, "dyn_operating_class") == 0) {
			dyn_operating_class_len = strlen(pos) / 2;
			if (hexstr2bin(pos, dyn_operating_class, strlen(pos) / 2))
				printf("ERR: dyn_operating_class not a hex str\n");
		}
	}
	fclose(fd);
	return 0;
}


int process_nai_home_realm_query(uint8_t *buf, int buflen)
{
	uint32_t be_anqp_len, anqp_len;
	uint8_t cmd[1024], *pos = buf + 16, *data;
	int i, j;
	uint8_t *realm, num_realms, num_matching_realms = 0;

	memcpy(cmd, buf, 12);

	data = cmd + 16 + 6;
	num_realms = *pos++;
	anqp_len = 0;
	PUT_LE16(cmd + 16, 263); /* nai realm list info id */
	for (i = 0; i < num_realms; i++) {
		uint8_t encoding, realm_len;
		encoding = *pos++;
		realm_len = *pos++;
		realm = pos;
		for (j = 0; j <= num_nai_home_realms; j++) {
			if (realm_len == nai_home_realms[j].realm_len &&
			    memcmp(realm, nai_home_realms[j].realm, realm_len)
			    == 0) {
				/* matching nai_realm */
				memcpy(data, nai_home_realms[j].val,
				       nai_home_realms[j].val_len);
				anqp_len += nai_home_realms[j].val_len;
				data += nai_home_realms[j].val_len;
				num_matching_realms++;
			}
		}
		pos += realm_len;
	}
	PUT_LE16(cmd + 18, anqp_len + 2);
	PUT_LE16(cmd + 20, num_matching_realms);
	be_anqp_len = htonl(anqp_len + 6);
	memcpy(cmd + 12, &be_anqp_len, 4);
	ctrl_send((char *) cmd, anqp_len + 6 + 16);
	return 0;
}


struct anqp_hdr {
	uint32_t anqp_type;
	uint8_t  sta_addr[6];
	uint8_t  dialog_token;
	uint32_t anqp_len;
	uint8_t  data[0];
};

int process_recv(uint8_t *buf, int buflen)
{
	struct anqp_hdr *hdr = (struct anqp_hdr *) buf;
	uint32_t anqp_type = ntohl(hdr->anqp_type);
	uint32_t be_anqp_len, anqp_len = ntohl(hdr->anqp_len);
	uint8_t cmd[256], *data;

	memcpy(cmd, buf, 12);
	switch (anqp_type) {
	case 0x102:
		data = dyn_venue_name;
		anqp_len = dyn_venue_name_len;
		break;
	case 0x104:
		data = dyn_net_auth;
		anqp_len = dyn_net_auth_len;
		break;
	case 0x105:
		data = dyn_roaming_consortium;
		anqp_len = dyn_roaming_consortium_len;
		break;
	case 0x106:
		data = dyn_ipaddr_type;
		anqp_len = dyn_ipaddr_type_len;
		break;
	case 0x107:
		data = dyn_nai_realm;
		anqp_len = dyn_nai_realm_len;
		break;
	case 0x108:
		data = dyn_cell_net;
		anqp_len = dyn_cell_net_len;
		break;
	case 0x10c:
		data = dyn_domain_name;
		anqp_len = dyn_domain_name_len;
		break;
	case 0x10003:
		data = dyn_oper_friendly_name;
		anqp_len = dyn_oper_friendly_name_len;
		break;
	case 0x10004:
		data = dyn_wan_metrics;
		anqp_len = dyn_wan_metrics_len;
		break;
	case 0x10005:
		data = dyn_conn_capability;
		anqp_len = dyn_conn_capability_len;
		break;
	case 0x10006:
		process_nai_home_realm_query(buf,buflen);
		return 0;
	case 0x10007:
		data = dyn_operating_class;
		anqp_len = dyn_operating_class_len;
		break;
	default:
		printf("ERR: Unrecognized anqp_type 0x%x\n", anqp_type);
		return 1;
	}
	be_anqp_len = htonl(anqp_len);
	memcpy(cmd + 12, &be_anqp_len, 4);
	memcpy(cmd + 16, data, anqp_len);
	ctrl_send((char *) cmd, anqp_len + 16);
	return 0;
}


#define ANQPSERVER_VERSION "version 1"

int main(int argc, char **argv)
{
	int retv = 0;
	int c;

	for (;;) {
		c = getopt(argc, argv, "i:p:vh");
		if (c < 0)
			break;
		switch (c) {
		case 'h':
			printf("%s [-i <vap>] [-p <ctrl-iface-path>]\n"
			       "default vap is first file in default path\n"
			       "default path \"/var/run/hostapd/\"\n",
			       argv[0]);
			return 0;
		case 'v':
			printf("%s\n", ANQPSERVER_VERSION);
			return 0;
		case 'i':
			ctrl_ifname = strdup(optarg);
			break;
		case 'p':
			ctrl_iface_dir = optarg;
			break;
		default:
			printf("%s [-i <vap>] [-p <ctrl-iface-path>]\n"
			       "default vap is first file in default path\n"
			       "default path \"/var/run/hostapd/\"\n",
			       argv[0]);
			return -1;
		}
	}

	if (ctrl_ifname == NULL) {
		struct dirent *dent;
		DIR *dir = opendir(ctrl_iface_dir);
		if (dir) {
			while ((dent = readdir(dir))) {
				if (strcmp(dent->d_name, ".") == 0 ||
				    strcmp(dent->d_name, "..") == 0)
					continue;
				printf("Selected interface '%s'\n",
				       dent->d_name);
				ctrl_ifname = strdup(dent->d_name);
				break;
			}
			closedir(dir);
		}
	}

	if (ctrl_conf()) {
		retv = 1;
		goto _main_exit;
	}
	if (ctrl_open(ctrl_ifname)) {
		retv = 2;
		goto _main_exit;
	}
	if (ctrl_attach()) {
		retv = 3;
		goto _main_exit;
	}
	while (1) {
		char recv_buf[256];
		size_t recv_len = sizeof(recv_buf);
		int ret;
		ret = ctrl_recv(recv_buf, &recv_len);
		if (!ret) {
			printf("recv - hexdump_ascii(len=%lu):\n",
			       (unsigned long) recv_len);
			size_t i, llen;
			const size_t line_len = 16;
			char *pos = recv_buf;
			while (recv_len) {
				llen = recv_len > line_len ?
					line_len : recv_len;
				printf("    ");
				for (i = 0; i < llen; i++)
					printf(" %02x", pos[i]);
				for (i = llen; i < line_len; i++)
					printf("   ");
				printf("   ");
				for (i = 0; i < llen; i++) {
					if (isprint(pos[i]))
						printf("%c", pos[i]);
					else
						printf("_");
				}
				for (i = llen; i < line_len; i++)
					printf(" ");
				printf("\n");
				pos += llen;
				recv_len -= llen;
			}
			process_recv((uint8_t *)recv_buf, recv_len);
		} else {
			printf("recv failed (%d)\n", ret);
			break;
		}
	}
_main_exit:
	ctrl_close();
	return retv;
}
