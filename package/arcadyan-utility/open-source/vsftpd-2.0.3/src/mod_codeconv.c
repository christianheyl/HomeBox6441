/*
 * ProFTPD: mod_codeconv -- local <-> remote charset conversion
 *
 * Copyright (c) 2004 by TSUJIKAWA Tohru <tsujikawa@tsg.ne.jp> / All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#define NULL 0
//#include	"conf.h"
#include        "include/converters.h"
#define GB2312
#ifdef GB2312
#include "include/gb2312.h"
#include "include/gbk.h"
#include "include/gb18030.h"
#endif
//#define BIG5
#ifdef BIG5
#include "include/big5.h"
#endif
#define CP1258
#ifdef CP1258
#include "include/cp1258.h"
#endif
#define ISO8859
#ifdef ISO8859
#include "include/iso8859_1.h"
#endif
//
// directive
//
#define	DIRECTIVE_CHARSETLOCAL		"CharsetLocal"
#define	DIRECTIVE_CHARSETREMOTE		"CharsetRemote"


//
// initialization
//
static int codeconv_init(void)
{
	return 0;
}

static int codeconv_sess_init(void)
{
	return 0;
}

char* DeCodeString(char* inString,int charset)
{
	int iLenA,iLenB,iLoopA,iLoopB;
	unsigned short* outStringA;
	char* outStringB;
	ucs4_t ucs4Out;
	int iRet;
	iLenA = strlen(inString);
	if(iLenA==0)
	{
		return 0;
	}
	outStringA = (unsigned short*)malloc(iLenA*sizeof(unsigned short));
	if(outStringA==0)
	{
		return 0;
	}
	memset(outStringA,0,iLenA*sizeof(unsigned short));
	for(iLoopA=0,iLoopB=0;iLoopA<iLenA;iLoopB++)
	{
		ucs4Out = 0;
		iRet = utf8_mbtowc(0,&ucs4Out,(unsigned char*)inString+iLoopA,4);
		if(iRet==-1)
		{
			free(outStringA);
			return 0;
		}
		else
		{
			outStringA[iLoopB] = ucs4Out;
			iLoopA += iRet;
		}
	}
	iLenA = iLoopB;
	outStringB = (char*)malloc(iLenA*2+1);
	if(outStringB==0)
	{
		free(outStringA);
		return 0;
	}
	memset(outStringB,0,iLenA*2+1);
	for(iLoopA=0,iLoopB=0;iLoopA<iLenA;iLoopA++)
	{
		switch(charset)
		{
		case 1:	
#ifdef GB2312	
			iRet = gb18030_wctomb(0,(unsigned char*)outStringB+iLoopB,outStringA[iLoopA],iLenA*3-iLoopB);
#endif
			break;
		case 2:
#ifdef BIG5		
			iRet = big5_wctomb(0,(unsigned char*)outStringB+iLoopB,outStringA[iLoopA],iLenA*3-iLoopB);
#endif
			break;
		case 3:
#ifdef CP1258		
			iRet = cp1258_wctomb(0,(unsigned char*)outStringB+iLoopB,outStringA[iLoopA],iLenA*3-iLoopB);
#endif
			break;
		case 4:
#ifdef ISO8859		
			iRet = iso8859_1_wctomb(0,(unsigned char*)outStringB+iLoopB,outStringA[iLoopA],iLenA*3-iLoopB);
#endif
			break;
		default:
			iRet=-1;
		}		
		if(iRet==-1)
		{
			free(outStringA);
			free(outStringB);
			return 0;
		}
		else
		{
			iLoopB += iRet;
		}
	}
	free(outStringA);
	return outStringB;
}

char* EnCodeString(char* inString,int charset)
{
	int iLenA,iLenB,iLoopA,iLoopB;
	unsigned short* outStringA;
	char* outStringB;
	ucs4_t ucs4Out;
	struct conv_struct conv;
	int iRet;
	iLenA = strlen(inString);
	if(iLenA==0)
	{
		return 0;
	}
	outStringA = (unsigned short*)malloc(iLenA*sizeof(unsigned short));
	if(outStringA==0)
	{
		return 0;
	}
	memset(outStringA,0,iLenA*sizeof(unsigned short));
	for(iLoopA=0,iLoopB=0;iLoopA<iLenA;iLoopB++)
	{
		ucs4Out = 0;
		switch(charset)
		{
		case 1:	
#ifdef GB2312	
			iRet = gb18030_mbtowc(0,&ucs4Out,(unsigned char*)inString+iLoopA,2);
#endif
			break;
		case 2:	
#ifdef BIG5	
			iRet = big5_mbtowc(0,&ucs4Out,(unsigned char*)inString+iLoopA,2);
#endif
			break;
		case 3:
#ifdef CP1258	
			conv.istate = 0;
			iRet = cp1258_mbtowc(&conv,&ucs4Out,(unsigned char*)inString+iLoopA,2);
#endif
			break;
		case 4:
#ifdef ISO8859		
			iRet = iso8859_1_mbtowc(0,&ucs4Out,(unsigned char*)inString+iLoopA,2);
#endif
			break;
		default:
			iRet = -1;
		}
		if(iRet==-1)
		{
			free(outStringA);
			return 0;
		}
		else
		{
			outStringA[iLoopB] = ucs4Out;
			iLoopA += iRet;
		}
	}
	iLenA = iLoopB;
	outStringB = (char*)malloc(iLenA*3+1);
	if(outStringB==0)
	{
		free(outStringA);
		return 0;
	}
	memset(outStringB,0,iLenA*3+1);
	for(iLoopA=0,iLoopB=0;iLoopA<iLenA;iLoopA++)
	{
		iRet = utf8_wctomb(0,(unsigned char*)outStringB+iLoopB,outStringA[iLoopA],iLenA*3-iLoopB);
		if(iRet==-1)
		{
			free(outStringA);
			free(outStringB);
			return 0;
		}
		else
		{
			iLoopB += iRet;
		}
	}
	free(outStringA);
	return outStringB;
}

char* remote2local(char* remote,int charset)
{
	char* out_ptr;
	//cprintf("remote=(%s)\r\n",remote);
	out_ptr = EnCodeString(remote,charset);
	if(out_ptr)
	{
		//cprintf("out_ptr=(%s)\r\n",out_ptr);
	}
	return out_ptr;
}
char* local2remote(char* local,int charset)
{
	char* out_ptr;
	//cprintf("local=(%s)\r\n",local);
	out_ptr = DeCodeString(local,charset);
        if(out_ptr)
        {
                //cprintf("out_ptr=(%s)\r\n",out_ptr);
        }
	return out_ptr;
}
#if 0
char* remote2local(struct pool* pool, char* remote)
{
	iconv_t	ic;
	char*	local;
	char*	in_ptr;
	char*	out_ptr;
	size_t	inbytesleft, outbytesleft;

	config_rec*	conf_l = NULL;
	config_rec*	conf_r = NULL;
	cprintf("file(%s)\r\n",main_server->conf);
	conf_l = find_config(main_server->conf, CONF_PARAM, DIRECTIVE_CHARSETLOCAL, FALSE);
	conf_r = find_config(main_server->conf, CONF_PARAM, DIRECTIVE_CHARSETREMOTE, FALSE);
	cprintf("conf_l(%s),conf_r(%s)\r\n",conf_l->argv[0],conf_r->argv[0]);
	if (!conf_l || !conf_r) return NULL;

	ic = iconv_open("UTF-8","GB2312");//conf_l->argv[0], conf_r->argv[0]);
	cprintf("ic=%d,error=%d\r\n",ic,errno);
	if (ic == (iconv_t)(-1)) return NULL;

	iconv(ic, NULL, NULL, NULL, NULL);

	inbytesleft = strlen(remote);
	outbytesleft = inbytesleft*3;
	local = palloc(pool, outbytesleft+1);

	in_ptr = remote; out_ptr = local;
	while (inbytesleft) {
		if (iconv(ic, &in_ptr, &inbytesleft, &out_ptr, &outbytesleft) == -1) {
			*out_ptr = '?'; out_ptr++; outbytesleft--;
			in_ptr++; inbytesleft--;
			break;
		}
	}
	*out_ptr = 0;

	iconv_close(ic);

	return local;
}


char* local2remote(char* local)
{
	iconv_t	ic;
	char*	remote;
	char*	in_ptr;
	char*	out_ptr;
	size_t	inbytesleft, outbytesleft;

	config_rec*	conf_l = NULL;
	config_rec*	conf_r = NULL;

	conf_l = find_config(main_server->conf, CONF_PARAM, DIRECTIVE_CHARSETLOCAL, FALSE);
	conf_r = find_config(main_server->conf, CONF_PARAM, DIRECTIVE_CHARSETREMOTE, FALSE);
	cprintf("conf_l(%s),conf_r(%s)\r\n",conf_l->argv[0],conf_r->argv[0]);
	if (!conf_l || !conf_r) return NULL;

	ic = iconv_open("GB2312","UTF-8");//conf_r->argv[0], conf_l->argv[0]);
	cprintf("1111ic=%d,error=%d\r\n",ic,errno);
	if (ic == (iconv_t)(-1)) return NULL;

	iconv(ic, NULL, NULL, NULL, NULL);

	inbytesleft = strlen(local);
	outbytesleft = inbytesleft*3;
	remote = malloc(outbytesleft+1);

	in_ptr = local; out_ptr = remote;
	while (inbytesleft) {
		if (iconv(ic, &in_ptr, &inbytesleft, &out_ptr, &outbytesleft) == -1) {
			*out_ptr = '?'; out_ptr++; outbytesleft--;
			in_ptr++; inbytesleft--;
			break;
		}
	}
	*out_ptr = 0;

	iconv_close(ic);

	return remote;
}

#endif
//
// module handler
//
#if 0
MODRET codeconv_pre_any(cmd_rec* cmd)
{
	char*	p;
	int		i;

	p = remote2local(cmd->pool, cmd->arg);
	if (p) cmd->arg = p;

	for (i = 0; i < cmd->argc; i++) {
		p = remote2local(cmd->pool, cmd->argv[i]);
		if (p) cmd->argv[i] = p;
	}

	return DECLINED(cmd);
}


//
// local charset directive "CharsetLocal"
//
MODRET set_charsetlocal(cmd_rec *cmd) {
  config_rec *c = NULL;

  /* Syntax: CharsetLocal iconv-charset-name */

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  c = add_config_param_str(DIRECTIVE_CHARSETLOCAL, 1, cmd->argv[1]);

  return HANDLED(cmd);
}

//
// remote charset directive "CharsetRemote"
//
MODRET set_charsetremote(cmd_rec *cmd) {
  config_rec *c = NULL;

  /* Syntax: CharsetRemote iconv-charset-name */

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  c = add_config_param_str(DIRECTIVE_CHARSETREMOTE, 1, cmd->argv[1]);

  return HANDLED(cmd);
}


//
// module 用 directive
//
static conftable codeconv_conftab[] = {
	{ DIRECTIVE_CHARSETLOCAL,		set_charsetlocal,		NULL },
	{ DIRECTIVE_CHARSETREMOTE,		set_charsetremote,		NULL },
	{ NULL, NULL, NULL }
};


//
// trap するコマンド一覧
//
static cmdtable codeconv_cmdtab[] = {
	{ PRE_CMD,		C_ANY,	G_NONE, codeconv_pre_any,	FALSE, FALSE },
	{ 0,			NULL }
};


//
// module 情報
//
module codeconv_module = {

	/* Always NULL */
	NULL, NULL,

	/* Module API version (2.0) */
	0x20,

	/* Module name */
	"codeconv",

	/* Module configuration directive handlers */
	codeconv_conftab,

	/* Module command handlers */
	codeconv_cmdtab,

	/* Module authentication handlers (none in this case) */
	NULL,

	/* Module initialization */
	codeconv_init,

	/* Session initialization */
	codeconv_sess_init

};
#endif
