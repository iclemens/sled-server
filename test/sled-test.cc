
#include <event2/event.h>
#include <sled.h>
#include <interface.h>

#include <machines/mch_intf.h>
#include <machines/mch_net.h>
#include <machines/mch_sdo.h>


struct machines_t
{
	mch_intf_t *mch_intf;
	mch_net_t *mch_net;
	mch_sdo_t *mch_sdo;
};


void intf_on_close(intf_t *intf, void *payload)
{
	machines_t *machines = (machines_t *) payload;
	mch_intf_handle_event(machines->mch_intf, EV_INTF_CLOSE);
}


/**
 * Handle network management state notification.
 *
 * @param intf  Interface that changed state
 * @param payload  Void pointer to state machine struct
 * @param state  New state
 */
void intf_on_nmt(intf_t *intf, void *payload, uint8_t state)
{
	machines_t *machines = (machines_t *) payload;

	switch(state) {
		case 0x04: mch_net_handle_event(machines->mch_net, EV_NET_STOPPED); break;
		case 0x05: mch_net_handle_event(machines->mch_net, EV_NET_OPERATIONAL); break;
		case 0x7F: mch_net_handle_event(machines->mch_net, EV_NET_PREOPERATIONAL); break;
	} 
}


/**
 * Inform network interface that the interface has opened.
 */
void mch_intf_on_open(mch_intf_t *mch_intf, void *payload)
{
	machines_t *machines = (machines_t *) payload;
	mch_net_handle_event(machines->mch_net, EV_NET_INTF_OPENED);
}


/**
 * Inform network interface that the interface has closed.
 */
void mch_intf_on_close(mch_intf_t *mch_intf, void *payload)
{
	machines_t *machines = (machines_t *) payload;
	mch_net_handle_event(machines->mch_net, EV_NET_INTF_CLOSED);
}


/**
 * Inform NMT machine that SDO queue is empty.
 */
void mch_sdo_on_queue_empty(mch_sdo_t *mch_sdo, void *payload)
{
	machines_t *machines = (machines_t *) payload;
	mch_net_handle_event(machines->mch_net, EV_NET_SDO_QUEUE_EMPTY);
}


/*  EV_NET_INTF_OPENED,	// Our parent state machine opened!
  EV_NET_INTF_CLOSED,	// Our parent state machine closed!*/ 


int main(int argc, char *argv[])
{
	machines_t machines;
	event_base *ev_base = event_base_new();

	// Construct state machines and interface
	intf_t *intf = intf_create(ev_base); 
	machines.mch_intf = mch_intf_create(intf);
	machines.mch_sdo = mch_sdo_create(intf);
	machines.mch_net = mch_net_create(intf, machines.mch_sdo);

	// Set machines structure as payload
	intf_set_callback_payload(intf, (void *) &machines);
	mch_intf_set_callback_payload(machines.mch_intf, (void *) &machines);
	mch_sdo_set_callback_payload(machines.mch_sdo, (void *) &machines);

	// Set callbacks
	intf_set_close_handler(intf, intf_on_close);
	intf_set_nmt_state_handler(intf, intf_on_nmt);
	mch_intf_set_opened_handler(machines.mch_intf, mch_intf_on_open);
	mch_intf_set_closed_handler(machines.mch_intf, mch_intf_on_close);
	mch_sdo_set_queue_empty_handler(machines.mch_sdo, mch_sdo_on_queue_empty);

	
	mch_intf_handle_event(machines.mch_intf, EV_INTF_OPEN);
	event_base_loop(ev_base, 0);
  
	//sled_t *sled = sled_create();
	//sled_destroy(&sled);

	return 0;
}
