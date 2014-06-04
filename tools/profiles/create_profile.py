import math

#
# This script writes motion profile tables for
# use in the sled system to standard output.
# These profiles cannot be used directly, but
# have to be converted into SREC format using
# the CALCLK tool provided by Kollmorgen. This
# SREC file then has to be uploaded to the drive
# using the Upgrade Tool.
#
# To download the CALCLK tool:
#  http://www.wiki-kollmorgen.eu/wiki/tiki-index.php?page=Tools
#
# And the Upgrade Tool:
#  http://www.wiki-kollmorgen.eu/ ...
#		... wiki/DanMoBilder/file/tools/Upgrade_Tool_V2.70.exe
#
# To create and upload a profile, do the following:
#  python create_profile.py > profiles.txt
#  calclk4.exe profiles.txt profiles.tab
#


def dump_sinusoid(npoints):
#
# Writes a sinusoidal position profile and the associated 
# acceleration to the standard output.
#
  print "0, "
  for i in range(npoints):
    t = i * math.pi / (npoints - 1)
    s = -math.cos(t) * 2 ** 30 + 2 ** 30
    a = -math.cos(t) * 2 ** 15

    if i == npoints - 1:
      print "{:0.0f},{:0.0f};".format(s, a)
    else:
      print "{:0.0f},{:0.0f}".format(s, a)  


def dump_min_jerk(npoints):
#
# Writes a minimum jerk position profile and the associated
# acceleration profile to standard output.
#
# For more information about the profile, see:
# http://www.shadmehrlab.org/book/minimum_jerk/minimumjerk.htm
#
  print "0, "
  for i in range(npoints):
    t = i / float(npoints - 1)

    s = (10 * t ** 3 + \
        -15 * t ** 4 + \
          6 * t ** 5) * (2 ** 31 - 1)
  
    a = (-60 * t ** 1 + \
         180 * t ** 2 + \
        -120 * t ** 3) * 32767.0 / 5.7735

    if i == npoints - 1:
      print "{:0.0f},{:0.0f};".format(s, a)
    else:
      print "{:0.0f},{:0.0f}".format(s, a)  


# For correct operation, at least profiles 0 and 2 should be present.
dump_sinusoid(2048)
dump_min_jerk(1024)
dump_min_jerk(2048)
