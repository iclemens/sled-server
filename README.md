sled-server
===========

Sled server that uses RT C3D protocol.

Before you begin
----------------

In order to compile the sled-server, you will need to install the following software:

	sudo apt-get install git
	sudo apt-get install cmake				# Makefile generator
	sudo apt-get install bison flex		# Parser and lexer generators
	sudo apt-get install build-essential	# C compiler
	sudo apt-get install libevent-dev		# Used to wait for file handle activity
	sudo apt-get install libpopt-dev

You will also need to compile the pcan linux driver and userspace library:

	wget http://www.peak-system.com/fileadmin/media/linux/files/peak-linux-driver-7.9.tar.gz
	tar -zxvf peak-linux-driver-7.9.tar.gz
	cd peak-linux-driver-7.9
	make NET=NO_NETDEV_SUPPORT
	sudo make install

Building
--------

	git clone https://github.com/iclemens/sled-server.git
	mkdir build
	cd build
	cmake ../sled-server
	make
	sudo make install

Parts
-----

The server consists of three distinct parts:
* librtc3d: Implements network server and low-level protocol handling.
* libsled: Implements basic CAN master and sled control functions.
* sled-server: Glue that links these two.

Currently everything works in one thread. Using libevent we wait for file handles (either CAN-Bus or TCP-socket) to become ready to read. Libevent then automatically calls an event handler which reads from the file handle, does some buffering (in case of an incomplete message), and executes the command. In addition, we have registered a timer with libevent which periodically sends sled position to all clients that have signed up to receive it.

Major issues
------------

* None of the code is currently thread-safe. This is not required at the moment, because we do not use threads.
* Functions that are only used within the file are NOT declared static!
* We are currently NOT const-correct.

Copyright and license
---------------------

* Copyright (c) 2013 Ivar Clemens
* Copyright (c) 2013 Donders Institute / Radboud University

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
