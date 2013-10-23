#include "sled.h"
#include "sled_profile.h"
#include "sled_internal.h"


int sled_profile_create(sled_t *sled)
{
  return 0;
}


int sled_profile_create_pt(sled_t *sled, double position, double time)
{
  return 0;
}


int sled_profile_execute(sled_t *sled, int profile)
{
  return -1;
}


int sled_profile_destroy(sled_t *sled, int profile)
{
  return -1;
}


int sled_profiles_reset(sled_t *sled)
{
  return -1;
}


int sled_profile_write_pending_changes(sled_t *sled, int profile_id)
{
  return -1;
}


int sled_profile_set_table(sled_t *sled, int profile, int table)
{
  return -1;
}


int sled_profile_set_target(sled_t *sled, int profile, position_type_t type, double position, double time)
{
  return -1;
}


int sled_profile_set_next(sled_t *sled, int profile, int next_profile, double delay, blend_type_t blend_type)
{
  return -1;
}

