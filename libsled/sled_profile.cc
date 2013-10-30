#include "sled_internal.h"

#include <syslog.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


/**
 * Compute control word for a profile.
 */
int sled_profile_get_controlword(sled_profile_t *profile)
{
	int control = 0;

	// Set bits 15, 2, 1 and 0 depending on position type
	switch(profile->position_type) {
		case pos_absolute: break;
		case pos_relative_actual: control |= 0x01 | 0x04; break;
		case pos_relative_target: control |= 0x01 | 0x02; break;
	}

	// Set bit 3 to indicate next motion task
	if(profile->next_profile >= 0)
		control |= 0x08;

	// Set bit 8, 7, 6, 5, 4 depending on blending
	switch(profile->blend_type) {
		case bln_none: break;
		case bln_before: control |= 0x100 | 0x10; break;
		case bln_after: control |= 0x10; break;
	}

	// Table vs trajectory generator (table)
	control |= 0x200;

	// Use SI units
	control |= 0x2000;

	return control;
}


/**
 * Marks a profile as unsaved. Changes will be written
 * to the device as soon as possible.
 */
void sled_profile_mark_as_unsaved(sled_profile_t *profile)
{
	assert(profile);

	profile->_ob_o_p   = FIELD_CHANGED;
	profile->_ob_o_v   = FIELD_CHANGED;
	profile->_ob_o_c   = FIELD_CHANGED;
	profile->_ob_o_acc = FIELD_CHANGED;
	profile->_ob_o_dec = FIELD_CHANGED;
	profile->_ob_o_tab = FIELD_CHANGED;
	profile->_ob_o_fn  = FIELD_CHANGED;
	profile->_ob_o_ft  = FIELD_CHANGED;
}


/**
 * Returns true when changes are pending. False if not.
 */
bool sled_profile_has_changes_pending(sled_profile_t *profile)
{
	assert(profile);

	if(profile->_ob_o_p   == FIELD_CHANGED) return true;
	if(profile->_ob_o_v   == FIELD_CHANGED) return true;
	if(profile->_ob_o_c   == FIELD_CHANGED) return true;
	if(profile->_ob_o_acc == FIELD_CHANGED) return true;
	if(profile->_ob_o_dec == FIELD_CHANGED) return true;
	if(profile->_ob_o_tab == FIELD_CHANGED) return true;
	if(profile->_ob_o_fn  == FIELD_CHANGED) return true;
	if(profile->_ob_o_ft  == FIELD_CHANGED) return true;

	return false;
}


/**
 * Set state of field identified by dictionary index.
 */
void sled_profile_set_field_state(sled_profile_t *profile, uint16_t index, field_state_t state)
{
	assert(profile);

	switch(index) {
		case OB_O_P:   profile->_ob_o_p   = state; break;
		case OB_O_V:   profile->_ob_o_v   = state; break;
		case OB_O_C:   profile->_ob_o_c   = state; break;
		case OB_O_ACC: profile->_ob_o_acc = state; break;
		case OB_O_DEC: profile->_ob_o_dec = state; break;
		case OB_O_TAB: profile->_ob_o_tab = state; break;
		case OB_O_FN:  profile->_ob_o_fn  = state; break;
		case OB_O_FT:  profile->_ob_o_ft  = state; break;
	}
}


/**
 * Mark field as written.
 */
void on_success_callback(void *data, uint16_t index, uint8_t subindex)
{
	sled_profile_t *profile = (sled_profile_t *) data;
	sled_profile_set_field_state(profile, index, FIELD_WRITTEN);
}


/**
 * Mark field as invalid (write failed).
 */
void on_failure_callback(void *data, uint16_t index, uint8_t subindex, uint32_t abort)
{
	sled_profile_t *profile = (sled_profile_t *) data;
	sled_profile_set_field_state(profile, index, FIELD_INVALID);

	syslog(LOG_ERR, "%s() uploading of profile failed \
			abort code %04x on index %04x:%02x",
			__FUNCTION__, abort, index, subindex);

	exit(1);
}


#define WRITE_FIELD_IF_CHANGED(name, index, value) \
	if(profile->_ ## name == FIELD_CHANGED) { \
		profile->_ ## name = FIELD_WRITING; \
		mch_sdo_queue_write_with_cb( \
				sled->mch_sdo, index, 0x01, value, 0x04, \
				on_success_callback, on_failure_callback, (void *) profile \
				); \
	}


#define COPY_MOTION_TASK(from, to) \
	mch_sdo_queue_write(sled->mch_sdo, OB_COPY_MOTION_TASK, 0x0, (from & 0xFFFF) | ((to & 0xFFFF) << 16), 0x04);


/**
 * Writes all pending changes to the device.
 */
int sled_profile_write_pending_changes(sled_t *sled, int profile_id)
{
	sled_profile_t *profile = &(sled->profiles[profile_id]);

	assert(sled && profile);

	// No changes pending, bail out
	if(!sled_profile_has_changes_pending(profile))
		return 0;

	// Load correct profile in slot 0
	if(sled->current_profile != profile->profile) {
		COPY_MOTION_TASK(profile->profile, 0x00)
			sled->current_profile = profile->profile;
	}

	WRITE_FIELD_IF_CHANGED(ob_o_p,   OB_O_P,   int32_t(profile->position * 1000.0 * 1000.0));
	WRITE_FIELD_IF_CHANGED(ob_o_v,   OB_O_V,   0x00);
	WRITE_FIELD_IF_CHANGED(ob_o_c,   OB_O_C,   sled_profile_get_controlword(profile));
	WRITE_FIELD_IF_CHANGED(ob_o_acc, OB_O_ACC, int32_t(profile->time * 1000.0 / 2.0 + 0.5));

	// Subtract one to compensate for the controller being 1ms late.
	WRITE_FIELD_IF_CHANGED(ob_o_dec, OB_O_DEC, int32_t(profile->time * 1000.0 / 2.0 + 0.5) - 1);
	WRITE_FIELD_IF_CHANGED(ob_o_tab, OB_O_TAB, profile->table);

	int next_profile = 0;
	if(profile->next_profile >= 0)
		next_profile = sled->profiles[profile->next_profile].profile;

	WRITE_FIELD_IF_CHANGED(ob_o_fn,  OB_O_FN,  next_profile);
	WRITE_FIELD_IF_CHANGED(ob_o_ft,  OB_O_FT,  int32_t(profile->delay * 1000.0));

	// Copy back to profile (this could be defered to a later time)
	COPY_MOTION_TASK(0x00, profile->profile);

	// Write profiles that the current profile depends on...
	if(profile->next_profile >= 0)
		sled_profile_write_pending_changes(sled, profile->next_profile);

	return 0;
}


/**
 * Reset profile structure.
 */
void sled_profile_clear(sled_t *sled, int profile, bool in_use)
{
	if(profile < 0 || profile >= MAX_PROFILES)
		return;

	sled_profile_t *p = &(sled->profiles[profile]);

	p->profile = 201 + profile;
	p->in_use = in_use;

	p->table = 2;
	p->position_type = pos_absolute;
	p->position = 0.0;
	p->time = 1.0;

	p->next_profile = -1;
	p->delay = 0.0;
	p->blend_type = bln_none;

	sled_profile_mark_as_unsaved(p);
}


/**
 * Mark all used profiles as unsaved.
 */
int sled_profiles_reset(sled_t *sled)
{
	for(int i = 0; i < MAX_PROFILES; i++) {
		sled_profile_t *p = &(sled->profiles[i]);
		if(p->in_use)
			sled_profile_mark_as_unsaved(p);
	}

	return 0;
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

	if(sled->profiles[profile].table != table) {
		sled->profiles[profile].table = table;
		sled->profiles[profile]._ob_o_tab = FIELD_CHANGED;
	}

	return 0;
}


/**
 * Sets profile target and time.
 *
 * Returns 0 on success, -1 on failure (invalid profile)
 */
int sled_profile_set_target(sled_t *sled, int profile, position_type_t type, double position, double time)
{
	if(profile < 0 || profile >= MAX_PROFILES) {
		syslog(LOG_ERR, "%s(%d, ...) invalid profile",
				__FUNCTION__, profile);
		return -1;
	}

	if(!sled->profiles[profile].in_use) {
		syslog(LOG_ERR, "%s(%d, ...) profile not in use",
				__FUNCTION__, profile);
		return -1;
	}

	if(sled->profiles[profile].position_type != type) {
		sled->profiles[profile]._ob_o_c = FIELD_CHANGED;
		sled->profiles[profile].position_type = type;
	}

	if(sled->profiles[profile].position != position) {
		sled->profiles[profile]._ob_o_p = FIELD_CHANGED;
		sled->profiles[profile].position = position;
	}

	if(sled->profiles[profile].time != time) {
		sled->profiles[profile]._ob_o_acc = FIELD_CHANGED;
		sled->profiles[profile]._ob_o_dec = FIELD_CHANGED;
		sled->profiles[profile].time = time;
	}

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

	if(sled->profiles[profile].next_profile != next_profile) {
		sled->profiles[profile]._ob_o_c = FIELD_CHANGED;
		sled->profiles[profile]._ob_o_fn = FIELD_CHANGED;
		sled->profiles[profile].next_profile = next_profile;
	}

	if(sled->profiles[profile].delay != delay) {
		sled->profiles[profile]._ob_o_c = FIELD_CHANGED;
		sled->profiles[profile]._ob_o_ft = FIELD_CHANGED;
		sled->profiles[profile].delay = delay;
	}

	if(sled->profiles[profile].blend_type != blend_type) {
		sled->profiles[profile]._ob_o_c = FIELD_CHANGED;
		sled->profiles[profile].blend_type = blend_type;
	}

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

	syslog(LOG_DEBUG, "%s(%d) internal number: %d", __FUNCTION__,
			profile, sled->profiles[profile].profile);

	if(mch_mp_active_state(sled->mch_mp) != ST_MP_PP_IDLE) {
		syslog(LOG_ERR, "%s(%d) unable to execute, motor not idle",
				__FUNCTION__, profile);
		return -1;
	}

	sled_profile_write_pending_changes(sled, profile);

	// Set motion profile to be executed
	mch_sdo_queue_write(sled->mch_sdo, OB_MOTION_TASK, 0x00, sled->profiles[profile].profile, 0x02);
	mch_sdo_queue_write(sled->mch_sdo, OB_CONTROL_WORD, 0x00, 0x1F | 0x20, 0x02);

	mch_mp_handle_event(sled->mch_mp, EV_MP_SETPOINT_SET);

	return 0;
}
