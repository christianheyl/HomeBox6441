//303002:JackLee 2005/06/16 Fix_webUI_firmware_upgrading_causes_webpage_error_issue 

/* ============================================================================
 * Copyright (C) 2005 -Infineon Technologies AG.
 *
 * All rights reserved.
 * ============================================================================
 *
 *============================================================================
 *
 * This document contains proprietary information belonging to Infineon 
 * Technologies AG. Passing on and copying of this document, and communication
 * of its contents is not permitted without prior written authorisation.
 * 
 * ============================================================================
 */

/* ===========================================================================
 *
 * File Name:   upgrade.c
 * Author :     Nirav Salot
 * Date: 	14th January, 2005
 *
 * ===========================================================================
 *
 * Project: Amazon
 *
 * ===========================================================================
 * Contents: This implements the image upgrade facility for Amazon devices
 *  
 * ===========================================================================
 * References: 
 *
 */

/* ===========================================================================
 * Revision History:
 *
 * $Log$
 * ===========================================================================
 */

#include "common.h"
#include <sys/ioctl.h>

#ifdef IFX_MULTILIB_UTIL 
#define main	upgrade_main
#endif

// LXDB26 doesn't support following information for retriving toolchain information.
//#include <toolchain/version.h>

#include "cmd_upgrade.h"
#include "command.h"
#include <sys/stat.h>
#include <signal.h>
#include <sys/reboot.h>

env_t env;

int main(int argc,char *argv[])
{
	int file_fd;
	char *fileData = NULL;
	struct stat filestat;
	int bRead = 0;
	char sCommand[32];
	int bSaveEnvCopy = 0;
	int ret = 0;
		
	if(argc < 5 || argc > 6)
	{
		printf("Usage : upgrade file_name image_type expand_direction saveenv_copy [reboot]\n");
//		printf("ToolChain:" TOOL_CHAIN_VERSION "\n");
		return 1;
	}

	if(argc == 6 && (strcmp(argv[5],"reboot") == 0))
	{
		printf("upgrade : reboot option is found and so killing all the processes\n");
		sleep(2);
		signal(SIGTERM,SIG_IGN);
		signal(SIGHUP,SIG_IGN);	
		setpgrp();
		kill(-1,SIGTERM);
		sleep(5);
	}

	if(read_env())
	{
		printf("read_env fail\n");
		ret = 1;
		goto abort;
	}

	if(strtol(argv[4],NULL,10) == 1)
		bSaveEnvCopy = 1;

	file_fd = open(argv[1],O_RDONLY);
	if(file_fd < 0)
	{	
		printf("The file %s could not be opened\n",argv[1]);
		ret = 1;
		goto abort;
	}

	fstat (file_fd,&filestat);
	if (!strcmp(argv[2], "kernel") || !strcmp(argv[2], "rootfs") || !strcmp(argv[2], "firmware") ) {
		ifx_debug_printf("upgrade : calling do_upgrade with srcAddr = 0x%08lx and size %d\n"
			, fileData, filestat.st_size);
		if(do_upgrade(file_fd, filestat.st_size)) {
			printf("Image %s could not be updated in dir=%s\n", argv[2], argv[3]);			
			ret = 1;
		} else {
			printf("Upgrade : successfully upgraded %s\n", argv[2]);
		}
                close(file_fd);
                printf("Erasing the input file %s\n",argv[1]);
                unlink(argv[1]);
	} else {
		fileData = (char *)malloc(filestat.st_size * sizeof(char));
		if(fileData == NULL)
		{
			printf("Can not allocate %d bytes for the buffer\n",filestat.st_size);
			ret = 1;
			goto abort;
		}
		bRead =	read(file_fd,fileData,filestat.st_size);
		if(bRead < filestat.st_size)
		{
			printf("Could read only read %d bytes out of %d bytes of the file\n",bRead,filestat.st_size);
			free(fileData);
			ret = 1;
			goto abort;
		}

		close(file_fd);
		printf("Erasing the input file %s\n",argv[1]);
		unlink(argv[1]);

		ifx_debug_printf("upgrade : calling upgrade_img with srcAddr = 0x%08lx and size %d\n"
			, fileData, filestat.st_size);
		if(upgrade_img((unsigned long)fileData,filestat.st_size,argv[2],strtol(argv[3],NULL,10),bSaveEnvCopy)) {
			printf("Image %s could not be updated in dir=%s\n", argv[2], argv[3]);
			ret = 1;
		} else {
			printf("Upgrade : successfully upgraded %s\n", argv[2]);
		}
		free(fileData);
	}
abort:
	if(argc == 6 && (strcmp(argv[5],"reboot") == 0))
	{
		printf("upgrade : Rebooting the system\n");
		reboot(RB_AUTOBOOT);
	}
	return ret;
}

