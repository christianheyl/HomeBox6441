#include "omci_config.h"

#ifdef INCLUDE_OMCI_ONU_UCI

#include <malloc.h>
#include <string.h>
#include <uci.h>
#include <ucimap.h>
#include "omci_uci_config.h"

static struct list_head ifs;
static struct uci_context *ctx;

static int
network_parse_ip(void *section, struct uci_optmap *om, union ucimap_data *data, const char *str)
{
	unsigned char *target;
	int tmp[4];
	int i;

	if (sscanf(str, "%d.%d.%d.%d", &tmp[0], &tmp[1], &tmp[2], &tmp[3]) != 4)
		return -1;

	target = malloc(4);
	if (!target)
		return -1;

	data->ptr = target;
	for (i = 0; i < 4; i++)
		target[i] = (char) tmp[i];

	return 0;
}

static int
network_format_ip(void *section, struct uci_optmap *om, union ucimap_data *data, char **str)
{
	static char buf[16];
	unsigned char *ip = (unsigned char *) data->ptr;

	if (ip) {
		sprintf(buf, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
		*str = buf;
	} else {
		*str = NULL;
	}

	return 0;
}

static void
network_free_ip(void *section, struct uci_optmap *om, void *ptr)
{
	free(ptr);
}

static int
network_init_interface(struct uci_map *map, void *section, struct uci_section *s)
{
	struct uci_network *net = section;

	INIT_LIST_HEAD(&net->list);
	INIT_LIST_HEAD(&net->alias);
	net->name = s->e.name;
	return 0;
}

static int
network_init_alias(struct uci_map *map, void *section, struct uci_section *s)
{
	struct uci_alias *alias = section;

	INIT_LIST_HEAD(&alias->list);
	alias->name = s->e.name;
	return 0;
}

static int
network_add_interface(struct uci_map *map, void *section)
{
	struct uci_network *net = section;

	list_add_tail(&net->list, &ifs);

	return 0;
}

static int
network_add_alias(struct uci_map *map, void *section)
{
	struct uci_alias *a = section;

	if (a->interface)
		list_add_tail(&a->list, &a->interface->alias);

	return 0;
}

static struct ucimap_section_data *
network_allocate(struct uci_map *map, struct uci_sectionmap *sm, struct uci_section *s)
{
	struct uci_network *p = malloc(sizeof(struct uci_network));
	memset(p, 0, sizeof(struct uci_network));
	return &p->map;
}

struct my_optmap {
	struct uci_optmap map;
	int test;
};

static struct uci_sectionmap network_interface;
static struct uci_sectionmap network_alias;

static struct my_optmap network_interface_options[] = {
	{
		.map = {
			UCIMAP_OPTION(struct uci_network, proto),
			.type = UCIMAP_STRING,
			.name = "proto",
			.data.s.maxlen = 32,
		}
	},
	{
		.map = {
			UCIMAP_OPTION(struct uci_network, ifname),
			.type = UCIMAP_STRING,
			.name = "ifname"
		}
	},
	{
		.map = {
			UCIMAP_OPTION(struct uci_network, ipaddr),
			.type = UCIMAP_CUSTOM,
			.name = "ipaddr",
			.data.s.maxlen = 4,
			.parse = network_parse_ip,
			.format = network_format_ip,
			.free = network_free_ip,
		}
	},
	{
		.map = {
			UCIMAP_OPTION(struct uci_network, netmask),
			.type = UCIMAP_CUSTOM,
			.name = "netmask",
			.data.s.maxlen = 4,
			.parse = network_parse_ip,
			.format = network_format_ip,
			.free = network_free_ip,
		}
	},
	{
		.map = {
			UCIMAP_OPTION(struct uci_network, gateway),
			.type = UCIMAP_CUSTOM,
			.name = "gateway",
			.data.s.maxlen = 4,
			.parse = network_parse_ip,
			.format = network_format_ip,
			.free = network_free_ip,
		}
	},
	{
		.map = {
			UCIMAP_OPTION(struct uci_network, host_name),
			.type = UCIMAP_STRING,
			.name = "host_name"
		}
	},
	{
		.map = {
			UCIMAP_OPTION(struct uci_network, client_id),
			.type = UCIMAP_STRING,
			.name = "client_id"
		}
	},
	{
		.map = {
			UCIMAP_OPTION(struct uci_network, enabled),
			.type = UCIMAP_BOOL,
			.name = "enabled",
		}
	},
	{
		.map = {
			UCIMAP_OPTION(struct uci_network, aliases),
			.type = UCIMAP_LIST | UCIMAP_SECTION | UCIMAP_LIST_AUTO,
			.data.sm = &network_alias
		}
	}
};

static struct uci_sectionmap network_interface = {
	UCIMAP_SECTION(struct uci_network, map),
	.type = "interface",
	.alloc = network_allocate,
	.init = network_init_interface,
	.add = network_add_interface,
	.options = &network_interface_options[0].map,
	.n_options = ARRAY_SIZE(network_interface_options),
	.options_size = sizeof(struct my_optmap)
};

static struct uci_optmap network_alias_options[] = {
	{
		UCIMAP_OPTION(struct uci_alias, interface),
		.type = UCIMAP_SECTION,
		.data.sm = &network_interface
	}
};

static struct uci_sectionmap network_alias = {
	UCIMAP_SECTION(struct uci_alias, map),
	.type = "alias",
	.options = network_alias_options,
	.init = network_init_alias,
	.add = network_add_alias,
	.n_options = ARRAY_SIZE(network_alias_options),
};

static struct uci_sectionmap *network_smap[] = {
	&network_interface,
	&network_alias,
};

static struct uci_map network_map = {
	.sections = network_smap,
	.n_sections = ARRAY_SIZE(network_smap),
};

static struct uci_package *pkg;

int omci_uci_init(void)
{
	int ret;

	INIT_LIST_HEAD(&ifs);
	ctx = uci_alloc_context();

	uci_set_confdir(ctx, OMCI_UCI_CFG_DIR);

	ucimap_init(&network_map);
	ret = uci_load(ctx, OMCI_UCI_NETWORK_CFG_NAME, &pkg);
	if (ret)
		return ret;

	ucimap_parse(&network_map, pkg);

	return 0;
}

void omci_uci_free(void)
{
	ucimap_cleanup(&network_map);
	uci_free_context(ctx);
}

int uci_network_revert(void)
{
	int ret = 0;
	struct uci_ptr ptr;

	if (omci_uci_init() == 0) {
		if (uci_lookup_ptr(ctx, &ptr,
				   OMCI_UCI_NETWORK_CFG_NAME,
				   false) != UCI_OK) {
			ret = -1;
		} else {
			if (uci_revert(ctx, &ptr) != UCI_OK)
				ret = -1;
		}

		omci_uci_free();
	} else {
		ret = -1;
	}

	return ret;
}

struct uci_network *uci_network_get(const char *name)
{
	struct uci_network *uci_network;
	struct list_head *p;
	struct uci_network *net;

	list_for_each(p, &ifs) {
		net = list_entry(p, struct uci_network, list);
		if (strcmp(net->name, name) == 0)
			return net;
	}

	return NULL;
}

int uci_network_set(struct uci_network *data)
{
	struct list_head *p;
	struct uci_network *net;

	list_for_each(p, &ifs) {
		net = list_entry(p, struct uci_network, list);
		if (strcmp(net->name, data->name) == 0) {
			ucimap_store_section(&network_map, pkg, &data->map);
			uci_save(ctx, pkg);
			return 0;
		}
	}

	return -1;
}

int uci_network_opt_set(struct uci_network *net, void **field,
			const void *val, const unsigned int val_size)
{
	struct ucimap_section_data *sd;
	struct uci_sectionmap *sm;
	struct uci_optmap *om;
	int ofs;

	if (!net || !field || !val)
		return -1;

	sd = &net->map;
	sm = sd->sm;
	ofs = (char *)&(*field) - (char *)((char *) sd - sm->smap_offset);

	uci_option_for_each(sm, om) {
		if (om->offset == ofs) {
			if (val_size > (unsigned int)om->data.s.maxlen)
				return -1;

			if (!*field)
				*field = malloc(val_size);
			if (!*field)
				return -1;

			switch (om->type) {
			case UCIMAP_CUSTOM:
			case UCIMAP_BOOL:
				memcpy(*field, val, val_size);
				break;
			case UCIMAP_STRING:
				strcpy((char*)*field, (char*)val);
				break;
			default:
				return -1;
			}
			ucimap_set_changed(sd, field);
			return 0;
		}
	}

	return -1;
}

int omci_uci_config_get(const char *path, const char *sec, const char *opt,
			char *out)
{
	struct uci_context *uci;
	char tuple[OMCI_UCI_TUPLE_STR_MAX_SIZE];
	struct uci_ptr ptr;
	int len, ret = 0;

	if (!out)
		return -1;

	if (path && sec) {
		len = sprintf(tuple, "%s.%s", path, sec);
		if (len < 0)
			return -1;
	} else {
		return -1;
	}

	if (opt)
		if (sprintf(&tuple[len], ".%s", opt) < 0)
			return -1;

	uci = uci_alloc_context();
	if (!uci)
		return -1;

	if (uci_lookup_ptr(uci, &ptr, tuple, true) != UCI_OK) {
		ret = -1;
		goto on_exit;
	}

	if (!(ptr.flags & UCI_LOOKUP_COMPLETE)) {
		ret = -1;
		goto on_exit;
	}

	switch(ptr.last->type) {
	case UCI_TYPE_SECTION:
		len = strlen(ptr.s->type);
		if (len > OMCI_UCI_PARAM_STR_MAX_SIZE) {
			ret = -1;
			break;
		}
		strcpy(out, ptr.s->type);
		break;
	case UCI_TYPE_OPTION:
		switch(ptr.o->type) {
		case UCI_TYPE_STRING:
			len = strlen(ptr.o->v.string);
			if (len >OMCI_UCI_PARAM_STR_MAX_SIZE) {
				ret = -1;
				break;
			}
			strcpy(out, ptr.o->v.string);
			break;
		default:
			ret = -1;
			break;
		}
		break;
	default:
		ret = -1;
		break;
	}

on_exit:
	uci_free_context(uci);
	return ret;
}

#endif
