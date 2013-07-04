
#include "../interface.h"
#include "mch_sdo.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <queue>


struct sdo_t {
	bool is_write;
	uint16_t index;
	uint8_t subindex;
	uint32_t value;
	uint8_t size;

	void *data;
	sdo_abort_callback_t abort_callback;
	sdo_write_callback_t write_callback;
	sdo_read_callback_t read_callback;
};


#define MACHINE_FILE() "mch_sdo_def.h"
#include "machine_body.h"


mch_sdo_state_t mch_sdo_next_state_given_event(mch_sdo_t *machine, mch_sdo_event_t event)
{
	switch(machine->state) {
		case ST_SDO_DISABLED:
		case ST_SDO_ERROR:
			if(event == EV_NET_SDO_ENABLED)
				return ST_SDO_WAITING;
			break;

		case ST_SDO_WAITING:
			if(event == EV_NET_SDO_DISABLED)
				return ST_SDO_DISABLED;
			if(event == EV_SDO_ITEM_AVAILABLE)
				return ST_SDO_SENDING;
			break;

		case ST_SDO_SENDING:
			if(event == EV_NET_SDO_DISABLED)
				return ST_SDO_DISABLED;
			if(event == EV_SDO_READ_RESPONSE)
				return ST_SDO_WAITING;
			if(event == EV_SDO_WRITE_RESPONSE)
				return ST_SDO_WAITING;
			if(event == EV_SDO_ABORT_RESPONSE)
				return ST_SDO_ERROR;
			break;
	}

	return machine->state;
}


void mch_sdo_on_enter(mch_sdo_t *machine)
{
	sdo_t sdo;

	switch(machine->state) {
		case ST_SDO_SENDING:
			sdo = machine->sdo_queue.front();
			if(sdo.is_write)
				intf_send_write_req(machine->interface, sdo.index, sdo.subindex, sdo.value, sdo.size);
			else
				fprintf(stderr, "Ignoring read request...\n");
			machine->sdo_queue.pop();

			break;

		case ST_SDO_WAITING:
			if(!machine->sdo_queue.empty()) {
				mch_sdo_handle_event(machine, EV_SDO_ITEM_AVAILABLE);
			}
			break;
	}
}


void mch_sdo_on_exit(mch_sdo_t *machine)
{
	switch(machine->state) {
		case ST_SDO_DISABLED:
		case ST_SDO_ERROR:
			while(!machine->sdo_queue.empty())
				machine->sdo_queue.pop();
			break;

		case ST_SDO_SENDING:
			if(machine->sdo_queue.empty())
				if(machine->queue_empty_handler)
					machine->queue_empty_handler(machine, machine->payload);
			break;
	}
}


/**
 * Enqueue a write request SDO and register callback.
 *
 * If the SDO is dropped or aborted, the abort_callback is invoked.
 * In case the write succeeds the write_callback is invoked.
 */
void mch_sdo_queue_write_with_cb(mch_sdo_t *machine, uint16_t index, uint8_t subindex, uint32_t value, uint8_t size,
  sdo_write_callback_t write_callback, sdo_abort_callback_t abort_callback, void *data)
{
	sdo_t sdo;
	sdo.is_write = true;
	sdo.index = index;
	sdo.subindex = subindex;
	sdo.value = value;
	sdo.size = size;

	sdo.write_callback = write_callback;
	sdo.read_callback = NULL;
	sdo.abort_callback = abort_callback;
	sdo.data = data;

	machine->sdo_queue.push(sdo);

	mch_sdo_handle_event(machine, EV_SDO_ITEM_AVAILABLE);
}


/**
 * Enqueue a write request SDO.
 */
void mch_sdo_queue_write(mch_sdo_t *machine, uint16_t index, uint8_t subindex, uint32_t value, uint8_t size)
{
	mch_sdo_queue_write_with_cb(machine, index, subindex, value, size, NULL, NULL, NULL);
}


/**
 * Enqueue a read request SDO and register callback.
 *
 * If the SDO is dropped or aborted, the abort_callback is invoked.
 * In case the read succeeds the read_callback is invoked.
 */
void mch_sdo_queue_read_with_cb(mch_sdo_t *machine, uint16_t index, uint8_t subindex, uint8_t size,
	sdo_read_callback_t read_callback, sdo_abort_callback_t abort_callback, void *data)
{
	sdo_t sdo;
	sdo.is_write = false;
	sdo.index = index;
	sdo.subindex = subindex;
	sdo.value = 0;
	sdo.size = size;

	sdo.write_callback = NULL;
	sdo.read_callback = read_callback;
	sdo.abort_callback = abort_callback;
	sdo.data = NULL;

	machine->sdo_queue.push(sdo);

	mch_sdo_handle_event(machine, EV_SDO_ITEM_AVAILABLE);
}


/**
 * Enqueue read request SDO.
 */
void mch_sdo_queue_read(mch_sdo_t *machine, uint16_t index, uint8_t subindex, uint8_t size)
{
	mch_sdo_queue_read_with_cb(machine, index, subindex, size, NULL, NULL, NULL);
}

