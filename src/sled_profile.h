#ifndef __SLED_PROFILE_H__
#define __SLED_PROFILE_H__

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

// Motion profiles
int sled_profile_create(sled_t *sled);
int sled_profile_create_pt(sled_t *sled, double position, double time);
int sled_profile_execute(sled_t *sled, int profile);
int sled_profile_destroy(sled_t *sled, int profile);

int sled_profiles_reset(sled_t *sled);
int sled_profile_write_pending_changes(sled_t *sled, int profile_id);

int sled_profile_set_table(sled_t *sled, int profile, int table);
int sled_profile_set_target(sled_t *sled, int profile, position_type_t type, double position, double time);
int sled_profile_set_next(sled_t *sled, int profile, int next_profile, double delay, blend_type_t blend_type);

#endif
