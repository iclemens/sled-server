/*
 * Implements a simple single-threaded realtime C3D server.
 */

#include <exception>
#include <map>

#include <syslog.h>
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


//////////////////////
//  Event handlers  //
//////////////////////


/**
 * Creates datastructure used to handle RT3CD protocol communications with a single client.
 *
 * @param net_conn Network connection that just connected.
 * @return RTC3D client context structure.
 */
void *connect_handler(net_connection_t *net_conn)
{
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
 * Frees all memory associated with the RTC3D connection.
 *
 * @param net_conn Network connection that just disconnected.
 * @param rtc3d_conn_v RTC3D context to free.
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
 *
 * @param rtc3d_conn Connection to read the data from.
 * @param offset Offset in bytes.
 *
 * @return 32-bit unsigned integer located at specified offset.
 */
uint32_t get_uint32(rtc3d_connection_t *rtc3d_conn, int offset)
{
  int *tmp = (int *) &(rtc3d_conn->data[offset]);
  return ntohl(*tmp);
}


/**
 * Parses the RTC3D package in the databuffer. Calls command and data handlers as needed.
 *
 * @param rtc3d_conn Connection to read the data from.
 */
void packet_handler(rtc3d_connection_t *rtc3d_conn) {
  rtc3d_server_t *rtc3d_server = (rtc3d_server_t *) net_get_global_data(rtc3d_conn->net_conn);
  int type = get_uint32(rtc3d_conn, 0);

  switch(type) {
    case PTYPE_COMMAND: {
      rtc3d_conn->data[rtc3d_conn->size] = '\0';
      syslog(LOG_DEBUG, "%s() command (%d): %s", __FUNCTION__, rtc3d_conn->size - 4, &rtc3d_conn->data[4]);

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
		syslog(LOG_NOTICE, "%s() packet has unknown type (%d)", __FUNCTION__, type);
    };
  };
}


/**
 * Handles incoming data from the network. Waits until a full
 * packet is in the buffer and then sends it to the packet parser.
 *
 * @param net_conn  Network connection the data was read from.
 * @param buf  Data that we received.
 * @param size  Number of bytes received.
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

        free(rtc3d_conn->data);

        rtc3d_conn->size = 0;
        rtc3d_conn->size_left = 4;
      }
    }
  }
}


////////////////////////
//  Server functions  //
////////////////////////


/**
 * Creates a new instance of a real-time C3D server.
 *
 * Note that the server is not operational until the event_base_loop() is started.
 *
 * @param event_base  Event base where file descriptor will be registered
 * @param user_context  Pointer passed to all callback-functions
 * @param port  TCP port on which the server listens
 *
 * @return Instance of the RTC3D server.
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
    syslog(LOG_ERR, "%s() failed", __FUNCTION__);
    delete rtc3d_server;
    return NULL;
  }

  net_set_connect_handler(rtc3d_server->net_server, connect_handler);
  net_set_disconnect_handler(rtc3d_server->net_server, disconnect_handler);
  net_set_read_handler(rtc3d_server->net_server, read_handler);

  return rtc3d_server;
}


/**
 * Shutdown realtime C3D server.
 *
 * @param rtc3d_server  Instance of the server to shutdown.
 */
void rtc3d_teardown_server(rtc3d_server_t **rtc3d_server)
{
  net_teardown_server(&(*rtc3d_server)->net_server);

  delete *rtc3d_server;
  *rtc3d_server = NULL;
}


/**
 * Set callback to be called when a client connects.
 *
 * @param rtc3d_server  Instance of the server.
 * @param connect_handler  Function to be called when a client connects.
 */
void rtc3d_set_connect_handler(rtc3d_server_t *rtc3d_server, rtc3d_connect_handler_t connect_handler)
{
  if(!rtc3d_server)
    return;
  rtc3d_server->connect_handler = connect_handler;
}


/**
 * Set callback to be called when a client disconnects.
 *
 * @param rtc3d_server  Instance of the server.
 * @param disconnect_handler  Function to be called when the client disconnects.
 */
void rtc3d_set_disconnect_handler(rtc3d_server_t *rtc3d_server, rtc3d_disconnect_handler_t disconnect_handler)
{
  if(!rtc3d_server)
    return;
  rtc3d_server->disconnect_handler = disconnect_handler;
}


/**
 * Set callback to be called when a client send an error.
 *
 * @param rtc3d_server  Instance of the server.
 * @param error_handler  Function to be called when an error packet arrives.
 */
void rtc3d_set_error_handler(rtc3d_server_t *rtc3d_server, rtc3d_error_handler_t error_handler)
{
  if(!rtc3d_server)
    return;
  rtc3d_server->error_handler = error_handler;
}


/**
 * Set callback to be called when a client invokes a command.
 *
 * @param rtc3d_server  Instance of the server.
 * @param command_handler  Function to be called when a command packet arrives.
 */
void rtc3d_set_command_handler(rtc3d_server_t *rtc3d_server, rtc3d_command_handler_t command_handler)
{
  if(!rtc3d_server)
    return;
  rtc3d_server->command_handler = command_handler;
}


/**
 * Set callback to be called when a client send data.
 *
 * @param rtc3d_server  Instance of the server.
 * @param data_handler  Function to be called when a data packet arrives.
 */
void rtc3d_set_data_handler(rtc3d_server_t *rtc3d_server, rtc3d_data_handler_t data_handler)
{
  if(!rtc3d_server)
    return;
  rtc3d_server->data_handler = data_handler;
}


////////////////////////////
//  Connection functions  //
////////////////////////////


/**
 * Set byte order for the given client.
 *
 * @param rtc3d_conn  Connection to set the byte-order for.
 * @param byte_order  Byte-order (byo_big_endian or byo_little_endian).
 *
 * @return Zero indicates success, everything else failure.
 */
int rtc3d_set_byte_order(rtc3d_connection_t *rtc3d_conn, byte_order_t byte_order)
{
  rtc3d_conn->byte_order = byte_order;

  return 0;
}


/**
 * Returns global pointer for an RTC3D connection.
 *
 * @param rtc3d_conn  RTC3D connection.
 *
 * @return Pointer to user context.
 */
void *rtc3d_get_global_data(rtc3d_connection_t *rtc3d_conn)
{
  rtc3d_server_t *rtc3d_server = (rtc3d_server_t *) net_get_global_data(rtc3d_conn->net_conn);
  return rtc3d_server->user_context;
}


/**
 * Returns per-connection pointer for an RTC3D connection.
 *
 * @param rtc3d_conn  RTC3D connection.
 *
 * @return Pointer to the per-connection data.
 */
void *rtc3d_get_local_data(rtc3d_connection_t *rtc3d_conn)
{
  return rtc3d_conn->user_context;
}


int rtc3d_disconnect(rtc3d_connection_t *rtc3d_conn)
{
  return net_disconnect(rtc3d_conn->net_conn);
}


/**
 * Sends a command to an RTC3D client.
 *
 * @param rtc3d_conn  Connection to send command to.
 * @param command  Command to send (null-terminated string)
 */
void rtc3d_send_command(rtc3d_connection_t *rtc3d_conn, char *command)
{
  int cmd_size = strlen(command);
  char *buffer = (char *) malloc(8 + cmd_size);

  if(buffer == NULL) {
    perror("malloc()");
    return;
  }

  int *size = (int *) &(buffer[0]);
  int *type = (int *) &(buffer[4]);

  *size = htonl(8 + cmd_size);
  *type = htonl(PTYPE_COMMAND);
  strncpy(&(buffer[8]), command, cmd_size);

  net_send(rtc3d_conn->net_conn, buffer, 8 + cmd_size);
  free(buffer);
}


/**
 * Sends an error to an RTC3D client.
 *
 * @param rtc3d_conn  Connection to send error to.
 * @param error  Error to send (null-terminated string)
 */
void rtc3d_send_error(rtc3d_connection_t *rtc3d_conn, char *error)
{
  int err_size = strlen(error);
  char *buffer = (char *) malloc(8 + err_size);

  if(buffer == NULL) {
    perror("malloc()");
    return;
  }

  int *size = (int *) &(buffer[0]);
  int *type = (int *) &(buffer[4]);

  *size = htonl(8 + err_size);
  *type = htonl(PTYPE_ERROR);
  strncpy(&(buffer[8]), error, err_size);

  net_send(rtc3d_conn->net_conn, buffer, 8 + err_size);
  free(buffer);
}

