import xml.etree.ElementTree as ET


def parse_states(state, prefix):
  if prefix == '':
    prefix = state.attrib['id']
    name = prefix
  else:
    name = prefix + '_' + state.attrib['id']

  states = list()  
  cur_state = {'name': name, 'transitions': list()};
  
  for child in state:
    if child.tag == 'onentry':
      cur_state['onentry'] = child.text
    elif child.tag == 'onexit':
      cur_state['onexit'] = child.text
    elif child.tag == 'transition':
      cur_state['transitions'].append(
        (child.attrib['event'], child.attrib['target'])
        )
    elif child.tag == 'state':
      states.extend(parse_states(child, prefix))

  states.append(cur_state)
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
      machine['initial'] = 'ST_' + child.attrib['initial']
      machine['name'] = child.attrib['id'].lower()
      machine['states'] = parse_states(child, 'ST');

  return machine
