
#include "../interface.h"
#include "mch_intf.h"

#include <stdlib.h>
#include <stdio.h>

struct mch_intf_t {
  mch_intf_state_t state;  
  intf_t *interface;
};


mch_intf_t *mch_intf_create(intf_t *interface)
{
  mch_intf_t *machine = (mch_intf_t *) malloc(sizeof(mch_intf_t));
  
  if(machine == NULL)
    return NULL;
  
  machine->state = ST_INTF_CLOSED;
  
  machine->interface = interface;
}


void mch_intf_destroy(mch_intf_t **machine)
{
  free(*machine);
  *machine = NULL;
}


mch_intf_state_t mch_intf_active_state(mch_intf_t *machine)
{
  return machine->state;
}


mch_intf_state_t mch_intf_next_state_given_event(mch_intf_t *machine, mch_intf_event_t event)
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


void mch_intf_on_enter(mch_intf_t *machine)
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
  }
}


void mch_intf_on_exit(mch_intf_t *machine)
{
}


const char *mch_intf_statename(mch_intf_state_t state)
{
  switch(state) {
    case ST_INTF_CLOSED: return "ST_INTF_CLOSED";
    case ST_INTF_OPENED: return "ST_INTF_OPENED";
    case ST_INTF_OPENING: return "ST_INTF_OPENING";
    case ST_INTF_CLOSING: return "ST_INTF_CLOSING";
  }
}


void mch_intf_handle_event(mch_intf_t *machine, mch_intf_event_t event)
{
  mch_intf_state_t next_state = mch_intf_next_state_given_event(machine, event);
  
  if(!(machine->state == next_state)) {
    mch_intf_on_exit(machine);
    machine->state = next_state;
    printf("Interface machine changed state: %s\n", mch_intf_statename(machine->state));
    mch_intf_on_enter(machine);
  }
}

