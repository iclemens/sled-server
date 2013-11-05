
from testclient import SledClient
import time

class TestFrequency():

  client = None
  ctime = list()
  stime = list()
  position = list()


  def positionCallback(self, position, time):
    self.ctime.append(self.client.time())
    self.stime.append(time)
    self.position.append(position[0][0])


  def run(self):
    self.client = SledClient()
    self.client.connect("localhost", 3375)
    self.client.startStream()

    self.client._positionCallback = self.positionCallback
    time.sleep(5)
    sum_interval = 0
    num_samples = len(self.stime)

    self.client.close()

    for i in range(len(self.stime) - 1):
      interval = (self.stime[i + 1] - self.stime[i])
      sum_interval += interval

    avg_frequency = num_samples / sum_interval

    print "Mean interval: {:.2f}ms; Frequency: {:.2f}Hz".format(1000 / avg_frequency, avg_frequency )

    if avg_frequency > 1.1:
      return False
    return True
    


test = TestFrequency()

if not test.run():
  print "Test run failed, frequency not within specification"



time.sleep(100)
