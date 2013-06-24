
#ifdef WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

#include <string.h>
#include "ndi_internal.h"
#include "ndi_dataframe.h"
#include "ndi_dataframe_internal.h"


uint64_t htonll(uint64_t value)
{
  int num = 1;
  if(*(char *) &num == 1)
    return (uint64_t(htonl(value & 0xFFFFFFFF)) << 32) | htonl(value >> 32);
  else
    return value;
}


/**
 * Determines size of [count] items of type [type].
 */
uint32_t compute_data_size(uint32_t type, uint32_t count)
{
  uint32_t single;
  switch(type) {
    case CTYPE_3D: return count * sizeof(ndi_3d_t);
    case CTYPE_ANALOG: return count * sizeof(ndi_analog_t);
    case CTYPE_FORCE: return count * sizeof(ndi_forceplate_t);
    case CTYPE_6D: return count * sizeof(ndi_6d_t);
    case CTYPE_EVENT: return count * sizeof(ndi_event_t);
  }

  return 0;
}


/**
 * Sets-up dataframe component
 */
ndi_component_t *ndi_df_create_component(uint32_t type, uint32_t frame, uint64_t time, uint32_t count, const void *data)
{
  ndi_component_t *component = (ndi_component_t *) malloc(sizeof(ndi_component_t));

  // Compute data size
  uint32_t data_size = compute_data_size(type, count);

  component->size = 20 + 4 + data_size;
  component->type = type;
  component->frame = frame;
  component->time = time;
  component->count = count; // Number of records

  // Allocate and copy component data
  component->data = (void *) malloc(data_size);
  memcpy(component->data, data, data_size);

  // No next component
  component->next_component = NULL;

  return component;
}


/**
 * Frees memory associated with a single component
 */
void ndi_df_destroy_component(ndi_component_t **component_v)
{
  ndi_component_t *component = *component_v;

  if(component->data)
    free(component->data);

  free(*component_v);
  *component_v = NULL;
}


/**
 * Create dataframe structure without any components
 */
ndi_dataframe_t *ndi_df_create_frame()
{
  ndi_dataframe_t *frame = (ndi_dataframe_t *) malloc(sizeof(ndi_dataframe_t));
  frame->first_component = NULL;
  return frame;
}


/**
 * Frees memory associated with data frame
 */
void ndi_df_destroy_frame(ndi_dataframe_t **dataframe)
{
  // First make sure all components are destroyed
  ndi_component_t *component = (*dataframe)->first_component;
  while(component) {
    ndi_component_t *next_component = component->next_component;
    ndi_df_destroy_component(&component);
    component = next_component;
  }

  // Then free frame
  free(*dataframe);
  *dataframe = NULL;
}


/**
 * Adds a component to the dataframe.
 */
void ndi_df_add_component(ndi_dataframe_t *dataframe, ndi_component_t *component)
{
  component->next_component = dataframe->first_component;
  dataframe->first_component = component;
}


/**
 * Add 3D component to dataframe
 */
ndi_component_t *ndi_df_add_3d_component(ndi_dataframe_t *dataframe, uint32_t frame, uint64_t time, uint32_t num_markers, ndi_3d_t *data)
{
  ndi_component_t *component = ndi_df_create_component(CTYPE_3D, frame, time, num_markers, (void *) data);
  ndi_df_add_component(dataframe, component);
  return component;
}


/**
 * Add 6D component to dataframe
 */
ndi_component_t *ndi_df_add_6d_component(ndi_dataframe_t *dataframe, uint32_t frame, uint64_t time, uint32_t num_tools, ndi_6d_t *data)
{
  ndi_component_t *component = ndi_df_create_component(CTYPE_6D, frame, time, num_tools, (void *) data);
  ndi_df_add_component(dataframe, component);
  return component;
}

#define write_uint32(b, p, v, e) *((uint32_t *)&b[p]) = (e==byo_big_endian?htonl(v):v); p += 4;
#define write_uint64(b, p, v, e) *((uint64_t *)&b[p]) = (e==byo_big_endian?htonll(v):v); p += 8;

void ndi_df_send(ndi_connection_t *ndi_conn, ndi_dataframe_t *frame)
{
  uint32_t num_components = 0;
  uint32_t component_size = 0;

  ndi_component_t *component = frame->first_component;
  while(component) {
    num_components++;
    component_size += component->size;
    component = component->next_component;
  }

  // Allocate buffer
  char *buffer =  (char *) malloc(8 + 4 + component_size);

  // Packet header
  uint32_t ptr = 0;
  write_uint32(buffer, ptr, 8 + 4 + component_size, byo_big_endian);
  write_uint32(buffer, ptr, PTYPE_DATAFRAME, byo_big_endian);

  // Dataframe header
  write_uint32(buffer, ptr, num_components, ndi_conn->byte_order);

  component = frame->first_component;
  while(component) {
    // Write component header
    write_uint32(buffer, ptr, component->size, ndi_conn->byte_order);
    write_uint32(buffer, ptr, component->type, ndi_conn->byte_order);
    write_uint32(buffer, ptr, component->frame, ndi_conn->byte_order);
    write_uint64(buffer, ptr, component->time, ndi_conn->byte_order);

    write_uint32(buffer, ptr, component->count, ndi_conn->byte_order);

    // Write component
    memcpy(&(buffer[ptr]), component->data, component->size - 24);
    uint32_t *data = (uint32_t *) &(buffer[ptr]);

    if(ndi_conn->byte_order == byo_big_endian) {
      for(int i = 0; i < (component->size / 4); i++)
        data[i] = htonl(data[i]);
    }

    component = component->next_component;
  }

  net_send(ndi_conn->net_conn, buffer, 8 + 4 + component_size, FADOPTBUFFER);
}


/**
 * Old way to send data...
 */
void ndi_send_data(ndi_connection_t *ndi_conn, uint32_t frame, uint64_t time, float point)
{
  char *buffer = (char *) malloc(8 + 4 + 20 + 4 + 16);

  uint32_t *size = (uint32_t *) &(buffer[0]);
  uint32_t *type = (uint32_t *) &(buffer[4]);

  uint32_t *ccount = (uint32_t *) &(buffer[8]);

  // Component 1
  uint32_t *csize = (uint32_t *) &(buffer[12]);
  uint32_t *ctype = (uint32_t *) &(buffer[16]);
  uint32_t *cframe = (uint32_t *) &(buffer[20]);
  uint64_t *ctime = (uint64_t *) &(buffer[24]);

  uint32_t *mcount = (uint32_t *) &(buffer[32]);
  float *x = (float *) &(buffer[36]);
  uint32_t *xi = (uint32_t *) &(buffer[36]);
  float *y = (float *) &(buffer[40]);
  float *z = (float *) &(buffer[44]);
  float *delta = (float *) &(buffer[48]);

  *size = htonl(8 + 4 + 20 + 4 + 16);
  *type = htonl(PTYPE_DATAFRAME);

  *ccount = htonl(1);

  *csize = htonl(20 + 4 + 16);
  *ctype = htonl(CTYPE_3D);
  *cframe = htonl(frame);
  *ctime = htonll(time);

  *mcount = htonl(1);
  *x = point;
  *xi = htonl(*xi);


}

