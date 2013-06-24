#ifndef __NDI_DATAFRAME_H__
#define __NDI_DATAFRAME_H__

#include<stdint.h>

struct ndi_3d_t
{
  float x;
  float y;
  float z;
  float reliability;
};

struct ndi_6d_t
{
  float q0;
  float qx;
  float qy;
  float qz;
  float x;
  float y;
  float error;
};

struct ndi_analog_t
{
  uint32_t voltage;
};

struct ndi_forceplate_t
{
  float fx; // Force
  float fy;
  float fz;
  float mx; // Moment
  float my;
  float mz;
};

struct ndi_event_t
{
  uint32_t id;
  uint32_t param1;
  uint32_t param2;
  uint32_t param3;
};


struct ndi_dataframe_t;
struct ndi_component_t;

ndi_dataframe_t *ndi_df_create_frame();
ndi_component_t *ndi_df_add_3d_component(ndi_dataframe_t *dataframe, uint32_t frame, uint64_t time, uint32_t num_markers, ndi_3d_t *data);
ndi_component_t *ndi_df_add_6d_component(ndi_dataframe_t *dataframe, uint32_t frame, uint64_t time, uint32_t num_tools, ndi_6d_t *data);
void ndi_df_send(ndi_connection_t *ndi_conn, ndi_dataframe_t *frame);

// Deprecated
void ndi_send_data(ndi_connection_t *ndi_conn, uint32_t frame, uint64_t time, float point);

#endif
