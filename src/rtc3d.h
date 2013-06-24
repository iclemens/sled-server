#ifndef __NDIFP_H__
#define __NDIFP_H__

#include <stdint.h>

/**
 * Things to do...
 *  Think of a nice way to alloc/dealloc CTYPE_* structures
 * Create state machine generator
 */

struct rtc3d_server_t;
struct rtc3d_connection_t;
struct event;

enum byte_order_t {
	byo_big_endian,
	byo_little_endian
};

struct marker_t {
	float x, y, z;
	float delta;
};

// Callbacks
typedef void*(*rtc3d_connect_handler_t)(rtc3d_connection_t *rtc3d_conn);
typedef void(*rtc3d_disconnect_handler_t)(rtc3d_connection_t *rtc3d_conn, void **ptr);

typedef void(*rtc3d_command_handler_t)(rtc3d_connection_t *rtc3d_conn, char *cmd);
typedef void(*rtc3d_error_handler_t)(rtc3d_connection_t *rtc3d_conn, char *err);
typedef void(*rtc3d_data_handler_t)(rtc3d_connection_t *rtc3d_conn); // FIXME: Add data field

// API Functions
rtc3d_server_t *rtc3d_setup_server(event_base *event_base, void *user_context, int port);
void rtc3d_teardown_server(rtc3d_server_t **rtc3d_server);

void rtc3d_set_connect_handler(rtc3d_server_t *rtc3d_server, rtc3d_connect_handler_t connect_handler);
void rtc3d_set_disconnect_handler(rtc3d_server_t *rtc3d_server, rtc3d_disconnect_handler_t disconnect_handler);
void rtc3d_set_error_handler(rtc3d_server_t *rtc3d_server, rtc3d_error_handler_t error_handler);
void rtc3d_set_command_handler(rtc3d_server_t *rtc3d_server, rtc3d_command_handler_t command_handler);
void rtc3d_set_data_handler(rtc3d_server_t *rtc3d_server, rtc3d_data_handler_t data_handler);

// Connection manipulation
int rtc3d_disconnect(rtc3d_connection_t *rtc3d_conn);
int rtc3d_set_byte_order(rtc3d_connection_t *rtc3d_conn, byte_order_t byte_order);
void rtc3d_send_error(rtc3d_connection_t *rtc3d_conn, char *error);
void rtc3d_send_command(rtc3d_connection_t *rtc3d_conn, char *error);

void *rtc3d_get_global_data(rtc3d_connection_t *rtc3d_conn);
void *rtc3d_get_local_data(rtc3d_connection_t *rtc3d_conn);


#ifdef __NDI_EXPERIMENTAL__
void ndi_send_data(ndi_connection_t *ndi_conn, uint32_t frame, uint64_t time, float point);
#endif


#endif

