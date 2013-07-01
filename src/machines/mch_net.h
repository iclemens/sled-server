#ifndef __MCH_NET_H__
#define __MCH_NET_H__

enum mch_net_event_t {
	EV_NET_INTF_OPENED,		// From interface machine (mch_intf.cc)
	EV_NET_INTF_CLOSED,		// From interface machine (mch_intf.cc)

	EV_NET_SDO_QUEUE_EMPTY, // From SDO machine (mch_sdo.cc)

	EV_NET_STOPPED,			// From CANOpen (interface.cc)
	EV_NET_OPERATIONAL,		// From CANOpen (interface.cc)
	EV_NET_PREOPERATIONAL	// From CANOpen (interface.cc)
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

struct mch_sdo_t;
struct mch_net_t;
struct intf_t;

// Callbacks
typedef void(*mch_net_sdos_enabled_handler_t)(mch_net_t *mch_net, void *payload);
typedef void(*mch_net_sdos_disabled_handler_t)(mch_net_t *mch_net, void *payload);
typedef void(*mch_net_enter_operational_handler_t)(mch_net_t *mch_net, void *payload);
typedef void(*mch_net_leave_operational_handler_t)(mch_net_t *mch_net, void *payload);

mch_net_t *mch_net_create(intf_t *interface, mch_sdo_t *mch_sdo);
void mch_net_destroy(mch_net_t **machine);

mch_net_state_t mch_net_active_state(mch_net_t *machine);
void mch_net_handle_event(mch_net_t *machine, mch_net_event_t event);

#endif
