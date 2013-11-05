#ifndef __NDI_INTERNAL_H__
#define __NDI_INTERNAL_H__

#include "rtc3d.h"
#include "server.h"

// Packet types
#define PTYPE_ERROR 0
#define PTYPE_COMMAND 1
#define PTYPE_XML 2
#define PTYPE_DATAFRAME 3
#define PTYPE_NODATA 4
#define PTYPE_C3DFILE 5

// Data types
#define CTYPE_3D 1
#define CTYPE_ANALOG 2
#define CTYPE_FORCE 3
#define CTYPE_6D 4
#define CTYPE_EVENT 5

struct rtc3d_server_t;

struct rtc3d_connection_t {
  // Connection
  net_connection_t *net_conn;

  void *user_context;   // Local context

  // TCP packet parsing
  uint8_t size_left;
  uint32_t size;

  uint8_t data_left;
  char *data;

  // Byte order
  byte_order_t byte_order;
};

struct rtc3d_server_t {
  net_server_t *net_server;
  void *user_context;

  rtc3d_connect_handler_t connect_handler;
  rtc3d_disconnect_handler_t disconnect_handler;

  rtc3d_error_handler_t error_handler;
  rtc3d_command_handler_t command_handler;
  rtc3d_data_handler_t data_handler;
};

#endif
