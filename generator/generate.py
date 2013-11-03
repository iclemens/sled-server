import sys
import pprint
import string

from icsxml_parse import parse_icsxml


###################
# Generate header #
###################


def generate_header(names, machine):
  fields = list()
  for field in machine['fields']:
    fields.append(field[0] + ' ' + field[1])
  argument_list = string.join(fields, ', ')

  header = \
"""#ifndef __MACHINE_{1}_H__

/* This file was automatically generated. Do not modify. */

{5}

struct {0[machine_type]};

enum {0[state_type]} {{
  {3}
}};

enum {0[event_type]} {{
  {4}
}};

{0[machine_type]} *{0[prefix]}_create({2});
void {0[prefix]}_destroy({0[machine_type]} **machine);

void {0[prefix]}_on_entry({0[machine_type]} *machine);
void {0[prefix]}_on_exit({0[machine_type]} *machine);

void {0[prefix]}_handle_event({0[machine_type]} *machine, {0[event_type]} event);
{0[state_type]} {0[prefix]}_next_state_given_event({0[machine_type]} *machine, {0[event_type]} event);
{0[state_type]} {0[prefix]}_active_state({0[machine_type]} *machine);

void {0[prefix]}_set_callback_payload({0[machine_type]} *machine, {0[event_type]} event);""" \
    .format(
      names, 
      machine['name'].upper(),  #1
      argument_list,            #2
      string.join(names['states'], ',\n\t'), #3
      string.join(names['events'], ',\n\t'), #4
      machine['header']
      )

  for callback in machine['callbacks']:
    header += "typedef void(*{0[prefix]}_{1}_t)({0[machine_type]} *machine, void *payload);\n".format(
      names, callback)
  header += "\n"

  for callback in machine['callbacks']:
    header += "void {0[prefix]}_set_{1}({0[machine_type]} *machine, {0[prefix]}_{1}_t handler);\n".format(
      names, callback)

  header += "\n#endif\n"
  
  return header


###################
# Generate source #
###################


def machine_structure(names, machine):
  struct = \
"""struct {0[machine_type]} {{
  {0[state_type]} state;
  void *payload;

""".format(names)

  for field in machine['fields']:
    struct += "\t{0[0]} {0[1]};\n".format(field)
  struct += "\n"

  for callback in machine['callbacks']:
    struct += "\t{0[prefix]}_{1}_t {1}_handler;\n".format(names, callback)

  struct += "};\n"
  
  return struct


#
# Function used to setup state machine structure.
#
def function_create_machine(names, machine):
  fields = list()
  for field in machine['fields']:
    fields.append(field[0] + ' ' + field[1])
  argument_list = string.join(fields, ', ')

  func = \
"""/**
 * Setup state machine structure.
 */
{0[machine_type]} *{0[prefix]}_create({1})
{{
  {0[machine_type]} *machine = new {0[machine_type]};
  
  machine->state = {2};
  machine->payload = NULL;

""".format(names, argument_list, machine['initial'])

  for field in machine['fields']:
    func += "\tmachine->{0} = {0};\n".format(field[1])
  func += "\n"
  
  for callback in machine['callbacks']:
    func += "\tmachine->{0}_handler = NULL;\n".format(callback)
  func += "\n"

  func += """
  assert(machine);
  return machine;
}
"""

  return func
  

#
# Function used to free state machine structure.
#
def function_destroy_machine(names, machine):
  return \
"""/**
 * Free state machine structure.
 */
void {0[prefix]}_destroy({0[machine_type]} **machine)
{{
  free(*machine);
  *machine = NULL;
}}
""".format(names)


def generate_func_set_callback_payload(names, machine):
  return \
"""/**
 * Sets a pointer that will be passed as an argument to all callback functions.
 */
void {0[prefix]}_set_callback_payload({0[machine_type]} *machine, void *payload)
{{
  machine->payload = payload;
}}
""".format(names)


def generate_func_callback_setter(names, machine, callback):
  return "void {0[prefix]}_set_{1}({0[machine_type]} *machine, {0[prefix]}_{1}_t handler)\n" \
         "{{\n" \
         "\tmachine->{1}_handler = handler;\n" \
         "}}\n".format(names, callback)


def generate_func_active_state(names, machines):
  return \
"""{0[state_type]} {0[prefix]}_active_state({0[machine_type]} *machine)
{{
  assert(machine);
  return machine->state;
}}
""".format(names)  


def generate_func_statename(names, machine):
  func = "const char *{0[prefix]}_statename({0[state_type]} state)\n" \
         "{{\n" \
         "\tswitch(state) {{\n".format(names)
  
  for state in names['states']:
    func += "\t\tcase {0}: return \"{0}\";\n".format(state)
  func += "\t}\n\n"
  
  func += "\treturn \"Invalid state\";\n"
  func += "}\n"
  
  return func


def generate_func_eventname(names, machine):
  func = "const char *{0[prefix]}_eventname({0[event_type]} event)\n" \
         "{{\n" \
         "\tswitch(event) {{\n".format(names)
  
  for event in names['events']:
    func += "\t\tcase {0}: return \"{0}\";\n".format(event)
  func += "\t}\n\n"
  
  func += "\treturn \"Invalid event\";\n"
  func += "}\n"
  
  return func


#
# Function that needs to be called when an event occurs.
# It checks whether a transition is required and executes it.
#
def function_handle_event(names, machine):
  return \
"""/**
 * Receives events and performs state transition if required.
 */
void {0[prefix]}_handle_event({0[machine_type]} *machine, {0[event_type]} event)
{{
  assert(machine);
  {0[state_type]} next_state = {0[prefix]}_next_state_given_event(machine, event);

  if(!machine->state == next_state) {{
    {0[prefix]}_on_exit(machine);
    machine->state = next_state;
    syslog(LOG_DEBUG, \"%s() state changed to %s\", 
      __FUNCTION__, {0[prefix]}_statename(machine->state));
    {0[prefix]}_on_entry(machine);
  }}
}}
""".format(names)


def generate_func_on_entry(names, machine):
  func = \
"""void {0[prefix]}_on_entry({0[machine_type]} *machine)
{{
  switch(machine->state)
  {{
""".format(names)
  
  for state in machine['states']:
    if 'onentry' in state:
      func += "    {0}:\n      {1}\n      break;\n".format(state['name'], state['onentry'])

  func += \
"""    default:
      return;
  }
}
"""
  return func


def generate_func_on_exit(names, machine):
  func = \
"""void {0[prefix]}_on_exit({0[machine_type]} *machine)
{{
  switch(machine->state)
  {{
""".format(names)
  
  for state in machine['states']:
    if 'onexit' in state:
      func += "\t\t{0}:\n\t\t\t{1}\n\t\t\tbreak;\n".format(state['name'], state['onexit'])

  func += \
"""    default:
      return;
  }
}
"""  
  return func


def function_next_state_given_event(names, machine):
  func = """/*
 * Returns whether an event should cause a state transition.
 */
{0[state_type]} {0[prefix]}_next_state_given_event({0[machine_type]} *machine, {0[event_type]} event)
{{
  {0[state_type]} current_state = {0[prefix]}_active_state(machine);

  switch(current_state)
  {{
""".format(names)


  for state in machine['states']:
    func += '    case {0}:\n'.format(state['name'])
    for transition in state['transitions']:
      func += '      if(event == {0})\n        return {1};\n'.format(transition[0], transition[1])
    func += '      return current_state;\n\n'

  func += \
"""    default:
      return current_state;
  }}
}}
""".format(names)
  return func


def generate_body(names, machine):
  body = \
    "#include <assert.h>\n" \
    "#include <syslog.h>\n" \
    "#include \"{0[prefix]}_test.h\"\n\n" \
    .format(names)

  body += machine_structure(names, machine) + "\n"
  body += machine['source'] + "\n\n"
  
  body += function_create_machine(names, machine) + "\n"  
  body += function_destroy_machine(names, machine) + "\n"  
  
  body += generate_func_set_callback_payload(names, machine) + "\n"
  
  for callback in machine['callbacks']:
    body += generate_func_callback_setter(names, machine, callback) + "\n"

  body += generate_func_active_state(names, machine) + "\n"
  body += generate_func_statename(names, machine) + "\n"
  body += generate_func_eventname(names, machine) + "\n"  

  body += function_handle_event(names, machine) + "\n"

  body += generate_func_on_entry(names, machine) + "\n"
  body += generate_func_on_exit(names, machine) + "\n"
  body += function_next_state_given_event(names, machine) + "\n"
  

  
  return body


def generate(input_filename):
  machine = parse_icsxml(input_filename)

  names = dict()
  names['prefix'] = prefix = 'mch_{}'.format(machine['name'])
  names['machine_type'] = '{}_t'.format(names['prefix'])
  names['state_type'] = '{}_state_t'.format(prefix)
  names['event_type'] = '{}_event_t'.format(prefix)

  # Prepare list of state and event names
  events = list()
  states = list()
  for state in machine['states']:
    states.append(state['name'])
    for transition in state['transitions']:
      if not transition[0] in events:
        events.append(transition[0])
  names['states'] = states
  names['events'] = events

  f = open(names['prefix'] + '_test.h', 'w')
  f.write(generate_header(names, machine))
  f.close()
  
  f = open(names['prefix'] + '_test.cc', 'w')
  f.write(generate_body(names, machine))
  f.close()


if __name__ == "__main__":
  if len(sys.argv) != 2:
    raise "Invalid number of arguments."

  generate(sys.argv[1])
