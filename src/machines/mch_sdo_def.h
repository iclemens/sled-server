#include "machine_empty.h"

#define INITIAL_STATE ST_SDO_DISABLED

BEGIN_EVENTS
	EVENT(EV_NET_SDO_DISABLED)		// From net machine (mch_net.cc)
	EVENT(EV_NET_SDO_ENABLED)		// From net machine (mch_net.cc)

	EVENT(EV_SDO_ITEM_AVAILABLE)	// Internal event

	EVENT(EV_SDO_READ_RESPONSE)		// From CANOpen (interface.cc)
	EVENT(EV_SDO_WRITE_RESPONSE)	// From CANOpen (interface.cc)
	EVENT(EV_SDO_ABORT_RESPONSE)	// From CANOpen (interface.cc)
END_EVENTS

BEGIN_STATES
	STATE(ST_SDO_DISABLED)
	STATE(ST_SDO_ERROR)
	STATE(ST_SDO_WAITING)
	STATE(ST_SDO_SENDING)
END_STATES

BEGIN_CALLBACKS
	CALLBACK(queue_empty)
END_CALLBACKS

BEGIN_FIELDS
	FIELD(intf_t *, interface)

	FIELD_DECL(std::queue<sdo_t *>, sdo_queue)
	FIELD_DECL(sdo_t *, sdo_active)

	FIELD_INIT(sdo_active, NULL)
END_FIELDS

GENERATE_DEFAULT_FUNCTIONS
