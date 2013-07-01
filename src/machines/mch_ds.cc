
#include <stdlib.h>
#include <stdio.h>

#include "../interface.h"
#include "mch_ds.h"

struct mch_ds_t {
	mch_ds_state_t state;

	intf_t *interface;
};


/**
 * Creates a state machine that keeps track of DS402 states.
 */
mch_ds_t *mch_ds_create(intf_t *interface)
{
	mch_ds_t *machine = new mch_ds_t();

	machine->interface = interface;

	return machine;
}


/**
 * Destroys the DS402 state machine.
 */
void mch_ds_destroy(mch_ds_t **machine)
{
	free(*machine);
	*machine = NULL;
}


mch_ds_state_t mch_ds_active_state(mch_ds_t *machine)
{
	return machine->state;
}


mch_ds_state_t mch_ds_next_state_given_event(mch_ds_t *machine, mch_ds_event_t event)
{
	switch(machine->state) {
		case ST_DS_DISABLED:
			if(event == EV_DS_NET_OPERATIONAL)
				return ST_DS_UNKNOWN;
			break;
	}
}


void mch_ds_send_control_word(mch_ds_t *machine, uint16_t control_word)
{
	intf_send_write_req(machine->interface, 0x6040, 0x00, control_word, 0x02);
}


void mch_ds_on_enter(mch_ds_t *machine)
{
	switch(machine->state) {
		case ST_DS_DISABLED:
			break;

		case ST_DS_UNKNOWN:
			mch_ds_send_control_word(machine, 0x06); // Shutdown
			break;

		case ST_DS_PREPARE_SWITCH_ON:
			mch_ds_send_control_word(machine, 0x06); // Shutdown
			break;

		case ST_DS_SWITCH_ON:
			mch_ds_send_control_word(machine, 0x07); // Switch on
			break;

		case ST_DS_SHUTDOWN:
			mch_ds_send_control_word(machine, 0x06); // Shutdown
			break;

		case ST_DS_ENABLE_OPERATION:
			mch_ds_send_control_word(machine, 0x0F); // Enable operation
			break;

		case ST_DS_DISABLE_OPERATION:
			mch_ds_send_control_word(machine, 0x07); // Disable operation
			break;
	}
}


void mch_ds_on_exit(mch_ds_t *machine)
{
}


const char *mch_ds_statename(mch_ds_state_t state)
{
	switch(state) {
		case ST_DS_DISABLED: return "ST_DS_DISABLED";
		case ST_DS_UNKNOWN: return "ST_DS_UNKNOWN";
	}

	return "Invalid state";
}


void mch_ds_handle_event(mch_ds_t *machine, mch_ds_event_t event)
{
	mch_ds_state_t next_state = mch_ds_next_state_given_event(machine, event);

	if(!(machine->state == next_state)) {
		mch_ds_on_exit(machine);
		machine->state = next_state;
		printf("DS machine changed state: %s\n", mch_ds_statename(machine->state));
		mch_ds_on_enter(machine);
	}
}

