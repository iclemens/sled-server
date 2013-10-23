#include "sled.h"
#include "sled_internal.h"

#include <stdlib.h>
#include <assert.h>

sled_t *sled_create(event_base *ev_base)
{
  sled_t *sled = (sled_t *) malloc(sizeof(sled_t));
  sled->time = 0;
  
  return sled;
}


void sled_destroy(sled_t **sled)
{
  assert(*sled);
  free(*sled);
  *sled = NULL;
}


int sled_rt_new_setpoint(sled_t *handle, double position)
{
  handle->time++;
  handle->position = position;
  
  return 0;
}


int sled_rt_get_position(sled_t *handle, double &position)
{
  position = handle->position;
  
  return 0;
}


int sled_rt_get_position_and_time(sled_t *handle, double &position, double &time)
{
  position = handle->position;
  time = handle->time;
  
  return 0;
}


int sled_sinusoid_start(sled_t *sled, double amplitude, double period)
{
  return -1;
}


int sled_sinusoid_stop(sled_t *sled)
{
  return -1;
}


int sled_light_set_state(sled_t *sled, bool state)
{
  return -1;
}


