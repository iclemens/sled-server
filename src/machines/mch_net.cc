
#include "../interface.h"
#include "mch_net.h"

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

		case ST_NET_STARTREMOTENODE:
			if(event == EV_NET_OPERATIONAL)
				return ST_NET_OPERATIONAL;
			break;
      
		case ST_NET_ENTERPREOPERATIONAL:
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

