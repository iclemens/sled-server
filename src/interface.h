#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#include <stdint.h>

struct event_base;
struct intf_t;

// Callbacks
typedef void(*intf_nmt_state_handler_t)(intf_t *intf, uint8_t state);
typedef void(*intf_read_resp_handler_t)(intf_t *intf, uint16_t index, uint8_t subindex, uint32_t value);
typedef void(*intf_write_resp_handler_t)(intf_t *intf, uint16_t index, uint8_t subindex);
typedef void(*intf_abort_resp_handler_t)(intf_t *intf, uint16_t index, uint8_t subindex, uint32_t abort);

// Functions
intf_t *intf_create(event_base *ev_base);
void intf_destroy(intf_t **intf);

int intf_open(intf_t *intf);
int intf_close(intf_t *intf);

int intf_send_nmt_command(intf_t *intf, uint8_t command);
int intf_send_write_req(intf_t *intf, uint16_t index, uint8_t subindex, uint32_t value, uint8_t size);

// Callback setters
void intf_set_nmt_state_handler(intf_t *intf, intf_nmt_state_handler_t handler);
void intf_set_read_resp_handler(intf_t *intf, intf_read_resp_handler_t handler);
void intf_set_write_resp_handler(intf_t *intf, intf_write_resp_handler_t handler);
void intf_set_abort_resp_handler(intf_t *intf, intf_abort_resp_handler_t handler);


#endif