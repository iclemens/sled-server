
#include "../interface.h"

#include "mch_intf.h"

#include <stdlib.h>
#include <stdio.h>

#define MACHINE_FILE() "mch_intf_def.h"
#include "machine_body.h"


static mch_intf_state_t mch_intf_next_state_given_event(const mch_intf_t *machine, mch_intf_event_t event)
{
  switch(machine->state) {
    case ST_INTF_CLOSED:      
      if(event == EV_INTF_OPEN)
				return ST_INTF_OPENING;      
      break;
      
    case ST_INTF_OPENING:
      if(event == EV_INTF_OPENED)
				return ST_INTF_OPENED;
      break;
    
    case ST_INTF_CLOSING:
      if(event == EV_INTF_CLOSED)
				return ST_INTF_CLOSED;
      break;

    case ST_INTF_OPENED:
      if(event == EV_INTF_CLOSE)
				return ST_INTF_CLOSING;
      break;      
  }
  
  return machine->state;
}


static void mch_intf_on_enter(mch_intf_t *machine)
{
	switch(machine->state) {
		case ST_INTF_OPENING:
			if(intf_open(machine->interface) != 0)
				mch_intf_handle_event(machine, EV_INTF_CLOSED);
			else
				mch_intf_handle_event(machine, EV_INTF_OPENED);
			break;
      
		case ST_INTF_CLOSING:
			intf_close(machine->interface);
			mch_intf_handle_event(machine, EV_INTF_CLOSED);
			break;

		case ST_INTF_OPENED:
			if(machine->opened_handler)
				machine->opened_handler(machine, machine->payload);
			break;

		case ST_INTF_CLOSED:
			if(machine->closed_handler)
				machine->closed_handler(machine, machine->payload);
			break;
	 }
}


static void mch_intf_on_exit(mch_intf_t *machine)
{
}

