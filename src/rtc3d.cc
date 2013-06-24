/**
 * Implements a simple single-threaded realtime C3D server.
 */

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
#include "rtc3d_internal.h"


/**
 * Creates datastructure used to handle NDI
 * protocol communications with a single client.
 * A void-pointer to this data-structure is returned.
 */
void *connect_handler(net_connection_t *net_conn) {
  rtc3d_server_t *rtc3d_server = (rtc3d_server_t *) net_get_global_data(net_conn);
  rtc3d_connection_t *rtc3d_conn = NULL;

  try {
    rtc3d_conn = new rtc3d_connection_t();
  } catch(std::bad_alloc e) {
    perror("malloc() in connect_handler()");
    return NULL;
  }

  // Store network connection and context  
  rtc3d_conn->net_conn = net_conn;
  rtc3d_conn->user_context = NULL;

  // First step is to read packet size
  rtc3d_conn->size_left = 4;
  rtc3d_conn->size = 0;

  // Set default byte order
  rtc3d_conn->byte_order = byo_big_endian;

  // Invoke callback
  if(rtc3d_server->connect_handler)
    rtc3d_conn->user_context = rtc3d_server->connect_handler(rtc3d_conn);

  return (void *) rtc3d_conn;
}


/**
 * Frees all memory associated with the NDI connection.
 */
void disconnect_handler(net_connection_t *net_conn, void **rtc3d_conn_v) {
  if(!rtc3d_conn_v)
    return;

  rtc3d_server_t *rtc3d_server = (rtc3d_server_t *) net_get_global_data(net_conn);
  rtc3d_connection_t *rtc3d_conn = (rtc3d_connection_t *) *rtc3d_conn_v;

  if(rtc3d_server->disconnect_handler)
    rtc3d_server->disconnect_handler(rtc3d_conn, &(rtc3d_conn->user_context));

  free( (rtc3d_connection_t *) *rtc3d_conn_v);
  *rtc3d_conn_v = NULL;
}


/**
 * Returns the uint32 located in the databuffer at the given offset.
 */
uint32_t get_uint32(rtc3d_connection_t *rtc3d_conn, int offset)
{
  int *tmp = (int *) &(rtc3d_conn->data[offset]);
  return ntohl(*tmp);
}


/**
 * Parses a full NDI package
 */
void packet_handler(rtc3d_connection_t *rtc3d_conn) {
  rtc3d_server_t *rtc3d_server = (rtc3d_server_t *) net_get_global_data(rtc3d_conn->net_conn); 
  int type = get_uint32(rtc3d_conn, 0);

  switch(type) {
    case PTYPE_COMMAND: {
      rtc3d_conn->data[rtc3d_conn->size] = '\0';
      printf("Command (%d): %s\n", rtc3d_conn->size - 4, &rtc3d_conn->data[4]);

      if(rtc3d_server->command_handler)
        rtc3d_server->command_handler(rtc3d_conn, &rtc3d_conn->data[4]);

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
  rtc3d_connection_t *rtc3d_conn = (rtc3d_connection_t *) net_get_local_data(net_conn);

  for(int i = 0; i < size; i++) {

    // Part of header (size)
    if(rtc3d_conn->size_left > 0) {
      char *tmp = (char *) &(rtc3d_conn->size);
      tmp[rtc3d_conn->size_left - 1] = buf[i];
      rtc3d_conn->size_left--;

      // Header is now complete, start reading data
      if(rtc3d_conn->size_left == 0) {
        rtc3d_conn->size -= 4;
        rtc3d_conn->data = (char *) malloc(rtc3d_conn->size + 1); // + 1 for \0
        rtc3d_conn->data_left = rtc3d_conn->size;
      }

      continue;
    }

    // Part of payload
    if((rtc3d_conn->size_left == 0) && (rtc3d_conn->data_left > 0)) {
      rtc3d_conn->data[rtc3d_conn->size - rtc3d_conn->data_left] = buf[i];
      rtc3d_conn->data_left--;

      // Data is complete, process packet
      if(rtc3d_conn->data_left == 0) {
        packet_handler(rtc3d_conn);
        //send_response(sock, info, &response);
        //ndi_send_command_response(conn, (char *)"ok-setbyteorder");

        free(rtc3d_conn->data);

        rtc3d_conn->size = 0;
        rtc3d_conn->size_left = 4;
      }
    }
  }
}


/**
 * Prepare TCP server handling the NDI FP protocol.
 */
rtc3d_server_t *rtc3d_setup_server(event_base *event_base, void *user_context, int port)
{
  rtc3d_server_t *rtc3d_server = NULL;

  try {
    rtc3d_server = new rtc3d_server_t();
  } catch(std::bad_alloc e) {
    return NULL;
  }

  rtc3d_server->user_context = user_context;

  rtc3d_server->net_server = net_setup_server(event_base, rtc3d_server, 3375);
  if(!rtc3d_server->net_server) {
    fprintf(stderr, "net_setup_server() failed\n");
    delete rtc3d_server;
    return NULL;
  }

  net_set_connect_handler(rtc3d_server->net_server, connect_handler);
  net_set_disconnect_handler(rtc3d_server->net_server, disconnect_handler);
  net_set_read_handler(rtc3d_server->net_server, read_handler);

  return rtc3d_server;
}


/**
 * Shutdown TCP server.
 */
void rtc3d_teardown_server(rtc3d_server_t **rtc3d_server)
{
  net_teardown_server(&(*rtc3d_server)->net_server);

  delete *rtc3d_server;
  *rtc3d_server = NULL;
}


/**
 * Set byte order for the given client.
 */
int rtc3d_set_byte_order(rtc3d_connection_t *rtc3d_conn, byte_order_t byte_order)
{
  rtc3d_conn->byte_order = byte_order;

  return 0;
}


void *rtc3d_get_global_data(rtc3d_connection_t *rtc3d_conn)
{
  rtc3d_server_t *rtc3d_server = (rtc3d_server_t *) net_get_global_data(rtc3d_conn->net_conn);
  return rtc3d_server->user_context;
}


void *rtc3d_get_local_data(rtc3d_connection_t *rtc3d_conn)
{
  return rtc3d_conn->user_context;
}


/**
 * Set callback to be called when a client connects.
 */
void rtc3d_set_connect_handler(rtc3d_server_t *rtc3d_server, rtc3d_connect_handler_t connect_handler)
{
  if(!rtc3d_server)
    return;
  rtc3d_server->connect_handler = connect_handler;
}


/**
 * Set callback to be called when a client disconnects.
 */
void rtc3d_set_disconnect_handler(rtc3d_server_t *rtc3d_server, rtc3d_disconnect_handler_t disconnect_handler)
{
  if(!rtc3d_server)
    return;
  rtc3d_server->disconnect_handler = disconnect_handler;
}


/**
 * Set callback to be called when a client send an error
 */
void rtc3d_set_error_handler(rtc3d_server_t *rtc3d_server, rtc3d_error_handler_t error_handler)
{
  if(!rtc3d_server)
    return;
  rtc3d_server->error_handler = error_handler;
}


/**
 * Set callback to be called when a client invokes a command
 */
void rtc3d_set_command_handler(rtc3d_server_t *rtc3d_server, rtc3d_command_handler_t command_handler)
{
  if(!rtc3d_server)
    return;
  rtc3d_server->command_handler = command_handler;
}


/**
 * Set callback to be called when a client send data
 */
void rtc3d_set_data_handler(rtc3d_server_t *rtc3d_server, rtc3d_data_handler_t data_handler)
{
  if(!rtc3d_server)
    return;
  rtc3d_server->data_handler = data_handler;
}


int rtc3d_disconnect(rtc3d_connection_t *rtc3d_conn)
{
  return net_disconnect(rtc3d_conn->net_conn);
}


/********************************
 * SENDING DATA TO AN NDI CLIENT
 */


void rtc3d_send_command(rtc3d_connection_t *rtc3d_conn, char *command)
{
  int cmd_size = strlen(command);
  char *buffer = (char *) malloc(8 + cmd_size);

  int *size = (int *) &(buffer[0]);
  int *type = (int *) &(buffer[4]);

  *size = htonl(8 + cmd_size);
  *type = htonl(PTYPE_COMMAND);
  strncpy(&(buffer[8]), command, cmd_size);

  net_send(rtc3d_conn->net_conn, buffer, 8 + cmd_size, F_ADOPT_BUFFER);
}


void rtc3d_send_error(rtc3d_connection_t *rtc3d_conn, char *error)
{
  int err_size = strlen(error);
  char *buffer = (char *) malloc(8 + err_size);

  int *size = (int *) &(buffer[0]);
  int *type = (int *) &(buffer[4]);

  *size = htonl(8 + err_size);
  *type = htonl(PTYPE_ERROR);
  strncpy(&(buffer[8]), error, err_size);

  net_send(rtc3d_conn->net_conn, buffer, 8 + err_size, F_ADOPT_BUFFER);
}
