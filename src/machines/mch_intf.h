#ifndef __MCH_INTF_H__
#define __MCH_INTF_H__

enum mch_intf_event_t {
  EV_INTF_OPEN,	// Open command
  EV_INTF_CLOSE,	// Close command
  
  EV_INTF_OPENED,	// Opened event
  EV_INTF_CLOSED  	// Closed event
};


enum mch_intf_state_t {
  ST_INTF_CLOSED,
  ST_INTF_OPENED,
  ST_INTF_OPENING,
  ST_INTF_CLOSING
};

struct mch_intf_t;
struct intf_t;

mch_intf_t *mch_intf_create(intf_t *interface);
void mch_intf_destroy(mch_intf_t **machine);

mch_intf_state_t mch_intf_active_state(mch_intf_t *machine);
void mch_intf_handle_event(mch_intf_t *machine, mch_intf_event_t event);

#endif
