#ifndef __SLED_INTERNAL_H__
#define __SLED_INTERNAL_H__

#include "sled.h"
#include "sled_profile.h"

#include "interface.h"

#include "machines/mch_intf.h"
#include "machines/mch_net.h"
#include "machines/mch_sdo.h"
#include "machines/mch_ds.h"
#include "machines/mch_mp.h"

#define MAX_PROFILES 100

/**
 * Generates callback function for callback FNAME of the SNAME machine.
 * When executed it sends event EVENT to the DNAME machine.
 */
#define CALLBACK_FUNCTION_EVENT(SNAME, FNAME, DNAME, EVENT) \
	void mch_ ## SNAME ## _ ## FNAME (mch_ ## SNAME ## _t *mch_ ## SNAME, void *payload) { \
	sled_t *sled = (sled_t *) payload; \
	mch_ ## DNAME ## _handle_event(sled->mch_ ## DNAME, EVENT); }

/**
 * Registers callback function FNAME with state machine SNAME.
 */
#define REGISTER_CALLBACK(SNAME, FNAME) \
	mch_ ## SNAME ## _set_ ## FNAME ## _handler(sled->mch_ ## SNAME, mch_ ## SNAME ## _on_ ## FNAME);

struct event_base;

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
	// Local copy of motion profiles
	sled_profile_t profiles[MAX_PROFILES];

	// Lib event event base
	event_base *ev_base;

	// CANOpen interface
	intf_t *interface;

	// State machines
	mch_intf_t *mch_intf;
	mch_net_t *mch_net;
	mch_sdo_t *mch_sdo;
	mch_ds_t *mch_ds;
	mch_mp_t *mch_mp;

	// Last position and velocity
	double last_position, last_velocity;

	// Profiles for sinusoid
	int sinusoid_there, sinusoid_back;
};

void sled_profile_clear(sled_t *sled, int profile, bool in_use);

#endif

