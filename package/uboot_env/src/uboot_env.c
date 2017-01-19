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
 * File Name:   uboot_env.c
 * Author :     Lin Mars
 * Date:	04-Dec-2008
 * ===========================================================================
 */

/* ===========================================================================
 * Revision History:
 *
 * $Log$
 * ===========================================================================
 */

#ifdef IFX_MULTILIB_UTIL 
#define main uboot_env_main
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "common.h"
#include "command.h"

env_t env;

static int usage(char *str)
{
	printf("To get or set environment variable in u-boot configuraton\n");
	printf("        %s --get options\n", str);
	printf("    available options:\n");
	printf("        --name parameter              parameter name to be got\n");
	printf("        %s --set options\n", str);
	printf("    available options:\n");
	printf("        --name parameter              parameter name to be set\n");
	printf("        --value value                 value to be assigned\n");
	return 0;
}

int main(int argc,char *argv[])
{
	int c;
	int option_index = 0;
	static int oper_get = 0, oper_set = 0;
	char *str_name = NULL, *str_value = NULL;
	//char *output = NULL;
	char output[1024] = {0x00};
	char *ep;
	static struct option long_options[] = {
		{"get", no_argument, &oper_get, 1}, 
		{"set", no_argument, &oper_set, 1}, 
		{"name", required_argument, 0, '1'},
		{"value", required_argument, 0, '2'},
		{NULL, 0, 0, 0}
	};

	if (argc < 4 || argc > 6) {
		usage(argv[0]);
		return -1;
	}

	while ((c = getopt_long(argc, argv, "1:2:", long_options, &option_index)) != EOF) {
		switch (c) {
		case 0:
			break;
		case '1':
			str_name = optarg;
			break;
		case '2':
			str_value = optarg;
			break;
		default:
			goto ERR_RET;
		}
	}

	if (oper_get == 1 && oper_set == 1)
		goto ERR_RET;
	if (oper_get == 0 && oper_set == 0)
		goto ERR_RET;
	if (str_name == NULL)
		goto ERR_RET;
	if (oper_set == 1 && str_value == NULL)
		goto ERR_RET;

	if (read_env() != 0) {
		fprintf( stderr, "Can not retrive u-boot configuration\n");
		return -1;
	}

	ep = get_env(str_name);
	if (ep == NULL) {
		fprintf( stderr, "parameter %s is not existed\n", str_name);
		return -1;
	}
	strcpy(output, ep);

	if (oper_get == 1) {
		printf("%s\n", output);
	} else {	// oper_set == 1
		if (set_env(str_name, str_value) != 0) {
			fprintf( stderr, "Can not set u-boot configuration (overflow?)\n" );
			return -1;
		}
		saveenv();
		printf("parameter %s value changed from %s to %s\n", str_name, output, str_value);
	}

	return 0;
ERR_RET:
	usage(argv[0]);
	return -1;
}

