#include "sled_internal.h"

#include <stdlib.h>
#include <stdio.h>


/**
 * Setup sled structures.
 */
sled_t *sled_create()
{
  sled_t *sled = (sled_t *) malloc(sizeof(sled_t));

  // Reset sled profiles
  for(int profile = 0; profile < MAX_PROFILES; profile++)
    sled_profile_clear(sled, profile, false);

  return sled;
}


void sled_destroy(sled_t **handle)
{
  sled_t *sled = *handle;
  free(handle);

  *handle = NULL;
}


// Set-points
int sled_rt_new_setpoint(sled_t *handle, double position)
{
  // No can do
  return -1;
}


int sled_rt_get_position(sled_t *handle, double &position)
{
  // Not implemented
  return -1;
}


// Sinusoids
int sled_sinusoid_start(sled_t *handle, double amplitude, double period)
{
  printf("sled_sinusoid_start(%f, %f)\n", amplitude, period);
  return -1;
}


int sled_sinusoid_stop(sled_t *handle)
{
  printf("sled_sinusoid_stop()\n");
  return -1;
}


// Light
int sled_light_set_state(sled_t *handle, bool state)
{
  printf("sled_light_set_state(%s)\n", state?"on":"off");
  return -1;
}

