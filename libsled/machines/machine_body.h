#include <syslog.h>
#include <assert.h>

// First generate structure type
#include "machine_undef.h"
#define FIELD(type, name) type name;
#define FIELD_DECL(type, name) type name;
#define FIELD_INIT(name, value)
#define CALLBACK(name) HANDLER_TYPE(name) CONCAT(name, _handler);

struct MACHINE_TYPE {
	STATE_TYPE state;
	void *payload;

#include MACHINE_FILE()
};

// Then generate simple functions (all except constructor)
#include "machine_undef.h"

#define BEGIN_CALLBACKS \
	void CONCAT(PREFIX, _set_callback_payload)(MACHINE_TYPE *machine, void *payload) \
	{ \
		machine->payload = payload; \
	}

#define CALLBACK(name, ...) \
	void HANDLER_SETTER_NAME(name)(MACHINE_TYPE *machine, HANDLER_TYPE(name) handler) \
	{ \
		machine->CONCAT(name, _handler) = handler; \
	}

#define BEGIN_STATES \
	const char *CONCAT(PREFIX, _statename)(STATE_TYPE state) { \
		switch(state) {
#define STATE(name) case name: return #name;
#define END_STATES } return "Invalid state"; }

#define BEGIN_EVENTS \
	const char *CONCAT(PREFIX, _eventname)(EVENT_TYPE event) { \
		switch(event) {
#define EVENT(name) case name: return #name;
#define END_EVENTS } return "Invalid event"; }

#define GENERATE_DEFAULT_FUNCTIONS \
	static STATE_TYPE CONCAT(PREFIX, _next_state_given_event)(const MACHINE_TYPE *machine, EVENT_TYPE event); \
	static void CONCAT(PREFIX, _on_enter)(MACHINE_TYPE *machine); \
	static void CONCAT(PREFIX, _on_exit)(MACHINE_TYPE *machine); \
	void CONCAT(PREFIX, _handle_event)(MACHINE_TYPE *machine, EVENT_TYPE event) \
	{ \
		assert(machine); \
		STATE_TYPE next_state = CONCAT(PREFIX, _next_state_given_event)(machine, event); \
		if(!(machine->state == next_state)) { \
		    CONCAT(PREFIX, _on_exit)(machine); \
			machine->state = next_state; \
			syslog(LOG_DEBUG, "%s() state changed to %s",  __FUNCTION__, CONCAT(PREFIX, _statename)(machine->state)); \
			CONCAT(PREFIX, _on_enter)(machine); \
		} \
	} \
	void CONCAT(PREFIX, _destroy)(MACHINE_TYPE **machine) \
	{ \
		free(*machine); \
		*machine = NULL; \
	} \
	STATE_TYPE CONCAT(PREFIX, _active_state)(MACHINE_TYPE *machine) \
	{ \
		assert(machine); \
		return machine->state; \
	}

#include MACHINE_FILE()

// Finally generate constructor
#include "machine_undef.h"

#define FIELD(type, name) type name,

MACHINE_TYPE *CONCAT(PREFIX, _create)(
#include MACHINE_FILE()
	...
)
{
	MACHINE_TYPE *machine = new MACHINE_TYPE();

	machine->state = INITIAL_STATE;
	machine->payload = NULL;

	#undef FIELD
	#undef FIELD_INIT
	#undef CALLBACK

	#define FIELD(type, name) machine->name = name;
	#define FIELD_INIT(name, value) machine->name = value;
	#define CALLBACK(name) machine->CONCAT(name, _handler) = NULL;
	#include MACHINE_FILE()

	assert(machine);
	return machine;
}

