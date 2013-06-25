
#include "interface.h"

#include <event2/event.h>
#include <libpcan.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/stat.h>

struct intf_t
{
  event_base *ev_base;
  HANDLE handle;
  int fd;
  
  event *read_event;
};

void intf_on_read(evutil_socket_t fd, short events, void *intf_v);


/**
 * Setup CAN Interface
 */
intf_t *intf_create(event_base *ev_base)
{
  intf_t *intf = (intf_t *) malloc(sizeof(intf_t));
  
  intf->ev_base = ev_base;
  intf->handle = 0;
  intf->fd = 0;
  
  intf->read_event = NULL;
  
  return intf;
}


/**
 * Destroy CAN Interface
 */
void intf_destroy(intf_t **intf)
{
  intf_close(*intf);
  free(*intf);
  *intf = NULL;
}


/**
 * Open connection to device
 */
int intf_open(intf_t *intf)
{
  const char *device = "/dev/pcanusb0";

  printf("Enabling CAN\n");

  // Device is already open
  if(intf->handle) {
    return 0;
  }

  // Check whether the device exists
  struct stat buf;
  if(stat(device, &buf) == -1)
  {
    fprintf(stderr, "Error while locating device node (%s)\n", device);
    return -1;
  }

  // Device should be a character device
  if(!S_ISCHR(buf.st_mode))
  {
    fprintf(stderr, "Device node is not a character device (%s)\n", device);
    return -1;
  }

  // Try to open interface
  intf->handle = LINUX_CAN_Open(device, 0);

  if(!intf->handle) {
    fprintf(stderr, "Opening of CAN device failed\n");
    return -1;
  }

  // Initialize interface
  DWORD result = CAN_Init(intf->handle, CAN_BAUD_1M, CAN_INIT_TYPE_ST);

  if(result != CAN_ERR_OK && result != CAN_ERR_QRCVEMPTY)
  {
    fprintf(stderr, "Initializing of CAN device failed\n");	
    intf_close(intf);
    return -1;
  }
   
  // Register handle with libevent
  intf->fd = LINUX_CAN_FileHandle(intf->handle);
  intf->read_event = event_new(intf->ev_base, intf->fd, EV_READ | EV_ET | EV_PERSIST, intf_on_read, (void *) intf);
  event_add(intf->read_event, NULL);
}


/**
 * Close the device connection.
 */
int intf_close(intf_t *intf)
{
  // Remove event
  event_del(intf->read_event);
  event_free(intf->read_event);
  intf->read_event = NULL;
  
  // Close connection
  DWORD result = CAN_Close(intf->handle);
  if(result != CAN_ERR_OK) {
    fprintf(stderr, "Closing of CAN device failed\n");
    return -1;
  }
  
  intf->handle = 0;
  intf->fd = 0;
  
  return 0;
}


/**
 * Called when data is pending
 */
void intf_on_read(evutil_socket_t fd, short events, void *intf_v)
{
  intf_t *intf = (intf_t *) intf_v;
  
  // We've been closed down, stop.
  if(!intf || !intf->fd)
    return;
  
  TPCANRdMsg message;
  DWORD result;

  result = LINUX_CAN_Read(intf->handle, &message);

  // Receive queue was empty, no message read
  if(result == CAN_ERR_QRCVEMPTY) {
    fprintf(stderr, "CAN Queue was empty, but intf_on_read() was called.\n");
    return;
  }

  // There was an error
  if(result != CAN_ERR_OK) {
    fprintf(stderr, "Error while reading from CAN bus, closing down.\n");
    intf_close(intf);
    return;
  }

  // A status message was received
  if(message.Msg.MSGTYPE & MSGTYPE_STATUS)
  {
    int32_t status = int32_t(CAN_Status(intf->handle));

    if(status < 0) {      
      fprintf(stderr, "Received invalid status message (%x), closing down.\n", status);
      intf_close(intf);
      return;
    }

    printf("Received status %x\n", status);
    // Status received, ignore for now?

    return;
  }

  uint64_t timestamp = message.dwTime * 1000 + message.wUsec;

  // Tell the world we've received a message?
  //can_message msg;
  //msg.id = message.Msg.ID;
  //msg.type = message.Msg.MSGTYPE;
  //msg.len = message.Msg.LEN;

  //for(int i = 0; i < 8; i++)
  //  msg.data[i] = message.Msg.DATA[i];

  return;
}
