#include "server.h"
#include "parser.h"

#include <math.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <syslog.h>
#include <event2/event.h>


// Interval at which new samples are sent in us
#define SAMPLE_INTERVAL 1000

// Interval after which samples are counted as invalid
#define MAX_SAMPLE_INTERVAL 2000

// Output summary statistics every 5 minutes
#define REPORT_EVERY_X_SAMPLES int(300 * (1e6/SAMPLE_INTERVAL))


/**
 * Returns current time in seconds.
 *
 * @return Current time in seconds.
 */
static double get_time()
{
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return double(ts.tv_sec) + double(ts.tv_nsec) / 1000.0 / 1000.0 / 1000.0;
}


/**
 * Translate protocol profile IDs into sled profile IDs.
 *
 * @param ctx  Sled context
 * @param profile  Profile ID received from network.
 * @return Internal ID
 */
static int tlate_profile_id(sled_server_ctx_t* ctx, int profile)
{
	if(ctx->profile_tlate.count(profile) == 1) {
		return ctx->profile_tlate[profile];
	}

	int profile_id = sled_profile_create(ctx->sled);
	ctx->profile_tlate.insert( std::pair<int, int>(profile, profile_id) );

	return profile_id;
}


/**
 * Called on client disconnect, makes sure that the client
 * is no longer on the stream-frames-list.
 */
static void rtc3d_disconnect_handler(rtc3d_connection_t *rtc3d_conn, void **ptr)
{
	sled_server_ctx_t *ctx = (sled_server_ctx_t *) rtc3d_get_global_data(rtc3d_conn);
	ctx->stream_clients.remove(rtc3d_conn);
	syslog(LOG_NOTICE, "%s() removing client", __FUNCTION__);
}


/**
 * Executes a (string) command by invoking either
 * the network or sled subsystem.
 */
static void rtc3d_command_handler(rtc3d_connection_t *rtc3d_conn, char *cmd)
{
	sled_server_ctx_t *ctx = (sled_server_ctx_t *) rtc3d_get_global_data(rtc3d_conn);
	command_t command;

	int retval = parser_parse_string(ctx->parser, cmd, &command);

	if(retval == -1) {
		rtc3d_send_error(rtc3d_conn, (char *) "err-syntaxerror");
		return;
	}

	switch(command.type) {
		case cmd_setbyteorder: {
			rtc3d_set_byte_order(rtc3d_conn, command.byte_order);
			rtc3d_send_command(rtc3d_conn, (char *) "ok-setbyteorder");
			break;
		}


		case cmd_streamframes: {
			if(command.boolean) {
				ctx->stream_clients.remove(rtc3d_conn);
				ctx->stream_clients.push_back(rtc3d_conn);
				syslog(LOG_NOTICE, "%s() adding client", __FUNCTION__);
			} else {
				ctx->stream_clients.remove(rtc3d_conn);
			}
			rtc3d_send_command(rtc3d_conn, (char *) "ok-streamframes");
			break;
		}


		case cmd_sendcurrentframe: {
			double position;
			if(sled_rt_get_position(ctx->sled, position) == -1)
				rtc3d_send_error(rtc3d_conn, (char *) "err-sendcurrentframe");
			else
				rtc3d_send_data(rtc3d_conn, -1, -1, position * 1000.0);
			break;
		}


		case cmd_profile_execute: {
			int profile_id = tlate_profile_id(ctx, command.profile);
			int retval = sled_profile_execute(ctx->sled, profile_id);

			if(retval == -1)
				rtc3d_send_error(rtc3d_conn, (char *) "err-profile-execute");
			else
				rtc3d_send_command(rtc3d_conn, (char *) "ok-profile-execute");
			break;
		}


		case cmd_profile_set: {
			int profile_id = tlate_profile_id(ctx, command.profile);

			if(profile_id < 0) {
				syslog(LOG_ERR, "%s() invalid profile %d", __FUNCTION__, profile_id);
				rtc3d_send_error(rtc3d_conn, (char *) "err-profile-set");
				break;
			}

			// Check position...
			if(abs(command.position) > 0.5) {
				syslog(LOG_ERR, "%s() invalid profile, position out of bounds", __FUNCTION__);
				rtc3d_send_error(rtc3d_conn, (char *) "err-profile-set");
			}

			if(sled_profile_set_target(ctx->sled, profile_id, 
					command.position_type, 
					command.position, 
					command.time) == -1) {
				syslog(LOG_ERR, "setting of position failed (%.3fm in %.2fs)", command.position, command.time);
				rtc3d_send_error(rtc3d_conn, (char *) "err-profile-set");
				break;
			}

			if(command.next_profile >= 0) {
				int next_profile_id = tlate_profile_id(ctx, command.next_profile);

				if(next_profile_id < 0) {
					rtc3d_send_error(rtc3d_conn, (char *) "err-profile-set");
					break;
				}

				sled_profile_set_next(ctx->sled, profile_id, next_profile_id, command.next_delay, command.blend_type);
			} else {
				sled_profile_set_next(ctx->sled, profile_id, -1, 0, bln_none);
			}

			rtc3d_send_command(rtc3d_conn, (char *) "ok-profile-set");

			break;
		}


		case cmd_sinusoid: {
			if(command.boolean) {
				if(sled_sinusoid_start(ctx->sled, command.amplitude, command.period) == -1) {
					syslog(LOG_ERR, "%s() could not start sinusoid", __FUNCTION__);
					rtc3d_send_error(rtc3d_conn, (char *) "err-sinusoid-start");
				} else {
					rtc3d_send_command(rtc3d_conn, (char *) "ok-sinusoid-start");
				}
			} else {
				if(sled_sinusoid_stop(ctx->sled) == -1) {
					syslog(LOG_ERR, "%s() could not stop sinusoid", __FUNCTION__);
					rtc3d_send_error(rtc3d_conn, (char *) "err-sinusoid-stop");
				} else {
					rtc3d_send_command(rtc3d_conn, (char *) "ok-sinusoid-stop");
				}
			}

			break;
		}


		case cmd_lights: {
			if(sled_light_set_state(ctx->sled, command.boolean) == -1)
				rtc3d_send_error(rtc3d_conn, (char *) "err-light");
			else
				rtc3d_send_command(rtc3d_conn, (char *) "ok-light");
			break;
		}


		case cmd_sendstatus: {
			break;
		}

		case cmd_home: {
			break;
		}

		case cmd_clearfault: {
			break;
		}


		case cmd_bye: {
			rtc3d_send_command(rtc3d_conn, (char *) "bye");
			rtc3d_disconnect(rtc3d_conn);
			break;
		}

		default: {
			rtc3d_send_error(rtc3d_conn, (char *) "err-notsupported");
		}
	}

}


static void update_timeout_stats(double time_actual)
{
	static double time_previous = time_actual;

	static double sum_delay = 0;
	static double max_delay = 0;

	static int num_samples = 0;
	static int num_unacceptable = 0;

	// Time since previous invocation
	double delay = time_actual - time_previous;
	time_previous = time_actual;

	// Update statistics
	if(delay > max_delay) max_delay = delay;
	if(delay > MAX_SAMPLE_INTERVAL/1e6) num_unacceptable++;

	sum_delay += delay;
	num_samples++;

	// Output summary statistics every X samples
	if(num_samples >= REPORT_EVERY_X_SAMPLES) {
		syslog(LOG_DEBUG, "%s() %d of %d timeouts missed; mean %.0f us; max %.0f us\n",
			__FUNCTION__, num_unacceptable, num_samples,
			(sum_delay/num_samples)*1e6,
			(max_delay * 1e6));

		sum_delay = max_delay = 0;
		num_samples = num_unacceptable = 0;
	}
}


static void on_timeout(evutil_socket_t sock, short events, void *arg)
{
	sled_server_ctx_t *ctx = (sled_server_ctx_t *) arg;

	if(events & EV_TIMEOUT == 0)
		return;

	double tcurrent = get_time();
	update_timeout_stats(tcurrent);

	// Get position
	double position, time;
	if(sled_rt_get_position_and_time(ctx->sled, position, time) == -1) {
		position = NAN;
	}

	// Send position to all clients
	static int frame = 0;

	for(std::list<rtc3d_connection_t *>::iterator it = (ctx->stream_clients).begin();
		it != (ctx->stream_clients).end(); it++) {
		rtc3d_send_data(*it, frame, (uint64_t) (time * 1e6), position * 1000.0);
	}

	frame++;
}


/**
 * Create server context (to be passed to RTC3D server).
 */
sled_server_ctx_t *setup_sled_server_context(event_base *ev_base)
{
	sled_server_ctx_t *ctx;

	try {
		ctx = new sled_server_ctx_t();
	} catch(std::bad_alloc e) {
		return NULL;
	}

	ctx->parser = parser_create();
	if(ctx->parser == NULL) {
		delete ctx;
		return NULL;
	}

	ctx->sled = sled_create(ev_base);

	/* Setup server */
	ctx->server = rtc3d_setup_server(ev_base, (void *) ctx, 3375);

	if(ctx->server == NULL) {
		sled_destroy(&(ctx->sled));
		parser_destroy(&(ctx->parser));
		delete ctx;
	}

	/* Install handlers */
	rtc3d_set_disconnect_handler(ctx->server, rtc3d_disconnect_handler);
	rtc3d_set_command_handler(ctx->server, rtc3d_command_handler);

	// Start periodic event
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = SAMPLE_INTERVAL;

	event *periodic = event_new(ev_base, fileno(stdout), EV_READ | EV_PERSIST, on_timeout, (void *) ctx);
	event_add(periodic, &timeout);

	return ctx;
}


/**
 * Destroy context passed to RTC3D server.
 */
void teardown_sled_server_context(sled_server_ctx_t **ctx)
{
	sled_destroy(&(*ctx)->sled);
	parser_destroy(&(*ctx)->parser);

	rtc3d_teardown_server(&(*ctx)->server);

	delete *ctx;
	*ctx = NULL;
}


