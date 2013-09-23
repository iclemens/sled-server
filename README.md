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

