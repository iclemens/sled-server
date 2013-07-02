#ifndef __MCH_SDO_H__
#define __MCH_SDO_H__

#undef PREFIX

// Define machine prefix
#define PREFIX mch_sdo

struct mch_sdo_t;

#include "machine_header.h"
#include "mch_sdo_def.h"

void mch_sdo_queue_write(mch_sdo_t *machine, uint16_t index, uint8_t subindex, uint32_t value, uint8_t size);
void mch_sdo_queue_read(mch_sdo_t *machine, uint16_t index, uint8_t subindex, uint8_t size);

#endif

