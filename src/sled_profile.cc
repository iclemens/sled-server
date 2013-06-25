#include "sled_internal.h"

#include <stdio.h>


/**
 * Reset profile structure.
 */
void sled_profile_clear(sled_t *sled, int profile, bool in_use)
{
  if(profile < 0 || profile >= MAX_PROFILES)
    return;

  sled_profile_t *p = &(sled->profiles[profile]);

  p->profile = 200 + profile;
  p->in_use = in_use;

  p->table = 2;
  p->position_type = pos_absolute;
  p->position = 0.0;
  p->time = 1.0;

  p->next_profile = -1;
  p->delay = 0.0;
  p->blend_type = bln_none;
}


/**
 * Allocates a profile on the sled.
 *
 * Returns profile handle or -1 on failure.
 */
int sled_profile_create(sled_t *sled)
{
  for(int i = 0; i < MAX_PROFILES; i++) {
    if(!sled->profiles[i].in_use) {
      sled_profile_clear(sled, i, true);
      sled->profiles[i].in_use = true;
      return i;
    }
  }

  return -1;
}


/**
 * Allocates a profile and fills basic details.
 *
 * Returns profile handle or -1 on failure.
 */
int sled_profile_create_pt(sled_t *sled, double position, double time)
{
  int profile = sled_profile_create(sled);

  if(profile == -1)
    return -1;

  sled->profiles[profile].position = position;
  sled->profiles[profile].time = time;

  return profile;
}


/**
 * Destroys a sled profile, freeing it for future use.
 *
 * Returns 0 on success, -1 on failure (invalid profile)
 */
int sled_profile_destroy(sled_t *sled, int profile)
{
  if(profile < 0 || profile >= MAX_PROFILES)
    return -1;

  if(!sled->profiles[profile].in_use)
    return -1;

  sled->profiles[profile].in_use = false;

  return 0;
}


/**
 * Sets type of profile (sinusoid or minimum jerk)
 *
 * Returns 0 on success, -1 on failure (invalid profile)
 */
int sled_profile_set_table(sled_t *sled, int profile, int table)
{
  if(profile < 0 || profile >= MAX_PROFILES)
    return -1;

  if(!sled->profiles[profile].in_use)
    return -1;

  sled->profiles[profile].table = table;
  return 0;
}


/**
 * Sets profile target and time.
 *
 * Returns 0 on success, -1 on failure (invalid profile)
 */
int sled_profile_set_target(sled_t *sled, int profile, position_type_t type, double position, double time)
{
  if(profile < 0 || profile >= MAX_PROFILES)
    return -1;

  if(!sled->profiles[profile].in_use)
    return -1;

  sled->profiles[profile].position_type = type;
  sled->profiles[profile].position = position;
  sled->profiles[profile].time = time;
  return 0;
}


/**
 * Sets next profile and blending type.
 *
 * Returns 0 on success, -1 on failure (invalid profile)
 */
int sled_profile_set_next(sled_t *sled, int profile, int next_profile, double delay, blend_type_t blend_type)
{
  if(profile < 0 || profile >= MAX_PROFILES)
    return -1;

  if(!sled->profiles[profile].in_use)
    return -1;

  sled->profiles[profile].next_profile = next_profile;
  sled->profiles[profile].delay = delay;
  sled->profiles[profile].blend_type = blend_type;
  return 0;
}


/**
 * Execute specified profile.
 *
 * Returns 0 on success, -1 on failure (invalid profile)
 * Success only indicates that the command has been received.
 *
 * Fixme: we might want to return some kind of handle such that
 * the client can wait for completion.
 */
int sled_profile_execute(sled_t *sled, int profile)
{
  if(profile < 0 || profile >= MAX_PROFILES)
    return -1;

  if(!sled->profiles[profile].in_use)
    return -1;

  printf("sled_profile_execute(%d -> %d)\n", profile, sled->profiles[profile].profile);

  return -1;
}

