/* 
   Unix SMB/CIFS implementation.
   Password and authentication handling
   Copyright (C) Andrew Tridgell              1992-2000
   Copyright (C) Luke Kenneth Casson Leighton 1996-2000
   Copyright (C) Andrew Bartlett              2001-2003
   Copyright (C) Gerald Carter                2003
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "includes.h"
#ifdef ARCADYAN_STASTISTIC_CFG
#include "statistic_main.h"
#endif

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_AUTH

#ifdef ARCADYAN_DT_724_SAMBA_EVENT_LOG
#define SMB_W724_MAIL_SUBJECT                               "Speedport W 724V - E-Mail-Benachrichtigung"


#ifdef SUPPORT_CCFG_GET_SET_LONG
//Simon@2014/04/30, ALDK middle layer only support mapi_ccfg_get_str/mapi_ccfg_set_str
//char * mapi_ccfg_get_str(int tid, const char * name, char * buf, int size);
long mapi_ccfg_get_long(int tid, char* buf, char* name, long lDft )
{
	char	sVal[16];
	long	lVal;
	char*	pEnd;

	mapi_ccfg_get_str( tid, name, buf, sizeof(buf));

	lVal = strtol( sVal, &pEnd, 0 );

	if ( pEnd == sVal )
		return lDft;

	return lVal;
}

int mapi_ccfg_set_long( int tid, char* sect, char* name, long lVal )
{
	char	sVal[16];

	snprintf( sVal, 16, "%ld", (long int)lVal );

	return mapi_ccfg_set_str( tid, sect, name, sVal );
}
#endif

#ifdef DISABLE_CFG_SECTION_NAME_SUPPORT
/*Simon@2014/05/08, new prototype for mapi_ccfg_get_str
from
char * mapi_ccfg_get_str(int tid, char * section_name, char * name, char * buf, int size, char * default_value);
to
char * mapi_ccfg_get_str(int tid, const char * name, char * buf, int size);
and
char * mapi_ccfg_set_str(int tid, char * section_name, char * name, char * buf);
to
char * mapi_ccfg_set_str(int tid, const char * name, const char * buf);
*/
#define mapi_ccfg_get_str(tid, section, name, buf, size, def) mapi_ccfg_get_str((tid), (name), (buf), (size))
#define mapi_ccfg_set_str(tid, section, name, buf) mapi_ccfg_set_str((tid), (name), (buf))
#endif

static int nShowLoginWindow = False;    /* used for clicking icon of Speedport or typing "\\<ip_address of server>" */

#ifdef ARCADYAN_STASTISTIC_CFG
/**************************************************************
 **  purpose: get integer value of item from USB_DEVICE section
 **
 **  Input
 **      @szItemName: Item name which save in USB_DEVICE section
 **      @nValue: return value
 **
 **  return value: 0 success, -1 failed.
 **************************************************************/
int statistic_CFG_int_get(char *szItemName, int *nValue)
{
    stat_loc_t      loc;
    unsigned char   buf[64];

    /* open section area */
    if ((stat_open(STAT_CATALOG_USB_DEVICE_INDEX) < 0) )
    {
        DEBUG(10, ("statistic cfg open error\n"));
        return -1;
    }

    /* seek the designated item */
    if (stat_seek_loc(&loc, STAT_CATALOG_USB_DEVICE_INDEX, szItemName) <= 0) /* less or equal to zero */
    {
        DEBUG(10, ("stat_seek_loc error in get\n"));
        stat_close(STAT_CATALOG_USB_DEVICE_INDEX);
        return -1;
    }

    /* get value of item */
    stat_get_val(&loc, buf);
    *nValue = *(int*)buf;

    /* successful case and close section area */
    stat_close(STAT_CATALOG_USB_DEVICE_INDEX);

    return 0;
}

/**************************************************************
 **  purpose: set integer value of item from USB_DEVICE section
 **
 **  Input
 **      @szItemName: Item name which save in USB_DEVICE section
 **      @nValue: int value
 **
 **  return value: 0 success, -1 failed.
 **************************************************************/
int statistic_CFG_int_set(char *szItemName, int nValue)
{
    stat_loc_t      loc;

    /* open section area */
    if ( (stat_open(STAT_CATALOG_USB_DEVICE_INDEX) < 0) )
    {
        DEBUG(10, ("statistic cfg open error\n"));
        return -1;
    }

    /* seek the designated item */
    if (stat_seek_loc(&loc, STAT_CATALOG_USB_DEVICE_INDEX, szItemName) < 0) /* less than zero */
    {
        DEBUG(10, ("stat_seek_loc error in set\n"));
        stat_close(STAT_CATALOG_USB_DEVICE_INDEX);
        return -1;
    }

    /* set value of item */
    stat_set_val(&loc, &nValue, sizeof(int));

    /* successful case and close section area */
    stat_close(STAT_CATALOG_USB_DEVICE_INDEX);

    return 0;
}

static BOOL smb_statistic_cfg_set_int(const char *username, const char *pstSubItemName, const int nItemValue)
{
	int		ret				= True;
	char    *pstItemName    = NULL; /* for item of statistic, smb_<username>_xxx */

	asprintf(&pstItemName, "%s_%s", username, pstSubItemName);
    DEBUG(10, ("pstItemName is %s\n", pstItemName));
    if (statistic_CFG_int_set(pstItemName, nItemValue) == -1)
    {
        DEBUG(10, ("failed to set %s/%s from statistic cfg\n", pstItemName, "USB_DEVICE"));
		ret = False;
    }
    free(pstItemName);

	return ret;
}

static BOOL smb_statistic_cfg_get_int(const char *username, const char *pstSubItemName, int *pstValue)
{
	int		ret				= True;
	char    *pstItemName    = NULL; /* for item of statistic, smb_<username>_xxx */

	asprintf(&pstItemName, "%s_%s", username, pstSubItemName);
    DEBUG(10, ("pstItemName is %s\n", pstItemName));
    if (statistic_CFG_int_get(pstItemName, pstValue) == -1)
    {
        DEBUG(10, ("failed to get %s/%s from statistic cfg\n", pstItemName, "USB_DEVICE"));
    	ret = False;
	}
    DEBUG(10, ("pstValue is %d\n", pstValue));
    free(pstItemName);

	return ret;
}
#endif

/**************************************************************************
 ** set event log, F103, to log system of DT724
 ** Description: User access for reactivated (after 60 min timeout expired)
 *************************************************************************/
#ifdef ARCADYAN_STASTISTIC_CFG
static BOOL smb_set_event_log_F103(const char *username)
#else
static BOOL smb_set_event_log_F103(const int tid, const char *pstSectionName, const char *username)
#endif
{
	char	*pstLogCmd      = NULL;

	#ifdef ARCADYAN_STASTISTIC_CFG
	if (username == NULL)
	#else
	if (username == NULL || pstSectionName == NULL)
	#endif
        return False;


	DEBUG(3,("[F003]%s access for reactivated (after 60 min timeout expired)\n", username));
    /* arcadyan, add event log, F001, for DT-724 */
    asprintf(&pstLogCmd, "umng_syslog_cli addEventCode -1 F103 %s", username);
    if (system(pstLogCmd) != 0)
        DEBUG(0,( "falied to add Event Log, F103\n"));
    else
        DEBUG(0,( "successful to add Event Log, F103\n"));
    free(pstLogCmd);

	#ifdef ARCADYAN_STASTISTIC_CFG
	/* set value of <username>_blockEnable@USB_DEVICE to be '0' */
	smb_statistic_cfg_set_int(username, "blockEnable", 0);
	#else
	/* set blockEnable@smb_<username> to be '0' */
    mapi_ccfg_set_str(tid, pstSectionName, "blockEnable", "0");
	#endif

	#ifdef ARCADYAN_STASTISTIC_CFG
    /* set value of <username>_blockInitTime@USB_DEVICE to be '0' */
	smb_statistic_cfg_set_int(username, "blockInitTime", 0);
	#else
	/* set blockInitTime@smb_<username> to be '0' */
    mapi_ccfg_set_str(tid, pstSectionName, "blockInitTime", "0");
	#endif

	return True;
}

/************************************************************************************
 ** set event log, F104, to log system of DT724
 ** Description: User with IP-address successfully logged in with protocol <protocol>
 ***********************************************************************************/
static BOOL smb_set_event_log_F104(const char *username)
{
	int				ret						= True;
	char 			*pstLogCmd 				= NULL;
    char            szBlockInitTimeBuf[64]   = {'\0'};
	char         	szBlockEnableBuf[8]      = {'\0'};
	struct timeval	tv;
	char			*pstSectionName			= NULL;	/* for section of mapi, smb_<username> */	
	int 			tid;							/* for API of mapi, mapi_ccfg_xxx() */
	int             nRecBlockEnable         = -1;	/* for statistic, smb_<username>_blockEnable@USB_DEVICE */
	int             nRecBlockInitTime       = -1;   /* for statistic, smb_<username>_blockInitTime@USB_DEVICE */

	if (username == NULL)
		return False;

	/* initialize transaction */	
	#ifndef ARCADYAN_STASTISTIC_CFG
    if ((tid = mapi_start_transc()) == MID_FAIL)
    {
        DEBUG(0,( "mapi_start_transc() failed\n"));
        ret = False;
		goto END;
    }
	asprintf(&pstSectionName, "smb_%s", username);
    #endif
	
	#ifdef ARCADYAN_STASTISTIC_CFG
	/* get value of <username>_blockEnable@USB_DEVICE */
	smb_statistic_cfg_get_int(username, "blockEnable", &nRecBlockEnable);
	#else
	/* get value of blockEnable@smb_<username> */
    mapi_ccfg_get_str(tid, pstSectionName, "blockEnable", szBlockEnableBuf, sizeof(szBlockEnableBuf), "empty");
	#endif

    /* check if username has been blocked */
	#ifdef ARCADYAN_STASTISTIC_CFG
    if (nRecBlockEnable == 1)
    #else
    if (szBlockEnableBuf[0] == '1')
	#endif
    {
        DEBUG(3,( "%s has been blocked and check whether block time has been exceeded 60 minutes\n", username));
		
		gettimeofday(&tv, NULL);

		#ifdef ARCADYAN_STASTISTIC_CFG		
		/* get value of <username>_blockInitTime@USB_DEVICE */
		smb_statistic_cfg_get_int(username, "blockInitTime", &nRecBlockInitTime);
		#else
		/* get value of blockInitTime@smb_<username> */
    	mapi_ccfg_get_str(tid, pstSectionName, "blockInitTime", szBlockInitTimeBuf, sizeof(szBlockInitTimeBuf), "0");
    	DEBUG(10, ( "szBlockInitTimeBuf = %s\n", szBlockInitTimeBuf));
		#endif

		#ifdef ARCADYAN_STASTISTIC_CFG
		if (tv.tv_sec - nRecBlockInitTime > 3600)
		#else
		if (tv.tv_sec - atol(szBlockInitTimeBuf) > 3600)
		#endif
		{
			/* reactivate access limit of username */
			#ifdef ARCADYAN_STASTISTIC_CFG
			smb_set_event_log_F103(username);
			#else
			smb_set_event_log_F103(tid, pstSectionName, username);
			#endif
		}
		else
		{
			DEBUG(3,( "%s is still blocked because block time doesn't exceed 60 minutes\n", username));
			ret = False;
			goto END;
		}
    }

	DEBUG(3,("[F104]User \"%s\" with \"%s\" successfully logged in with protocol\n", username, client_addr()));
    /* arcadyan, add event log, F104, for DT-724 */
    asprintf(&pstLogCmd, "umng_syslog_cli addEventCode -3 F104 %s %s SMB", username, client_addr());
    if (system(pstLogCmd) != 0)
		DEBUG(0,( "falied to add Event Log, F104\n"));
    else
     	DEBUG(0,( "successful to add Event Log, F104\n")); 
	free(pstLogCmd);
	
	#ifdef ARCADYAN_STASTISTIC_CFG
    /* set value of <username>_retryCount@USB_DEVICE to be '0' */
    smb_statistic_cfg_set_int(username, "retryCount", 0);
	#else
	/* set retryCount@smb_<username> to be '0' */
	mapi_ccfg_set_str(tid, pstSectionName, "retryCount", "0");
	#endif

	/* set value back to be False */
	nShowLoginWindow = False;
	
	END:
		#ifndef ARCADYAN_STASTISTIC_CFG
		/* finish transaction */
    	mapi_end_transc(tid);
		/* Don't ferget to free */
        free(pstSectionName);
		#endif
		
		return ret;
}

/***********************************************************************************************************
 ** set event log, F001, to log system of DT724
 ** Description: Local user login failed after >5 unsuccessful attempts (account 'x' blocked for 60 minutes)
 **********************************************************************************************************/
#ifdef ARCADYAN_STASTISTIC_CFG
static BOOL smb_set_event_log_F001(const char *username, const struct timeval *tv)
#else
static BOOL smb_set_event_log_F001(const int tid, const char *pstSectionName, const char *username, const struct timeval *tv)
#endif
{
	char                    *pstLogCmd      	= NULL;
	char					*pstBlockInitTime	= NULL;	/* for node of mapi, blockInitTime@smb_<username> */

	if (username == NULL || tv == NULL)
        return False;
	
	DEBUG(3,("[F001]Local user login failed after > 5 unsuccessful attempts (account '%s'blocked for 60 minutes)\n", username));
    /* arcadyan, add event log, F001, for DT-724 */
    asprintf(&pstLogCmd, "umng_syslog_cli addErrorCode -1 F001 %s", username);
    if (system(pstLogCmd) != 0)
        DEBUG(0,( "falied to add Error Log, F001\n"));
    else
        DEBUG(0,( "successful to add Error Log, F001\n"));
    free(pstLogCmd);

	#ifdef ARCADYAN_STASTISTIC_CFG
	/* set value of <username>_blockEnable@USB_DEVICE to be '1' */
    smb_statistic_cfg_set_int(username, "blockEnable", 1);
	#else
	/* set blockEnable@smb_<username> to be '1' */
	mapi_ccfg_set_str(tid, pstSectionName, "blockEnable", "1");
	#endif

	#ifdef ARCADYAN_STASTISTIC_CFG
    /* set value of <username>_blockInitTime@USB_DEVICE to be tv->tv_sec */
    smb_statistic_cfg_set_int(username, "blockInitTime", tv->tv_sec);
	#else
	/* set blockInitTime@smb_<username> */	
	asprintf(&pstBlockInitTime, "%ld", tv->tv_sec);
	mapi_ccfg_set_str(tid, pstSectionName, "blockInitTime", pstBlockInitTime);
	free(pstBlockInitTime);
	#endif

	return True;
}

/****************************************************************************************************************************
 ** set event log, F003, to log system of DT724, 
 ** Description: User <username, IP-address> login failed with protocol
 ** Note: special case clicking icon of Speedport or typing command would trigger continuously login failed about 2 ~ 3 times
 ***************************************************************************************************************************/
static BOOL smb_set_event_log_F003(const char *username)
{
	char 					*pstLogCmd 					= NULL;
	char  					szRetryCountBuf[8] 			= {'\0'};
	char                    szBlockEnableBuf[8]        	= {'\0'};
	struct 	timeval			tv;
	static 	struct timeval  last_tv;
	float					nTotaltime;							/* unit: time */
	int						tid;								/* for API, mapi_ccfg_xxx() */
	char					*pstSectionName				= NULL; /* for section, for smb_<username> */
	char					*pstRetryCount				= NULL;	/* for node, retryCount@smb_<username> */
    int             		nRecBlockEnable         	= -1;   /* for statistic, <username>_blockEnable@USB_DEVICE */
	int						nRecRetryCount				= -1;	/* for statistic, <username>_retryCount@USB_DEVICE */
	char    				szMailAddressBuf[64]        = {'\0'};	/* subject of email */
    char    				szF003ContentsOfEmail[128]  = {'\0'};	/* contents of email */
	int     				id                          = 0;	/* used by email */

	DEBUG(0,("Alive\n"));
	if (username == NULL)
        return False;

	DEBUG(0,("Alive\n"));
	/* initialize transaction */
    if ((tid = mapi_start_transc()) == MID_FAIL)
    {
        DEBUG(0,( "mapi_start_transc() failed\n"));
        goto End;
    }
	asprintf(&pstSectionName, "smb_%s", username);

	DEBUG(0,("Alive\n"));

	/* calculate time interval between login request */
	gettimeofday(&tv, NULL);
	#ifdef ARCADYAN_STASTISTIC_CFG
	/* get value of <username>_lastSecTime@USB_DEVICE */
	/* get value of <username>_lastUsecTime@USB_DEVICE */
    smb_statistic_cfg_get_int(username, "lastSecTime", &last_tv.tv_sec);
	smb_statistic_cfg_get_int(username, "lastUsecTime", &last_tv.tv_usec);
	#else
	last_tv.tv_sec = mapi_ccfg_get_long(tid, pstSectionName, "lastSecTime", 0);
	last_tv.tv_usec = mapi_ccfg_get_long(tid, pstSectionName, "lastUsecTime", 0);
	#endif

	DEBUG(0,("Alive\n"));
	
	nTotaltime = (float)((tv.tv_sec - last_tv.tv_sec) * 1000000 + (tv.tv_usec - last_tv.tv_usec)) / 1000000;
	DEBUG(3,("tv.tv_sec = %d, last_tv.tv_sec = %d\n", tv.tv_sec, last_tv.tv_sec));	
	DEBUG(3,("totaltime = %f\n", nTotaltime));	

	#ifdef ARCADYAN_STASTISTIC_CFG
    /* get value of <username>_blockEnable@USB_DEVICE */
    smb_statistic_cfg_get_int(username, "blockEnable", &nRecBlockEnable);
    #else	
	/* get value of blockEnable@smb_<username> */
	mapi_ccfg_get_str(tid, pstSectionName, "blockEnable", szBlockEnableBuf, sizeof(szBlockEnableBuf), "empty");
	DEBUG(10, ( "szBlockEnableBuf = %s\n", szBlockEnableBuf));
	#endif

	DEBUG(0,("Alive\n"));

	/* check if username has been blocked */
	#ifdef ARCADYAN_STASTISTIC_CFG
    if (nRecBlockEnable == 1)
    #else
	if (szBlockEnableBuf[0] == '1')
	#endif
	{
		DEBUG(0,( "%s has been blocked without doing anything\n", username));
		goto End;
	}

	DEBUG(0,("Alive\n"));

	#ifdef ARCADYAN_STASTISTIC_CFG
    /* get value of <username>_retryCount@USB_DEVICE */
    smb_statistic_cfg_get_int(username, "retryCount", &nRecRetryCount);
	#else
	/* get value of retryCount@smb_<username> */
	mapi_ccfg_get_str(tid, pstSectionName, "retryCount", szRetryCountBuf, sizeof(szRetryCountBuf), "empty");
	DEBUG(10, ( "szRetryCountBuf = %s\n", szRetryCountBuf));
	#endif

	DEBUG(0,("Alive\n"));

	/* Retry count of 5 user login attempts has exhausted */
	#ifdef ARCADYAN_STASTISTIC_CFG
    if (nRecRetryCount == 5 && (nTotaltime >= 1.2))
    #else
	if (szRetryCountBuf[0] == '5' && (nTotaltime >= 1.2))	
	#endif
	{
		#ifdef ARCADYAN_STASTISTIC_CFG
		smb_set_event_log_F001(username, &tv);
		#else
		smb_set_event_log_F001(tid, pstSectionName, username, &tv);
		#endif
		goto End;
	}

	DEBUG(0,("Alive\n"));

	/* Normal situation, one is clicking icon or typing command to trigger login fialed at 1st time, the other is user types wrong password */
	if(nTotaltime >= 1.2 || nTotaltime < 0)
	{
		DEBUG(0,("Alive\n"));
		DEBUG(3,("[F003]User \"%s\", \"%s\" login failed with protocol as a result of \"Invalid password\"\n", username, client_addr()));
    	/* arcadyan, add event log, F003, for DT-724 */
    	asprintf(&pstLogCmd, "umng_syslog_cli addErrorCode -3 F003 %s %s SMB", username, client_addr());
    	if (system(pstLogCmd) != 0)
    		DEBUG(0,( "falied to add Error Log, F003\n"));
    	else
       		DEBUG(0,( "successful to add Error Log, F003\n"));    
		free(pstLogCmd);	

		/* do action, "send email" */
		/* get value of email_sendto_address@nas */
    	mapi_ccfg_get_str(tid, "nas", "email_sendto_address", szMailAddressBuf, sizeof(szMailAddressBuf), "empty");
    	DEBUG(10, ( "szMailAddressBuf = %s\n", szMailAddressBuf));

		if(strcmp(szMailAddressBuf, "empty") != 0)
		{
			/* fill contents of email */
			snprintf(szF003ContentsOfEmail, 128, "Anmeldung des Benutzers %s, %s uber SMB nicht erfolgreich.", username, client_addr());
        	DEBUG(10, ("szF003ContentsOfEmail is %s\n", szF003ContentsOfEmail));
	
			/* send email */
			utilSendMailByProfileID(id, szMailAddressBuf, SMB_W724_MAIL_SUBJECT, szF003ContentsOfEmail);
		}

		#ifdef ARCADYAN_STASTISTIC_CFG
		if (nRecRetryCount == -1 || nRecRetryCount == 0)			/* set value of <username>_retryCount@USB_DEVICE to be '1' */
    		smb_statistic_cfg_set_int(username, "retryCount", 1);	
		else														/* increase value of <username>_retryCount@USB_DEVICE */
			smb_statistic_cfg_set_int(username, "retryCount", nRecRetryCount + 1);
		#else
		if (strcmp(szRetryCountBuf, "empty") == 0 || szRetryCountBuf[0] == '0') /* set value of retryCount@smb_<username> in case, no its node or its value is '0' */
		{
			mapi_ccfg_set_str(tid, pstSectionName, "retryCount", "1");
		}
		else /* set value of retryCount@smb_<username> while vaule of node equal to or greater than '1' */
		{
			asprintf(&pstRetryCount, "%d", atoi(szRetryCountBuf) + 1);
			mapi_ccfg_set_str(tid, pstSectionName, "retryCount", pstRetryCount);
			free(pstRetryCount);
		}
		#endif

		/* Value of nShowLoginWindow to be True would trigger to decrease value of retryCount@smb_<username> once if in "else" case */ 
		nShowLoginWindow = True;
		DEBUG(0,("Alive\n"));
	}
	/* this situation should be only running while clicking icon of Speedport of DT724 or typing "\\<ip_address of server" to trgger login failed after 2nd time */
	else
	{
		DEBUG(0,("Alive\n"));
		DEBUG(5,("Continuously trying to login but failed while clicking icon and typing command which triggers login request twice\n"));		

		#ifdef ARCADYAN_STASTISTIC_CFG
        if (nShowLoginWindow == True && (nRecRetryCount != -1 || nRecRetryCount != 0))    	/* decrease value of <username>_retryCount@USB_DEVICE */
		{
            smb_statistic_cfg_set_int(username, "retryCount", nRecRetryCount - 1);
			nShowLoginWindow = False;
		}
		#else
		if (nShowLoginWindow == True && (strcmp(szRetryCountBuf, "empty") != 0 || szRetryCountBuf[0] != '0'))
		{
			asprintf(&pstRetryCount, "%d", atoi(szRetryCountBuf) - 1);
            mapi_ccfg_set_str(tid, pstSectionName, "retryCount", pstRetryCount);
            free(pstRetryCount);

			nShowLoginWindow = False;
		}	
		#endif
		DEBUG(0,("Alive\n"));
	}
	DEBUG(0,("Alive\n"));

	End:
		#ifdef ARCADYAN_STASTISTIC_CFG
		/* set value of <username>_lastSecTime@USB_DEVICE */
		/* set value of <username>_lastUsecTime@USB_DEVICE */
    	smb_statistic_cfg_set_int(username, "lastSecTime", tv.tv_sec);
		smb_statistic_cfg_set_int(username, "lastUsecTime", tv.tv_usec);
		#else
		mapi_ccfg_set_long(tid, pstSectionName, "lastSecTime", tv.tv_sec);
    	mapi_ccfg_set_long(tid, pstSectionName, "lastUsecTime", tv.tv_usec);
		#endif
		DEBUG(0,("Alive\n"));

		/* finish transaction */
        mapi_end_transc(tid);
        /* Don't ferget to free */
        free(pstSectionName);
		DEBUG(0,("Alive\n"));
		return True;
}
#endif

/****************************************************************************
 Core of smb password checking routine.
****************************************************************************/

static BOOL smb_pwd_check_ntlmv1(const DATA_BLOB *nt_response,
				 const uchar *part_passwd,
				 const DATA_BLOB *sec_blob,
				 DATA_BLOB *user_sess_key
				 #ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD /* arcadyan, new parameter, "username" */
				 , const char *username  
				 #endif
				 )
{
	/* Finish the encryption of part_passwd. */
	uchar p24[24];
	
	if (part_passwd == NULL) {
		DEBUG(10,("No password set - DISALLOWING access\n"));
		/* No password set - always false ! */
		return False;
	}
	
	if (sec_blob->length != 8) {
		DEBUG(0, ("smb_pwd_check_ntlmv1: incorrect challenge size (%lu)\n", 
			  (unsigned long)sec_blob->length));
		return False;
	}
	
	if (nt_response->length != 24) {
		DEBUG(0, ("smb_pwd_check_ntlmv1: incorrect password length (%lu)\n", 
			  (unsigned long)nt_response->length));
		return False;
	}

	SMBOWFencrypt(part_passwd, sec_blob->data, p24);
	if (user_sess_key != NULL) {
		*user_sess_key = data_blob(NULL, 16);
		SMBsesskeygen_ntv1(part_passwd, NULL, user_sess_key->data);
	}
	
#ifdef DEBUG_PASSWORD	/* arcadyan, change log level from 100 to 3 */
	#ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD
	DEBUG(3,("check password for user, \"%s\"\n", username));
	#endif
	DEBUG(3,("Part password (P16) was |\n"));
	dump_data(3, (const char *)part_passwd, 16);
	DEBUGADD(3,("Password from client was |\n"));
	dump_data(3, (const char *)nt_response->data, nt_response->length);
	DEBUGADD(3,("Given challenge was |\n"));
	dump_data(3, (const char *)sec_blob->data, sec_blob->length);
	DEBUGADD(3,("Value from encryption was |\n"));
	dump_data(3, (const char *)p24, 24);
#endif

	#ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD /* arcadyan, for gast, function, "memcmp", returns 0 which means NULL password of gast */
	if (strcmp(username, "GUEST") == 0)
		return (!(memcmp(p24, nt_response->data, 24) == 0));
	else
	#endif
	return (memcmp(p24, nt_response->data, 24) == 0);
	
}

/****************************************************************************
 Core of smb password checking routine. (NTLMv2, LMv2)
 Note:  The same code works with both NTLMv2 and LMv2.
****************************************************************************/

static BOOL smb_pwd_check_ntlmv2(const DATA_BLOB *ntv2_response,
				 const uchar *part_passwd,
				 const DATA_BLOB *sec_blob,
				 const char *user, const char *domain,
				 BOOL upper_case_domain, /* should the domain be transformed into upper case? */
				 DATA_BLOB *user_sess_key
				 #ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD /* arcadyan, new parameter, "username" */
                 , const char *username
                 #endif
				 )
{
	/* Finish the encryption of part_passwd. */
	uchar kr[16];
	uchar value_from_encryption[16];
	uchar client_response[16];
	DATA_BLOB client_key_data;
	BOOL res;

	if (part_passwd == NULL) {
		DEBUG(10,("No password set - DISALLOWING access\n"));
		/* No password set - always False */
		return False;
	}

	if (sec_blob->length != 8) {
		DEBUG(0, ("smb_pwd_check_ntlmv2: incorrect challenge size (%lu)\n", 
			  (unsigned long)sec_blob->length));
		return False;
	}
	
	if (ntv2_response->length < 24) {
		/* We MUST have more than 16 bytes, or the stuff below will go
		   crazy.  No known implementation sends less than the 24 bytes
		   for LMv2, let alone NTLMv2. */
		DEBUG(0, ("smb_pwd_check_ntlmv2: incorrect password length (%lu)\n", 
			  (unsigned long)ntv2_response->length));
		return False;
	}

	client_key_data = data_blob(ntv2_response->data+16, ntv2_response->length-16);
	/* 
	   todo:  should we be checking this for anything?  We can't for LMv2, 
	   but for NTLMv2 it is meant to contain the current time etc.
	*/

	memcpy(client_response, ntv2_response->data, sizeof(client_response));

	if (!ntv2_owf_gen(part_passwd, user, domain, upper_case_domain, kr)) {
		return False;
	}

	SMBOWFencrypt_ntv2(kr, sec_blob, &client_key_data, value_from_encryption);
	if (user_sess_key != NULL) {
		*user_sess_key = data_blob(NULL, 16);
		SMBsesskeygen_ntv2(kr, value_from_encryption, user_sess_key->data);
	}

#if DEBUG_PASSWORD	/* arcadyan, change log level from 100 to 3 */
	#ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD
    DEBUG(3,("check password for user, \"%s\"\n", username));
    #endif
	DEBUG(3,("Part password (P16) was |\n"));
	dump_data(3, (const char *)part_passwd, 16);
	DEBUGADD(3,("Password from client was |\n"));
	dump_data(3, (const char *)ntv2_response->data, ntv2_response->length);
	DEBUGADD(3,("Variable data from client was |\n"));
	dump_data(3, (const char *)client_key_data.data, client_key_data.length);
	DEBUGADD(3,("Given challenge was |\n"));
	dump_data(3, (const char *)sec_blob->data, sec_blob->length);
	DEBUGADD(3,("Value from encryption was |\n"));
	dump_data(3, (const char *)value_from_encryption, 16);
#endif
	data_blob_clear_free(&client_key_data);
	#ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD /* arcadyan, for gast, function, "memcmp", returns 0 which means NULL password of gast */
    if (strcmp(username, "GUEST") == 0)
	{
		if (*domain == '\x0')	/* special case for gast account while domain is '\x0' */
			res = (memcmp(value_from_encryption, client_response, 16) == 0);
		else
        	res = (!(memcmp(value_from_encryption, client_response, 16) == 0));
    }
	else
	#endif
		res = (memcmp(value_from_encryption, client_response, 16) == 0);
	if ((!res) && (user_sess_key != NULL))
		data_blob_clear_free(user_sess_key);
	return res;
}

/**
 * Check a challenge-response password against the value of the NT or
 * LM password hash.
 *
 * @param mem_ctx talloc context
 * @param challenge 8-byte challenge.  If all zero, forces plaintext comparison
 * @param nt_response 'unicode' NT response to the challenge, or unicode password
 * @param lm_response ASCII or LANMAN response to the challenge, or password in DOS code page
 * @param username internal Samba username, for log messages
 * @param client_username username the client used
 * @param client_domain domain name the client used (may be mapped)
 * @param nt_pw MD4 unicode password from our passdb or similar
 * @param lm_pw LANMAN ASCII password from our passdb or similar
 * @param user_sess_key User session key
 * @param lm_sess_key LM session key (first 8 bytes of the LM hash)
 */

NTSTATUS ntlm_password_check(TALLOC_CTX *mem_ctx,
			     const DATA_BLOB *challenge,
			     const DATA_BLOB *lm_response,
			     const DATA_BLOB *nt_response,
			     const DATA_BLOB *lm_interactive_pwd,
			     const DATA_BLOB *nt_interactive_pwd,
			     const char *username, 
			     const char *client_username, 
			     const char *client_domain,
			     const uint8 *lm_pw, const uint8 *nt_pw, 
			     DATA_BLOB *user_sess_key, 
			     DATA_BLOB *lm_sess_key)
{
	static const unsigned char zeros[8] = { 0, };

	if (nt_pw == NULL) {
		DEBUG(3,("ntlm_password_check: NO NT password stored for user %s.\n", 
			 username));
	}

	if (nt_interactive_pwd && nt_interactive_pwd->length && nt_pw) { 
		if (nt_interactive_pwd->length != 16) {
			DEBUG(3,("ntlm_password_check: Interactive logon: Invalid NT password length (%d) supplied for user %s\n", (int)nt_interactive_pwd->length,
				 username));
			return NT_STATUS_WRONG_PASSWORD;
		}

		if (memcmp(nt_interactive_pwd->data, nt_pw, 16) == 0) {
			if (user_sess_key) {
				*user_sess_key = data_blob(NULL, 16);
				SMBsesskeygen_ntv1(nt_pw, NULL, user_sess_key->data);
			}
			return NT_STATUS_OK;
		} else {
			DEBUG(3,("ntlm_password_check: Interactive logon: NT password check failed for user %s\n",
				 username));
			return NT_STATUS_WRONG_PASSWORD;
		}

	} else if (lm_interactive_pwd && lm_interactive_pwd->length && lm_pw) { 
		if (lm_interactive_pwd->length != 16) {
			DEBUG(3,("ntlm_password_check: Interactive logon: Invalid LANMAN password length (%d) supplied for user %s\n", (int)lm_interactive_pwd->length,
				 username));
			return NT_STATUS_WRONG_PASSWORD;
		}

		if (!lp_lanman_auth()) {
			DEBUG(3,("ntlm_password_check: Interactive logon: only LANMAN password supplied for user %s, and LM passwords are disabled!\n",
				 username));
			return NT_STATUS_WRONG_PASSWORD;
		}

		if (memcmp(lm_interactive_pwd->data, lm_pw, 16) == 0) {
			return NT_STATUS_OK;
		} else {
			DEBUG(3,("ntlm_password_check: Interactive logon: LANMAN password check failed for user %s\n",
				 username));
			return NT_STATUS_WRONG_PASSWORD;
		}
	}

	/* Check for cleartext netlogon. Used by Exchange 5.5. */
	if (challenge->length == sizeof(zeros) && 
	    (memcmp(challenge->data, zeros, challenge->length) == 0 )) {

		DEBUG(4,("ntlm_password_check: checking plaintext passwords for user %s\n",
			 username));
		if (nt_pw && nt_response->length) {
			unsigned char pwhash[16];
			mdfour(pwhash, nt_response->data, nt_response->length);
			if (memcmp(pwhash, nt_pw, sizeof(pwhash)) == 0) {
				return NT_STATUS_OK;
			} else {
				DEBUG(3,("ntlm_password_check: NT (Unicode) plaintext password check failed for user %s\n",
					 username));
				return NT_STATUS_WRONG_PASSWORD;
			}

		} else if (!lp_lanman_auth()) {
			DEBUG(3,("ntlm_password_check: (plaintext password check) LANMAN passwords NOT PERMITTED for user %s\n",
				 username));

		} else if (lm_pw && lm_response->length) {
			uchar dospwd[14]; 
			uchar p16[16]; 
			ZERO_STRUCT(dospwd);
			
			memcpy(dospwd, lm_response->data, MIN(lm_response->length, sizeof(dospwd)));
			/* Only the fisrt 14 chars are considered, password need not be null terminated. */

			/* we *might* need to upper-case the string here */
			E_P16((const unsigned char *)dospwd, p16);

			if (memcmp(p16, lm_pw, sizeof(p16)) == 0) {
				return NT_STATUS_OK;
			} else {
				DEBUG(3,("ntlm_password_check: LANMAN (ASCII) plaintext password check failed for user %s\n",
					 username));
				return NT_STATUS_WRONG_PASSWORD;
			}
		} else {
			DEBUG(3, ("Plaintext authentication for user %s attempted, but neither NT nor LM passwords available\n", username));
			return NT_STATUS_WRONG_PASSWORD;
		}
	}

	if (nt_response->length != 0 && nt_response->length < 24) {
		DEBUG(2,("ntlm_password_check: invalid NT password length (%lu) for user %s\n", 
			 (unsigned long)nt_response->length, username));		
	}
	
	if (nt_response->length >= 24 && nt_pw) {
		if (nt_response->length > 24) {
			/* We have the NT MD4 hash challenge available - see if we can
			   use it 
			*/
			DEBUG(4,("ntlm_password_check: Checking NTLMv2 password with domain [%s]\n", client_domain));
			if (smb_pwd_check_ntlmv2( nt_response, 
						  nt_pw, challenge, 
						  client_username, 
						  client_domain,
						  False,
						  user_sess_key
						  #ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD /* arcadyan, new parameter, "username" */
                          , username
                          #endif
						  )) {
				#ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD /* arcadyan, debug message for normal user and gast */
                if (strcmp(username, "GUEST") == 0)
                    DEBUG(3,("ntlm_password_check: NTLMv2 password check failed for user %s, but to be accepted except NULL password\n",
                     username));
                else
                    DEBUG(3,("ntlm_password_check: NTLMv2 password check successful for user %s\n",
                     username));
                #endif

				#ifdef ARCADYAN_DT_724_SAMBA_EVENT_LOG
				if (smb_set_event_log_F104(username) == False) /* username is still blocked */	
					return NT_STATUS_WRONG_PASSWORD;
				#endif

				return NT_STATUS_OK;
			}
			
			DEBUG(4,("ntlm_password_check: Checking NTLMv2 password with uppercased version of domain [%s]\n", client_domain));
			if (smb_pwd_check_ntlmv2( nt_response, 
						  nt_pw, challenge, 
						  client_username, 
						  client_domain,
						  True,
						  user_sess_key
						  #ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD /* arcadyan, new parameter, "username" */
                          , username
                          #endif
						  )) {
				#ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD /* arcadyan, debug message for normal user and gast */
                if (strcmp(username, "GUEST") == 0)
                    DEBUG(3,("ntlm_password_check: NTLMv2 password check failed for user %s, but to be accepted except NULL password\n",
                     username));
                else
                    DEBUG(3,("ntlm_password_check: NTLMv2 password check successful for user %s\n",
                     username));
                #endif

				#ifdef ARCADYAN_DT_724_SAMBA_EVENT_LOG
                if (smb_set_event_log_F104(username) == False) /* username is still blocked */
                    return NT_STATUS_WRONG_PASSWORD;
                #endif

				return NT_STATUS_OK;
			}
			
			DEBUG(4,("ntlm_password_check: Checking NTLMv2 password without a domain\n"));
			if (smb_pwd_check_ntlmv2( nt_response, 
						  nt_pw, challenge, 
						  client_username, 
						  "",
						  False,
						  user_sess_key
						  #ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD /* arcadyan, new parameter, "username" */
                          , username
                          #endif
						  )) {
				#ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD /* arcadyan, debug message for normal user and gast */
                if (strcmp(username, "GUEST") == 0)
                    DEBUG(3,("ntlm_password_check: NTLMv2 password check failed for user %s, but to be accepted except NULL password\n",
                     username));
                else
                    DEBUG(3,("ntlm_password_check: NTLMv2 password check successful for user %s\n",
                     username));
                #endif

				#ifdef ARCADYAN_DT_724_SAMBA_EVENT_LOG
                if (smb_set_event_log_F104(username) == False) /* username is still blocked */
                    return NT_STATUS_WRONG_PASSWORD;
                #endif

				return NT_STATUS_OK;
			} else {
				#ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD /* arcadyan, debug message for gast account */
                if (strcmp(username, "GUEST") == 0)
                    DEBUG(3,("ntlm_password_check: NTLMv2 password check successful for user %s, but to be rejected as a result of NULL password\n",
                     username));
                else
					DEBUG(3,("ntlm_password_check: NTLMv2 password check failed for %s\n", username));
                #endif
				DEBUG(3,("ntlm_password_check: NTLMv2 password check failed\n"));
				
				#ifdef ARCADYAN_DT_724_SAMBA_EVENT_LOG
				smb_set_event_log_F003(username);
				#endif

				return NT_STATUS_WRONG_PASSWORD;
			}
		}

		if (lp_ntlm_auth()) {		
			/* We have the NT MD4 hash challenge available - see if we can
			   use it (ie. does it exist in the smbpasswd file).
			*/
			DEBUG(4,("ntlm_password_check: Checking NT MD4 password\n"));
			if (smb_pwd_check_ntlmv1(nt_response, 
						 nt_pw, challenge,
						 user_sess_key
						 #ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD /* arcadyan, new parameter, "username" */
                 		 , username
                 		 #endif
						 )) {
				/* The LM session key for this response is not very secure, 
				   so use it only if we otherwise allow LM authentication */

				if (lp_lanman_auth() && lm_pw) {
					uint8 first_8_lm_hash[16];
					memcpy(first_8_lm_hash, lm_pw, 8);
					memset(first_8_lm_hash + 8, '\0', 8);
					if (lm_sess_key) {
						*lm_sess_key = data_blob(first_8_lm_hash, 16);
					}
				}

				#ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD /* arcadyan, debug message for normal user and gast */
				if (strcmp(username, "GUEST") == 0)				
					DEBUG(3,("ntlm_password_check: NT MD4 password check failed for user %s, but to be accepted except NULL password\n",
                     username));
				else
					DEBUG(3,("ntlm_password_check: NT MD4 password check successful for user %s\n",
                     username));
				#endif

				#ifdef ARCADYAN_DT_724_SAMBA_EVENT_LOG
                if (smb_set_event_log_F104(username) == False) /* username is still blocked */
                    return NT_STATUS_WRONG_PASSWORD;
                #endif

				return NT_STATUS_OK;
			} 
			#if 0/* arcadyan, not used now, it let gast pass in any password including NULL password */
			else if (strcmp(username, "gast") == 0) {
				return NT_STATUS_OK;				
			}
			#endif
			else {
				#ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD /* arcadyan, debug message for gast account */
				if (strcmp(username, "GUEST") == 0)
					DEBUG(3,("ntlm_password_check: NT MD4 password check successful for user %s, but to be rejected as a result of NULL password\n",
                     username));
				else
				#endif
				DEBUG(3,("ntlm_password_check: NT MD4 password check failed for user %s\n",
					 username));
				
				#ifdef ARCADYAN_DT_724_SAMBA_EVENT_LOG
                smb_set_event_log_F003(username);
                #endif

				return NT_STATUS_WRONG_PASSWORD;
			}
		} else {
			DEBUG(2,("ntlm_password_check: NTLMv1 passwords NOT PERMITTED for user %s\n",
				 username));			
			/* no return, becouse we might pick up LMv2 in the LM field */
		}
	}
	
	if (lm_response->length == 0) {
		DEBUG(3,("ntlm_password_check: NEITHER LanMan nor NT password supplied for user %s\n",
			 username));
		return NT_STATUS_WRONG_PASSWORD;
	}
	
	if (lm_response->length < 24) {
		DEBUG(2,("ntlm_password_check: invalid LanMan password length (%lu) for user %s\n", 
			 (unsigned long)nt_response->length, username));		
		return NT_STATUS_WRONG_PASSWORD;
	}
		
	if (!lp_lanman_auth()) {
		DEBUG(3,("ntlm_password_check: Lanman passwords NOT PERMITTED for user %s\n",
			 username));
	} else if (!lm_pw) {
		DEBUG(3,("ntlm_password_check: NO LanMan password set for user %s (and no NT password supplied)\n",
			 username));
	} else {
		DEBUG(4,("ntlm_password_check: Checking LM password\n"));
		if (smb_pwd_check_ntlmv1(lm_response, 
					 lm_pw, challenge,
					 NULL
					 #ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD /* arcadyan, new parameter, "username" */
                     , username
                     #endif
					 )) {
			uint8 first_8_lm_hash[16];
			memcpy(first_8_lm_hash, lm_pw, 8);
			memset(first_8_lm_hash + 8, '\0', 8);
			if (user_sess_key) {
				*user_sess_key = data_blob(first_8_lm_hash, 16);
			}

			if (lm_sess_key) {
				*lm_sess_key = data_blob(first_8_lm_hash, 16);
			}
			return NT_STATUS_OK;
		}
	}
	
	if (!nt_pw) {
		DEBUG(4,("ntlm_password_check: LM password check failed for user, no NT password %s\n",username));
		return NT_STATUS_WRONG_PASSWORD;
	}
	
	/* This is for 'LMv2' authentication.  almost NTLMv2 but limited to 24 bytes.
	   - related to Win9X, legacy NAS pass-though authentication
	*/
	DEBUG(4,("ntlm_password_check: Checking LMv2 password with domain %s\n", client_domain));
	if (smb_pwd_check_ntlmv2( lm_response, 
				  nt_pw, challenge, 
				  client_username,
				  client_domain,
				  False,
				  NULL
				  #ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD /* arcadyan, new parameter, "username" */
                  , username
                  #endif
				  )) {
		return NT_STATUS_OK;
	}
	
	DEBUG(4,("ntlm_password_check: Checking LMv2 password with upper-cased version of domain %s\n", client_domain));
	if (smb_pwd_check_ntlmv2( lm_response, 
				  nt_pw, challenge, 
				  client_username,
				  client_domain,
				  True,
				  NULL
				  #ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD /* arcadyan, new parameter, "username" */
                  , username
                  #endif
				  )) {
		return NT_STATUS_OK;
	}
	
	DEBUG(4,("ntlm_password_check: Checking LMv2 password without a domain\n"));
	if (smb_pwd_check_ntlmv2( lm_response, 
				  nt_pw, challenge, 
				  client_username,
				  "",
				  False,
				  NULL
				  #ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD /* arcadyan, new parameter, "username" */
                  , username
                  #endif
				  )) {
		return NT_STATUS_OK;
	}

	/* Apparently NT accepts NT responses in the LM field
	   - I think this is related to Win9X pass-though authentication
	*/
	DEBUG(4,("ntlm_password_check: Checking NT MD4 password in LM field\n"));
	if (lp_ntlm_auth()) {
		if (smb_pwd_check_ntlmv1(lm_response, 
					 nt_pw, challenge,
					 NULL
					 #ifdef ARCADYAN_GUEST_PASSWORD_ACCEPT_EXCEPT_NULL_PASSWORD /* arcadyan, new parameter, "username" */
                     , username
                     #endif
					 )) {
			/* The session key for this response is still very odd.  
			   It not very secure, so use it only if we otherwise 
			   allow LM authentication */

			if (lp_lanman_auth() && lm_pw) {
				uint8 first_8_lm_hash[16];
				memcpy(first_8_lm_hash, lm_pw, 8);
				memset(first_8_lm_hash + 8, '\0', 8);
				if (user_sess_key) {
					*user_sess_key = data_blob(first_8_lm_hash, 16);
				}

				if (lm_sess_key) {
					*lm_sess_key = data_blob(first_8_lm_hash, 16);
				}
			}
			return NT_STATUS_OK;
		}
		DEBUG(3,("ntlm_password_check: LM password, NT MD4 password in LM field and LMv2 failed for user %s\n",username));
	} else {
		DEBUG(3,("ntlm_password_check: LM password and LMv2 failed for user %s, and NT MD4 password in LM field not permitted\n",username));
	}
	return NT_STATUS_WRONG_PASSWORD;
}

