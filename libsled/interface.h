#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#include <stdint.h>

struct event_base;
struct intf_t;

/**
 * Network management commands.
 */
#define NMT_ENTERPREOPERATIONAL	0x80
#define NMT_STARTREMOTENODE		0x01


/**
 * Data dictionary objects.
 */
#define OB_O_P    0x35BE	// Position
#define OB_O_V    0x35BF	// Velocity
#define OB_O_C    0x35B9	// Settings
#define OB_O_ACC  0x35B7	// Acceleration
#define OB_O_DEC  0x35BA  // Deceleration
#define OB_O_TAB  0x35B8  // Profile table
#define OB_O_FN   0x35BC  // Next profile
#define OB_O_FT   0x35BD  // Delay before next
#define OB_O_MOVE 0x3642  // Execute motion task

#define OB_O_O1		0x35AE	// State of digital output 1
#define OB_O_O2		0x35B1	// State of digital output 2

#define OB_MOTION_TASK       0x2080  // Motion task to be executed
#define OB_ACTIVE_TASK       0x2081  // Currently active motion task
#define OB_COPY_MOTION_TASK	 0x2082	 // Copy motion task
#define OB_CONTROL_WORD 		 0x6040	 // Control word


// Callbacks
typedef void(*intf_nmt_state_handler_t)(intf_t *intf, void *payload, uint8_t state);
typedef void(*intf_tpdo_handler_t)(intf_t *intf, void *payload, int pdo, uint8_t *data);
typedef void(*intf_close_handler_t)(intf_t *intf, void *payload);

/**
 * Callbacks for send_write_req and send_read_req.
 */
typedef void(*intf_abort_callback_t)(void *data, uint16_t index, uint8_t subindex, uint32_t code);
typedef void(*intf_write_callback_t)(void *data, uint16_t index, uint8_t subindex);
typedef void(*intf_read_callback_t)(void *data, uint16_t index, uint8_t subindex, uint32_t value);

// Functions
intf_t *intf_create(event_base *ev_base);
void intf_destroy(intf_t **intf);

int intf_open(intf_t *intf);
int intf_close(intf_t *intf);

int intf_send_nmt_command(intf_t *intf, uint8_t command);
int intf_send_read_req(intf_t *intf, uint16_t index, uint8_t subindex, intf_read_callback_t read_callback, intf_abort_callback_t abort_callback, void *data);
int intf_send_write_req(intf_t *intf, uint16_t index, uint8_t subindex, uint32_t value, uint8_t size, intf_write_callback_t write_callback, intf_abort_callback_t abort_callback, void *data);

// Callback setters
void intf_set_callback_payload(intf_t *intf, void *payload);

void intf_set_nmt_state_handler(intf_t *intf, intf_nmt_state_handler_t handler);
void intf_set_tpdo_handler(intf_t *intf, intf_tpdo_handler_t handler);
void intf_set_close_handler(intf_t *intf, intf_close_handler_t handler);

#endif
