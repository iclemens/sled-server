
#include <event2/event.h>
#include <sled.h>
#include <interface.h>

#include <machines/mch_intf.h>

void intf_on_close(intf_t *intf, void *payload)
{
  mch_intf_t *mch_intf = (mch_intf_t *) payload;  
  mch_intf_handle_event(mch_intf, EV_INTF_CLOSE);
}

int main(int argc, char *argv[])
{
  event_base *ev_base = event_base_new();
  
  intf_t *intf = intf_create(ev_base);
  intf_set_close_handler(intf, intf_on_close);
  
  mch_intf_t *mch_intf = mch_intf_create(intf);
  
  intf_set_callback_payload(intf, (void *) mch_intf);

  mch_intf_handle_event(mch_intf, EV_INTF_OPEN);

  event_base_loop(ev_base, 0);
  
  //sled_t *sled = sled_create();
  //sled_destroy(&sled);

  return 0;
}
