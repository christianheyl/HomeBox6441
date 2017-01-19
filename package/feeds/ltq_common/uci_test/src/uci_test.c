#include <stdint.h>
#include <assert.h>
#include <string.h>
#include "omci_config.h"
#include "omci_uci_config.h"

struct omci_context { int dummy; };
struct me { uint16_t instance_id; };
enum omci_error {
	OMCI_SUCCESS = 0,
	OMCI_ERROR = -1
};

const char *ip_host_ifname_get(uint16_t instance_id)
{
	/* let ME instance 0 be the wan interface */
	static const char *net_name[2] = {"wan", "lan"};

	assert(instance_id < 2);

	return net_name[instance_id];
}

static enum omci_error current_gateway_get(struct omci_context *context,
					 struct me *me,
					 void *data,
					 size_t data_size)
{
#ifdef INCLUDE_OMCI_ONU_UCI
	struct uci_network *net = NULL;
#endif
	assert(data_size == 4);

#ifdef INCLUDE_OMCI_ONU_UCI
	if (omci_uci_init() == 0) {
		net = uci_network_get(ip_host_ifname_get(me->instance_id));
		if (net) {
			if (net->gateway)
				memcpy(data, net->gateway, data_size);
			else
				memset(data, 0, data_size);
		}
		omci_uci_free();
	}
#endif

	return OMCI_SUCCESS;
}

int main(int argc, char *argv[])
{
	struct me me;
	uint32_t data;
	enum omci_error error;

	me.instance_id = 1;

#ifdef INCLUDE_DMALLOC
	dmalloc_logpath = "/tmp/uci_test_dmalloc";

	if (!dmalloc_debug_current())
	dmalloc_debug(0xFFFFFFFF ^ (1 << 28));
#endif

	error = current_gateway_get(NULL, &me, &data, sizeof(data));
	assert(error == OMCI_SUCCESS);

	printf("data=0x%x\n", data);

	return 0;
}
