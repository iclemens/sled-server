
#include "../interface.h"
#include "mch_sdo.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <queue>


struct sdo_t {
	bool is_write;
	uint16_t index;
	uint8_t subindex;
	uint32_t value;
	uint8_t size;

	void *data;
	sdo_abort_callback_t abort_callback;
	sdo_write_callback_t write_callback;
	sdo_read_callback_t read_callback;
};


#define MACHINE_FILE() "mch_sdo_def.h"
#include "machine_body.h"


const char *mch_sdo_abort_to_message(uint32_t code)
{
	switch(code) {
		case 0x06010000: return "Unsupported access to this object.";
		case 0x06010001: return "Attempted read access to a write-only object.";
		case 0x06010002: return "Attempted write access to a read-only object.";
		case 0x06020000: return "Object does not exist in Object Dictionary.";
		case 0x06040041: return "Object cannot be mapped to a PDO.";
		case 0x06040042: return "Size and number of mapped objects exceed permissible PDO length.";
		case 0x06040043: return "General parameter incompatibility.";
		case 0x06070010: return "Data type incompatible, length of service parameter is incompatible.";
		case 0x06090011: return "Subindex does not exist.";
		case 0x06090030: return "Outside value range for the parameter (only for write access).";
		case 0x06090031: return "Parameter value too high.";
		case 0x06090032: return "Parameter value too low.";
		case 0x08000020: return "Data cannot be transmitted or saved.";
		case 0x08000022: return "Data cannot be transmitted or saved because of device status.";
	}

	return "Unknown abort code.";
}


void mch_sdo_read_callback(void *data, uint16_t index, uint8_t subindex, uint32_t value)
{
	assert(data);

	// Unpack SDO
	mch_sdo_t *machine = (mch_sdo_t *) data;
	sdo_t *sdo = machine->sdo_active;

	assert(sdo);
	assert(!sdo->is_write);
	assert(machine->sdo_active->index == index);
	assert(machine->sdo_active->subindex == subindex);

	// Invoke callback
	if(sdo->read_callback)
		sdo->read_callback(sdo->data, index, subindex, value);

	// Free memory
	delete sdo;
	machine->sdo_active = NULL;

	// Notify state machine
	mch_sdo_handle_event(machine, EV_SDO_READ_RESPONSE);
}


void mch_sdo_write_callback(void *data, uint16_t index, uint8_t subindex)
{
  assert(data);

  // Unpack SDO
  mch_sdo_t *machine = (mch_sdo_t *) data;
  sdo_t *sdo = machine->sdo_active;

  assert(sdo);
	assert(sdo->is_write);
  assert(machine->sdo_active->index == index);
  assert(machine->sdo_active->subindex == subindex);

  // Invoke callback
  if(sdo->write_callback)
    sdo->write_callback(sdo->data, index, subindex);

  // Free memory
  delete sdo;
  machine->sdo_active = NULL;

  // Notify state machine
	mch_sdo_handle_event(machine, EV_SDO_WRITE_RESPONSE);
}


void mch_sdo_abort_callback(void *data, uint16_t index, uint8_t subindex, uint32_t code)
{
  assert(data);

  // Unpack SDO
  mch_sdo_t *machine = (mch_sdo_t *) data;
  sdo_t *sdo = machine->sdo_active;

  assert(sdo);
  assert(machine->sdo_active->index == index);
  assert(machine->sdo_active->subindex == subindex);

	fprintf(stderr, "%s SDO to %04x:%02x was aborted because: %s\n",
		machine->sdo_active->is_write?"Write":"Read", index, subindex,
		mch_sdo_abort_to_message(code));

  // Invoke callback
  if(sdo->abort_callback)
    sdo->abort_callback(sdo->data, index, subindex, code);

  // Free memory
  delete sdo;
  machine->sdo_active = NULL;

  // Notify state machine
	mch_sdo_handle_event(machine, EV_SDO_ABORT_RESPONSE);
}


mch_sdo_state_t mch_sdo_next_state_given_event(const mch_sdo_t *machine, mch_sdo_event_t event)
{
	switch(machine->state) {
		case ST_SDO_DISABLED:
		case ST_SDO_ERROR:
			if(event == EV_NET_SDO_ENABLED)
				return ST_SDO_WAITING;
			break;

		case ST_SDO_WAITING:
			if(event == EV_NET_SDO_DISABLED)
				return ST_SDO_DISABLED;
			if(event == EV_SDO_ITEM_AVAILABLE)
				return ST_SDO_SENDING;
			break;

		case ST_SDO_SENDING:
			if(event == EV_NET_SDO_DISABLED)
				return ST_SDO_DISABLED;
			if(event == EV_SDO_READ_RESPONSE)
				return ST_SDO_WAITING;
			if(event == EV_SDO_WRITE_RESPONSE)
				return ST_SDO_WAITING;
			if(event == EV_SDO_ABORT_RESPONSE)
				return ST_SDO_ERROR;
			break;
	}

	return machine->state;
}


void mch_sdo_send(mch_sdo_t *machine, sdo_t *sdo)
{
	if(sdo->is_write) {
		intf_send_write_req(machine->interface,
			sdo->index, sdo->subindex, sdo->value, sdo->size,
			mch_sdo_write_callback, mch_sdo_abort_callback, (void *) machine);
	} else {
		intf_send_read_req(machine->interface,
			sdo->index, sdo->subindex,
			mch_sdo_read_callback, mch_sdo_abort_callback, (void *) machine);
	}
}


void mch_sdo_on_enter(mch_sdo_t *machine)
{
	switch(machine->state) {
		case ST_SDO_SENDING:
			machine->sdo_active = machine->sdo_queue.front();
			machine->sdo_queue.pop();

			mch_sdo_send(machine, machine->sdo_active);
			break;

		case ST_SDO_WAITING:
			if(!machine->sdo_queue.empty()) {
				mch_sdo_handle_event(machine, EV_SDO_ITEM_AVAILABLE);
			}
			break;

		default:
			break;
	}
}


/**
 * Drop all SDOs in the queue.
 */
void mch_sdo_clear_queue(mch_sdo_t *machine)
{
	while(!machine->sdo_queue.empty()) {
		sdo_t *sdo = machine->sdo_queue.front();
		machine->sdo_queue.pop();

		if(sdo->abort_callback)
			sdo->abort_callback(sdo->data, sdo->index, sdo->subindex, 0);

		delete sdo;
	}
}


void mch_sdo_on_exit(mch_sdo_t *machine)
{
	switch(machine->state) {
		case ST_SDO_DISABLED:
		case ST_SDO_ERROR:
			mch_sdo_clear_queue(machine);
			break;

		case ST_SDO_SENDING:
			break;

		default:
			break;
	}
}


/**
 * Enqueue a write request SDO and register callback.
 *
 * If the SDO is dropped or aborted, the abort_callback is invoked.
 * In case the write succeeds the write_callback is invoked.
 */
void mch_sdo_queue_write_with_cb(mch_sdo_t *machine, uint16_t index, uint8_t subindex, uint32_t value, uint8_t size,
  sdo_write_callback_t write_callback, sdo_abort_callback_t abort_callback, void *data)
{
	sdo_t *sdo = new sdo_t();
	sdo->is_write = true;
	sdo->index = index;
	sdo->subindex = subindex;
	sdo->value = value;
	sdo->size = size;

	sdo->write_callback = write_callback;
	sdo->read_callback = NULL;
	sdo->abort_callback = abort_callback;
	sdo->data = data;

	machine->sdo_queue.push(sdo);
	mch_sdo_handle_event(machine, EV_SDO_ITEM_AVAILABLE);
}


/**
 * Enqueue a write request SDO.
 */
void mch_sdo_queue_write(mch_sdo_t *machine, uint16_t index, uint8_t subindex, uint32_t value, uint8_t size)
{
	mch_sdo_queue_write_with_cb(machine, index, subindex, value, size, NULL, NULL, NULL);
}


/**
 * Enqueue a read request SDO and register callback.
 *
 * If the SDO is dropped or aborted, the abort_callback is invoked.
 * In case the read succeeds the read_callback is invoked.
 */
void mch_sdo_queue_read_with_cb(mch_sdo_t *machine, uint16_t index, uint8_t subindex,
	sdo_read_callback_t read_callback, sdo_abort_callback_t abort_callback, void *data)
{
	sdo_t *sdo = new sdo_t();
	sdo->is_write = false;
	sdo->index = index;
	sdo->subindex = subindex;
	sdo->value = 0;
	sdo->size = 0;

	sdo->write_callback = NULL;
	sdo->read_callback = read_callback;
	sdo->abort_callback = abort_callback;
	sdo->data = NULL;

	machine->sdo_queue.push(sdo);
	mch_sdo_handle_event(machine, EV_SDO_ITEM_AVAILABLE);
}


/**
 * Enqueue read request SDO.
 */
void mch_sdo_queue_read(mch_sdo_t *machine, uint16_t index, uint8_t subindex)
{
	mch_sdo_queue_read_with_cb(machine, index, subindex, NULL, NULL, NULL);
}

