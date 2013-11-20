
import sys
import xml.etree.ElementTree as ElementTree


###################################
# Functions for header generation #
###################################


def enum_definition(name, values):
  enum = "enum {} {{\n".format(name)
  for value in values:
    enum += "\t{},\n".format(value)
  return enum + "};\n\n"


def compile_header(data):
  header = ""
  prefix = data.attrib['prefix']

  header = "// WARNING: Automatically generated file! Do not modify!\n" \
           "#ifndef __{1}_H__\n" \
           "#define __{1}_H__\n\n" \
           "struct {0}_t;\n" \
           "\n".format(prefix, prefix.upper())

  header += enum_definition("{}_event_t".format(prefix), [e.text for e in data.findall('./events/event')])
  header += enum_definition("{}_state_t".format(prefix), [e.text for e in data.findall('./states/state')])

  for element in data.findall("./include"):
    header += element.text + "\n"

  args = ", ".join([e.attrib['type'] + e.text for e in data.findall("./fields/field")])

  header += "{0}_t *{0}_create({1});\n" \
            "void {0}_destroy({0}_t **machine);\n" \
            "{0}_state_t {0}_active_state({0}_t *machine);\n" \
            "void {0}_handle_event({0}_t *machine, {0}_event_t event);\n" \
            "void {0}_set_callback_payload({0}_t *machine, void *payload);\n" \
            "\n".format(prefix, args)

  for element in data.findall("./callbacks/callback"):
    header += "typedef void(*{0}_{1}_handler_t)({0}_t *machine, void *payload);\n" \
              "void {0}_set_{1}_handler({0}_t *machine, {0}_{1}_handler_t handler);\n" \
              "\n".format(prefix, element.text)

  return header + "#endif\n\n"


###################################
# Functions for source generation #
###################################


def function_enum_to_string(fname, ename, values):
  source = "const char *{0}({1} val)\n{{\n" \
           "\tswitch(val) {{\n".format(fname, ename)
  for value in values:
    source += "\t\tcase {0}: return \"{0}\";\n".format(value)
  return source + "\t}\n\treturn \"Value not in enum.\";\n}\n\n"


def function_create(data, forward = False):
  prefix = data.attrib['prefix']
  args = ", ".join([e.attrib['type'] + e.text for e in data.findall("./fields/field")])

  func = "{0}_t *{0}_create({1})".format(prefix, args)
  if forward:
    return func + ";\n"

  func += "\n{\n"
  func += "\t{0}_t *machine = new {0}_t();\n\n".format(prefix)
  func += "\tmachine->state = {0};\n".format(data.findall("initial")[0].text)
  func += "\tmachine->payload = NULL;\n\n"

  for element in data.findall("./fields/field"):
    func += "\tmachine->{0} = {0};\n".format(element.text)
  func += "\n"

  for element in data.findall("./fields/field_decl"):
    if 'init' in element.attrib:
      func += "\tmachine->{0} = {1};\n".format(element.text, element.attrib['init'])
  func += "\n"

  for element in data.findall("./callbacks/callback"):
    func += "\tmachine->{0}_handler = NULL;\n".format(element.text)

  return func + "}\n\n"


# Generates a definition for the state-machine structure
def structure_definition(data):
  prefix = data.attrib['prefix']
  struct = "struct {0}_t {{\n" \
           "\t{0}_state_t state;\n" \
           "\tvoid *payload;\n\n".format(prefix)

  for element in data.findall("./callbacks/callback"):
    struct += "\t{0}_{1}_handler_t {1}_handler;\n".format(prefix, element.text)
  for element in data.findall("./fields/field"):
    struct += "\t{0} {1};\n".format(element.attrib['type'], element.text)
  for element in data.findall("./fields/field_decl"):
    struct += "\t{0} {1};\n".format(element.attrib['type'], element.text)

  return struct + "};\n\n"


def compile_source(data):
  prefix = data.attrib['prefix']

  source = "// WARNING: Automatically generated file! Do not modify!\n" \
           "#include \"{0}.h\"\n" \
           "#include <assert.h>\n" \
           "#include <stdlib.h>\n" \
           "#include <syslog.h>\n" \
           "\n".format(prefix)

  source += structure_definition(data)

  # Forward declarations
  source += "static {0}_state_t {0}_next_state_given_event({0}_t *machine, {0}_event_t event);\n" \
            "static void {0}_on_enter({0}_t *machine);\n" \
            "static void {0}_on_exit({0}_t *machine);\n" \
            "\n".format(prefix)

  source += function_create(data)

  # Destroy function
  source += "void {0}_destroy({0}_t **machine)\n{{\n" \
            "\tfree(*machine);\n" \
            "\t*machine = NULL;\n" \
            "}}\n\n".format(prefix)

  # Active state
  source += "{0}_state_t {0}_active_state({0}_t *machine)\n{{\n" \
            "\tassert(machine);\n" \
            "\treturn machine->state;\n" \
            "}}\n\n".format(prefix)

  # Callback payload
  source += "void {0}_set_callback_payload({0}_t *machine, void *payload)\n{{\n" \
            "\tmachine->payload = payload;\n" \
            "}}\n\n".format(prefix)

  for element in data.findall("./callbacks/callback"):
    source += "void {0}_set_{1}_handler({0}_t *machine, {0}_{1}_handler_t handler)\n{{\n" \
              "\tmachine->{1}_handler = handler;\n" \
              "}}\n\n".format(prefix, element.text)

  source += function_enum_to_string(prefix + "_statename", prefix + "_state_t", [e.text for e in data.findall('./states/state')])
  source += function_enum_to_string(prefix + "_eventname", prefix + "_event_t", [e.text for e in data.findall('./events/event')])
  
  # Handle event
  source += "void {0}_handle_event({0}_t *machine, {0}_event_t event)\n{{\n" \
            "\tassert(machine);\n" \
            "\t{0}_state_t next_state = {0}_next_state_given_event(machine, event);\n" \
            "\tif(!(machine->state == next_state)) {{\n" \
            "\t\t{0}_on_exit(machine);\n" \
            "\t\tmachine->state = next_state;\n" \
            "\t\tsyslog(LOG_DEBUG, \"%s() state changed to %s\",  __FUNCTION__, {0}_statename(machine->state));\n" \
            "\t\t{0}_on_enter(machine);\n" \
            "\t}}\n" \
            "}}\n\n".format(prefix)

  return source



