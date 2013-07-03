#include "sled_internal.h"

#include <stdio.h>


#define OB_O_P		0x35BE
#define OB_O_V		0x35BF
#define OB_O_C		0x35B9
#define OB_O_ACC	0x35B7
#define OB_O_DEC	0x35BA
#define OB_O_TAB	0x35B8
#define OB_O_FN		0x35BC
#define OB_O_FT		0x35BD
#define OB_CopyMotionTasks 	0x2082
#define OB_Move		0x3642


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

	if(mch_mp_active_state(sled->mch_mp) != ST_MP_PP_IDLE) {
		fprintf(stderr, "Unable to execute motion task, motor not idle.\n");
		return -1;
	}

	int position = 1000.0 * 1000.0 * sled->profiles[profile].position;	// In micrometers
	int time = 1000.0 * sled->profiles[profile].time;						// In milliseconds
	int control = 0;

	// Set bits 15, 2, 1 and 0 depending on position type
	switch(sled->profiles[profile].position_type) {
		case pos_absolute: break;
		case pos_relative_actual: control |= 0x01 | 0x04; break;
		case pos_relative_target: control |= 0x01 | 0x02; break;
	}

	// Set bit 3 to indicate next motion task
	if(sled->profiles[profile].next_profile >= 0)
		control |= 0x08;

	// Set bit 8, 7, 6, 5, 4 depending on blending
	switch(sled->profiles[profile].blend_type) {
		case bln_none: break;
		case bln_before: control |= 0x100 | 0x10; break;
		case bln_after: control |= 0x10; break;
	}

	// Table vs trajectory generator (table)
	control |= 0x200;

	// Use SI units
	control |= 0x2000;

	// Send command to sled
	mch_sdo_queue_write(sled->mch_sdo, OB_O_P, 0x01, position, 0x04);
	mch_sdo_queue_write(sled->mch_sdo, OB_O_V, 0x01, 0x00, 0x04);
	mch_sdo_queue_write(sled->mch_sdo, OB_O_C, 0x01, control, 0x04);
	mch_sdo_queue_write(sled->mch_sdo, OB_O_ACC, 0x01, time / 2, 0x04);
	mch_sdo_queue_write(sled->mch_sdo, OB_O_DEC, 0x01, time / 2, 0x04);
	mch_sdo_queue_write(sled->mch_sdo, OB_O_TAB, 0x01, sled->profiles[profile].table, 0x04);

	// Only send next profile if it is set
	if(sled->profiles[profile].next_profile >= 0) {
		mch_sdo_queue_write(sled->mch_sdo, OB_O_FN, 0x01, sled->profiles[profile].next_profile, 0x04);
		mch_sdo_queue_write(sled->mch_sdo, OB_O_FT, 0x01, sled->profiles[profile].delay, 0x04);
	}

	mch_sdo_queue_write(sled->mch_sdo, OB_CopyMotionTasks, 0x0, (sled->profiles[profile].profile && 0xFFFF) << 16 , 0x04);
	mch_sdo_queue_write(sled->mch_sdo, OB_Move, 0x01, sled->profiles[profile].profile , 0x04);

	return -1;
}

