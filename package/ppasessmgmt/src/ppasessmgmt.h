#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <ctype.h>

#define PPASESSMGMT_SOCKET "/tmp/ppasessmgmt_socket"
#define PPA_DEVICE   "/dev/ifx_ppa"

//#define DBG_PRINT 1

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

typedef struct mgmt_Args {
	unsigned int interval:12; // Polling interval in seconds Default = 60
	unsigned int priority:4; // Priority threshold. Sessions above this priority are never removed from Hardware acceleration default = 0
	unsigned int byteratebased:1; // Bytes rate based management default = 1
	unsigned int packetratebased:1; // Rate based Management. Default = 0
	unsigned int history:4; // Poll history: Number of polling history to look backward before decision default = 1
	unsigned int maxdelsession:8; // Maximum number of session deleted in one poll interval. Default 10
	unsigned int mode:1; // Aggressive mode = 1 or normal mode = 0. Default normal = 0
	unsigned int status:1; // to return the status of sessession management daemon
	unsigned int f_inte:1; //polling interval flag
	unsigned int f_prio:1; // Priority threshold flag
	unsigned int f_brate:1; //  Bytes rate based flag
	unsigned int f_prate:1; //packet rate flag
	unsigned int f_hist:1; //Poll history flag
	unsigned int f_maxd:1; //maxdelsession flag
	unsigned int f_mode:1; //mode flag
	unsigned int f_stat:1; //status flag
} mgmt_args;
