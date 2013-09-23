#ifndef __NDI_DATAFRAME_H__
#define __NDI_DATAFRAME_H__

#include <stdint.h>

struct rtc3d_3d_t
{
	float x;
	float y;
	float z;
	float reliability;
};

struct rtc3d_6d_t
{
	float q0;
	float qx;
	float qy;
	float qz;
	float x;
	float y;
	float error;
};

struct rtc3d_analog_t
{
	uint32_t voltage;
};

struct rtc3d_forceplate_t
{
	float fx; // Force
	float fy;
	float fz;
	float mx; // Moment
	float my;
	float mz;
};

struct rtc3d_event_t
{
	uint32_t id;
	uint32_t param1;
	uint32_t param2;
	uint32_t param3;
};


struct rtc3d_dataframe_t;
struct rtc3d_component_t;

rtc3d_dataframe_t *rtc3d_df_create_frame();
rtc3d_component_t *rtc3d_df_add_3d_component(rtc3d_dataframe_t *dataframe, uint32_t frame, uint64_t time, uint32_t num_markers, rtc3d_3d_t *data);
rtc3d_component_t *rtc3d_df_add_6d_component(rtc3d_dataframe_t *dataframe, uint32_t frame, uint64_t time, uint32_t num_tools, rtc3d_6d_t *data);
void rtc3d_df_send(rtc3d_connection_t *rtc3d_conn, rtc3d_dataframe_t *frame);

// Deprecated
void rtc3d_send_data(rtc3d_connection_t *rtc3d_conn, uint32_t frame, uint64_t time, float point);

#endif
