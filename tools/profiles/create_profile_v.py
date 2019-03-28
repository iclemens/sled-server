#!/usr/bin/env python3
from math import cos, pi, sin
from numpy import linspace

# several s-waves, all in normalized units for x (0,0 to 1,1)
def sinusoid(t):
	# normalized sinusoid
	x = .5 - .5*cos(pi*t)
	v = .5*pi*sin(pi*t)
	a = .5*pi**2*cos(pi*t)
	return x, v, a

def min_jerk(npoints):
	# lowest order polynomial that has an s curve with zero velocity and acceleration at start and end
	x = 10 * t**3 -  15 * t**4  +  6 * t**5
	v = 30 * t**2 -  60 * t**3  + 30 * t**4
	a = 60 * t    - 180 * t**2 + 120 * t**3
	return x, v, a

def rsinusoid(t):
	# lowest order polynomial that has an s curve with zero velocity at start and end
	# and zero acceleration at start, and -pi**2/2 acceleration at end
	x =   (40-pi**2)/4 * t**3 +    (pi**2-30)/2 * t**4 +   (24-pi**2)/4  * t**5
	v = 3*(40-pi**2)/4 * t**2 +  4*(pi**2-30)/2 * t**3 +  5*(24-pi**2)/4 * t**4
	a = 6*(40-pi**2)/4 * t    + 12*(pi**2-30)/2 * t**2 + 20*(24-pi**2)/4 * t**3
	return x, v, a


for t in linspace(0, 1, 100):
	print("{:.6f}".format(t), end="")
	print("\t{:.6f}\t{:.6f}\t{:.6f}".format(*sinusoid(t)), end="")
	print("\t{:.6f}\t{:.6f}\t{:.6f}".format(*min_jerk(t)), end="")
	print("\t{:.6f}\t{:.6f}\t{:.6f}".format(*rsinusoid(t)), end="")
	print()
	#gnuplot:
	# plot "tt.dat" u 1:2 w l lw 3 t "sinusoid" , "tt.dat" u 1:5 w l lw 3 t "minjerk", "tt.dat" u 1:8 w l lw 3 t "rsinusoid"
