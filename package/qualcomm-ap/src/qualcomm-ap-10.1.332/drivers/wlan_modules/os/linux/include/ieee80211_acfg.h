typedef enum {
	WL_EVENT_TYPE_DISASSOC_AP = 1,
	WL_EVENT_TYPE_DISASSOC_STA,
	WL_EVENT_TYPE_DEAUTH_AP,
	WL_EVENT_TYPE_DEAUTH_STA,
	WL_EVENT_TYPE_ASSOC_AP,
	WL_EVENT_TYPE_ASSOC_STA,
	WL_EVENT_TYPE_AUTH_AP,
	WL_EVENT_TYPE_AUTH_STA,
	WL_EVENT_TYPE_NODE_LEAVE,
	WL_EVENT_TYPE_PUSH_BUTTON,
} acfg_event_type_t;

typedef struct acfg_event_data {
	u_int16_t result;	
	u_int16_t reason;	
	u_int8_t addr[IEEE80211_ADDR_LEN];	
	u_int8_t downlink;
} acfg_event_data_t;

#if UMAC_SUPPORT_ACFG
void acfg_send_event(struct net_device *dev, acfg_event_type_t type,
                    acfg_event_data_t *acfg_event);
#else
#define acfg_send_event(dev, type,acfg_event) {}
#endif

