#ifndef __SERVER_H__
#define __SERVER_H__

#include <map>
#include <list>
#include <libsled/sled.h>
#include <librtc3dserver/rtc3d.h>

enum command_type_t {
  cmd_setbyteorder,
  cmd_sendcurrentframe,
  cmd_sendstatus,
  cmd_streamframes,
  cmd_profile_execute,
  cmd_profile_set,
  cmd_sinusoid,
  cmd_lights,
  cmd_bye,
  cmd_home,
  cmd_clearfault,
  cmd_setinternalstatus,
  cmd_sendinternalstatus
};

struct sled_server_ctx_t {
	void *parser;
	sled_t *sled;
	rtc3d_server_t *server;

	std::list<rtc3d_connection_t *> stream_clients;

	// Maps protocol profile ids onto sled profile ids
	std::map<int, int> profile_tlate;
};

struct event_base;

sled_server_ctx_t *setup_sled_server_context(event_base *ev_base);
void teardown_sled_server_context(sled_server_ctx_t **ctx);

#endif

