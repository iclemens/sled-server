#ifndef __NDIFP_H__
#define __NDIFP_H__

#include <stdint.h>

#ifdef WIN32
#define APIFUNC __declspec(dllexport)
#else
#define APIFUNC
#endif

struct rtc3d_server_t;
struct rtc3d_connection_t;
struct event;
struct event_base;

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
APIFUNC rtc3d_server_t *rtc3d_setup_server(event_base *event_base, void *user_context, int port);
APIFUNC void rtc3d_teardown_server(rtc3d_server_t **rtc3d_server);

APIFUNC void rtc3d_set_connect_handler(rtc3d_server_t *rtc3d_server, rtc3d_connect_handler_t connect_handler);
APIFUNC void rtc3d_set_disconnect_handler(rtc3d_server_t *rtc3d_server, rtc3d_disconnect_handler_t disconnect_handler);
APIFUNC void rtc3d_set_error_handler(rtc3d_server_t *rtc3d_server, rtc3d_error_handler_t error_handler);
APIFUNC void rtc3d_set_command_handler(rtc3d_server_t *rtc3d_server, rtc3d_command_handler_t command_handler);
APIFUNC void rtc3d_set_data_handler(rtc3d_server_t *rtc3d_server, rtc3d_data_handler_t data_handler);

// Connection manipulation
APIFUNC int rtc3d_disconnect(rtc3d_connection_t *rtc3d_conn);
APIFUNC int rtc3d_set_byte_order(rtc3d_connection_t *rtc3d_conn, byte_order_t byte_order);
APIFUNC void rtc3d_send_error(rtc3d_connection_t *rtc3d_conn, const char *error);
APIFUNC void rtc3d_send_command(rtc3d_connection_t *rtc3d_conn, const char *error);

APIFUNC void *rtc3d_get_global_data(rtc3d_connection_t *rtc3d_conn);
APIFUNC void *rtc3d_get_local_data(rtc3d_connection_t *rtc3d_conn);

void rtc3d_send_data(const rtc3d_connection_t *rtc3d_conn, uint32_t frame, uint64_t time, float point);

#endif
