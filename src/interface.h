#ifndef __CAN_H__
#define __CAN_H__

struct event_base;
struct intf_t;

intf_t *intf_create(event_base *ev_base);
void intf_destroy(intf_t **intf);

int intf_open(intf_t *intf);
int intf_close(intf_t *intf);

//int intf_send_nmt(intf_t *intf, 



#endif