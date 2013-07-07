
#include <syslog.h>

#include <stdexcept>
#include <utility>

#include <sched.h>
#include <sys/mman.h>
#include <execinfo.h>
#include <signal.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <event2/event.h>
#include "server.h"

#define MAX_EVENTS 10
#define PRIORITY 49


/**
 * Prints a backtrace on segmentation faults.
 */
void signal_handler(int sig)
{
	if(SIGSEGV == sig) {
		void *array[10];
		size_t size;

		size = backtrace(array, 10);
		backtrace_symbols_fd(array, size, 2);
		exit(1);
	}
}


/**
 * Try to setup real-time environment.
 *
 * See https://rt.wiki.kernel.org/index.php/RT_PREEMPT_HOWTO
 */
int setup_realtime()
{
	/* Declare ourself as a realtime task */
	sched_param param;
	param.sched_priority = PRIORITY;
	if(sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
		perror("sched_setscheduler failed");
		return -1;
	}

	/* Lock memory */
	if(mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
		perror("mlockall failed");
		return -1;
	}

	return 0;
}


int main()
{
	/* Print stack-trace on segmentation fault. */
	signal(SIGSEGV, signal_handler);

	/* Open system log. */
	openlog("sled", LOG_NDELAY | LOG_NOWAIT, LOG_LOCAL3);	

	if(setup_realtime() == -1) {
		fprintf(stderr, "Could not setup realtime environment.\n");
		return 1;
	}

	/* Setup libevent */
	event_config *cfg = event_config_new();
	event_config_require_features(cfg, EV_FEATURE_FDS);
	event_base *ev_base = event_base_new_with_config(cfg);

	if(!ev_base) {
		fprintf(stderr, "Error initializing libevent.\n");
		return 1;
	}

	/* Setup context */
	sled_server_ctx_t *context = setup_sled_server_context(ev_base);

	if(context == NULL)
		return 1;

	printf("Starting event loop.\n");

	// Event loop
	event_base_loop(ev_base, 0);

	/* Shutdown server */	
	teardown_sled_server_context(&context);

	return 0;
}

