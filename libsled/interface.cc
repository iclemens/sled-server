
#include "interface.h"
#include "interface_internal.h"

#include <syslog.h>
#include <assert.h>
#include <event2/event.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#include <sys/stat.h>


/**
 * Writes PEAK CAN interface status to system log.
 *
 * @param function  Function where the status message was received.
 * @param status  Status message received.
 */
static void intf_log_status(const char *function, int status)
{
  assert(function);

	if(status & 0x01)
		syslog(LOG_ALERT, "%s() chip-send-buffer full.", function);

	if(status & 0x02)
		syslog(LOG_ALERT, "%s() chip-receive-buffer overrun.", function);

	if(status & 0x04)
		syslog(LOG_WARNING, "%s() bus warning.", function);

	if(status & 0x08)
		syslog(LOG_WARNING, "%s() bus passive.", function);

	if(status & 0x10)
		syslog(LOG_ERR, "%s() bus off.", function);

	if(status & 0x20)
		syslog(LOG_INFO, "%s() receive buffer is empty.", function);

	if(status & 0x40)
		syslog(LOG_ALERT, "%s() receive buffer overrun.", function);

	if(status & 0x80)
		syslog(LOG_ALERT, "%s() send-buffer is full.", function);
}


static void intf_clear_sdo_callbacks(intf_t *intf)
{
	assert(intf);

	intf->sdo_callback_data = NULL;
	intf->read_callback = NULL;
	intf->write_callback = NULL;
	intf->abort_callback = NULL;
}


/**
 * Setup CAN Interface
 *
 * @param ev_base  LibEvent event_base.
 * @return CAN Interface instance.
 */
intf_t *intf_create(event_base *ev_base)
{
	assert(ev_base);

	intf_t *intf = new intf_t();

	intf->ev_base = ev_base;
	intf->handle = 0;
	intf->fd = 0;

	intf->read_event = NULL;

	intf->payload = NULL;

	// Initialize callback functions
	intf->nmt_state_handler = NULL;
	intf->tpdo_handler = NULL;
	intf->close_handler = NULL;

	intf_clear_sdo_callbacks(intf);

  return intf;
}


/**
 * Destroy CAN Interface
 *
 * @param intf  Interface to destroy.
 */
void intf_destroy(intf_t **intf)
{
	intf_close(*intf);
	free(*intf);
	*intf = NULL;
}


/**
 * Open connection to device
 *
 * @param intf  Interface to open.
 * @return 0 on success, -1 on failure.
 */
int intf_open(intf_t *intf)
{
	assert(intf);

	#ifdef WIN32
	return -1;
	#else
	const char *device = "/dev/pcanpci0";

	// Device is already open
	if(intf->handle) {
		return 0;
	}

	// Check whether the device exists
	struct stat buf;
	if(stat(device, &buf) == -1)
	{
		fprintf(stderr, "Error locating device node (%s)\n", device);
		return -1;
	}

	// Device should be a character device
	if(!S_ISCHR(buf.st_mode))
	{
		fprintf(stderr, "Device node is not a character device (%s)\n", device);
		return -1;
	}

	// Try to open interface
	intf->handle = LINUX_CAN_Open(device, O_RDWR);

	if(!intf->handle) {
		fprintf(stderr, "Opening of CAN device failed\n");
		return -1;
	}

	// Initialize interface
	DWORD result = CAN_Init(intf->handle, CAN_BAUD_1M, CAN_INIT_TYPE_ST);

	if(result != CAN_ERR_OK && result != CAN_ERR_QRCVEMPTY)
	{
		fprintf(stderr, "Initializing of CAN device failed\n");
		intf_close(intf);
		return -1;
	}

	// Register handle with libevent and set priority to important
	intf->fd = LINUX_CAN_FileHandle(intf->handle);
	intf->read_event = event_new(intf->ev_base, intf->fd, 
		EV_READ | EV_PERSIST, intf_on_read, (void *) intf);
	event_priority_set(intf->read_event, 0);
	event_add(intf->read_event, NULL);

	return 0;
#endif
}


/**
 * Close the device connection.
 */
int intf_close(intf_t *intf)
{
	// Remove event
	if(intf->read_event) {
		event_del(intf->read_event);
		event_free(intf->read_event);
		intf->read_event = NULL;
	}

	#ifdef WIN32
	#else
	// Close connection
	if(intf->handle) {
		DWORD result = CAN_Close(intf->handle);
		if(result != CAN_ERR_OK) {
			fprintf(stderr, "Closing of CAN device failed\n");
			return -1;
		}

		intf->handle = 0;
		intf->fd = 0;
	}
	#endif

	if(intf->close_handler) {
		intf->close_handler(intf, intf->payload);
	}

	return 0;
}


/**
 * Writes a single message
 */
static int intf_write(const intf_t *intf, can_message_t msg)
{
	assert(intf);

	TPCANMsg cmsg;
	cmsg.ID = msg.id;
	cmsg.MSGTYPE = msg.type;
	cmsg.LEN = msg.len;

	for(int i = 0; i < 8; i++)
		cmsg.DATA[i] = msg.data[i];

	syslog(LOG_DEBUG, "%s() %04x %02x %02x (%02x %02x %02x %02x %02x %02x %02x %02x)\n",
		__FUNCTION__, cmsg.ID, cmsg.MSGTYPE, cmsg.LEN,
		cmsg.DATA[0], cmsg.DATA[1], cmsg.DATA[2], cmsg.DATA[3],
		cmsg.DATA[4], cmsg.DATA[5], cmsg.DATA[6], cmsg.DATA[7]);

	#ifdef WIN32
	DWORD result = -1;
	#else
	DWORD result = CAN_Write(intf->handle, &cmsg);
	#endif

	if(result == -1) {
		syslog(LOG_ALERT, "%s() could not send message: %s", __FUNCTION__, strerror(errno));
		return -1;
	}

	return 0;
}


/**
 * Sends an NMT command
 */
int intf_send_nmt_command(const intf_t *intf, uint8_t command)
{
	assert(intf);

	can_message_t msg;
	msg.id = 0;
	msg.type = mt_standard;
	msg.len = 2;
	msg.data[0] = command;
	msg.data[1] = 1;

	for(int i = 2; i < 8; i++)
		msg.data[i] = 0;

	return intf_write(intf, msg);
}


/**
 * Send read request.
 */
int intf_send_read_req(intf_t *intf, uint16_t index, uint8_t subindex,
	intf_read_callback_t read_callback, intf_abort_callback_t abort_callback, void *data)
{
	assert(intf);

	can_message_t msg;
	msg.id = (0x0C << 7) + 1;
	msg.type = mt_standard;

	msg.len = 8;
	msg.data[0] = 0x40;
	msg.data[1] = (index & 0x00FF);
	msg.data[2] = (index & 0xFF00) >> 8;
	msg.data[3] = subindex;
	msg.data[4] = 0;
	msg.data[5] = 0;
	msg.data[6] = 0;
	msg.data[7] = 0;

	intf->read_callback = read_callback;
	intf->write_callback = NULL;
	intf->abort_callback = abort_callback;
	intf->sdo_callback_data = data;

	return intf_write(intf, msg);
}

/**
 * Send write request.
 */
int intf_send_write_req(intf_t *intf, uint16_t index, uint8_t subindex, uint32_t value, uint8_t size,
	intf_write_callback_t write_callback, intf_abort_callback_t abort_callback, void *data)
{
	assert(intf);

	can_message_t msg;
	msg.id = (0x0C << 7) + 1;
	msg.type = mt_standard;

	msg.len = 8;
	msg.data[0] = 0x2F - ((size - 1) * 4);
	msg.data[1] = (index & 0x00FF);
	msg.data[2] = (index & 0xFF00) >> 8;
	msg.data[3] = subindex;
	msg.data[4] = (value & 0x000000FF);
	msg.data[5] = (value & 0x0000FF00) >> 8;
	msg.data[6] = (value & 0x00FF0000) >> 16;
	msg.data[7] = (value & 0xFF000000) >> 24;

	intf->read_callback = NULL;
	intf->write_callback = write_callback;
	intf->abort_callback = abort_callback;
	intf->sdo_callback_data = data;

	return intf_write(intf, msg);
}


/**
 * Parses CAN message and invokes callbacks.
 */
static void intf_dispatch_msg(intf_t *intf, can_message_t msg)
{
	assert(intf);
	int function = msg.id >> 7;

  // Emergency messages are only logged (not handled)
	if(function == 0x01) {
		syslog(LOG_ALERT, "%s() received an emergency message.", __FUNCTION__);
	}

	// TPDOs
	if(function == 0x03 || function == 0x05 || function == 0x07 || function == 0x09) {
		if(intf->tpdo_handler)
			intf->tpdo_handler(intf, intf->payload, (function-1)/2, msg.data);
	}

	// Node guard message
	if(function == 0x0E && intf->nmt_state_handler) {
		uint8_t state = msg.data[0] & 0x7F;
		intf->nmt_state_handler(intf, intf->payload, state);
	}

	// SDO response
	if(function == 0x0B) {
		uint16_t index = msg.data[1] + (msg.data[2] << 8);
		uint8_t subindex = msg.data[3];
		uint32_t value;

		switch(msg.data[0]) {
			case 0x80:
			case 0x43:
				value = msg.data[4] + (msg.data[5] << 8) + (msg.data[6] << 16) + (msg.data[7] << 24);
				break;
			case 0x47: value = msg.data[4] + (msg.data[5] << 8) + (msg.data[6] << 16); break;
			case 0x4B: value = msg.data[4] + (msg.data[5] << 8); break;
			case 0x4F: value = msg.data[4]; break;
		}

		// Write response
		if(msg.data[0] == 0x60) {
			if(intf->write_callback) {
				intf->write_callback(intf->sdo_callback_data, index, subindex);
			} else {
				syslog(LOG_NOTICE, "%s() received a write response, but no callback function has been set.", __FUNCTION__);
			}
		}

		// Read response
		if(msg.data[0] == 0x43 || msg.data[0] == 0x47 || msg.data[0] == 0x4B || msg.data[0] == 0x4F) {
			if(intf->read_callback) {
				intf->read_callback(intf->sdo_callback_data, index, subindex, value);
			} else {
				syslog(LOG_NOTICE, "%s() received a read response, but no callback function has been set.", __FUNCTION__);
			}
		}

		// Abort response
		if(msg.data[0] == 0x80) {
			if(intf->abort_callback) {
				intf->abort_callback(intf->sdo_callback_data, index, subindex, value);
			} else {
				syslog(LOG_NOTICE, "%s() received an abort response, but no callback function has been set.", __FUNCTION__);
			}
		}
	}
}


/**
 * Called when data is pending
 */
static void intf_on_read(evutil_socket_t fd, short events, void *intf_v)
{
	#ifndef WIN32
	intf_t *intf = (intf_t *) intf_v;

	// We've been closed down, stop.
	if(!intf || !intf->fd)
		return;

	TPCANRdMsg message;
	DWORD result;

	result = LINUX_CAN_Read(intf->handle, &message);

	/*count++;
	if(count % 1000 == 0) {
		double delta = get_time() - start;
		printf("%d messages in %f seconds: %.0f Hz (%.4f sec/msg)\n", count, delta, double(count)/delta, delta/double(count));
	}*/

	// Receive queue was empty, no message read
	if(result == CAN_ERR_QRCVEMPTY) {
		syslog(LOG_NOTICE, "%s() expected message in queue, but found none.", __FUNCTION__);
		return;
	}

	// There was an error
	if(result != CAN_ERR_OK) {
		syslog(LOG_ALERT, "%s() reading from the CAN bus failed interface will be closed.", __FUNCTION__);
		intf_close(intf);
		return;
	}

	// A status message was received
	if(message.Msg.MSGTYPE & MSGTYPE_STATUS == MSGTYPE_STATUS)
	{
		int32_t status = int32_t(CAN_Status(intf->handle));

		if(status < 0) {
			syslog(LOG_ALERT, "%s() received invalid status (%x) interface will be closed.", __FUNCTION__, status);
			intf_close(intf);
			return;
		}

		if(status != 0x20 && status != 0x00)
			intf_log_status(__FUNCTION__, status);

		return;
	}

	uint64_t timestamp = message.dwTime * 1000 + message.wUsec;

	// Tell the world we've received a message
	can_message_t msg;
	msg.id = message.Msg.ID;
	msg.type = message.Msg.MSGTYPE;
	msg.len = message.Msg.LEN;

	for(int i = 0; i < 8; i++)
		msg.data[i] = message.Msg.DATA[i];

	intf_dispatch_msg(intf, msg);

	return;
  #endif
}


void intf_set_callback_payload(intf_t *intf, void *payload)
{
	intf->payload = payload;
}


void intf_set_nmt_state_handler(intf_t *intf, intf_nmt_state_handler_t handler)
{
	intf->nmt_state_handler = handler;
}


void intf_set_tpdo_handler(intf_t *intf, intf_tpdo_handler_t handler)
{
	intf->tpdo_handler = handler;
}


void intf_set_close_handler(intf_t *intf, intf_close_handler_t handler)
{
  intf->close_handler = handler;
}

