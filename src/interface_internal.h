#ifndef __INTERFACE_INTERNAL_H__
#define __INTERFACE_INTERNAL_H__

#include <stdint.h>
#include <libpcan.h>
#include <event2/event.h>

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
  intf_read_resp_handler_t read_resp_handler;
  intf_write_resp_handler_t write_resp_handler;
  intf_abort_resp_handler_t abort_resp_handler;
  intf_close_handler_t close_handler;
};

void intf_on_read(evutil_socket_t fd, short events, void *intf_v);

#endif
