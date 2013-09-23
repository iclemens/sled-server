#ifndef __SERVER_INTERNAL_H__
#define __SERVER_INTERNAL_H__

#include "server.h"
#include <map>

struct net_server_t;
struct event;
struct evutil_socket_t;

struct fragment_t {
  int offset;
  int size;
  char *data;

  fragment_t *next;
};


struct net_connection_t {
	// Socket file descriptor
	int fd;

	// Server that owns connection
	net_server_t *server;

	// Events
	event *read_event;
	event *write_event;

	// Context for this connection
	void *local;

	// Fragments to be written
	fragment_t *frags_head;
	fragment_t *frags_tail;
};


struct net_server_t {
  void *context;                            // Pointer passed to all callbacks

  int sock;                                      // Listening socket
  event_base *ev_base;
  std::map<int, net_connection_t *> connection_data; // Per-client persistent data

  // Event handlers
  connect_handler_t connect_handler;        // Creates new local context
  disconnect_handler_t disconnect_handler;  // Destroys local context
  read_handler_t read_handler;              // Handles new data
};


// Server functions
net_server_t *net_setup_server(event_base *ev_base, void *context, int port);
int net_teardown_server(net_server_t **s);
void net_set_connect_handler(net_server_t *server, connect_handler_t handler);
void net_set_disconnect_handler(net_server_t *server, disconnect_handler_t handler);
void net_set_read_handler(net_server_t *server, read_handler_t handler);

// Connection functions
int write_single_fragment(net_connection_t *conn);
int net_send(net_connection_t *conn, char *buf, size_t size, int flags);
int net_disconnect(net_connection_t *conn);
void *net_get_global_data(net_connection_t *conn);
void *net_get_local_data(net_connection_t *conn);

// Utility functions
int setup_server_socket(int port);
int accept_client(int sock);

#endif

