#
# This script verifies the frequency at which the
# sled server returns position information.
#
# Scenarios:
#  - Single client
#  - Multiple streaming clients start before and end after test
#  - Other clients start at random times within the measurement
#

from testclient import SledClient
import time
import threading

class TestFrequency():

  client = None
  ctime = list()
  stime = list()
  position = list()


  def positionCallback(self, position, time):
    self.ctime.append(self.client.time())
    self.stime.append(time)
    self.position.append(position[0][0])


  def run(self, interval):
    self.client = SledClient()
    self.client.connect("sled", 3375)
    self.client.startStream()

    self.ctime = list()
    self.stime = list()
    self.position = list()

    self.client._positionCallback = self.positionCallback
    time.sleep(interval)
    sum_interval = 0
    num_samples = len(self.stime)

    self.client.close()

    for i in range(len(self.stime) - 1):
      interval = (self.stime[i + 1] - self.stime[i])
      sum_interval += interval

    if sum_interval == 0:
			return False

    avg_frequency = num_samples / sum_interval

    print "Mean interval: {:.2f}ms; Frequency: {:.2f}Hz".format(1000 / avg_frequency, avg_frequency )

    if avg_frequency < 990 or avg_frequency > 1010:
      return False
    return True


def simpleClient(start, duration):
  client = SledClient()
  time.sleep(start)
  client.connect("sled", 3375)
  client.startStream()
  time.sleep(duration)
  client.close()

def spawnClient(start, duration):
  thread = threading.Thread(target = lambda: simpleClient(start, duration))
  thread.setDaemon(True)
  thread.start()

test = TestFrequency()

# Run for five seconds
if not test.run(5):
  print "Test run failed, frequency not within specification"

# Spawn clients before measurement and see how far we can go
for nClients in [1, 2, 4, 8, 16, 32, 64, 128, 256]:
  print "Running test with {} concurrent clients".format(nClients)
  for i in range(nClients):
    spawnClient(0, 10)

  time.sleep(3)
  if not test.run(5):
    print " - Test failed"

  time.sleep(5)






