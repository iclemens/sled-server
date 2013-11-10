
#include <stdlib.h>
#include <stdio.h>

#include "../interface.h"
#include "mch_mp.h"

#define MACHINE_FILE() "mch_mp_def.h"
#include "machine_body.h"


static void mch_mp_send_mode_switch(mch_mp_t *machine, uint8_t mode)
{
	mch_sdo_queue_write(machine->mch_sdo, 0x6060, 0x00, mode, 0x01);
}


static void mch_mp_send_control_word(mch_mp_t *machine, uint16_t control_word)
{
	mch_sdo_queue_write(machine->mch_sdo, OB_CONTROL_WORD, 0x00, control_word, 0x02);
}


static mch_mp_state_t mch_mp_next_state_given_event(const mch_mp_t *machine, mch_mp_event_t event)
{
	if(!machine->state == ST_MP_DISABLED && event == EV_MP_DS_INOPERATIONAL)
		return ST_MP_DISABLED;

	switch(machine->state) {
		case ST_MP_DISABLED:
			if(event == EV_MP_DS_OPERATIONAL)
				return ST_MP_SWITCH_MODE_HOMING;
			break;

		case ST_MP_SWITCH_MODE_HOMING:
			if(event == EV_MP_MODE_HOMING)
				return ST_MP_HH_UNKNOWN;
			break;

		case ST_MP_HH_UNKNOWN:
			if(event == EV_MP_NOTHOMED)
				return ST_MP_HH_HOMING;
			if(event == EV_MP_HOMED)
				return ST_MP_SWITCH_MODE_PP;
			break;

		case ST_MP_SWITCH_MODE_PP:
			if(event == EV_MP_MODE_PP)
				return ST_MP_PP_IDLE;
			break;

		case ST_MP_HH_HOMING:
			if(event == EV_MP_HOMED)
				return ST_MP_SWITCH_MODE_PP;
			break;

		case ST_MP_PP_IDLE:
			if(event == EV_MP_SETPOINT_SET)
				return ST_MP_PP_SP_NACK;
			break;

		case ST_MP_PP_SP_NACK:
			if(event == EV_MP_SETPOINT_ACK)
				return ST_MP_PP_SP_ACK;
			break;

		case ST_MP_PP_SP_ACK:
			if(event == EV_MP_SETPOINT_NACK)
				return ST_MP_PP_IDLE;
			break;
	}

	return machine->state;
}


static void mch_mp_on_enter(mch_mp_t *machine)
{
	switch(machine->state) {
		case ST_MP_SWITCH_MODE_HOMING:
			mch_mp_send_mode_switch(machine, 0x06);
			break;

		case ST_MP_SWITCH_MODE_PP:
			mch_mp_send_mode_switch(machine, 0x01);
			break;

		case ST_MP_HH_HOMING:
			// Set new_setpoint bit to initiate homing sequence.
			mch_mp_send_control_word(machine, 0x1F | 0x20);
			break;

		case ST_MP_PP_SP_ACK:
			// Setpoint has been acknowledged, reset new_setpoint.
			mch_mp_send_control_word(machine, 0x0F | 0x20);
			break;

		default:
			break;
	}
}


static void mch_mp_on_exit(mch_mp_t *machine)
{
	switch(machine->state) {
		case ST_MP_HH_HOMING:
			// Reset new_setpoint_bit when homing is complete.
			mch_mp_send_control_word(machine, 0x0F | 0x20);
			break;

		default:
			break;
	}
}
