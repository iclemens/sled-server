#ifndef __MCH_SDO_H__
#define __MCH_SDO_H__

enum mch_sdo_event_t {
	EV_NET_SDO_DISABLED,		// From net machine (mch_net.cc)
	EV_NET_SDO_ENABLED,			// From net machine (mch_net.cc)

	EV_SDO_ITEM_AVAILABLE,		// Internal event

	EV_SDO_READ_RESPONSE,		// From CANOpen (interface.cc)
	EV_SDO_WRITE_RESPONSE,		// From CANOpen (interface.cc)
	EV_SDO_ABORT_RESPONSE		// From CANOpen (interface.cc)
};

enum mch_sdo_state_t {
	ST_SDO_DISABLED,
	ST_SDO_ERROR,
	ST_SDO_WAITING,
	ST_SDO_SENDING
};

struct mch_sdo_t;

mch_sdo_t *mch_sdo_create(intf_t *interface);
void mch_sdo_destroy(mch_sdo_t **machine);

mch_sdo_state_t mch_sdo_active_state(mch_sdo_t *machine);
void mch_sdo_handle_event(mch_sdo_t *machine, mch_sdo_event_t event);

void mch_sdo_queue_write(mch_sdo_t *machine, uint16_t index, uint8_t subindex, uint32_t value, uint8_t size);
void mch_sdo_queue_read(mch_sdo_t *machine, uint16_t index, uint8_t subindex, uint8_t size);

#endif

