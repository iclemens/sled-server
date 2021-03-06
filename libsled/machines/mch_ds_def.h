#include "machine_empty.h"

#define INITIAL_STATE ST_DS_DISABLED

BEGIN_EVENTS
	EVENT(EV_DS_NET_OPERATIONAL)	// From NMT machine (mch_net.cc)
	EVENT(EV_DS_NET_INOPERATIONAL)	// From NMT machine (mch_net.cc)

	EVENT(EV_DS_NOT_READY_TO_SWITCH_ON)
	EVENT(EV_DS_READY_TO_SWITCH_ON)
	EVENT(EV_DS_SWITCHED_ON)
	EVENT(EV_DS_OPERATION_ENABLED)
	EVENT(EV_DS_FAULT)
	EVENT(EV_DS_FAULT_REACTION_ACTIVE)
	EVENT(EV_DS_QUICK_STOP_ACTIVE)

	EVENT(EV_DS_VOLTAGE_ENABLED)
	EVENT(EV_DS_VOLTAGE_DISABLED)
END_EVENTS

BEGIN_STATES
	STATE(ST_DS_DISABLED)
	STATE(ST_DS_UNKNOWN)

	STATE(ST_DS_FAULT)
	STATE(ST_DS_CLEARING_FAULT)

	STATE(ST_DS_READY_TO_SWITCH_ON)
	STATE(ST_DS_SWITCH_ON)
	STATE(ST_DS_SWITCHED_ON)
	STATE(ST_DS_PREPARE_SWITCH_ON)
	STATE(ST_DS_SHUTDOWN)
	STATE(ST_DS_SWITCH_ON_DISABLED)
	STATE(ST_DS_ENABLE_OPERATION)
	STATE(ST_DS_DISABLE_OPERATION)
	STATE(ST_DS_OPERATION_ENABLED)
END_STATES

BEGIN_FIELDS
	FIELD(intf_t *, interface)
	FIELD(mch_sdo_t *, mch_sdo)
END_FIELDS

BEGIN_CALLBACKS
	CALLBACK(operation_enabled)
	CALLBACK(operation_disabled)
END_CALLBACKS

GENERATE_DEFAULT_FUNCTIONS
