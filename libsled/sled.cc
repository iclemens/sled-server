#include "sled_internal.h"

#include <assert.h>
#include <event2/event.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <time.h>


/* Maximum amount of seconds between NMT messages */
#define MAX_NMT_DELAY 2.0


static double get_time()
{
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return double(ts.tv_sec) + double(ts.tv_nsec) / 1000.0 / 1000.0 / 1000.0;
}


/** ******************
 * Callback functions
 ****************** **/


/**
 * Inform interface state machine that interface has closed.
 */
static void intf_on_close(intf_t *intf, void *payload)
{
	sled_t *sled = (sled_t *) payload;
	mch_intf_handle_event(sled->mch_intf, EV_INTF_CLOSE);
}


/**
 * Handle network management state notification.
 *
 * @param intf  Interface that changed state
 * @param payload  Void pointer to state machine struct
 * @param state  New state
 */
static void intf_on_nmt(intf_t *intf, void *payload, uint8_t state)
{
	sled_t *sled = (sled_t *) payload;

	switch(state) {
		case 0x04: mch_net_handle_event(sled->mch_net, EV_NET_STOPPED); break;
		case 0x05: mch_net_handle_event(sled->mch_net, EV_NET_OPERATIONAL); break;
		case 0x7F: mch_net_handle_event(sled->mch_net, EV_NET_PREOPERATIONAL); break;
	}

	sled->time_last_nmt_msg = get_time();
}


/**
 * Handle TPDO.
 */
static void intf_on_tpdo(intf_t *intf, void *payload, int pdo, uint8_t *data)
{
	sled_t *sled = (sled_t *) payload;

	if(pdo == 1) {
		uint16_t status = (data[1] << 8) | data[0];
		uint8_t mode = data[2];

		if((status & 0x4F) == 0x40) mch_ds_handle_event(sled->mch_ds, EV_DS_NOT_READY_TO_SWITCH_ON);
		if((status & 0x6F) == 0x21) mch_ds_handle_event(sled->mch_ds, EV_DS_READY_TO_SWITCH_ON);
		if((status & 0x6F) == 0x23) mch_ds_handle_event(sled->mch_ds, EV_DS_SWITCHED_ON);
		if((status & 0x6F) == 0x27) mch_ds_handle_event(sled->mch_ds, EV_DS_OPERATION_ENABLED);
		if((status & 0x4F) == 0x08) mch_ds_handle_event(sled->mch_ds, EV_DS_FAULT);
		if((status & 0x4F) == 0x0F) mch_ds_handle_event(sled->mch_ds, EV_DS_FAULT_REACTION_ACTIVE);
		if((status & 0x6F) == 0x07) mch_ds_handle_event(sled->mch_ds, EV_DS_QUICK_STOP_ACTIVE);

		if((status & 0x10) == 0x10)
			mch_ds_handle_event(sled->mch_ds, EV_DS_VOLTAGE_ENABLED);
		else
			mch_ds_handle_event(sled->mch_ds, EV_DS_VOLTAGE_DISABLED);

		if((status & 0x400) == 0x400)
			mch_mp_handle_event(sled->mch_mp, EV_MP_TARGET_REACHED);

		// Profile position (PP) mode
		if(mode == 0x01) {
			mch_mp_handle_event(sled->mch_mp, EV_MP_MODE_PP);

			if((status & 0x1000) == 0x1000)
				mch_mp_handle_event(sled->mch_mp, EV_MP_SETPOINT_ACK);
			else
				mch_mp_handle_event(sled->mch_mp, EV_MP_SETPOINT_NACK);
		}

		// Homing mode
		if(mode == 0x06) {
			mch_mp_handle_event(sled->mch_mp, EV_MP_MODE_HOMING);

			if((status & 0x1000) == 0x1000)
				mch_mp_handle_event(sled->mch_mp, EV_MP_HOMED);
			else
				mch_mp_handle_event(sled->mch_mp, EV_MP_NOTHOMED);
		}
	}

	if(pdo == 2) {
		int32_t position = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];
		int32_t velocity = (data[7] << 24) | (data[6] << 16) | (data[5] << 8) | data[4];

		sled->last_time = get_time();
		sled->last_position = position / 1000.0 / 1000.0;
		sled->last_velocity = velocity / 1000.0 / 1000.0;
	}
}


static void nmt_watchdog(evutil_socket_t fd, short flags, void *param)
{
	static bool message_written = false;

	sled_t *sled = (sled_t *) param;
	double delta = get_time() - sled->time_last_nmt_msg;

	// More than two seconds ago...
	if(delta > MAX_NMT_DELAY) {
		if(!message_written) {
			syslog(LOG_ERR, "%s() last NMT message was received "
				"%.2f seconds ago where only %.2f seconds are allowed",
				__FUNCTION__, delta, MAX_NMT_DELAY);
			message_written = true;
		}
		mch_net_handle_event(sled->mch_net, EV_NET_WATCHDOG_FAILED);
	} else {
		if(message_written) {
			syslog(LOG_NOTICE, "%s() NMT message received",
				__FUNCTION__);
			message_written = false;
		}
	}
}


// Inform network interface that the interface has opened / closed.
CALLBACK_FUNCTION_EVENT(intf, on_opened, net, EV_NET_INTF_OPENED);
CALLBACK_FUNCTION_EVENT(intf, on_closed, net, EV_NET_INTF_CLOSED);

// Inform SDO machine that SDO transmission is (not) possible.
CALLBACK_FUNCTION_EVENT(net, on_sdos_enabled, sdo, EV_NET_SDO_ENABLED);
CALLBACK_FUNCTION_EVENT(net, on_sdos_disabled, sdo, EV_NET_SDO_DISABLED);

// Notify DS402 that NMT is (in)operational.
void mch_net_on_enter_operational(mch_net_t *mch_net, void *payload)
{
	sled_t *sled = (sled_t *) payload;
	sled_profiles_reset(sled);
	mch_ds_handle_event(sled->mch_ds, EV_DS_NET_OPERATIONAL);
}

CALLBACK_FUNCTION_EVENT(net, on_leave_operational, ds, EV_DS_NET_INOPERATIONAL);

// Inform MP machine that DS is (in)operational.
CALLBACK_FUNCTION_EVENT(ds, on_operation_enabled, mp, EV_MP_DS_OPERATIONAL);
CALLBACK_FUNCTION_EVENT(ds, on_operation_disabled, mp, EV_MP_DS_INOPERATIONAL);


/**
 * Setup state machines
 */
static void setup_state_machines(sled_t *sled)
{
	// Setup state machines
	sled->mch_intf = mch_intf_create(sled->interface);
	sled->mch_sdo = mch_sdo_create(sled->interface);
	sled->mch_net = mch_net_create(sled->interface, sled->mch_sdo);
	sled->mch_ds = mch_ds_create(sled->interface, sled->mch_sdo);
	sled->mch_mp = mch_mp_create(sled->interface, sled->mch_sdo);

	// Register callback payload
	mch_intf_set_callback_payload(sled->mch_intf, (void *) sled);
	mch_net_set_callback_payload(sled->mch_net, (void *) sled);
	mch_sdo_set_callback_payload(sled->mch_sdo, (void *) sled);
	mch_ds_set_callback_payload(sled->mch_ds, (void *) sled);

	// Register callback functions for state machines.
	REGISTER_CALLBACK(intf, opened);
	REGISTER_CALLBACK(intf, closed);
	REGISTER_CALLBACK(net, sdos_enabled);
	REGISTER_CALLBACK(net, sdos_disabled);
	REGISTER_CALLBACK(net, enter_operational);
	REGISTER_CALLBACK(net, leave_operational);
	REGISTER_CALLBACK(ds, operation_enabled);
	REGISTER_CALLBACK(ds, operation_disabled);
}


/** *******************************
 * Implementation of API functions
 ******************************* **/


/**
 * Setup sled structures.
 */
sled_t *sled_create(event_base *ev_base)
{
	sled_t *sled = (sled_t *) malloc(sizeof(sled_t));
	sled->ev_base = ev_base;

	/* Make sure the watchdog times out */
	sled->time_last_nmt_msg = get_time() - MAX_NMT_DELAY;

	////////////////////////
	// Initialise profiles

	// Reset sled profiles
	for(int profile = 0; profile < MAX_PROFILES; profile++)
		sled_profile_clear(sled, profile, false);

	// Register sinusoid profiles
	sled->sinusoid_there = sled_profile_create(sled);
	sled->sinusoid_back = sled_profile_create(sled);

	sled_profile_set_table(sled, sled->sinusoid_there, 0);
	sled_profile_set_table(sled, sled->sinusoid_back, 0);
	sled_profile_set_next(sled, sled->sinusoid_there, sled->sinusoid_back, 0.0, bln_after);
	sled_profile_set_next(sled, sled->sinusoid_back, sled->sinusoid_there, 0.0, bln_after);

	////////////////////////////
	// Interface-specific part

	// Create interface
	sled->interface = intf_create(sled->ev_base);
	intf_set_callback_payload(sled->interface, (void *) sled);

	// Setup state machines
	setup_state_machines(sled);

	// Register interface callback functions
	intf_set_close_handler(sled->interface, intf_on_close);
	intf_set_nmt_state_handler(sled->interface, intf_on_nmt);
	intf_set_tpdo_handler(sled->interface, intf_on_tpdo);

	// Create watch-dog timer
	timeval watchdog_timeout;
	watchdog_timeout.tv_sec = 3;
	watchdog_timeout.tv_usec = 0;

	sled->watchdog = 
		event_new(ev_base, -1, EV_PERSIST, nmt_watchdog, (void *) sled);
	event_priority_set(sled->watchdog, 0);	/* Important */
	event_add(sled->watchdog, &watchdog_timeout);

	// Open interface
	mch_intf_handle_event(sled->mch_intf, EV_INTF_OPEN);

	return sled;
}


/**
 * Free sled structures.
 *
 * @param handle  Sled handle.
 */
void sled_destroy(sled_t **handle)
{
	sled_t *sled = *handle;

	free(*handle);
	*handle = NULL;
}


/**
 * Set new setpoint.
 *
 * @param handle  Sled handle.
 * @param position  Position set-point in meters.
 */
int sled_rt_new_setpoint(sled_t *handle, double position)
{
	// No can do
	return -1;
}


/**
 * Returns current sled position and time it was received.
 *
 * If not current position is available, the position field
 * is set to NAN, time will be set to the current time and
 * the function will return -1.
 */
int sled_rt_get_position_and_time(const sled_t *handle, double &position, double &time)
{
	assert(handle);

	if(mch_net_active_state(handle->mch_net) != ST_NET_OPERATIONAL) {
    time = get_time();
    position = NAN;
		return -1;
  }

	time = handle->last_time;
	position = handle->last_position;

	return 0;
}


/**
 * Returns current sled position.
 *
 * @param handle  Sled handle.
 * @param position  Position (by-reference) in meters.
 */
int sled_rt_get_position(const sled_t *handle, double &position)
{
	assert(handle);

	// Position is only valid if sled is operational
	if(mch_net_active_state(handle->mch_net) != ST_NET_OPERATIONAL)
		return -1;

	position = handle->last_position;
	return 0;
}


/**
 * Start sinusoidal motion.
 *
 * @param handle  Sled handle.
 * @param amplitude  Amplitude of sinusoid in meters
 * @param period  Period of sinusoidal motion in seconds.
 */
int sled_sinusoid_start(sled_t *handle, double amplitude, double period)
{
	assert(handle);
	syslog(LOG_DEBUG, "%s(%.3f, %.2f)", __FUNCTION__, amplitude, period);

	sled_profile_set_target(handle, handle->sinusoid_there, pos_absolute, handle->last_position + amplitude * 2.0, period / 2.0);
	sled_profile_set_target(handle, handle->sinusoid_back, pos_absolute, handle->last_position, period / 2.0);

	sled_profile_set_next(handle, handle->sinusoid_there, handle->sinusoid_back, 0.0, bln_after);
	sled_profile_set_next(handle, handle->sinusoid_back, handle->sinusoid_there, 0.0, bln_after);

	return sled_profile_execute(handle, handle->sinusoid_there);
}


/**
 * Stop sinusoidal motion.
 */
int sled_sinusoid_stop(sled_t *handle)
{
	assert(handle);
	syslog(LOG_DEBUG, "%s()", __FUNCTION__);

	sled_profile_set_next(handle, handle->sinusoid_there, -1, 0.0, bln_after);
	sled_profile_set_next(handle, handle->sinusoid_back, -1, 0.0, bln_after);

	return sled_profile_write_pending_changes(handle, handle->sinusoid_there);
}


/**
 * Switch (emergency) lights on or off.
 *
 * @param handle  libsled handle.
 * @param state  True for on, False for off.
 *
 * @return 0 on success, -1 on failure.
 */
int sled_light_set_state(const sled_t *handle, bool state)
{
	assert(handle);
	syslog(LOG_DEBUG, "%s(%s)", __FUNCTION__, state?"on":"off");

	// SDOs must be enabled for the light-switch to work.
	if(mch_sdo_active_state(handle->mch_sdo) == ST_SDO_DISABLED)
		return -1;

	mch_sdo_queue_write(handle->mch_sdo, OB_O_O1, 0x01, state?0x01:0x00, 0x04);

	return 0;
}

