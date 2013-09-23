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
