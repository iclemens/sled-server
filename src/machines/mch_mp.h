#ifndef __MCH_MP_H__
#define __MCH_MP_H__

enum mch_mp_event_t {
	EV_MP_DS_OPERATIONAL,		// From DS machine (mch_ds.cc)
	EV_MP_DS_INOPERATIONAL,		// From DS machine (mch_ds.cc)

	EV_MP_MODE_HOMING,
	EV_MP_MODE_PP,

	EV_MP_HOMED,
	EV_MP_NOTHOMED
};


enum mch_mp_state_t {
	ST_MP_DISABLED,

	ST_MP_SWITCH_MODE_HOMING,
	ST_MP_HH_UNKNOWN,
	ST_MP_HH_HOMING,

	ST_MP_SWITCH_MODE_PP,
	ST_MP_PP_IDLE,
	ST_MP_PP_MOVING
};

struct mch_mp_t;
struct intf_t;

mch_mp_t *mch_mp_create(intf_t *interface);
void mch_mp_destroy(mch_mp_t **machine);

mch_mp_state_t mch_mp_active_state(mch_mp_t *machine);
void mch_mp_handle_event(mch_mp_t *machine, mch_mp_event_t event);

#endif
