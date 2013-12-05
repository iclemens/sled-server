#
# This script verifies the frequency at which the
# sled server returns position information.
#
# Scenarios:
#  - Single client
#  - Multiple streaming clients start before and end after test
#  - Other clients start at random times within the measurement
#

from sledclient import SledClient
import numpy
import numpy.linalg
import matplotlib.pyplot as pyplot
import math
import scipy
import time
import threading


def collectPositions(client, duration = 5.0):
  samples = list()
  t0 = client.time()

  while client.time() - t0 < duration:
    try:
      ttime = client.t[-1]
      position = numpy.asarray(client.p[-1])[0][0]
      samples.append((ttime, position))
    except:
      pass
    time.sleep(0.001)
  return samples


def createFakeData(duration = 5.0, smpfreq = 1000.0, sinfreq = 1.0):
  t = numpy.linspace(0, duration, 1 + duration * smpfreq)
  p = numpy.sin(2.0 * math.pi * sinfreq * t)
  return zip(t, p)


def connect():
  client = SledClient()
  client.connect("sled", 3375)
  client.startStream()

  # Wait until ready
  while len(client.t) < 1:
    time.sleep(0.01)

  return client


def fitSine(data):
  sinfreq = 1/1.6

  r = numpy.vstack((
    numpy.sin(2.0 * math.pi * sinfreq * data[:, 0]), 
    numpy.cos(2.0 * math.pi * sinfreq * data[:, 0]), 
    numpy.ones((1, data.shape[0]))
  )).T

  sol = numpy.linalg.lstsq(r, data[:, 1])[0]

  Y = numpy.dot(r, sol)
  return numpy.vstack((data[:,0], Y)).T



def runTest():
  client = connect()
  client.sendCommand("Sinusoid Stop")
  # Move to home position
  client.goto(0)
  time.sleep(2.1)

  client.sendCommand("Sinusoid Start 0.15 1.6")
 
  data = collectPositions(client)
  client.sendCommand("Sinusoid Stop")

  #data = createFakeData()
  data = numpy.array(data)

  sine = fitSine(data)

  error = numpy.mean(numpy.abs(data[:, 1] - sine[:, 1]))
  print "Sine error: {}".format(error)

  if error > 0.0005:
    print "THIS ERROR IS TOO BIG!!! NO RELIABLE SINE!!!"

  pyplot.plot(data[:, 0], data[:, 1])
  pyplot.plot(sine[:, 0], sine[:, 1])
  pyplot.show()

  

if __name__ == "__main__":
  runTest()

