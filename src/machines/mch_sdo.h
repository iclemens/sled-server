#ifndef __MCH_SDO_H__
#define __MCH_SDO_H__

enum mch_sdo_event_t {
	EV_INTF_SDO_DISABLED,
	EV_INTF_SDO_ENABLED,

	EV_SDO_READ_RESPONSE,
	EV_SDO_WRITE_RESPONSE,
	EV_SDO_ABORT_RESPONSE
};

enum mch_intf_state_t {
	ST_SDO_DISABLED,
	ST_SDO_ERROR,
	ST_SDO_WAITING,
	ST_SDO_SENDING
};

struct mch_sdo_t;

void mch_sdo_queue_write(mch_sdo_t *machine, uint16_t index, uint8_t subindex, uint32_t value, uint8_t size);
void mch_sdo_queue_read(mch_sdo_t *machine, uint16_t index, uint8_t subindex, uint8_t size);

#endif

