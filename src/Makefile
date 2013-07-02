
CPP=g++
CC=gcc

SRC = sled.cc sled_profile.cc interface.cc machines/mch_intf.cc machines/mch_net.cc machines/mch_sdo.cc machines/mch_ds.cc machines/mch_mp.cc
OBJ = $(patsubst %.cc,.obj/%.o,$(SRC))

.obj:
	mkdir .obj

.obj/%.o: %.cc .obj
	$(CPP) -c -g -fPIC -o $@ $<

.obj/%.o: %.c .obj
	$(CC) -c -g -fPIC -o $@ $<

libsled.so: $(OBJ)
	$(CC) -shared -g -Wl,-soname,libsled -o ../libsled.so $(OBJ)
