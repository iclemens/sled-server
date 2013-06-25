#ifndef __SLED_H__
#define __SLED_H__

extern "C" {

struct sled_t;

/**
 * Sled status
 */
enum status_t {
  sta_off,
  sta_preoperational,
  sta_operational,
  sta_outputenabled,
  sta_fault
};

/**
 * Position is specified in absolute or relative coordinates.
 */
enum position_type_t {
  pos_absolute,
  pos_relative_target,
  pos_relative_actual
};

/**
 * Blend next before before, or after end of current.
 */
enum blend_type_t {
  bln_none,
  bln_before,
  bln_after
};


// Opening and closing of connection to sled
sled_t *sled_create();
void sled_destroy(sled_t **sled);

// Motion profiles
int sled_profile_create(sled_t *sled);
int sled_profile_create_pt(sled_t *sled, double position, double time);
int sled_profile_execute(sled_t *sled, int profile);
int sled_profile_destroy(sled_t *sled, int profile);

int sled_profile_set_table(sled_t *sled, int profile, int table);
int sled_profile_set_target(sled_t *sled, int profile, position_type_t type, double position, double time);
int sled_profile_set_next(sled_t *sled, int profile, int next_profile, double delay, blend_type_t blend_type);

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

