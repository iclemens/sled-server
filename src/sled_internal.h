#ifndef __SLED_INTERNAL_H__
#define __SLED_INTERNAL_H__

#include "sled.h"

#define MAX_PROFILES 100

struct sled_profile_t {
  bool in_use;
  int profile;

  int table;
  position_type_t position_type;
  double position, time;

  int next_profile;
  double delay;
  blend_type_t blend_type;
};

struct sled_t {
  sled_profile_t profiles[MAX_PROFILES];
};

void sled_profile_clear(sled_t *sled, int profile, bool in_use);

#endif

