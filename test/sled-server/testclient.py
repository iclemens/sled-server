#!/usr/bin/python
#
# Modified version of the Python First Principles client that allows
# for better testing of the sled server. This code should not be
# used outside of this test suite.
#
# Copyright (C) 2013 Wilbert van Ham
#

from __future__ import print_function
from collections import deque

import copy
import binascii
import logging
import math
import numpy as np
import socket
import struct
import sys
import threading
import time
import warnings
import weakref


def exitHandler():
  print("exit")

class SledClient:
  "Client for NDI First Principles"

  # package and component types
  pTypes = ['Error', 'Command', 'XML', 'Data', 'Nodata', 'C3D']
  cTypes = ['', '3D', 'Analog', 'Force', '6D', 'Event']  

  _positionCallback = None

  def __init__(self, verbose=0, nBuffer=3):
    # 1 verbose, 2: very verbose
    self.verbose = verbose 

    # number of buffered marker coordinate set 
    if nBuffer >= 3:
      self.nBuffer = nBuffer
    else:
      logging.warning("illegal number for nBuffer: {}, using nBuffer=3".format(nBuffer))

    self.sock = None
    self.thread = None
  
    # This event allows us to start/stop the thread loop
    # which is useful when debugging the sled server.
    self.threadActiveEvent = threading.Event()
    self.threadActiveEvent.set()

    self.stoppingStream = False
    self.win32TimerOffset = time.time()
    time.clock() # start time.clock 

  
  def __del__(self):
    self.close()

  
  # low level communication functions
  def connect(self, host="localhost", port=3375):
    self.host = host
    self.port = port
    print ("opening port :{}".format(port))
    print ("localhost : {}".format(host))
    # Create a socket (SOCK_STREAM means a TCP socket)
    self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # socket without nagling
    self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    # Connect to server and send data
    self.sock.settimeout(3)
    try:
      self.sock.connect((host, port))
    except Exception as e:
      logging.error("ERROR connecting to FP Server: "+str(e))
      raise


  def send(self, body, pType):
    #type  0: error, 1: command, 2: xml, 3: data, 4 nodata, 5: c3d
    # pack length and type as 32 bit big endian.
    head = struct.pack(">I", len(body)+8) + struct.pack(">I", pType)
    if self.verbose:
      print("Sent: Body ({}): #{}#".format(len(body), body))
      #print "Sent: Body ({}): #{}#".format(len(body), binascii.hexlify(body))
    try:
      self.sock.sendall(head+body)
    except Exception as e:
      logging.error("ERROR sending to FP server: " + str(e))
      raise


  def sendHandshake(self):
    head = struct.pack(">I", 0) + struct.pack("BBBB", 1,3,3,6)


  def sendCommand(self, body):
    self.send(body, 1)

    
  def sendXml(self, body):
    self.send(body, 2)


  def receive(self):
    # set pSize, pType and pContent
    try:
      # Receive data from the server and shut down
      # the size given is the buffer size, which is the maximum package size
      self.pSize = struct.unpack(">I", self.sock.recv(4))[0] # package size in bytes
      self.pType = struct.unpack(">I", self.sock.recv(4))[0]
      if self.verbose>1:
        print ("Received ({}, {}): ".format(self.pSize, self.pTypes[self.pType]))
      if self.pSize>8:
        self.pContent = self.sock.recv(self.pSize-8)
      else:
        self.pContent = buffer("", 0)
      return self.pType
    except Exception as e:
      logging.error("ERROR receiving from FP server: " + str(e))
      raise

    
  def close(self):
    self.stopStream()
    if self.thread is not None:
      self.thread.join() #wait for other thread to die
    if self.sock is not None:
      self.sock.close()


  def parse3D(self):
    "Parse 3D data components in the last received package, "
    "return them as n x 3 array and an "
    if self.pType == 3: # data
      cCount = struct.unpack(">I", buffer(self.pContent,0,4))[0]
      pointer = 4
      markerList = []
      for i in range(cCount):
        [cSize, cType, cFrame, cTime] = struct.unpack(">IIIQ", buffer(self.pContent, pointer, 20))
        pointer += 20
        if cType == 1: # 3D
          # number of markers
          [mCount] = struct.unpack(">I", buffer(self.pContent, pointer, 4))
          pointer+=4
          for j in range(mCount):
            [x, y, z, delta] = struct.unpack(">ffff", buffer(self.pContent, pointer, 16))
            pointer+=16
            markerList.append([x, y, z])          
      return (np.matrix(markerList)*1e-3, cTime*1e-6)
    else:
      logging.error("expected 3d data but got packageType: {}".format(self.pType))
      if self.pType==1:
        logging.info("  package: ", self.pContent)
      return np.matrix([])


  def time(self):
    if sys.platform == "win32":
      # on Windows, the highest resolution timer is time.clock()
      # time.time() only has the resolution of the interrupt timer (1-15.6 ms)
      # Note that time.clock does not provide time of day information. 
      # EXPECT DRIFT if you do not have ntp better than the MS version
      return self.win32TimerOffset + time.clock()
    else:
      # on most other platforms, the best timer is time.time()
      return time.time()


  # background thead functions
  def startThread(self):
    """The response time ot the StreamFrames command is used to establish the network delay."""
    # I dont care about warnings that my markers are not moving
    warnings.simplefilter('ignore', np.RankWarning)
    logging.info("starting client thread")

    self.sendCommand("SetByteOrder BigEndian")
    self.receive()
    
    self.sendCommand("StreamFrames FrequencyDivisor:1")
    self.receive()

    # main data retrieval loop
    while not self.stoppingStream:
      if not self.threadActiveEvent.isSet():
        self.threadActiveEvent.wait(None)

      retval = self.receive()

      if retval == 0 or retval == 1:
        continue
      elif retval != 3: # not data
        continue

      (pp, tServer) = self.parse3D() # position of markers and fp server time

      if self._positionCallback is not None:
        self._positionCallback(pp, tServer)

    self.sendCommand("Bye")
    logging.info("stopped")
    self.stoppingStream = False;


  def suspendThread(self):
    self.threadActiveEvent.clear()


  def resumeThread(self):
    self.threadActiveEvent.set()

  
  def startStream(self):
    "Start the background thread that synchronizes with the fp server"
    self.thread = threading.Thread(target = self.startThread)
    self.thread.setDaemon(True)
    self.thread.start()

    
  def stopStream(self):
    "Stop the background thread that synchronizes with the fp server"
    if self.thread is not None and self.thread.isAlive():
      self.stoppingStream = True


