
#include <stdlib.h>
#include <stdio.h>

#include "../interface.h"
#include "mch_ds.h"

#define MACHINE_FILE() "mch_ds_def.h"
#include "machine_body.h"


void mch_ds_send_control_word(mch_ds_t *machine, uint16_t control_word)
{
	mch_sdo_queue_write(machine->mch_sdo, 0x6040, 0x00, control_word, 0x02);
}


mch_ds_state_t mch_ds_next_state_given_event(mch_ds_t *machine, mch_ds_event_t event)
{
	if(!machine->state == ST_DS_DISABLED && event == EV_DS_NET_INOPERATIONAL)
		return ST_DS_DISABLED;

	switch(machine->state) {
		case ST_DS_DISABLED:
			if(event == EV_DS_NET_OPERATIONAL)
				return ST_DS_UNKNOWN;
			break;

		case ST_DS_UNKNOWN:
			if(event == EV_DS_NOT_READY_TO_SWITCH_ON)
				return ST_DS_SWITCH_ON_DISABLED;
			if(event == EV_DS_READY_TO_SWITCH_ON)
				return ST_DS_READY_TO_SWITCH_ON;
			break;

		case ST_DS_SWITCH_ON_DISABLED:
			if(event == EV_DS_VOLTAGE_ENABLED)
				return ST_DS_PREPARE_SWITCH_ON;
			break;

		case ST_DS_PREPARE_SWITCH_ON:
			if(event == EV_DS_READY_TO_SWITCH_ON)
				return ST_DS_READY_TO_SWITCH_ON;
			break;

		case ST_DS_READY_TO_SWITCH_ON:
			if(event == EV_DS_VOLTAGE_ENABLED)
				return ST_DS_SWITCH_ON;
			break;

		case ST_DS_SWITCH_ON:
			if(event == EV_DS_SWITCHED_ON)
				return ST_DS_SWITCHED_ON;
			if(event == EV_DS_VOLTAGE_DISABLED)
				return ST_DS_SHUTDOWN;
			break;

		case ST_DS_SHUTDOWN:
			if(event == EV_DS_NOT_READY_TO_SWITCH_ON)
				return ST_DS_SWITCH_ON_DISABLED;
			if(event == EV_DS_READY_TO_SWITCH_ON)
				return ST_DS_READY_TO_SWITCH_ON;
			break;

		case ST_DS_SWITCHED_ON:
			if(event == EV_DS_VOLTAGE_ENABLED)
				return ST_DS_ENABLE_OPERATION;
			if(event == EV_DS_VOLTAGE_DISABLED)
				return ST_DS_SHUTDOWN;
			break;

		case ST_DS_DISABLE_OPERATION:
			if(event == EV_DS_SWITCHED_ON)
				return ST_DS_SWITCHED_ON;
			break;

		case ST_DS_ENABLE_OPERATION:
			if(event == EV_DS_OPERATION_ENABLED)
				return ST_DS_OPERATION_ENABLED;
			break;

		case ST_DS_OPERATION_ENABLED:
			if(event == EV_DS_VOLTAGE_DISABLED)
				return ST_DS_DISABLE_OPERATION;
			break;
	}

	return machine->state;
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

		case ST_DS_OPERATION_ENABLED:
			if(machine->operation_enabled_handler)
				machine->operation_enabled_handler(machine, machine->payload);
			break;
	}
}


void mch_ds_on_exit(mch_ds_t *machine)
{
	switch(machine->state) {
		case ST_DS_OPERATION_ENABLED:
			if(machine->operation_disabled_handler)
				machine->operation_disabled_handler(machine, machine->payload);
			break;
	}
}
