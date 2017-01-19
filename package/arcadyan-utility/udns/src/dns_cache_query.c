#include "dns_cache_query.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "dns_query.h"

#ifdef SUPERTASK
//#define OLD_DNS
#else
#define iprintf printf
#endif
#ifdef OLD_DNS
#include "intf.h"
#include "config.h"
#include "dns_addr.h"
#endif
dns_srv_ans_t *dns_srv_ans_list = NULL;
dns_a4_ans_t *dns_a4_ans_list = NULL;
dns_a4_ans_t *dns_a6_ans_list = NULL;
dns_naptr_ans_t *dns_naptr_ans_list = NULL;

int dns_validIP(char *ipAddress)
{
	return dns_validIP_query(ipAddress);
}

int dns_cache_release()
{
	while (dns_a4_ans_list != NULL)
	{
		dns_a4_ans_t *tmpA = dns_a4_ans_list;
		dns_a4_ans_list = dns_a4_ans_list->next;
		dns_a4_ans_free(tmpA);
	}

	while (dns_srv_ans_list != NULL)
	{
		dns_srv_ans_t *tmpS = dns_srv_ans_list;
		dns_srv_ans_list = dns_srv_ans_list->next;
		dns_srv_ans_free(tmpS);
	}

	while (dns_naptr_ans_list != NULL)
	{
		dns_naptr_ans_t *tmpN = dns_naptr_ans_list;
		dns_naptr_ans_list = dns_naptr_ans_list->next;
		dns_naptr_ans_free(tmpN);
	}

#ifdef OLD_DNS
	DNS_ADDR_CacheClearAll();
#endif

	return 0;
}

int dns_check_expire_naptr(char *domain)
{
	time_t tt = time(NULL);
	dns_naptr_ans_t *pointer = dns_naptr_ans_list;
	dns_naptr_ans_t *privious = dns_naptr_ans_list;

	while (pointer != NULL)
	{
		if (!strcmp(pointer->domainName, domain))
		{
			if (tt >= pointer->ttl)	// timeout
			{
				if (privious == pointer)
				{
					dns_srv_ans_list = pointer->next;
				}
				else
				{
					privious->next = pointer->next;
				}

				dns_naptr_ans_free(pointer);

				return 1;
			}
			else
			{
				return 0;
			}
		}
		else	// unmatch
		{
			if (tt >= pointer->ttl)	// timeout
			{
				dns_srv_ans_t *tmp = pointer;

				if (privious == pointer)
				{
					dns_srv_ans_list = pointer->next;
					privious = pointer->next;
				}
				else
				{
					privious->next = pointer->next;
				}

				dns_naptr_ans_free(tmp);
				continue;
			}
		}

		privious = pointer;
		pointer = pointer->next;
	}

	return 2;
}

int dns_check_expire_srv(char *domain)
{
	time_t tt = time(NULL);
	dns_srv_ans_t *pointer = dns_srv_ans_list;
	dns_srv_ans_t *privious = dns_srv_ans_list;

	while (pointer != NULL)
	{
		if (!strcmp(pointer->domainName, domain))
		{
			if (tt >= pointer->ttl)	// timeout
			{
				if (privious == pointer)
				{
					dns_srv_ans_list = pointer->next;
				}
				else
				{
					privious->next = pointer->next;
				}

				dns_srv_ans_free(pointer);

				return 1;
			}
			else
			{
				return 0;
			}
		}
		else	// unmatch
		{
			if (tt >= pointer->ttl)	// timeout
			{
				dns_srv_ans_t *tmp = pointer;

				if (privious == pointer)
				{
					dns_srv_ans_list = pointer->next;
					privious = pointer->next;
				}
				else
				{
					privious->next = pointer->next;
				}

				dns_srv_ans_free(tmp);
				continue;
			}
		}

		privious = pointer;
		pointer = pointer->next;
	}

	return 2;
}

int dns_check_expire_a4(char *domain)
{
	time_t tt = time(NULL);
	dns_a4_ans_t *pointer = dns_a4_ans_list;
	dns_a4_ans_t *privious = dns_a4_ans_list;

	while (pointer != NULL)
	{
		if (!strcmp(pointer->domainName, domain))
		{
			if (tt >= pointer->ttl)	// timeout
			{
				if (privious == pointer)
				{
					dns_a4_ans_list = pointer->next;
				}
				else
				{
					privious->next = pointer->next;
				}

				dns_a4_ans_free(pointer);

				return 1;
			}
			else
			{
				return 0;
			}
		}
		else	// unmatch
		{
			if (tt >= pointer->ttl)	// timeout
			{
				if (privious == pointer)
				{
					dns_a4_ans_list = pointer->next;
				}
				else
				{
					privious->next = pointer->next;
				}

				dns_a4_ans_free(pointer);
			}
		}

		privious = pointer;
		pointer = pointer->next;
	}

	return 2;
}

int dns_check_expire(char *domain)	// 0:finded		1:timerout	2:can't find
{
	int j = dns_check_expire_naptr(domain);

	if (j == 2)
	{
		int i = dns_check_expire_srv(domain);

		if (i == 2)
		{
			return dns_check_expire_a4(domain);
		}
		else
		{
			return i;
		}
	}
	else
	{
		return j;
	}
}


int dns_a4_ans_free(dns_a4_ans_t *ans)
{
	if (ans == NULL)
	{
		return 0;
	}

	while (ans->ipArrHead != NULL)
	{
		struct ipArr *tmpA = ans->ipArrHead->next;
		free(ans->ipArrHead->ip);
		free(ans->ipArrHead);
		ans->ipArrHead = tmpA;
	}

	free(ans->domainName);
	free(ans);

	return 0;
}

void dns_srv_list_free(srvAns_t * ls)
{
	while (ls != NULL)
	{
		struct srvAns *tmp = ls->next;
		free(ls->domainName);

		while (ls->ipArrHead != NULL)
		{
			struct ipArr *tmpA = ls->ipArrHead->next;
			free(ls->ipArrHead->ip);
			free(ls->ipArrHead);
			ls->ipArrHead = tmpA;
		}

		free(ls);
		ls = tmp;
	}
}

void dns_naptr_list_free(naptrAns_t *ls)
{
	while (ls)
	{
		naptrAns_t *tmp = ls->next;

		if (ls->flags) free(ls->flags);
		if (ls->replacement) free(ls->replacement);
		if (ls->service) free(ls->service);

		dns_srv_list_free(ls->srvList);

		free(ls);
		ls = tmp;
	}
}

int dns_naptr_ans_free(dns_naptr_ans_t *ans)
{
	if (ans == NULL)
	{
		return 0;
	}

	dns_naptr_list_free(ans->naptrList);

	free(ans->domainName);
	free(ans);

	return 0;
}

int dns_srv_ans_free(dns_srv_ans_t *ans)
{
	if (ans == NULL)
	{
		return 0;
	}

	dns_srv_list_free(ans->srvHead);

	free(ans->domainName);
	free(ans);

	return 0;
}

dns_a4_ans_t *dns_a4_ans_dup(dns_a4_ans_t *ans)
{
	dns_a4_ans_t *_ans;
	ipArr_t *last;
	ipArr_t *pointer;

	if (ans == NULL)
	{
		return NULL;
	}

	_ans = (dns_a4_ans_t *) malloc(sizeof(dns_a4_ans_t));

	_ans->domainName = strdup(ans->domainName);
	_ans->next = NULL;
	_ans->ttl = ans->ttl;

	last = NULL;
	pointer = ans->ipArrHead;

	while (pointer != NULL)
	{
		ipArr_t *tmpA = (ipArr_t *) malloc(sizeof(ipArr_t));

		tmpA->ip = strdup(pointer->ip);
		tmpA->next = NULL;

		if (last == NULL)
		{
			_ans->ipArrHead = tmpA;
		}
		else
		{
			last->next = tmpA;
		}

		last = tmpA;

		pointer = pointer->next;
	}

	return _ans;
}

dns_srv_ans_t *dns_srv_ans_dup(dns_srv_ans_t *ans)
{
	dns_srv_ans_t *_ans;
	ipArr_t *last;
	ipArr_t *pointer;
	srvAns_t *last_srv;
	srvAns_t *pointer_srv;

	if (ans == NULL)
	{
		return NULL;
	}

	_ans = (dns_srv_ans_t *) malloc(sizeof(dns_srv_ans_t));

	_ans->domainName = strdup(ans->domainName);
	_ans->next = NULL;
	_ans->ttl = ans->ttl;

	last_srv = NULL;
	pointer_srv = ans->srvHead;

	while (pointer_srv != NULL)
	{
		srvAns_t *tmpS = (srvAns_t *) malloc(sizeof(srvAns_t));

		tmpS->domainName = strdup(pointer_srv->domainName);
		tmpS->ipArrHead = NULL;
		tmpS->next = NULL;
		tmpS->port = pointer_srv->port;

		if (last_srv == NULL)
		{
			_ans->srvHead = tmpS;
		}
		else
		{
			last_srv->next = tmpS;
		}

		last_srv = tmpS;

		last = NULL;
		pointer = pointer_srv->ipArrHead;

		while (pointer != NULL)
		{
			ipArr_t *tmpA = (ipArr_t *) malloc(sizeof(ipArr_t));

			tmpA->ip = strdup(pointer->ip);
			tmpA->next = NULL;

			if (last == NULL)
			{
				last_srv->ipArrHead = tmpA;
			}
			else
			{
				last->next = tmpA;
			}

			last = tmpA;

			pointer = pointer->next;
		}

		pointer_srv = pointer_srv->next;

	}

	return _ans;
}

dns_ip_t *dns_cache_query(char *server, char *service, dns_txp_mode_t proto, char *domain, char *my_ip, unsigned short my_port, unsigned int timeout, unsigned short default_answer_port, dns_query_type_t dns_type)
{
	dns_ip_t *head_ip = NULL;
	dns_ip_t *last_ip = NULL;
	dns_srv_ans_t *ans_srv = NULL;
	dns_naptr_ans_t *ans_naptr = NULL;

	char *dns_server_list = (char *)malloc(strlen(server) + 4);

	if (dns_server_list == NULL)
	{
		return NULL;
	}

	strcpy(dns_server_list, server);

	if (dns_type >= DNS_NAPTR)
	{
		printf("\n#[%s] %d, dns_cache_query_naptr domain %s\n", __FUNCTION__, __LINE__, domain); 
		ans_naptr = dns_cache_query_naptr(dns_server_list, domain, my_ip, my_port, timeout);
	}

	if (ans_naptr)
	{
		naptrAns_t *tmpN = ans_naptr->naptrList;

		while (tmpN)
		{
			srvAns_t *tmpS = tmpN->srvList;

			while (tmpS)
			{
				ipArr_t *tmpA = tmpS->ipArrHead;

				while (tmpA)
				{
					dns_ip_t *tmpIP = (dns_ip_t *) malloc(sizeof(dns_ip_t));
					tmpIP->ip = strdup(tmpA->ip);
					tmpIP->port = tmpS->port ? tmpS->port : 5060;
					tmpIP->proto_type = strstr(tmpN->service, "D2T") ? UDNS_TCP : UDNS_UDP;
					tmpIP->next = NULL;

					if (last_ip == NULL)
					{
						head_ip = tmpIP;
					}
					else
					{
						last_ip->next = tmpIP;
					}

					last_ip = tmpIP;

					tmpA = tmpA->next;
				}

				tmpS = tmpS->next;
			}

			tmpN = tmpN->next;
		}

		if (head_ip)
		{
			free(dns_server_list);
			return head_ip;
		}
	}

	if (dns_type >= DNS_SRV)
	{
		printf("\n#[%s] %d, dns_cache_query_srv domain %s\n", __FUNCTION__, __LINE__, domain); 
		ans_srv = dns_cache_query_srv(dns_server_list, service, proto == UDNS_TCP ? "tcp" : "udp", domain, my_ip, my_port, timeout);
	}

	if (ans_srv)
	{
		srvAns_t *tmpS = ans_srv->srvHead;

		while (tmpS != NULL)
		{
			ipArr_t *tmpA = tmpS->ipArrHead;

			while (tmpA != NULL)
			{
				dns_ip_t *tmpIP = (dns_ip_t *) malloc(sizeof(dns_ip_t));
				tmpIP->ip = strdup(tmpA->ip);
				tmpIP->port = tmpS->port ? tmpS->port : 5060;
				tmpIP->proto_type = proto;
				tmpIP->next = NULL;

				if (last_ip == NULL)
				{
					head_ip = tmpIP;
				}
				else
				{
					last_ip->next = tmpIP;
				}

				last_ip = tmpIP;

				tmpA = tmpA->next;
			}

			tmpS = tmpS->next;
		}
	}
	else
	{
		dns_a4_ans_t *ans_a4;

		printf("\n#[%s] %d, dns_cache_query_a4 domain %s\n", __FUNCTION__, __LINE__, domain); 
		ans_a4 = dns_cache_query_a4(dns_server_list, domain, my_ip, my_port, timeout);

		if (ans_a4 != NULL)
		{
			ipArr_t *tmpA = ans_a4->ipArrHead;

			while (tmpA != NULL)
			{
				dns_ip_t *tmpIP = (dns_ip_t *) malloc(sizeof(dns_ip_t));
				tmpIP->ip = strdup(tmpA->ip);
				tmpIP->port = default_answer_port;
				tmpIP->proto_type = proto;
				tmpIP->next = NULL;

				if (last_ip == NULL)
				{
					head_ip = tmpIP;
				}
				else
				{
					last_ip->next = tmpIP;
				}

				last_ip = tmpIP;

				tmpA = tmpA->next;
			}
		}
	}

	free(dns_server_list);

	return head_ip;
}


dns_srv_ans_t *dns_cache_query_srv(char *server, char *service, char *proto, char *domain, char *my_ip, unsigned short my_port, unsigned int timeout)
{
	time_t tt = time(NULL);
	dns_srv_ans_t *pointer = dns_srv_ans_list;
	dns_srv_ans_t *privious = dns_srv_ans_list;
	dns_srv_ans_t *srv_ans;

	pointer = dns_srv_ans_list;

	while (pointer != NULL)
	{

		if (tt >= pointer->ttl)	// timeout
		{
			dns_srv_ans_t *tmp = pointer;

			if (privious == pointer)
			{
				dns_srv_ans_list = pointer->next;
				privious = pointer->next;
			}
			else
			{
				privious->next = pointer->next;
			}

			pointer = pointer->next;
			dns_srv_ans_free(tmp);
			continue;
		}
		else
		{
			if (!strcmp(pointer->domainName, domain))
			{
				return pointer;
			}
		}

		privious = pointer;
		pointer = pointer->next;
	}

	srv_ans = dns_query_srv(server, service, proto, domain, my_ip, my_port, timeout);

	if (srv_ans != NULL)
	{
		srvAns_t *pointer_s;
		srvAns_t *privious_s;

		srv_ans->ttl += tt;

		if (privious != NULL)
		{
			privious->next = srv_ans;
		}
		else
		{
			dns_srv_ans_list = srv_ans;
		}

		srv_ans->next = NULL;

		pointer_s = srv_ans->srvHead;
		privious_s = srv_ans->srvHead;

		while (pointer_s != NULL)
		{
			if (pointer_s->ipArrHead != NULL)
			{
				privious_s = pointer_s;
				pointer_s = pointer_s->next;
			}
			else
			{
				dns_a4_ans_t *ans_a4 = dns_query_a4(server, pointer_s->domainName, my_ip, my_port, timeout);

				if (ans_a4 != NULL)
				{
					pointer_s->ipArrHead = ans_a4->ipArrHead;
					free(ans_a4->domainName);
					free(ans_a4);

					privious_s = pointer_s;
					pointer_s = pointer_s->next;
				}
				else
				{
					srvAns_t *srv = pointer_s;
					pointer_s = pointer_s->next;

					if (privious_s == srv)
					{
						privious_s = pointer_s;
						srv_ans->srvHead = pointer_s;
					}

					free(srv->domainName);
					free(srv);
				}
			}
		}

		if (srv_ans->srvHead == NULL)
		{
			dns_srv_ans_free(srv_ans);
		}
		else
		{
			return srv_ans;
		}
	}

	return NULL;
}

dns_a4_ans_t *dns_cache_query_a4(char *server, char *domain, char *my_ip, unsigned short my_port, unsigned int timeout)
{
	time_t tt = time(NULL);
	dns_a4_ans_t *pointer = dns_a4_ans_list;
	dns_a4_ans_t *privious = dns_a4_ans_list;
	dns_a4_ans_t *a4_ans;

	while (pointer != NULL)
	{
		if (tt >= pointer->ttl)	// timeout
		{
			dns_a4_ans_t *tmp = pointer;

			if (privious == pointer)
			{
				dns_a4_ans_list = pointer->next;
				privious = pointer->next;
			}
			else
			{
				privious->next = pointer->next;
			}

			pointer = pointer->next;
			dns_a4_ans_free(tmp);
			continue;
		}
		else
		{
			if (!strcmp(pointer->domainName, domain))
			{
				return pointer;
			}
		}

		privious = pointer;
		pointer = pointer->next;
	}

//iprintf("[test][dns_cache_query_a4]\n");
#ifdef OLD_DNS
	{
		int retVar = 0, ifno, vcIdx;
		struct timeval tt;
		DNS_ADDR_T *anss;

		tt.tv_sec = 1;
		tt.tv_usec = 0;

		vcIdx = 0;

		if (vcIdx == 0)
		{
			vcIdx = 1;
		}

		ifno = T_ATM_INT + (vcIdx - 1);

		if (getWANType(ifno) == WAN_TYPE_PPPOE)
		{
			retVar = T_PPPoE_INT + (vcIdx - 1);
		}
		else
		{
			retVar = ifno;
		}

		DNS_ADDR_QueryEx(domain, tt, &anss, retVar);

		if (anss == NULL)
		{
			iprintf("\n#[%s] %d, no ans\n", __FUNCTION__, __LINE__);
			return NULL;
		}

		{
			dns_a4_ans_t *_ans;
			DNS_ADDR_T *tmp;

			_ans = (dns_a4_ans_t *) malloc(sizeof(dns_a4_ans_t));
			_ans->domainName = dns_strdup(domain);
			_ans->ipArrHead = NULL;
			_ans->ttl = anss->ttl;

			tmp = anss;

			while (tmp != NULL)
			{
				struct ipArr *_new = (struct ipArr *) malloc(sizeof(struct ipArr));
				struct in_addr addrin;
				addrin.s_addr = tmp->addr;
				_new->ip = dns_strdup(inet_ntoa(addrin));
				_new->next = NULL;

				if (_ans->ipArrHead)
				{
					tail->next = _new;
				}
				else
				{
					_ans->ipArrHead = _new;
				}
				tail = _new;

				tmp = tmp->next;
			}

			DNS_ADDR_RecordFree(anss);

			a4_ans = _ans;


		}
	}
#else
	a4_ans = dns_query_a4(server, domain, my_ip, my_port, timeout);
#endif

//iprintf("[test][dns_cache_query_a4]\n");
	if (a4_ans != NULL)
	{

		if (a4_ans->ttl < 16)
		{
			a4_ans->ttl = 16 + tt;
		}
		else
		{
			a4_ans->ttl += tt;
		}


		//a4_ans->ttl = tt;

		if (privious != NULL)
		{
			privious->next = a4_ans;
		}
		else
		{
			dns_a4_ans_list = a4_ans;
		}

		a4_ans->next = NULL;
		return a4_ans;
	}

//iprintf("[test][dns_cache_query_a4] null\n");
	return NULL;
}

int dns_ip_printf(dns_ip_t *ip)
{
	dns_ip_t *tmpIP = ip;

	while (tmpIP != NULL)
	{
		iprintf("%s\n", tmpIP->ip);
		tmpIP = tmpIP->next;
	}

	return 0;
}

int dns_ip_free(dns_ip_t *ip)
{

	while (ip != NULL)
	{
		dns_ip_t *tmpIP = ip;
		ip = ip->next;

		free(tmpIP->ip);
		free(tmpIP);
	}

	return 0;
}

dns_a6_ans_t *dns_cache_query_a6(char *server, char *domain, char *my_ip, unsigned short my_port, unsigned int timeout)
{
	time_t tt = time(NULL);
	dns_a6_ans_t *pointer = dns_a6_ans_list;
	dns_a6_ans_t *privious = dns_a6_ans_list;
	dns_a6_ans_t *a6_ans;

	while (pointer != NULL)
	{
		if (tt >= pointer->ttl)	// timeout
		{
			dns_a4_ans_t *tmp = pointer;

			if (privious == pointer)
			{
				dns_a4_ans_list = pointer->next;
				privious = pointer->next;
			}
			else
			{
				privious->next = pointer->next;
			}

			pointer = pointer->next;
			dns_a4_ans_free(tmp);
			continue;
		}
		else
		{
			if (!strcmp(pointer->domainName, domain))
			{
				return pointer;
			}
		}

		privious = pointer;
		pointer = pointer->next;
	}

	a6_ans = dns_query_a6(server, domain, my_ip, my_port, timeout);

//iprintf("[test][dns_cache_query_a4]\n");
	if (a6_ans != NULL)
	{
		//printf("\n#[%s] %d, %d\n", __FUNCTION__, __LINE__, a6_ans->ttl); 
		if (a6_ans->ttl < 16)
		{
			a6_ans->ttl = 16 + tt;
		}
		else
		{
			a6_ans->ttl += tt;
		}


		//a4_ans->ttl = tt;

		if (privious != NULL)
		{
			privious->next = a6_ans;
		}
		else
		{
			dns_a6_ans_list = a6_ans;
		}

		a6_ans->next = NULL;
		return a6_ans;
	}

//iprintf("[test][dns_cache_query_a4] null\n");
	return NULL;
}


dns_ip_t *dns_query_test_ipv6(char *server, char *service, char *proto, char *domain, char *my_ip, unsigned short my_port, unsigned int timeout, unsigned short default_answer_port, char dns_type)
{
	dns_ip_t *head_ip = NULL;
	dns_ip_t *last_ip = NULL;
	dns_srv_ans_t *ans_srv = NULL;

	char *dns_server_list = (char *)malloc(strlen(server) + 4);

	if (dns_server_list == NULL)
	{
		return NULL;
	}

	{
		dns_a6_ans_t *ans_a6;

		strcpy(dns_server_list, server);
		ans_a6 = dns_cache_query_a6(dns_server_list, domain, my_ip, my_port, timeout);

		//printf("\n#[%s] %d, \n", __FUNCTION__, __LINE__); 

		if (ans_a6 != NULL)
		{
			ipArr_t *tmpA = ans_a6->ipArrHead;

			while (tmpA != NULL)
			{
				//printf("\n#[%s] %d, %s\n", __FUNCTION__, __LINE__, tmpA->ip); 
				dns_ip_t *tmpIP = (dns_ip_t *) malloc(sizeof(dns_ip_t));
				tmpIP->ip = strdup(tmpA->ip);
				tmpIP->port = default_answer_port;
				tmpIP->next = NULL;

				if (last_ip == NULL)
				{
					head_ip = tmpIP;
				}
				else
				{
					last_ip->next = tmpIP;
				}

				last_ip = tmpIP;

				tmpA = tmpA->next;
			}
		}
	}

	free(dns_server_list);

	return head_ip;
}

dns_naptr_ans_t *dns_cache_query_naptr(char *server, char *domain, char *my_ip, unsigned short my_port, unsigned int timeout)
{
	time_t tt = time(NULL);
	dns_naptr_ans_t *pointer = dns_naptr_ans_list;
	dns_naptr_ans_t *privious = dns_naptr_ans_list;
	dns_naptr_ans_t *naptr_ans = NULL;

	pointer = dns_naptr_ans_list;

	while (pointer != NULL)
	{
		if (tt >= pointer->ttl)	// timeout
		{
			dns_naptr_ans_t *tmp = pointer;

			if (privious == pointer)
			{
				dns_naptr_ans_list = pointer->next;
				privious = pointer->next;
			}
			else
			{
				privious->next = pointer->next;
			}

			pointer = pointer->next;
			dns_naptr_ans_free(tmp);
			continue;
		}
		else
		{
			if (!strcmp(pointer->domainName, domain))
			{
				return pointer;
			}
		}

		privious = pointer;
		pointer = pointer->next;
	}

	naptr_ans = dns_query_naptr(server, domain, my_ip, my_port, timeout);

	if (naptr_ans != NULL)
	{
		naptrAns_t *ptr;

		naptr_ans->ttl += tt;

		if (privious != NULL)
		{
			privious->next = naptr_ans;
		}
		else
		{
			dns_naptr_ans_list = naptr_ans;
		}

		naptr_ans->next = NULL;

		ptr = naptr_ans->naptrList;

		while (ptr)
		{
			if (ptr->srvList == NULL)
			{
				dns_srv_ans_t *srv_ans = NULL;

				srv_ans = dns_query_srv(server, NULL, NULL, ptr->replacement, my_ip, my_port, timeout);
/*
				if (ptr->service && strcmp(ptr->service, "SIP+D2U") == 0)
				{
					srv_ans = dns_query_srv(server, NULL, NULL, domain, my_ip, my_port, timeout);
				}
				else if (ptr->service && strcmp(ptr->service, "SIP+D2T") == 0)
				{
					srv_ans = dns_query_srv(server, "sip", "tcp", domain, my_ip, my_port, timeout);
				}
				else
				{
					printf("\n#[%s] %d, Not Support %s\n", __FUNCTION__, __LINE__, ptr->service); 
				}
	*/			
				if (srv_ans)
				{
					srvAns_t *pointer_s = srv_ans->srvHead;
					srvAns_t *privious_s = srv_ans->srvHead;

					while (pointer_s != NULL)
					{
						if (pointer_s->ipArrHead != NULL)
						{
							privious_s = pointer_s;
							pointer_s = pointer_s->next;
						}
						else
						{
							dns_a4_ans_t *ans_a4 = dns_query_a4(server, pointer_s->domainName, my_ip, my_port, timeout);

							if (ans_a4 != NULL)
							{
								pointer_s->ipArrHead = ans_a4->ipArrHead;
								free(ans_a4->domainName);
								free(ans_a4);

								privious_s = pointer_s;
								pointer_s = pointer_s->next;
							}
							else
							{
								srvAns_t *srv = pointer_s;
								pointer_s = pointer_s->next;

								if (privious_s == srv)
								{
									privious_s = pointer_s;
									srv_ans->srvHead = pointer_s;
								}

								free(srv->domainName);
								free(srv);
							}
						}
					}

					ptr->srvList = srv_ans->srvHead;
					free(srv_ans->domainName);
					free(srv_ans);
				}
			}

			ptr = ptr->next;
		}
	}

	return naptr_ans;
}
