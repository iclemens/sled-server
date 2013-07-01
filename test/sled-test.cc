
#include <event2/event.h>
#include <sled.h>
#include <interface.h>

#include <machines/mch_intf.h>


struct machines_t
{
	mch_intf_t *mch_intf;
	mch_net_t *mch_net;
};


void intf_on_close(intf_t *intf, void *payload)
{
	machines_t *machines = (machines_t *) payload;
	mch_intf_handle_event(machines->mch_intf, EV_INTF_CLOSE);
}


void intf_on_nmt(intf_t *intf, void *payload, uint8_t state)
{
	machines_t *machines = (machines_t *) payload;

	switch(state) {
		0x04: mch_intf_handle_event(machines->mch_net, EV_NET_STOPPED); break;
		0x05: mch_intf_handle_event(machines->mch_net, EV_NET_OPERATIONAL); break;
		0x7F: mch_intf_handle_event(machines->mch_net, EV_NET_PREOPERATIONAL); break;
	} 
}


/*  EV_NET_INTF_OPENED,	// Our parent state machine opened!
  EV_NET_INTF_CLOSED,	// Our parent state machine closed!*/ 


int main(int argc, char *argv[])
{
	machines_t machines;

	event_base *ev_base = event_base_new();
  
	intf_t *intf = intf_create(ev_base);
	intf_set_close_handler(intf, intf_on_close);
	intf_set_nmt_state_handler(intf, intf_on_nmt)
  
	machines.mch_intf = mch_intf_create(intf);
	machines.mch_net = mch_net_create(intf);
  
	intf_set_callback_payload(intf, (void *) &machines);

	mch_intf_handle_event(mch_intf, EV_INTF_OPEN);

	event_base_loop(ev_base, 0);
  
	//sled_t *sled = sled_create();
	//sled_destroy(&sled);

	return 0;
}
