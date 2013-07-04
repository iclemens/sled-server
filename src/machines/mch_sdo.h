#ifndef __MCH_SDO_H__
#define __MCH_SDO_H__

#undef PREFIX

// Define machine prefix
#define PREFIX mch_sdo

typedef void(*sdo_abort_callback_t)(void *data, uint16_t index, uint8_t subindex, uint32_t code);
typedef void(*sdo_write_callback_t)(void *data, uint16_t index, uint8_t subindex);
typedef void(*sdo_read_callback_t)(void *data, uint16_t index, uint8_t subindex, uint32_t value);

#include "machine_header.h"
#include "mch_sdo_def.h"

void mch_sdo_queue_write(mch_sdo_t *machine, uint16_t index, uint8_t subindex, uint32_t value, uint8_t size);
void mch_sdo_queue_read(mch_sdo_t *machine, uint16_t index, uint8_t subindex, uint8_t size);

void mch_sdo_queue_write_with_cb(mch_sdo_t *machine, 
	uint16_t index, uint8_t subindex, uint32_t value, uint8_t size,
	sdo_write_callback_t write_callback, sdo_abort_callback_t abort_callback, void *data);

void mch_sdo_queue_read_with_cb(mch_sdo_t *machine, 
  uint16_t index, uint8_t subindex, uint32_t value, uint8_t size,
  sdo_read_callback_t read_callback, sdo_abort_callback_t abort_callback, void *data);

#endif

