import xml.etree.ElementTree as ET


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
