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

Motion profiles
---------------

The sled server requires two motion profiles: a sinusoidal profile at position 0 and a minimum jerk profile at position 2. These profiles need to be generated and then loaded into the drive when first using the sled. To do this, you need access to a Windows computer connected to the S700 drive using either the serial interface or the CAN bus.

First, download the CALCLK utility which converts profile tables in CSV format into SREC. Also download and install the UpgradeTool required to download a new motion profile into the drive.

* http://www.wiki-kollmorgen.eu/wiki/tiki-index.php?page=Tools
* http://www.wiki-kollmorgen.eu/wiki/DanMoBilder/file/tools/Upgrade_Tool_V2.70.exe

Then use the script in tools/profiles to create a new profile table and upload it to the drive.

	python create_profile.py > profiles.txt
	calclk4.exe profiles.txt profiles.tab
	
PLC program
-----------

In addition to motion profile, the PLC program (written in IEC 61131) should also be uploaded to the drive. The program is located in libsled/plc/sinusoid.pl3. Before using the UpgradeTool to download the program into the drive, it too has to be converted into SREC format.

You can use the MacroStar software to perform this conversion:

* http://www.wiki-kollmorgen.eu/wiki/tiki-index.php?page=MacroStar+Software

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

Even though these modules are statically linked, we will keep them as separate libraries for now as this will facilitate testing.

Currently everything works in one thread. Using libevent we wait for file handles (either CAN-Bus or TCP-socket) to become ready to read. Libevent then automatically calls an event handler which reads from the file handle, does some buffering (in case of an incomplete message), and executes the command. In addition, we have registered a timer with libevent which periodically sends sled position to all clients that have signed up to receive it.

Copyright and license
---------------------

* Copyright (c) 2013-2014 Ivar Clemens
* Copyright (c) 2013-2014 Donders Institute / Radboud University Nijmegen

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
