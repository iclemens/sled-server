#ifndef __INTERFACE_INTERNAL_H__
#define __INTERFACE_INTERNAL_H__

#include <stdint.h>
#include <event2/event.h>

#ifdef WIN32
#include <PCANBasic.h>
#else
#include <libpcan.h>
#endif

#include "interface.h"

enum message_type_t
{
	mt_status = 0x80,
	mt_extended = 0x02,
	mt_rtr = 0x01,
	mt_standard = 0x00
};

struct can_message_t
{
	uint16_t id;
	uint8_t type;
	uint8_t len;
	uint8_t data[8];
};

struct intf_t
{
  event_base *ev_base;
  HANDLE handle;
  int fd;
  
  event *read_event;
  
  // Callbacks
  void *payload;
  
  intf_nmt_state_handler_t nmt_state_handler;  
	intf_tpdo_handler_t tpdo_handler;
  intf_close_handler_t close_handler;

	// Data passed to read/write/abort callbacks
	void *sdo_callback_data;

	// Write SDO callback
	intf_write_callback_t write_callback;

	// Read SDO callback
	intf_read_callback_t read_callback;

	// Abort SDO callback
	intf_abort_callback_t abort_callback;
};

static void intf_on_read(evutil_socket_t fd, short events, void *intf_v);

#endif
