
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <event2/event.h>
#include <sled.h>
#include <interface.h>

#include <machines/mch_intf.h>
#include <machines/mch_net.h>
#include <machines/mch_sdo.h>
#include <machines/mch_ds.h>


struct machines_t
{
	mch_intf_t *mch_intf;
	mch_net_t *mch_net;
	mch_sdo_t *mch_sdo;
	mch_ds_t *mch_ds;
};


void signal_handler(int signal)
{
	void *array[10];
	size_t size;

	size = backtrace(array, 10);
	backtrace_symbols_fd(array, size, STDERR_FILENO);
	exit(1);
}


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


void intf_on_write_response(intf_t *intf, void *payload, uint16_t index, uint8_t subindex)
{
	machines_t *machines = (machines_t *) payload;
	mch_sdo_handle_event(machines->mch_sdo, EV_SDO_READ_RESPONSE);
}


void intf_on_abort_response(intf_t *intf, void *payload, uint16_t index, uint8_t subindex, uint32_t abort)
{
	machines_t *machines = (machines_t *) payload;
	mch_sdo_handle_event(machines->mch_sdo, EV_SDO_ABORT_RESPONSE);
}


void intf_on_tpdo(intf_t *intf, void *payload, int pdo, uint8_t *data)
{
	machines_t *machines = (machines_t *) payload;

	if(pdo == 1) {
		uint16_t status = (data[1] << 8) | data[0];
		uint8_t mode = data[2];

		if((status & 0x4F) == 0x40) mch_ds_handle_event(machines->mch_ds, EV_DS_NOT_READY_TO_SWITCH_ON);
    if((status & 0x6F) == 0x21) mch_ds_handle_event(machines->mch_ds, EV_DS_READY_TO_SWITCH_ON);
    if((status & 0x6F) == 0x23) mch_ds_handle_event(machines->mch_ds, EV_DS_SWITCHED_ON);
    if((status & 0x6F) == 0x27) mch_ds_handle_event(machines->mch_ds, EV_DS_OPERATION_ENABLED);
    if((status & 0x4F) == 0x08) mch_ds_handle_event(machines->mch_ds, EV_DS_FAULT);
    if((status & 0x4F) == 0x0F) mch_ds_handle_event(machines->mch_ds, EV_DS_FAULT_REACTION_ACTIVE);
    if((status & 0x6F) == 0x07) mch_ds_handle_event(machines->mch_ds, EV_DS_QUICK_STOP_ACTIVE);

    if((status & 0x10) == 0x10) mch_ds_handle_event(machines->mch_ds, EV_DS_VOLTAGE_ENABLED);

		//printf("Status: %04x\tMode: %02x\n", status, mode);
	}

	if(pdo == 2) {
		int32_t position = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];
		int32_t velocity = (data[7] << 24) | (data[6] << 16) | (data[5] << 8) | data[4];

		//printf("Position: %d\tVelocity: %d\n", position, velocity);
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
 * Inform SDO machine that SDO transmission is possible.
 */
void mch_net_on_sdos_enabled(mch_net_t *mch_net, void *payload)
{
	machines_t *machines = (machines_t *) payload;
	mch_sdo_handle_event(machines->mch_sdo, EV_NET_SDO_ENABLED);
}


/**
 * Inform SDO machine that SDOs have been disabled.
 */
void mch_net_on_sdos_disabled(mch_net_t *mch_net, void *payload)
{
	machines_t *machines = (machines_t *) payload;
	mch_sdo_handle_event(machines->mch_sdo, EV_NET_SDO_DISABLED);
}


/**
 * Notify DS402 that NMT is operational.
 */
void mch_net_on_enter_operational(mch_net_t *mch_net, void *payload)
{
	machines_t *machines = (machines_t *) payload;
	mch_ds_handle_event(machines->mch_ds, EV_DS_NET_OPERATIONAL);
}


/**
 * Notify DS402 that NMT is inoperational.
 */
void mch_net_on_leave_operational(mch_net_t *mch_net, void *payload)
{
	machines_t *machines = (machines_t *) payload;
	mch_ds_handle_event(machines->mch_ds, EV_DS_NET_INOPERATIONAL);
}


/**
 * Inform NMT machine that SDO queue is empty.
 */
void mch_sdo_on_queue_empty(mch_sdo_t *mch_sdo, void *payload)
{
	machines_t *machines = (machines_t *) payload;
	mch_net_handle_event(machines->mch_net, EV_NET_SDO_QUEUE_EMPTY);
}



int main(int argc, char *argv[])
{
	signal(SIGSEGV, signal_handler);

	machines_t machines;

	event_config *cfg = event_config_new();
	event_config_require_features(cfg, EV_FEATURE_FDS);
	event_base *ev_base = event_base_new_with_config(cfg);

	if(!ev_base) {
		fprintf(stderr, "Unable to initialize event base\n");
		exit(1);
	}

	// Construct state machines and interface
	intf_t *intf = intf_create(ev_base); 
	machines.mch_intf = mch_intf_create(intf);
	machines.mch_sdo = mch_sdo_create(intf);
	machines.mch_net = mch_net_create(intf, machines.mch_sdo);
	machines.mch_ds = mch_ds_create(intf);

	// Set machines structure as payload
	intf_set_callback_payload(intf, (void *) &machines);
	mch_intf_set_callback_payload(machines.mch_intf, (void *) &machines);
	mch_net_set_callback_payload(machines.mch_net, (void *) &machines);
	mch_sdo_set_callback_payload(machines.mch_sdo, (void *) &machines);
	//mch_ds_set_callback_payload(machines.mch_ds, (void *) &machines);

	// Set callbacks
	intf_set_close_handler(intf, intf_on_close);
	intf_set_nmt_state_handler(intf, intf_on_nmt);
	intf_set_write_resp_handler(intf, intf_on_write_response);
	intf_set_abort_resp_handler(intf, intf_on_abort_response);
	intf_set_tpdo_handler(intf, intf_on_tpdo);

	mch_intf_set_opened_handler(machines.mch_intf, mch_intf_on_open);
	mch_intf_set_closed_handler(machines.mch_intf, mch_intf_on_close);
	mch_net_set_sdos_enabled_handler(machines.mch_net, mch_net_on_sdos_enabled);
	mch_net_set_sdos_disabled_handler(machines.mch_net, mch_net_on_sdos_disabled);
	mch_net_set_enter_operational_handler(machines.mch_net, mch_net_on_enter_operational);
	mch_net_set_leave_operational_handler(machines.mch_net, mch_net_on_leave_operational);
	mch_sdo_set_queue_empty_handler(machines.mch_sdo, mch_sdo_on_queue_empty);
	
	mch_intf_handle_event(machines.mch_intf, EV_INTF_OPEN);
	event_base_loop(ev_base, 0);
  
	//sled_t *sled = sled_create();
	//sled_destroy(&sled);

	return 0;
}

