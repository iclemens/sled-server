
#include "../interface.h"
#include "mch_net.h"
#include "mch_sdo.h"

#include <stdlib.h>
#include <stdio.h>

struct mch_net_t {
	mch_net_state_t state;  
	intf_t *interface;
};


mch_net_t *mch_net_create(intf_t *interface)
{
	mch_net_t *machine = (mch_net_t *) malloc(sizeof(mch_net_t));
  
	if(machine == NULL)
		return NULL;
  
	machine->state = ST_NET_DISABLED;  
	machine->interface = interface;

	return machine;
}


void mch_net_destroy(mch_net_t **machine)
{
	free(*machine);
	*machine = NULL;
}


/**
 * Enqueue controller configuration for transmission.
 *
 * @param mch_sdo  SDO state machine that owns the queue.
 */
void mch_net_queue_setup(mch_sdo_t *mch_sdo)
{
	// Setup TPDO1
	mch_sdo_queue_write(mch_sdo, 0x1A00, 0x00, 0x00, 0x01);
	mch_sdo_queue_write(mch_sdo, 0x1A00, 0x01, 0x60410010, 0x04);	// Status word
	mch_sdo_queue_write(mch_sdo, 0x1A00, 0x02, 0x60610008, 0x04);	// Mode of operation
	mch_sdo_queue_write(mch_sdo, 0x1A00, 0x00, 0x02, 0x01);

	mch_sdo_queue_write(mch_sdo, 0x1800, 0x01, 0x40000181, 0x04);
	mch_sdo_queue_write(mch_sdo, 0x1800, 0x02, 0xFF, 0x01);			// Event triggered
	mch_sdo_queue_write(mch_sdo, 0x1800, 0x03, 0x0A, 0x02);			// Inhibit timer
	mch_sdo_queue_write(mch_sdo, 0x1800, 0x05, 0x0A, 0x02);			// Event timer

	// Setup TPDO2
	mch_sdo_queue_write(mch_sdo, 0x1A01, 0x00, 0x00, 0x01);
	mch_sdo_queue_write(mch_sdo, 0x1A01, 0x01, 0x60640020, 0x04);	// Position
	mch_sdo_queue_write(mch_sdo, 0x1A01, 0x02, 0x606C0020, 0x04);	// Velocity
	mch_sdo_queue_write(mch_sdo, 0x1A01, 0x00, 0x02, 0x01);

	mch_sdo_queue_write(mch_sdo, 0x1801, 0x01, 0x40000281, 0x04);
	mch_sdo_queue_write(mch_sdo, 0x1801, 0x02, 0xFF, 0x01);			// On change (should be sync or timer?)

	// Setup RPDO2 for IP mode
	mch_sdo_queue_write(mch_sdo, 0x1601, 0x00, 0x00, 0x01);
	mch_sdo_queue_write(mch_sdo, 0x1601, 0x01, 0x60C10120, 0x04);
	mch_sdo_queue_write(mch_sdo, 0x1601, 0x00, 0x01, 0x01);

	mch_sdo_queue_write(mch_sdo, 0x1401, 0x02, 0x01, 0x01);			// Every sync

	// Disable TPDO 3 and 4
	mch_sdo_queue_write(mch_sdo, 0x1A02, 0x00, 0x00, 0x01);
	mch_sdo_queue_write(mch_sdo, 0x1802, 0x01, 0x40000381, 0x04);
	
	mch_sdo_queue_write(mch_sdo, 0x1A03, 0x00, 0x00, 0x01);
	mch_sdo_queue_write(mch_sdo, 0x1803, 0x01, 0x40000481, 0x04);
}


mch_net_state_t mch_net_active_state(mch_net_t *machine)
{
	return machine->state;
}


mch_net_state_t mch_net_next_state_given_event(mch_net_t *machine, mch_net_event_t event)
{
	switch(machine->state) {
		case ST_NET_DISABLED:
			if(event == EV_NET_INTF_OPENED)
				return ST_NET_UNKNOWN;
			break;

		case ST_NET_UNKNOWN:
			if(event == EV_NET_STOPPED)
				return ST_NET_STOPPED;
			if(event == EV_NET_PREOPERATIONAL)
				return ST_NET_PREOPERATIONAL;
			if(event == EV_NET_OPERATIONAL)
				return ST_NET_OPERATIONAL;
			if(event == EV_NET_INTF_CLOSED)
				return ST_NET_DISABLED;
			break;	

		case ST_NET_STOPPED:
			if(event == EV_NET_INTF_CLOSED)
				return ST_NET_DISABLED;
			break;

		case ST_NET_PREOPERATIONAL:
			if(event == EV_NET_INTF_CLOSED)
				return ST_NET_DISABLED;
			break;

		case ST_NET_OPERATIONAL:
			if(event == EV_NET_INTF_CLOSED)
				return ST_NET_DISABLED;
			break;

		case ST_NET_UPLOADCONFIG:
			if(event == EV_NET_INTF_CLOSED)
				return ST_NET_DISABLED;
			if(event == EV_NET_SDO_QUEUE_EMPTY)
				return ST_NET_STARTREMOTENODE;
			break;

		case ST_NET_STARTREMOTENODE:
			if(event == EV_NET_INTF_CLOSED)
				return ST_NET_DISABLED;
			if(event == EV_NET_OPERATIONAL)
				return ST_NET_OPERATIONAL;
			break;
      
		case ST_NET_ENTERPREOPERATIONAL:
			if(event == EV_NET_INTF_CLOSED)
				return ST_NET_DISABLED;
			if(event == EV_NET_PREOPERATIONAL)
				return ST_NET_PREOPERATIONAL;
			break;
	}
  
	return machine->state;
}


void mch_net_on_enter(mch_net_t *machine)
{
	switch(machine->state) {
		case ST_NET_DISABLED:
			break;

		case ST_NET_STARTREMOTENODE:
			intf_send_nmt_command(machine->interface, NMT_STARTREMOTENODE);
			break;

		case ST_NET_ENTERPREOPERATIONAL:
			intf_send_nmt_command(machine->interface, NMT_ENTERPREOPERATIONAL);
			break;

		case ST_NET_UPLOADCONFIG:
			//mch_net_queue_setup(...);
			break;
	}
}


void mch_net_on_exit(mch_net_t *machine)
{
}


const char *mch_net_statename(mch_net_state_t state)
{
	switch(state) {
		case ST_NET_DISABLED: return "ST_NET_DISABLED";
		case ST_NET_UNKNOWN: return "ST_NET_UNKNOWN";
    
		case ST_NET_STOPPED: return "ST_NET_STOPPED";
		case ST_NET_PREOPERATIONAL: return "ST_NET_PREOPERATIONAL";
		case ST_NET_OPERATIONAL: return "ST_NET_OPERATIONAL";
    
		case ST_NET_ENTERPREOPERATIONAL: return "ST_NET_ENTERPREOPERATIONAL";
		case ST_NET_STARTREMOTENODE: return "ST_NET_STARTREMOTENODE";
	}
  
	return "Invalid state";
}


void mch_net_handle_event(mch_net_t *machine, mch_net_event_t event)
{
	mch_net_state_t next_state = mch_net_next_state_given_event(machine, event);
  
	if(!(machine->state == next_state)) {
		mch_net_on_exit(machine);
		machine->state = next_state;
		printf("Network machine changed state: %s\n", mch_net_statename(machine->state));
		mch_net_on_enter(machine);
	}
}

