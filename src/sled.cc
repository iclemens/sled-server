#include "sled_internal.h"

#include <stdlib.h>
#include <stdio.h>


/** ******************
 * Callback functions
 ****************** **/


/**
 * Inform interface state machine that interface has closed.
 */
void intf_on_close(intf_t *intf, void *payload)
{
	sled_t *sled = (sled_t *) payload;
	mch_intf_handle_event(sled->mch_intf, EV_INTF_CLOSE);
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
	sled_t *sled = (sled_t *) payload;

	switch(state) {
		case 0x04: mch_net_handle_event(sled->mch_net, EV_NET_STOPPED); break;
		case 0x05: mch_net_handle_event(sled->mch_net, EV_NET_OPERATIONAL); break;
		case 0x7F: mch_net_handle_event(sled->mch_net, EV_NET_PREOPERATIONAL); break;
	}
}


/**
 * Handle write (SDO) response.
 */
void intf_on_write_response(intf_t *intf, void *payload, uint16_t index, uint8_t subindex)
{
	sled_t *sled = (sled_t *) payload;
	mch_sdo_handle_event(sled->mch_sdo, EV_SDO_READ_RESPONSE);
}


/**
 * Handle read (SDO) response.
 */
void intf_on_abort_response(intf_t *intf, void *payload, uint16_t index, uint8_t subindex, uint32_t abort)
{
	sled_t *sled = (sled_t *) payload;
	mch_sdo_handle_event(sled->mch_sdo, EV_SDO_ABORT_RESPONSE);
}


/**
 * Handle TPDO.
 */
void intf_on_tpdo(intf_t *intf, void *payload, int pdo, uint8_t *data)
{
	sled_t *sled = (sled_t *) payload;

	if(pdo == 1) {
		uint16_t status = (data[1] << 8) | data[0];
		uint8_t mode = data[2];

		if((status & 0x4F) == 0x40) mch_ds_handle_event(sled->mch_ds, EV_DS_NOT_READY_TO_SWITCH_ON);
		if((status & 0x6F) == 0x21) mch_ds_handle_event(sled->mch_ds, EV_DS_READY_TO_SWITCH_ON);
		if((status & 0x6F) == 0x23) mch_ds_handle_event(sled->mch_ds, EV_DS_SWITCHED_ON);
		if((status & 0x6F) == 0x27) mch_ds_handle_event(sled->mch_ds, EV_DS_OPERATION_ENABLED);
		if((status & 0x4F) == 0x08) mch_ds_handle_event(sled->mch_ds, EV_DS_FAULT);
		if((status & 0x4F) == 0x0F) mch_ds_handle_event(sled->mch_ds, EV_DS_FAULT_REACTION_ACTIVE);
		if((status & 0x6F) == 0x07) mch_ds_handle_event(sled->mch_ds, EV_DS_QUICK_STOP_ACTIVE);

		if((status & 0x10) == 0x10)
			mch_ds_handle_event(sled->mch_ds, EV_DS_VOLTAGE_ENABLED);
		else
			mch_ds_handle_event(sled->mch_ds, EV_DS_VOLTAGE_DISABLED);

		if((status & 0x1000) == 0x1000)
			mch_mp_handle_event(sled->mch_mp, EV_MP_HOMED);
		else
			mch_mp_handle_event(sled->mch_mp, EV_MP_NOTHOMED);

		if(mode == 0x06)
			mch_mp_handle_event(sled->mch_mp, EV_MP_MODE_HOMING);
		if(mode == 0x08)
			mch_mp_handle_event(sled->mch_mp, EV_MP_MODE_PP);

		//printf("Status: %04x\tMode: %02x\n", status, mode);
	}

	if(pdo == 2) {
		int32_t position = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];
		int32_t velocity = (data[7] << 24) | (data[6] << 16) | (data[5] << 8) | data[4];

		//printf("Position: %d\tVelocity: %d\n", position, velocity);
	}
}

// Inform network interface that the interface has opened / closed.
CALLBACK_FUNCTION_EVENT(intf, on_opened, net, EV_NET_INTF_OPENED)
CALLBACK_FUNCTION_EVENT(intf, on_closed, net, EV_NET_INTF_CLOSED)

// Inform SDO machine that SDO transmission is (not) possible.
CALLBACK_FUNCTION_EVENT(net, on_sdos_enabled, sdo, EV_NET_SDO_ENABLED)
CALLBACK_FUNCTION_EVENT(net, on_sdos_disabled, sdo, EV_NET_SDO_DISABLED)

// Notify DS402 that NMT is (in)operational.
CALLBACK_FUNCTION_EVENT(net, on_enter_operational, ds, EV_DS_NET_OPERATIONAL);
CALLBACK_FUNCTION_EVENT(net, on_leave_operational, ds, EV_DS_NET_INOPERATIONAL);

// Inform NMT machine that SDO queue is empty.
CALLBACK_FUNCTION_EVENT(sdo, on_queue_empty, net, EV_NET_SDO_QUEUE_EMPTY);

// Inform MP machine that DS is (in)operational.
CALLBACK_FUNCTION_EVENT(ds, on_operation_enabled, mp, EV_MP_DS_OPERATIONAL);
CALLBACK_FUNCTION_EVENT(ds, on_operation_disabled, mp, EV_MP_DS_INOPERATIONAL);


/**
 * Setup state machines
 */
void setup_state_machines(sled_t *sled)
{
	// Setup state machines
	sled->mch_intf = mch_intf_create(sled->interface);
	sled->mch_sdo = mch_sdo_create(sled->interface);
	sled->mch_net = mch_net_create(sled->interface, sled->mch_sdo);
	sled->mch_ds = mch_ds_create(sled->interface);
	sled->mch_mp = mch_mp_create(sled->interface);

	// Register callback payload
	mch_intf_set_callback_payload(sled->mch_intf, (void *) sled);
	mch_net_set_callback_payload(sled->mch_net, (void *) sled);
	mch_sdo_set_callback_payload(sled->mch_sdo, (void *) sled);
	mch_ds_set_callback_payload(sled->mch_ds, (void *) sled);

	// Register callback functions for state machines.
	REGISTER_CALLBACK(intf, opened);
	REGISTER_CALLBACK(intf, closed);
	REGISTER_CALLBACK(net, sdos_enabled);
	REGISTER_CALLBACK(net, sdos_disabled);
	REGISTER_CALLBACK(net, enter_operational);
	REGISTER_CALLBACK(net, leave_operational);
	REGISTER_CALLBACK(sdo, queue_empty);
	REGISTER_CALLBACK(ds, operation_enabled);
	REGISTER_CALLBACK(ds, operation_disabled);
}


/** *******************************
 * Implementation of API functions
 ******************************* **/


/**
 * Setup sled structures.
 */
sled_t *sled_create()
{
	sled_t *sled = (sled_t *) malloc(sizeof(sled_t));

	// Reset sled profiles
	for(int profile = 0; profile < MAX_PROFILES; profile++)
		sled_profile_clear(sled, profile, false);

	// Create interface
	sled->interface = intf_create(sled->ev_base);
	intf_set_callback_payload(sled->interface, (void *) sled);

	// Register interface callback functions
	intf_set_close_handler(sled->interface, intf_on_close);
	intf_set_nmt_state_handler(sled->interface, intf_on_nmt);
	intf_set_write_resp_handler(sled->interface, intf_on_write_response);
	intf_set_abort_resp_handler(sled->interface, intf_on_abort_response);
	intf_set_tpdo_handler(sled->interface, intf_on_tpdo);

	return sled;
}


void sled_destroy(sled_t **handle)
{
  sled_t *sled = *handle;

  free(*handle);
  *handle = NULL;
}


// Set-points
int sled_rt_new_setpoint(sled_t *handle, double position)
{
  // No can do
  return -1;
}


int sled_rt_get_position(sled_t *handle, double &position)
{
  // Not implemented
  return -1;
}


// Sinusoids
int sled_sinusoid_start(sled_t *handle, double amplitude, double period)
{
  printf("sled_sinusoid_start(%f, %f)\n", amplitude, period);
  return -1;
}


int sled_sinusoid_stop(sled_t *handle)
{
  printf("sled_sinusoid_stop()\n");
  return -1;
}


// Light
int sled_light_set_state(sled_t *handle, bool state)
{
  printf("sled_light_set_state(%s)\n", state?"on":"off");
  return -1;
}

