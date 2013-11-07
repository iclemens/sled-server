#include "rtc3d_dataframe.h"
#include "rtc3d_internal.h"

#ifdef WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

#include <stdio.h>
#include <string.h>


static uint64_t htonll(uint64_t value)
{
  int num = 1;
  if(*(char *) &num == 1)
    return (uint64_t(htonl(value & 0xFFFFFFFF)) << 32) | htonl(value >> 32);
  else
    return value;
}


static float htonf(float value)
{
  uint32_t * const ptr = (uint32_t *) &value;
  *ptr = htonl(*ptr);
  return value;
}


static void rtc3d_set_packet_header(char *buffer, uint32_t size, uint32_t type)
{
  packet_header_t *header = (packet_header_t *) buffer;  
  header->size = htonl(size);
  header->type = htonl(type);
}


static void rtc3d_set_component_header(char *buffer, uint32_t size, uint32_t type, uint32_t frame, uint64_t time)
{
  component_header_t *header = (component_header_t *) buffer;
  header->size = htonl(size);
  header->type = htonl(type);
  header->frame = htonl(frame);
  header->time = htonll(time);  
}


static void rtc3d_set_marker(char *buffer, float x, float y, float z, float delta)
{
  frame_3d_t *frame = (frame_3d_t *) buffer;
  frame->x = htonf(x);
  frame->y = htonf(y);
  frame->z = htonf(z);
  frame->reliability = htonf(delta);
}


/**
 * Sends a single position.
 */
void rtc3d_send_data(rtc3d_connection_t *rtc3d_conn, uint32_t frame, uint64_t time, float point)
{
  char buffer[52];
  rtc3d_set_packet_header(buffer, 52, PTYPE_DATAFRAME);

  // Component count
  uint32_t * const ccount = (uint32_t *) &(buffer[8]);
  *ccount = htonl(1);

  rtc3d_set_component_header(&(buffer[12]), 40, CTYPE_3D, frame, time);

  // Marker count
  uint32_t * const mcount = (uint32_t *) &(buffer[32]);
  *mcount = htonl(1);

  rtc3d_set_marker(&(buffer[36]), point, 0, 0, 0);

  net_send(rtc3d_conn->net_conn, buffer, 8 + 4 + 20 + 4 + 16);
}
