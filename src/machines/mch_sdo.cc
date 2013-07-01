
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


typedef int mch_sdo_state_t;


struct mch_sdo_t {
	mch_sdo_state_t state;
	intf_t *interface;

	std::queue<sdo_t> sdo_queue;
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
  
	machine->state = 0;	// Not started...
	machine->interface = interface;

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
			if(event == EV_INTF_SDO_ENABLED)
				return ST_SDO_WAITING;
			break;

		case ST_SDO_WAITING:
			if(event == EV_INTF_SDO_DISABLED)
				return ST_SDO_DISABLED;
			if(!machine->sdo_queue.empty())			// << Fix this!
				return ST_SDO_SENDING;
			break;

		case ST_SDO_SENDING:
			if(event == EV_INTF_SDO_DISABLED)
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

	switch(machine->state) {
		case ST_SDO_SENDING:
			// pop from queue and send
			break;
	}
}


void mch_sdo_on_exit(mch_sdo_t *machine)
{
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
		printf("Network machine changed state: %s\n", mch_sdo_statename(machine->state));
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
}



