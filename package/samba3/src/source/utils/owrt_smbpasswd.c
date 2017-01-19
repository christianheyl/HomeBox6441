/*
 * Copyright (C) John Crispin <blogic@openwrt.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 675
 * Mass Ave, Cambridge, MA 02139, USA.  */

#include "includes.h"
#include <endian.h>

#define arc_dbg_printf(fmt, args...) do { \
        FILE *fp = fopen("/dev/console", "w"); \
        if (fp) { \
                fprintf(fp, fmt , ## args); \
                fprintf(fp, "\n"); \
                fclose(fp); \
        } \
        else { \
            char *pszBuf = malloc(strlen(fmt)+30); \
            char *pszCmd = malloc(strlen(fmt)+1024); \
            snprintf(pszBuf, (strlen(fmt)+30), "echo %s >/dev/console", fmt); \
            snprintf(pszCmd, (strlen(fmt)+1024), pszBuf, ## args); \
            system(pszCmd); \
            free(pszBuf); \
            free(pszCmd); \
        } \
} while (0)
/* 2014/01/16, jeffery, change format from utf8 to utf16 */
#define UTF_SWAP_WORD(s)    ((((s)&0x00ff)<<8)+(((s)&0xff00)>>8))

/* 2014/01/16, jeffery, change format from utf8 to utf16 */
int utf8to16_word(const unsigned char *src, unsigned short *dest, unsigned int dest_size,
            unsigned char change_dest_order, int *src_proc_len )
{
    unsigned long c;
    int extra_bytes;
    int len = 0;
	unsigned short c2;

    *src_proc_len = 0;
    c = (unsigned long)*src++ & 0xFFUL;
    (*src_proc_len)++;

    if((c & 0x80UL) == 0UL) //1 octet
    {
    	c2 = (unsigned short)c;
    	if( change_dest_order )
            c2 = (unsigned short)UTF_SWAP_WORD(c2);
        *dest++ = c2;
        len++;
        return len ;
    }
    else if((c & 0xE0UL) == 0xC0UL) //2 octet
    {
        c -= 0xC0UL;
        extra_bytes = 1;
    }
    else if((c & 0xF0UL) == 0xE0UL) // 3 octet
    {
        c -= 0xE0UL;
        extra_bytes = 2;
    }
    else if((c & 0xF8UL) == 0xF0UL) // 4 octet
    {
        c -= 0xF0UL;
        extra_bytes = 3;
    }
    else
    {
        //5 or 6 octets cannot be converted to UTF-16
        return 0;
    }

    while(extra_bytes)
    {
        if(*src == 0) return 0; //unexpected end of string
        if((*src & 0xC0UL) != 0x80UL) return 0; //illegal trailing byte

        c <<= 6;
        c += (unsigned long)*src++ & 0x3FUL;
        (*src_proc_len)++;

        extra_bytes--;
    }

    if(c < 0x10000UL)
    {
        //value between 0xD800 and 0xDFFF are preserved for UTF-16 pairs
        if(c >= 0xD800UL && c <= 0xDFFFUL) return 0;

    	c2 = (unsigned short)c;
    	if( change_dest_order )
            c2 = (unsigned short)UTF_SWAP_WORD(c);
        *dest++ = c2;
        len++;
    }
    else
    {
        c -= 0x10000UL;

        //value greater than 0x10FFFF, illegal UTF-16 value;
        if(c >= 0x100000UL) return 0;

		if( len+2 > dest_size ){
			arc_dbg_printf("[%s, %s, %d]Error! buffer overflow, dest_size=%d, len=%d\n", __FILE__, __func__, __LINE__, dest_size, len + 2);
            return len;
		}

        c2 = (unsigned short)(0xD800UL + (c >> 10));
    	if( change_dest_order )
            c2 = (unsigned short)UTF_SWAP_WORD(c2);
        *dest++ = c2;

        c2 = (unsigned short)(0xDC00UL + (c & 0x3FFUL));
    	if( change_dest_order )
            c2 = (unsigned short)UTF_SWAP_WORD(c2);
		*dest++ = c2 ;
        len += 2;
     }

    return len ;
}

/* 2014/01/16, jeffery, change format from utf8 to utf16 */
int utf8to16_order(unsigned char *src, unsigned short *dest, unsigned int dest_size, unsigned char change_dest_order)
{
	int len, tmp_words, src_proc_len ;
	unsigned short *dest_org = dest;
	unsigned short *dest_end = (unsigned short *)((unsigned char *)dest + dest_size - 2); // -2: unsigned short
    unsigned short tmp[4] ;

    len = 0 ;
    while(*src)
    {
        tmp_words = utf8to16_word( src, tmp, sizeof(tmp)/sizeof(unsigned short), change_dest_order, &src_proc_len );
        if( tmp_words==0 ) return 0; // fail

        src += src_proc_len ;

    	if( dest + tmp_words > dest_end ){
    		arc_dbg_printf("[%s, %s, %d]Error! buffer overflow, dest_size=%d, len=%d (bytes)\n", __FILE__, __func__, __LINE__, dest_size, len * 2);
    		len = dest - dest_org ;
    		break;
    	}

        switch(tmp_words){
        case 4: dest[3] = tmp[3] ;
        case 3: dest[2] = tmp[2] ;
        case 2: dest[1] = tmp[1] ;
        case 1: dest[0] = tmp[0] ;
        }

        dest += tmp_words ;
        len += tmp_words ;
    }

    *dest = (unsigned short) 0;
    return len;
}

void E_md4hash(const char *passwd, uchar p16[16])
{
	int len;
	smb_ucs2_t wpwd[129];
	int i, wordCnt, byteCnt;

#ifdef ARCADYAN_DT_724_UTF8_TO_UTF16	/* 2014-01-16, change from utf8 to utf16 for DT-724 */
	wordCnt = utf8to16_order((unsigned char *)passwd, wpwd, sizeof(wpwd), 1) ;  // device is big-endian
	
	#ifdef SMBPASSWD_DEBUG
	arc_dbg_printf("[%s, %s, %d]wordCnt = %d", __FILE__, __func__, __LINE__, wordCnt);
	for (i = 0; i < wordCnt; i++)
		arc_dbg_printf("[%s, %s, %d]wpwd = %x", __FILE__, __func__, __LINE__, wpwd[i]);
	#endif
	byteCnt = wordCnt * 2;
	mdfour(p16, (unsigned char *)wpwd, byteCnt);
#else
	len = strlen(passwd);
	for (i = 0; i < len; i++) {
		#if __BYTE_ORDER == __LITTLE_ENDIAN
		wpwd[i] = (unsigned char)passwd[i];
		arc_dbg_printf("[%s, %s, %d]pwd[%d] = %x", __FILE__, __func__, __LINE__, i, wpwd[i]);
		#else
		wpwd[i] = (unsigned char)passwd[i] << 8;
		arc_dbg_printf("[%s, %s, %d]pwd[%d] = %x", __FILE__, __func__, __LINE__, i, wpwd[i]);
		#endif
	}
	wpwd[i] = 0;

	len = len * sizeof(int16);
	mdfour(p16, (unsigned char *)wpwd, len);
#endif
	ZERO_STRUCT(wpwd);
}

/* returns -1 if user is not present in /etc/passwd*/
int find_uid_for_user(char *user)
{
	char t[256];
	FILE *fp = fopen("/etc/passwd", "r");
	int ret = -1;

	if(!fp)
	{
		printf("failed to open /etc/passwd");
		goto out;
	}

	while(!feof(fp))
	{
		if(fgets(t, 255, fp))
		{
			char *p1, *p2;
			p1 = strchr(t, ':');
			if(p1 && (p1 - t == strlen(user)) && (strncmp(t, user, strlen(user))) == 0)
			{
				p1 = strchr(t, ':');
				if(!p1)
					goto out;
				p2 = strchr(++p1, ':');
				if(!p2)
					goto out;
				p1 = strchr(++p2, ':');
				if(!p1)
					goto out;
				*p1 = '\0';
				ret = atoi(p2);
				goto out;
			}
		}
	}
	printf("No valid user found in /etc/passwd\n");

out:
	if(fp)
		fclose(fp);
	return ret;
}

void insert_user_in_smbpasswd(char *user, char *line)
{
	char t[256];
	FILE *fp = fopen("/etc/samba/smbpasswd", "r+");

	if(!fp)
	{
		printf("failed to open /etc/samba/smbpasswd");
		goto out;
	}

	while(!feof(fp))
	{
		if(fgets(t, 255, fp))
		{
			char *p;
			p = strchr(t, ':');
			if(p && (p - t == strlen(user)) && (strncmp(t, user, strlen(user))) == 0)
			{
				fseek(fp, -strlen(line), SEEK_CUR);
				break;
			}
		}
	}

	fprintf(fp, line);

out:
	if(fp)
		fclose(fp);
}

void delete_user_from_smbpasswd(char *user)
{
	char t[256];
	FILE *fp = fopen("/etc/samba/smbpasswd", "r+");

	if(!fp)
	{
		printf("failed to open /etc/samba/smbpasswd");
		goto out;
	}

	while(!feof(fp))
	{
		if(fgets(t, 255, fp))
		{
			char *p;
			p = strchr(t, ':');
			if(p && (p - t == strlen(user)) && (strncmp(t, user, strlen(user))) == 0)
			{
				fpos_t r_pos, w_pos;
				char t2[256];
				fgetpos(fp, &r_pos);
				w_pos = r_pos;
				w_pos.__pos -= strlen(t);
				while(fgets(t2, 256, fp))
				{
					fsetpos(fp, &w_pos);
					fputs(t2, fp);
					r_pos.__pos += strlen(t2);
					w_pos.__pos += strlen(t2);
					fsetpos(fp, &r_pos);
				}
				ftruncate(fileno(fp), w_pos.__pos);
				break;
			}
		}
	}

out:
	if(fp)
		fclose(fp);
}

int main(int argc, char **argv)
{
	unsigned uid;
	uchar new_nt_p16[NT_HASH_LEN];
	int g;
	int smbpasswd_present;
	char smbpasswd_line[256];
	char *s;
	//char jeffery_password[10] = {0x61, 0x31, 0x32, 0x33, 0x34, 0xE4, 0x35, 0x36, 0x37, 0x00};	/* test only */
	int nPasswordIndex;

	#ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD	/* arcadyan, encrypted NULL password of MD4 */
	uchar null_nt_p16[16] = {0x31, 0xd6, 0xcf, 0xe0, 0xd1, 0x6a, 0xe9, 0x31, 0xb7, 0x3c, 0x59, 0xd7, 0xe0, 0xc0, 0x89, 0xc0};
	#endif

	if(argc != 3)
	{
		printf("usage for openwrt_smbpasswd - \n\t%s USERNAME PASSWD\n\t%s -del USERNAME\n", argv[0], argv[0]);
		exit(1);
	}
	if(strcmp(argv[1], "-del") == 0)
	{
		printf("deleting user %s\n", argv[2]);
		delete_user_from_smbpasswd(argv[2]);
		return 0;
	}
	uid = find_uid_for_user(argv[1]);
	if(uid == -1)
		exit(2);

	#ifdef SMBPASSWD_DEBUG
    arc_dbg_printf("[%s, %s, %d]Username: %s\nPassword: %s\nHex of Password as below,", __FILE__, __func__, __LINE__, argv[1], argv[2]);
    for (nPasswordIndex = 0; nPasswordIndex < strlen(argv[2]); nPasswordIndex++)
        arc_dbg_printf("[%s, %s, %d]Password[%d] = %x", __FILE__, __func__, __LINE__, nPasswordIndex, argv[2][nPasswordIndex]);
	#endif

	E_md4hash(argv[2], new_nt_p16);

	s = smbpasswd_line;
	s += snprintf(s, 256 - (s - smbpasswd_line), "%s:%u:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:", argv[1], uid);
	for(g = 0; g < 16; g++)
	#ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD /* arcadyan, for MD4 of gast account */
	{
		if (strcmp(argv[1], "GUEST") == 0)
			s += snprintf(s, 256 - (s - smbpasswd_line), "%02X", null_nt_p16[g]);
		else
			s += snprintf(s, 256 - (s - smbpasswd_line), "%02X", new_nt_p16[g]);
	}
	#else
		s += snprintf(s, 256 - (s - smbpasswd_line), "%02X", new_nt_p16[g]);
	#endif
	snprintf(s, 256 - (s - smbpasswd_line), ":[U          ]:LCT-00000001:\n");

	insert_user_in_smbpasswd(argv[1], smbpasswd_line);

	return 0;
}
