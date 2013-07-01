
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
};


struct mch_sdo_t {
	mch_sdo_state_t state;
	intf_t *interface;

	std::queue<sdo_t> sdo_queue;

	void *payload;
	mch_sdo_queue_empty_handler_t queue_empty_handler;
};


/**
 * Create a state machine for SDO transmission.
 *
 * @param interface  Interface to use when sending SDOs.
 * @return Instance of SDO state machine.
 */
mch_sdo_t *mch_sdo_create(intf_t *interface)
{
	mch_sdo_t *machine = new mch_sdo_t();
  
	machine->state = ST_SDO_DISABLED;
	machine->interface = interface;
	machine->queue_empty_handler = NULL;

	return machine;
}


/**
 * Tears down SDO state machine.
 *
 * @param machine  SDO state machine to tear-down.
 */
void mch_sdo_destroy(mch_sdo_t **machine)
{
	free(*machine);
	*machine = NULL;
}


/**
 * Query active state.
 *
 * @param machine  SDO state machine to query state from.
 * @return Current state of machine.
 */
mch_sdo_state_t mch_sdo_active_state(mch_sdo_t *machine)
{
	return machine->state;
}


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


const char *mch_sdo_statename(mch_sdo_state_t state)
{
	switch(state) {
		case ST_SDO_ERROR: return "ST_SDO_ERROR";
		case ST_SDO_DISABLED: return "ST_SDO_DISABLED";
		case ST_SDO_SENDING: return "ST_SDO_SENDING";
		case ST_SDO_WAITING: return "ST_SDO_WAITING";
	}
  
	return "Invalid state";
}


void mch_sdo_handle_event(mch_sdo_t *machine, mch_sdo_event_t event)
{
	mch_sdo_state_t next_state = mch_sdo_next_state_given_event(machine, event);
  
	if(!(machine->state == next_state)) {
		mch_sdo_on_exit(machine);
		machine->state = next_state;
		//printf("SDO machine changed state: %s\n", mch_sdo_statename(machine->state));
		mch_sdo_on_enter(machine);
	}
}


/**
 * Enqueue a write request SDO.
 */
void mch_sdo_queue_write(mch_sdo_t *machine, uint16_t index, uint8_t subindex, uint32_t value, uint8_t size)
{
	sdo_t sdo;
	sdo.is_write = true;
	sdo.index = index;
	sdo.subindex = subindex;
	sdo.value = value;
	sdo.size = size;
	machine->sdo_queue.push(sdo);

	mch_sdo_handle_event(machine, EV_SDO_ITEM_AVAILABLE);
}


/**
 * Enqueue a read request SDO.
 */
void mch_sdo_queue_read(mch_sdo_t *machine, uint16_t index, uint8_t subindex, uint8_t size)
{
	sdo_t sdo;
	sdo.is_write = false;
	sdo.index = index;
	sdo.subindex = subindex;
	sdo.value = 0;
	sdo.size = size;
	machine->sdo_queue.push(sdo);

	mch_sdo_handle_event(machine, EV_SDO_ITEM_AVAILABLE);
}


void mch_sdo_set_callback_payload(mch_sdo_t *mch_sdo, void *payload)
{
	mch_sdo->payload = payload;
}


void mch_sdo_set_queue_empty_handler(mch_sdo_t *mch_sdo, mch_sdo_queue_empty_handler_t handler)
{
	mch_sdo->queue_empty_handler = handler;
}

