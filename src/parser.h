#ifndef __PARSER_H__
#define __PARSER_H__

#include <libsled/sled.h>
#include <libsled/sled_profile.h>
#include "main.h"


enum status_t {
	sta_off, sta_preoperational, sta_operational, sta_outputenabled
};


/**
 * Structure used to return parsed command.
 */
struct command_t {
  command_type_t type;

  // boolean for start/stop on/off
  bool boolean;

  // set byte order
  byte_order_t byte_order;

  // setinternalstatus
  status_t status;

  // sinusoid
  double amplitude, period;

  // profile
  int profile, table;
  position_type_t position_type;
  double position, time;
  int next_profile;
  double next_delay;
  blend_type_t blend_type;
};


/**
 * Local parser variables.
 */
struct parser_t {
  void *scanner;
  command_t *command;
};


void *parser_create();
void parser_destroy(void **parser);
int parser_parse_string(void *p, const char *s, command_t *c);

#endif

