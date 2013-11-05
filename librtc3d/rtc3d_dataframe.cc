
#ifdef WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

#include <stdio.h>
#include <string.h>
#include "rtc3d_internal.h"
#include "rtc3d_dataframe.h"
#include "rtc3d_dataframe_internal.h"


inline uint64_t htonll(uint64_t value)
{
  int num = 1;
  if(*(char *) &num == 1)
    return (uint64_t(htonl(value & 0xFFFFFFFF)) << 32) | htonl(value >> 32);
  else
    return value;
}


inline float htonf(float value)
{
  uint32_t *ptr = (uint32_t *) &value;
  *ptr = htonl(*ptr);
  return value;
}


inline void rtc3d_set_packet_header(char *buffer, uint32_t size, uint32_t type)
{
  uint32_t *_size = (uint32_t *) &(buffer[0]);
  uint32_t *_type = (uint32_t *) &(buffer[4]);

  *_size = htonl(size);
  *_type = htonl(type);
}


inline void rtc3d_set_component_header(char *buffer, uint32_t size, uint32_t type, uint32_t frame, uint64_t time)
{
  uint32_t *_size = (uint32_t *) &(buffer[0]);
  uint32_t *_type = (uint32_t *) &(buffer[4]);
  uint32_t *_frame = (uint32_t *) &(buffer[8]);
  uint64_t *_time = (uint64_t *) &(buffer[12]);

  *_size = htonl(size);
  *_type = htonl(type);
  *_frame = htonl(frame);
  *_time = htonll(time);
}


inline void rtc3d_set_marker(char *buffer, float x, float y, float z, float delta)
{
  float *_x = (float *) &(buffer[0]);
  float *_y = (float *) &(buffer[4]);
  float *_z = (float *) &(buffer[8]);
  float *_d = (float *) &(buffer[12]);

  *_x = htonf(x);
  *_y = htonf(y);
  *_z = htonf(z);
  *_d = htonf(delta);  
}


/**
 * Sends a single position.
 */
void rtc3d_send_data(rtc3d_connection_t *rtc3d_conn, uint32_t frame, uint64_t time, float point)
{
  char buffer[52];
  rtc3d_set_packet_header(buffer, 52, PTYPE_DATAFRAME);

  // Component count
  uint32_t *ccount = (uint32_t *) &(buffer[8]);
  *ccount = htonl(1);

  rtc3d_set_component_header(&(buffer[12]), 40, CTYPE_3D, frame, time);

  // Marker count
  uint32_t *mcount = (uint32_t *) &(buffer[32]);
  *mcount = htonl(1);

  rtc3d_set_marker(&(buffer[36]), point, 0, 0, 0);

  net_send(rtc3d_conn->net_conn, buffer, 8 + 4 + 20 + 4 + 16);
}

