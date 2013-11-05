#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdlib.h>

#ifdef WIN32
#define APIFUNC __declspec(dllexport)
#else
#define APIFUNC
#endif

struct event_base;
struct net_server_t;
struct net_connection_t;

// Callbacks
typedef void*(*connect_handler_t)(net_connection_t *conn);
typedef void(*disconnect_handler_t)(net_connection_t *conn, void **ctx);
typedef void(*read_handler_t)(net_connection_t *conn, char *buf, int size);

// API functions
APIFUNC net_server_t *net_setup_server(event_base *ev_base, void *context, int port);
APIFUNC int net_teardown_server(net_server_t **server);

// Setters for callback methods
APIFUNC void net_set_connect_handler(net_server_t *server, connect_handler_t handler);
APIFUNC void net_set_disconnect_handler(net_server_t *server, disconnect_handler_t handler);
APIFUNC void net_set_read_handler(net_server_t *server, read_handler_t handler);

// Functions to be used in callbacks
APIFUNC void *net_get_global_data(net_connection_t *conn);
APIFUNC void *net_get_local_data(net_connection_t *conn);
APIFUNC int net_disconnect(net_connection_t *conn);
APIFUNC int net_send(net_connection_t *conn, const char *buf, size_t size);

#endif
