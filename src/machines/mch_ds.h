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
	EV_DS_QUICK_STOP_ACTIVE,

	EV_DS_HOMED,
	EV_DS_NOTHOMED,
	EV_DS_VOLTAGE_ENABLED,
	EV_DS_VOLTAGE_DISABLED
};


enum mch_ds_state_t {
	ST_DS_DISABLED,
	ST_DS_UNKNOWN,

	ST_DS_FAULT,
	ST_DS_CLEARING_FAULT,

	ST_DS_READY_TO_SWITCH_ON,
	ST_DS_SWITCH_ON,
	ST_DS_SWITCHED_ON,
	ST_DS_PREPARE_SWITCH_ON,
	ST_DS_SHUTDOWN,
	ST_DS_SWITCH_ON_DISABLED,
	ST_DS_ENABLE_OPERATION,
	ST_DS_DISABLE_OPERATION,
	ST_DS_OPERATION_ENABLED
};

struct mch_ds_t;
struct intf_t;

// Callbacks
typedef void(*mch_ds_operation_enabled_handler_t)(mch_ds_t *mch_ds, void *payload);
typedef void(*mch_ds_operation_disabled_handler_t)(mch_ds_t *mch_ds, void *payload);

mch_ds_t *mch_ds_create(intf_t *interface);
void mch_ds_destroy(mch_ds_t **machine);

mch_ds_state_t mch_ds_active_state(mch_ds_t *machine);
void mch_ds_handle_event(mch_ds_t *machine, mch_ds_event_t event);

// Callback setters
void mch_ds_set_callback_payload(mch_ds_t *mch_ds, void *payload);
void mch_ds_set_operation_enabled_handler(mch_ds_t *mch_ds, mch_ds_operation_enabled_handler_t handler);
void mch_ds_set_operation_disabled_handler(mch_ds_t *mch_ds, mch_ds_operation_disabled_handler_t handler);

#endif
