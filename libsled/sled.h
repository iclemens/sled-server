#ifndef __SLED_H__
#define __SLED_H__

extern "C" {

struct event_base;
struct sled_t;

// Opening and closing of connection to sled
sled_t *sled_create(event_base *ev_base);
void sled_destroy(sled_t **sled);

// Set-points
int sled_rt_new_setpoint(sled_t *handle, double position);
int sled_rt_get_position(const sled_t *handle, double &position);
int sled_rt_get_position_and_time(const sled_t *handle, double &position, double &time);

// Sinusoids
int sled_sinusoid_start(sled_t *sled, double amplitude, double period);
int sled_sinusoid_stop(sled_t *sled);

// Light
int sled_light_set_state(sled_t *sled, bool state);

}

#endif

