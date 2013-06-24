#ifndef __SERVER_INTERNAL_H__
#define __SERVER_INTERNAL_H__

#include "server.h"

#include <event2/event.h>
#include <map>

struct net_server_t;


struct fragment_t {
  int offset;
  int size;
  char *data;

  fragment_t *next;
};


struct net_connection_t {
  int fd;           // Socket file descriptor
  net_server_t *server; // Server that owns connection
  void *local;      // External local context

  fragment_t *frags_head;
  fragment_t *frags_tail;
};


struct net_server_t {
  void *context;                            // Pointer passed to all callbacks

  int sock;                                      // Listening socket
  event_base *event_base;
  std::map<int, net_connection_t *> connection_data; // Per-client persistent data

  // Event handlers
  connect_handler_t connect_handler;        // Creates new local context
  disconnect_handler_t disconnect_handler;  // Destroys local context
  read_handler_t read_handler;              // Handles new data
};


#endif

