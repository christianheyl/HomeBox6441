#include "ppasessmgmt.h"
#include <ifx_config.h>
//#include <ifx_common.h>
//#include "ifx_amazon_cfg.h"

#ifdef IFX_MULTILIB_UTIL
#define main	ppasessmgmt_main
#endif

static void ppa_session_mgmt_help_fn(char *prg)
{
	printf("Usage: %s [[-i <num>] [-t <num>] [-b <bool>] [-p <bool>] [-h <num>] [-n <num>] [-m <bool>]] [-s]\n",prg);
	printf(" -i Polling Interval  <0-4095> (default = [60])\n");
	printf(" -t Priority Threshold <1-8> (default = [7])\n");
	printf(" -b Byte Rate based Management (default = Enabled [1])\n");
	printf(" -p Packet Rate based Management (default = Disabled [0])\n");
	printf(" -h Poll history <0-15> (default = [1])\n");
	printf(" -n Maximum delete sessions <1-255> (default = [10])\n");
	printf(" -m Mode <Normal =0 Agressive =1> (default = Normal [0])\n");
	printf(" -s prints the current Status\n");
}

int ppa_session_mgmt_parse_args(mgmt_args *paramvals, int argc, char *argv[])
{
	int i, val=0;
	char c;
	if(!(argc%2)) {
		if(!strcmp(argv[1],"-s")) {
			paramvals->f_stat=1;
			paramvals->status=1;
			return 0;
		} else
			return 1;
	}
	for(i=0; i<argc; i++) {
		if(argv[i][0] == '-') {
			if(strlen(argv[i])>2) {
				return 1;
			}
			c = argv[i][1];
			i++;
			val = atoi(argv[i]);
			switch(c) {
			case 'i':
				if( val < 0 || val > 4095) {
					return 1;
				}
				paramvals->f_inte=1;
				paramvals->interval = val;
				break;
			case 't':
				if( val < 0 || val > 8) {
					return 1;
				}
 			 	paramvals->f_prio=1;
				paramvals->priority = val;
				break;
			case 'b':
				if( val < 0 || val > 1) {
					return 1;
				}
				paramvals->f_brate=1;
				paramvals->byteratebased = val;
				break;
			case 'p':
				if( val < 0 || val > 1) {
					return 1;
				}
				paramvals->f_prate=1;
				paramvals->packetratebased = val;
				break;
			case 'h':
				if( val < 0 || val > 15) {
					return 1;
				}
				paramvals->f_hist=1;
				paramvals->history = val;
				break;
			case 'n':
				if( val < 0 || val > 255) {
					return 1;
				}
				paramvals->f_maxd=1;
				paramvals->maxdelsession = val;
				break;
			case 'm':
				if( val < 0 || val > 1) {
					return 1;
				}
				paramvals->f_mode=1;
				paramvals->mode = val;
				break;
			default:
				return 1;
				break;
			}
		}
	}
	return 0;
}

int main (int argc, char *argv[])
{
	struct sockaddr_un address;
	int  socket_fd, nbytes;
	mgmt_args param_vals;

	bzero(&param_vals,sizeof(mgmt_args));
	//read the arguements
	if(argc > 1) {
		if(ppa_session_mgmt_parse_args(&param_vals, argc, argv)) {
			ppa_session_mgmt_help_fn(argv[0]);
			return 0;
		} else {
#ifdef DBG_PRINT
			printf(" Polling Interval = %d\n", param_vals.interval);
			printf(" Priority Threshold = %d\n", param_vals.priority);
			printf(" Byte Rate based Management = %d\n", param_vals.byteratebased);
			printf(" Packet Rate based Management = %d\n", param_vals.packetratebased);
			printf(" Poll history  = %d\n", param_vals.history);
			printf(" Maximum delete sessions = %d\n", param_vals.maxdelsession);
			printf(" Mode = %d\n", param_vals.mode);
#endif
		}
	} else {
		ppa_session_mgmt_help_fn(argv[0]);
		return 0;
	}
	// try to open socket open socket
	socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(socket_fd < 0) {
		printf("socket() failed\n");
		return 1;
	}
	/* start with a clean address structure */
	memset(&address, 0, sizeof(struct sockaddr_un));
	address.sun_family = AF_UNIX;
	snprintf(address.sun_path, UNIX_PATH_MAX, PPASESSMGMT_SOCKET);
	if(connect(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0) {
		// daemon doesn't exist; so start the daemon.
		system("/usr/sbin/ppasessmgmtd &");
		sleep(1);
		if(connect(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0) {
			printf("connect() failed\n");
			return 1;
		}
	}
	// send the parameters to daemon
	nbytes = write(socket_fd, &param_vals, sizeof(mgmt_args));
#ifdef DBG_PRINT
	printf("number of bytes sent = %d\n",nbytes);
#endif
	// close socket
	close(socket_fd);
	return 0;
}
