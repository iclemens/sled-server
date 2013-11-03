import xml.etree.ElementTree as ET

def parse_onentry(onentry):
  return None

def parse_onexit(onexit):
  return None

def parse_transition(transition):
  transition_dct = dict()  
  transition_dct["type"] = "external"
  transition_dct["cond"] = "true"
  transition_dct["event"] = None

  if "event" in transition.attrib:
    transition_dct["event"] = transition.attrib["event"]
  if "target" in transition.attrib:
    transition_dct["target"] = transition.attrib["target"]
  if "cond" in transition.attrib:
    transition_dct["cond"] = transition.attrib["cond"]
  if "type" in transition.attrib:
    transition_dct["type"] = transition.attrib["type"]

  if transition_dct["type"] not in ["internal", "external"]:
    raise "Invalid transition type, must be internal or external"

  return transition_dct

  
def parse_state(state, parallel = False):
  state_dct = dict()
  
  if parallel:
    state_dct["type"] = "parallel"
  else:
    state_dct["type"] = "state"

  if not state.tag == state_dct["type"]:  
    raise "Invalid state tag."

  # None state id required assignment of unique id later.
  if "id" in state.attrib:
    state_dct["id"] = state.attrib["id"]
  else:
    state_dct["id"] = None  

  # Set initial state from attribute
  if "initial" in state.attrib:
    state_dct["initial"] = state.attrib["initial"]

  state_dct["onentry"] = list()
  state_dct["onexit"] = list()

  state_dct["children"] = list()
  state_dct["transitions"] = list()
  
  for child in state:
    if child.tag == "onentry":
      state_dct["onentry"].append(parse_onentry(child))
    elif child.tag == "onexit":
      state_dct["onexit"].append(parse_onexit(child))
    elif child.tag == "transition":    
      state_dct["transitions"].append(parse_transition(child))
    elif child.tag == "initial":
      if parallel:
        raise "Initial state not allowed for parallel state."
      if "initial" in state_dct:
        raise "Initial state can only be set once."
      state_dct["initial"] = child.text
    elif child.tag == 'state':
      child_dct = parse_state(child)
      state_dct["children"].append(child_dct)
    elif child.tag == "parallel":
      child_dct = parse_state(child, True)
      state_dct["children"].append(child_dct)
    elif child.tag == "final":
      if parallel:
        raise "Final state not allowed for parallel state."
      pass
    elif child.tag == "history":
      pass
    elif child.tag == "datamodel":
      pass
    elif child.tag == "invoke":
      pass      
  return state_dct


#
# Reads SCXML data and performs initial validation.
#
def parse_scxml(scxml):
  machine = dict()
  
  if scxml.tag != "icsxml":
    raise "Invalid root tag."
  
  # Read attributes
  if not "version" in scxml.attrib:
    raise "Version attribute not set."
  
  if not scxml.attrib["version"] == "1.0":
    raise "Invalid SCXML version."

  if "datamodel" in scxml.attrib:
    machine["datamodel_type"] = scxml.attrib["datamodel"]
  else:
    machine["datamodel_type" ] = None
  
  if "binding" in scxml.attrib:
    machine["binding"] = scxml.attrib["binding"]
  else:
    machine["binding"] = "early"

  if not machine["binding"] in ["early", "late"]:
    raise "Invalid binary, only early and late allowed."

  # Read children
  machine["states"] = list()
  
  for child in scxml:
    if child.tag == "state":
      state = parse_state(child)
      machine["states"].append(state)
    elif child.tag == "parallel":
      parallel = parse_parallel(child, True)
      machine["states"].append(parallel)
    elif child.tag == "final":
      #machine["finals"]
      pass
    elif child.tag == "datamodel":
      if "datamodel" in machine:
        raise "No more than one datamodel element allowed."
      machine["datamodel"] = list()
    elif child.tag == "script":
      if "script" in machine:
        raise "No more than one script element allowed."
      machine["script"] = list()

  # Set defaults
  if not "datamodel" in machine:
    machine["datamodel"] = list()
  if not "script" in machine:
    machine["script"] = list()

  return machine



#
# If the current state has child states:
#  - Add all states to parent state
#  - Replace all occurences of our name with name of initial state
#  - ? What do we do with our on/entry on/exit events ?
#  - Add of our transitions to our children
#
# Support event="STATENAME.done"
#         event="done.state.STATENAME")
# (if all parallel states are in final
#  Add <final> and <initial>
#  Add <parallel> state (doesn't have initial)
#       Has final
#  Transition can have a body
#



def parse_icsxml(filename):
  tree = ET.parse(filename)
  root = tree.getroot()
  
  return parse_scxml(root)
  
  machine = {
    'header': '',
    'source': '',
    'fields': list(),
    'callbacks': list(),

    'initial': None,
    'states': dict()
    }

  if 'initial' in child.attribs:
    machine['initial'] = child.attribs['initial']

  for child in root:
    if child.tag == 'header':
      machine['header'] += child.text
      
    elif child.tag == 'source':
      machine['source'] += child.text

    elif child.tag == 'datamodel':
      for field in child:
        machine['fields'].append((field.attrib['type'], field.attrib['id']))

    elif child.tag == 'callbacks':
      for callback in child:
        machine['callbacks'].append(callback.attrib['id'])

    elif child.tag == 'initial':
      machine['initial'] = child.text

    elif child.tag == 'state':
      state_dct = parse_state(child)
      state_id = state_dct['id']
      machine['states'][state_id] = state_dct

  # If no initial state is set, use the first one
  if machine['initial'] is None:
    machine['initial'] = machine['states'].keys()[0]

  return machine
