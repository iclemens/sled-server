#ifndef __NDI_INTERNAL_H__
#define __NDI_INTERNAL_H__

#include "server.h"
#include "rtc3d.h"

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

struct ndi_server_t;

struct ndi_connection_t {
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

struct ndi_server_t {
  net_server_t *net_server;
  void *user_context;

  ndi_connect_handler_t connect_handler;
  ndi_disconnect_handler_t disconnect_handler;

  ndi_error_handler_t error_handler;
  ndi_command_handler_t command_handler;
  ndi_data_handler_t data_handler;
};

#endif

