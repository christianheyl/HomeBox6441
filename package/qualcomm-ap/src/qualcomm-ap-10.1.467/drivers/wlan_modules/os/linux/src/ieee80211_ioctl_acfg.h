#include <linux/wireless.h>
#include <linux/etherdevice.h>
#include <net/iw_handler.h>

#if UMAC_SUPPORT_ACFG

#define ACFG_PVT_IOCTL  (SIOCWANDEV)
int
acfg_handle_ioctl(struct net_device *dev, void *data); 

#else
#define acfg_handle_ioctl(dev, data) {}
#endif 
