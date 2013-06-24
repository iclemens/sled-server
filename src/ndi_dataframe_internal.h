#ifndef __NDI_DATAFRAME_INTERNAL_H__
#define __NDI_DATAFRAME_INTERNAL_H__

struct ndi_component_t;

struct ndi_component_t 
{
  uint32_t size;    // Size in bytes (including 20 byte header)
  uint32_t type;    // Type of frame e.g. CTYPE_3D
  uint32_t frame;   // Frame number
  uint64_t time;    // Timestamp
  uint32_t count;   // Number of items in component

  union {
    ndi_3d_t *data_3d;
    ndi_6d_t *data_6d;
    ndi_analog_t *data_analog;
    ndi_forceplate_t *data_force;
    ndi_event_t *data_event;

    void *data;
  };

  ndi_component_t *next_component;
};

struct ndi_dataframe_t
{
  ndi_component_t *first_component;
};

#endif
