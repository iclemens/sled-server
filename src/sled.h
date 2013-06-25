#ifndef __SLED_H__
#define __SLED_H__

extern "C" {

struct sled_t;

// Opening and closing of connection to sled
sled_t *sled_create();
void sled_destroy(sled_t **sled);

// Set-points
int sled_rt_new_setpoint(sled_t *handle, double position);
int sled_rt_get_position(sled_t *handle, double &position);

// Sinusoids
int sled_sinusoid_start(sled_t *sled, double amplitude, double period);
int sled_sinusoid_stop(sled_t *sled);

// Light
int sled_light_set_state(sled_t *sled, bool state);

}

#endif

