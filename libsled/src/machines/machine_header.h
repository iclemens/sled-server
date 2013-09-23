#include "machine_undef.h"

#ifndef PREFIX
#define PREFIX machine
#endif

// Concatenation trick to allow #defines to be
//  concatenated with normal tokens.
#define CONCAT_(a, b) a ## b
#define CONCAT(a, b) CONCAT_(a, b)

// Define special types
#define MACHINE_TYPE CONCAT(PREFIX, _t)
#define STATE_TYPE CONCAT(PREFIX, _state_t)
#define EVENT_TYPE CONCAT(PREFIX, _event_t)

#define HANDLER_SETTER_NAME(name) CONCAT(CONCAT(PREFIX, _set_ ## name), _handler)
#define HANDLER_TYPE(name) CONCAT(CONCAT(PREFIX, _ ## name), _handler_t)

// States, events, callbacks and fields
#define BEGIN_STATES enum STATE_TYPE {
#define BEGIN_EVENTS enum EVENT_TYPE {
#define BEGIN_CALLBACKS

#define END_STATES };
#define END_EVENTS };
#define END_CALLBACKS

#define STATE(name) name,
#define EVENT(name) name,
#define CALLBACK(name, ...) \
	typedef void(*HANDLER_TYPE(name))(MACHINE_TYPE *machine, void *payload, ## __VA_ARGS__); \
	void HANDLER_SETTER_NAME(name)(MACHINE_TYPE *machine, HANDLER_TYPE(name) handler);

// Forward declaration
struct MACHINE_TYPE;

#define GENERATE_DEFAULT_FUNCTIONS \
	void CONCAT(PREFIX, _destroy)(MACHINE_TYPE **machine); \
	STATE_TYPE CONCAT(PREFIX, _active_state)(MACHINE_TYPE *machine); \
	void CONCAT(PREFIX, _handle_event)(MACHINE_TYPE *machine, EVENT_TYPE event); \
	void CONCAT(PREFIX, _set_callback_payload)(MACHINE_TYPE *machine, void *payload); \
	STATE_TYPE CONCAT(PREFIX, _next_state_given_event)(MACHINE_TYPE *machine, EVENT_TYPE event); \
	void CONCAT(PREFIX, _on_enter)(MACHINE_TYPE *machine); \
	void CONCAT(PREFIX, _on_exit)(MACHINE_TYPE *machine);

#define BEGIN_FIELDS \
	MACHINE_TYPE *CONCAT(PREFIX, _create)(
#define FIELD(type, name) type name,
#define END_FIELDS ...);
