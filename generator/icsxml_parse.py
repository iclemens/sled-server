import xml.etree.ElementTree as ET

#      machine['initial'] = 'ST_' + child.attrib['initial']
#      machine['name'] = child.attrib['id'].lower()
#      machine['states'] = parse_states(child, 'ST');


def parse_state(state):
  state_dct = dict()  
  state_dct['name'] = state.attrib['id']

  if 'initial' in state.attrib:
    state_dct['initial'] = state.attrib['initial']
  else:
    state_dct['initial'] = None

  state_dct['children'] = dict()
  state_dct['transitions'] = dict()
  state_dct['onentry'] = ''
  state_dct['onexit'] = ''
  
  for child in state:
    if child.tag == 'state':
      child_dct = parse_state(child)
      child_name = child_dct['name']
      state_dct['children'][child_name] = child_dct
    elif child.tag == 'transition':
      event_name = child.attrib['event']
      event_target = child.attrib['target']
      
      if 'type' in child.attrib:
        event_type = child.attrib['type']
      else:
        event_type = 'internal'

      state_dct['transitions'][event_name] = {'target': event_target, 'type': event_type}
    elif child.tag == 'onentry':
      state_dct['onentry'] = child.text
    elif child.tag == 'onexit':
      state_dct['onexit'] = child.text
      
  return state_dct


def parse_states(state, prefix):
  if prefix == '':
    prefix = state.attrib['id']
    name = prefix
  else:
    name = prefix + '_' + state.attrib['id']

  states = list()  
  cur_state = {'name': name, 'transitions': list()};
  
  if 'initial' in state.attrib:
    cur_state['initial'] = state.attrib['initial']
  else:
    cur_state['initial'] = None
  
  for child in state:
    if child.tag == 'onentry':
      cur_state['onentry'] = child.text
    elif child.tag == 'onexit':
      cur_state['onexit'] = child.text
    elif child.tag == 'transition':
      cur_state['transitions'].append(
        (child.attrib['event'], prefix + '_' + child.attrib['target'])
        )
    elif child.tag == 'state':
      states.extend(parse_states(child, prefix))

  if len(states) == 0:
    states.append(cur_state)
  else:
    # Set onentry and onexit event to all child states
    # Add transitions to all child states
    states.append({'name': name, 'link': cur_state['initial']})
    # Make this state a synonym of the initial state
    pass

  return states


def parse_icsxml(filename):
  tree = ET.parse(filename)
  root = tree.getroot()
  
  machine = {
    'header': '',
    'source': '',
    'fields': list(),
    'callbacks': list(),
    'events': list(),
    'states': list()
    }

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

    elif child.tag == 'state':
      machine['states'] = parse_state(child)

  return machine
