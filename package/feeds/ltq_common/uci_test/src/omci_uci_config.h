#ifndef __omci_uci_config_h__
#define __omci_uci_config_h__

#include <ucimap.h>
#include "list.h"

#ifdef INCLUDE_DMALLOC
#define DMALLOC
#define DMALLOC_FUNC_CHECK

#include <dmalloc.h>

#define IFXOS_MemAlloc(SIZE) malloc((SIZE))
#define IFXOS_MemFree(PTR) free((PTR))
#endif

#define OMCI_UCI_TUPLE_STR_MAX_SIZE	256
#define OMCI_UCI_PARAM_STR_MAX_SIZE	256

#define OMCI_UCI_NETWORK_CFG_NAME	"network"

#define uci_option_for_each(_sm, _o) \
	if (!(_sm)->options_size) \
		(_sm)->options_size = sizeof(struct uci_optmap); \
	for (_o = &(_sm)->options[0]; \
		 ((char *)(_o)) < ((char *) &(_sm)->options[0] + \
			(_sm)->options_size * (_sm)->n_options); \
		 _o = (struct uci_optmap *) ((char *)(_o) + \
			(_sm)->options_size))

struct uci_network {
	struct ucimap_section_data map;
	struct list_head list;
	struct list_head alias;

	const char *name;
	const char *ifname;
	char *proto;
	char *host_name;
	char *client_id;
	unsigned char *ipaddr;
	unsigned char *netmask;
	unsigned char *gateway;
	bool enabled;
	struct ucimap_list *aliases;
};

struct uci_alias {
	struct ucimap_section_data map;
	struct list_head list;

	const char *name;
	struct uci_network *interface;
};

int omci_uci_init(void);
void omci_uci_free(void);
int uci_network_revert(void);

struct uci_network *uci_network_get(const char *name);
int uci_network_set(struct uci_network *);

int uci_network_opt_set(struct uci_network *net, void **field,
			const void *val, const unsigned int val_size);

int omci_uci_config_get(const char *path, const char *sec, const char *opt,
			char *out);

#endif
