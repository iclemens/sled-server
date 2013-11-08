#ifndef __NDI_DATAFRAME_H__
#define __NDI_DATAFRAME_H__

#include <stdint.h>

struct rtc3d_connection_t;

void rtc3d_send_data(rtc3d_connection_t *rtc3d_conn, uint32_t frame, uint64_t time, float point);

#endif
