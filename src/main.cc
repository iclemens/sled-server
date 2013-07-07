
#include <syslog.h>

#include <getopt.h>
#include <stdexcept>
#include <utility>

#include <sys/types.h>
#include <sys/stat.h>

#include <sched.h>
#include <sys/mman.h>
#include <execinfo.h>
#include <signal.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
		backtrace_symbols_fd(array, size, fileno(stdout));
		exit(EXIT_FAILURE);
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


static long int writer(void *cookie, const char *data, size_t len)
{
	syslog(LOG_DEBUG, "%.*s", (int) len, data);
}


static cookie_io_functions_t log_functions = {
	(cookie_read_function_t *) NULL,
	writer,
	(cookie_seek_function_t *) NULL,
	(cookie_close_function_t *) NULL
};


void to_syslog(FILE **file)
{
	*file = fopencookie(NULL, "w", log_functions);
	setvbuf(*file, NULL, _IOLBF, 0);
}


/**
 * Fork-off current process and initialize the new fork.
 */
void daemonize()
{
	/* Create fork */
	pid_t pid = fork();
	if(pid < 0)
		exit(EXIT_FAILURE);
	if(pid > 0)
		exit(EXIT_SUCCESS);

	/* Change file-mode mask */
	umask(0);

	/* Create new session */
	pid_t sid = setsid();
	if(sid < 0) {
		syslog(LOG_ALERT, "%s() failed to create session", __FUNCTION__);
		exit(EXIT_FAILURE);
	}

	/* Change working directory */
	if((chdir("/")) < 0) {
		syslog(LOG_ALERT, "%s() could not change working directory", __FUNCTION__);
		exit(EXIT_FAILURE);
	}

	/* Close file handles */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	/* Redirect stderr and stdout to syslog */
	to_syslog(&stdout);
	to_syslog(&stderr);
}


void print_help()
{
	printf("Sled control server\n");
	printf("\n");
	printf("Compiled on %s %s\n", __DATE__, __TIME__);
	printf("\n");
	printf("  --no-daemon   Do not daemonize.\n");
	printf("  --help        Print help text.\n");
	printf("\n");
}


int main(int argc, char **argv)
{
	int daemonize_flag = 1;	

	/* Parse command line arguments */
	static struct option long_options[] =
		{
			{"no-daemon",	no_argument, &daemonize_flag, 0},
			{"help",		no_argument, 0, 'h'},
			{"\0", 0, 0, 0}
		};

	int option_index = 0;
	int c = 0;

	while((c = getopt_long(argc, argv, "h", long_options, &option_index)) != -1) {
		switch(c) {
			case 0:
				if(option_index == 1) {
					print_help();
					exit(EXIT_SUCCESS);
				}
				break;

			case 'h':
				print_help();
				exit(EXIT_SUCCESS);
				break;
		}
	}

	/* Open system log. */
	openlog("sled", LOG_NDELAY | LOG_NOWAIT, LOG_LOCAL3);
	syslog(LOG_DEBUG, "%s() compiled %s %s", __FUNCTION__, __DATE__, __TIME__);

	/* Daemonize process. */
	if(daemonize_flag)
		daemonize();

	/* Print stack-trace on segmentation fault. */
	signal(SIGSEGV, signal_handler);

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

