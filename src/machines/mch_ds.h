#ifndef __MCH_DS_H__
#define __MCH_DS_H__

enum mch_ds_event_t {
	EV_DS_NET_OPERATIONAL,		// From NMT machine (mch_net.cc)
	EV_DS_NET_INOPERATIONAL,	// From NMT machine (mch_net.cc)

	EV_DS_NOT_READY_TO_SWITCH_ON,
	EV_DS_READY_TO_SWITCH_ON,
	EV_DS_SWITCHED_ON,
	EV_DS_OPERATION_ENABLED,
	EV_DS_FAULT,
	EV_DS_FAULT_REACTION_ACTIVE,
	EV_DS_QUICKSTOP_ACTIVE,

	EV_DS_HOMED,
	EV_DS_NOTHOMED,
	EV_DS_VOLTAGE_ENABLED
};


enum mch_ds_state_t {
	ST_DS_DISABLED,

	ST_DS_UNKNOWN
};

struct mch_ds_t;
struct intf_t;

mch_ds_t *mch_ds_create(intf_t *interface);
void mch_ds_destroy(mch_ds_t **machine);

mch_ds_state_t mch_ds_active_state(mch_ds_t *machine);
void mch_ds_handle_event(mch_ds_t *machine, mch_ds_event_t event);

#endif
