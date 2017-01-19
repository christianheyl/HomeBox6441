/* dnsget.c
   simple host/dig-like application using UDNS library

   Copyright (C) 2005  Michael Tokarev <mjt@corpit.ru>
   This file is part of UDNS library, an async DNS stub resolver.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library, in file named COPYING.LGPL; if not,
   write to the Free Software Foundation, Inc., 59 Temple Place,
   Suite 330, Boston, MA  02111-1307  USA

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(LINUX) || defined(_ALDK)
#define iprintf printf
#include <netinet/in.h>
#elif defined(WINDOWS)
#define iprintf printf
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sockapi.h>
//#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#endif
//#include <time.h>
//#include <stdarg.h>
//#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "udns.h"
#include "dns_query.h"

/* zxzxas
#ifndef HAVE_GETOPT
#include "getopt.c"
#endif
*/

#ifndef AF_INET6
# define AF_INET6 10
#endif

#ifdef SUPERTASK
#define printf iprintf
#elif defined(LINUX) || defined(_ALDK)
char *inet_ntoa(struct in_addr in);
#elif defined(WIN32)
#endif

int dns_initialize()
{
	dns_init_server(NULL);

	return 0;
}

int dns_validIP_query(char *ipAddress)
{
	struct sockaddr_in sa;
	int result = dns_pton(AF_INET, ipAddress, &(sa.sin_addr));
	return result != 0;
}




void *
dns_malloc(size_t size)
{
	void *ptr = malloc(size);

	if (ptr != NULL)
	{
		memset(ptr, 0, size);
	}

	return ptr;
}

char *
dns_strncpy(char *dest, const char *src, size_t length)
{
	strncpy(dest, src, length);
	dest[length] = '\0';
	return dest;
}

char *
dns_strdup(const char *ch) // copy from osip
{
	char *copy;
	size_t length;

	if (ch == NULL)
	{
		return NULL;
	}

	length = strlen(ch);
	copy = (char *) dns_malloc(length + 1);

	if (copy == NULL)
	{
		return NULL;
	}

	dns_strncpy(copy, ch, length);
	return copy;
}

struct dns_ctx *udns_init(struct dns_ctx *ctx, char *dnsserver);

dns_srv_ans_t *dns_query_srv(char *server, char *service, char *proto, char *domain, char *localip, unsigned short localport, unsigned int timeout)
{
	dns_srv_ans_t *_ans;
	struct dns_ctx *nctx = NULL;
	struct dns_rr_srv *_rrs;
	srvAns_t *ptr;
	int i;

	printf("\n#[%s] %d, %s, %s, %s\n", __FUNCTION__, __LINE__, server, domain, localip);

	if (!(nctx = udns_init(NULL, server)))
	{
		return NULL;
	}

	if (dns_open(nctx, localip, localport) < 0)
	{
		return NULL;
	}

	//dns_set_dbgfn(nctx, dbgcb); // debug function
	_rrs = dns_resolve_srv(nctx, domain, service, proto, 0);

	if (!_rrs)
	{
		//printf( "unable to resolve nameserver %s: %s", domain, dns_strerror(dns_status(nctx)));
		dns_free(nctx);
		return NULL;
	}

	dns_close(nctx);
	dns_free(nctx);

	_ans = (dns_srv_ans_t *) malloc(sizeof(dns_srv_ans_t));
	_ans->domainName = dns_strdup(domain);
	_ans->srvHead = NULL;
	_ans->ttl = _rrs->dnssrv_ttl;

	for (i = 0; i < _rrs->dnssrv_nrr; i++)
	{
		ptr = (srvAns_t *) malloc(sizeof(srvAns_t));
		ptr->domainName = dns_strdup(_rrs->dnssrv_srv[i].name);
		ptr->port = _rrs->dnssrv_srv[i].port;
		ptr->ipArrHead = _rrs->dnssrv_srv[i].ipArrHead;

		ptr->next = _ans->srvHead;
		_ans->srvHead = ptr;
	}

	free(_rrs);

	return _ans;
}


int dns_query_a_by_srv(srvAns_t *srv)
{
	return 0;
	/*
		dns_ans_t *_ans;
		if( !dns_query_a(srv->domainName, &_ans) )
		{
			srv->ipArrHead = _ans->ipArrHead;
			_ans->ipArrHead = NULL;
			dns_ans_free(_ans);
			return 0;
		}
		else return -1;
		*/
}



dns_a4_ans_t *dns_query_a4(char *server, char *domain, char *localip, unsigned short localport, unsigned int timeout)
{
	dns_a4_ans_t *_ans;
	struct dns_ctx *nctx = NULL;
	struct dns_rr_a4 *_rra;
	int i;

	if (!(nctx = udns_init(NULL, server)))
	{
		return NULL;
	}

	if ((i = dns_open(nctx, localip, localport)) < 0)
	{
		iprintf("\n#[%s] %d, dns_open fail %d\n", __FUNCTION__, __LINE__, i);
		return NULL;
	}

	_rra = dns_resolve_a4(nctx, domain, 0);

	if (!_rra)
	{
		//printf( "unable to resolve nameserver %s: %s", domain, dns_strerror(dns_status(nctx)));
		dns_free(nctx);
		return NULL;
	}

	dns_close(nctx);
	dns_free(nctx);

	_ans = (dns_a4_ans_t *) malloc(sizeof(dns_a4_ans_t));
	_ans->domainName = dns_strdup(domain);
	_ans->ipArrHead = NULL;
	_ans->ttl = _rra->dnsa4_ttl;

	for (i = 0; i < _rra->dnsa4_nrr; i++)
	{
		struct ipArr *_new = (struct ipArr *) malloc(sizeof(struct ipArr));
		_new->ip = dns_strdup(inet_ntoa(_rra->dnsa4_addr[i]));
		_new->next = _ans->ipArrHead;
		_ans->ipArrHead = _new;
	}

	free(_rra);

	return _ans;
}

#define INET6_ADDRSTRLEN 64

dns_a4_ans_t *dns_query_a6(char *server, char *domain, char *localip, unsigned short localport, unsigned int timeout)
{
	dns_a4_ans_t *_ans;
	struct dns_ctx *nctx = NULL;
	struct dns_rr_a6 *_rra;
	int i;
	char str[INET6_ADDRSTRLEN];

	if (!(nctx = udns_init(NULL, server)))
	{
		return NULL;
	}

	if ((i = dns_open(nctx, localip, localport)) < 0)
	{
		iprintf("\n#[%s] %d, dns_open fail %d\n", __FUNCTION__, __LINE__, i);
		return NULL;
	}

	_rra = dns_resolve_a6(nctx, domain, 0);

	if (!_rra)
	{
		//printf( "unable to resolve nameserver %s: %s", domain, dns_strerror(dns_status(nctx)));
		dns_free(nctx);
		return NULL;
	}

	dns_close(nctx);
	dns_free(nctx);

	_ans = (dns_a6_ans_t *) malloc(sizeof(dns_a6_ans_t));
	_ans->domainName = dns_strdup(domain);
	_ans->ipArrHead = NULL;
	_ans->ttl = _rra->dnsa6_ttl;

	for (i = 0; i < _rra->dnsa6_nrr; i++)
	{
		struct ipArr *_new = (struct ipArr *) malloc(sizeof(struct ipArr));
		_new->ip = dns_strdup(inet_ntop(AF_INET6, &(_rra->dnsa6_addr[i]), str, INET6_ADDRSTRLEN));
		_new->next = _ans->ipArrHead;
		_ans->ipArrHead = _new;
	}

	free(_rra);

	return _ans;
}

int dns_query_naptr(char *server, char *domain, char *localip, unsigned short localport, unsigned int timeout)
{
	dns_naptr_ans_t *_ans = NULL;
	naptrAns_t *_tail = NULL;
	struct dns_ctx *nctx = NULL;
	struct dns_rr_naptr *_rrn;
	int i, j;

	if (!(nctx = udns_init(NULL, server)))
	{
		return NULL;
	}

	if ((i = dns_open(nctx, localip, localport)) < 0)
	{
		iprintf("\n#[%s] %d, dns_open fail %d\n", __FUNCTION__, __LINE__, i);
		return NULL;
	}

	_rrn = dns_resolve_naptr(nctx, domain, 0);

	if (!_rrn)
	{
		//printf( "unable to resolve nameserver %s: %s", domain, dns_strerror(dns_status(nctx)));
		dns_free(nctx);
		return NULL;
	}

	dns_close(nctx);
	dns_free(nctx);

	for (i = 0; i < _rrn->dnsnaptr_nrr - 1; i++)
	{
		for (j = i + 1; j < _rrn->dnsnaptr_nrr; j++)
		{
			if (_rrn->dnsnaptr_naptr[i].preference > _rrn->dnsnaptr_naptr[j].preference)
			{
				struct dns_naptr tmp;
				memcpy(&tmp, &(_rrn->dnsnaptr_naptr[i].preference), sizeof(struct dns_naptr));
				memcpy(&(_rrn->dnsnaptr_naptr[i].preference), &(_rrn->dnsnaptr_naptr[j].preference), sizeof(struct dns_naptr));
				memcpy(&(_rrn->dnsnaptr_naptr[j].preference), &tmp, sizeof(struct dns_naptr));
			}
			else if (_rrn->dnsnaptr_naptr[i].preference == _rrn->dnsnaptr_naptr[j].preference)
			{
				if (_rrn->dnsnaptr_naptr[i].order > _rrn->dnsnaptr_naptr[j].order)
				{
					struct dns_naptr tmp;
					memcpy(&tmp, &(_rrn->dnsnaptr_naptr[i].preference), sizeof(struct dns_naptr));
					memcpy(&(_rrn->dnsnaptr_naptr[i].preference), &(_rrn->dnsnaptr_naptr[j].preference), sizeof(struct dns_naptr));
					memcpy(&(_rrn->dnsnaptr_naptr[j].preference), &tmp, sizeof(struct dns_naptr));
				}
			}
		}
	}

	_ans = (dns_naptr_ans_t *) malloc(sizeof(dns_naptr_ans_t));
	_ans->domainName = dns_strdup(domain);
	_ans->ttl = _rrn->dnsnaptr_ttl;
	_ans->naptrList = NULL;

	for (i = 0; i < _rrn->dnsnaptr_nrr; i++)
	{
		naptrAns_t *tmp = (naptrAns_t *) malloc(sizeof(naptrAns_t));

		tmp->flags = dns_strdup(_rrn->dnsnaptr_naptr[i].flags);
		tmp->service = dns_strdup(_rrn->dnsnaptr_naptr[i].service);
		tmp->replacement = dns_strdup(_rrn->dnsnaptr_naptr[i].replacement);
		tmp->srvList = NULL;
		tmp->next = NULL;

		if (_ans->naptrList)
		{
			_tail->next = tmp;
			_tail = tmp;
		}
		else
		{
			_ans->naptrList = tmp;
			_tail = tmp;
		}
	}

	free(_rrn);

	return _ans;
}
