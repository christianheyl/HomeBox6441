
#include "stdio.h"
#include "stdlib.h"
#include "errno.h"
#include "string.h"
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
//#ifdef _UDNS
#include "dns_cache_query.h"
//#endif
#include <mid_mapi_midcore.h>
#include <msg_def.h>

typedef struct InterfaceInfo_S
{
	char MACAddress[6];
	char IPAddress[18];
	char IP6Address[48];
	char SubnetMask[18];
} InterfaceInfo_T;

static int GetIPInterfaceInfo(char *ifName, InterfaceInfo_T *ifInfo);
static int StripEnter(char str[]);
static int GetDNSServer(char *inf, char *DNSServer);
static int parse_used_udns(char *domain, char *dnsServer, char *srcIP);
static char *_getword(char *line, char stop);
static int IsRouteExisted(char *testif);

int main(int argc, char *argv[])
{
	int i;
	char inf[32];
	char domain[256];
	InterfaceInfo_T ifInfo;
	char DNSServer[32];

	memset(inf, 0, sizeof(inf));
	memset(domain, 0, sizeof(domain));
	memset(&ifInfo, 0, sizeof(InterfaceInfo_T));
	memset(DNSServer, 0, sizeof(DNSServer));

	for (i=1; i<argc; i++)
	{
		if (strcmp(argv[i],"-i")==0 && i < argc -1)
		{
			strncpy(inf, argv[i+1], sizeof(inf) -1);
			inf[sizeof(inf) -1] = '\0';
			i++;
			continue;
		}
		else if (strcmp(argv[i], "-d")==0 && i < argc -1)
		{
			strncpy(DNSServer, argv[i+1], sizeof(DNSServer) -1);
			DNSServer[sizeof(DNSServer) -1] = '\0';
			i++;
			continue;
		}
		else
		{
			strncpy(domain, argv[i], sizeof(domain) -1);
			domain[sizeof(domain) -1] = '\0';
		}
	}

	if (strlen(inf) == 0)
		strcpy(inf, "ppp0");
	if (IsRouteExisted(inf))
	{
		GetIPInterfaceInfo(inf, &ifInfo);
		if (strlen(DNSServer) == 0)
			GetDNSServer(inf, DNSServer);
		if (strlen(ifInfo.IPAddress) != 0 && strlen(DNSServer) != 0)
		{
//#ifdef _UDNS
			parse_used_udns(domain, DNSServer, ifInfo.IPAddress);
//#endif
		}
		else
			printf("wrong interface %s.\n", inf);
	}
	else
	{
		printf("wrong interface %s.\n", inf);
		printf("usage: testudns -i <ifname> -d <DnsServerIP> <domain>\n");
	}
	
	return 0;
}

static int IsRouteExisted(char *testif)
{
	char data[204];
	FILE *fp;
	int found=0;
	int nScan;
	char if_name[32];

	fp = fopen("/proc/net/route", "r");
	if (fp == NULL) {
		printf("open /proc/net/route fail\n");
		return 0; // no such file?
	}

	while (fgets(data, 200, fp) != NULL) {
		nScan = sscanf(data, "%s", if_name);
		if (nScan == 1 && strcmp(if_name, testif) == 0)
		{
			found = 1;
			break;
		}			
	}

	fclose(fp);

	return found;
}

static int GetIPInterfaceInfo(char *ifName, InterfaceInfo_T *ifInfo)
{
	char buffer[256];
	char *ptr, *str;
	int find=0;
	char cmdbuf[64];
	FILE *fp;
	int pfd[2];
	char *args[3] = {NULL};
	pid_t pid;
	int i;

	/* create pipe */
	if (pipe(pfd)<0)
		return 0;

	args[0] = strdup("ifconfig");
	args[1] = strdup(ifName);

	pid = fork();
	if (pid<0) 
	{
		close(pfd[0]);
		close(pfd[1]);
		goto error_free;	
	} else if (pid==0) 
	{ /* child process */
		dup2(pfd[1], STDOUT_FILENO);
		//dup2(pfd[1], STDERR_FILENO);
		close(pfd[0]);

		execvp(args[0], args);
		exit(0);
	} 
	else
	{
		close(pfd[1]);
		//if ((fp = popen(cmdbuf, "r")) != NULL) 
		if ((fp = fdopen(pfd[0], "r")) != NULL) 
		{
			memset(ifInfo, 0, sizeof(InterfaceInfo_T));
			while(fgets(buffer, 255, fp) !=NULL) 
			{
				if (find == 1)
					continue;
				StripEnter(buffer);			
				ptr = strstr(buffer, "inet addr");
				if (ptr != NULL)
				{
					str = _getword(ptr, ':');
					ptr = _getword(str, ' ');
					strcpy(ifInfo->IPAddress, str);
					find = 1;
					continue;
				}
			}
			//pclose(fp);
			fclose(fp);
			waitpid((pid_t)pid, NULL, 0);
		}
		else
			close(pfd[0]);
	}

error_free:
	for (i=0; i<2; i++)
	{
		if (args[i] != NULL)
			free(args[i]);
	}

	if (find == 1)
		return 1;
	else
		return 0;
}

static int GetDNSServer(char *inf, char *DNSServer)
{
	char *ptr, *tok; 
	int ifNum = 0, tid;
	char VAR_DNS[32];
	char DNSServers[64];

	ptr = inf;
	while (*ptr != '\0')
	{
		if (*ptr >= '0' && *ptr <= '9')
		{
			ifNum = atoi(ptr);
			break;
		}
		ptr++;
	}

	tid = mapi_start_transc();
	sprintf(VAR_DNS, "ARC_WAN_%d_IP4_DNS", ifNum);
	memset(DNSServers, 0, sizeof(DNSServers));
	mapi_ccfg_get_str(tid, VAR_DNS, DNSServers, sizeof(DNSServers));
	mapi_end_transc(tid);
	ptr = strtok_r(DNSServers, " ,", &tok);
	if (ptr !=NULL && strlen(ptr) > 0)
	{
		strcpy(DNSServer, ptr);
		return 0;
	}
	else
		return -1;
}

//#ifdef _UDNS
static int parse_used_udns(char *domain, char *dnsServer, char *srcIP)
{
	int pureV6=0;
	char szIP[48];
	dns_ip_t *ans = NULL;

	while (*domain) {
		if (*domain == ' ') domain++;
		else break;
	}

	if (strlen(domain) == 0) 
	{
		printf("Domain is an empty string.\n");
		return -1;
	}

	printf("Bind IP %s User server %s.\n", srcIP, dnsServer);
	if (pureV6 == 0)
	{
		if (inet_addr(domain) == INADDR_NONE)
		{
			ans = dns_cache_query(dnsServer, "", UDNS_UDP, domain, srcIP, 0, 1000, 53, 0);
			if (ans == NULL)
			{
				printf("Not found %s\n", domain);
				return -2;
			}
			else
			{
				printf("Query domain %s, find IP %s.\n", domain, ans->ip);
				dns_ip_free(ans);
			}
		}
		else
			printf("Find %s is an IP.\n", domain);
	}

	return 0;
}
//#endif

static int StripEnter(char str[])
{
	int len;
	
	if (str == NULL)
		return 0;
	
	len = strlen(str);
	
	if (len >= 1 && str[len-1]==0xA)
	{
		str[len-1]='\0';
		len--;
	}
	
	if (len >= 1 && str[len-1]==0xD)
	{
		str[len-1]='\0';
		len--;
	}

	return len;
}

static char *_getword(char *line, char stop) {
	while (*line && *line != stop) line++;

	if (*line == stop) {
		*line++ = '\0';
		if (stop == 0xD && *line == 0xA)
			*line++ = '\0';
	}

	return line;
}


