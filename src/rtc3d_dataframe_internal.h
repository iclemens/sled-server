#ifndef __NDI_DATAFRAME_INTERNAL_H__
#define __NDI_DATAFRAME_INTERNAL_H__

struct rtc3d_component_t;

struct rtc3d_component_t 
{
  uint32_t size;    // Size in bytes (including 20 byte header)
  uint32_t type;    // Type of frame e.g. CTYPE_3D
  uint32_t frame;   // Frame number
  uint64_t time;    // Timestamp
  uint32_t count;   // Number of items in component

  union {
    rtc3d_3d_t *data_3d;
    rtc3d_6d_t *data_6d;
    rtc3d_analog_t *data_analog;
    rtc3d_forceplate_t *data_force;
    rtc3d_event_t *data_event;

    void *data;
  };

  rtc3d_component_t *next_component;
};

struct rtc3d_dataframe_t
{
  rtc3d_component_t *first_component;
};

#endif
