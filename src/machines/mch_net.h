#ifndef __MCH_NET_H__
#define __MCH_NET_H__

enum mch_net_event_t {
	EV_NET_INTF_OPENED,	// Our parent state machine opened!
	EV_NET_INTF_CLOSED,	// Our parent state machine closed!

	EV_NET_SDO_QUEUE_EMPTY,

	EV_NET_STOPPED,
	EV_NET_OPERATIONAL,
	EV_NET_PREOPERATIONAL
};


enum mch_net_state_t {
	ST_NET_DISABLED,

	ST_NET_UNKNOWN,
	ST_NET_STOPPED,
	ST_NET_PREOPERATIONAL,
	ST_NET_OPERATIONAL,

	ST_NET_ENTERPREOPERATIONAL,
	ST_NET_UPLOADCONFIG,
	ST_NET_STARTREMOTENODE  
};

struct mch_net_t;
struct intf_t;

mch_net_t *mch_net_create(intf_t *interface);
void mch_net_destroy(mch_net_t **machine);

mch_net_state_t mch_net_active_state(mch_net_t *machine);
void mch_net_handle_event(mch_net_t *machine, mch_net_event_t event);

#endif
