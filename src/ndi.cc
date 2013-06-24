/**
 * Implements a simple single-threaded First Principles server.
 */

// The following to to get access to
// the server's epoll file descriptor.
#define __SERVER_LEAK_IMPL_DETAILS__

#include <exception>
#include <map>

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

#ifdef WIN32
#define strncmp(a, b, n) strncmp_s(a, b, n)
#define strncpy(a, b, n) strncpy_s(a, 1, b, n)
#endif

#include "server.h"
#include "ndi_internal.h"


/**
 * Creates datastructure used to handle NDI
 * protocol communications with a single client.
 * A void-pointer to this data-structure is returned.
 */
void *connect_handler(net_connection_t *net_conn) {
  ndi_server_t *ndi_server = (ndi_server_t *) net_get_global_data(net_conn);
  ndi_connection_t *ndi_conn = NULL;

  try {
    ndi_conn = new ndi_connection_t();
  } catch(std::bad_alloc e) {
    perror("malloc() in connect_handler()");
    return NULL;
  }

  // Store network connection and context  
  ndi_conn->net_conn = net_conn;
  ndi_conn->user_context = NULL;

  // First step is to read packet size
  ndi_conn->size_left = 4;
  ndi_conn->size = 0;

  // Set default byte order
  ndi_conn->byte_order = byo_big_endian;

  // Invoke callback
  if(ndi_server->connect_handler)
    ndi_conn->user_context = ndi_server->connect_handler(ndi_conn);

  return (void *) ndi_conn;
}


/**
 * Frees all memory associated with the NDI connection.
 */
void disconnect_handler(net_connection_t *net_conn, void **ndi_conn_v) {
  if(!ndi_conn_v)
    return;

  ndi_server_t *ndi_server = (ndi_server_t *) net_get_global_data(net_conn);
  ndi_connection_t *ndi_conn = (ndi_connection_t *) *ndi_conn_v;

  if(ndi_server->disconnect_handler)
    ndi_server->disconnect_handler(ndi_conn, &(ndi_conn->user_context));

  free( (ndi_connection_t *) *ndi_conn_v);
  *ndi_conn_v = NULL;
}


/**
 * Returns the uint32 located in the databuffer at the given offset.
 */
uint32_t get_uint32(ndi_connection_t *ndi_conn, int offset)
{
  int *tmp = (int *) &(ndi_conn->data[offset]);
  return ntohl(*tmp);
}


/**
 * Parses a full NDI package
 */
void packet_handler(ndi_connection_t *ndi_conn) {
  ndi_server_t *ndi_server = (ndi_server_t *) net_get_global_data(ndi_conn->net_conn); 
  int type = get_uint32(ndi_conn, 0);

  switch(type) {
    case PTYPE_COMMAND: {
      ndi_conn->data[ndi_conn->size] = '\0';
      printf("Command (%d): %s\n", ndi_conn->size - 4, &ndi_conn->data[4]);

      if(ndi_server->command_handler)
        ndi_server->command_handler(ndi_conn, &ndi_conn->data[4]);

      break;
    };

    case PTYPE_XML: {
      break;
    };

    case PTYPE_DATAFRAME: {
      break;
    };

    case PTYPE_NODATA: {
      break;
    };

    case PTYPE_C3DFILE: {
    };

    default: {
      fprintf(stderr, "Ignoring packet, unknown type: %d\n", type);
    };
  };
}


/**
 * Reads a packet from the stream and send it to the packet handler.
 */
void read_handler(net_connection_t *net_conn, char *buf, int size) {
  ndi_connection_t *ndi_conn = (ndi_connection_t *) net_get_local_data(net_conn);

  for(int i = 0; i < size; i++) {

    // Part of header (size)
    if(ndi_conn->size_left > 0) {
      char *tmp = (char *) &(ndi_conn->size);
      tmp[ndi_conn->size_left - 1] = buf[i];
      ndi_conn->size_left--;

      // Header is now complete, start reading data
      if(ndi_conn->size_left == 0) {
        ndi_conn->size -= 4;
        ndi_conn->data = (char *) malloc(ndi_conn->size + 1); // + 1 for \0
        ndi_conn->data_left = ndi_conn->size;
      }

      continue;
    }

    // Part of payload
    if((ndi_conn->size_left == 0) && (ndi_conn->data_left > 0)) {
      ndi_conn->data[ndi_conn->size - ndi_conn->data_left] = buf[i];
      ndi_conn->data_left--;

      // Data is complete, process packet
      if(ndi_conn->data_left == 0) {
        packet_handler(ndi_conn);
        //send_response(sock, info, &response);
        //ndi_send_command_response(conn, (char *)"ok-setbyteorder");

        free(ndi_conn->data);

        ndi_conn->size = 0;
        ndi_conn->size_left = 4;
      }
    }
  }
}


/**
 * Prepare TCP server handling the NDI FP protocol.
 */
ndi_server_t *ndi_setup_server(event_base *event_base, void *user_context, int port)
{
  ndi_server_t *ndi_server = NULL;

  try {
    ndi_server = new ndi_server_t();
  } catch(std::bad_alloc e) {
    return NULL;
  }

  ndi_server->user_context = user_context;

  ndi_server->net_server = net_setup_server(event_base, ndi_server, 3375);
  if(!ndi_server->net_server) {
    fprintf(stderr, "net_setup_server() failed\n");
    delete ndi_server;
    return NULL;
  }

  net_set_connect_handler(ndi_server->net_server, connect_handler);
  net_set_disconnect_handler(ndi_server->net_server, disconnect_handler);
  net_set_read_handler(ndi_server->net_server, read_handler);

  return ndi_server;
}


/**
 * Shutdown TCP server.
 */
void ndi_teardown_server(ndi_server_t **ndi_server)
{
  net_teardown_server(&(*ndi_server)->net_server);

  delete *ndi_server;
  *ndi_server = NULL;
}


/**
 * Set byte order for the given client.
 */
int ndi_set_byte_order(ndi_connection_t *ndi_conn, byte_order_t byte_order)
{
  ndi_conn->byte_order = byte_order;

  return 0;
}


void *ndi_get_global_data(ndi_connection_t *ndi_conn)
{
  ndi_server_t *ndi_server = (ndi_server_t *) net_get_global_data(ndi_conn->net_conn);
  return ndi_server->user_context;
}


void *ndi_get_local_data(ndi_connection_t *ndi_conn)
{
  return ndi_conn->user_context;
}


/**
 * Set callback to be called when a client connects.
 */
void ndi_set_connect_handler(ndi_server_t *ndi_server, ndi_connect_handler_t connect_handler)
{
  if(!ndi_server)
    return;
  ndi_server->connect_handler = connect_handler;
}


/**
 * Set callback to be called when a client disconnects.
 */
void ndi_set_disconnect_handler(ndi_server_t *ndi_server, ndi_disconnect_handler_t disconnect_handler)
{
  if(!ndi_server)
    return;
  ndi_server->disconnect_handler = disconnect_handler;
}


/**
 * Set callback to be called when a client send an error
 */
void ndi_set_error_handler(ndi_server_t *ndi_server, ndi_error_handler_t error_handler)
{
  if(!ndi_server)
    return;
  ndi_server->error_handler = error_handler;
}


/**
 * Set callback to be called when a client invokes a command
 */
void ndi_set_command_handler(ndi_server_t *ndi_server, ndi_command_handler_t command_handler)
{
  if(!ndi_server)
    return;
  ndi_server->command_handler = command_handler;
}


/**
 * Set callback to be called when a client send data
 */
void ndi_set_data_handler(ndi_server_t *ndi_server, ndi_data_handler_t data_handler)
{
  if(!ndi_server)
    return;
  ndi_server->data_handler = data_handler;
}


int ndi_disconnect(ndi_connection_t *ndi_conn)
{
  return net_disconnect(ndi_conn->net_conn);
}


/********************************
 * SENDING DATA TO AN NDI CLIENT
 */


void ndi_send_command(ndi_connection_t *ndi_conn, char *command)
{
  int cmd_size = strlen(command);
  char *buffer = (char *) malloc(8 + cmd_size);

  int *size = (int *) &(buffer[0]);
  int *type = (int *) &(buffer[4]);

  *size = htonl(8 + cmd_size);
  *type = htonl(PTYPE_COMMAND);
  strncpy(&(buffer[8]), command, cmd_size);

  net_send(ndi_conn->net_conn, buffer, 8 + cmd_size, F_ADOPT_BUFFER);
}


void ndi_send_error(ndi_connection_t *ndi_conn, char *error)
{
  int err_size = strlen(error);
  char *buffer = (char *) malloc(8 + err_size);

  int *size = (int *) &(buffer[0]);
  int *type = (int *) &(buffer[4]);

  *size = htonl(8 + err_size);
  *type = htonl(PTYPE_ERROR);
  strncpy(&(buffer[8]), error, err_size);

  net_send(ndi_conn->net_conn, buffer, 8 + err_size, F_ADOPT_BUFFER);
}
