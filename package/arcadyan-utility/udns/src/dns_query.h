#ifndef _DNS_QUERY_H
#define _DNS_QUERY_H

#include <time.h>
#include "dns_cache_query.h"

dns_a4_ans_t *dns_query_a4(char *server, char *domain, char *localip, unsigned short localport, unsigned int timeout);
dns_a4_ans_t *dns_query_a6(char *server, char *domain, char *localip, unsigned short localport, unsigned int timeout);
dns_srv_ans_t *dns_query_srv(char *server, char *service, char *proto, char *domain, char *my_ip, unsigned short my_port, unsigned int timeout);
int dns_validIP_query(char *ipAddress);

#endif