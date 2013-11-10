
#include "../interface.h"
#include "mch_net.h"
#include "mch_sdo.h"

#include <stdlib.h>
#include <stdio.h>

#define MACHINE_FILE() "mch_net_def.h"
#include "machine_body.h"


static void mch_net_sdo_abort_callback(void *data, uint16_t index, uint8_t subindex, uint32_t abort)
{
	fprintf(stderr, "Error while uploading configuration (SDO index %04x:%02x abort code %04x).\n", index, subindex, abort);
	exit(1);
}


static void mch_net_sdo_write_callback(void *data, uint16_t index, uint8_t subindex)
{
	mch_net_t *machine = (mch_net_t *) data;
	mch_net_handle_event(machine, EV_NET_UPLOAD_COMPLETE);
}


#define ENQUEUE(final, index, subindex, value, size) \
	mch_sdo_queue_write_with_cb( \
		mch_sdo, index, subindex, value, size, \
		final?mch_net_sdo_write_callback:NULL, mch_net_sdo_abort_callback, (void *) mch_net);

/**
 * Enqueue controller configuration for transmission.
 *
 * @param mch_sdo  SDO state machine that owns the queue.
 */
static void mch_net_queue_setup(mch_net_t *mch_net, mch_sdo_t *mch_sdo)
{
	// Setup TPDO1
	ENQUEUE(0, 0x1A00, 0x00, 0x00, 0x01);
	ENQUEUE(0, 0x1A00, 0x01, 0x60410010, 0x04);	// Status word
	ENQUEUE(0, 0x1A00, 0x02, 0x60610008, 0x04);	// Mode of operation
	ENQUEUE(0, 0x1A00, 0x00, 0x02, 0x01);

	ENQUEUE(0, 0x1800, 0x01, 0x40000181, 0x04);
	ENQUEUE(0, 0x1800, 0x02, 0xFF, 0x01);			// Event triggered
	ENQUEUE(0, 0x1800, 0x03, 0x0A, 0x02);			// Inhibit timer
	ENQUEUE(0, 0x1800, 0x05, 0x0A, 0x02);			// Event timer

	// Setup TPDO2
	ENQUEUE(0, 0x1A01, 0x00, 0x00, 0x01);
	ENQUEUE(0, 0x1A01, 0x01, 0x60640020, 0x04);	// Position
	ENQUEUE(0, 0x1A01, 0x02, 0x606C0020, 0x04);	// Velocity
	ENQUEUE(0, 0x1A01, 0x00, 0x02, 0x01);

	ENQUEUE(0, 0x1801, 0x01, 0x40000281, 0x04);
	ENQUEUE(0, 0x1801, 0x02, 0xFF, 0x01);			// On change (should be sync or timer?)
	ENQUEUE(0, 0x1801, 0x03, 0x0A, 0x02);
	ENQUEUE(0, 0x1801, 0x05, 0x0A, 0x02);

	// Setup RPDO2 for IP mode
	ENQUEUE(0, 0x1601, 0x00, 0x00, 0x01);
	ENQUEUE(0, 0x1601, 0x01, 0x60C10120, 0x04);
	ENQUEUE(0, 0x1601, 0x00, 0x01, 0x01);

	ENQUEUE(0, 0x1401, 0x02, 0x01, 0x01);			// Every sync

	// Disable TPDO 3 and 4
	ENQUEUE(0, 0x1A02, 0x00, 0x00, 0x01);
	ENQUEUE(0, 0x1802, 0x01, 0x40000381, 0x04);
	
	ENQUEUE(0, 0x1A03, 0x00, 0x00, 0x01);
	ENQUEUE(1, 0x1803, 0x01, 0x40000481, 0x04);
}


static mch_net_state_t mch_net_next_state_given_event(const mch_net_t *machine, mch_net_event_t event)
{
	//fprintf(stderr, "Received event: %s\n", mch_net_eventname(event));

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

			// This might seem counter-intuitive, but
			// is to make sure the correct configuration 
			// is uploaded before we accept the
			// operational state.
			if(event == EV_NET_OPERATIONAL)
				return ST_NET_ENTERPREOPERATIONAL;
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
			if(event == EV_NET_UPLOAD_COMPLETE)
				return ST_NET_STARTREMOTENODE;
			if(event == EV_NET_STOPPED)
				return ST_NET_UNKNOWN;
			if(event == EV_NET_WATCHDOG_FAILED)
				return ST_NET_UNKNOWN;
			break;

		case ST_NET_OPERATIONAL:
			if(event == EV_NET_INTF_CLOSED)
				return ST_NET_DISABLED;
			if(event == EV_NET_STOPPED)
				return ST_NET_UNKNOWN;
			if(event == EV_NET_PREOPERATIONAL)
				return ST_NET_UNKNOWN;
			if(event == EV_NET_WATCHDOG_FAILED)
				return ST_NET_UNKNOWN;
			break;

		case ST_NET_UPLOADCONFIG:
			if(event == EV_NET_INTF_CLOSED)
				return ST_NET_DISABLED;
			if(event == EV_NET_UPLOAD_COMPLETE)
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


static void mch_net_on_enter(mch_net_t *machine)
{
	switch(machine->state) {
		case ST_NET_DISABLED:
			if(machine->sdos_disabled_handler)
				machine->sdos_disabled_handler(machine, machine->payload);
			break;

		case ST_NET_UNKNOWN:
			if(machine->sdos_disabled_handler)
				machine->sdos_disabled_handler(machine, machine->payload);
			intf_send_nmt_command(machine->interface, NMT_ENTERPREOPERATIONAL);
			break;

		case ST_NET_STOPPED:
			if(machine->sdos_disabled_handler)
				machine->sdos_disabled_handler(machine, machine->payload);

		case ST_NET_STARTREMOTENODE:
			intf_send_nmt_command(machine->interface, NMT_STARTREMOTENODE);
			break;

		case ST_NET_ENTERPREOPERATIONAL:
			intf_send_nmt_command(machine->interface, NMT_ENTERPREOPERATIONAL);
			break;

		case ST_NET_UPLOADCONFIG:
			mch_net_queue_setup(machine, machine->mch_sdo);
			break;

		case ST_NET_OPERATIONAL:
			if(machine->sdos_enabled_handler)
				machine->sdos_enabled_handler(machine, machine->payload);
			if(machine->enter_operational_handler)
				machine->enter_operational_handler(machine, machine->payload);
			break;

		case ST_NET_PREOPERATIONAL:
			if(machine->sdos_enabled_handler)
				machine->sdos_enabled_handler(machine, machine->payload);
			mch_net_queue_setup(machine, machine->mch_sdo);
			break;

		default:
			break;
	}
}


static void mch_net_on_exit(mch_net_t *machine)
{
	switch(machine->state) {
		case ST_NET_OPERATIONAL:
			if(machine->leave_operational_handler)
				machine->leave_operational_handler(machine, machine->payload);
			break;

		default:
			break;
	}
}
