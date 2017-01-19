#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>

#include "vrx218_common.h"


int g_dbg_enable = DBG_ENABLE_MASK_ERR;
int g_dump_cnt = -1;

struct host_desc_mem g_host_desc_base = {0};


int print_fw_ver(char *buf, int buf_len, struct fw_ver_id ver)
{
    static char * fw_ver_family_str[] = {
        "reserved - 0",
        "Danube",
        "Twinpass",
        "Amazon-SE",
        "reserved - 4",
        "AR9",
        "GR9",
        "VR9",
        "AR10",
        "VRX318",
    };

    static char * fw_ver_package_str[] = {
        "Reserved - 0",
        "A1",
        "B1 - PTM_BONDING",
        "E1",
        "A5",
        "D5",
        "D5v2",
        "E5",
    };

    static char *fw_feature[] = {
		"  ATM/PTM TC-Layer                Support",
		"  ATM/PTM TC-Layer Retransmission Support",
		"  ATM/PTM TC-Layer Bonding        Support",
		"  L2 Trunking                     Support",
		"  Packet Acceleration             Support",
		"  IPv4                            Support",
		"  IPv6                            Support",
		"  6RD                             Support",
		"  DS-Lite                         Support",
		"  Unified FW QoS                  Support",
	};

    int len = 0;
    int i;

    len += snprintf(buf + len, buf_len - len, "PPE firmware info:\n");
    len += snprintf(buf + len, buf_len - len,     "  Version ID: %d.%d.%d.%d.%d\n", (int)ver.family, (int)ver.package, (int)ver.major, (int)ver.middle, (int)ver.minor);
    if ( ver.family >= ARRAY_SIZE(fw_ver_family_str) )
        len += snprintf(buf + len, buf_len - len, "  Family    : reserved - %d\n", (int)ver.family);
    else
        len += snprintf(buf + len, buf_len - len, "  Family    : %s\n", fw_ver_family_str[ver.family]);

    if ( ver.package >= ARRAY_SIZE(fw_ver_package_str) )
        len += snprintf(buf + len, buf_len - len, "  FW Package: reserved - %d\n", (int)ver.package);
    else
        len += snprintf(buf + len, buf_len - len, "  FW Package: %s\n", fw_ver_package_str[ver.package]);

    len += snprintf(buf + len, buf_len - len,     "  Release   : %u.%u.%u\n", (int)ver.major, (int)ver.middle, (int)ver.minor);

    len += snprintf(buf + len, buf_len - len, "PPE firmware feature:\n");

    for(i = 0; i < ARRAY_SIZE(fw_feature); i ++){
        if(ver.features & (1 << (31-i))){
            len += snprintf(buf + len, buf_len - len, "%s\n", fw_feature[i]);
        }
    }

    return len;
}

int proc_buf_copy(char **pbuf, int size, off_t off, int *ppos, const char *str, int len)
{
    if ( *ppos <= off && *ppos + len > off )
    {
        memcpy(*pbuf, str + off - *ppos, *ppos + len - off);
        *pbuf += *ppos + len - off;
    }
    else if ( *ppos > off )
    {
        memcpy(*pbuf, str, len);
        *pbuf += len;
    }
    *ppos += len;
    return *ppos >= off + size;
}

int get_token(char **p1, char **p2, int *len, int *colon)
{
    int tlen = 0;

    while ( *len && !((**p1 >= 'A' && **p1 <= 'Z') || (**p1 >= 'a' && **p1<= 'z') || (**p1 >= '0' && **p1<= '9')) )
    {
        (*p1)++;
        (*len)--;
    }
    if ( !*len )
        return 0;

    if ( *colon )
    {
        *colon = 0;
        *p2 = *p1;
        while ( *len && **p2 > ' ' && **p2 != ',' )
        {
            if ( **p2 == ':' )
            {
                *colon = 1;
                break;
            }
            (*p2)++;
            (*len)--;
            tlen++;
        }
        **p2 = 0;
    }
    else
    {
        *p2 = *p1;
        while ( *len && **p2 > ' ' && **p2 != ',' )
        {
            (*p2)++;
            (*len)--;
            tlen++;
        }
        **p2 = 0;
    }

    return tlen;
}

int get_number(char **p, int *len, int is_hex)
{
    int ret = 0;
    int n = 0;

    if ( (*p)[0] == '0' && (*p)[1] == 'x' )
    {
        is_hex = 1;
        (*p) += 2;
        (*len) -= 2;
    }

    if ( is_hex )
    {
        while ( *len && ((**p >= '0' && **p <= '9') || (**p >= 'a' && **p <= 'f') || (**p >= 'A' && **p <= 'F')) )
        {
            if ( **p >= '0' && **p <= '9' )
                n = **p - '0';
            else if ( **p >= 'a' && **p <= 'f' )
               n = **p - 'a' + 10;
            else if ( **p >= 'A' && **p <= 'F' )
                n = **p - 'A' + 10;
            ret = (ret << 4) | n;
            (*p)++;
            (*len)--;
        }
    }
    else
    {
        while ( *len && **p >= '0' && **p <= '9' )
        {
            n = **p - '0';
            ret = ret * 10 + n;
            (*p)++;
            (*len)--;
        }
    }

    return ret;
}

void ignore_space(char **p, int *len)
{
    while ( *len && (**p <= ' ' || **p == ':' || **p == '.' || **p == ',') )
    {
        (*p)++;
        (*len)--;
    }
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
int strncasecmp(const char *p1, const char *p2, int n)
{
    int c1 = 0, c2;

    while ( n && *p1 && *p2 )
    {
        c1 = *p1 >= 'A' && *p1 <= 'Z' ? *p1 + 'a' - 'A' : *p1;
        c2 = *p2 >= 'A' && *p2 <= 'Z' ? *p2 + 'a' - 'A' : *p2;
        if ( (c1 -= c2) )
            return c1;
        p1++;
        p2++;
        n--;
    }

    return n ? *p1 - *p2 : c1;
}

int strcasecmp(const char *p1, const char *p2)
{
    int c1, c2;

    while ( *p1 && *p2 )
    {
        c1 = *p1 >= 'A' && *p1 <= 'Z' ? *p1 + 'a' - 'A' : *p1;
        c2 = *p2 >= 'A' && *p2 <= 'Z' ? *p2 + 'a' - 'A' : *p2;
        if ( (c1 -= c2) )
            return c1;
        p1++;
        p2++;
    }

    return *p1 - *p2;
}
#endif
