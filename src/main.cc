#include <syslog.h>

#include <getopt.h>
#include <stdexcept>
#include <utility>

#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

#include <sched.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>
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
#define MIN_MEMLOCK 104857600

/**
 * Prints a backtrace on segmentation faults.
 */
static void signal_handler(int sig)
{
  syslog(LOG_NOTICE, "%s() received signal %d (%s).",
    __FUNCTION__, sig, strsignal(sig));

	if(SIGSEGV == sig) {
		void *array[10];
		size_t size;

		size = backtrace(array, 10);
		backtrace_symbols_fd(array, size, fileno(stdout));
		exit(EXIT_FAILURE);
	}

	if(SIGTERM == sig) {
		syslog(LOG_WARNING, "%s() warning, proper shutdown "
			"procedure has not been implemented", __FUNCTION__);
		exit(EXIT_SUCCESS);
	}
}


/**
 * Try to setup real-time environment.
 *
 * See https://rt.wiki.kernel.org/index.php/RT_PREEMPT_HOWTO
 */
static int setup_realtime()
{
	/* Declare ourself as a realtime task */
	sched_param param;
	param.sched_priority = PRIORITY;
	if(sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
		perror("setup_realtime():sched_setscheduler()");
		return -1;
	}

	/* Check whether we can lock enough memory in RAM */
	struct rlimit limit;
	if(getrlimit(RLIMIT_MEMLOCK, &limit) == -1) {
		perror("setup_realtime():getrlimit()");
		return -1;
	}

	if(limit.rlim_cur < MIN_MEMLOCK) {
		fprintf(stderr, "Memlock limit of %lu MB is not sufficient. "
						"Increase to at least %lu MB by editing "
						"/etc/security/limits.conf, logout and "
						"try again.\n",
						limit.rlim_cur / 1048576,
						(long unsigned) MIN_MEMLOCK / 1048576);
		return -1;
	}

	/* Lock memory */
	if(mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
		perror("setup_realtime():mlockall()");
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


static void to_syslog(FILE **file)
{
	*file = fopencookie(NULL, "w", log_functions);
	setvbuf(*file, NULL, _IOLBF, 0);
}


/**
 * Fork-off current process and initialize the new fork.
 */
static void daemonize()
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
		syslog(LOG_ALERT, "%s() could not change "
			"working directory", __FUNCTION__);
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


/**
 * Returns user-id given a name.
 */
static uid_t get_uid_by_name(const char *name)
{
	if(name == NULL)
		return -1;

	passwd *pwent = getpwnam(name);

	if(pwent == NULL)
		return -1;

	uid_t uid = pwent->pw_uid;

	return uid;
}

// These make sure macros are expanded before concatenation
#define STRR(x) #x
#define STR(x) STRR(x)

static void print_help()
{
  printf("Sled control server " 
	STR(VERSION_MAJOR) "." STR(VERSION_MINOR) " \n\n");
  printf("Compiled from " STR(VERSION_BRANCH) "/" STR(VERSION_HASH) 
	" on " __DATE__ " " __TIME__ "\n");

	printf("\n");
	printf("  --no-daemon   Do not daemonize.\n");
	printf("  --help        Print help text.\n");
	printf("\n");
}


int main(int argc, char **argv)
{
	int daemonize_flag = 1;
	uid_t uid = get_uid_by_name("sled");

	/* Parse command line arguments */
	static struct option long_options[] =
		{
			{"no-daemon",	no_argument, &daemonize_flag, 0},
			{"help",		no_argument, 0, 'h'},
			{"user",		required_argument, 0, 'u'},
			{"\0", 0, 0, 0}
		};

	int option_index = 0;
	int c = 0;

	while((c = getopt_long(argc, argv, "hu:", long_options, &option_index)) != -1) {
		switch(c) {
			case 'u':
				uid = get_uid_by_name(optarg);
				if(uid == -1) {
					fprintf(stderr, "Invalid user specified (%s).\n", optarg);
					exit(EXIT_FAILURE);
				}
				break;

			case 'h':
				print_help();
				exit(EXIT_SUCCESS);
				break;
		}
	}

	if(daemonize_flag && (uid == -1)) {
		fprintf(stderr, "Default user 'sled' does not exist, "
			"and no user was specified.\n");
		exit(EXIT_FAILURE);
	}

	/* Open system log. */
	openlog("sled", LOG_NDELAY | LOG_NOWAIT, LOG_LOCAL3);

	/* Write version information to log */
	syslog(LOG_DEBUG, "%s() " STR(VERSION_BRANCH) 
    	" version " STR(VERSION_MAJOR) "." 
					STR(VERSION_MINOR) " " 
					STR(VERSION_HASH) 
    	" compiled " __DATE__ " " __TIME__, __FUNCTION__);

	/* Daemonize process. */
	if(daemonize_flag)
		daemonize();

	/* Print stack-trace on segmentation fault. */
	signal(SIGSEGV, signal_handler);
	signal(SIGTERM, signal_handler);

	if(setup_realtime() == -1) {
		fprintf(stderr, "Could not setup realtime environment.\n");
		return 1;
	}

	/* Drop privileges */
	if(uid != -1) {
		if(setuid(uid) == -1) {
			perror("setuid failed");
			exit(EXIT_FAILURE);
		}
	}

	/* Setup libevent */
	event_config *cfg = event_config_new();
	event_config_require_features(cfg, EV_FEATURE_FDS);
	#ifdef EVENT_BASE_FLAG_PRECISE_TIMER
	event_config_set_flag(cfg, EVENT_BASE_FLAG_PRECISE_TIMER);
	#endif
	event_config_set_flag(cfg, EVENT_BASE_FLAG_NOLOCK);
	event_config_set_flag(cfg, EVENT_BASE_FLAG_NO_CACHE_TIME);

	event_base *ev_base = event_base_new_with_config(cfg);
	event_base_priority_init(ev_base, 2);

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

