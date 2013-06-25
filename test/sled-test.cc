
#include <event2/event.h>
#include <sled.h>
#include <interface.h>

int main(int argc, char *argv[])
{
  event_base *ev_base = event_base_new();    
  intf_t *intf = intf_create(ev_base);  
  intf_open(intf);
  
  event_base_loop(ev_base, 0);
  
  //sled_t *sled = sled_create();
  //sled_destroy(&sled);

  return 0;
}
