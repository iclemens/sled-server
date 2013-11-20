import argparse
import generator
import sys
import xml.etree.ElementTree as ElementTree

parser = argparse.ArgumentParser(
  description = "Builds a C-based state machine from an XML representation.")
parser.add_argument('INPUT_FILE', help = "State machine source (.xml) file")
parser.add_argument('-hdr', dest = 'H_OUTPUT_FILE', help = "Header (.h) output file")
parser.add_argument('-src', dest = 'C_OUTPUT_FILE', help = "Source (.cc) output file")
args = parser.parse_args()

x_filename = args.INPUT_FILE
h_filename = args.H_OUTPUT_FILE
c_filename = args.C_OUTPUT_FILE

try:
  root = ElementTree.parse(x_filename).getroot()
except Exception as e:
  print "Unable to process input file ({}): {}".format(x_filename, e.message)
  sys.exit(1)

if h_filename is not None:
  hfile = open(h_filename, 'w')
  hfile.write(generator.compile_header(root))

if c_filename is not None:
  cfile = open(c_filename, 'w')
  cfile.write(generator.compile_source(root))

