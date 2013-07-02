
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <event2/event.h>
#include <sled.h>
#include <interface.h>


void signal_handler(int signal)
{
	void *array[10];
	size_t size;

	size = backtrace(array, 10);
	backtrace_symbols_fd(array, size, STDERR_FILENO);
	exit(1);
}


int main(int argc, char *argv[])
{
	signal(SIGSEGV, signal_handler);

	event_config *cfg = event_config_new();
	event_config_require_features(cfg, EV_FEATURE_FDS);
	event_base *ev_base = event_base_new_with_config(cfg);

	if(!ev_base) {
		fprintf(stderr, "Unable to initialize event base\n");
		exit(1);
	}

	sled_t *sled = sled_create(ev_base);

	//mch_intf_handle_event(machines.mch_intf, EV_INTF_OPEN);
	event_base_loop(ev_base, 0);

	sled_destroy(&sled);

	return 0;
}

