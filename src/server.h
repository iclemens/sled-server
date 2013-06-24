#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdlib.h>

#define FADOPTBUFFER 1

struct net_server_t;
struct net_connection_t;

// Callbacks
typedef void*(*connect_handler_t)(net_connection_t *conn);
typedef void(*disconnect_handler_t)(net_connection_t *conn, void **ctx);
typedef void(*read_handler_t)(net_connection_t *conn, char *buf, int size);

// API functions
net_server_t *net_setup_server(void *context, int port);
int net_teardown_server(net_server_t **server);

// Setters for callback methods
void net_set_connect_handler(net_server_t *server, connect_handler_t handler);
void net_set_disconnect_handler(net_server_t *server, disconnect_handler_t handler);
void net_set_read_handler(net_server_t *server, read_handler_t handler);

// Functions to be used in callbacks
void *net_get_global_data(net_connection_t *conn);
void *net_get_local_data(net_connection_t *conn);
int net_disconnect(net_connection_t *conn);
int net_send(net_connection_t *conn, char *buf, size_t size, int flags);

#endif
