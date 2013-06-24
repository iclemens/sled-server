#ifndef __NDIFP_H__
#define __NDIFP_H__

#include <stdint.h>

/**
 * Things to do...
 *  Think of a nice way to alloc/dealloc CTYPE_* structures
 * Create state machine generator
 */

struct ndi_server_t;
struct ndi_connection_t;

enum byte_order_t {
  byo_big_endian,
  byo_little_endian
};

struct marker_t {
  float x, y, z;
  float delta;
};

// Callbacks
typedef void*(*ndi_connect_handler_t)(ndi_connection_t *ndi_conn);
typedef void(*ndi_disconnect_handler_t)(ndi_connection_t *ndi_conn, void **ptr);

typedef void(*ndi_command_handler_t)(ndi_connection_t *ndi_conn, char *cmd);
typedef void(*ndi_error_handler_t)(ndi_connection_t *ndi_conn, char *err);
typedef void(*ndi_data_handler_t)(ndi_connection_t *ndi_conn);                // FIXME: Add data field

// API Functions
ndi_server_t *ndi_setup_server(void *user_context, int port);
int ndi_run_event_loop(ndi_server_t *ndi_server);
void ndi_teardown_server(ndi_server_t **ndi_server);

void ndi_set_connect_handler(ndi_server_t *ndi_server, ndi_connect_handler_t connect_handler);
void ndi_set_disconnect_handler(ndi_server_t *ndi_server, ndi_disconnect_handler_t disconnect_handler);
void ndi_set_error_handler(ndi_server_t *ndi_server, ndi_error_handler_t error_handler);
void ndi_set_command_handler(ndi_server_t *ndi_server, ndi_command_handler_t command_handler);
void ndi_set_data_handler(ndi_server_t *ndi_server, ndi_data_handler_t data_handler);

// Connection manipulation
int ndi_disconnect(ndi_connection_t *ndi_conn);
int ndi_set_byte_order(ndi_connection_t *ndi_conn, byte_order_t byte_order);
void ndi_send_error(ndi_connection_t *ndi_conn, char *error);
void ndi_send_command(ndi_connection_t *ndi_conn, char *error);

void *ndi_get_global_data(ndi_connection_t *ndi_conn);
void *ndi_get_local_data(ndi_connection_t *ndi_conn);


#ifdef __NDI_LEAK_IMPL_DETAILS__
/**
 * These functions leak epoll file descriptors and should be
 * replaced by something better.
 */

int ndi_get_epoll_fd(ndi_server_t *server);
int ndi_handle_epoll_events(ndi_server_t *server);
#endif


#ifdef __NDI_EXPERIMENTAL__
void ndi_send_data(ndi_connection_t *ndi_conn, uint32_t frame, uint64_t time, float point);
#endif


#endif

