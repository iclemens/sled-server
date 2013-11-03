import sys
import pprint
import string

from icsxml_parse import parse_icsxml

def generate_header(names, machine):
  prefix = 'mch_{}'.format(machine['name'])

  machine_type = '{}_t'.format(prefix)
  state_type = '{}_state_t'.format(prefix)
  event_type = '{}_event_t'.format(prefix)
  
  fields = list()
  for field in machine['fields']:
    fields.append(field[0] + ' ' + field[1])
  argument_list = string.join(fields, ', ')

  header = \
"""#ifndef __MACHINE_{0}_H__

/* This file was automatically generated. Do not modify. */

{8}

struct {2};

enum {3} {{
  {6}
}};

enum {4} {{
  {7}
}};

{2} *{1}_create({5});
void {1}_destroy({2} **machine);

void {1}_on_entry({2} *machine);
void {1}_on_exit({2} *machine);

void {1}_handle_event({2} *machine, {4} event);
{3} {1}_next_state_given_event({2} *machine, {4} event);
{3} {1}_active_state({2} *machine);

void {1}_set_callback_payload({2} *machine, {4} event);""" \
    .format(
      machine['name'].upper(),  #0
      prefix,                   #1
      machine_type,             #2
      state_type,               #3
      event_type,               #4
      argument_list,            #5
      string.join(names['states'], ',\n\t'),#6
      string.join(names['events'], ',\n\t'),#7
      machine['header']
      )

  for callback in machine['callbacks']:
    header += "typedef void(*{}_{}_t)({} *machine, void *payload);\n".format(
      prefix, callback, machine_type)
  header += "\n"

  for callback in machine['callbacks']:
    header += "void {0}_set_{1}({2} *machine, {0}_{1}_t handler);\n".format(
      prefix, callback, machine_type)

  header += "\n#endif\n"
  
  return header


def generate_struct_def(names, machine):
  struct = "struct {0[machine_type]} {{\n" \
    "\t{0[state_type]} state;\n" \
    "\tvoid *payload;\n\n".format(names)

  for field in machine['fields']:
    struct += "\t{0[0]} {0[1]};\n".format(field)
  struct += "\n"

  for callback in machine['callbacks']:
    struct += "\t{0[prefix]}_{1}_t {1}_handler;\n".format(names, callback)

  struct += "};\n"
  
  return struct


def generate_func_set_callback_payload(names, machine):
  return "void {0[prefix]}_set_callback_payload({0[machine_type]} *machine, void *payload)\n{{\n" \
         "\tmachine->payload = payload;\n" \
         "}}\n".format(names)


def generate_func_callback_setter(names, machine, callback):
  return "void {0[prefix]}_set_{1}({0[machine_type]} *machine, {0[prefix]}_{1}_t handler)\n" \
         "{{\n" \
         "\tmachine->{1}_handler = handler;\n" \
         "}}\n".format(names, callback)


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



def generate_func_on_entry(names, machine):
  func = "void {0[prefix]}_on_entry({0[machine_type]} *machine)\n" \
         "{{\n" \
         "\tswitch(machine->state)\n" \
         "\t{{\n".format(names)
  
  for state in machine['states']:
    if 'onentry' in state:
      func += "\t\t{0}:\n\t\t\t{1}\n\t\t\tbreak;\n".format(state['name'], state['onentry'])

  func += "\t\tdefault:\n\t\t\treturn;\n"
  func += "\t}\n}\n"
  
  return func


def generate_func_on_exit(names, machine):
  func = "void {0[prefix]}_on_exit({0[machine_type]} *machine)\n" \
         "{{\n" \
         "\tswitch(machine->state)\n" \
         "\t{{\n".format(names)
  
  for state in machine['states']:
    if 'onexit' in state:
      func += "\t\t{0}:\n\t\t\t{1}\n\t\t\tbreak;\n".format(state['name'], state['onexit'])

  func += "\t\tdefault:\n\t\t\treturn;\n"
  func += "\t}\n}\n"
  
  return func

  
def generate_func_active_state(names, machines):
  func = "{0[state_type]} {0[prefix]}_active_state({0[machine_type]} *machine)\n" \
         "{{\n" \
         "\tassert(machine);\n" \
         "\treturn machine->state;\n" \
         "}}\n".format(names)  
  return func


def generate_body(names, machine):
  body = \
    "#include <assert.h>\n" \
    "#include <syslog.h>\n" \
    "#include \"{1}_test.h\"\n\n" \
    .format(
      machine['name'].upper(),  #0
      names['prefix'],          #1
      names['machine_type'],    #2
      names['state_type'],      #3
      names['event_type']       #4
      )

  body += generate_struct_def(names, machine) + "\n"
  body += machine['source'] + "\n"
  
  body += function_create_machine(names, machine) + "\n"  
  body += function_destroy_machine(names, machine) + "\n"  
  
  body += generate_func_set_callback_payload(names, machine) + "\n"
  
  for callback in machine['callbacks']:
    body += generate_func_callback_setter(names, machine, callback) + "\n"

  body += generate_func_statename(names, machine) + "\n"
  body += generate_func_eventname(names, machine) + "\n"  

  body += generate_func_on_entry(names, machine) + "\n"
  body += generate_func_on_exit(names, machine) + "\n"

  body += function_handle_event(names, machine) + "\n"
  
  body += generate_func_active_state(names, machine) + "\n"

  
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
